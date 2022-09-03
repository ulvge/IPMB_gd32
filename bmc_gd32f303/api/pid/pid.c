/*
******************************************************************************
* @file    pid.c
* @author  
* @version V1.0
* @date    
* @brief   
******************************************************************************
* @attention
*
******************************************************************************
*/

#include  "pid/pid.h"
/*
*********************************************************************************************************
*                                             VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            FUNCTIONS
*********************************************************************************************************
*/


void pidInit(PidObject* pid, const int32_t desired, const int32_t dt)
{
	pid->error     = 0;
	pid->prevError = 0;
	pid->integ     = 0;
	pid->deriv     = 0;
	pid->desired = desired;
	pid->kp = 2048;   // raw 2   x1024
	pid->ki = 20;		 // raw 0.02  x1024
	pid->kd = 0;
	pid->iLimit    = PID_INTEGRATION_LIMIT;
	pid->iLimitLow = -PID_INTEGRATION_LIMIT;
	pid->dt        = dt; //        x1024
}

int32_t pidUpdate(PidObject* pid, const int32_t error)
{
	int32_t output=0;

	pid->error = error;   

	pid->integ += pid->error * pid->dt;
	if (pid->integ > pid->iLimit)
	{
		pid->integ = pid->iLimit;
	}
	else if (pid->integ < pid->iLimitLow)
	{
		pid->integ = pid->iLimitLow;
	}

	//pid->deriv = (pid->error - pid->prevError) / pid->dt;

	pid->outP = pid->kp * pid->error;
	pid->outI = pid->ki * pid->integ;
	//pid->outD = pid->kd * pid->deriv;

	output = (pid->outP + (pid->outI>>10))>>10;// + pid->outD;

	pid->prevError = pid->error;

	return output;
}

int32_t pidOutLimit(const int32_t value, const int32_t lowLimit, const int32_t highLimit)
{
	int32_t res = 0;

	if(value <= lowLimit){
		res = lowLimit;
	}else if(value >= highLimit){
		res = highLimit;
	}else{
		res = value;
	}

	return res;
}

void pidSetDt(PidObject* pid, const int32_t dt) 
{
    pid->dt = dt;
}

void pidSetIntegralLimit(PidObject* pid, const int32_t limit) 
{
    pid->iLimit = limit;
}

void pidSetIntegralLimitLow(PidObject* pid, const int32_t limitLow) 
{
    pid->iLimitLow = limitLow;
}

void pidReset(PidObject* pid)
{
	pid->error     = 0;
	pid->prevError = 0;
	pid->integ     = 0;
	pid->deriv     = 0;
}

/* --------------------------------------end of file--------------------------------------- */
