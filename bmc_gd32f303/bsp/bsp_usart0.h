#ifndef __BSP_USART0_H
#define	__BSP_USART0_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "project_select.h"
#include <stdio.h>
#include <stdbool.h>  
#include <bsp_uartcomm.h>


#define COM0                        USART0
#define COM0_CLK                    RCU_USART0
#define COM0_TX_PIN                 GPIO_PIN_9
#define COM0_RX_PIN                 GPIO_PIN_10
#define COM0_GPIO_PORT              GPIOA
#define COM0_GPIO_CLK               RCU_GPIOA

					 
void UART0_init(void);

int fgetc(FILE *f);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_USART1_H */
