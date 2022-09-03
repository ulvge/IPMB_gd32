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
 * @file	OBSMDevice.h
 * @author	Hari Lakshmanan <haril@ami.com>
 * @brief	OBSM Commands
 ****************************************************************/

#ifndef OBSMDEVICE_H
#define OBSMDEVICE_H

#include "Types.h"

/*** Function Prototypes ***/
extern int  OBSMGetOpenBladeProps		(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int  OBSMGetAddrInfo				(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int  OBSMPlatformEvtMsg			(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int  OBSMManagedModuleBMICtrl	(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMManagedModulePayldCtrl	(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMSetSysEvntLogPolicy		(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMSetModuleActvnPolicy	(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMGetModuleActvnPolicy	(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMSetModuleActivation		(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMSetPwrLevel				(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMGetPwrLevel				(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMRenegotiatePwr			(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMSetChannelState			(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMGetChannelState			(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMSetCoolingDomPwr		(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);
extern int  OBSMSetCoolingDomThermState	(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes);

#endif  /* OBSMDEVICE_H */
