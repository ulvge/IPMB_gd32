#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "api_sensor.h" 

static void DevTaskHandler(void *pArg);
// config GPIO
const GPIOConfig_Handler g_gpioConfigHandler_storage2 = {
    .mode = SUB_DEVICE_MODE_STORAGE2,
    .gpioCfgSize = 0,
    .gpioCfg = NULL,
};

// config ADC
static const  ADCChannlesConfig g_adcChannlConfig_storage2[] = {
    {ADC_CHANNEL_10,        ADC_CONFIG_GROUP_DEAULT, GPIOC, RCU_GPIOC, GPIO_PIN_0},
    {ADC_CHANNEL_5,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_6},
};

// config Sensor
static const  SensorConfig g_sensor_storage2[] = {
    {ADC_CHANNEL_10,        SUB_DEVICE_SDR_P3V3,        "P3V3_STBY"},
    {ADC_CHANNEL_5,         SUB_DEVICE_SDR_TEMP,        "WORKING_TEMP"},
    {ADC_CHANNEL_6,         SUB_DEVICE_SDR_P12V,        "P12V"},
};
static SubDevice_Reading_T g_sensorVal_storage2[ARRARY_SIZE(g_sensor_storage2)];
const Dev_Handler g_devHandler_storage2 = {
    .mode = SUB_DEVICE_MODE_STORAGE2,
    .val = g_sensorVal_storage2,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_storage2),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_storage2),
    .TaskHandler = DevTaskHandler,
};

static void DevTaskHandler(void *pArg)
{
    while (1)
    {
        vTaskDelay(2000);
        LOG_D("filename = %s, line = %d", __FILE__, __LINE__);
        vTaskDelete(NULL);
    }
}





