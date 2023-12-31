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
 * Bridging.h
 * Bridging Commands
 * 
 * Author: Gowtham.M.S <gowthamms@amiindia.co.in>
 * 
 *****************************************************************/
#ifndef BRIDGING_H
#define BRIDGING_H

#include "Types.h"

/**
 * @var g_IsDiscovered
 * @brief Bridging status.
**/
extern BOOL g_IsBridging;


/*** External Definitions ***/
#define ICMB_BR_REQ_TIMEOUT     10


/**
 * @defgroup bbf Bridging Command handlers
 * @ingroup bridge
 * IPMI Intelligent Chassis Management Bus Bridging command interface.
 * Implemented as per ICMB ver 1.0, Revision 1.3
 * @{
**/
extern  int BridgeReq (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern  int BridgeMsg (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

#endif  /*BRIDGING*/
