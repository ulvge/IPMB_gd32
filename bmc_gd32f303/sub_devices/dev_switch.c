#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "dev_switch_sdr.h" 


const static GPIOConfig g_gpioConfig_switch[] = {
    {GPIO_IN_GAP0,            GPIOG, GPIO_PIN_9, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
};

const GPIOConfig_Handler g_gpioConfigHandler_switch = {
    .mode = SUB_DEVICE_MODE_SWITCH,
    GPIOCONFIG_CREATE_HANDLER(g_gpioConfig_switch),
};


static const  ADCChannlesConfig g_adcChannlConfig_switch[] = {
    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, SUB_DEVICE_SDR_NO_PARTIAL, "P0V9_VCORE"},
};

static SubDevice_Reading_T g_adcVal_switch[ARRARY_SIZE(g_adcChannlConfig_switch)];

const ADCChannlesConfig_Handler g_adcChannlHandler_switch = {
    .mode = SUB_DEVICE_MODE_NET,
    .val = g_adcVal_switch,
    .cfgSize = ARRARY_SIZE(g_adcChannlConfig_switch),  
    .cfg = g_adcChannlConfig_switch, 
    
	.sdrSize = ARRARY_SIZE(g_sensor_sdr_switch),
    .sdr = g_sensor_sdr_switch, 
};

