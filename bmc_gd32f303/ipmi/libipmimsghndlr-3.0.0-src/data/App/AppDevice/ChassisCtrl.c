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

#include "ChassisCtrl.h"
#include "ChassisDevice.h"
#include "Message.h"
#include "bsp_gpio.h"
#include "debug_print.h"
#include "freertos.h"
#include "libipmi.h"
#include "main.h"
#include "timers.h"
#include <stdbool.h>
#include <stdio.h>

static void ChassisCtrlTimerCallBack(xTimerHandle pxTimer)
{
    CHASSIS_CMD_CTRL cmd = (CHASSIS_CMD_CTRL)((uint32_t)pvTimerGetTimerID(pxTimer));

    switch (cmd) {
        case CHASSIS_SOFT_OFF:
        case CHASSIS_POWER_OFF:
            LOG_W("ChassisCtrl : CHASSIS_POWER_OFF\n");
            GPIO_setPinStatus(GPIO_ALIAS_TO_CPLD_POWER_ONOFF, DISABLE);
            break;
        case CHASSIS_POWER_ON:
            LOG_W("ChassisCtrl : CHASSIS_POWER_ON\n");
            GPIO_setPinStatus(GPIO_ALIAS_TO_CPLD_POWER_ONOFF, DISABLE);
            break;
        case CHASSIS_POWER_RESET:
            LOG_W("ChassisCtrl : CHASSIS_POWER_RESET\n");
            GPIO_setPinStatus(GPIO_ALIAS_TO_CPLD_RESET, DISABLE);
            // NVIC_SystemReset();
            break;
        default:
            LOG_E("Sorry, CMD doesn't support it yet\n");
            break;
    }
    xTimerDelete(pxTimer, 200);
}

static BaseType_t ChassisCtrlTimerCreate(INT32U cmd, INT32U delayMs)
{
    TimerHandle_t xTimersIpmiReset = xTimerCreate("TimerIpmiReset", delayMs / portTICK_RATE_MS, pdFALSE,
                                                  (void *)cmd, ChassisCtrlTimerCallBack);
    return xTimerStart(xTimersIpmiReset, portMAX_DELAY);
}
void ChassisCtrl(SamllMsgPkt_T *msg)
{
    INT32U delayMs = 100; // CPLD delay deglitch = 0xffff(clk=1k)  == 65ms
    switch (msg->Cmd) {
        case CHASSIS_SOFT_OFF:
        case CHASSIS_POWER_OFF:
            if (DevPower_IsPowerGood()) {
                GPIO_setPinStatus(GPIO_ALIAS_TO_CPLD_POWER_ONOFF, ENABLE);
                break;
            }
            LOG_W("ChassisCtrl : power off already\n");
            return;
        case CHASSIS_POWER_ON:
            if (!DevPower_IsPowerGood()) {
                GPIO_setPinStatus(GPIO_ALIAS_TO_CPLD_POWER_ONOFF, ENABLE);
                break;
            }
            LOG_W("ChassisCtrl : power on already\n");
            return;
        case CHASSIS_POWER_RESET:
            GPIO_setPinStatus(GPIO_ALIAS_TO_CPLD_RESET, ENABLE);
            delayMs = GPIO_ACTIVE_PULSE_TIME_MS;
            break;
        default:
            LOG_E("Sorry, CMD doesn't support it yet\n");
            return;
    }

    if (ChassisCtrlTimerCreate(msg->Cmd, delayMs) == pdFAIL) {
        LOG_E("ChassisCtrl : creteTimer failed\n");
    }
}
