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
#define ENABLE_DEBUG_MACROS 0
#include "MsgHndlr.h"
#include "Support.h"
#include "Debug.h"
#include "OSPort.h"
#include "Message.h"
#include "IPMIDefs.h"
// #include "PMConfig.h"
// #include "SharedMem.h"
#include "Session.h"
#include "LANIfc.h"
#include "MD.h"
#include "Util.h"
#include "RMCP.h"
#include "RMCP+.h"
#include "App.h"
#include "IPMI_Main.h"
#include "Ethaddr.h"
#include "IPMIConf.h"
#include "IPMI_SensorEvent.h"
#include "Badpasswd.h"
#include "PendTask.h"
#include "SEL.h"
// #include "featuredef.h"
// #include "blowfish.h"
// #include "Encode.h"
#include "AMIDevice.h"

/*** Local definitions ***/
#define RMCP_VERSION                6
#define IPMI_MESSAGE_CLASS          7
#define PRESENCE_PING_MSGTYPE       0x80
#define RMCP_VERSION                6

#define AMI_CMD_NETFN_LUN           (((0x2E | 1) << 2) | 0x00)
#define AMI_CMD_12                  0x12
#define PING_IPMI_15_SUPPORT        1
#define PING_IPMI_20_SUPPORT        2
#define MAX_AUTH_CODE_SIZE          12
#define INIT_VECTOR_SIZE            16
#define INTEGRITY_MASK              BIT6
#define CONFIDENT_MASK              BIT7

#define PAYLOAD_RSSP_OS_REQ         0x10
#define PAYLOAD_RSSP_OS_RES         0x11
#define PAYLOAD_RAKP_MSG1           0x12
#define PAYLOAD_RAKP_MSG2           0x13
#define PAYLOAD_RAKP_MSG3           0x14
#define PAYLOAD_RAKP_MSG4           0x15

IfcType_T IfcType = LAN_IFC;

/*** Prototype Declaration ***/
static int   ProcIPMIReq        (_FAR_ SessionInfo_T*  pSessionInfo, INT8U Payload, MiscParams_T *pParams,INT8U Channel,int BMCInst);
static BOOL  ValidateRMCPHdr    (_NEAR_ RMCPHdr_T* pRMCPHdr);
static BOOL  ValidateSessionHdr (INT32U SessionID, INT32U SeqNo, int BMCInst);
static INT8U ProcessPingMsg     (_NEAR_ RMCPHdr_T* pRMCPReq,
                                 _NEAR_ RMCPHdr_T* pRMCPRes,int BMCInst);
static BOOL  ValidateAuthCode   (_NEAR_ INT8U* pAuthCode, _FAR_ INT8U* pPassword,
                                 _NEAR_ SessionHdr_T* pSessionHdr,
                                 _NEAR_ IPMIMsgHdr_T* pIPMIMsg);
static int   Proc20Payload      (_NEAR_ RMCPHdr_T* pRMCPReq,
                                 _NEAR_ RMCPHdr_T* pRMCPRes, MiscParams_T *pParams, INT8U Channel, int BMCInst);


/*** Local typedefs ***/
/**
 * @struct PreSessionCmd_T
 * @brief Pre-session command entry.
**/
typedef struct
{
    INT8U   NetFn;
    INT8U   Cmd;
} PreSessionCmd_T;

/**
 * @brief Message Payload Handler function.
 * @param pReq   - Request message.
 * @param ReqLen - Request length.
 * @param pRes   - Response message.
 * @return 0 if success, -1 if error.
**/
typedef int (*pPayloadHndlr_T) (_NEAR_ INT8U* pReq, INT8U ReqLen,
                                _NEAR_ INT8U* pRes, MiscParams_T *pParams,INT8U Channel, int BMCInst);

/**
 * @struct PayloadTbl_T;
 * @brief Payload Table structure.
**/
typedef struct
{
    INT8U           Payload;
    pPayloadHndlr_T PayloadHndlr;
} PayloadTbl_T;


