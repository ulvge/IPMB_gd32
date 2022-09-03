/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2002-2012, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200, Norcross,        **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * HPM.c
 * HPM Net Function Extension Command Handlerr
 *
 * Author: Joey Chen <joeychen@ami.com.tw>
 *
 *****************************************************************/
#define UNIMPLEMENTED_AS_FUNC

#include "Types.h"
#include "MsgHndlr.h"
#include "Support.h"

#include "IPMI_GroupExtn.h"
#include "IPMI_HPM.h"
#include "IPMI_HPMCmds.h"
#include "HPM.h"
#include "HPMCmds.h"

/*** Global Variables ***/
const CmdHndlrMap_T g_HPM_CmdHndlr [] =
{
#if PICMG_DEVICE == 1
    { CMD_GET_TARGET_UPLD_CAPABLITIES,  PRIV_USER,	GET_TARGET_UPLD_CAPABLITIES,  	sizeof(GetTargetUpgradeCapablitiesReq_T),   0xAAAA, 0xFFFF },
    { CMD_GET_COMPONENT_PROPERTIES,     PRIV_USER,	GET_COMPONENT_PROPERTIES,     	sizeof(GetComponentPropertiesReq_T),        0xAAAA, 0xFFFF },
    { CMD_INITIATE_UPG_ACTION,      	PRIV_USER,  INITIATE_UPGRADE_ACTION,        sizeof(InitiateUpgActionReq_T),             0xAAAA, 0xFFFF },
#if IPM_DEVICE == 1
    { CMD_QUERY_SELF_TEST_RESULTS,      PRIV_USER,  QUERY_SELF_TEST_RESULTS,        sizeof(QuerySelfTestResultsReq_T),          0xAAAA, 0xFFFF },
#endif	/* IPM DEVICE */
    { CMD_ABORT_FIRMWARE_UPGRADE,      	PRIV_USER,  ABORT_FIRMWARE_UPGRADE,         sizeof(AbortFirmwareUpgradeReq_T),          0xAAAA, 0xFFFF },
    { CMD_UPLOAD_FIRMWARE_BLOCK,      	PRIV_USER,  UPLOAD_FIRMWARE_BLOCK,          0xFF,                                       0xAAAA, 0xFFFF },
    { CMD_FINISH_FIRMWARE_UPLOAD,      	PRIV_USER,  FINISH_FIRMWARE_UPLOAD,         sizeof(FinishFWUploadReq_T),                0xAAAA, 0xFFFF },
    { CMD_GET_UPGRADE_STATUS,      	    PRIV_USER,  GET_UPGRADE_STATUS,             sizeof(GetUpgradeStatusReq_T),              0xAAAA, 0xFFFF },
    { CMD_ACTIVATE_FIRMWARE,      	    PRIV_USER,  ACTIVATE_FIRMWARE,              sizeof(ActivateFWReq_T),                    0xAAAA, 0xFFFF },
    { CMD_QUERY_ROLLBACK_STATUS,        PRIV_USER,  QUERY_ROLLBACK_STATUS,          sizeof(QueryRollbackStatusReq_T),           0xAAAA, 0xFFFF },
    { CMD_INITIATE_MANUAL_ROLLBACK,     PRIV_USER,  INITIATE_MANUAL_ROLLBACK,       sizeof(InitiateManualRollbackReq_T),        0xAAAA, 0xFFFF },
    
#endif	/* PICMG_DEVICE */
    { 0x00,								0x00,		0x00,							0x00,                                       0x0000, 0x0000 }
};
