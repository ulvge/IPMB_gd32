#ifndef __API_FAN_H
#define __API_FAN_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "project_select.h"
#include <stdio.h>
#include <stdbool.h>

#include "bsp_pwm.h"
#include "bsp_capture.h"
#include "pid/pid.h"


void fan_init(void);
bool fan_get_rotate_rpm(unsigned char channel, uint16_t *fan_rpm);
void fan_set_rotate_rpm(int channel, uint32_t rpm);
void fan_set_duty_percent(int channel, unsigned char duty);  // duty percent
void fan_set_duty(int channel, unsigned char duty);     // convert to rpm


void fan_ctrl_loop(void);
	 
#ifdef __cplusplus
}
#endif

#endif