// static const PayloadTbl_T m_PayloadTbl [] =
// {
//      /*  Payload              Handler           */
//     {PAYLOAD_RSSP_OS_REQ,   RSSPOpenSessionReq  },
//     {PAYLOAD_RAKP_MSG1,     RAKPMsg1            },
//     {PAYLOAD_RAKP_MSG3,     RAKPMsg3            },
// };

/* Pre-Session establishment commands */
static const PreSessionCmd_T m_PreSessionCmdsTbl[] =
{
    { NETFN_APP,    CMD_GET_CH_AUTH_CAP },
    { NETFN_APP,    CMD_GET_SESSION_CHALLENGE },
    { NETFN_APP,    CMD_GET_DEV_GUID },
    { NETFN_APP,    CMD_GET_CH_CIPHER_SUITES },
    { NETFN_APP,    CMD_GET_SYSTEM_GUID },
    { NETFN_SENSOR, CMD_PET_ACKNOWLEDGE},
};

/**
 * @struct IPMIAuditMaskTbl_T;
 * @brief  Maps Audit Event types to their bitmask in configuration.
**/
typedef struct
{
    INT8U EventType;
    INT8U EventMask;
}IPMIAuditMaskTbl_T;

static const IPMIAuditMaskTbl_T m_IPMIAuditMaskMap[] =
{
    { EVENT_LOGIN,           0x0 },
    { EVENT_LOGOUT,          0x1 },
    { EVENT_AUTO_LOGOUT,     0x2 },
    { EVENT_LOGIN_FAILURE,   0x3 },
};


int RmcpSeqNumValidation(SessionInfo_T* pSessionInfo, INT32U SessionSeqNum, IPMIMsgHdr_T* pIPMIMsgReq)
{
 

    return 0;
}

/*-------------------------------------------
 * ProcessRMCPReq
 *-------------------------------------------*/
