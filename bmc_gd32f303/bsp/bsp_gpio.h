#ifndef __BSP_GPIO_H_
#define	__BSP_GPIO_H_


#include "project_select.h"
#include <stdbool.h>
#include "main.h"     
#include "api_subdevices.h"


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
    GPIO_CPLD_MCU_1 = 0,
    GPIO_CPLD_MCU_2,
    GPIO_CPLD_MCU_3,
    GPIO_CPLD_MCU_4,
    GPIO_CPLD_MCU_5,
    GPIO_CPLD_MCU_6,
    GPIO_CPLD_MCU_7,
    GPIO_CPLD_MCU_8,
    GPIO_CPLD_MCU_9,
    GPIO_CPLD_MCU_10,
    GPIO_CPLD_MCU_11,
    GPIO_CPLD_MCU_12,
    GPIO_CPLD_MCU_13,

    GPIO_OUT_LED_RED,
    GPIO_OUT_LED_GREEN,
    GPIO_OUT_CPU_POWER_ON,
    GPIO_OUT_CPU_POWER_OFF,
    GPIO_OUT_CPU_RESET,
    GPIO_OUT_BMC_POWER_ON_FINISHED,

    GPIO_IN_GAP0,
    GPIO_IN_GAP1,
    GPIO_IN_GAP2,
    
    GPIO_PIN_MAX
}BMC_GPIO_enum;

typedef struct {
    BMC_GPIO_enum     	alias;
    uint32_t      		gpioPort;
    uint32_t      		pin;
    rcu_periph_enum     gpioClk;
    uint8_t             pinMode;
    uint8_t             pinSpeed;
    uint8_t             activeMode; // if output, hi or low active
} GPIOConfig;
  
typedef struct {          
	SUB_DEVICE_MODE mode;
    const GPIOConfig *dev;
    uint8_t configSize;
} GPIOConfig_Handler;

#define GPIOCONFIG_CREATE_HANDLER(config)   .configSize = ARRARY_SIZE(config),  .dev = config 

extern const GPIOConfig_Handler g_gpioConfigHandler_main;
extern const GPIOConfig_Handler g_gpioConfigHandler_net; 
extern const GPIOConfig_Handler g_gpioConfigHandler_switch; 
extern const GPIOConfig_Handler g_gpioConfigHandler_power; 
extern const GPIOConfig_Handler g_gpioConfigHandler_storage0; 

void      GPIO_bspInit     (void);
uint8_t   get_board_addr     (void);


FlagStatus GPIO_getPinStatus(BMC_GPIO_enum alias);    
bool GPIO_setPinStatus(BMC_GPIO_enum alias, ControlStatus isActive);


#endif /* __BSP_GPIO_H_ */
