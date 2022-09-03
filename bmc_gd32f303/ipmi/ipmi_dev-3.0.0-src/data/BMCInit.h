/******************************************************************
 ******************************************************************
 ***                                                             **
 ***    (C)Copyright 2012, American Megatrends Inc.              **
 ***                                                             **
 ***    All Rights Reserved.                                     **
 ***                                                             **
 ***    5555 , Oakbrook Pkwy, Norcross,                          **
 ***                                                             **
 ***    Georgia - 30093, USA. Phone-(770)-246-8600.              **
 ***                                                             **
 ******************************************************************
 ******************************************************************
 ******************************************************************
 *
 * BMCInit.h
 * BMC Initialization routines
 *
 ******************************************************************/

#ifndef _BMCINIT_H_
#define _BMCINIT_H_

#include "Types.h"
#include "Debug.h"
#include "OSPort.h"
#include "PendTask.h"
#include "PEF.h"
#include "SensorAPI.h"
#include "featuredef.h"
#include "BMCInfo.h"
#include "IPMIConf.h"
#include "AMIDevice.h"
#include "ChassisCtrl.h"
#include "NVRAccess.h"
#include "Platform.h"
#include "GUID.h"
#include "SDR.h"
#include "SEL.h"
#include "FRU.h"
#include "Sensor.h"

#define PWR_ALWAYS_OFF             0x00
#define PWR_RESTORED               0x01
#define PWR_ALWAYS_ON              0x02
#define PWR_NO_CHANGE              0x03
#define PREV_POWER_STATE        0x01

/**
*@fn PreInitMsgHndlr
*@brief Initializtions to be done before invoking MsgHndlr task
*@param BMCInst - BMC Instance
*/
extern int PreInitMsgHndlr(int BMCInst);

/**
*@fn PostInitMsgHndlr
*@brief Initializtions to be done after invoking MsgHndlr task
*@param BMCInst - BMC Instance
*/
extern int PostInitMsgHndlr(int BMCInst);

#endif
