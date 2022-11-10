/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2005-2006, American Megatrends Inc.        **
 ***                                                            **
 ***            All Rights Reserved.                            **
 ***                                                            **
 ***        6145-F, Northbelt Parkway, Norcross,                **
 ***                                                            **
 ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 ***                                                            **
 *****************************************************************
 *****************************************************************
 ******************************************************************
 *
 * ChassisCtrl.c
 * Chassis control functions.
 *
 *  Author: AMI MegaRAC PM Team
 ******************************************************************/



#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "Message.h"
#include "ChassisDevice.h"
#include "libipmi.h"
#include "bsp_gpio.h"
#include "freertos.h"
#include "timers.h"
#include "debug_print.h"
					 
#define GPIO_ACTIVE_PULSE_TIME_MS 1500


static void ChassisCtrlTimerCallBack(xTimerHandle pxTimer)
{
	CHASSIS_CMD_CTRL cmd = (CHASSIS_CMD_CTRL)((uint32_t)pvTimerGetTimerID(pxTimer));

    switch (cmd)
    {
        case CHASSIS_SOFT_OFF :
        case CHASSIS_POWER_OFF :
            GPIO_setPinStatus(GPIO_OUT_CPU_POWER_OFF, DISABLE);    
			LOG_W("ChassisCtrl : CHASSIS_POWER_OFF\n");
			break;
        case CHASSIS_POWER_ON :
            GPIO_setPinStatus(GPIO_OUT_CPU_POWER_ON, DISABLE);   
			LOG_W("ChassisCtrl : CHASSIS_POWER_ON\n");
			break;
        case CHASSIS_POWER_RESET :
            GPIO_setPinStatus(GPIO_OUT_CPU_RESET, DISABLE);   
			LOG_W("ChassisCtrl : CHASSIS_POWER_RESET\n");
	        NVIC_SystemReset();
			break;
        default : 
			LOG_E("Sorry, CMD doesn't support it yet\n");
            break;
    }
    xTimerDelete(pxTimer, 200);
}

static BaseType_t ChassisCtrlTimerCreate(INT32U cmd, INT32U delayMs)
{
    TimerHandle_t xTimersIpmiReset = xTimerCreate("TimerIpmiReset", delayMs/portTICK_RATE_MS, pdFALSE, 
                                    (void*)cmd, ChassisCtrlTimerCallBack);
    return xTimerStart(xTimersIpmiReset, portMAX_DELAY);
}
void ChassisCtrl(SamllMsgPkt_T *msg)
{
    switch (msg->Cmd)
    {   
        case CHASSIS_SOFT_OFF :
        case CHASSIS_POWER_OFF :
            GPIO_setPinStatus(GPIO_OUT_CPU_POWER_OFF, ENABLE);
			break;
        case CHASSIS_POWER_ON :
            GPIO_setPinStatus(GPIO_OUT_CPU_POWER_ON, ENABLE);
			break;
        case CHASSIS_POWER_RESET :
            GPIO_setPinStatus(GPIO_OUT_CPU_RESET, ENABLE);
			break;
        default :
			LOG_E("Sorry, CMD doesn't support it yet\n");
            return;
    }
	
    if (ChassisCtrlTimerCreate(msg->Cmd, GPIO_ACTIVE_PULSE_TIME_MS) == pdFAIL){
	    LOG_E("ChassisCtrl : creteTimer failed\n");
    }
}








