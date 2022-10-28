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


const static GPIOConfig g_gpioConfig_net[] = {
    {GPIO_IN_SLAVE_ADDRESS0,            GPIOG, GPIO_PIN_9, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
    {GPIO_IN_SLAVE_ADDRESS1,            GPIOG, GPIO_PIN_10, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
    {GPIO_IN_SLAVE_ADDRESS2,            GPIOG, GPIO_PIN_11, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
};

const GPIOConfig_Handler g_gpioConfigHandler_net = GPIOCONFIG_CREATE_HANDLER(SUB_DEVICE_MODE_NET, g_gpioConfig_net);


