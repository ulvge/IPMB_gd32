#ifndef __BSP_CAPTURE_H
#define __BSP_CAPTURE_H

#include "project_select.h"
#include <stdio.h>
#include <stdbool.h>
#include "api_sensor.h"

typedef struct
{
    SENSOR_ENUM             fanSensorNum;
    rcu_periph_enum      	timerRcu;//RCU_TIMER1
    uint32_t timerPeriph;   //TIMER1
    uint8_t timerCh;        //TIMER_CH_1
    uint8_t timerIntCh;     //TIMER_INT_CH1

    rcu_periph_enum      	gpioRcu;
    uint32_t gpioPort;
    uint32_t pin;
    uint32_t remap;
} CaptureConfig;
typedef struct 
{
    uint8_t idx;
    const CaptureConfig *config;
    
	bool     is_start;
	uint16_t cap_value_first;
	uint16_t cap_value_second;
	uint16_t period_cnt;
	uint16_t cap_no_update_cnt;
	bool     is_valid;

	uint32_t total_value;
} CaptureStruct;

// Cap1  PC

void capture_init        (void);
int32_t capture_getTotalNum(void);
bool capture_get_value   (unsigned char fanSensorNum, uint32_t* cap_value);
bool capture_getIsValid(unsigned char fanSensorNum);
CaptureStruct *capture_getHandler(int idx);


#endif
