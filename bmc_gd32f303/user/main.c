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
#include "tmp/api_tmp.h"
#include "cpu/api_cpu.h"
#include "utc/api_utc.h"

#include "shell_port.h"

#include "MsgHndlr.h"

#include "update/update.h"
#include "bsp_timer.h"
#include "ChassisCtrl.h"
#include "mac5023.h"

#define FAN_TASK_PRIO 22
#define TEST_TASK_PRIO 9
#define COM_TASK_PRIO 21
#define LANIFC_TASK_PRIO 23
#define DEV_TASK_PRIO 25

void start_task(void *pvParameters);
void fan_task(void *pvParameters);
TaskHandle_t ComTask_Handler;
void com_task(void *pvParameters);
void misc_task(void *pvParameters);

static void watch_dog_init(void);  
static void debug_config(void);

int g_debugLevel = DBG_INFO;

__IO uint32_t g_localtime = 0; /* for creating a time reference incremented by 10ms */
__IO uint64_t g_utc_time_bmc_firmware_build = 0;
__IO uint16_t g_bmc_firmware_version = 0;

const char *projectInfo =
    "\r\n"
    "********************************************\r\n"
    "************      BMC INFO      ************\r\n"
    "********************************************\r\n"
    "Build:    "__DATE__
    "  "__TIME__
    "\r\n"
    "Version:  " BMC_VERSION " \r\n"
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
    // nvic_vector_table_set(NVIC_VECTTAB_FLASH, 0x16000);
    bsp_systick_config();
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

    platform_init();

    UART_init();
    GPIO_bspInit();
    LOG_I("%s", projectInfo);
    g_utc_time_bmc_firmware_build = currentSecsSinceEpoch(__DATE__, __TIME__);
    g_bmc_firmware_version = GetBmcFirmwareVersion(BMC_VERSION);

    //timer_config_init();
    xTaskCreate(start_task, "start", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    xTaskCreate(misc_task, "misc", configMINIMAL_STACK_SIZE, NULL, 26, NULL);
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
    //fan_init();

#ifdef FATFS_ENABLE
    fatfs_init();
#endif

    taskENTER_CRITICAL();

   if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY == 
       xTaskCreate(Dev_Task, "dev_task", configMINIMAL_STACK_SIZE, NULL, DEV_TASK_PRIO, NULL)) {
       errCreateTask |= 1;
   }
   if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY == 
       xTaskCreate(com_task, "com", configMINIMAL_STACK_SIZE * 2, NULL, COM_TASK_PRIO, (TaskHandle_t *)&ComTask_Handler)) {
       errCreateTask |= 1;
   }

    // xTaskCreate(updateTask, "updateTask", configMINIMAL_STACK_SIZE*3, NULL, 18, NULL);

    // if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY == 
    //     xTaskCreate(tmpSampleTask, "tmpSampleTask", configMINIMAL_STACK_SIZE * 2, NULL, 12, NULL)) {
    //     errCreateTask |= 2;
    // }

//    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY == 
//        xTaskCreate(cpuGetInfoTask, "cpu", configMINIMAL_STACK_SIZE * 2, NULL, 11, NULL)) {
//        errCreateTask |= 4;
//    }
    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY == 
        xTaskCreate(shellTask, "shellTask", 300, &shell, 2, NULL)) {
        errCreateTask |= 8;
    }
	if (errCreateTask == 0){
		LOG_I("create task finished : succeed\r\n");
	} else {
		LOG_I("create task finished : error = %d\r\n", errCreateTask);
	}
    vTaskDelete(NULL);
    taskEXIT_CRITICAL();
}

xQueueHandle g_chassisCtrl_Queue = NULL;
void misc_task(void *pvParameters)
{                                 
	SamllMsgPkt_T msg;
    g_chassisCtrl_Queue = xQueueCreate(2, sizeof(SamllMsgPkt_T));
    while (1)
    {
        //LOG_D("abcde\r\n");
        adc_sample_all();
        if (xQueueReceive(g_chassisCtrl_Queue, &msg, 20) == pdPASS){
            ChassisCtrl(&msg);
        }
    }
}

void com_task(void *pvParameters)
{
    MsgCoreHndlr(pvParameters);
}

/*!
    \brief      updates the system local time
    \param[in]  none
    \param[out] none
    \retval     none
*/
void time_update(void)
{
    g_localtime += portTICK_PERIOD_MS;
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
    if (nowTick - lastTick > 1000) {
        vTaskPrintThreadStatus();
        lastTick = nowTick;
    }
}
/// @brief interrupt FWDGT_IRQHandler
__attribute__((unused)) static void watch_dog_init()
{
    /* check if the system has resumed from FWDGT reset */
    if (SET == rcu_flag_get(RCU_FLAG_FWDGTRST))
    {
        LOG_W("system reset reason: FWDG\n");
    }
    else if (SET == rcu_flag_get(RCU_FLAG_PORRST))
    {
        LOG_W("system reset reason: power on\n");
    }
    else if (SET == rcu_flag_get(RCU_FLAG_SWRST))
    {
        LOG_W("system reset reason: soft\n");
    }
    else if (SET == rcu_flag_get(RCU_FLAG_EPRST))
    {
        LOG_W("system reset reason: external PIN\n");
    }
	else {
        LOG_W("system reset reason: unkown\n");
	}
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
