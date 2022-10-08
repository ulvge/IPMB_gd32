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

#include "bsp_uart3.h"
					  
#define    BMC_VERSION    "1.0.1"

	#define DEBUG_UART_PERIPH    USART1
	#define IPMI_UART_PERIPH    USART1
	
	#define USE_UART1_AS_IPMI  1 

	//#define USE_I2C0_AS_IPMB 0
	#define USE_I2C1_AS_IPMB 1
	// #define FATFS_ENABLE    

	#define  ft_uart_write(x, len)    UART_sendData(USART1, x, len)
	

#define  CPU_IntDisable()           { __set_PRIMASK(0xFFFF); }	/* Interrupt Disable */
#define  CPU_IntEnable()            { __set_PRIMASK(0x0000); }	/* Interrupt Enable  */

//#define  CPU_IntDisable()           { __set_PRIMASK(1); }	
//#define  CPU_IntEnable()            { __set_PRIMASK(0); }

//#define USE_DHCP       /* enable DHCP, if disabled static address is used */

//#define USE_ENET_INTERRUPT
//#define TIMEOUT_CHECK_USE_LWIP

/* MAC address: MAC_ADDR0:MAC_ADDR1:MAC_ADDR2:MAC_ADDR3:MAC_ADDR4:MAC_ADDR5 */

extern unsigned char MAC_ADDR0;
extern unsigned char MAC_ADDR1;
extern unsigned char MAC_ADDR2;
extern unsigned char MAC_ADDR3;
extern unsigned char MAC_ADDR4;
extern unsigned char MAC_ADDR5;

/*
#define MAC_ADDR0   2
#define MAC_ADDR1   0xA
#define MAC_ADDR2   0xF
#define MAC_ADDR3   0xE
#define MAC_ADDR4   0xD
#define MAC_ADDR5   6
*/
/* static IP address: IP_ADDR0.IP_ADDR1.IP_ADDR2.IP_ADDR3 */

extern unsigned char IP_ADDR0;
extern unsigned char IP_ADDR1;
extern unsigned char IP_ADDR2;
extern unsigned char IP_ADDR3;

/* remote IP address: IP_S_ADDR0.IP_S_ADDR1.IP_S_ADDR2.IP_S_ADDR3 */

#define IP_S_ADDR0   192
#define IP_S_ADDR1   168
#define IP_S_ADDR2   2
#define IP_S_ADDR3   88
  
/* net mask */

#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

/* gateway address */
extern unsigned char  GW_ADDR0;
extern unsigned char  GW_ADDR1;
extern unsigned char  GW_ADDR2;
extern unsigned char  GW_ADDR3; 

#define MSG_I2C_BIT   0x00000001
#define MSG_UART_BIT  0x00000010

typedef enum 
{ 
    MSG_SRC_I2C = 0,
    MSG_SRC_UART = 1
}IPMI_MSG_SRC;

/* function declarations */
/* updates the system local time */
void time_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */

