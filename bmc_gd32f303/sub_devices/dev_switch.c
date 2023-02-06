#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"
#include "api_sensor.h" 

static void DevTaskHandler(void *pArg);
// config GPIO
static const GPIOConfig g_gpioConfig_switch[] = {
    {GPIO_OUT_WX_NRST,       GPIOD, GPIO_PIN_8,  RCU_GPIOD, GPIO_MODE_OUT_OD,       GPIO_OSPEED_10MHZ, 1},

    {GPIO_IN_P0V9_PWRGD,     GPIOC, GPIO_PIN_6,  RCU_GPIOC, GPIO_MODE_IN_FLOATING,  GPIO_OSPEED_10MHZ, 1},//unused
    {GPIO_OUT_P0V9_EN,       GPIOC, GPIO_PIN_7,  RCU_GPIOC, GPIO_MODE_IN_FLOATING,  GPIO_OSPEED_10MHZ, 1},//unused
    {GPIO_IN_P1V8_PWRGD,     GPIOC, GPIO_PIN_8,  RCU_GPIOC, GPIO_MODE_IN_FLOATING,  GPIO_OSPEED_10MHZ, 1},//unused
    {GPIO_OUT_P1V8_EN,       GPIOC, GPIO_PIN_9,  RCU_GPIOC, GPIO_MODE_IN_FLOATING,  GPIO_OSPEED_10MHZ, 1},//unused
};

static GPIO_CONFIG_EXPORT(g_gpioConfigHandler_switch, SUB_DEVICE_MODE_SWITCH, g_gpioConfig_switch, ARRARY_SIZE(g_gpioConfig_switch));

// config ADC
static const  ADCChannlesConfig g_adcChannlConfig_switch[] = {
    {ADC_CHANNEL_13,        ADC_CONFIG_GROUP_DEAULT, GPIOC, RCU_GPIOC, GPIO_PIN_3},
    {ADC_CHANNEL_1,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_1},
    {ADC_CHANNEL_5,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_6},
    {ADC_CHANNEL_7,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_7},
};

// config Sensor
static const  SensorConfig g_sensor_switch[] = {
    {ADC_CHANNEL_13,        SUB_DEVICE_SDR_P3V3,       "P3V3_AUX"},
    {ADC_CHANNEL_1,         SUB_DEVICE_SDR_P0V9,       "VCC_P0V9"},
    {ADC_CHANNEL_5,         SUB_DEVICE_SDR_TEMP,       "WORKING_TEMP"},
    {ADC_CHANNEL_6,         SUB_DEVICE_SDR_P12V,       "P12V"},
    {ADC_CHANNEL_7,         SUB_DEVICE_SDR_P1V8,       "P1V8_IO"},
};
static SubDevice_Reading_T g_sensorVal_switch[ARRARY_SIZE(g_sensor_switch)];
const Dev_Handler g_devHandler_switch = {
    .mode = SUB_DEVICE_MODE_SWITCH,
    .val = g_sensorVal_switch,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_switch),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_switch),
    .TaskHandler = DevTaskHandler,
};

static void DevTaskHandler(void *pArg)
{
    while (1)
    {
        vTaskDelay(50);
        LOG_D("filename = %s, line = %d", __FILE__, __LINE__);
        GPIO_setPinStatus(GPIO_OUT_WX_NRST, ENABLE);

        vTaskDelete(NULL);
    }
}
