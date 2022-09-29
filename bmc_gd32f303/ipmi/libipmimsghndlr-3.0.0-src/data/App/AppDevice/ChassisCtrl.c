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
			printf("Sorry, CMD doesn't support it yet");
            break;
    }
}








