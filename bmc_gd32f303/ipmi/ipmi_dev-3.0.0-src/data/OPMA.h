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
 * OPMACommands.h
 * Header file for OPMACommands.c
 *
 *  Author: Shivaranjani Chelladurai <shivaranjanic@ami.com>
 *
 ******************************************************************/

#ifndef __OPMA__H__
#define __OPMA__H__

#include "MsgHndlr.h"


/* OPMA Commands Handler */
extern const CmdHndlrMap_T  g_OPMA_CmdHndlr1 [];
extern const CmdHndlrMap_T  g_OPMA_CmdHndlr2 [];




extern int SetSensorReadingOffset     (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSensorReadingOffset     (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetSystemTypeIdentifier    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSystemTypeIdentifier    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetmCardCapabilities       (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int ClearCMOS                  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetLocalAccessLockOutState (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetLocalAccessLockOutState (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSupportedHostIDs        (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);

#endif //__OPMA_COMMANDS_H__
