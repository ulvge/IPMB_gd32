#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "api_sensor.h" 
#include "dev_fsm.h"

static void DevTaskHandler(void *pArg);
// config GPIO
const static GPIOConfig g_gpioConfig_storage2[] = {
    {GPIO_CPLD_MCU_DESTORY_LED_M2B_IN,  GPIO_RCU_CONFIG(GPIOB), GPIO_PIN_12, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, 0}, //M.2_B_DESTROY_LED
    {GPIO_CPLD_MCU_DESTORY_LED_M2A_IN,  GPIO_RCU_CONFIG(GPIOB), GPIO_PIN_13, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, 0}, //M.2_A_DESTROY_LED
    {GPIO_CPLD_MCU_DESTORY_OUT,         GPIO_RCU_CONFIG(GPIOB), GPIO_PIN_14, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 0}, //DESTROY_LED 
};
static GPIO_CONFIG_EXPORT(g_gpioConfigHandler_storage2, SUB_DEVICE_MODE_STORAGE2, g_gpioConfig_storage2, ARRARY_SIZE(g_gpioConfig_storage2));

// config ADC
static const  ADCChannlesConfig g_adcChannlConfig_storage2[] = {
    {ADC_CHANNEL_10,        ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_0},
    {ADC_CHANNEL_5,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_6},
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

static Key_ScanST g_LED_M2A_IN = {
    .pin = GPIO_CPLD_MCU_DESTORY_LED_M2A_IN
}; 
static Key_ScanST g_LED_M2B_IN= {
    .pin = GPIO_CPLD_MCU_DESTORY_LED_M2B_IN
};
#define DEV_TASK_DELAY_XMS  8
static void DevTaskHandler(void *pArg)
{
    uint32_t printDiv = 0;
    bool isDestoryPressed = false;
    LOG_D("filename = %s, line = %d", __FILE__, __LINE__);
    while (1)
    {
        vTaskDelay(DEV_TASK_DELAY_XMS);
        KeyPressedDurationMs(&g_LED_M2A_IN);
        KeyPressedDurationMs(&g_LED_M2B_IN);

// 单片机:
// M.2_A_DESTROY_LED
// M.2_B_DESTROY_LED
// 这两个输入信号，平时是高电平，销毁完成是是低电平
// 代码逻辑：只要检测到一个有效信号，则置为低

// CPLD:
// DESTROY_LED 输出指示灯，是高亮
        if (g_LED_M2A_IN.isPreesed || g_LED_M2B_IN.isPreesed) {
            GPIO_setPinStatus(GPIO_CPLD_MCU_DESTORY_OUT, ENABLE);
            if (!isDestoryPressed) {
                LOG_I("\tM.2_DESTROY_LED active, tick = %d\r\n", GetTickMs());
            }
            isDestoryPressed = true;
        } else {
            GPIO_setPinStatus(GPIO_CPLD_MCU_DESTORY_OUT, DISABLE);

            if (isDestoryPressed) {
                LOG_I("\tM.2_DESTROY_LED inactive, tick = %d\r\n", GetTickMs());
            }
            isDestoryPressed = false;
        }

        if (printDiv++ > (2000 / DEV_TASK_DELAY_XMS)) {
            printDiv = 0;
            LOG_I("\tM.2_DESTROY_LED task running\r\n");
        }
    }
}





