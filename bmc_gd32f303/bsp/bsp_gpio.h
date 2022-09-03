#ifndef __BSP_GPIO_H_
#define	__BSP_GPIO_H_


#include "gd32f20x.h"
#include <stdbool.h>
#include "main.h"


#define GA0_PIN                         GPIO_PIN_0
#define GA0_GPIO_PORT                   GPIOG
#define GA0_GPIO_CLK                    RCU_GPIOG

#define GA1_PIN                         GPIO_PIN_1
#define GA1_GPIO_PORT                   GPIOG
#define GA1_GPIO_CLK                    RCU_GPIOG

#define GA2_PIN                         GPIO_PIN_2
#define GA2_GPIO_PORT                   GPIOG
#define GA2_GPIO_CLK                    RCU_GPIOG

#define GA3_PIN                         GPIO_PIN_3
#define GA3_GPIO_PORT                   GPIOG
#define GA3_GPIO_CLK                    RCU_GPIOG

#define GA4_PIN                         GPIO_PIN_4
#define GA4_GPIO_PORT                   GPIOG
#define GA4_GPIO_CLK                    RCU_GPIOG

#define GAP_PIN                         GPIO_PIN_5
#define GAP_GPIO_PORT                   GPIOG
#define GAP_GPIO_CLK                    RCU_GPIOG

// test gpio
#define TEST_GPIO_PIN                    GPIO_PIN_9
#define TEST_GPIO_PORT                   GPIOE
#define TEST_GPIO_CLK                    RCU_GPIOE

  
void      addr_gpio_init     (void);
uint8_t   get_board_addr     (void);

void      test_gpio_init     (void);
void      test_gpio_set      (bool status);


#endif /* __BSP_GPIO_H_ */
