/*******************************************************************
********************************************************************
****                                                              **
****    (C)Copyright 2008-2009, American Megatrends Inc.          **
****                                                              **
****    All Rights Reserved.                                      **
****                                                              **
****    5555 , Oakbrook Pkwy, Norcross,                           **
****                                                              **
****    Georgia - 30093, USA. Phone-(770)-246-8600.               **
****                                                              **
********************************************************************
********************************************************************
********************************************************************
**
** BridgeMsgAPI.h
** Bridge Messaging API
**
** Author: Manimehalai S (manimehalais@amiindia.co.in)
*******************************************************************/
#ifndef BRIDGEMSGAPI_H
#define BRIDGEMSGAPI_H
#include "Types.h"


#define ERR_NULLPKT      -1
#define ERR_INVALIDCH    -2
#define ERR_PBTBL_FULL   -3
#define ERR_POSTMSG_FAIL -4
#define ENABLE_BRIDGE_RETRY_MASK   0x80
#define ORIGIN_SRC_MASK            ~ENABLE_BRIDGE_RETRY_MASK

extern int
API_BridgeInternal(MsgPkt_T* pMsgPkt, ResMsgHdr_T*  pResMsgHdr, INT8U DestAddr, 
                      INT8U OriginSrc, int Channel, BOOL ResponseTracking, int BMCInst);

extern int
API_BridgeInternal_Retry(MsgPkt_T* pMsgPkt, ResMsgHdr_T*  pResMsgHdr, INT8U DestAddr, 
                                    INT8U OriginSrc, int Channel, BOOL ResponseTracking, 
                                    INT8U TimeOut, INT8U RetryCount, int BMCInst);
#endif

