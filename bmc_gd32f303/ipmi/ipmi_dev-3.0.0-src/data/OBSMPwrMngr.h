/****************************************************************
 **                                                            **
 **    (C)Copyright 2007-2008, American Megatrends Inc.        **
 **                                                            **
 **                   All Rights Reserved.                     **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200, Norcross,        **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************/

 /****************************************************************
 * @file	OBSMPwrMngr.h
 * @author	Velu	<velmuruganv@amiindia.co.in>
 * @brief
 ****************************************************************/
#ifndef OBSMPWRMNGR_H
#define OBSMPWRMNGR_H

#include "Types.h"

/*** Definitions and Macros ***/
#define PARAM_OBSM_PWR_TIMER_TICK				1
#define PARAM_OBSM_SANCTION_PWR					2
#define PARAM_OBSM_SURRENDER_PWR				3
#define PARAM_OBSM_PS_STATE_CHANGE				4

/**
 * @brief Get the devices in the power domain
 * @param domNum is the cooling domain number
 * @return count of devices
**/
extern INT8U OBSM_GetPwrDomDeviceCount (INT8U domNum, int BMCInst);

/**
 * @brief Process the blade pending power requests
 * @param pBlade is the Blade object
 * @return 0 if success, -1 if error
**/
extern int OBSM_ProcessPendingBladePwrReq (BladeInfo_T* pBlade, int BMCInst);

/**
 * @brief Process the OBSM fan pending power requests
 * @param pFan is the fan object
 * @return None
**/
extern int OBSM_ProcessPendingFanPwrReq (FanInfo_T *pFan, int BMCInst);

/**
 * @brief Process the blade power needs during M2 to M3 transition
 * @param pBlade is the Blade object
 * @return 0 if success, -1 if error
**/
extern int OBSM_ProcessM2ToM3BladePwr (BladeInfo_T* pBlade, int BMCInst);

/**
 * @brief Process the blade power needs during M6 to M1 transition
 * @param pBlade is the Blade object
 * @return 0 if success, -1 if error
**/
extern int OBSM_ProcessM6ToM1BladePwr (BladeInfo_T* pBlade, int BMCInst);

/**
 * @brief Get the current power and allocate it from pool
 * @param pBlade is the Blade object
 * @return 0 if success, -1 if error
**/
extern int OBSM_SanctionMissedStateBladePwr (BladeInfo_T* pBlade, int BMCInst);

/**
 * @brief Give the allocated power back to the pool
 * @param pBlade is the Blade object
 * @return 0 if success, -1 if error
**/
extern int OBSM_SurrenderMissedStateBladePwr (BladeInfo_T* pBlade, int BMCInst);

/**
 * @brief Process the pwr routines on removal of a blade
 * @param pBlade is the Blade object
 * @return 0 if success, -1 if error
**/
extern int OBSM_ProcessPwrOnBladeRemoval(BladeInfo_T* pBlade, int BMCInst);

/**
 * @brief Get the power from power pool
 * @param pFan is the Fan object
 * @param pwr is the amount of power required
 * @return 0 if success, -1 if error
**/
extern int OBSM_ProcessSanctioningFanPwr (FanInfo_T *pFan, INT16U reqPwr, int BMCInst);

/**
 * @brief Give the power back to the chassis pool
 * @param pFan is the Fan object
 * @param pwr is the amount of power to surrender
 * @return 0 if success, -1 if error
**/
extern int OBSM_ProcessSurrenderingFanPwr (FanInfo_T* pFan, INT16U pwr, int BMCInst);

/**
 * @brief Routine called while the power supply insertion is detected
 * @param pwrDomNum is the power domain number
 * @param pwrSupplySlotNum is the slot number of the power supply
 * @return 0 if success, -1 if error
**/
extern int OBSM_ProcessPwrSupplyInsertion (INT8U pwrDomNum,INT8U pwrSupplySlotNum, int BMCInst);

/**
 * @brief Invoked by OBSM task on arrival of power supply insertion/removal event
 * @param pEvent is the SEL event message
 * @return 0 if success, -1 if error
**/
extern int OBSM_PwrSupplyPresenceEventHndlr (SELEventRecord_T *pEvent, int BMCInst);

/**
 * @brief Method to intialize the chassis power specific structs
 * @param None
 * @return 0 if success, -1 if error
**/
extern int OBSM_InitChassisPwrInfo (int BMCInst);

/**
 * @brief OBSM Power task.
**/
extern void* OBSMPwrTask (void *pArg);

#endif /*OBSMPWRMNGR_H*/
