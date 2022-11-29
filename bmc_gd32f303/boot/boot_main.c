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

void start_task(void *pvParameters);
						 
static void watch_dog_init(void);
static void debug_config(void);

void Delay_NoSchedue(uint32_t clk);


#define MONITOR_TASK_DELAY_ms 1000
#define RESEND_TIMEOUT (4000 / MONITOR_TASK_DELAY_ms)
#define BOOT_DELAY_MAX (5000 / MONITOR_TASK_DELAY_ms)
int g_debugLevel = DBG_LOG;

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
    "support xmodem(crc&checkSum) and package length 128 Byte only\r\n"
    "When the system starts, you have x seconds to select\r\n"
    "'u' or 'CTRL+C' can stop startup, and prepare to update \r\n"    
    "'q' reset and startup from boot \r\n"
    "'a' When you are updating, you can give up and continue startup the APP system \r\n"
    "\r\n";

__weak void platform_init(void)
{
}
void updateMonitor(void *pvParameters)
{   
	static char sendBuff[100];
	vTaskDelay(MONITOR_TASK_DELAY_ms);
    while (1) {
        g_resendCount++;
        switch (g_UpdatingSM) {
            case UPDATE_SM_INIT:
                if (g_resendCount >= BOOT_DELAY_MAX) {
                    const char *tips_Jump2APP = "jump to APP \n";
					UART_sendDataBlock(USART0, (uint8_t *)tips_Jump2APP, strlen(tips_Jump2APP));
                    JumpToAPP();
                }else {                     
					sprintf(sendBuff, "jump to APP :countdown = %d s\r\n", (BOOT_DELAY_MAX - g_resendCount));	
					UART_sendDataBlock(USART0, (uint8_t *)sendBuff, strlen(sendBuff));
					//LOG_I("jump to APP :countdown = %d s\r\n", (BOOT_DELAY_MAX - g_resendCount));
				}
                break;
            case UPDATE_SM_ERROR_TRYAGAIN:
                if (g_resendCount >= RESEND_TIMEOUT) {
                    boot_UartSendByte(XMODEM_NAK);
                }
                break;
            case UPDATE_SM_START:
                if (g_resendCount % (2000 / MONITOR_TASK_DELAY_ms) == 0) {
                    g_xmodemIsCheckTpyeCrc = !g_xmodemIsCheckTpyeCrc;
                }
                if (g_xmodemIsCheckTpyeCrc) {
                    boot_UartSendByte(XMODEM_HANDSHAKECRC);
                } else  {
                    boot_UartSendByte(XMODEM_NAK);
                }
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

__attribute__((unused)) static void SPC_config(void)
{
    if (RESET == ob_spc_get()){
        fmc_unlock();
        ob_unlock();
        while (FMC_BUSY == ob_security_protection_config(FMC_USPC))
        {
            ;
        }
        ob_lock();
        fmc_lock();
        NVIC_SystemReset();
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
    //SPC_config();
    bsp_systick_config();
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

    platform_init();

    UART_init();
    UART_sendDataBlock(USART0, (uint8_t *)projectInfo, strlen(projectInfo));
    UART_sendDataBlock(USART0, (uint8_t *)g_bootUsage, strlen(g_bootUsage));

    watch_dog_init();
    debug_config();

    xTaskCreate(updateMonitor, "updateMonitor", configMINIMAL_STACK_SIZE * 2, NULL, 25, NULL);
    xTaskCreate(updateTask, "update", configMINIMAL_STACK_SIZE * 2, NULL, 20, NULL);
    vTaskStartScheduler();
    while (1) {
        LOG_I("vTaskStartScheduler  error");
    }
}

/*!
    \brief     task idle hook functions
    \param[in]  none
    \param[out] none
    \retval     none
*/
void vApplicationIdleHook(void)
{
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
