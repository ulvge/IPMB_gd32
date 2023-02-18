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
 * ChassisCtrl.h
 * Chassis control functions.
 *
 *  Author: AMI MegaRAC PM Team
 ******************************************************************/
#ifndef _CHASSIS_CTRL_H_
#define _CHASSIS_CTRL_H_
#include "Types.h"
#include "Message.h"

/**
 * @def Parameters to controle the Chassis actions
 *
**/

#define CHASSIS_CTRL_ACTION             0x01
#define ON_SYSTEM_EVENT_DETECTED        0x02
#define ON_POWER_EVENT_DETECTED         0x03
#define ON_SET_RESTART_CAUSE            0x04

#define GPIO_ACTIVE_PULSE_TIME_MS 100

/**
 * @brief Initialize Chassis Control module.
 * @return 0 if success, -1 if error.
**/
extern void OnSetRestartCause (INT8U u8SysRestartCause, INT8U u8MadeChange,int BMCInst);
extern void ChassisCtrl(SamllMsgPkt_T *msg);


#endif /*_CHASSIS_CTRL_H_*/
