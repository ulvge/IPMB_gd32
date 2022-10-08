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
#include "Debug.h"
#include "Message.h"
#include "IPMIDefs.h"
// #include "PMConfig.h"
#include "SharedMem.h"
#include "Support.h"
#include "RMCP.h"
#include "RMCP+.h"
#include "Util.h"
#include "NVRAccess.h"
#include "Session.h"
#include "LANIfc.h"
#include "IPMI_IPM.h"
#include "IPMDevice.h"
#include "MD.h"
#include "Ciphertable.h"
#include "nwcfg.h"
#include "Ethaddr.h"
#include "IPMIConf.h"
#include "Badpasswd.h"
#include "featuredef.h"
#include "blowfish.h"
#include "Encode.h"
#include <openssl/hmac.h>
#include <openssl/rand.h>

#if IPMI20_SUPPORT == 1

/*** Local definitions ***/
#define AUTH_ALGORITHM                      00
#define INTEGRITY_ALGORITHM                 01
#define CONFIDENTIALITY_ALGORITHM           02

#define MAX_ROLE_SUPPORTED                  0x05
#define MAX_REM_CON_RAND_NO_LEN             0x10
#define MAX_MGD_SYS_RAND_NO_LEN             0x10
#define MAX_MGD_SYS_GUID_LEN                0x10
#define MAX_HMAC_BUF_SIZE                   128
#define RAKP1_HASH_SIZE                     20
#define RAKP1_HASH_HMAC_MD5_SIZE            16
#define RAKP1_HASH_HMAC_SHA256_SIZE  		32

/* RSSP and RAKP Message Status Codes */
#define SC_NO_ERROR                         0
#define SC_INSUFFICIENT_RESOURCE            1
#define SC_INV_SESSION_ID                   2
#define SC_INV_PAYLOAD_TYPE                 3
#define SC_INV_AUTH_ALGORITHM               4
#define SC_INV_INTEGRITY_ALGORITHM          5
#define SC_NO_MATCHED_AUTH_PAYLOAD          6
#define SC_NO_MATCHED_INTEGRITY_PAYLOAD     7
#define SC_INACTIVE_SESSION_ID              8
#define SC_INV_ROLE                         9
#define SC_UNAUTHORISED_ROLE                10
#define SC_INSUFFICIENT_RESOURCE_AT_ROLE    11
#define SC_INV_NAME_LEN                     12
#define SC_UNAUTHORISED_NAME                13
#define SC_UNAUTHORISED_GUID                14
#define SC_INV_INTEGRITY_CHECK              15
#define SC_INV_CONFIDENTIALITY_ALGORTHM     16
#define SC_NO_CIPHER_SUITE_MATCH            17
#define SC_ILLEGAL_OR_UNRECOGNIZED_PARAM    18

/* Authentication Algorithm Numbers */
#define RAKP_NONE                   0
#define RAKP_HMAC_SHA1              1
#define RAKP_HMAC_MD5	            2
#define RAKP_HMAC_SHA256            3

#define HASH_KEY1_CONST_SIZE                20
#define HASH_KEY2_CONST_SIZE                20

/**
 * @var m_SIK
 * @brief Session Key.
**/
static  INT8U m_SIK [SHA2_HASH_KEY_SIZE];


/*----------------------------------------
 * RSSPOpenSessionReq
 *----------------------------------------*/