INT32U
ProcessRMCPReq(_NEAR_ RMCPHdr_T* pRMCPReq, _NEAR_ RMCPHdr_T* pRMCPRes, MiscParams_T *pParams,INT8U Channel, int BMCInst)
{
   _FAR_   SessionInfo_T*  pSessionInfo;
    // _NEAR_  IPMIMsgHdr_T*   pIPMIMsgReq;
    // _NEAR_  IPMIMsgHdr_T*   pIPMIMsgRes;
    _NEAR_  MsgPkt_T   pIPMIMsgReq;
    _NEAR_  MsgPkt_T   pIPMIMsgRes;
    _NEAR_  INT8U*          pReqMsgAuthCode;
    _NEAR_  INT8U*          pResMsgAuthCode;
    _NEAR_  SessionHdr_T*   pReqSessionHdr = (_NEAR_ SessionHdr_T*)(pRMCPReq + 1);
    _NEAR_  SessionHdr_T*   pResSessionHdr = (_NEAR_ SessionHdr_T*)(pRMCPRes + 1);
            INT8U           IPMIMsgLen,AuthType;
            INT32U          SessionID;
            INT32U          SessionSeqNum;
            INT32U          ResLen, IPMIMsgResLen;
    _FAR_ BMCInfo_t*        pBMCInfo = &g_BMCInfo;
    _FAR_   UserInfo_T*     pUserInfo;


    /* Validate RMCP Header */
    if (TRUE != ValidateRMCPHdr(pRMCPReq))
    {
        IPMI_WARNING ("RMCP.c : RMCP header validation failed\r\n");
        return 0;
    }

    /* If RMCP Ping, process it seperately */
    if (pRMCPReq->MsgClass == 0x06)
    {
        // LOG_I("ProcessPingMsg.");
        return ProcessPingMsg (pRMCPReq, pRMCPRes, 0);
    }

    /* Process IPMI 2.0 Separately */
#if IPMI20_SUPPORT == 1
    if (RMCP_PLUS_FORMAT == pReqSessionHdr->AuthType)
    {
        ResLen = Proc20Payload (pRMCPReq, pRMCPRes, pParams,Channel, 0);
    }
    else
#endif
    {
        AuthType              = pReqSessionHdr->AuthType;
        SessionID             = pReqSessionHdr->SessionID;
        SessionSeqNum   = pReqSessionHdr->SessionSeqNum;

        /* Validate IPMI Session Header */
        if (TRUE != ValidateSessionHdr (SessionID, SessionSeqNum, 0))
        {
            IPMI_WARNING ("RMCP.c : IPMI Session header validation failed\n");
            return 0;
        }
    
        if (0 == pReqSessionHdr->AuthType)
        {
            IPMIMsgLen  = (INT8U) (*((_NEAR_ INT8U*)(pReqSessionHdr + 1)));
            pIPMIMsgReq.Size = IPMIMsgLen;
            _fmemcpy(pIPMIMsgReq.Data, ((INT8U*)pRMCPReq)+sizeof(RMCPHdr_T)+sizeof(SessionHdr_T)+1, IPMIMsgLen);
            

            // pIPMIMsgReq = (_NEAR_ IPMIMsgHdr_T*) (((_NEAR_ INT8U*)(pReqSessionHdr + 1)) +
            //               sizeof (IPMIMsgLen));
            // pIPMIMsgRes = (_NEAR_ IPMIMsgHdr_T*) (((_NEAR_ INT8U*)(pResSessionHdr + 1)) +
            //               sizeof (IPMIMsgLen));
        }
        else
        {
            pReqMsgAuthCode = ((_NEAR_ INT8U*)(pReqSessionHdr + 1));
            pResMsgAuthCode = ((_NEAR_ INT8U*)(pResSessionHdr + 1));
            IPMIMsgLen      = *(pReqMsgAuthCode + AUTH_CODE_LEN);
            // pIPMIMsgReq     = (_NEAR_ IPMIMsgHdr_T*) (pReqMsgAuthCode + AUTH_CODE_LEN +
            //                   sizeof (IPMIMsgLen));
            // pIPMIMsgRes     = (_NEAR_ IPMIMsgHdr_T*) (pResMsgAuthCode + AUTH_CODE_LEN +
            //                   sizeof (IPMIMsgLen));


            // if(FindUserLockStatus(pSessionInfo->UserId, Channel, 0) == 0)
            // if(1)
            // {
            //     if (TRUE != ValidateAuthCode (pReqMsgAuthCode, pSessionInfo->Password,
            //                                   pReqSessionHdr, pIPMIMsgReq))
            //     {
            //         // LockUser(pSessionInfo->UserId,Channel, 0);
            //         IPMI_WARNING ("RMCP.c : Invalid Authentication Code \n");

            //         if ( 0 != AddLoginEvent(pSessionInfo->UserId, NULL, EVENT_LOGIN_FAILURE, 0))
            //         {
            //              TCRIT("Problem while adding Log record \n");
            //         }

            //         OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            //         return 0;
            //     }
            //     // UnlockUser(pSessionInfo->UserId,Channel, 0);
            // }
            // else
            // {
            //     return 0;
            // }
        }

        // /* Post Msg to MsgHndlr and Get Res */
        // if (0 != ProcIPMIReq (pSessionInfo, PAYLOAD_IPMI_MSG, pParams, Channel, BMCInst))
        // {
        //     return 0;
        // }
        ProcessIPMIReq(&pIPMIMsgReq, &pIPMIMsgRes);  //chengjia 2020.07.13

        // int i;
        // LOG_RAW("ipmi res:");
        // for(i=0; i<pIPMIMsgRes.Size; i++)
        // {
        //     LOG_RAW(" %02x", pIPMIMsgRes.Data[i]);
        // }
        // LOG_RAW("\r\n");

        *(((char *)pRMCPRes) + sizeof(RMCPHdr_T)+sizeof(SessionHdr_T)) = pIPMIMsgRes.Size;  // lan packet data length byte
        /* Fill Response data */
        _fmemcpy (((char *)pRMCPRes) + sizeof(RMCPHdr_T)+sizeof(SessionHdr_T)+1, pIPMIMsgRes.Data, pIPMIMsgRes.Size);
    
        /* Increment session sequence number */
        // if (NULL != pSessionInfo)
        // {
        //     if(pSessionInfo->OutboundSeq == SEQNUM_ROLLOVER)
        //     {
        //         ((_NEAR_ SessionHdr_T*)pResSessionHdr)->SessionSeqNum = 0x00;
        //     }
        //     else
        //     {
        //         ((_NEAR_ SessionHdr_T*)pResSessionHdr)->SessionSeqNum =
        //                                     pSessionInfo->OutboundSeq++;
        //     }
        // }

        /* Fill Authentication Code */
        // if (0 != pReqSessionHdr->AuthType)
        // {
        //     // pResMsgAuthCode = (_NEAR_ INT8U*)(pResSessionHdr + 1);
        //     // pIPMIMsgRes     = (_NEAR_ IPMIMsgHdr_T*)((_NEAR_ INT8U*)(pResSessionHdr + 1) +
        //     //                   AUTH_CODE_LEN + sizeof (IPMIMsgLen));
        //     // IPMIMsgResLen      = AUTH_CODE_LEN + sizeof (IPMIMsgLen) + pBMCInfo->LANConfig.MsgRes.Size;
        //     // /* Fill IPMI Message */
        //     // _fmemcpy (pIPMIMsgRes, pBMCInfo->LANConfig.MsgRes.Data, pBMCInfo->LANConfig.MsgRes.Size);
        //     // *(pResMsgAuthCode + AUTH_CODE_LEN) = pBMCInfo->LANConfig.MsgRes.Size;

        //     // ComputeAuthCode (pSessionInfo->Password, pResSessionHdr, pIPMIMsgRes,
        //     //                  pResMsgAuthCode, MULTI_SESSION_CHANNEL);
        // }
        // else
        // {
        //     pIPMIMsgRes = (_NEAR_ IPMIMsgHdr_T*)((_NEAR_ INT8U*)(pResSessionHdr + 1) +
        //                   sizeof (IPMIMsgLen));
        //     IPMIMsgResLen  = pBMCInfo->LANConfig.MsgRes.Size + sizeof (IPMIMsgLen);
        //     /* Fill IPMI Message */
        //     _fmemcpy (pIPMIMsgRes, pBMCInfo->LANConfig.MsgRes.Data, pBMCInfo->LANConfig.MsgRes.Size);
        //     *((_NEAR_ INT8U*) (pResSessionHdr + 1)) = pBMCInfo->LANConfig.MsgRes.Size;
        // }

        // if( (NETFN_APP == (pIPMIMsgReq->NetFnLUN >> 2)) && (CMD_GET_SESSION_CHALLENGE == pIPMIMsgReq->Cmd) && 
        //     (CC_GET_SESSION_INVALID_USER == pBMCInfo->LANConfig.MsgRes.Data[sizeof(IPMIMsgHdr_T)]) && (NULL == pSessionInfo) )
        // {
        //     if ( 0 != AddLoginEvent( 0xFF, &pBMCInfo->LANConfig.MsgReq.Data[7], EVENT_LOGIN_FAILURE, BMCInst ))
        //     {
        //         TCRIT("Problem while adding Log record \n");
        //     }
        // }
        ResLen = sizeof (RMCPHdr_T) + sizeof (SessionHdr_T) + pIPMIMsgRes.Size + 1;
    }

    return ResLen;
}

