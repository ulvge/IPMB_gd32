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

#include "main.h"
#include <stdbool.h>
#include <stdio.h>

#include "OSPort.h"

#include "bsp_uartcomm.h"
#include "systick.h"

#include "boot_update.h"
#include "bsp_timer.h"
#include "tools.h"
#include "update/jump.h"

#define FAN_TASK_PRIO 22
#define TEST_TASK_PRIO 9
#define COM_TASK_PRIO 21
#define LANIFC_TASK_PRIO 23
#define DEV_TASK_PRIO 25

TaskHandle_t updateMonitorHandle;

void start_task(void *pvParameters);

xQueueHandle CPU_recvDatMsg_Queue = NULL;

static void watch_dog_init(void);
static void debug_config(void);

void Delay_NoSchedue(uint32_t clk);


#define MONITOR_TASK_DELAY_ms 1000
#define RESEND_TIMEOUT (4000 / MONITOR_TASK_DELAY_ms)
#define BOOT_DELAY_MAX (5000 / MONITOR_TASK_DELAY_ms)
int g_debugLevel = DBG_LOG;

__IO uint32_t g_localtime = 0; /* for creating a time reference incremented by 10ms */

const char *projectInfo =
    "\r\n"
    "********************************************\r\n"
    "************      bootloader      ************\r\n"
    "********************************************\r\n"
    "Build:    "__DATE__
    "  "__TIME__
    "\r\n"
    "Copyright: (c) HXZY\r\n"
    "********************************************\r\n"
    "\r\n";

static const char *g_bootUsage =
    "\r\n"
    "** boot help  ************\r\n"
    "support xmodem and package length 128 Byte only\r\n"
    "When the system starts, you have 3 seconds to select\r\n"
    "'u' or 'CTRL+C' can stop startup, and prepare to update \r\n"
    "'a' When you are updating, you can give up and continue startup the APP system \r\n"
    "\r\n";

__weak void platform_init(void)
{
}
void updateMonitor(void *pvParameters)
{   
	vTaskDelay(MONITOR_TASK_DELAY_ms);
    while (1) {
        g_resendCount++;
        switch (g_UpdatingSM) {
            case UPDATE_SM_INIT:
                if (g_resendCount >= BOOT_DELAY_MAX) {
                    LOG_I("jump to APP \n");
                    JumpToAPP();
                }
                LOG_I("jump to APP :countdown = %d s\r\n", (BOOT_DELAY_MAX - g_resendCount));
                break;
            case UPDATE_SM_ERROR_TRYAGAIN:
                if (g_resendCount >= RESEND_TIMEOUT) {
                    boot_UartSendByte(XMODEM_NAK);
                }
                break;
            case UPDATE_SM_START:
                boot_UartSendByte(XMODEM_HANDSHAKE);
                break;
            case UPDATE_SM_FINISHED:
                JumpToAPP();
                break;
            case UPDATE_SM_CANCEL:
                NVIC_SystemReset();
                break;
            case UPDATE_SM_PROGRAMING:
                if (g_resendCount >= RESEND_TIMEOUT) {
                    boot_UartSendByte(XMODEM_ACK);
                }
            default:
                break;
        }
        vTaskDelay(MONITOR_TASK_DELAY_ms);
    }
}

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    UINT32 count = 0;
    bsp_systick_config();
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

    platform_init();

    UART_init();
    UART_sendDataBlock(DEBUG_UART_PERIPH, (uint8_t *)projectInfo, strlen(projectInfo));
    UART_sendDataBlock(DEBUG_UART_PERIPH, (uint8_t *)g_bootUsage, strlen(g_bootUsage));

    watch_dog_init();
    debug_config();

    xTaskCreate(updateMonitor, "updateMonitor", configMINIMAL_STACK_SIZE * 2, NULL, 25, &updateMonitorHandle);
    xTaskCreate(updateTask, "update", configMINIMAL_STACK_SIZE * 2, NULL, 20, NULL);
    vTaskStartScheduler();
    while (1) {
        LOG_I("vTaskStartScheduler  error");
    }
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
}
/// @brief interrupt FWDGT_IRQHandler
__attribute__((unused)) static void watch_dog_init()
{
    /* check if the system has resumed from FWDGT reset */
    if (SET == rcu_flag_get(RCU_FLAG_FWDGTRST)) {
        LOG_W("system reset reason: FWDG\n");
    } else if (SET == rcu_flag_get(RCU_FLAG_WWDGTRST)) {
        LOG_W("system reset reason: WWDGT\n");
    } else if (SET == rcu_flag_get(RCU_FLAG_PORRST)) {
        LOG_W("system reset reason: power on\n");
    } else if (SET == rcu_flag_get(RCU_FLAG_SWRST)) {
        LOG_W("system reset reason: soft\n");
    } else if (SET == rcu_flag_get(RCU_FLAG_EPRST)) {
        LOG_W("system reset reason: external PIN\n");
    } else if (SET == rcu_flag_get(RCU_FLAG_LPRST)) {
        LOG_W("system reset reason: low-power reset\n");
    } else {
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

void Delay_NoSchedue(uint32_t clk)
{
    for (uint32_t i = 0; i < clk; i++) {
        ;
    }
}