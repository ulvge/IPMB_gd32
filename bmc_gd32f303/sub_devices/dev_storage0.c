#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "api_sensor.h" 

static void DevTaskHandler(void *pArg);
// config GPIO
const static GPIOConfig g_gpioConfig_storage0[] = {
    {GPIO_OUT_P3V3_EN,      GPIOB, GPIO_PIN_12, RCU_GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_IN_P3V3_PWRGD,    GPIOB, GPIO_PIN_13, RCU_GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
};

const GPIOConfig_Handler g_gpioConfigHandler_storage0 = {
    .mode = SUB_DEVICE_MODE_STORAGE0,
    CREATE_CONFIG_HANDLER(gpio, g_gpioConfig_storage0),
};

// config ADC
static const  ADCChannlesConfig g_adcChannlConfig_storage0[] = {
    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0},
    {ADC_CHANNEL_11,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_1},
    {ADC_CHANNEL_5,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_6},
};

// config Sensor
static const  SensorConfig g_sensor_storage0[] = {
    {ADC_CHANNEL_10,        SUB_DEVICE_SDR_P3V3,        "P3V3"},
    {ADC_CHANNEL_11,        SUB_DEVICE_SDR_P2V5,        "P3V3_STBY"},
    {ADC_CHANNEL_5,         SUB_DEVICE_SDR_TEMP,        "WORKING_TEMP"},
    {ADC_CHANNEL_6,         SUB_DEVICE_SDR_P12V,        "P12V"},
};
static SubDevice_Reading_T g_sensorVal_storage0[ARRARY_SIZE(g_sensor_storage0)];
const Dev_Handler g_devHandler_storage0 = {
    .mode = SUB_DEVICE_MODE_STORAGE0,
    .val = g_sensorVal_storage0,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_storage0),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_storage0),
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





