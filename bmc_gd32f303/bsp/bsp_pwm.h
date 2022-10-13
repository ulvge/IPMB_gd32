#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include "project_select.h"
#include <stdio.h>


#define FAN_PWM_MAX_DUTY_VALUE        4800   // PWM max 4800

typedef struct 
{
    uint32_t timerPeriph;   //TIMER1
    uint8_t timerCh;        //TIMER_CH_1
    uint16_t maxRotateRpm;
} PwmChannleConfig;

void pwm_init (void);

#endif
