#ifndef __BSP_UART7_H
#define	__BSP_UART7_H


#include "gd32f20x.h"
#include <stdio.h>
#include <stdbool.h>



#define COM7                        UART7
#define COM7_CLK                    RCU_UART7
#define COM7_TX_PIN                 GPIO_PIN_1
#define COM7_RX_PIN                 GPIO_PIN_0
#define COM7_GPIO_PORT              GPIOE
#define COM7_GPIO_CLK               RCU_GPIOE




void com7_init(void);

void uart7_send_dat( uint8_t *str, uint16_t len);
void uart7_send_string( char *str);

bool uart7_get_data(uint8_t *p_buffer, uint32_t *len);


#endif /* __BSP_USART2_H */