/**
 * @brief Process the IPMI request and prepare response.
 * @param pSessionInfo - Session information.
 * @param Payload     - Payload type.
 * @return 0 if success, -1 if error.
**/
static int
ProcIPMIReq (_FAR_  SessionInfo_T*  pSessionInfo, INT8U Payload, MiscParams_T *pParams,INT8U Channel, int BMCInst)
{

    return 0;
}


/**
 * @brief Validate RMCP Header
 * @param pRMCPHdr - RMCP header.
 * @return TRUE if valid, FALSE if invalid.
**/
static BOOL
ValidateRMCPHdr (_NEAR_ RMCPHdr_T* pRMCPHdr)
{
    /* If RMCP Packet is NULL */
    if (pRMCPHdr == NULL)
    {
        IPMI_WARNING ("RMCP.c : RMCP Packet is NULL\n");
        return FALSE;
    }

    /* Verify RMCP Version */
    if (pRMCPHdr->Version != RMCP_VERSION)
    {
        IPMI_WARNING ("RMCP.c : Invalid RMCP Version\n");
        return FALSE;
    }

    /* LOOK for RMCP MessageClass */
    if ((pRMCPHdr->MsgClass != IPMI_MESSAGE_CLASS) &&
        (pRMCPHdr->MsgClass != 0x06))
    {
        IPMI_DBG_PRINT ("RMCP.c : Invalid Message Class\n");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Validate session header.
 * @param SessionID - Session ID.
 * @param SeqNo    - Session Sequence Number.
 * @return TRUE if valid, FALSE if invalid.
**/
static BOOL
ValidateSessionHdr (INT32U SessionID, INT32U SeqNo, int BMCInst)
{

    return TRUE;
}

/**
 * @brief Process RMCP Ping Message.
 * @param pRMCPReq - Request RMCP message.
 * @param pRMCPRes - Response RMCP message.
 * @return the response length.
**/
static INT8U
ProcessPingMsg (_NEAR_ RMCPHdr_T* pRMCPReq, _NEAR_ RMCPHdr_T* pRMCPRes,int BMCInst)
{
    _NEAR_ RMCPPingHdr_T* pReqPingHdr = (_NEAR_ RMCPPingHdr_T*)(pRMCPReq + 1);
    _NEAR_ RMCPPingHdr_T* pResPingHdr = (_NEAR_ RMCPPingHdr_T*)(pRMCPRes + 1);
    _FAR_ BMCInfo_t*        pBMCInfo = &g_BMCInfo;

    if (PRESENCE_PING_MSGTYPE != pReqPingHdr->MsgType) { return 0; }
    if((pReqPingHdr->IANANum[0]!=0x00)||(pReqPingHdr->IANANum[1]!=0x00)||
         (pReqPingHdr->IANANum[2]!=0x11)||(pReqPingHdr->IANANum[3]!=0xBE)) 
       { return 0; }

    /* Construct Response Header */
    _fmemcpy (pResPingHdr, pReqPingHdr, sizeof (RMCPPingHdr_T));
    pResPingHdr->MsgType = 0x40;
    pResPingHdr->DataLen = 0x10;

    /* Fill Response Data */
    _fmemset (pResPingHdr + 1, 0, pResPingHdr->DataLen);
    *((_NEAR_ INT8U*)(pResPingHdr + 1) + 8) = 0x81;
    *((_NEAR_ INT8U*)(pResPingHdr + 1) + 4) = PING_IPMI_15_SUPPORT;

#if IPMI20_SUPPORT == 1
    *((_NEAR_ INT8U*)(pResPingHdr + 1) + 4) |= PING_IPMI_20_SUPPORT;
#endif

    /*Update the OEM IANA Number for DCMI Discovery (36465 = Data Center Manageability Forum,Spec .1.5)*/
    // if(g_corefeatures.dcmi_1_5_support == ENABLED)
    // {
    //     if(pBMCInfo->IpmiConfig.DCMISupport == 1)
    //     {
    //         *((_NEAR_ INT8U*)(pResPingHdr + 1) + 2) = 0x8E;
    //         *((_NEAR_ INT8U*)(pResPingHdr + 1) + 3) = 0x71;
    //     }
    // }

    return (sizeof (RMCPHdr_T) + sizeof (RMCPPingHdr_T) + pResPingHdr->DataLen);
}


/**
 * @brief Validate Authentication Code
 * @param pAuthCode - Request authentication code.
 * @param pPassword - Password string.
 * @param pSessionHdr - Request Session header.
 * @param pIPMIMsg - Request IPMI message.
 * @return TRUE if valid, FALSE if invalid.
**/
static BOOL
ValidateAuthCode (_NEAR_ INT8U* pAuthCode, _FAR_ INT8U* pPassword,
                  _NEAR_ SessionHdr_T* pSessionHdr, _NEAR_ IPMIMsgHdr_T* pIPMIMsg)
{

    return true;
}

#if IPMI20_SUPPORT

/*-------------------------------------------
 * Frame20Payload
 *-------------------------------------------*/
int
Frame20Payload (INT8U PayloadType, _NEAR_ RMCPHdr_T* pRMCPPkt,
                _FAR_ INT8U* pPayload,  INT32U PayloadLen, _FAR_
                SessionInfo_T* pSessionInfo, int BMCInst)
{

    return 0;
}

int RMCPplusSeqNumValidation(SessionInfo_T * pSessionInfo,INT32U SessionSeqNum)
{

    return 0;
}


/**
 * @brief Process IPMI 2.0 Payload.
 * @param pRMCPReq - RMCP request message.
 * @param pRMCPRes _ RMCP response message.
 * @return 0 if success, -1 if error.
**/
static int
Proc20Payload (_NEAR_ RMCPHdr_T* pRMCPReq, _NEAR_ RMCPHdr_T* pRMCPRes, MiscParams_T *pParams,INT8U Channel, int BMCInst)
{
    _NEAR_ SessionHdr2_T*  pReqSession2Hdr = (_NEAR_ SessionHdr2_T*)(pRMCPReq + 1);
    _NEAR_ SessionHdr2_T*  pResSession2Hdr = (_NEAR_ SessionHdr2_T*)(pRMCPRes + 1);
    _NEAR_ INT8U*          pReq  = (_NEAR_ INT8U *)(pReqSession2Hdr + 1);
    _NEAR_ INT8U*          pRes  = (_NEAR_ INT8U *)(pResSession2Hdr + 1);
    _FAR_  SessionInfo_T*  pSessionInfo = NULL;
    _NEAR_ INT8U*          pIntPad;
    _NEAR_ INT8U*          pConfHdr;
    _NEAR_ INT8U*          pConfPayld;
    _NEAR_ INT8U*          pReqMsgAuthCode;
    _FAR_  UserInfo_T*     pUserInfo;
    _FAR_  BMCInfo_t*      pBMCInfo = &g_BMCInfo;
           INT8U           Payload, IntPadLen, ComputedAuthCode [25];
           INT16U          IPMIMsgLen, AuthCodeLen, ConfPayldLen;
           INT32U          SessionID;
           INT32U          SessionSeqNum;
           int             len, i;
           INT8U           UserPswd [MAX_PASSWORD_LEN];
           char  EncryptedPswd[MAX_ENCRYPTED_PSWD_LEN + 1] = {0};
           INT8U PwdEncKey[MAX_SIZE_KEY + 1] = {0};
        //    EVP_CIPHER_CTX ctx;
           uint32 tmplen;


    return 0;
}

/**************************************************************************** 
 * fn AddLoginEvent
 * params:
 * pRMCPSession  pointer to RMCP Session information
 * EvtType	0x9 - login, 0xa - logout, 0xb - autologout, 0xc - connection lost
 *
 * return 	0 - success, -1 - failure
 ***************************************************************************/
int AddLoginEvent ( INT8U UserID, INT8U* UserName, INT8U EvtType, int BMCInst)
{
    return 0;
}

#endif /*#if IPMI20_SUPPORT*/
