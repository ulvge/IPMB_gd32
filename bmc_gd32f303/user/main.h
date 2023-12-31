/*!
    \file    main.h
    \brief   the header file of main

    \version 2014-12-26, V1.0.0, firmware for GD32F10x
    \version 2017-06-20, V2.0.0, firmware for GD32F10x
    \version 2018-07-31, V2.1.0, firmware for GD32F10x
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

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

#define    HARDWARE_VERSION     "1.0"
#define    BOOT_VERSION         "1.1"
#define    BMC_VERSION          "1.3"

#ifdef BOOTLOADER
extern unsigned int g_bootDebugUartPeriph;
#define DEBUG_UART_PERIPH    g_bootDebugUartPeriph
#else
#define DEBUG_UART_PERIPH    USART0
#endif

#define IPMI_UART_PERIPH    USART0

#define CPU_UART_PERIPH    USART1

#define TASK_PRIO_ADC_SAMPLE            5
#define TASK_PRIO_SHELL                 13
#define TASK_PRIO_DEV_HANDLE            15
#define TASK_PRIO_UPLOAD                20
#define TASK_PRIO_UPDATE_DEV            21

#define TASK_PRIO_MSG_HANDLE            22
#define TASK_PRIO_MSG_RESPONSE_HANDLE   24


#define  CPU_IntDisable()           { __set_PRIMASK(0xFFFF); }	/* Interrupt Disable */
#define  CPU_IntEnable()            { __set_PRIMASK(0x0000); }	/* Interrupt Enable  */

extern int g_debugLevel;
/* function declarations */
/* updates the system local time */
void time_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */

