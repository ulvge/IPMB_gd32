/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2020-2020,Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **                   **
 **                                                            **
 **               **
 **                                                            **
 ****************************************************************
 */

/******************************************************************
*
*
*
*
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

typedef struct 
{
    uint8_t fannum;
    const PwmChannleConfig *config;
    PidObject PID;
    int32_t rpmSet;
    int32_t rpmCalculate;
} FanConfig;

static const PwmChannleConfig g_pwmChannleConfig[] = {
    {TIMER2,   TIMER_CH_0,  6800},
    {TIMER3,   TIMER_CH_2,  6800},
};
#define SIZE_PWM_CONFIG     sizeof(g_pwmChannleConfig)/sizeof(g_pwmChannleConfig[0])
FanConfig g_FanConfig[SIZE_PWM_CONFIG];

extern CapPreiodCapInfo_T g_timer_cap_info[4];

static TimerHandle_t xTimersFanWatchdog = NULL;

static void 	 vTimerFanWatchdogCallback      (xTimerHandle pxTimer);

FanConfig *fan_getConfig(int channel)
{
    if (channel >= SIZE_PWM_CONFIG){
        return NULL;
    }
    return &g_FanConfig[channel];
}
void fan_init(void)
{
	int i = 0;
	
    capture_init();
    pwm_init();

    for(i = 0; i < SIZE_PWM_CONFIG; i++)
    {
        FanConfig *pFanconfig = fan_getConfig(i);
        pFanconfig->fannum = i;
        pFanconfig->config = &g_pwmChannleConfig[i];
        pFanconfig->rpmSet = 0;
        pFanconfig->rpmCalculate = 0;

        pidInit(&pFanconfig->PID, 0, PID_UPDATE_DT);
        fan_set_duty_percent(i, 100);
    }

    xTimersFanWatchdog = xTimerCreate("Timer", 1000/portTICK_RATE_MS, pdTRUE, (void*)1, vTimerFanWatchdogCallback); 
    xTimerStart(xTimersFanWatchdog, portMAX_DELAY);	

    for(i=0; i<sizeof(g_timer_cap_info)/sizeof(g_timer_cap_info[0]); i++)
    {
        fan_set_duty_percent(i, 100);
        vTaskDelay(10);
    }
}

void fan_ctrl_loop(void)
{
    uint16_t curent_rpm = 0;
    int32_t pid_out = 0; 
    int i;

    for(uint32_t i = 0; i < SIZE_PWM_CONFIG; i++) {
        FanConfig *pFan = fan_getConfig(i);
        if(pFan == NULL)
        {
            continue;
        }
        if(fan_get_rotate_rpm(i, &curent_rpm) == false){
            continue;
        }
        pid_out = pidUpdate(&pFan->PID, pFan->rpmSet - curent_rpm);
        pFan->rpmCalculate = pidOutLimit(pFan->rpmCalculate + pid_out, 0, FAN_PWM_MAX_DUTY_VALUE);
        fan_set_rotate_rpm(i, pid_out);
    }
}

void fan_ctrl_loop_task(void)
{
    uint16_t curent_rpm = 0;
    int32_t pid_out = 0;
    int i;

	UNUSED(curent_rpm);
	UNUSED(pid_out);
    while(1)
    {
        // for(i=0; i<sizeof(g_fan_rpm_set)/sizeof(uint32_t); i++)
        // {
        //     fan_get_rotate_rpm(i, &curent_rpm);
        //     pid_out = pidUpdate(&g_fan_pid[i], g_fan_rpm_set[i] - curent_rpm);
        //     g_fan_rpm_calculate[i] = pidOutLimit(g_fan_rpm_calculate[i]+pid_out, 0, FAN_PWM_MAX_DUTY_VALUE);
        //     fan_set_rotate_rpm(i, pid_out);
        // }
        vTaskDelay(1000);

        // vTaskDelay(PID_UPDATE_DT);
    }
}
bool fan_get_rotate_rpm(unsigned char channel, uint16_t *fan_rpm)
{
    uint32_t cap_value;
	
    if(g_timer_cap_info[channel].is_valid == false)
    {
        *fan_rpm = 0;
        return false;
    }

    capture_get_value(channel, &cap_value);
    *fan_rpm = (uint32_t)1000000 * 30 / cap_value;
    if (*fan_rpm > 10000 || *fan_rpm < 100)
    {
        return false;
    }

    return true;
}

void fan_set_rotate_rpm(int channel, uint32_t rpm)
{
    FanConfig *pFan = fan_getConfig(channel);
    if(pFan == NULL)
    {
        return;
    }
    pFan->rpmSet = rpm;
}

/* convert to rpm */
void fan_set_duty(int channel, unsigned char duty)
{
    FanConfig *pFan = fan_getConfig(channel);
    if(pFan == NULL)
    {
        return;
    }
    pFan->rpmSet = duty * (pFan->config->maxRotateRpm) /100;
}

/* pwm raw duty percent */
void fan_set_duty_percent(int channel, unsigned char duty)
{
    uint32_t duty_value = 0;

    duty_value = duty * FAN_PWM_MAX_DUTY_VALUE/100;
  
    FanConfig *pFanconfig = fan_getConfig(channel);
    if(pFanconfig == NULL)
    {
        return;
    }
    timer_channel_output_pulse_value_config(pFanconfig->config->timerPeriph, pFanconfig->config->timerCh, duty_value);
}

static void vTimerFanWatchdogCallback(xTimerHandle pxTimer)
{
    int i;
    uint32_t ulTimerID;	
	ulTimerID = (uint32_t) pvTimerGetTimerID( pxTimer );         
	
	switch ( ulTimerID )
	{
    case 1: // software timer1
        for(i=0; i<sizeof(g_timer_cap_info)/sizeof(CapPreiodCapInfo_T); i++)
        {
            g_timer_cap_info[i].cap_no_update_cnt++;  
            if(g_timer_cap_info[i].cap_no_update_cnt >= 2) // 2s  fan cap err
            {
                g_timer_cap_info[i].is_valid = false;
            }  
        }                                   
        break;
    default:
      break;			
	}	
    
}
