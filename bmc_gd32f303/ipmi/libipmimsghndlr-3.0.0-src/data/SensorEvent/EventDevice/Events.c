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
 *****************************************************************
 *
 * Events.c
 * Event Commands Handler
 *
 * Author: Bakka Ravinder Reddy <bakkar@ami.com>
 * 
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "Events.h"
#include "MsgHndlr.h"
#include "IPMIDefs.h"
#include "Support.h"
#include "SEL.h"
#include "IPMI_SEL.h"
#include "SELRecord.h"
#include "Message.h"
#include "IPMI_Events.h"
#include "SharedMem.h"
#include "PEF.h"
#include "IPMI_Main.h"
#include "AppDevice.h"
#include "IPMI_KCS.h"
#include "IPMI_BT.h"
#include "SensorMonitor.h"
#include "IPMIConf.h"
#include "featuredef.h"
#include "PDKCmdsAccess.h"

/* Reserved bit macro definitions */
#define RESERVED_BITS_SETEVENTRECEIVER                  0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)

#if EVENT_PROCESSING_DEVICE == 1

/*** Local Definitions ***/
#define     EVTDATA_LENGTH_SYS_INTERFACE        0x08
#define     EVTDATA_LENGTH_NON_SYS_INTERFACE    0x07
#define     LUN_MASK                            0x03
#define     UNSPECIFIED_EVT_DATA                0xFF


/*---------------------------------------
 * SetEventReceiver
 *---------------------------------------*/
