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
#include "jump.h" 


#define MONITOR_TASK_DELAY_ms 1000
#define RESEND_HANDSHAKE 1000
#define RESEND_TIMEOUT 3000
#define BOOT_DELAY_DEFAULT 2000
#define BOOT_DELAY_RESET_FROM_APP (2* 60 * 1000)

int g_debugLevel = DBG_LOG;
UINT32 g_bootDebugUartPeriph = USART0;
TaskHandle_t updateMonitorHandle;

void start_task(void *pvParameters);

static void watch_dog_init(void);
static void debug_config(void);

void Delay_NoSchedue(uint32_t clk);

const char *projectInfo =
    "\r\n"
    "********************************************\r\n"
    "************      bootloader      ************\r\n"
    "********************************************\r\n"
    "Build:    "__DATE__
    "  "__TIME__
    "\r\n"    
    "boot Version:  " BOOT_VERSION " \r\n"
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
    "'a' When you are booting, you can give up wait and directly start the APP system \r\n"
    "\r\n";

__weak void platform_init(void)
{
}
void boot_setPrintUartPeriph(UINT32 periph)
{
    g_bootDebugUartPeriph = periph;
}

void updateMonitor(void *pvParameters)
{
    UINT32 jumpToAPPMaxDelay = BOOT_DELAY_DEFAULT;

    if (update_BkpDateRead(APP_WANTTO_UPDATE_KEYS_ADDR) == APP_WANTTO_UPDATE_KEYS) {
        jumpToAPPMaxDelay = BOOT_DELAY_RESET_FROM_APP;  // Avoid misoperation, still need to manually start
    }
    if (update_BkpDateRead(I2C_UPDATE_KEYS_ADDR) == I2C_UPDATE_KEYS) {
        g_UpdatingSM = UPDATE_SM_START; // auto start
        boot_i2c_init();
    }

    vTaskDelay(100);
    while (1) {
        vTaskDelay(MONITOR_TASK_DELAY_ms);
        g_resendCount++;
        if ((MONITOR_TASK_DELAY_ms > 1000) || (g_resendCount % (1000 / MONITOR_TASK_DELAY_ms)) != 0) {
            continue;
        }
        switch (g_UpdatingSM) {
            case UPDATE_SM_INIT:
                if ((g_resendCount * MONITOR_TASK_DELAY_ms) >= jumpToAPPMaxDelay) {
                    LOG_I("jump to APP \r\n");
                    update_BkpDateWrite(APP_WANTTO_UPDATE_KEYS_ADDR, 0);
                    update_BkpDateWrite(I2C_UPDATE_KEYS_ADDR, 0);
                    update_JumpToRun(ADDRESS_APP_START);
                }else {
                    LOG_I("jump to APP :countdown = %d s\r\n", 
                        (jumpToAPPMaxDelay - (g_resendCount * MONITOR_TASK_DELAY_ms)) / 1000);
                }
                break;
            case UPDATE_SM_ERROR_TRYAGAIN:
                if ((g_resendCount * MONITOR_TASK_DELAY_ms) >= RESEND_TIMEOUT) {
                    g_resendCount = 0;
                    boot_sendMsg2Dev(XMODEM_NAK);
                }
                break;
            case UPDATE_SM_PROGRAMING:
                if ((g_resendCount * MONITOR_TASK_DELAY_ms) >= RESEND_TIMEOUT) {
                    g_resendCount = 0;
                    boot_sendMsg2Dev(XMODEM_ACK);
                }
                break;
            case UPDATE_SM_START:
                if ((g_resendCount * MONITOR_TASK_DELAY_ms) >= RESEND_HANDSHAKE) { // switch per 2 sec
                    g_resendCount = 0;
                    g_xmodemIsCheckTpyeCrc = !g_xmodemIsCheckTpyeCrc;
                }
                if (g_xmodemIsCheckTpyeCrc) {
                    boot_sendMsg2Dev(XMODEM_HANDSHAKECRC);
                } else  {
                    boot_sendMsg2Dev(XMODEM_NAK);
                }
                break;
            case UPDATE_SM_FINISHED:
                vTaskDelay(2); // print over
                update_BkpDateWrite(APP_WANTTO_UPDATE_KEYS_ADDR, 0);
                update_BkpDateWrite(I2C_UPDATE_KEYS_ADDR, 0);
                update_JumpToRun(ADDRESS_APP_START);
                break;
            case UPDATE_SM_CANCEL:
                NVIC_SystemReset();
                break;
            default:
                break;
        }
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

    boot_setPrintUartPeriph(USART0);
    UART_init();
    UART_sendDataBlock(USART0, (uint8_t *)projectInfo, strlen(projectInfo));
    UART_sendDataBlock(USART0, (uint8_t *)g_bootUsage, strlen(g_bootUsage));

    watch_dog_init();
    debug_config();
    xTaskCreate(updateMonitor, "updateMonitor", configMINIMAL_STACK_SIZE * 2, NULL, 25, &updateMonitorHandle);
    xTaskCreate(boot_updateTask, "update", configMINIMAL_STACK_SIZE * 2, NULL, 20, NULL);
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
    rcu_flag_enum resetCause;
    /* check if the system has resumed from FWDGT reset */
    if (SET == rcu_flag_get(RCU_FLAG_FWDGTRST)) {
        resetCause = RCU_FLAG_FWDGTRST;
    } else if (SET == rcu_flag_get(RCU_FLAG_WWDGTRST)) {
        resetCause = RCU_FLAG_WWDGTRST;
    } else if (SET == rcu_flag_get(RCU_FLAG_PORRST)) {
        resetCause = RCU_FLAG_PORRST;
    } else if (SET == rcu_flag_get(RCU_FLAG_SWRST)) {
        resetCause = RCU_FLAG_SWRST;
    } else if (SET == rcu_flag_get(RCU_FLAG_EPRST)) {
        resetCause = RCU_FLAG_EPRST;
    } else if (SET == rcu_flag_get(RCU_FLAG_LPRST)) {
        resetCause = RCU_FLAG_LPRST;
    } else {
        resetCause = (rcu_flag_enum)NULL;
    }
    common_printfResetCause(resetCause);
    update_BkpDateWrite(MCU_RESET_CAUSE_ADDR_H, (resetCause >> 16)&0xffff);
    update_BkpDateWrite(MCU_RESET_CAUSE_ADDR_L, resetCause & 0xffff);
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
