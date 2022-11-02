#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"
#include "api_sensor.h" 

// config GPIO
const static GPIOConfig g_gpioConfig_switch[] = {
    {GPIO_IN_GAP0,            GPIOG, GPIO_PIN_9, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
};

const GPIOConfig_Handler g_gpioConfigHandler_switch = {
    .mode = SUB_DEVICE_MODE_SWITCH,
    CREATE_CONFIG_HANDLER(gpio, g_gpioConfig_switch),
};

// config ADC
static const  ADCChannlesConfig g_adcChannlConfig_switch[] = {
    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0},
};
static SubDevice_Reading_T g_adcVal_switch[ARRARY_SIZE(g_adcChannlConfig_switch)];

// config Sensor
static const  SensorConfig g_sensor_switch[] = {
    {ADC_CHANNEL_10,         SUB_DEVICE_SDR_TEMP,        "X100 temp"},
};
static SubDevice_Reading_T g_sensorVal_switch[ARRARY_SIZE(g_sensor_switch)];
const Sensor_Handler g_sensorHandler_switch = {
    .mode = SUB_DEVICE_MODE_SWITCH,
    .val = g_sensorVal_switch,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_switch),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_switch),
};
