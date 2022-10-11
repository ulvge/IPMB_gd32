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
#include "bsp_i2c.h"
#include "ipmi_common.h"

#define AUTH_CAP2BMC_SWITCH_ENABLE  BIT4


/*** Function Prototypes ***/
static int  AuthenticateSerialCmd (INT8U NetFn, INT8U Cmd,int BMCInst);
static void ActivateBasicMode (IPMIMsgHdr_T* pIPMIReqHdr,
                               IPMIMsgHdr_T* pIPMIResHdr,int BMCInst);


/**
 * @brief Process the authenticates the serial request and activates the basic mode
 * @param pReq pointer to the request MsgPkt
 * @param Res pointer to the response MsgPkt
 * @param BMCInst Holds the Instance value of BMC
 * @return the size of the response else return 0 if it fails
 **/
INT16U
ProcessSerialMessage (MsgPkt_T* pReq, MsgPkt_T* pRes,int BMCInst)
{                                                 
	IPMIMsgHdr_T *pIPMIReqHdr = (IPMIMsgHdr_T *)pReq->Data;
    if (pIPMIReqHdr->ResAddr == I2C_SLAVE_ADDRESS7) {
        // msghandlr and master msg send to ipmitool
        return ProcessIPMIReq(pReq, pRes); //get&hand map
    } else {
        //Forwarded message   
//		IPMIMsgHdr_T *pIPMIResHdr = (IPMIMsgHdr_T *)pRes->Data;  
//        pIPMIReqHdr->ResAddr = I2C_SLAVE_ADDRESS7;
//        SwapIPMIMsgHdr((const IPMIMsgHdr_T *)pIPMIReqHdr, pIPMIResHdr); //get&hand map
//        pRes->Size = pReq->Size ;
//		
//		pIPMIResHdr->ChkSum = CalculateCheckSum((INT8U*)pIPMIResHdr, 2); 
//		pRes->Data[pRes->Size - 1] = CalculateCheckSum2(pRes->Data, pRes->Size - 1);
//        
//		ipmb_set_dualaddr(pIPMIResHdr->ReqAddr);
//        ipmb_write(pRes->Data, pRes->Size);
										   
		ipmb_set_dualaddr(pIPMIReqHdr->ReqAddr);
		ipmb_write(pReq->Data, pReq->Size);
        return 0;
    }
}


/**
 * @brief Checks the Network Function and command whether authentication 
 * is required or not
 * @param NetFn Network function of request/response
 * @param Cmd Command ID
 * @return 0 if authenticated else -1
 **/
__attribute__((unused)) static int
AuthenticateSerialCmd (INT8U NetFn, INT8U Cmd,int BMCInst)
{

    return -1;
}


/**
 * @brief Depending on the command, it Activates/ Deactivates the Serial Basic mode.
 * @param pIPMIReqHdr Pointer to the IPMI request message header
 * @param pIPMIResHdr Pointer to the IPMI response message header
 **/
__attribute__((unused)) static void 
ActivateBasicMode (IPMIMsgHdr_T* pIPMIReqHdr, IPMIMsgHdr_T* pIPMIResHdr,int BMCInst)
{
 
}




