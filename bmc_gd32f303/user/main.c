/*!
    \file    main.c
*/

/*
    Copyright (c) 2018, GigaDevice Semiconductor Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdbool.h>
#include "main.h"

#include "OSPort.h"

#include "systick.h"
#include "bsp_uartcomm.h"
#include "bsp_i2c.h"
#include "bsp_gpio.h"

#include "fan/api_fan.h"
#include "adc/api_adc.h"
#include "cpu/api_cpu.h"
#include "utc/api_utc.h"

#include "shell_port.h"
#include "MsgHndlr.h"

#include "jump.h"
#include "bsp_timer.h"
#include "ChassisCtrl.h"
#include "cm_backtrace.h"


void start_task(void *pvParameters);
void fan_task(void *pvParameters);
TaskHandle_t ComTask_Handler;
void msg_handle_task(void *pvParameters);
void adc_sample_task(void *pvParameters);

static void watch_dog_init(void);  
static void debug_config(void);

int g_debugLevel = DBG_INFO;

__IO uint64_t g_utc_time_bmc_firmware_build = 0;

const char *projectInfo =
    "\r\n"
    "********************************************\r\n"
    "************      BMC INFO      ************\r\n"
    "********************************************\r\n"
    "Build:    "__DATE__
    "  "__TIME__
    "\r\n"
    "App Version:  " BMC_VERSION " \r\n"
    "Copyright: (c) HXZY\r\n"
    "********************************************\r\n"
    "\r\n";

__weak void platform_init(void)
{
}
/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    //nvic_vector_table_set(ADDRESS_APP_START, 0);
    bsp_systick_config();
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

    platform_init();

    UART_init();
    GPIO_bspInit();
    LOG_I("%s", projectInfo); 

    /* CmBacktrace initialize */
    cm_backtrace_init("CmBacktrace", HARDWARE_VERSION, BMC_VERSION);

    g_utc_time_bmc_firmware_build = currentSecsSinceEpoch(__DATE__, __TIME__);

    xTaskCreate(start_task, "start", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    watch_dog_init();
    debug_config();
    vTaskStartScheduler();      //prvIdleTask
    while (1)
    {
    }
}

void start_task(void *pvParameters)
{
    uint32_t errCreateTask = 0;
    i2c_int();
    // fan_init();

    taskENTER_CRITICAL();

    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ==
        xTaskCreate(Dev_Task, "dev_task", configMINIMAL_STACK_SIZE* 3, NULL, TASK_PRIO_DEV_HANDLE, NULL)) {
        errCreateTask |= 1;
    }
    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ==
        xTaskCreate(msg_handle_task, "com", configMINIMAL_STACK_SIZE * 2, NULL, TASK_PRIO_MSG_HANDLE, (TaskHandle_t *)&ComTask_Handler)) {
        errCreateTask |= 2;
    }
    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ==
        xTaskCreate(adc_sample_task, "adc_sample", configMINIMAL_STACK_SIZE* 2, NULL, TASK_PRIO_ADC_SAMPLE, NULL)) {
        errCreateTask |= 4;
    }
    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ==
        xTaskCreate(shellTask, "shellTask", 200, &shell, TASK_PRIO_SHELL, NULL)) {
        errCreateTask |= 8;
    }
    if (errCreateTask == 0) {
        LOG_I("create task finished : succeed\r\n");
    } else {
        LOG_I("create task finished : error = %d\r\n", errCreateTask);
    }
    vTaskDelete(NULL);
    taskEXIT_CRITICAL();
}

xQueueHandle g_chassisCtrl_Queue = NULL;
void adc_sample_task(void *pvParameters)
{                                 
	SamllMsgPkt_T msg;
    g_chassisCtrl_Queue = xQueueCreate(1, sizeof(SamllMsgPkt_T));
    while (1)
    {
        //LOG_D("abcde\r\n");
        adc_sample_all();
        if (xQueueReceive(g_chassisCtrl_Queue, &msg, 20) == pdPASS){
            ChassisCtrl(&msg);
        }
    }
}

void msg_handle_task(void *pvParameters)
{
    MsgCoreHndlr(pvParameters);
}

/*!
    \brief     task idle hook functions
    \param[in]  none
    \param[out] none
    \retval     none
*/
void vApplicationIdleHook(void)
{
    static uint32_t lastTick = 0;
    /* reload FWDGT counter */
    fwdgt_counter_reload();
    uint32_t nowTick = GetTickMs();
    if (nowTick - lastTick > 3000) {
        vTaskPrintThreadStatus();
        lastTick = nowTick;
    }
}
/// @brief interrupt FWDGT_IRQHandler
__attribute__((unused)) static void watch_dog_init()
{
    /* check if the system has resumed from FWDGT reset */
    uint32_t resetCause = update_BkpDateRead(MCU_RESET_CAUSE_ADDR_H) << 16;
    resetCause |= update_BkpDateRead(MCU_RESET_CAUSE_ADDR_L);
    common_printfResetCause((rcu_flag_enum)resetCause);

    rcu_all_reset_flag_clear();
    /* confiure FWDGT counter clock: 40KHz(IRC40K) / 64 = 0.625 KHz */
    fwdgt_config(2 * 500, FWDGT_PSC_DIV64);
    /* after 1.6 seconds to generate a reset */
    fwdgt_enable();
}
static void debug_config(void)
{
    /* disable wdg when the mcu is in debug mode */
    dbg_periph_enable(DBG_FWDGT_HOLD);
	
    dbg_periph_enable(DBG_TIMER2_HOLD);
	
    dbg_periph_enable(DBG_TIMER3_HOLD);
}
