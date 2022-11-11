/*
 ****************************************************************
 **                                                            **
 **    api_fan.c
 **                                                            **
 ****************************************************************
******************************************************************/

#include "fan/api_fan.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h" 
#include "sensor/api_sensor.h"
#include "debug_print.h" 

#define FAN_TIMER_ID_TASK   1
#define FAN_TIMER_ID_WDT    2
typedef struct 
{
    uint8_t fanIdx;
    const PwmChannleConfig *config;
    int32_t rpmSet;
    int32_t rpmCalculate;
} FanStruct;

static const PwmChannleConfig g_pwmChannleConfig[] = {
    {FAN_CHANNEL_1, 30000, 2, false, RCU_TIMER2, TIMER2,   TIMER_CH_0,  RCU_GPIOC, GPIOC, GPIO_PIN_6, GPIO_TIMER2_FULL_REMAP},
    {FAN_CHANNEL_2, 30000, 2, false, RCU_TIMER3, TIMER3,   TIMER_CH_2,  RCU_GPIOD, GPIOD, GPIO_PIN_14, GPIO_TIMER3_REMAP},
};
#define SIZE_FAN_CONFIG     sizeof(g_pwmChannleConfig)/sizeof(g_pwmChannleConfig[0])
static FanStruct g_Fan[SIZE_FAN_CONFIG];

static TimerHandle_t xTimersFanWatchdog = NULL;

static void 	 vTimerFanWatchdogCallback      (xTimerHandle pxTimer);

static FanStruct *fan_getHandler(int sensorNum)
{
    for(uint32_t i = 0; i < SIZE_FAN_CONFIG; i++)
    {
        FanStruct *pFan = &g_Fan[i];
        if (pFan->config->fanSensorNum == sensorNum){
            return pFan;
        }
    }
    return NULL;
}
bool fan_getFanSensorNum(int idx, uint32_t *sensorNum)
{
    for(uint32_t i = 0; i < SIZE_FAN_CONFIG; i++)
    {
        FanStruct *pFan = &g_Fan[i];
        if (pFan->fanIdx == idx){
            *sensorNum = pFan->config->fanSensorNum;
            return true;
        }
    }
    return false;
}
int32_t fan_getFanNum(void)
{
    return SIZE_FAN_CONFIG;
}
void fan_init(void)
{

    for(int32_t i = 0; i < SIZE_FAN_CONFIG; i++)
    {
        FanStruct *pFan = &g_Fan[i];
        pFan->fanIdx = i;
        pFan->config = &g_pwmChannleConfig[i];
        pFan->rpmSet = 0;
        pFan->rpmCalculate = 0;

        pwm_timer_gpio_config(pFan->config);
        pwm_timer_config(pFan->config);
        fan_set_duty_percent(pFan->config->fanSensorNum, 30);
    }

    TimerHandle_t xTimersFanTask = xTimerCreate("fanTimer", 500/portTICK_RATE_MS, pdTRUE, (void*)FAN_TIMER_ID_TASK, vTimerFanWatchdogCallback); 
    xTimerStart(xTimersFanTask, portMAX_DELAY);

    TimerHandle_t xTimersFanWatchdog = xTimerCreate("fanTimer", 1000/portTICK_RATE_MS, pdTRUE, (void*)FAN_TIMER_ID_WDT, vTimerFanWatchdogCallback); 
    xTimerStart(xTimersFanWatchdog, portMAX_DELAY);

    capture_init();
}

static void fan_ctrl_task(void)
{
    uint8_t ipmbVal;
    uint16_t curent_rpm = 0;
    int32_t pid_out = 0; 
    bool isAlreadyPrint = false;
    uint8_t sensorNum;
    float humanVal;

    for(uint32_t i = 0; i < SIZE_FAN_CONFIG; i++) {
        FanStruct *pFan = &g_Fan[i];
        sensorNum = pFan->config->fanSensorNum;
        if(fan_get_rotate_rpm(sensorNum, &curent_rpm) == false){
            continue;
        }

        if (api_sensorConvertIPMBValBySensorNum(SubDevice_GetMyMode(), sensorNum, curent_rpm, &ipmbVal))
        {
            api_sensorSetValRaw(sensorNum, ipmbVal);
            if (api_sensorConvert2HumanVal(SubDevice_GetMyMode(), sensorNum, ipmbVal, &humanVal)) {
                api_sensorSetValHuman(sensorNum, humanVal);
            }
        }

		isAlreadyPrint = true;
		LOG_D("fan id = %d, curent_rpm = %d\n", i, curent_rpm);
    } 
	if (isAlreadyPrint) {
		//LOG_D("\n");
	}
}
bool fan_get_rotate_rpm(unsigned char sensorNum, uint16_t *fan_rpm)
{
    uint32_t capCount;
	uint16_t rpm;

    if(capture_getIsValid(sensorNum) == false)
    {
        *fan_rpm = 0;
        return false;
    }
    FanStruct *pFan = fan_getHandler(sensorNum);
    if(pFan == NULL)
    {
        return false;
    }
    capture_get_value(sensorNum, &capCount);
	//n =  f * 60/ p
    rpm = ((uint32_t)CAPTURE_TIMER_FREQUENCY / capCount) * CAPTURE_1MIN_XSEC / pFan->config->polesNum;
    if (rpm > pFan->config->maxRotateRpm || rpm < 100)
    {
        return false;
    }
	*fan_rpm = rpm;
    return true;
}

bool fan_set_rotate_rpm(int sensorNum, uint32_t rpm)
{
    FanStruct *pFan = fan_getHandler(sensorNum);
    if(pFan == NULL)
    {
        return false;
    }
    if (rpm > pFan->config->maxRotateRpm)
    {
        return false;
    }
    pFan->rpmSet = rpm;
    return true;
}

/* convert to rpm */
bool fan_set_duty(int sensorNum, unsigned char duty)
{
    FanStruct *pFan = fan_getHandler(sensorNum);
    if(pFan == NULL)
    {
        return false;
    }
    pFan->rpmSet = duty * (pFan->config->maxRotateRpm) /100;
    return true;
}

/* pwm raw duty percent */
bool fan_set_duty_percent(int sensorNum, unsigned char duty)
{
    uint32_t duty_value = 0;

    duty_value = duty * FAN_PWM_MAX_DUTY_VALUE/100;
  
    FanStruct *pFan = fan_getHandler(sensorNum);
    if(pFan == NULL)
    {
        return false;
    }
    timer_channel_output_pulse_value_config(pFan->config->timerPeriph, pFan->config->timerCh, duty_value);
    return true;
}
static void vTimerFanWatchdogCallback(xTimerHandle pxTimer)
{
    uint32_t ulTimerID;	
	ulTimerID = (uint32_t) pvTimerGetTimerID( pxTimer );         
	
	switch ( ulTimerID )
	{
    case FAN_TIMER_ID_TASK: // software timer1
        fan_ctrl_task();
        break;
    case FAN_TIMER_ID_WDT: // software timer1
        for(uint32_t i = 0; i < capture_getTotalNum(); i++)
        {
            CaptureStruct *pCap = capture_getHandler(i);
            
            if (pCap == NULL)
            {
                continue;
            }

            if(pCap->cap_no_update_cnt++ > 2) // 2s  fan cap err
            {
                pCap->is_valid = false;
            }
        }
        break;
    default:
      break;
	}	
    
}