int
RSSPOpenSessionReq (INT8U* pReq, INT8U ReqLen, INT8U* pRes, MiscParams_T *pParams, INT8U Channel, int BMCInst)
{
    RSSPOpenSessionReq_T    *Req        =
                                      (RSSPOpenSessionReq_T*)pReq;
    RSSPOpenSessionRes_T    *Res        =
                                      (RSSPOpenSessionRes_T*)pRes;
    RSSPOpenSessionErrRes_T *ErrRes     =
                                      (RSSPOpenSessionErrRes_T*)pRes;
      BMCInfo_t               *pBMCInfo   = 
                                    &g_BMCInfo[BMCInst];
      ChannelInfo_T           *pChannelInfo;
    INT8U                           Role, i, id;
    BOOL                            IsMatchReq      = FALSE;
    BOOL                            TrackRollOver   = FALSE;
    SessionInfo_T                   SessionInfo;
    INT32U                          TempSessId;
    INT8U                           CipherSuitePrivilage = 0;
    INT8U                           EthIndex;
    INT32U                          TrackRollOverSeq = SEQNUM_ROLLOVER;

    if(Channel == 0xFF)
    {
        Channel=GetLANChannel(Req->Reserved[0], BMCInst);
    }
    EthIndex= GetEthIndex(Channel, BMCInst);

    if(0xff == EthIndex)
    {
        IPMI_WARNING("\n Invalid Channel number :%x",Channel);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    /* Get information abt this channel */
    pChannelInfo = getChannelInfo (Channel, BMCInst);
    if (NULL == pChannelInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        Res->StatusCode = SC_INSUFFICIENT_RESOURCE;
        return sizeof (RSSPOpenSessionErrRes_T);
    }

    if(pChannelInfo->ActiveSession >= pChannelInfo->SessionLimit)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        /* Session limit reached*/
        IPMI_WARNING ("RMCP+.c : OpenSessionReq - Session limit exceeded :%x\t%x BMCInst %x\n ",pChannelInfo->ActiveSession,pChannelInfo->SessionLimit,BMCInst);
        Res->StatusCode = SC_INSUFFICIENT_RESOURCE;
        return sizeof (RSSPOpenSessionErrRes_T);
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    /* Get Role */
    Role = Req->Role & 0x0F;

    /* Fill Err Response to return, in case any error occurred */
    ErrRes->Reserved = 0;
    ErrRes->RemConSessionID = Req->RemConSessionID;

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex,WAIT_INFINITE);
    if (GetNumOfUsedSessions(BMCInst) >= pBMCInfo->IpmiConfig.MaxSession)
    {
        if(FALSE == CleanSession(BMCInst))
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            /* Session limit reached*/
            IPMI_WARNING ("RMCP+.c : OpenSessionReq - Session limit exceeded\n");
            Res->StatusCode = SC_INSUFFICIENT_RESOURCE;
            return sizeof (RSSPOpenSessionErrRes_T);
        }
    }

    /* Check Role  */
    if (Role > MAX_ROLE_SUPPORTED)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        /* Invalid Payload Type */
        IPMI_WARNING ("RMCP+.c : OpenSessionReq - Invalid role\n");
        Res->StatusCode = SC_INV_ROLE;
        return sizeof (RSSPOpenSessionErrRes_T);
    }

    /*Check Payload Type */
    if ((AUTH_ALGORITHM             != Req->Auth.PayloadType) ||
        (INTEGRITY_ALGORITHM        != Req->Integrity.PayloadType) ||
        (CONFIDENTIALITY_ALGORITHM  != Req->Confidentiality.PayloadType))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        /* Invalid Payload Type */
        IPMI_WARNING ("RMCP+.c : OpenSessionReq - Invalid Payload\n");
        Res->StatusCode = SC_INV_PAYLOAD_TYPE;
        return sizeof (RSSPOpenSessionErrRes_T);
    }

    /* Search for the matching cipher suite id from Cipher suite record. if matching id found
           check for for requested role/privilege is allowed for privilege level for the cipher suite */
    for (i = 0, id = 0; i < MAX_CIPHER_SUITES_BYTES; i++, id++)
    {
        if (i * 5 + 1 > MAX_CIPHER_SUITES_BYTES)
        {
            break;
        }

        if (
            (((Req->Auth.PayloadType 	     << 6) | (Req->Auth.Algorithm & 0x3F))     	 	== g_CipherRec [(i * 5) + 2]) &&
            (((Req->Integrity.PayloadType    << 6) | (Req->Integrity.Algorithm & 0x3F))     == g_CipherRec [(i * 5) + 3]) &&
            (((Req->Confidentiality.PayloadType<< 6) | (Req->Confidentiality.Algorithm & 0x3F))== g_CipherRec [(i * 5) + 4])
           )
        {
            id = g_CipherRec[(i * 5) + 1];
            /* The Cipher Suite ID is matched */
            /* get the privilege level for the given cipher suite id, if id is even its lower nibble else upper nibble */
            CipherSuitePrivilage = pBMCInfo->LANCfs[EthIndex].CipherSuitePrivLevels [(id/2)+1];
            CipherSuitePrivilage = ((id % 2) == 0) ? (CipherSuitePrivilage & 0x0f) : ((CipherSuitePrivilage >> 4) & 0x0f) ;

            if(CipherSuitePrivilage == 0)
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                Res->StatusCode = SC_NO_CIPHER_SUITE_MATCH;
                return sizeof (RSSPOpenSessionErrRes_T);
            }

            /*if requested privilege level is greater than cipher suite id privilege level return errror*/
            if ((CipherSuitePrivilage != 0) && (Role > CipherSuitePrivilage))
            {
                /* Invalid Payload Type */
                //Res->StatusCode = SC_UNAUTHORISED_ROLE;
                //return sizeof (RSSPOpenSessionErrRes_T);
                Role = CipherSuitePrivilage;
            }
            break;
        }
    }
    /* Role 0 indicates to find possible match */
    if (0 == Role)
    {
        IsMatchReq  = TRUE;
        Role        = MAX_ROLE_SUPPORTED;
    }

    /* Find Match */
    do
    {
        Res->StatusCode = SC_NO_ERROR;

        /*Check if requested authentication Algorithm supported **/
        if (0 != Req->Auth.PayloadLen)
        {
            if ((sizeof (RSSPPayloadInfo_T) != Req->Auth.PayloadLen) ||
                (0 == (pBMCInfo->RMCPPlus[EthIndex].Algorithm [Req->Auth.PayloadType] [Role] &
                         (1 << Req->Auth.Algorithm))))
            {
                /* AuthAlgorithm not supported */
                IPMI_DBG_PRINT ("RMCP+.c : OpenSessionReq - Authentication not supported \n");
                Res->StatusCode = SC_NO_CIPHER_SUITE_MATCH;
            }
        }

        /*Check if requested Integrity Algorithm supported  **/
        if (0 != Req->Integrity.PayloadLen)
        {
            if ((sizeof (RSSPPayloadInfo_T) != Req->Integrity.PayloadLen) ||
                (0 == (pBMCInfo->RMCPPlus[EthIndex].Algorithm [Req->Integrity.PayloadType] [Role] &
                         (1 << Req->Integrity.Algorithm))))
            {
                /* Integrity Algorithm not supported */
                IPMI_DBG_PRINT ("RMCP+.c : OpenSessionReq - Integrity not supported\n");
                Res->StatusCode = SC_NO_CIPHER_SUITE_MATCH;
            }
        }

        /*Check if requested Confidentiality Algorithm supported    **/
        if (0 != Req->Confidentiality.PayloadLen)
        {
            if ((sizeof (RSSPPayloadInfo_T) != Req->Confidentiality.PayloadLen) ||
                (0 == (pBMCInfo->RMCPPlus[EthIndex].Algorithm [Req->Confidentiality.PayloadType] [Role] &
                         (1 << Req->Confidentiality.Algorithm))))
            {
                /* Confidentiality Algorithm not supported */
                IPMI_DBG_PRINT ("RMCP+.c : OpenSessionReq - Confidentiality not supported \n");
                Res->StatusCode = SC_NO_CIPHER_SUITE_MATCH;
            }
        }

        if ((SC_NO_ERROR  == Res->StatusCode) &&
            ((CipherSuitePrivilage == 0) || (Role <= CipherSuitePrivilage)))
        {
            /* Found Match */
            break;
        }

    } while (IsMatchReq && Role--);

    /* Return Err response */
    if (Res->StatusCode != SC_NO_ERROR)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        IPMI_WARNING ("RMCP+.c : OpenSessionReq - Error Response\n");
        return sizeof (RSSPOpenSessionErrRes_T);
    }

    if (GetNumOfActiveSessions (BMCInst) >= pBMCInfo->IpmiConfig.MaxSession)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        /* No slot available */
        Res->StatusCode = SC_INSUFFICIENT_RESOURCE;
        return sizeof (RSSPOpenSessionErrRes_T);
    }
    /* Add Session Info */
    _fmemset (&SessionInfo, 0, sizeof (SessionInfo_T));
    /* Activated in ActivateSession Command */
    SessionInfo.Activated       = FALSE;
    SessionInfo.RemConSessionID = Req->RemConSessionID;
    SessionInfo.MaxPrivilege = Role;

    do
    {
        /*generate 32 bit temp session Id*/
        RAND_bytes((INT8U*)&TempSessId,sizeof(INT32U));
    } while ((NULL != getSessionInfo (SESSION_ID_INFO, &TempSessId, BMCInst)) ||
             (0 == TempSessId));

    SessionInfo.SessionID               = TempSessId;
    SessionInfo.Channel                 = Channel;
    SessionInfo.AuthAlgorithm           = Req->Auth.Algorithm;
    SessionInfo.IntegrityAlgorithm      = Req->Integrity.Algorithm;
    SessionInfo.ConfidentialityAlgorithm = Req->Confidentiality.Algorithm;
    if(pParams->IsPktFromLoopBack)
    {
        if(IPMITimeout > 0)
        {
            SessionInfo.TimeOutValue = (IPMITimeout+10);
        }
        else
        {
            /*If it is not defined the timeout values for loop back session should be 
                    SESSION_TIMEOUT defined in config.make.ipmi (60 seconds) */
            SessionInfo.TimeOutValue =  pBMCInfo->IpmiConfig.SessionTimeOut;
        }
        SessionInfo.IsLoopBack              = TRUE;
    }
    else
        SessionInfo.TimeOutValue          = pBMCInfo->IpmiConfig.SessionTimeOut;
    
    if(pBMCInfo->IpmiConfig.SOLIfcSupport == 1)
    {
        if(SessionInfo.SessPyldInfo [PAYLOAD_SOL].Type == PAYLOAD_SOL)
        {
            SessionInfo.TimeOutValue = pBMCInfo->IpmiConfig.SOLSessionTimeOut;
        }
    }
    
    SessionInfo.UserId  = 0xff;
    SessionInfo.SessPyldInfo [0].AuxConfig [0] =
        ((0 != Req->Integrity.Algorithm) ? 0x40: 0x00) |
        ((0 != Req->Confidentiality.Algorithm) ? 0x80: 0x00);
    SessionInfo.InitialInboundSeq   = SEQNUM_ROLLOVER;
    SessionInfo.InboundSeq  = 0x00;

    for(i=0; i < RMCPPLUS_SEQLOWLIMIT; i++)
    {
        if(((SessionInfo.InitialInboundSeq - (i+1)) != 0) &&(TrackRollOver == FALSE))
            SessionInfo.InboundTrac[i] = SessionInfo.InitialInboundSeq - (i+1);
        else if(((SessionInfo.InitialInboundSeq - (i+1)) == 0) &&(TrackRollOver == FALSE))
        {
            SessionInfo.InboundTrac[i] = SessionInfo.InitialInboundSeq - (i+1);
            TrackRollOver = TRUE;
        }
        else if(TrackRollOver == TRUE)
        {
            SessionInfo.InboundTrac[i] = TrackRollOverSeq;
            TrackRollOverSeq--;
        }
    }

    SessionInfo.InboundRecv = 0xFF; 
    AddSession (&SessionInfo, BMCInst);

    /* Load RSSP Open session Response */
    Res->Role = Role;
    Res->Reserved = 0x00;
    Res->RemConSessionID    = Req->RemConSessionID;
    Res->MgdSysSessionID    = SessionInfo.SessionID;

    /* Auth Algorithm Details */
    Res->Auth.PayloadType   = AUTH_ALGORITHM;
    Res->Auth.Reserved1 = 0;
    Res->Auth.PayloadLen    = sizeof (RSSPPayloadInfo_T);
    Res->Auth.Algorithm     = Req->Auth.Algorithm;
    memset(Res->Auth.Reserved2, 0x0, 3);

    /* Integrity Algorithm Details */
    Res->Integrity.PayloadType  = INTEGRITY_ALGORITHM;
    Res->Integrity.Reserved1 = 0;
    Res->Integrity.PayloadLen   = sizeof (RSSPPayloadInfo_T);
    Res->Integrity.Algorithm    = Req->Integrity.Algorithm;
    memset(Res->Integrity.Reserved2, 0x0, 3);

    /* Confidentiality Algorithm Details */
    Res->Confidentiality.PayloadType = CONFIDENTIALITY_ALGORITHM;
    Res->Confidentiality.Reserved1 = 0;
    Res->Confidentiality.PayloadLen  = sizeof (RSSPPayloadInfo_T);
    Res->Confidentiality.Algorithm   = Req->Confidentiality.Algorithm;
    memset(Res->Confidentiality.Reserved2, 0x0, 3);
    
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);

    return sizeof (RSSPOpenSessionRes_T);
}


