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
 * PEFDevice.h
 * PEF Commands Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com> 
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 * 
 *****************************************************************/
#ifndef PEFDEVICE_H
#define PEFDEVICE_H

#include "Types.h"

/*** Macro Definitions ***/
#define PARAMETER_REVISION_FORMAT   0x11

/**
 * @defgroup pefc PEF and Alerting Commands
 * @ingroup senevt
 * IPMI PEF Device and Alerting Command Handlers. Invoked by the message handler
 * @{
 **/
extern int GetPEFCapabilities       (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int ArmPEFPostponeTimer      (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetPEFConfigParams       (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetPEFConfigParams       (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetLastProcessedEventId  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetLastProcessedEventId  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int AlertImmediate           (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int PETAcknowledge           (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

#define SEND_ALERT_IMM 0x0
#define GET_ALERT_IMM_STATUS  0x1
#define CLEAR_ALERT_IMM_STATUS 0x2

#define PEF_CONTROL                     1
#define PEF_ACTION_CONTROL              2
#define PEF_STARTUP_DELAY               3
#define PEF_ALERT_STARTUP_DELAY         4
#define NUM_EVT_ENTRIES                 5
#define EVT_ENTRY                       6
#define EVT_ENTRY_BYTE1                 7
#define NUM_ALERT_ENTRIES               8
#define ALERT_ENTRY                     9
#define GUID                            10
#define NUM_ALERT_STR                   11
#define ALERT_STRING_KEYS               12
#define ALERT_STRINGS                   13


#endif  /* APPDEVICE_H */
