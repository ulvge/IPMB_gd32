#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "api_sensor.h"    
#include "MsgHndlr.h"
#include "ipmi_common.h"

										 
static void DevTaskHandler(void *pArg);
// config GPIO
static const GPIOConfig g_gpioConfig_main[] = {
    {GPIO_CPLD_MCU_13,                  GPIOB, GPIO_PIN_15,RCU_GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_12,                  GPIOD, GPIO_PIN_8, RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_11,                  GPIOD, GPIO_PIN_9, RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_10,                  GPIOD, GPIO_PIN_10,RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_9,                   GPIOD, GPIO_PIN_11,RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_8,                   GPIOD, GPIO_PIN_12,RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_7,                   GPIOD, GPIO_PIN_13,RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_6,                   GPIOD, GPIO_PIN_14,RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_5,                   GPIOD, GPIO_PIN_15,RCU_GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused

    {GPIO_CPLD_MCU_4,                   GPIOC, GPIO_PIN_6,RCU_GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_3,                   GPIOC, GPIO_PIN_7,RCU_GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_2,                   GPIOC, GPIO_PIN_8,RCU_GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_1,                   GPIOC, GPIO_PIN_9,RCU_GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1}, // CPU run state
};

const GPIOConfig_Handler g_gpioConfigHandler_main = {
    .mode = SUB_DEVICE_MODE_MAIN,
    CREATE_CONFIG_HANDLER(gpio, g_gpioConfig_main),
};
static const  ADCChannlesConfig g_adcChannlConfig_main[] = {
#ifdef GD32F1x
    {ADC_CHANNEL_8,         ADC_CONFIG_GROUP_DEAULT, GPIOB, RCU_GPIOB, GPIO_PIN_0},
    {ADC_CHANNEL_0,      	ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_0},
    {ADC_CHANNEL_10,        ADC_CONFIG_GROUP_DEAULT, GPIOC, RCU_GPIOC, GPIO_PIN_0},
#else
    {ADC_CHANNEL_10,        ADC_CONFIG_GROUP_DEAULT, GPIOC, RCU_GPIOC, GPIO_PIN_0},
    {ADC_CHANNEL_11,        ADC_CONFIG_GROUP_DEAULT, GPIOC, RCU_GPIOC, GPIO_PIN_1},
    {ADC_CHANNEL_13,        ADC_CONFIG_GROUP_DEAULT, GPIOC, RCU_GPIOC, GPIO_PIN_3},

    {ADC_CHANNEL_1,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_1},
    {ADC_CHANNEL_5,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_6},
    {ADC_CHANNEL_7,         ADC_CONFIG_GROUP_DEAULT, GPIOA, RCU_GPIOA, GPIO_PIN_7},
    {ADC_CHANNEL_14,        ADC_CONFIG_GROUP_DEAULT, GPIOC, RCU_GPIOC, GPIO_PIN_4},
#endif
};

static const  SensorConfig g_sensor_main[] = {
#ifdef GD32F1x
    {ADC_CHANNEL_8,         SUB_DEVICE_SDR_TEMP,        "X100_temp"},
    {ADC_CHANNEL_0,      	SUB_DEVICE_SDR_P1V8,        "P1V8_VCC"},
    {ADC_CHANNEL_10,        SUB_DEVICE_SDR_P12V_10_1,   "P12V_standby"},
#else
    {ADC_CHANNEL_10,        SUB_DEVICE_SDR_P0V9, "P0V9_VCORE"},
    {ADC_CHANNEL_11,        SUB_DEVICE_SDR_P2V5, "CPU_P2V5_DDR4"},
    {ADC_CHANNEL_13,        SUB_DEVICE_SDR_P1V2, "P1V2_VDDQ"},

    {ADC_CHANNEL_1,         SUB_DEVICE_SDR_P1V8, "P1V8_IO"},
    {ADC_CHANNEL_5,         SUB_DEVICE_SDR_TEMP, "WORKING_TEMP"},
    {ADC_CHANNEL_6,         SUB_DEVICE_SDR_P12V, "P12V"},
    {ADC_CHANNEL_7,         SUB_DEVICE_SDR_P3V3, "P3V3"},
    {ADC_CHANNEL_14,        SUB_DEVICE_SDR_TEMP, "CPU_TEMP"},
#endif
};

static SubDevice_Reading_T g_sensorVal_main[ARRARY_SIZE(g_sensor_main)];
const Dev_Handler g_devHandler_main = {
    .mode = SUB_DEVICE_MODE_MAIN,
    .val = g_sensorVal_main,
    CREATE_CONFIG_HANDLER(adc, g_adcChannlConfig_main),

    CREATE_CONFIG_HANDLER(sensor, g_sensor_main),
    .TaskHandler = DevTaskHandler,
};

xQueueHandle CPU_recvDatMsg_Queue = NULL;
static bool CPU_MsgCoreInit(void)
{
    CPU_recvDatMsg_Queue = xQueueCreate(1, sizeof(SamllMsgPkt_T));
    if (CPU_recvDatMsg_Queue == NULL) {
        return false;
    }
    return true;
}
static void CPU_MsgCoreHndlr(void)
{
    SamllMsgPkt_T recvPkt;
    INT8U cmd;

    BaseType_t err = xQueueReceive(CPU_recvDatMsg_Queue, &recvPkt, 0);
    if (err == pdFALSE) {
        return;
    }
    if (recvPkt.Param == SERIAL_REQUEST) {
        //recvPkt.Size = DecodeSerialPkt(recvPkt.Data, recvPkt.Size);
        /* 1 Validate the checksum */
        if (0 != CalculateCheckSum (recvPkt.Data, recvPkt.Size))
        {
            LOG_I("receved msg from CPU, but checksum err!");
            return;
        }
        cmd = recvPkt.Data[0];
        ControlStatus isActive = (cmd == 1) ? ENABLE : DISABLE;
        GPIO_setPinStatus(GPIO_CPLD_MCU_1, isActive);
    }
}

TaskHandle_t xHandleDevTask = NULL;
static void DevTaskHandler(void *pArg)
{
    uint32_t errCreate = 0;
    LOG_D("filename = %s, line = %d", __FILE__, __LINE__);
    if (!CPU_MsgCoreInit()) {
        LOG_E("CPU_MsgCoreInit create ERR!");
        errCreate++;
    }
    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ==
        xTaskCreate(SubDevice_uploadTask, "upload", configMINIMAL_STACK_SIZE * 1, NULL, TASK_PRIO_UPLOAD, &xHandleDevTask)) {
        LOG_E("SubDevice_uploadTask create ERR!");
        errCreate++;
    }
    if (errCreate != 0) {
        LOG_E("DevTaskHandler init error DevTaskHandler");
        vTaskDelete(NULL);
    }

    while (1) {
        vTaskDelay(500);
        CPU_MsgCoreHndlr();
    }
}