/*----------------------------------------
 * RAKPMsg1
 *----------------------------------------*/
int
RAKPMsg1 (INT8U* pReq, INT8U ReqLen, INT8U* pRes, MiscParams_T *pParams, INT8U Channel, int BMCInst)
{
    RAKPMsg1Req_T       *Req     = (RAKPMsg1Req_T*) pReq;
    RAKPMsg2Res_T       *Res     = (RAKPMsg2Res_T*) pRes;
    RAKPMsg2ErrRes_T    *ErrRes  = (RAKPMsg2ErrRes_T*) pRes;
      SessionInfo_T       *pSessInfo;
      ChannelUserInfo_T   *pChUserInfo;
      ChannelInfo_T       *pChannelInfo;
      UserInfo_T          *pUserInfo;
      RAKPMsg1HMAC_T      *pMsghmac;
      BMCInfo_t           *pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U   Index;
    INT8U   AuthCodeLen = 0;
    INT8U   Role;
    INT32U  ManSysSessionID;
    INT8U PwdEncKey[MAX_SIZE_KEY + 1] = {0};
    char  EncryptedPswd[MAX_ENCRYPTED_PSWD_LEN + 1] = {0};

    /*Validate Mgd session ID*/
    Role                    = Req->Role & 0x0f;
    ManSysSessionID         = Req->ManSysSessionID;
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex,WAIT_INFINITE);
    pSessInfo               = getSessionInfo (SESSION_ID_INFO, (void*)&ManSysSessionID, BMCInst);
    if (NULL == pSessInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        IPMI_WARNING ("RMCP+.c : RAKPMsg1 - Invalid Session ID\n");
        ErrRes->StatusCode  = SC_INV_SESSION_ID;
        return sizeof (RAKPMsg2ErrRes_T);
    }
    /* In case of error, delete this session info */
    pBMCInfo->LANConfig.DeleteThisLANSessionID = ManSysSessionID;

    if(Channel == 0xFF)
    {
        Channel= pSessInfo->Channel;
    }

    ErrRes->RemConSessionID = pSessInfo->RemConSessionID;

    /* Check Role  */
    if (Role >  MAX_ROLE_SUPPORTED)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        /* Invalid Role */
        IPMI_WARNING ("RMCP+.c : RAKPMsg1 - Invalid Role\n");
        Res->StatusCode = SC_INV_ROLE;
        return sizeof (RAKPMsg2ErrRes_T);
    }

    /* Check User Name Length */
    if (Req->UsrNameLen > MAX_USERNAME_LEN)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        /* Invalid Name Len */
        IPMI_WARNING ("RMCP+.c : RAKPMsg1 - Username len exceeded\n");
        Res->StatusCode = SC_INV_NAME_LEN;
        return sizeof (RAKPMsg2ErrRes_T);
    }

    /* Privilege for the session has to be assigned in RAKP1 only */
    pSessInfo->Privilege = Role & 0xF;

    /* Pad with NULL characters */
    _fmemset (&Req->UsrName [Req->UsrNameLen], 0, MAX_USERNAME_LEN - Req->UsrNameLen);

    pChannelInfo = getChannelInfo (Channel, BMCInst);
    if (NULL == pChannelInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        /* InSufficient resources */
        IPMI_WARNING ("RMCP+.c : RAKPMsg1 - Insufficient resource\n");
        Res->StatusCode = SC_INSUFFICIENT_RESOURCE;
        return sizeof (RAKPMsg2ErrRes_T);
    }


    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pSessInfo->Lookup = ((Req->Role & 0x10) >> 0x04);

    if( pParams->IsPktFromLoopBack == TRUE  &&  Req->UsrName[0] == 0 )
    {
        pUserInfo = getUserIdInfo (NULL_USER, BMCInst);
        pSessInfo->UserId = NULL_USER;
        memset (pSessInfo->Password, 0, MAX_PASSWORD_LEN);
    }
    else
    {
        if (USER_ROLE_LOOKUP == pSessInfo->Lookup)
        {
        /* Get userInfo for the given userName and privilege */
        pChUserInfo = getChUserPrivInfo ((char *)Req->UsrName, Role, &Index,
        pChannelInfo->ChannelUserInfo, BMCInst);

        /* If user not found  */
        if (NULL == pChUserInfo)
        {
            /* Invalid user     */
            IPMI_WARNING ("RMCP+.c : RAKPMsg1 - user_priv - User not found : %s\n", (char *)Req->UsrName);

            if ( 0 != AddLoginEvent( 0xFF, Req->UsrName, EVENT_LOGIN_FAILURE, BMCInst ) )
            {
                    TCRIT("Problem while adding Log record \n");
            }
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            Res->StatusCode = SC_UNAUTHORISED_NAME;
            return sizeof (RAKPMsg2ErrRes_T);
        }

        /* Check Role  */
        if (pChUserInfo->AccessLimit == 0xF )
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            /* Invalid Role */
            IPMI_WARNING ("RMCP+.c : RAKPMsg1 - user_priv - Invalid Role\n");
            Res->StatusCode = SC_INV_ROLE;
            return sizeof (RAKPMsg2ErrRes_T);
        }

        //we cannot assign the pSessInfo->privilege here..this will cause RAKP3 integrity check to fail
        //this is because the client will do integrity with the requested role rather than the max permissible role
        // which we set here. client has no way of knowing at that time
        //pSessInfo->Privilege = Role;
        }
        else /* if (NAME_ONLY_LOOKUP == pSessInfo->Lookup) */
        {
            /* Get userInfo for the given userName*/
            pChUserInfo = getChUserInfo ((char *)Req->UsrName, &Index,
            pChannelInfo->ChannelUserInfo, BMCInst);
            /* If user not found  */
            if (NULL == pChUserInfo)
            {
                /* Invalid user     */
                IPMI_WARNING ("RMCP+.c : RAKPMsg1 - name_only - User not found\n");

                if ( 0 != AddLoginEvent( 0xFF, Req->UsrName, EVENT_LOGIN_FAILURE, BMCInst ))
                {
                    TCRIT("Problem while adding Log record \n");
                }

                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                Res->StatusCode = SC_UNAUTHORISED_NAME;
                return sizeof (RAKPMsg2ErrRes_T);
            }

            /* Check Role  */
            if (pChUserInfo->AccessLimit == 0xF )
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                /* Invalid Role */
                IPMI_WARNING ("RMCP+.c : RAKPMsg1 - name_only - Invalid Role\n");
                Res->StatusCode = SC_INV_ROLE;
                return sizeof (RAKPMsg2ErrRes_T);
            }
        }

        pUserInfo = getUserIdInfo (pChUserInfo->UserId, BMCInst);
        if (pUserInfo == NULL || FALSE == pUserInfo->UserStatus)
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            /*user name not enabled*/
            IPMI_WARNING ("RMCP+.c : RAKPMsg1 - User not found in database\n");
            Res->StatusCode = SC_UNAUTHORISED_NAME;
            return sizeof (RAKPMsg2ErrRes_T);
        }

        /* Load UserId Session Info */
        pSessInfo->UserId = (INT8U)pChUserInfo->UserId;
        if (g_corefeatures.userpswd_encryption == ENABLED)
        {
            /* Get Encryption Key from the MBMCInfo_t structure */
            memcpy(PwdEncKey, &(g_MBMCInfo.PwdEncKey), MAX_SIZE_KEY);
            if(g_corefeatures.encrypted_pwd_in_hexa_format == ENABLED)
            {
                //Extract Hex Array content from string
                ConvertStrtoHex((char *)pBMCInfo->EncryptedUserInfo[pChUserInfo->UserId - 1].EncryptedHexPswd, (char *)EncryptedPswd, MAX_ENCRYPTED_PSWD_LEN);
            }
            else
            {
                Decode64((char *)EncryptedPswd, (char *)pBMCInfo->EncryptedUserInfo[pChUserInfo->UserId - 1].EncryptedHexPswd,MAX_ENCRYPTED_PSWD_LEN );
            }

            if(DecryptPassword((INT8S *)EncryptedPswd, MAX_PASSWORD_LEN, (INT8S*)pSessInfo->Password, MAX_PASSWORD_LEN, PwdEncKey))
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                TCRIT("Error in decrypting the user password for user ID:%d. .\n", pChUserInfo->UserId);
                Res->StatusCode  = SC_INSUFFICIENT_RESOURCE;
                return sizeof(RAKPMsg2ErrRes_T);
            }
        }
        else
        {
        	_fmemcpy (pSessInfo->Password, pUserInfo->UserPassword, MAX_PASSWORD_LEN);
        }
    }

    /* Check for Empty password login */
    if ( ENABLED == g_corefeatures.disable_empty_passwd_login && NULL_USER != pSessInfo->UserId && 0 == pSessInfo->Password[0])
    {
        IPMI_WARNING("RMCP+c : RAKPMsg1 - Empty Password Login is not allowed \n");

        if ( 0 != AddLoginEvent(pSessInfo->UserId, NULL, EVENT_LOGIN_FAILURE, BMCInst))
        {
             TCRIT("Problem while adding Log record \n");
        }

        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        Res->StatusCode = SC_ILLEGAL_OR_UNRECOGNIZED_PARAM;
        return sizeof(RAKPMsg2ErrRes_T);
    }

    /* Check for Default Empty password login */
    if(ENABLED != g_corefeatures.allow_default_empty_passwd_login && NULL_USER != pSessInfo->UserId && 0 == pSessInfo->Password[0] && pUserInfo->UserPasswdConfigured == 0)
    {
        TDBG("RMCP+c : RAKPMsg1 - User Password is not yet configured; Default Empty Password Login is not allowed \n");
        if ( 0 != AddLoginEvent(pSessInfo->UserId, NULL, EVENT_LOGIN_FAILURE, BMCInst))
        {
             TCRIT("Problem while adding Log record \n");
        }
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        Res->StatusCode = SC_ILLEGAL_OR_UNRECOGNIZED_PARAM;
        return sizeof(RAKPMsg2ErrRes_T);
    }

    if(FindUserLockStatus(pSessInfo->UserId, Channel, BMCInst) != 0)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        IPMI_WARNING("RMCP+c : RAKPMsg1 - User is Locked \n");
        /* As of now sending Invalid Role for bad password*/
        Res->StatusCode = SC_INV_ROLE;
        return sizeof(RAKPMsg2ErrRes_T);
    }

    /* Check the number of active sessions */
    if (pUserInfo->CurrentSession >= pUserInfo->MaxSession)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        /*can't accept any more session for this user*/
        Res->StatusCode = SC_INSUFFICIENT_RESOURCE;
        return  sizeof (RAKPMsg2ErrRes_T);
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    /* Load Response */
    Res->StatusCode = SC_NO_ERROR;
    /* Store the Remote Console Random number in SessionInfo */
    _fmemcpy (pSessInfo->RemConRandomNo, Req->RemConRandomNo,
              MAX_REM_CON_RAND_NO_LEN);

    RAND_bytes(Res->ManSysRandomNo,MAX_MGD_SYS_RAND_NO_LEN);

    /* Store the Managed System Random number in SessionInfo */
    _fmemcpy (pSessInfo->MgdSysRandomNo, Res->ManSysRandomNo,
              MAX_MGD_SYS_RAND_NO_LEN);

    /* Copy the System GUID */
    _fmemcpy (Res->ManSysGUID, BMC_GET_SHARED_MEM (BMCInst)->SystemGUID, 16);

    /* Key Exchange Auth Code  */
    switch (pSessInfo->AuthAlgorithm)
    {
        case RAKP_NONE:
            AuthCodeLen = 0;
            break;

        case RAKP_HMAC_SHA1:
            {
                INT8U PasswdLen = 0;

                pMsghmac                    = (RAKPMsg1HMAC_T*)pBMCInfo->LANConfig.HmacInBuf;
                pMsghmac->RemConSessionID   = pSessInfo->RemConSessionID;
                pMsghmac->MgdSysSessionID   = pSessInfo->SessionID;
                /* Copy Random no.s  and GUID */
                _fmemcpy (pMsghmac->RemConRandNo, pSessInfo->RemConRandomNo, 16);
                _fmemcpy (pMsghmac->MgdSysRandNo, pSessInfo->MgdSysRandomNo, 16);
                _fmemcpy (pMsghmac->MgdSysGUID, Res->ManSysGUID, MAX_MGD_SYS_GUID_LEN);

                pMsghmac->Role          = Req->Role;
                pMsghmac->UsrNameLen    = Req->UsrNameLen;
                _fmemcpy (pMsghmac->UsrName, Req->UsrName, Req->UsrNameLen);

                PasswdLen = _fstrlen ((char*)pSessInfo->Password);
                PasswdLen = (PasswdLen > MAX_PASSWORD_LEN) ?
                            MAX_PASSWORD_LEN : PasswdLen;
                HMAC(EVP_sha1(), (INT8U *)pSessInfo->Password, PasswdLen
                                , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, (sizeof (RAKPMsg1HMAC_T) - MAX_USERNAME_LEN + Req->UsrNameLen)
                                , (INT8U *)&pRes [sizeof (RAKPMsg2Res_T)], NULL);

                AuthCodeLen = RAKP1_HASH_SIZE;
                break;
            }

        case RAKP_HMAC_MD5:
            {
                INT8U PasswdLen = 0;

                pMsghmac                    = (RAKPMsg1HMAC_T*)pBMCInfo->LANConfig.HmacInBuf;
                pMsghmac->RemConSessionID   = pSessInfo->RemConSessionID;
                pMsghmac->MgdSysSessionID   = pSessInfo->SessionID;
                /* Copy Random no.s  and GUID */
                _fmemcpy (pMsghmac->RemConRandNo, pSessInfo->RemConRandomNo, 16);
                _fmemcpy (pMsghmac->MgdSysRandNo, pSessInfo->MgdSysRandomNo, 16);
                _fmemcpy (pMsghmac->MgdSysGUID, Res->ManSysGUID, MAX_MGD_SYS_GUID_LEN);

                pMsghmac->Role          = Req->Role;
                pMsghmac->UsrNameLen    = Req->UsrNameLen;
                _fmemcpy (pMsghmac->UsrName, Req->UsrName, Req->UsrNameLen);

                PasswdLen = _fstrlen ((char*)pSessInfo->Password);
                PasswdLen = (PasswdLen > MAX_PASSWORD_LEN) ?
                            MAX_PASSWORD_LEN : PasswdLen;
                HMAC(EVP_md5(), (INT8U *)pSessInfo->Password, PasswdLen
                                , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, (sizeof (RAKPMsg1HMAC_T) - MAX_USERNAME_LEN + Req->UsrNameLen)
                                , (INT8U *)&pRes [sizeof (RAKPMsg2Res_T)], NULL);
                AuthCodeLen = RAKP1_HASH_HMAC_MD5_SIZE;
                break;
            }

        case RAKP_HMAC_SHA256:
			{
        		INT8U PasswdLen = 0;

         		pMsghmac                    = (RAKPMsg1HMAC_T*)pBMCInfo->LANConfig.HmacInBuf;
                pMsghmac->RemConSessionID   = pSessInfo->RemConSessionID;
                pMsghmac->MgdSysSessionID   = pSessInfo->SessionID;
                /* Copy Random nos  and GUID */
				_fmemcpy (pMsghmac->RemConRandNo, pSessInfo->RemConRandomNo, 16);
				_fmemcpy (pMsghmac->MgdSysRandNo, pSessInfo->MgdSysRandomNo, 16);
				_fmemcpy (pMsghmac->MgdSysGUID, Res->ManSysGUID, MAX_MGD_SYS_GUID_LEN);

				pMsghmac->Role          = Req->Role;
				pMsghmac->UsrNameLen    = Req->UsrNameLen;
				_fmemcpy (pMsghmac->UsrName, Req->UsrName, Req->UsrNameLen);

				PasswdLen = _fstrlen ((char*)pSessInfo->Password);
				PasswdLen = (PasswdLen > MAX_PASSWORD_LEN) ?
								MAX_PASSWORD_LEN : PasswdLen;

                HMAC(EVP_sha256(), (INT8U *)pSessInfo->Password, PasswdLen
                                , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, (sizeof (RAKPMsg1HMAC_T) - MAX_USERNAME_LEN + Req->UsrNameLen)
                                , (INT8U *)&pRes [sizeof (RAKPMsg2Res_T)], NULL);
				AuthCodeLen = RAKP1_HASH_HMAC_SHA256_SIZE;
                break;
           	}        	
			
        default:
            IPMI_WARNING ("RMCP+.c : RAKPMsg1 - Invalid Authentication\n");
            AuthCodeLen = 0;
    }

    pBMCInfo->LANConfig.DeleteThisLANSessionID = 0;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);

    return sizeof (RAKPMsg2Res_T) + AuthCodeLen;
}


