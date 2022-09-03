#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include "gd32f20x.h"
#include <stdio.h>


#define FAN_PWM_MAX_DUTY_VALUE        4800   // PWM max 4800

void pwm_init (void);
void pwm_set_duty_percent(unsigned char fannum, unsigned char percent);
void pwm_set_duty_raw(unsigned char fannum, uint32_t value);

#endif
