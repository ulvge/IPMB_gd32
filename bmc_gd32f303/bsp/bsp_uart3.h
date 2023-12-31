#ifndef __BSP_UART3_H
#define	__BSP_UART3_H


#include "project_select.h"
#include <stdio.h>
#include <stdbool.h>



#define COM3                        UART3
#define COM3_CLK                    RCU_UART3
#define COM3_TX_PIN                 GPIO_PIN_10
#define COM3_RX_PIN                 GPIO_PIN_11
#define COM3_GPIO_PORT              GPIOC
#define COM3_GPIO_CLK               RCU_GPIOC


void com3_init(void);

#endif /* __BSP_USART2_H */
