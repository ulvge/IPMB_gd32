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
 * RMCP.c
 * RMCP Message Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS     0
#include "Types.h"
#include "Serial.h"
#include "Support.h"
//#include "Debug.h"
#include "MsgHndlr.h"
#include "Session.h"
//#include "NVRAccess.h"
#include "App.h"
#include "IPMIDefs.h"
#include "IPMI_Main.h"
#include "IPMI_AppDevice.h"
//#include "SharedMem.h"
//#include "PDKAccess.h"
//#include "PDKDefs.h"
#include "IPMIConf.h"
#include "MsgHndlr.h"

#define AUTH_CAP2BMC_SWITCH_ENABLE  BIT4


/*** Function Prototypes ***/
static int  AuthenticateSerialCmd (INT8U NetFn, INT8U Cmd,int BMCInst);
static void ActivateBasicMode (_NEAR_ IPMIMsgHdr_T* pIPMIReqHdr,
                               _NEAR_ IPMIMsgHdr_T* pIPMIResHdr,int BMCInst);


/**
 * @brief Process the authenticates the serial request and activates the basic mode
 * @param pReq pointer to the request MsgPkt
 * @param Res pointer to the response MsgPkt
 * @param BMCInst Holds the Instance value of BMC
 * @return the size of the response else return 0 if it fails
 **/
INT8U
ProcessSerialMessage (_NEAR_ MsgPkt_T* pReq, _NEAR_ MsgPkt_T* pRes,int BMCInst)
{
    // bridge GetMsgFromI2C        from slave
    // ProcessIPMIReq(pReq, pRes);
    // msghandlr and master msg send to ipmitool
    ProcessIPMIReq(pReq, pRes);
    return pRes->Size;
}


/**
 * @brief Checks the Network Function and command whether authentication 
 * is required or not
 * @param NetFn Network function of request/response
 * @param Cmd Command ID
 * @return 0 if authenticated else -1
 **/
static int
AuthenticateSerialCmd (INT8U NetFn, INT8U Cmd,int BMCInst)
{

    return -1;
}


/**
 * @brief Depending on the command, it Activates/ Deactivates the Serial Basic mode.
 * @param pIPMIReqHdr Pointer to the IPMI request message header
 * @param pIPMIResHdr Pointer to the IPMI response message header
 **/
static void 
ActivateBasicMode (_NEAR_ IPMIMsgHdr_T* pIPMIReqHdr, _NEAR_ IPMIMsgHdr_T* pIPMIResHdr,int BMCInst)
{
 
}




