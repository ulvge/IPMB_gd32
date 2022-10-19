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
extern int GetFan (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetFan (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetBladId (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int UpdateFirmware(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst);

extern int GetCPUInfo(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst);
extern int GetBMCInfo(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst);
/** @} */


#endif  /* OEM_H */


