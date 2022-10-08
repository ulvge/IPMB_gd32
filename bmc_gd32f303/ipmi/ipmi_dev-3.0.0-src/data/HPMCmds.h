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
 * HPMCmds.h
 * HPM Command functions
 *
 * Author: Joey Chen <JoeyChen@ami.com.tw>
 *
 *****************************************************************/
#ifndef HPMCMDS_H
#define HPMCMDS_H

#include "Types.h"

/** Extern Definitions **/

extern int GetTargetUpgradeCapablities  (INT8U * pReq,INT8U ReqLen,INT8U * pRes, int BMCInst); 
extern int GetComponentProperties       (INT8U * pReq,INT8U ReqLen,INT8U * pRes, int BMCInst); 
extern int InitiateUpgradeAction (INT8U* pReq, INT8U ReqLen, INT8U* pRes, int BMCInst); 
extern int QuerySelfTestResults (INT8U* pReq, INT8U ReqLen, INT8U* pRes, int BMCInst); 

extern int AbortFirmwareUpgrade (INT8U* pReq, INT8U ReqLen, INT8U* pRes, int BMCInst); 
extern int UploadFirmwareBlock (INT8U* pReq, INT8U ReqLen, INT8U* pRes, int BMCInst);
extern int FinishFirmwareUpload (INT8U* pReq, INT8U ReqLen, INT8U* pRes, int BMCInst); 
extern int GetUpgradeStatus (INT8U* pReq, INT8U ReqLen, INT8U* pRes, int BMCInst); 
extern int ActivateFirmware (INT8U* pReq, INT8U ReqLen, INT8U* pRes, int BMCInst);
extern int QueryRollbackStatus (INT8U* pReq, INT8U ReqLen, INT8U* pRes, int BMCInst);
extern int InitiateManualRollback (INT8U* pReq, INT8U ReqLen, INT8U* pRes, int BMCInst);
 
#endif  /* HPMCMDS_H */
