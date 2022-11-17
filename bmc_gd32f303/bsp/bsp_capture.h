#ifndef __BSP_CAPTURE_H
#define __BSP_CAPTURE_H

#include "project_select.h"
#include <stdio.h>
#include <stdbool.h>
#include "api_sensor.h"

#define CAPTURE_TIMER_FREQUENCY  1000000U   
#define CAPTURE_1MIN_XSEC  	60
#define CAPTURE_TIMER_PERIOD    UINT16_MAX //在定时器被配置为输入捕获模式时，该寄存器需要被配置成一个大于用户期望值的非 0 值(例如 0xFFFF)


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

	uint8_t     is_valid :1;
	uint8_t     is_start :1;
	uint16_t cap_value_first;
	uint16_t cap_value_second;
	uint16_t period_cnt;
	uint16_t cap_no_update_cnt;

    const CaptureConfig *config;
	uint32_t total_value;
} CaptureStruct;

// Cap1  PC

void capture_init        (void);
int32_t capture_getTotalNum(void);
bool capture_get_value   (unsigned char fanSensorNum, uint32_t* cap_value);
bool capture_getIsValid(unsigned char fanSensorNum);
CaptureStruct *capture_getHandler(int idx);


#endif
