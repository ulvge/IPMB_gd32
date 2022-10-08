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
extern int  OBSMGetOpenBladeProps		(INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  OBSMGetAddrInfo				(INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  OBSMPlatformEvtMsg			(INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  OBSMManagedModuleBMICtrl	(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMManagedModulePayldCtrl	(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMSetSysEvntLogPolicy		(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMSetModuleActvnPolicy	(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMGetModuleActvnPolicy	(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMSetModuleActivation		(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMSetPwrLevel				(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMGetPwrLevel				(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMRenegotiatePwr			(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMSetChannelState			(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMGetChannelState			(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMSetCoolingDomPwr		(INT8U* pReq, INT8U ReqLen, INT8U* pRes);
extern int  OBSMSetCoolingDomThermState	(INT8U* pReq, INT8U ReqLen, INT8U* pRes);

#endif  /* OBSMDEVICE_H */
