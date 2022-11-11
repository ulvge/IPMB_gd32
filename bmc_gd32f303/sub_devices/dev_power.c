#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"
#include "api_sensor.h" 

// config GPIO
const static GPIOConfig g_gpioConfig_power[] = {
    {GPIO_IN_GAP0,            GPIOG, GPIO_PIN_9, RCU_GPIOG, GPIO_MODE_IPD, GPIO_OSPEED_10MHZ, 0},
};

const GPIOConfig_Handler g_gpioConfigHandler_power = {
    .mode = SUB_DEVICE_MODE_POWER,
    CREATE_CONFIG_HANDLER(gpio, g_gpioConfig_power),
};

// config ADC
static const  ADCChannlesConfig g_adcChannlConfig_power[] = {
    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0},
};
static SubDevice_Reading_T g_adcVal_power[ARRARY_SIZE(g_adcChannlConfig_power)];

// config Sensor
static const  SensorConfig g_sensor_power[] = {
    {ADC_CHANNEL_10,         SUB_DEVICE_SDR_TEMP,        "X100_temp"},
};
static SubDevice_Reading_T g_sensorVal_power[ARRARY_SIZE(g_sensor_power)];
const Dev_Handler g_devHandler_power = {
    .mode = SUB_DEVICE_MODE_POWER,
    .val = g_sensorVal_power,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_power),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_power),
};
