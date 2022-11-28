/*!
    \file  gd32f20x_it.c
    \brief interrupt service routines

    \version 2015-07-15, V1.0.0, demo for GD32F20x
    \version 2017-06-05, V2.0.0, demo for GD32F20x
    \version 2018-10-31, V2.1.0, demo for GD32F20x
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
#include <string.h>
#include "gd32f30x_it.h"
#include "main.h"
#include "systick.h"
#include "FreeRTOS.h"
#include "task.h" 
#include "api_cpu.h"
#include "bsp_uartcomm.h"

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
}


static char faultBuf[90];
extern const char *projectInfo;
/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
	static INT32U *r_msp;
	r_msp = (INT32U *)__get_PSP();
	
	//_lr = (unsigned long)sp[5];
	//_pc = (unsigned long)sp[6];                                              
	sprintf(faultBuf + strlen(faultBuf), "\n\n");                                 
	sprintf(faultBuf + strlen(faultBuf), ">> HardFault !!!  prepare reset\n");
	sprintf(faultBuf + strlen(faultBuf), ">> lr = 0x%08x\n", *(r_msp+5)); // seek behind
	sprintf(faultBuf + strlen(faultBuf), ">> pc = 0x%08x\n", *(r_msp+6));                            
	sprintf(faultBuf + strlen(faultBuf), "\n\n");     
	
	UART_sendDataBlock(DEBUG_UART_PERIPH, (uint8_t *)faultBuf, strlen(faultBuf));
										
	UART_sendDataBlock(DEBUG_UART_PERIPH, (uint8_t *)projectInfo, strlen(projectInfo));  
	
    /* if Hard Fault exception occurs, go to infinite loop */
    while(1){
	  //NVIC_SystemReset();	
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while (1);
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while (1);
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while (1);
}

/*!
    \brief      this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
//void SVC_Handler(void)
//{
//}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
}

/*!
    \brief      this function handles PendSV exception
    \param[in]  none
    \param[out] none


    \retval     none
*/
//void PendSV_Handler(void)
//{
//}

/*!
    \brief      this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
extern void xPortSysTickHandler(void);
void SysTick_Handler(void)
{
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//系统已经运行
	{
		xPortSysTickHandler();	
	}
}

/*!
    \brief      this function handles ethernet interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ENET_IRQHandler(void)
{
    
}

