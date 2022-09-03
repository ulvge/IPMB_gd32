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
 * sensor.h
 * Sensor functions.
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 *          
 ******************************************************************/
#ifndef OEM_H
#define OEM_H

#include "Types.h"
#include "MsgHndlr.h"
#include "IPMI_Oem.h"

extern const CmdHndlrMap_T g_Oem_CmdHndlr [];

/**
 * @defgroup sdc Sensor Device Commands
 * @ingroup senevt
 * IPMI Sensor Device interface command handlers. These commands are 
 * used for configuring hysterisis and thresholds and reading
 * sensor values.
 * @{
**/
extern int SetFan (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int UpdateFirmware(_NEAR_ INT8U *pReq, INT8U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ int BMCInst);

extern int GetCPUInfo(_NEAR_ INT8U *pReq, INT8U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ int BMCInst);
extern int GetBMCInfo(_NEAR_ INT8U *pReq, INT8U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ int BMCInst);
/** @} */


#endif  /* OEM_H */


