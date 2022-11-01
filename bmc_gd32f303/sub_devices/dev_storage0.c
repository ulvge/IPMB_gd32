#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "dev_storage0_sdr.h" 


const static GPIOConfig g_gpioConfig_storage0[] = {
    {GPIO_IN_GAP0,            GPIOG, GPIO_PIN_9, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
};

const GPIOConfig_Handler g_gpioConfigHandler_storage0 = {
    .mode = SUB_DEVICE_MODE_STORAGE0,
    GPIOCONFIG_CREATE_HANDLER(g_gpioConfig_storage0),
};


static const  ADCChannlesConfig g_adcChannlConfig_storage0[] = {
    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, SUB_DEVICE_SDR_P0V9, "P0V9_VCORE"},
};

static SubDevice_Reading_T g_adcVal_storage0[ARRARY_SIZE(g_adcChannlConfig_storage0)];

const ADCChannlesConfig_Handler g_adcChannlHandler_storage0 = {
    .mode = SUB_DEVICE_MODE_NET,
    .val = g_adcVal_storage0,
    .cfgSize = ARRARY_SIZE(g_adcChannlConfig_storage0),  
    .cfg = g_adcChannlConfig_storage0, 
    
	.sdrSize = ARRARY_SIZE(g_sensor_sdr_storage0),
    .sdr = g_sensor_sdr_storage0, 
};





