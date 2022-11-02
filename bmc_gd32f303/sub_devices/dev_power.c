#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"


const static GPIOConfig g_gpioConfig_power[] = {
    {GPIO_IN_GAP0,            GPIOG, GPIO_PIN_9, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
};

const GPIOConfig_Handler g_gpioConfigHandler_power = {
    .mode = SUB_DEVICE_MODE_POWER,
    CREATE_CONFIG_HANDLER(g_gpioConfig_power),
};



static const  ADCChannlesConfig g_adcChannlConfig_power[] = {
    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, SUB_DEVICE_SDR_P0V9, "P0V9_VCORE"},
};

static SubDevice_Reading_T g_adcVal_power[ARRARY_SIZE(g_adcChannlConfig_power)];

const ADCChannlesConfig_Handler g_adcChannlHandler_power = {
    .mode = SUB_DEVICE_MODE_NET,
    .val = g_adcVal_power,
    .cfgSize = ARRARY_SIZE(g_adcChannlConfig_power),  
    .cfg = g_adcChannlConfig_power, 
};