/*----------------------------------------
 * RAKPMsg3
 *----------------------------------------*/
int
RAKPMsg3 (INT8U* pReq, INT8U ReqLen, INT8U* pRes, MiscParams_T *pParams,INT8U Channel, int BMCInst)
{
    RAKPMsg3Req_T       *Req     = (RAKPMsg3Req_T*)pReq;
    RAKPMsg4Res_T       *Res     = (RAKPMsg4Res_T*)pRes;
      SessionInfo_T       *pSessInfo;
    INT8U               *pKeyXchgCode;
    SIKhmac_T           *pSIKhmac;
      Msg3hmac_T          *pMsg3hmac;
      UserInfo_T          *pUserInfo;
      RAKPMsg4hmac_T      *pMsg4hmac;
      ChannelInfo_T       *pChannelInfo;
      ChannelUserInfo_T   *pChUserInfo;
      BMCInfo_t           *pBMCInfo = &g_BMCInfo[BMCInst];
            INT8U               TempKey [HASH_KEY1_CONST_SIZE];
            INT8U               TempKey2 [HASH_KEY2_CONST_SIZE];
            INT8U               UserPasswdLen=0;
            int                 i;
            INT8U               ResIntigrityKeyLen=0;
            INT8U               Index;
            INT8U               EthIndex;
            INT8U               UserPswd [MAX_PASSWORD_LEN];
            char  EncryptedPswd[MAX_ENCRYPTED_PSWD_LEN + 1] = {0};
            INT32U              SessionID;
            INT8U PwdEncKey[MAX_SIZE_KEY + 1] = {0};

    SessionID = Req->ManSysSessionID;
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex,WAIT_INFINITE);
    pSessInfo = getSessionInfo (SESSION_ID_INFO, &SessionID, BMCInst);
    /* In case of error, delete this session info */
    pBMCInfo->LANConfig.DeleteThisLANSessionID = Req->ManSysSessionID;

    /* Check if Management System SessionID */
    if (NULL == pSessInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        IPMI_WARNING ("RMCP+.c : RAKPMsg3 - Invalid Session ID\n");
        Res->StatusCode = SC_INV_SESSION_ID;
        return sizeof (RAKPMsg4Res_T);
    }

    /* Check if previous transactions caused an error */
    if (Req->StatusCode !=  SC_NO_ERROR)
    {
        if(Req->StatusCode == SC_INV_INTEGRITY_CHECK)
        {
            OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
            LockUser(pSessInfo->UserId,Channel, BMCInst);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        }
        IPMI_WARNING ("RMCP+.c : RAKPMsg3 - Invalid Status\n");

        if ( 0 != AddLoginEvent( pSessInfo->UserId, NULL, EVENT_LOGIN_FAILURE, BMCInst ))
        {
            IPMI_WARNING("Problem while adding Log record \n");
        }

        Res->StatusCode = Req->StatusCode;
        DeleteSession(pSessInfo,BMCInst);
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        return 0;
    }

    UnlockUser(pSessInfo->UserId,Channel,BMCInst);

    if(Channel == 0xFF)
    {
        Channel= pSessInfo->Channel;
    }

    EthIndex= GetEthIndex(Channel, BMCInst);

    if(0xff == EthIndex)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    /* Get User Info */
    pUserInfo = getUserIdInfo((INT8U)pSessInfo->UserId, BMCInst);
    if (NULL == pUserInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        IPMI_WARNING ("RMCP+.c : RAKPMsg3 - User not found\n");
        Res->StatusCode = SC_UNAUTHORISED_NAME;
        return sizeof (RAKPMsg4Res_T);
    }

    if (g_corefeatures.userpswd_encryption == ENABLED)
    {
        /* Get Encryption Key from the MBMCInfo_t structure */
            memcpy(PwdEncKey, &(g_MBMCInfo.PwdEncKey), MAX_SIZE_KEY);
            if(g_corefeatures.encrypted_pwd_in_hexa_format == ENABLED)
            {
                //Extract Hex Array content from string
                ConvertStrtoHex((char *)pBMCInfo->EncryptedUserInfo[pSessInfo->UserId - 1].EncryptedHexPswd, (char *)EncryptedPswd, MAX_ENCRYPTED_PSWD_LEN);
            }
            else
            {
                Decode64( (char *)EncryptedPswd, (char *)pBMCInfo->EncryptedUserInfo[pSessInfo->UserId - 1].EncryptedHexPswd, MAX_ENCRYPTED_PSWD_LEN);
            }
        if(DecryptPassword((INT8S *)EncryptedPswd, MAX_PASSWORD_LEN, (char *)UserPswd, MAX_PASSWORD_LEN, PwdEncKey))
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            IPMI_ERROR("Error in decrypting the user password for user ID:%d. .\n", pSessInfo->UserId);
            Res->StatusCode = CC_UNSPECIFIED_ERR;
            return sizeof(RAKPMsg4Res_T);
        }
    }
    else
    {
    	_fmemcpy (UserPswd, pUserInfo->UserPassword, MAX_PASSWORD_LEN);
    }

    UserPasswdLen = _fstrlen ((char*)UserPswd);
    UserPasswdLen = (UserPasswdLen > MAX_PASSWORD_LEN) ?
                    MAX_PASSWORD_LEN : UserPasswdLen;

    /* Check for Key Exchange Auth Code */
    pKeyXchgCode = (INT8U*)(pReq + sizeof (RAKPMsg3Req_T));

    /*Construct hmac to check Auth code */
    pMsg3hmac = (_FAR_	Msg3hmac_T*) &pBMCInfo->LANConfig.HmacInBuf;
    _fmemcpy (pMsg3hmac->MgdSysRandNo, pSessInfo->MgdSysRandomNo, 16);
    pMsg3hmac->RemConSessionID  = pSessInfo->RemConSessionID;
    pMsg3hmac->Role             = pSessInfo->Privilege | (pSessInfo->Lookup << 4);
    pMsg3hmac->UsrNameLen       = _fstrlen ((char*)pUserInfo->UserName);
    pMsg3hmac->UsrNameLen        = (pMsg3hmac->UsrNameLen > MAX_USERNAME_LEN) ?
                                        MAX_USERNAME_LEN : pMsg3hmac->UsrNameLen;

    _fmemcpy ((char*)pMsg3hmac->UsrName,
          (char*)pUserInfo->UserName, pMsg3hmac->UsrNameLen);

    /* Key Exchange Auth Code  */
    switch (pSessInfo->AuthAlgorithm)
    {
        case RAKP_NONE:
            break;

        case RAKP_HMAC_SHA1:
                HMAC(EVP_sha1(), (INT8U *)UserPswd, UserPasswdLen
                                , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, sizeof (Msg3hmac_T) - (16 - pMsg3hmac->UsrNameLen)
                                , (INT8U *)&pRes [ sizeof (RAKPMsg4Res_T)], NULL);

            if (0 != _fmemcmp (pKeyXchgCode, &pRes [sizeof (RAKPMsg4Res_T)], RAKP1_HASH_SIZE))
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                IPMI_WARNING ("RMCP+.c : RAKPMsg3 - Invalid integrity check\n");
                Res->StatusCode = SC_INV_INTEGRITY_CHECK;
                return sizeof (RAKPMsg4Res_T);
            }
            break;

        case RAKP_HMAC_MD5:
                HMAC(EVP_md5(), (INT8U *)UserPswd, UserPasswdLen
                                , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, sizeof (Msg3hmac_T) - (MAX_USERNAME_LEN - pMsg3hmac->UsrNameLen)
                                , (INT8U *)&pRes [ sizeof (RAKPMsg4Res_T)], NULL);

            if (0 != _fmemcmp (pKeyXchgCode, &pRes [sizeof (RAKPMsg4Res_T)], RAKP1_HASH_HMAC_MD5_SIZE))
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                IPMI_WARNING ("RMCP+.c : RAKPMsg3 - Invalid integrity check\n");
                Res->StatusCode = SC_INV_INTEGRITY_CHECK;
                return sizeof (RAKPMsg4Res_T);
            }
            break;

        case RAKP_HMAC_SHA256:
                HMAC(EVP_sha256(), (INT8U *)pSessInfo->Password, UserPasswdLen
                                , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, sizeof (Msg3hmac_T) - (MAX_USERNAME_LEN - pMsg3hmac->UsrNameLen)
                                , (INT8U *)&pRes [ sizeof (RAKPMsg4Res_T)], NULL);

        	 if (0 != _fmemcmp (pKeyXchgCode, &pRes [sizeof (RAKPMsg4Res_T)], RAKP1_HASH_HMAC_SHA256_SIZE))
                {
                    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
                    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                    IPMI_WARNING ("RMCP+.c : RAKPMsg3 - Invalid integrity check\n");
                    Res->StatusCode = SC_INV_INTEGRITY_CHECK;
                    return sizeof (RAKPMsg4Res_T);
                }
                break;        

        default:
            IPMI_WARNING ("RMCP+.c : RAKPMsg3 - Invalid Authentication\n");
    }

    /*Construct SIK to send the Integrity check */
    memset(pBMCInfo->LANConfig.HmacInBuf,0,sizeof(pBMCInfo->LANConfig.HmacInBuf));
    memset(pBMCInfo->LANConfig.SIK,0,sizeof(pBMCInfo->LANConfig.SIK));

    pSIKhmac = ( SIKhmac_T*) &pBMCInfo->LANConfig.HmacInBuf;

    _fmemcpy (pSIKhmac->RemConRandNo, pSessInfo->RemConRandomNo, 16);
    _fmemcpy (pSIKhmac->MgdSysRandNo, pSessInfo->MgdSysRandomNo, 16);
    pSIKhmac->Role = pSessInfo->Privilege | (pSessInfo->Lookup << 4) ;
    pSIKhmac->UsrNameLen = _fstrlen ((char*)pUserInfo->UserName);
    pSIKhmac->UsrNameLen = (pSIKhmac->UsrNameLen > MAX_USERNAME_LEN) ?
                           MAX_USERNAME_LEN : pSIKhmac->UsrNameLen;
    _fmemcpy ((char*)pSIKhmac->UsrName,
              (char*)pUserInfo->UserName, pSIKhmac->UsrNameLen);

    for (i = 0; i < HASH_KEY_LEN; i++)
    {
        if (pBMCInfo->RMCPPlus[EthIndex].KGHashKey [i] != 0)
        {
            break;
        }
    }

    switch(pSessInfo->AuthAlgorithm)
    {
        case RAKP_NONE:
             break;

        case RAKP_HMAC_SHA1: 
        {
            // Encryption key must not be used for packets from VLAN or Loopback.
        #ifdef  LAN_RESTRICTIONS_BYPASS_FOR_LOOPBACK_AND_VLAN
            if ((i < HASH_KEY_LEN) && !(pParams->IsPktFromVLAN || pParams->IsPktFromLoopBack) )
        #else
            if ((i < HASH_KEY_LEN))
        #endif
            {
                /* Use the KG (BMC Key set through SetChSecurityKeys command) Key */
                HMAC(EVP_sha1(), (INT8U *)pBMCInfo->RMCPPlus[EthIndex].KGHashKey, HASH_KEY_LEN
                                , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, (sizeof (SIKhmac_T) - 16 + pSIKhmac->UsrNameLen)
                                , (INT8U *)pBMCInfo->LANConfig.SIK, NULL);

            }
            else
            {
                /* Use the KUID (User Password) Key */
                HMAC(EVP_sha1(), (INT8U *)UserPswd, UserPasswdLen
                                , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, (sizeof (SIKhmac_T) - 16 + pSIKhmac->UsrNameLen)
                                , (INT8U *)pBMCInfo->LANConfig.SIK, NULL);
            }

            /* Create Key1 & Key2 for Rest of packet Integrity & Encryption */
            _fmemset (TempKey, 1, HASH_KEY1_CONST_SIZE);
            HMAC(EVP_sha1(), (INT8U *)pBMCInfo->LANConfig.SIK, SESSION_INTEGRITY_KEY_SIZE
                            , (INT8U*)TempKey, HASH_KEY1_CONST_SIZE
                            , (INT8U *)pSessInfo->Key1, NULL);
            _fmemset (TempKey2, 2, HASH_KEY2_CONST_SIZE);
            HMAC(EVP_sha1(), (INT8U *)pBMCInfo->LANConfig.SIK, SESSION_INTEGRITY_KEY_SIZE
                            , (INT8U*)TempKey2, HASH_KEY2_CONST_SIZE
                            , (INT8U *)pSessInfo->Key2, NULL);

            /* Construct HMAC to send the Integrity check value using SIK got from prev hmac*/
            pMsg4hmac = (RAKPMsg4hmac_T*) &pBMCInfo->LANConfig.HmacInBuf;
            _fmemcpy (pMsg4hmac->RemConRandNo, pSessInfo->RemConRandomNo, 16);
            pMsg4hmac->MgdSysSessionID = pSessInfo->SessionID;
            /*Get System GUID */
            _fmemcpy (pMsg4hmac->MgdSysGUID, BMC_GET_SHARED_MEM (BMCInst)->SystemGUID, 16);
            HMAC(EVP_sha1(), (INT8U *)pBMCInfo->LANConfig.SIK, SESSION_INTEGRITY_KEY_SIZE
                            , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, sizeof (RAKPMsg4hmac_T)
                            , (INT8U *)&pRes [sizeof (RAKPMsg4Res_T)], NULL);
            ResIntigrityKeyLen = HMAC_SHA1_96_LEN;
            break;
        }

        case RAKP_HMAC_MD5:
            {
            #ifdef  LAN_RESTRICTIONS_BYPASS_FOR_LOOPBACK_AND_VLAN
                if ((i < HASH_KEY_LEN) && !(pParams->IsPktFromVLAN || pParams->IsPktFromLoopBack) )
            #else
                if ((i < HASH_KEY_LEN))
            #endif
                {
                    /* Use the KG (BMC Key set through SetChSecurityKeys command) Key */
                    HMAC(EVP_md5(), (INT8U *)pBMCInfo->RMCPPlus[EthIndex].KGHashKey, HASH_KEY_LEN
                                    , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, (sizeof (SIKhmac_T) - 16 + pSIKhmac->UsrNameLen)
                                    , (INT8U *)pBMCInfo->LANConfig.SIK, NULL);
                }
                else
                {
                    /* Use the KUID (User Password) Key */
                    HMAC(EVP_md5(), (INT8U *)UserPswd, UserPasswdLen
                                    , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, (sizeof (SIKhmac_T) - 16 + pSIKhmac->UsrNameLen)
                                    , (INT8U *)pBMCInfo->LANConfig.SIK, NULL);
                }
    
                /* Create Key1 & Key2 for Rest of packet Integrity & Encryption */
                _fmemset (TempKey, 1, HASH_KEY1_CONST_SIZE);
                    HMAC(EVP_md5(), (INT8U *)pBMCInfo->LANConfig.SIK, SESSION_HMAC_MD5_I_KEY_SIZE
                                    , (INT8U*)TempKey, HASH_KEY1_CONST_SIZE
                                    , (INT8U *)pSessInfo->Key1, NULL);
                _fmemset (TempKey2, 2, HASH_KEY2_CONST_SIZE);
                    HMAC(EVP_md5(), (INT8U *)pBMCInfo->LANConfig.SIK, SESSION_HMAC_MD5_I_KEY_SIZE
                                    , (INT8U*)TempKey2, HASH_KEY2_CONST_SIZE
                                    , (INT8U *)pSessInfo->Key2, NULL);

                /* Construct HMAC to send the Integrity check value using SIK got from prev hmac*/
                pMsg4hmac = (RAKPMsg4hmac_T*) &pBMCInfo->LANConfig.HmacInBuf;
                _fmemcpy (pMsg4hmac->RemConRandNo, pSessInfo->RemConRandomNo, 16);
                pMsg4hmac->MgdSysSessionID = pSessInfo->SessionID;
                /*Get System GUID */
                _fmemcpy (pMsg4hmac->MgdSysGUID, BMC_GET_SHARED_MEM (BMCInst)->SystemGUID, 16);
                    HMAC(EVP_md5(), (INT8U *)pBMCInfo->LANConfig.SIK, SESSION_INTEGRITY_KEY_SIZE
                                    , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, sizeof (RAKPMsg4hmac_T)
                                    , (INT8U *)&pRes [sizeof (RAKPMsg4Res_T)], NULL);
                ResIntigrityKeyLen = HMAC_MD5_LEN;
            }
            break;

        
 
#if 0   //algorithm not specified in IPMI spec, to be removed after review         
        case AUTH_MD5_128:
            {
                // Encryption key must not be used for packets from VLAN or Loopback.
            #ifdef  LAN_RESTRICTIONS_BYPASS_FOR_LOOPBACK_AND_VLAN
                if ((i < HASH_KEY_LEN) && !(pParams->IsPktFromVLAN || pParams->IsPktFromLoopBack) )
            #else
                if ((i < HASH_KEY_LEN))
            #endif
                {
                    /* Use the KG (BMC Key set through SetChSecurityKeys command) Key */
                    MD5_128 ((char *)pBMCInfo->RMCPPlus[EthIndex].KGHashKey, HASH_KEY_LEN, (char *)pBMCInfo->LANConfig.HmacInBuf,
                                (sizeof (SIKhmac_T) - 16 + pSIKhmac->UsrNameLen),
                                (char *)pBMCInfo->LANConfig.SIK, SESSION_MD5_KEY_SIZE);
                }
                else
                {
                    /* Use the KUID (User Password) Key */
                    MD5_128 ((char *)pUserInfo->UserPassword, UserPasswdLen, (char *)pBMCInfo->LANConfig.HmacInBuf,
                                (sizeof (SIKhmac_T) - 16 + pSIKhmac->UsrNameLen), (char *)pBMCInfo->LANConfig.SIK, SESSION_MD5_KEY_SIZE);
                }

                /* Create Key1 & Key2 for Rest of packet Integrity & Encryption */
                _fmemset (TempKey, 1, HASH_KEY1_CONST_SIZE);
                MD5_128 ((char *)pBMCInfo->LANConfig.SIK, SESSION_INTEGRITY_KEY_SIZE, (char *)TempKey, HASH_KEY1_CONST_SIZE,
                                (char *)pSessInfo->Key1, SESSION_MD5_KEY_SIZE);
                _fmemset (TempKey2, 2, HASH_KEY2_CONST_SIZE);
                MD5_128 ((char *)pBMCInfo->LANConfig.SIK, SESSION_INTEGRITY_KEY_SIZE, (char *)TempKey2, HASH_KEY2_CONST_SIZE,
                                (char *)pSessInfo->Key2, SESSION_MD5_KEY_SIZE);

                /* Construct HMAC to send the Integrity check value using SIK got from prev hmac*/
                pMsg4hmac = (RAKPMsg4hmac_T*) &pBMCInfo->LANConfig.HmacInBuf;
                _fmemcpy (pMsg4hmac->RemConRandNo, pSessInfo->RemConRandomNo, 16);
                pMsg4hmac->MgdSysSessionID = pSessInfo->SessionID;

                /*Get System GUID */
                _fmemcpy (pMsg4hmac->MgdSysGUID, BMC_GET_SHARED_MEM (BMCInst)->SystemGUID, 16);
                MD5_128 ((char *)pBMCInfo->LANConfig.SIK, SESSION_INTEGRITY_KEY_SIZE, (char *)pBMCInfo->LANConfig.HmacInBuf, sizeof (RAKPMsg4hmac_T),
                                (char *)&pRes [sizeof (RAKPMsg4Res_T)], SESSION_MD5_KEY_SIZE);
                ResIntigrityKeyLen = MD5_LEN;
                break;
            }
#endif           
            
	case RAKP_HMAC_SHA256:
	{
		// Encryption key must not be used for packets from VLAN or Loopback.
		#ifdef	LAN_RESTRICTIONS_BYPASS_FOR_LOOPBACK_AND_VLAN
			if ((i < HASH_KEY_LEN) && !(pParams->IsPktFromVLAN || pParams->IsPktFromLoopBack) )
		#else
			if ((i < HASH_KEY_LEN))
		#endif
			{
				/* Use the KG (BMC Key set through SetChSecurityKeys command) Key */
                    HMAC(EVP_sha256(), (INT8U *)pBMCInfo->RMCPPlus[EthIndex].KGHashKey, HASH_KEY_LEN
                                    , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, (sizeof (SIKhmac_T) - 16 + pSIKhmac->UsrNameLen)
                                    , (INT8U *)m_SIK, NULL);
			}
			else
			{
				/* Use the KUID (User Password) Key */
                    HMAC(EVP_sha256(), (INT8U *)UserPswd, UserPasswdLen
                                    , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, (sizeof (SIKhmac_T) - 16 + pSIKhmac->UsrNameLen)
                                    , (INT8U *)m_SIK, NULL);
					
			}

		/* Create Key1 & Key2 for Rest of packet Integrity & Encryption */
				_fmemset (TempKey, 1, HASH_KEY1_CONST_SIZE);
                HMAC(EVP_sha256(), (INT8U *)m_SIK, SHA2_HASH_KEY_SIZE
                                 , (INT8U*)TempKey, HASH_KEY1_CONST_SIZE
                                 , (INT8U *)pSessInfo->Key1, NULL);
				_fmemset (TempKey2, 2, HASH_KEY2_CONST_SIZE);
             HMAC(EVP_sha256(), (INT8U *)m_SIK, SHA2_HASH_KEY_SIZE
                              , (INT8U*)TempKey2, HASH_KEY2_CONST_SIZE
                              , (INT8U *)pSessInfo->Key2, NULL);
       /* Construct HMAC to send the Integrity check value using SIK got from prev hmac*/
			 pMsg4hmac = (RAKPMsg4hmac_T*) &pBMCInfo->LANConfig.HmacInBuf;
			 _fmemcpy (pMsg4hmac->RemConRandNo, pSessInfo->RemConRandomNo, 16);
			pMsg4hmac->MgdSysSessionID = pSessInfo->SessionID;
			 /*Get System GUID */
			 _fmemcpy (pMsg4hmac->MgdSysGUID, BMC_GET_SHARED_MEM (BMCInst)->SystemGUID, 16);
             HMAC(EVP_sha256(), (INT8U *)m_SIK, SHA2_HASH_KEY_SIZE
                              , (INT8U*)pBMCInfo->LANConfig.HmacInBuf, sizeof (RAKPMsg4hmac_T)
                              , (INT8U *)&pRes [sizeof (RAKPMsg4Res_T)], NULL);
			 ResIntigrityKeyLen = HMAC_SHA256_128_LEN;
	 
			break;
	}
        default:
            IPMI_DBG_PRINT("\nRMCP+.c : Invalid Integrity Algorithm \n");
    }

    /* Get information abt this channel */
    pChannelInfo = getChannelInfo (Channel, BMCInst);
    if (NULL == pChannelInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        IPMI_WARNING ("RMCP+.c : RAKPMsg3 - Invalid Integrity check\n");
        Res->StatusCode = SC_INV_INTEGRITY_CHECK;
        return sizeof (RAKPMsg4Res_T);
    }

    pUserInfo = getUserIdInfo((INT8U)pSessInfo->UserId, BMCInst);

    if((!pSessInfo->IsLoopBack) && (pSessInfo->Activated == FALSE))
    {
        /* Update active sessions for current user */
        pUserInfo->CurrentSession++;

        /* Number of active session */
        pChannelInfo->ActiveSession++;
    }

    /* Get userInfo for the given Used Id & Channel */
    pChUserInfo = getChUserIdInfo (pSessInfo->UserId, &Index, pChannelInfo->ChannelUserInfo, BMCInst);

