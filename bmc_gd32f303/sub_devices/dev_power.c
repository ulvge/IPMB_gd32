#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"
#include "api_sensor.h" 

static void DevTaskHandler(void *pArg);
// config GPIO
const static GPIOConfig g_gpioConfig_power[] = {
    {GPIO_OUT_VBAT_EN,      GPIOB, GPIO_PIN_12, RCU_GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_R_FAIL_N,     GPIOD, GPIO_PIN_8,  RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 0},
    {GPIO_OUT_P12V_EN,      GPIOD, GPIO_PIN_15, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_P3V3_EN,      GPIOC, GPIO_PIN_7,  RCU_GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_P5V_EN,       GPIOC, GPIO_PIN_9,  RCU_GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},

    {GPIO_IN_R_GPIO0,       GPIOB, GPIO_PIN_15, RCU_GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},
    {GPIO_IN_P3V3_PWRGD,    GPIOD, GPIO_PIN_14, RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1},
    {GPIO_IN_P5V_PWRGD,     GPIOC, GPIO_PIN_6,  RCU_GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1},
    {GPIO_IN_P12V_PWRGD,    GPIOC, GPIO_PIN_8,  RCU_GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1},
};

const GPIOConfig_Handler g_gpioConfigHandler_power = {
    .mode = SUB_DEVICE_MODE_POWER,
    CREATE_CONFIG_HANDLER(gpio, g_gpioConfig_power),
};

// config ADC
static const  ADCChannlesConfig g_adcChannlConfig_power[] = {
    {ADC_CHANNEL_13,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_3},
    {ADC_CHANNEL_1,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_1},
    {ADC_CHANNEL_2,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_2},
    {ADC_CHANNEL_5,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_6},
    {ADC_CHANNEL_7,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_7},
};
static SubDevice_Reading_T g_adcVal_power[ARRARY_SIZE(g_adcChannlConfig_power)];

// config Sensor
static const  SensorConfig g_sensor_power[] = {
    {ADC_CHANNEL_13,        SUB_DEVICE_SDR_P3V3,       "P3V3_AUX_M"},
    {ADC_CHANNEL_1,         SUB_DEVICE_SDR_P5V,        "P5V_M"},
    {ADC_CHANNEL_2,         SUB_DEVICE_SDR_P3V3,       "VBAT_M"},
    {ADC_CHANNEL_5,         SUB_DEVICE_SDR_TEMP,       "WORKING_TEMP"},
    {ADC_CHANNEL_6,         SUB_DEVICE_SDR_P12V,       "P12V_M"},
    {ADC_CHANNEL_7,         SUB_DEVICE_SDR_P3V3,       "P3V3_M"},
};
static SubDevice_Reading_T g_sensorVal_power[ARRARY_SIZE(g_sensor_power)];
const Dev_Handler g_devHandler_power = {
    .mode = SUB_DEVICE_MODE_POWER,
    .val = g_sensorVal_power,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_power),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_power),
    .TaskHandler = DevTaskHandler,
};
static UINT32 CountPinPluseMs(BMC_GPIO_enum pin)
{
    #define GPIO_SCAN_PEROID 10
    uint32_t  lastMs = GetTickMs();
    uint32_t offsetMs;
    if (!GPIO_isPinActive(GPIO_IN_R_GPIO0)){
        return 0;
    }else{
        while (GPIO_isPinActive(GPIO_IN_R_GPIO0))
        {
            vTaskDelay(GPIO_SCAN_PEROID);
        }
        offsetMs = GetTickMs() - lastMs;
        return offsetMs;
    }
}
static void DevTaskHandler(void *pArg)
{
    while (1)
    {
        vTaskDelay(10);
        if (GPIO_isPinActive(GPIO_IN_R_GPIO0)){
            
        }
        LOG_E("filename = %s, line = %d", __FILE__, __LINE__);

        if (api_sensorGetValHuman(ADC_CHANNEL_13) > 3.0){
            
        }
    }
}
