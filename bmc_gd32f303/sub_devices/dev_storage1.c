#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "api_sensor.h" 

static void DevTaskHandler(void *pArg);
// config GPIO
const static GPIOConfig g_gpioConfig_storage1[] = {
    {GPIO_OUT_P3V3_EN,      GPIOB, GPIO_PIN_12, RCU_GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_IN_P3V3_PWRGD,    GPIOB, GPIO_PIN_13, RCU_GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
};

GPIO_CONFIG_EXPORT(g_gpioConfigHandler_storage1, SUB_DEVICE_MODE_STORAGE1, g_gpioConfig_storage1, ARRARY_SIZE(g_gpioConfig_storage1));
// config ADC
static const  ADCChannlesConfig g_adcChannlConfig_storage1[] = {
    {ADC_CHANNEL_10,        ADC_CONFIG_GROUP_DEAULT, GPIOC, RCU_GPIOC, GPIO_PIN_0},
    {ADC_CHANNEL_11,        ADC_CONFIG_GROUP_DEAULT, GPIOC, RCU_GPIOC, GPIO_PIN_1},
    {ADC_CHANNEL_5,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_6},
};

// config Sensor
static const  SensorConfig g_sensor_storage1[] = {
    {ADC_CHANNEL_10,        SUB_DEVICE_SDR_P3V3,        "P3V3"},
    {ADC_CHANNEL_11,        SUB_DEVICE_SDR_P3V3,        "P3V3_STBY"},
    {ADC_CHANNEL_5,         SUB_DEVICE_SDR_TEMP,        "WORKING_TEMP"},
    {ADC_CHANNEL_6,         SUB_DEVICE_SDR_P12V,        "P12V"},
};
static SubDevice_Reading_T g_sensorVal_storage1[ARRARY_SIZE(g_sensor_storage1)];
const Dev_Handler g_devHandler_storage1 = {
    .mode = SUB_DEVICE_MODE_STORAGE1,
    .val = g_sensorVal_storage1,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_storage1),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_storage1),
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





