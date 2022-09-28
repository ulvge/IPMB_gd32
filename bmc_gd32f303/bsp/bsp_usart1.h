#ifndef __BSP_USART1_H
#define	__BSP_USART1_H


#include "project_select.h"
#include <stdio.h>
#include <stdbool.h>

#define UART1_REMAP

#ifdef UART1_REMAP

#define COM1                        USART1
#define COM1_CLK                    RCU_USART1
#define COM1_TX_PIN                 GPIO_PIN_5
#define COM1_RX_PIN                 GPIO_PIN_6
#define COM1_GPIO_PORT              GPIOD
#define COM1_GPIO_CLK               RCU_GPIOD

#else

#define COM1                        USART1
#define COM1_CLK                    RCU_USART1
#define COM1_TX_PIN                 GPIO_PIN_2
#define COM1_RX_PIN                 GPIO_PIN_3
#define COM1_GPIO_PORT              GPIOA
#define COM1_GPIO_CLK               RCU_GPIOA

#endif


void com1_init(void);

#endif /* __BSP_USART2_H */
