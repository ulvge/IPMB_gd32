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
 * chassis.c
 * Chassis commands
 *
 *  Author: Rama Bisa <ramab@ami.com>
 *          
 ******************************************************************/
#define UNIMPLEMENTED_AS_FUNC

#include "Types.h"
#include "Chassis.h"
#include "IPMI_ChassisDevice.h"
#include "ChassisDevice.h"
#include "Support.h"

/*** Global Variables ***/

 const CmdHndlrMap_T g_Chassis_CmdHndlr [] = 
{
    /*--------------------- Chassis Commands ---------------------------------*/
#if CHASSIS_DEVICE == 1
    // { CMD_GET_CHASSIS_CAPABILITIES,     PRIV_USER,      GET_CHASSIS_CAPABILITIES,       0x00,                               0xAAAA ,0xFFFF},
    // { CMD_GET_CHASSIS_STATUS,           PRIV_USER,      GET_CHASSIS_STATUS,             0x00,                               0xAAAA ,0xFFFF},
    { CMD_CHASSIS_CONTROL,              PRIV_OPERATOR,  CHASSIS_CONTROL,                sizeof(ChassisControlReq_T),        0xAAAA ,0xFFFF},
    // { CMD_CHASSIS_RESET,                PRIV_OPERATOR,  CHASSIS_RESET_CMD,              0x00,                               0xAAAA ,0xFFFF},
    // { CMD_CHASSIS_IDENTIFY,             PRIV_OPERATOR,  CHASSIS_IDENTIFY_CMD,           0xFF,                               0xAAAA ,0xFFFF},
    // { CMD_SET_CHASSIS_CAPABILITIES,     PRIV_ADMIN,     SET_CHASSIS_CAPABILITIES,       0xFF,                               0xAAAA ,0xFFFF},
    // { CMD_SET_POWER_RESTORE_POLICY,     PRIV_OPERATOR,  SET_POWER_RESTORE_POLICY,       sizeof(SetPowerRestorePolicyReq_T), 0xAAAA ,0xFFFF},
    // { CMD_GET_SYSTEM_RESTART_CAUSE,     PRIV_USER,      GET_SYSTEM_RESTART_CAUSE,       0x00,                               0xAAAA ,0xFFFF},
    // { CMD_SET_SYSTEM_BOOT_OPTIONS,      PRIV_OPERATOR,  SET_SYSTEM_BOOT_OPTIONS,        0xFF,                               0xAAAA ,0xFFFF},
    // { CMD_GET_SYSTEM_BOOT_OPTIONS,      PRIV_OPERATOR,  GET_SYSTEM_BOOT_OPTIONS,        sizeof (GetBootOptionsReq_T),       0xAAAA ,0xFFFF},
    // { CMD_GET_POH_COUNTER,              PRIV_USER,      GET_POH_COUNTER,                0x00,                               0xAAAA ,0xFFFF},
    // { CMD_SET_FP_BTN_ENABLES,           PRIV_ADMIN,     SET_FP_BTN_ENABLES,             sizeof(SetFPBtnEnablesReq_T),		0xAAAA ,0xFFFF},
    // { CMD_SET_POWER_CYCLE_INTERVAL, 	PRIV_ADMIN,		SET_POWER_CYCLE_INTERVAL,		sizeof(SetPowerCycleIntervalReq_T),		0xAAAA ,0xFFFF},
#endif  /* CHASSIS_DEVICE */
    { 0x00,                             0x00,           0x00,                           0x00,                               0x0000 ,           0x0000}
    
};