#ifdef  LAN_RESTRICTIONS_BYPASS_FOR_LOOPBACK_AND_VLAN
    if( pParams->IsPktFromVLAN || pParams->IsPktFromLoopBack )
    {
        /* If the packet is from VLAN or LoopBack, then channel privilege should not be
                    considered for session privilege calculation */
        pSessInfo->Privilege = 0x4; //set to admin for loopback and VLAN always
        //UTIL_MIN (pSessInfo->Privilege, pChUserInfo->AccessLimit);
    }
    else
    {
        /* if requested privilege is greater than privilege level for channel or user
                set the minimum of Channel or user privilege*/
        pSessInfo->Privilege =
        UTIL_MIN (pSessInfo->Privilege, UTIL_MIN (pChannelInfo->MaxPrivilege, UTIL_MIN(pSessInfo->MaxPrivilege, pChUserInfo->AccessLimit)));
    }
#else
    /* if requested privilege is greater than privilege level for channel or user
            set the minimum of Channel or user privilege*/
    pSessInfo->Privilege =
    UTIL_MIN (pSessInfo->Privilege, UTIL_MIN (pChannelInfo->MaxPrivilege, UTIL_MIN(pSessInfo->MaxPrivilege, pChUserInfo->AccessLimit)));
#endif

    pSessInfo->AuthType     = RMCP_PLUS_FORMAT;
    pSessInfo->Activated    = TRUE;
    /* Set the Max Privilege allowed for the Session*/
    pSessInfo->MaxPrivilege = pSessInfo->Privilege;
    BMC_GET_SHARED_MEM (BMCInst)->SessionHandle += 1;
    pSessInfo->SessionHandle = BMC_GET_SHARED_MEM (BMCInst)->SessionHandle;

    /*Load Response */
    Res->StatusCode      = SC_NO_ERROR;
    Res->RemConSessionID = pSessInfo->RemConSessionID;
    pSessInfo->EventFlag = 1;
    pBMCInfo->LANConfig.DeleteThisLANSessionID = 0;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
    return (sizeof (RAKPMsg4Res_T) + ResIntigrityKeyLen);
}


#endif /*#if IPMI20_SUPPORT == 1*/
