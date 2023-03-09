#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"  
#include "api_sensor.h"    
#include "MsgHndlr.h"
#include "ipmi_common.h"

#define CPU_LOSE_MSG_XMS 3000
#define CPU_LED_FLASH_XMS 500
typedef enum
{
    CPU_RUN_STATE_ALARM = 0,
    CPU_RUN_STATE_NORMAL = 1,
} CPURunState;

typedef struct
{
    CPURunState runState;
    ControlStatus ledLastState;
    uint8_t rcvCmd;
    uint32_t rcvCmdTimerStamp;
} CPURunStateCtrl;
static CPURunStateCtrl g_CPURunStateCtrl = {
	.rcvCmdTimerStamp = (uint32_t)(-CPU_LOSE_MSG_XMS),
};
static void DevTaskHandler(void *pArg);
// config GPIO
static const GPIOConfig g_gpioConfig_main[] = {
    {GPIO_CPLD_MCU_13,                  GPIO_RCU_CONFIG(GPIOB), GPIO_PIN_15, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_12,                  GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_8,  GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_11,                  GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_9, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_10,                  GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_10, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_9,                   GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_11, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_8,                   GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_12, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_7,                   GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_13, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_6,                   GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_14, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_5,                   GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_15, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused

    {GPIO_CPLD_MCU_4,                   GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_6, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 1}, // CPU run state
    {GPIO_CPLD_MCU_3,                   GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_7, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1}, //unused
    {GPIO_CPLD_MCU_2,                   GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_8, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 0}, //reset from MCU to CPLD
    {GPIO_CPLD_MCU_1,                   GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_9, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, 0}, //upower on/off from MCU to CPLD
};

static GPIO_CONFIG_EXPORT(g_gpioConfigHandler_main, SUB_DEVICE_MODE_MAIN, g_gpioConfig_main, ARRARY_SIZE(g_gpioConfig_main));

static const  ADCChannlesConfig g_adcChannlConfig_main[] = {
    {ADC_CHANNEL_10,        ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_0},
    {ADC_CHANNEL_11,        ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_1},
    {ADC_CHANNEL_13,        ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_3},

    {ADC_CHANNEL_1,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_1},
    {ADC_CHANNEL_5,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_6},
    {ADC_CHANNEL_7,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_7},
    {ADC_CHANNEL_14,        ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_4},
};

static const  SensorConfig g_sensor_main[] = {
    {ADC_CHANNEL_10,        SUB_DEVICE_SDR_P0V9, "P0V9_VCORE"},
    {ADC_CHANNEL_11,        SUB_DEVICE_SDR_P2V5, "CPU_P2V5_DDR4"},
    {ADC_CHANNEL_13,        SUB_DEVICE_SDR_P1V2, "P1V2_VDDQ"},

    {ADC_CHANNEL_1,         SUB_DEVICE_SDR_P1V8, "P1V8_IO"},
    {ADC_CHANNEL_5,         SUB_DEVICE_SDR_TEMP, "WORKING_TEMP"},
    {ADC_CHANNEL_6,         SUB_DEVICE_SDR_P12V, "P12V"},
    {ADC_CHANNEL_7,         SUB_DEVICE_SDR_P3V3, "P3V3"},
    {ADC_CHANNEL_14,        SUB_DEVICE_SDR_TEMP, "CPU_TEMP"},
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
    CPU_recvDatMsg_Queue = xQueueCreate(3, sizeof(SamllMsgPkt_T));
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
        // 没收到 | 进入系统的时候 : 灭
        // 报警 : 闪
        // 正常进入系统后 : 常亮
        g_CPURunStateCtrl.rcvCmd = cmd;
        g_CPURunStateCtrl.rcvCmdTimerStamp = GetTickMs(); 
        LOG_D("CPU_MsgCoreHndlr rcvCmd = %s", cmd == CPU_RUN_STATE_ALARM ? "alarm" : " normal");
    }
}

static void TimerCpuRunStateCallBack(xTimerHandle pxTimer)
{
    UNUSED(pxTimer);
    if (GetTickMs() - g_CPURunStateCtrl.rcvCmdTimerStamp > CPU_LOSE_MSG_XMS) {
        GPIO_setPinStatus(GPIO_CPLD_MCU_4, DISABLE);
    } else {
        if (g_CPURunStateCtrl.rcvCmd == CPU_RUN_STATE_ALARM) {
            GPIO_setPinStatus(GPIO_CPLD_MCU_4, g_CPURunStateCtrl.ledLastState);
            g_CPURunStateCtrl.ledLastState = (g_CPURunStateCtrl.ledLastState == DISABLE ? ENABLE : DISABLE);
        } else if (g_CPURunStateCtrl.rcvCmd == CPU_RUN_STATE_NORMAL) {
            GPIO_setPinStatus(GPIO_CPLD_MCU_4, ENABLE);
        }
    }
}
TaskHandle_t xHandleUploadTask = NULL;
static void DevTaskHandler(void *pArg)
{
    uint32_t errCreate = 0;
    LOG_D("filename = %s, line = %d", __FILE__, __LINE__);
    if (!CPU_MsgCoreInit()) {
        LOG_E("CPU_MsgCoreInit create ERR!");
        errCreate++;
    }
    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ==
        xTaskCreate(SubDevice_uploadTask, "uploadTask", configMINIMAL_STACK_SIZE * 2, NULL, TASK_PRIO_UPLOAD, &xHandleUploadTask)) {
        LOG_E("SubDevice_uploadTask create ERR!");
        errCreate++;
    }
    TimerHandle_t xTimersCpuRunState = xTimerCreate("TimerCpuRunState", CPU_LED_FLASH_XMS / portTICK_RATE_MS, pdTRUE, 
                                    (void*)0, TimerCpuRunStateCallBack);
    if (pdFAIL == xTimerStart(xTimersCpuRunState, portMAX_DELAY)) {
        LOG_E("DevTaskHandler init create  TimerCpuRunState failed");
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
