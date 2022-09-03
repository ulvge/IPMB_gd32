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

extern int GetTargetUpgradeCapablities  (_NEAR_ INT8U * pReq,INT8U ReqLen,_NEAR_ INT8U * pRes, _NEAR_ int BMCInst); 
extern int GetComponentProperties       (_NEAR_ INT8U * pReq,INT8U ReqLen,_NEAR_ INT8U * pRes, _NEAR_ int BMCInst); 
extern int InitiateUpgradeAction (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst); 
extern int QuerySelfTestResults (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst); 

extern int AbortFirmwareUpgrade (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst); 
extern int UploadFirmwareBlock (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst);
extern int FinishFirmwareUpload (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst); 
extern int GetUpgradeStatus (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst); 
extern int ActivateFirmware (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst);
extern int QueryRollbackStatus (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst);
extern int InitiateManualRollback (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst);
 
#endif  /* HPMCMDS_H */
