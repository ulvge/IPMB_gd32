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
 * BridgeEvt.h
 * Bridge Event Commands
 *
 * Author: Gowtham.M.S <gowthamms@amiindia.co.in>
 *
 *****************************************************************/
#ifndef BRIDGE_EVT_H
#define BRIDGE_EVT_H

#include "Types.h"

#pragma pack( 1 )
/**
 * @struct BridgeEvt_T
 * @brief Bridge event message.
**/
typedef struct {
    INT8U   EvtCount;
    INT16U  EvtDestAddr;
    INT8U   EvtState;
    INT8U   EvtSlaveAddr;
    INT8U   EvtLUN;

} PACKED  BridgeEvt_T;

#pragma pack( )

/**
 * @defgroup bef Bridge Event Command handlers
 * @ingroup bridge
 * IPMI Intelligent Chassis Management Bus Bridge Event command interface.
 * Implemented as per ICMB ver 1.0, Revision 1.3
 * @{
**/
extern  int GetEventCount   (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int SetEventDest    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int SetEventReceptionState (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int SendICMBEventMsg(INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int GetEventDest    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int GetEventReceptionState (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

#endif  /*BRIDGE_EVT_H*/
