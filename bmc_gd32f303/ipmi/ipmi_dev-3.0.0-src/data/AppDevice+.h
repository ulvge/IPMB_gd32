/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * AppDevice.h
 * AppDevice Commands Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#ifndef APPDEVICE_PLUS_H
#define APPDEVICE_PLUS_H

#include "Types.h"

/*** Function Prototypes ***/
/**
 * @defgroup apcf4 IPMI2.0 Payload Commands
 * @ingroup apcf
 * IPMI2.0 RMCP+ Payload Command Handlers. Invoked by the message handler
 * @{
 **/
extern int  ActivatePayload     (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  DeactivatePayload   (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  GetPayldActStatus   (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  GetPayldInstInfo    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  SetUsrPayloadAccess (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  GetUsrPayloadAccess (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  GetChPayloadSupport (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  GetChPayloadVersion (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  GetChOemPayloadInfo (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  GetChCipherSuites   (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  SusResPayldEncrypt  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  SetChSecurityKeys   (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int  GetSysIfcCaps       (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

#endif  /* APPDEVICE_H */
