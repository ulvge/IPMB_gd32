#include <string.h>  
#include <stdlib.h>
#include "debug_print.h"
#include "api_subdevices.h"
#include "bsp_gpio.h"
#include "bsp_i2c.h"
#include "IPMIConf.h"
#include "IPMDevice.h"
#include "OSPort.h"
#include "IPMI_SensorEvent.h"
#include "ipmi_common.h"
#include "sdr.h"
#include "IPMI_Sensor.h"
#include "sensor_helpers.h"
#include "sensor.h"          
#include "sensor.h"


static const GPIOConfig g_gpioConfig_main[] = {
    {GPIO_OUT_LED_RED,                  GPIOD, GPIO_PIN_8,  RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_LED_GREEN,                GPIOD, GPIO_PIN_9,  RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_CPU_POWER_ON,             GPIOD, GPIO_PIN_10, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_CPU_POWER_OFF,            GPIOD, GPIO_PIN_11, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_CPU_RESET,                GPIOD, GPIO_PIN_12, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_BMC_POWER_ON_FINISHED,    GPIOD, GPIO_PIN_13, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},

    {GPIO_IN_SLAVE_ADDRESS0,            GPIOG, GPIO_PIN_9, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
    {GPIO_IN_SLAVE_ADDRESS1,            GPIOG, GPIO_PIN_10, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
    {GPIO_IN_SLAVE_ADDRESS2,            GPIOG, GPIO_PIN_11, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
};


const GPIOConfig_Handler g_gpioConfigHandler_main = GPIOCONFIG_CREATE_HANDLER(SUB_DEVICE_MODE_MAIN, g_gpioConfig_main);


