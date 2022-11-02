#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "api_sensor.h"       

static const GPIOConfig g_gpioConfig_main[] = {
    {GPIO_OUT_LED_RED,                  GPIOD, GPIO_PIN_8,  RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_LED_GREEN,                GPIOD, GPIO_PIN_9,  RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_CPU_POWER_ON,             GPIOD, GPIO_PIN_10, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_CPU_POWER_OFF,            GPIOD, GPIO_PIN_11, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_CPU_RESET,                GPIOD, GPIO_PIN_12, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_BMC_POWER_ON_FINISHED,    GPIOD, GPIO_PIN_13, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
};

const GPIOConfig_Handler g_gpioConfigHandler_main = {
    .mode = SUB_DEVICE_MODE_MAIN,
    CREATE_CONFIG_HANDLER(gpio, g_gpioConfig_main),
};
static const  ADCChannlesConfig g_adcChannlConfig_main[] = {
#if 1
    {ADC_CHANNEL_8,         ADC0, RCU_ADC0, GPIOB, RCU_GPIOB, GPIO_PIN_0},
    {ADC_CHANNEL_0,      	ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_0},
    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0},
#else
    {ADC_CHANNEL_10,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0},
    {ADC_CHANNEL_11,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_1},
    {ADC_CHANNEL_13,        ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_3},

    {ADC_CHANNEL_1,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_1},
    {ADC_CHANNEL_2,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_2},

    {ADC_CHANNEL_5,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_6},
    {ADC_CHANNEL_7,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_7},
    {ADC_CHANNEL_8,         ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_8},
#endif
};

static const  SensorConfig g_sensor_main[] = {
#if 1
    {ADC_CHANNEL_8,         SUB_DEVICE_SDR_TEMP,        "X100 temp"},
    {ADC_CHANNEL_0,      	SUB_DEVICE_SDR_P1V8,        "P1V8 VCC"},
    {ADC_CHANNEL_10,        SUB_DEVICE_SDR_P12V_10_1,   "P12V standby"},
    {FAN_CHANNEL_1,			SUB_DEVICE_SDR_FAN,         "FAN_CHANNEL_1"},
#else
    {ADC_CHANNEL_10,        SUB_DEVICE_SDR_P0V9, "P0V9_VCORE"},
    {ADC_CHANNEL_11,        SUB_DEVICE_SDR_P2V5, "CPU_P2V5_DDR4"},
    {ADC_CHANNEL_13,        SUB_DEVICE_SDR_P2V5, "P1V2_VDDQ"},

    {ADC_CHANNEL_1,         SUB_DEVICE_SDR_P1V8, "P1V8_IO"},
    {ADC_CHANNEL_2,         SUB_DEVICE_SDR_P3V3, "VBAT"},

    {ADC_CHANNEL_5,         SUB_DEVICE_SDR_TEMP, "WORKING_TEMP"},
    {ADC_CHANNEL_6,         SUB_DEVICE_SDR_P12V, "P12V"},
    {ADC_CHANNEL_7,         SUB_DEVICE_SDR_P3V3, "P3V3"},
    {ADC_CHANNEL_8,         SUB_DEVICE_SDR_TEMP, "CPU_TEMP"},
#endif
};

static SubDevice_Reading_T g_sensorVal_main[ARRARY_SIZE(g_sensor_main)];
const Sensor_Handler g_sensorHandler_main = {
    .mode = SUB_DEVICE_MODE_MAIN,
    .val = g_sensorVal_main,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_main),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_main),
};
