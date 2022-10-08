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
 * @file	AMIOBSMDevice.h
 * @author	Velu	<velmuruganv@amiindia.co.in>
 * @brief
 ****************************************************************/

#ifndef AMIOBSMDEVICE_H
#define AMIOBSMDEVICE_H

#include "Types.h"

/*** Function Prototypes ***/
extern int   AMIGetSlotMapInfo           (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   AMIGetSlotInfo              (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   AMIGetPwrInfo               (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   AMIGetPwrDomInfo            (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   AMIGetPwrSupplyInfo         (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   AMIGetCoolingInfo           (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   AMIGetCoolingDomInfo        (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   AMIGetFanInfo               (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   AMIGetBladeStatus           (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   AMIEthRestartAll            (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);

extern int   DbgGetBladeInfo             (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   DbgGetChassisPwrInfo        (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   DbgGetChassisCoolingInfo    (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   DbgBladeInsRemEvent         (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   DbgPSStateChangeEvent       (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   DbgFanStateChangeEvent      (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int   DbgThermalStateChangeEvent  (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);

#endif
