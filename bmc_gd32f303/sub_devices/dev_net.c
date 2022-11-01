#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  


const static GPIOConfig g_gpioConfig_net[] = {
    {GPIO_IN_GAP0,            GPIOG, GPIO_PIN_9, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
};

const GPIOConfig_Handler g_gpioConfigHandler_net = {
    .mode = SUB_DEVICE_MODE_NET,
    GPIOCONFIG_CREATE_HANDLER(g_gpioConfig_net),
};

static const  ADCChannlesConfig g_adcChannlConfig_net[] = {
    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, SUB_DEVICE_SDR_P0V9, "P0V9_VCORE"},
    
};

static SubDevice_Reading_T g_adcVal_net[ARRARY_SIZE(g_adcChannlConfig_net)];

const ADCChannlesConfig_Handler g_adcChannlHandler_net = {
    .mode = SUB_DEVICE_MODE_NET,
    .val = g_adcVal_net,
    .cfgSize = ARRARY_SIZE(g_adcChannlConfig_net),  
    .cfg = g_adcChannlConfig_net, 
};

