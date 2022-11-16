#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"
#include "api_sensor.h" 
#include "mac5023.h" 


#define DEV_POWER_TASK_DELAY_XMS  10
#define MAC5023_SAMPLE_PERIOD_XMS 1000
#define MAC5023_FAILED_MAX_COUNT 10
    

static void DevTaskHandler(void *pArg);
// config GPIO
const static GPIOConfig g_gpioConfig_power[] = {
    {GPIO_OUT_VBAT_EN,      GPIOB, GPIO_PIN_12, RCU_GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_IN_R_GPIO0,       GPIOB, GPIO_PIN_15, RCU_GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},
    {GPIO_OUT_R_FAIL_N,     GPIOD, GPIO_PIN_8,  RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 0},

    {R_CPLD_MCU_5,          GPIOD, GPIO_PIN_9,  RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused
    {R_CPLD_MCU_4,          GPIOD, GPIO_PIN_10, RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused
    {R_CPLD_MCU_3,          GPIOD, GPIO_PIN_11, RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused
    {R_CPLD_MCU_2,          GPIOD, GPIO_PIN_12, RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused
    {R_CPLD_MCU_1,          GPIOD, GPIO_PIN_13, RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused

    {GPIO_IN_P12V_PWRGD,    GPIOD, GPIO_PIN_14, RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_OUT_P12V_EN,      GPIOD, GPIO_PIN_15, RCU_GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_IN_P3V3_PWRGD,    GPIOC, GPIO_PIN_6,  RCU_GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1},//unused
    {GPIO_OUT_P3V3_EN,      GPIOC, GPIO_PIN_7,  RCU_GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
    {GPIO_IN_P5V_PWRGD,     GPIOC, GPIO_PIN_8,  RCU_GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1},//unused
    {GPIO_OUT_P5V_EN,       GPIOC, GPIO_PIN_9,  RCU_GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1},
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
    {ADC_CHANNEL_13,        SUB_DEVICE_SDR_P3V3,       "P3V3_AUX"},
    {ADC_CHANNEL_1,         SUB_DEVICE_SDR_P5V,        "P5V"},
    {ADC_CHANNEL_2,         SUB_DEVICE_SDR_VBAT,       "VBAT"},
    {ADC_CHANNEL_5,         SUB_DEVICE_SDR_TEMP,       "WORKING_TEMP"},
    {ADC_CHANNEL_6,         SUB_DEVICE_SDR_P12V,       "P12V"},
    {ADC_CHANNEL_7,         SUB_DEVICE_SDR_P3V3,       "P3V3"},

    {MAC5023_CHANNLE_PIN,   SUB_DEVICE_SDR_MAC5023_P,  "MAC5023_PIN"},
    {MAC5023_CHANNLE_VIN,   SUB_DEVICE_SDR_MAC5023_V,  "MAC5023_VIN"},
    {MAC5023_CHANNLE_VOUT,  SUB_DEVICE_SDR_MAC5023_V,  "MAC5023_VOUT"},
    {MAC5023_CHANNLE_IOUT,  SUB_DEVICE_SDR_MAC5023_I,  "MAC5023_IOUT"},
};
static SubDevice_Reading_T g_sensorVal_power[ARRARY_SIZE(g_sensor_power)];

const Dev_Handler g_devHandler_power = {
    .mode = SUB_DEVICE_MODE_POWER,
    .val = g_sensorVal_power,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_power),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_power),
    .TaskHandler = DevTaskHandler,
};

static UINT32 CountPinPluseMs(BMC_GPIO_enum pin, uint32_t *lastMs)
{
    #define GPIO_SCAN_PEROID 10
    uint32_t offsetMs;
    if (!GPIO_isPinActive(pin)){
        *lastMs = GetTickMs(); //update to newest tick
        return 0;
    }else{
        if (GPIO_isPinActive(pin))
        {
            return 0; //not update
        }
        offsetMs = GetTickMs() - *lastMs;
        *lastMs = GetTickMs(); //update to newest tick
        return offsetMs;
    }
}
static void DevPower_SampleMAC5023(void)
{
    float humanVal;
    UINT8 ipmbVal = 0;
    for (UINT8 i; i < ARRARY_SIZE(g_sensor_power); i++){
        UINT8 sensorNum = g_sensor_power[i].sensorNum;
        if ((sensorNum > MAC5023_CHANNLE_START) && (sensorNum < MAC5023_CHANNLE_END)){
            if (MAC5023_Sample(0, sensorNum, &humanVal, &ipmbVal)){
                g_sensorVal_power[i].rawAdc = 0;
                g_sensorVal_power[i].raw = ipmbVal;
                g_sensorVal_power[i].errCnt = 0;
                g_sensorVal_power[i].human = humanVal;
                continue;
            }
            if (g_sensorVal_power[i].errCnt++ > MAC5023_FAILED_MAX_COUNT)
            {
                g_sensorVal_power[i].rawAdc = 0;
                g_sensorVal_power[i].errCnt = 0;
                g_sensorVal_power[i].raw = 0;
                g_sensorVal_power[i].human = 0;
            }
        }
    }
}
static void DevTaskHandler(void *pArg)
{
    static uint32_t lastMs_IN_R_GPIO0;
    uint32_t mac5023SampleDiv = 0;
    while (1)
    {
        vTaskDelay(DEV_POWER_TASK_DELAY_XMS);
        if (CountPinPluseMs(GPIO_IN_R_GPIO0, &lastMs_IN_R_GPIO0) > 90)
        {
        }
        else
        {
        }
        if (++mac5023SampleDiv >= (MAC5023_SAMPLE_PERIOD_XMS / DEV_POWER_TASK_DELAY_XMS))
        {
            mac5023SampleDiv = 0;
            DevPower_SampleMAC5023();
        }
        //        if (api_sensorGetValHuman(ADC_CHANNEL_13) > 3.0){
        //
        //        }
    }
}
