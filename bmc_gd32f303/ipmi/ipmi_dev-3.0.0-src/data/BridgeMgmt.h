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
 ****************************************************************
 * 
 * BridgMgmt.h
 * Bridge Management Commands
 * 
 * Author: Gowtham.M.S <gowthamms@amiindia.co.in>
 * 
 *****************************************************************/
#ifndef BRIDGE_MGMT_H
#define BRIDGE_MGMT_H

#include "Types.h"
#include "IPMI_BrMgmt.h"

#pragma pack( 1 )
/**
 * @struct BridgeMgmtSHM_T
 * @brief Bridge management information in Shared Memory.
**/
typedef struct {
    INT8S           State;
    INT16U          ICMBAddr;
    INT16U          ICMBProxyAddr;
    INT8U           IPMBProxyAddr;
    INT8U           ProxyLUN;
    ICMBStats0_T    ICMBStats0;
    ICMBStats1_T    ICMBStats1;

} PACKED  BridgeMgmtSHM_T;

#pragma pack( )

/**
 * @defgroup bmf Bridge Management Command handlers
 * @ingroup bridge
 * IPMI Intelligent Chassis Management Bus Bridge Management command interface.
 * Implemented as per ICMB ver 1.0, Revision 1.3
 * @{
**/
extern  int GetBridgeState        (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int SetBridgeState        (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int GetICMBAddr           (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int SetICMBAddr           (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int SetBridgeProxyAddr    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int GetBridgeProxyAddr    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int GetBridgeStatistics   (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int ClearBridgeStatistics (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int GetICMBCaps           (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int GetICMBConnectorInfo  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int GetICMBConnectionID   (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int SendICMBConnectionID  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

#endif  /*BRIDGE_MGMT_H*/
