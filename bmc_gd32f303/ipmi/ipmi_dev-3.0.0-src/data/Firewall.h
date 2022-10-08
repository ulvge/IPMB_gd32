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
 ******************************************************************
 * 
 * Firewall.h
 * IPMI firmware firewall commands
 *
 *  Author: Ravinder Reddy <bakkar@ami.com>
 *          Basavaraj Astekar <basavaraja@ami.com>
 *          
 ******************************************************************/
#ifndef FIREWALL_H
#define FIREWALL_H
#include "Types.h"

#pragma pack(1)
typedef struct{
  INT8U IANA[3];
}PACKED IANA_T;
#pragma pack()
/*** Function Prototypes ***/

/**
 * @defgroup apcf5 Firmware Firewall Commands
 * @ingroup apcf
 * IPMI IPM Device Command Handlers. Invoked by the message handler
 * (IPMI 2.0 feature)
 * @{
 **/
extern int GetNetFnSup      (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetCmdSup        (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSubFnSup      (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetConfigCmds    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetConfigSubFns  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetCmdEnables    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetCmdEnables    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetSubFnEnables  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSubFnEnables  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetOEMNetFnIANASupport  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

#endif /* FIREWALL_H */
