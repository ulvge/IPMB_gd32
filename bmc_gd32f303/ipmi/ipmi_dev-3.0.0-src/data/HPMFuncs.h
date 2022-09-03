/* ***************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2012, American Megatrends Inc.             **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200,  Norcross,       **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************/
/****************************************************************
 *
 * HPMFuncs.h
 * HPM firmware upgrade related functions
 *
 * Author: Joey Chen <JoeyChen@ami.com.tw>
 *
 *****************************************************************/
#ifndef HPMFUNCS_H
#define HPMFUNCS_H

#include "Types.h"
#include "IPMI_HPMCmds.h"

#define INVALID_COMPONENT_ID 0xFF

/* HPM Timer relating variables*/
#define HPM_FLASH_SUCCEEDED	 1
#define HPM_UPLOAD_BLK_INPROGRESS	 2
#define HPM_FLASH_FW_INPROGRESS	 3
#define HPM_MSECS_PER_TICK		1000
/* Timer timeouts for diff state*/
#define HPM_UPLOAD_BLK_TIMER_COUNT	60
#define HPM_FLASH_FW_TIMER_COUNT	900


BOOL IsFwUpSupportIfc(int BMCInst);
INT8U GetSelfTestResultByte(INT8U ByteNum, int BMCInst);

HPMStates_E GetFwUpgState(INT8U ComponentID);
void UpdateHPMStatus (INT8U CompCode, INT8U Cmd, INT8U CmdDuration);
int GetHPMStatus (HPMCmdStatus_T *HPMStatus);

BOOL IsCachedComponentID(INT8U ComponentID);
BOOL IsLastHPMCmdCorrect(INT8U LastCmd);

int VerifyImageLength(INT32U ImageLength);
 
int InitBackupComponents(INT8U Components);
int InitPreComponents(INT8U Components);
int InitUpload(INT8U UpgradeAction, INT8U Component);

int HandleFirmwareBlock(INT8U BlkNum, INT8U *Data, INT16U Len);
int HandleUploadedFirmware(void);

INT8U HandleAbortFirmwareUpgrade(void);

BOOL HandleActivateFirmware(void);

INT8U GetRollbackComponents(void);
int HandleManualRollback(void);

void RevertToInitState(INT8U ComponentID);
INT8U GetHPMActCompsFromBootParam(void);
int SetBootSelectorToBootParam(char image);
void* HPMTimerTask(void *pArg);
void SetHPMFlashStatus(INT8U status);
void SetHPMTimerCnt(INT8U secs);
#endif  /* HPMFUNCS_H */
