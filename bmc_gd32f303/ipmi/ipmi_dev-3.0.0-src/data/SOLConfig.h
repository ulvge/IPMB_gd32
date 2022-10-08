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
/****************************************************************
 *
 * SOLCmds.c
 * Serial Over Lan commands.
 *
 *  Author: Govindarajan <govindarajann@amiindia.co.in>
 *          Vinoth Kumar <vinothkumars@amiindia.co.in>
 ****************************************************************/
#ifndef SOL_H
#define SOL_H

#include "Types.h"

/**
 * @defgroup sol SOL Command handlers
 * @ingroup devcfg
 * IPMI Serial Over LAN interface command handlers.
 * Commands are used for activating/deactivating SOL payload and 
 * retrieving/setting various SOL parameters.
 * @{
**/
extern int SOLActivating (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetSOLConfig (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSOLConfig (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

#endif  /* SOL_H */