int
SetEventReceiver (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _FAR_ MsgPkt_T Msg;
    HQueue_T hSMHndlr_Q;
    _NEAR_ SetEvtRcvReq_T* pSetEvtReq = (_NEAR_ SetEvtRcvReq_T*) pReq;
    _FAR_  BMCSharedMem_T* pSharedMem = BMC_GET_SHARED_MEM(BMCInst);

    /* Check for the reserved bytes should b zero */

    if  ( 0 !=  (pSetEvtReq->RcvLUN & RESERVED_BITS_SETEVENTRECEIVER ) )
    {
         pRes[0] = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    pSharedMem->EvRcv_SlaveAddr = pSetEvtReq->RcvSlaveAddr;
    pSharedMem->EvRcv_LUN       = 0;
    pSharedMem->EvRcv_LUN       |= (pSetEvtReq->RcvLUN & LUN_MASK);
     Msg.Param = PARAM_REARM_ALL_SENSORS;
     Msg.Size = 0;

    GetQueueHandle(SM_HNDLR_Q,&hSMHndlr_Q,BMCInst);
    /* Post Msg to sensormonitor task Thread to rearm all sensors */
    if ( -1 != hSMHndlr_Q )
    {
        PostMsg(&Msg, SM_HNDLR_Q, BMCInst);
        pRes[0] = CC_NORMAL;
    }else
    {
        pRes[0] = 0xFF;
    }

    return sizeof(*pRes);
}


/*---------------------------------------
 * GetEventReceiver  
 *---------------------------------------*/
int
GetEventReceiver (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetEvtRcvRes_T* pGetEvtRes = (_NEAR_ GetEvtRcvRes_T*) pRes;
    _FAR_  BMCSharedMem_T* pSharedMem = BMC_GET_SHARED_MEM(BMCInst);

    pGetEvtRes->RcvSlaveAddr = pSharedMem->EvRcv_SlaveAddr;
    pGetEvtRes->RcvLUN       = pSharedMem->EvRcv_LUN;

    pGetEvtRes->CompletionCode = CC_NORMAL;

    return sizeof(GetEvtRcvRes_T);
}


/*---------------------------------------
 * PlatformEventMessage
 *---------------------------------------*/
int
PlatformEventMessage (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    SELEventRecord_T     EvtRec;
    AddSELRes_T          AddSelRes;
    MsgPkt_T             MsgToPEF;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    IPMIMsgHdr_T* pIPMIMsgHdr = NULL;    
    INT8U curchannel;
    
   // Request Length varies for channel (from which the request was received)
   // software ID exist only for Sys IFC, for all other IFC, it won't
    EvtRec.EvtData2 = UNSPECIFIED_EVT_DATA;
    EvtRec.EvtData3 = UNSPECIFIED_EVT_DATA;

    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    if ( (((pBMCInfo->IpmiConfig.SYSIfcSupport == 0x01 && (curchannel & 0xF) == SYS_IFC_CHANNEL) || (pBMCInfo->IpmiConfig.SMBUSIfcSupport == 0x01 && (curchannel & 0xF) == pBMCInfo->SMBUSCh))
                       && ((ReqLen < EVTDATA_LENGTH_SYS_INTERFACE - 2) || (EVTDATA_LENGTH_SYS_INTERFACE < ReqLen)) )
                       ||((((curchannel & 0xF) != SYS_IFC_CHANNEL) && ((curchannel & 0xF) != pBMCInfo->SMBUSCh)) 
                       && ((ReqLen < EVTDATA_LENGTH_NON_SYS_INTERFACE - 2) || (EVTDATA_LENGTH_NON_SYS_INTERFACE < ReqLen)) ) )
    {
        pRes[0] = CC_REQ_INV_LEN;
        return sizeof(*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    MsgToPEF.Channel = curchannel & 0xF;
    MsgToPEF.Cmd = 0;

    if ((pBMCInfo->IpmiConfig.SYSIfcSupport == 0x01 && SYS_IFC_CHANNEL == (curchannel & 0xF)) ||
       (pBMCInfo->IpmiConfig.SMBUSIfcSupport == 0x01 && pBMCInfo->SMBUSCh == (curchannel & 0xF)))
    {
        EvtRec.GenID [0] = *pReq| BIT0;   // system software ID
        EvtRec.GenID [1] = 0;
        _fmemcpy ((_FAR_ INT8U*)&EvtRec.EvMRev, &pReq [1], ReqLen - 1);
    }
    else
    { // RqAddr & LUN are already in the Msg Hdr, use that here
        pIPMIMsgHdr = ((IPMIMsgHdr_T *) &pReq [0] ) -1; 
        EvtRec.GenID [0] = pIPMIMsgHdr->ReqAddr;
        EvtRec.GenID [1] = ((curchannel & 0xF) << 4) | (pIPMIMsgHdr->RqSeqLUN & 0x3);
        _fmemcpy ((_FAR_ INT8U*)&EvtRec.EvMRev , &pReq [0], ReqLen);
    }

    EvtRec.hdr.Type      = 0x02;        /* Mark as System Event Record */
    EvtRec.hdr.TimeStamp = GetSelTimeStamp(BMCInst);
    LockedAddSELEntry ((_NEAR_ INT8U*)&EvtRec, sizeof (SELEventRecord_T),
                 (_NEAR_ INT8U*)&AddSelRes, (((SYS_IFC_CHANNEL == (curchannel & 0xF)) || (pBMCInfo->SMBUSCh == (curchannel & 0xF))) ? TRUE : FALSE), POST_SEL_AND_PEF, BMCInst);
    EvtRec.hdr.ID        = AddSelRes.RecID;

    if (AddSelRes.CompletionCode == CC_PARAM_NOT_SUP_IN_CUR_STATE)
    {
        // CC_PARAM_NOT_SUP_IN_CUR_STATE will be returned when SEL is disabled.
        EvtRec.hdr.ID = 0xFFFF;
    }
    else if (AddSelRes.CompletionCode != CC_NORMAL)
    {
        pRes [0] = AddSelRes.CompletionCode;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (*pRes);
    }

    _fmemcpy (MsgToPEF.Data, (_FAR_ INT8U*)&EvtRec, sizeof (SELEventRecord_T));
    MsgToPEF.Size  = sizeof (SELEventRecord_T);
    MsgToPEF.Param = PARAM_PLATFORM_EVT_MSG;

    // the AddSELEntry() function alreadys forwards the event to PEF, we shouldn't pass it again to the PEF here.
    // Otherwise, the PEF will get and act on the event twice.
    //PostMsgNonBlock (&MsgToPEF, hPEFTask_Q);

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->EventMutex,WAIT_INFINITE);
    //30069
    /* Post to the Event message queue only if event message buffer enabled */
    if ((BMC_GET_SHARED_MEM (BMCInst)->GlobalEnables & 0x04) &&
        (BMC_GET_SHARED_MEM (BMCInst)->NumEvtMsg < EVT_MSG_BUF_SIZE))
    {

        PostMsgNonBlock (&MsgToPEF, EVT_MSG_Q,BMCInst);
        BMC_GET_SHARED_MEM (BMCInst)->NumEvtMsg++;
    }

    if (BMC_GET_SHARED_MEM (BMCInst)->GlobalEnables & 0x04)
    {
        /* If Event MessageBuffer is Full set the SMS_ATN bit */                        
        if (BMC_GET_SHARED_MEM (BMCInst)->NumEvtMsg >= EVT_MSG_BUF_SIZE) 
        {
            //SET_SMS_ATN ();
            if (pBMCInfo->IpmiConfig.KCS1IfcSupport == 1)
            {
                SET_SMS_ATN (0, BMCInst);
                if(g_corefeatures.kcs_obf_bit == ENABLED)
                {
                    SET_OBF (0, BMCInst);
                }
            }

            if (pBMCInfo->IpmiConfig.KCS2IfcSupport == 1)
            {
                SET_SMS_ATN (1, BMCInst);
                if(g_corefeatures.kcs_obf_bit == ENABLED)
                {
                    SET_OBF (1, BMCInst);
                }
            }

            if (pBMCInfo->IpmiConfig.KCS3IfcSuppport == 1)
            {
                SET_SMS_ATN (2, BMCInst);
                if(g_corefeatures.kcs_obf_bit == ENABLED)
                {
                    SET_OBF (2, BMCInst);
                }
            }
            
            if(pBMCInfo->IpmiConfig.BTIfcSupport == 1 )
            {
                SET_BT_SMS_ATN (0, BMCInst);
            }
        }
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->EventMutex);

    // To send notification to CIM
    if(g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM] != NULL)
    {       
    	uint8 CMD;
    	
	// Set bits for SEL Event & Add operation
    	CMD = 0x21;
    	((int(*)(uint8, uint16))g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM])(CMD, AddSelRes.RecID);
    }
    

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
    /* Fill the completion code */  
    pRes [0] = CC_NORMAL;

    return sizeof(*pRes);
}

#endif  /* EVENT_PROCESSING_DEVICE */

