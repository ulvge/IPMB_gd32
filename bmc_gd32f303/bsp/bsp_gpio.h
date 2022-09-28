#ifndef __BSP_GPIO_H_
#define	__BSP_GPIO_H_


#include "project_select.h"
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

typedef enum 
{ 
    GPIO_OUT_LED_RED = 0,
    GPIO_OUT_LED_GREEN,
    GPIO_OUT_CPU_POWER_ON,
    GPIO_OUT_CPU_POWER_OFF,
    GPIO_OUT_CPU_RESET,
    GPIO_OUT_BMC_POWER_ON_FINISHED,

    GPIO_IN_GAP0,
    GPIO_IN_GAP1,
    GPIO_IN_GAP2,
    GPIO_IN_GAP3,
    GPIO_IN_GAP4,
    GPIO_IN_GAP5,
    GPIO_PIN_MAX
}BMC_GPIO_enum;

typedef struct {
    BMC_GPIO_enum     	alias;
    uint32_t      		gpioPort;
    uint32_t      		pin;
    rcu_periph_enum     gpioClk;
    uint8_t             pinMode;
    uint8_t             pinSpeed;
} GPIOConfig;
  
void      gpio_bspInit     (void);
uint8_t   get_board_addr     (void);

FlagStatus gpio_getPinStatus(BMC_GPIO_enum alias);
void gpio_setPinStatus(BMC_GPIO_enum alias, bool status);

#endif /* __BSP_GPIO_H_ */
