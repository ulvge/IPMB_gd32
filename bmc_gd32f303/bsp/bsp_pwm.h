#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include "project_select.h"
#include <stdio.h>


#define FAN_PWM_MAX_DUTY_VALUE        4800   // PWM max 4800

typedef struct 
{
    uint8_t fanSensorNum;
    uint16_t maxRotateRpm;
    rcu_periph_enum      	timerRcu;
    uint32_t timerPeriph;   //TIMER1
    uint8_t timerCh;        //TIMER_CH_1

    rcu_periph_enum      	gpioRcu;
    uint32_t gpioPort;       
    uint32_t pin;
    uint32_t remap;
} PwmChannleConfig;

void pwm_timer_gpio_config(const PwmChannleConfig *config);
void pwm_timer_config(const PwmChannleConfig *config);

#endif
