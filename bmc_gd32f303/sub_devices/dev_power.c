#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "dev_power_sdr.h" 


const static GPIOConfig g_gpioConfig_power[] = {
    {GPIO_IN_GAP0,            GPIOG, GPIO_PIN_9, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
};

const GPIOConfig_Handler g_gpioConfigHandler_power = {
    .mode = SUB_DEVICE_MODE_POWER,
    GPIOCONFIG_CREATE_HANDLER(g_gpioConfig_power),
};



static const  ADCChannlesConfig g_adcChannlConfig_power[] = {
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

static SubDevice_Reading_T g_adcVal_power[ARRARY_SIZE(g_adcChannlConfig_power)];

const ADCChannlesConfig_Handler g_adcChannlHandler_power = {
    .mode = SUB_DEVICE_MODE_NET,
    .val = g_adcVal_power,
    .cfgSize = ARRARY_SIZE(g_adcChannlConfig_power),  
    .cfg = g_adcChannlConfig_power, 
    
	.sdrSize = ARRARY_SIZE(g_sensor_sdr_power),
    .sdr = g_sensor_sdr_power, 
};

