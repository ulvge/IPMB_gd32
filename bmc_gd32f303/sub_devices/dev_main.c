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
#include "api_sensor.h"    
#include "dev_main_sdr.h"

static const GPIOConfig g_gpioConfig_main[] = {
    {GPIO_OUT_LED_RED,                  GPIOD, GPIO_PIN_8,  RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_LED_GREEN,                GPIOD, GPIO_PIN_9,  RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_CPU_POWER_ON,             GPIOD, GPIO_PIN_10, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_CPU_POWER_OFF,            GPIOD, GPIO_PIN_11, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_CPU_RESET,                GPIOD, GPIO_PIN_12, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_BMC_POWER_ON_FINISHED,    GPIOD, GPIO_PIN_13, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
};

const GPIOConfig_Handler g_gpioConfigHandler_main = {
    .mode = SUB_DEVICE_MODE_MAIN,
    GPIOCONFIG_CREATE_HANDLER(g_gpioConfig_main),
};

static const  ADCChannlesConfig g_adcChannlConfig_main[] = {
    {ADC_CHANNEL_TEMP_X100, ADC0, RCU_ADC0, GPIOB, RCU_GPIOB, GPIO_PIN_0, "X100 temp"},
    {ADC_CHANNEL_P1V8,      ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_0, "P1V8 VCC"},
    {ADC_CHANNEL_P12V,      ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, "P12V standby"},

    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, "P0V9_VCORE"},
    {ADC_CHANNEL_11,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_1, "CPU_P2V5_DDR4"},
    {ADC_CHANNEL_13,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_3, "P1V2_VDDQ"},

    {ADC_CHANNEL_1,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_1, "P1V8_IO"},
    {ADC_CHANNEL_2,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_2, "VBAT"},

    {ADC_CHANNEL_5,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_5, "WORKING_TEMP"},
    {ADC_CHANNEL_6,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_6, "P12V"},
    {ADC_CHANNEL_7,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_7, "P3V3"},
    {ADC_CHANNEL_8,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_8, "CPU_TEMP"},
};

static SubDevice_Reading_T g_adcVal_main[ARRARY_SIZE(g_adcChannlConfig_main)];

const ADCChannlesConfig_Handler g_adcChannlHandler_main = {
    .mode = SUB_DEVICE_MODE_MAIN,
    .val = g_adcVal_main,
    .cfgSize = ARRARY_SIZE(g_adcChannlConfig_main),  
    .cfg = g_adcChannlConfig_main, 
    
	.sdrSize = ARRARY_SIZE(g_sensor_sdr_main),
    .sdr = g_sensor_sdr_main, 
};

