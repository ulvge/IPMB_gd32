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


uint32_t s_max_rotate_rpm[4] = {0};
uint32_t s_max_duty_value[4] = {0};

PidObject g_fan_pid[4];
int32_t  g_fan_rpm_set[4] = {0};
int32_t  g_fan_rpm_calculate[4] = {0};

extern CapPreiodCapInfo_T g_timer_cap_info[4];

static TimerHandle_t xTimersFanWatchdog = NULL;

static bool      fan_set_para_max_rpm           (int channel, uint32_t rpm);
static void 	 vTimerFanWatchdogCallback      (xTimerHandle pxTimer);

void fan_init(void)
{
	int i = 0;
	
    capture_init();
    pwm_init();
    fan_set_para_max_rpm(1, 6800);
    fan_set_para_max_rpm(2, 6800);

    for(i=0; i<sizeof(g_fan_pid)/sizeof(PidObject); i++)
    {
        pidInit(&g_fan_pid[i], 0, PID_UPDATE_DT);
        g_fan_rpm_calculate[i] = 0;
        g_fan_rpm_set[i] = 0;
    }
}

void fan_ctrl_loop(void)
{
    uint16_t curent_rpm = 0;
    int32_t pid_out = 0;
    int i;

    xTimersFanWatchdog = xTimerCreate("Timer", 1000/portTICK_RATE_MS, pdTRUE, (void*)1, vTimerFanWatchdogCallback); 
    xTimerStart(xTimersFanWatchdog, portMAX_DELAY);	

    for(i=0; i<sizeof(g_timer_cap_info)/sizeof(g_timer_cap_info[0]); i++)
    {
        fan_set_duty_percent(i, 100);
        vTaskDelay(10);
    }

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

static bool fan_set_para_max_rpm(int channel, uint32_t rpm)
{
    if (channel > sizeof(s_max_rotate_rpm) / sizeof(s_max_rotate_rpm[0]))
    {
        return false;
    }
    s_max_rotate_rpm[channel] = rpm;
    return true;
}

void fan_set_rotate_rpm(int channel, uint32_t rpm)
{
    if(channel >=0 && channel < sizeof(g_fan_rpm_set)/sizeof(int32_t))
    {
        g_fan_rpm_set[channel] = rpm;
    }
}

/* convert to rpm */
void fan_set_duty(int channel, unsigned char duty)
{
    if(channel >=0 && channel < sizeof(g_fan_rpm_set)/sizeof(int32_t))
    {
        g_fan_rpm_set[channel] = duty*s_max_rotate_rpm[channel]/100;
    }	
}

/* pwm raw duty percent */
void fan_set_duty_percent(int channel, unsigned char duty)
{
    pwm_set_duty_percent(channel, duty);
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
