#include <string.h>  
#include <stdlib.h>
#include "bsp_gpio.h"
#include "sensor.h"
#include "api_sensor.h" 
#include "mac5023.h" 
#include "dev_fsm.h"
#include "shell_ext.h"
#include "shell_port.h"
#include "jump.h"

#define DEV_POWER_TASK_DELAY_XMS  10
#define MAC5023_SAMPLE_PERIOD_XMS 1000
#define MAC5023_FAILED_MAX_COUNT 10
    
#define WAIT_POWERON_STABILIZE_XMS  100
#define WAIT_POWERDOWN_STABILIZE_XMS  100

#define BATTERY_SAMPLE_PERIOD_XMS   60 * 1000

static void DevTaskHandler(void *pArg);
// config GPIO
const static GPIOConfig g_gpioConfig_power[] = {
    {GPIO_OUT_VBAT_EN,      GPIO_RCU_CONFIG(GPIOB), GPIO_PIN_12, GPIO_MODE_OUT_PP,      GPIO_OSPEED_10MHZ, 1},
    {GPIO_IN_R_GPIO0,       GPIO_RCU_CONFIG(GPIOB), GPIO_PIN_15, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},
    {GPIO_OUT_R_FAIL_N,     GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_8,  GPIO_MODE_OUT_PP,      GPIO_OSPEED_10MHZ, 0},

    {R_CPLD_MCU_5,          GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_9,  GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused
    {R_CPLD_MCU_4,          GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_10, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused
    {R_CPLD_MCU_3,          GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_11, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused
    {R_CPLD_MCU_2,          GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_12, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused
    {R_CPLD_MCU_1,          GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_13, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},//unused

    {GPIO_IN_P12V_PWRGD,    GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_14, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_P12V_EN,      GPIO_RCU_CONFIG(GPIOD), GPIO_PIN_15, GPIO_MODE_OUT_PP,      GPIO_OSPEED_10MHZ, 1},
    {GPIO_IN_P3V3_PWRGD,    GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_6,  GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_P3V3_EN,      GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_7,  GPIO_MODE_OUT_PP,      GPIO_OSPEED_10MHZ, 1},
    {GPIO_IN_P5V_PWRGD,     GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_8,  GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 1},
    {GPIO_OUT_P5V_EN,       GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_9,  GPIO_MODE_OUT_PP,      GPIO_OSPEED_10MHZ, 1},
};

static GPIO_CONFIG_EXPORT(g_gpioConfigHandler_power, SUB_DEVICE_MODE_POWER, g_gpioConfig_power, ARRARY_SIZE(g_gpioConfig_power));

// config ADC
static const  ADCChannlesConfig g_adcChannlConfig_power[] = {
    {ADC_CHANNEL_13,        ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOC), GPIO_PIN_3},
    {ADC_CHANNEL_1,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_1},
    {ADC_CHANNEL_2,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_2},
    {ADC_CHANNEL_5,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_5},
    {ADC_CHANNEL_6,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_6},
    {ADC_CHANNEL_7,         ADC_CONFIG_GROUP_DEAULT, GPIO_RCU_CONFIG(GPIOA), GPIO_PIN_7},
};

static bool DevPower_VBATSampleHookBefore(void);
static bool DevPower_VBATSampleHookAfter(void);
// config Sensor
static const  SensorConfig g_sensor_power[] = {
    {ADC_CHANNEL_13,        SUB_DEVICE_SDR_P3V3,       "P3V3_AUX"},
    {ADC_CHANNEL_1,         SUB_DEVICE_SDR_P5V,        "P5V"},
    {ADC_CHANNEL_2,         SUB_DEVICE_SDR_VBAT,       "VBAT", DevPower_VBATSampleHookBefore, DevPower_VBATSampleHookAfter},
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

static bool DevPower_VBATSampleHookBefore(void)
{                                                             
    static UINT32 lastSampleTickBak = BATTERY_SAMPLE_PERIOD_XMS * 1000;
    if (GetTickMs() - lastSampleTickBak >= BATTERY_SAMPLE_PERIOD_XMS) {
        GPIO_setPinStatus(GPIO_OUT_VBAT_EN, ENABLE);
        lastSampleTickBak = GetTickMs();
        vTaskDelay(25);
        return true;
    }
    return false;
}
static bool DevPower_VBATSampleHookAfter(void)
{
    return GPIO_setPinStatus(GPIO_OUT_VBAT_EN, DISABLE);
}

// *******************   StateMachine start   *******************
static bool DevPowerSM_P12VEn(void *pSM, FSM_EventID eventId)
{
    return GPIO_setPinStatus(GPIO_OUT_P12V_EN, ENABLE);
}
static bool DevPowerSM_WaitP12PowerGD(void *pSM, FSM_EventID eventId)
{
    if (GPIO_isPinActive(GPIO_IN_P12V_PWRGD)) {
        GPIO_setPinStatus(GPIO_OUT_P5V_EN,  ENABLE);
        GPIO_setPinStatus(GPIO_OUT_P3V3_EN, ENABLE);
        return true;
    }
    return false;
}
static bool DevPowerSM_WaitP5VP3VPowerGD(void *pSM, FSM_EventID eventId)
{                                           
    FSM_StateMachine *pFsm = (FSM_StateMachine *)pSM;
    bool P3V3 = GPIO_isPinActive(GPIO_IN_P3V3_PWRGD);
    bool P5V = GPIO_isPinActive(GPIO_IN_P5V_PWRGD);

    if ((P3V3 && P5V) || (GetTickMs() - pFsm->lastHandlerTimeStamp > WAIT_POWERDOWN_STABILIZE_XMS)) {
        return true;
    }
    return false;
}
static bool DevPowerSM_P12Dis(void *pSM, FSM_EventID eventId)
{
    return GPIO_setPinStatus(GPIO_OUT_P12V_EN, DISABLE);
}
static bool DevPowerSM_P5VP3VDis(void *pSM, FSM_EventID eventId)
{
    if (GPIO_isPinActive(GPIO_IN_P12V_PWRGD)) {
        return false;
    }
    GPIO_setPinStatus(GPIO_OUT_P12V_EN, DISABLE);
    GPIO_setPinStatus(GPIO_OUT_P12V_EN, DISABLE);
    return true;
}
static bool DevPowerSM_PowerOff(void *pSM, FSM_EventID eventId)
{
    FSM_StateMachine *pFsm = (FSM_StateMachine *)pSM;
    if (GetTickMs() - pFsm->lastHandlerTimeStamp > WAIT_POWERDOWN_STABILIZE_XMS) {
        return true;
    }
    return false;
}
static const FSM_StateST g_FSM_StateAlias[] = {
    {.state = DEV_ST_POWEROFF,  .alias="Power off finished"},
    {.state = DEV_ST_P12VEN,    .alias="P12V opened"},
    {.state = DEV_ST_P5V_P3VEN, .alias="P12V powerGD, P5V P3V3 opened"},
    {.state = DEV_ST_POWERON,   .alias="All power GD already"},

    {.state = DEV_ST_P12VDIS,   .alias="P12V closed"},
    {.state = DEV_ST_P5VP3V_DIS,.alias="P5V P3V3 closed, all power closed"},
};
static const FSM_StateTransform g_powerStateTran[] = {
    {DEV_ST_POWEROFF,       DEV_EVENT_KEY_RELEASED, DEV_ST_P12VEN,      DevPowerSM_P12VEn},
    {DEV_ST_P12VEN,         DEV_EVENT_NULL,         DEV_ST_P5V_P3VEN,   DevPowerSM_WaitP12PowerGD},
    {DEV_ST_P5V_P3VEN,      DEV_EVENT_NULL,         DEV_ST_POWERON,     DevPowerSM_WaitP5VP3VPowerGD},

    {DEV_ST_POWERON,        DEV_EVENT_KEY_RELEASED, DEV_ST_P12VDIS,     DevPowerSM_P12Dis},
    {DEV_ST_P12VDIS,        DEV_EVENT_NULL,         DEV_ST_P5VP3V_DIS,  DevPowerSM_P5VP3VDis},
    {DEV_ST_P5VP3V_DIS,     DEV_EVENT_NULL,         DEV_ST_POWEROFF,    DevPowerSM_PowerOff},
};               
static void DevPower_printStateAlias(FSM_State sta);
static FSM_StateMachine g_powerSM = {
    .curState = DEV_ST_POWEROFF,
    .transNum = ARRARY_SIZE(g_powerStateTran),
    .lastHandlerTimeStamp = 0,
    .transform = g_powerStateTran,
    .printState = DevPower_printStateAlias,
};

static void DevPower_printStateAlias(FSM_State curState)
{
    for (int i = 0; i < ARRARY_SIZE(g_FSM_StateAlias); i++)
    {
        if (g_FSM_StateAlias[i].state == curState){
            UINT32 elapse = GetTickMs() - g_powerSM.lastHandlerTimeStamp;
            LOG_I("DevPower FSM: %s, elapse [%d] ms \r\n", g_FSM_StateAlias[i].alias, elapse);
            break;
        }
    }
}
bool DevPower_IsPowerGood(void)
{
	float p12v;
	float p3v3;
    switch (SubDevice_GetMyMode())
    {
        case SUB_DEVICE_MODE_MAIN:
            p12v = api_sensorGetModeValHuman(SUB_DEVICE_MODE_POWER, ADC_CHANNEL_6);
            p3v3 = api_sensorGetModeValHuman(SUB_DEVICE_MODE_POWER, ADC_CHANNEL_7);
            if ((p12v > 1.0f) || (p3v3 > 1.0f)){
                return true;
            }
            return false;
        case SUB_DEVICE_MODE_POWER:
            if (g_powerSM.curState == DEV_ST_POWERON) {
                return true;
            } 
            return false;
        default:
            return true;
    }
}
static int DevPower_shellPowerState(int argc, char *argv[])
{
    uint32_t resetCause = update_BkpDateRead(MCU_RESET_CAUSE_ADDR_H) << 16;
    resetCause |= update_BkpDateRead(MCU_RESET_CAUSE_ADDR_L);
    common_printfResetCause((rcu_flag_enum)resetCause); // output reset cause
    if (SubDevice_GetMyMode() == SUB_DEVICE_MODE_POWER) {
        DevPower_printStateAlias(g_powerSM.curState); // output Power State
    }
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, ps, DevPower_shellPowerState, power state & reset cause);

// *******************   StateMachine end   *******************
static void DevPower_SampleMAC5023(void)
{
    float humanVal;
    UINT8 ipmbVal = 0;
    for (UINT8 i = 0; i < ARRARY_SIZE(g_sensor_power); i++){
        UINT8 sensorNum = g_sensor_power[i].sensorNum;
        if ((sensorNum > MAC5023_CHANNLE_START) && (sensorNum < MAC5023_CHANNLE_END)){
            if (MAC5023_Sample(0, sensorNum, &humanVal, &ipmbVal)){
                g_sensorVal_power[i].rawAdc = 0;    // The API does not support raw values
                g_sensorVal_power[i].errCnt = 0;
                g_sensorVal_power[i].rawIPMB = ipmbVal;
                g_sensorVal_power[i].human = humanVal;
                continue;
            }
            if (g_sensorVal_power[i].errCnt++ > MAC5023_FAILED_MAX_COUNT)
            {
                g_sensorVal_power[i].rawAdc = 0;
                g_sensorVal_power[i].errCnt = 0;
                g_sensorVal_power[i].rawIPMB = 0;
                g_sensorVal_power[i].human = 0;
            }
        }
    }
}
static void DevTaskHandler(void *pArg)
{
    Key_ScanST g_key_GPIO_IN_R_GPIO0 = {0};
    uint32_t durationKeyMs;
    uint32_t mac5023SampleDiv = 0;
    vTaskDelay(WAIT_POWERON_STABILIZE_XMS);
    fsm_Handler(&g_powerSM, DEV_EVENT_KEY_RELEASED); // auto power on
    g_key_GPIO_IN_R_GPIO0.pin = GPIO_IN_R_GPIO0;
    while (1)
    {
        vTaskDelay(DEV_POWER_TASK_DELAY_XMS);
        durationKeyMs = KeyPressedDurationMs(&g_key_GPIO_IN_R_GPIO0);
        if (g_key_GPIO_IN_R_GPIO0.isReleased && (durationKeyMs > 90)) {
            fsm_Handler(&g_powerSM, DEV_EVENT_KEY_RELEASED);
        } else {
            fsm_Handler(&g_powerSM, DEV_EVENT_NULL);
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
