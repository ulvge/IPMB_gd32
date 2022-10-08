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
 * AppDevice.c
 * AppDevice Commands Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "Debug.h"
#include "IPMIDefs.h"
#include "PMConfig.h"
#include "SharedMem.h"
#include "IPMI_Main.h"
#include "Support.h"
#include "IPMI_AppDevice+.h"
#include "AppDevice+.h"
#include "RMCP.h"
#include "RMCP+.h"
#include "NVRAccess.h"
#include "Platform.h"
#include "SOL.h"
#include "KCS.h"
#include "nwcfg.h"
#include "Ethaddr.h"
#include "IPMIConf.h"
#include "safesystem.h"
#include "SSIFIfc.h"

#ifdef  APP_DEVICE

/*** Local Macros definitions ***/
#define     SET_KEYS_OPER_READ      00
#define     SET_KEYS_OPER_SET       01
#define     SET_KEYS_OPER_LOCK      02
#define     SET_KEYS_OPER_TEST      03

#define     KEY_ID_KR               0
#define     KEY_ID_KG               1
#define IPMI_PYLD_TYPE 0

#define     ENCRYPTION_ONLY             0x80
#define     AUTHENTICATION_ONLY         0x40
#define     ENCRYPTION_AUTHENTICATION   0xC0
#define     AUTHTYPE_RMCP_PLUS_FORMAT   0x06

#define	  UNSUPPORTED_PAYLOAD_TYPE 0x20

/* Reserved bit macro definitions */
#define RESERVED_BITS_SETCHSECKEY 0xF0 //(BIT7 | BIT6 | BIT5 |BIT4)
#define RESERVED_BITS_GETSYSIFCCAPS 0XF0

/*** Module variables ***/
#if GET_SYS_IFC_CAPS != UNIMPLEMENTED

static const INT8U m_SSIfcData[]  =
        {0x80 | 0x08 | 0x00, MAX_SSIF_REQ_PKT_SIZE, MAX_SSIF_RES_PKT_SIZE - 3}; /**< SSIF Interface capabilities data */   /*MAX_SSIF_RES_PKT_SIZE - 3 is to drop Length, RsNetFnLun, Cmd*/

static const INT8U m_KCSIfcData[] = {0x00, 0xFF};/**< KCS Interface capabilities data */

#endif /* GET_SYS_IFC_CAPS */

static const INT8U  m_SupportedAlgrothims [] = { 0x00, 0x01, 0x02, 0x03, 0x40, 0x41, 0x42, 0x43, 0x44, 0x80, 0x81 };

/*---------------------------------------
 * ActivatePayload
 *---------------------------------------*/
int
ActivatePayload (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    ActivatePayloadReq_T* Req = (ActivatePayloadReq_T*)pReq;
    ActivatePayloadRes_T* Res = (ActivatePayloadRes_T*)pRes;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    int                 i;
    INT16U              ActvtInst = 0;
    INT8U		 EthIndex;
    MsgPkt_T            MsgPkt;
    struct stat fp;
    char solsessionfile[MAXFILESIZE] = {0};
    INT8U   Index;
       ChannelInfo_T*        pChannelInfo=NULL;
       ChannelUserInfo_T*    pChUserInfo=NULL;
       SessionInfo_T*        pSessInfo=NULL;
    INT32U CurSesID,curchannel;

    if((Req->PayldType & (BIT7 | BIT6)) || (Req->PayldInst & (BIT7 | BIT6 | BIT5 | BIT4)) 
        || (Req->PayldInst == 0) || (Req->PayldType == IPMI_PYLD_TYPE))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    /* Check Payload Instance */
    if(Req->PayldInst > MAX_PAYLD_INST)
    {
        IPMI_DBG_PRINT ("AppDevice+.c : ActivatePayload - Payload instance exceeded the limit\n");
        Res->CompletionCode = CC_INST_EXCEEDED;
        return sizeof(*pRes);
    }

    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);
    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    Res->CompletionCode = CC_INV_DATA_FIELD;

    OS_THREAD_TLS_GET(g_tls.CurSessionID,CurSesID);
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex, WAIT_INFINITE);
    /* Activate Payload */
    pSessInfo = getSessionInfo (SESSION_ID_INFO, &CurSesID, BMCInst);

    if (NULL == pSessInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        *pRes = CC_UNSPECIFIED_ERR;
        return	sizeof (*pRes); 
    }

    if(pSessInfo->AuthType != AUTHTYPE_RMCP_PLUS_FORMAT)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }


    for (i = 0; i < MAX_PYLDS_SUPPORT; i++)
    {
        if ((0 != Req->PayldType) &&
            (Req->PayldType == pBMCInfo->RMCPPlus[EthIndex].PayloadInfo [i].Type))
        {
            break;
        }
    }

    /* Check if PayloadType exist */
    if (i >= MAX_PYLDS_SUPPORT)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        /* Check Payload Type No */
        Res->CompletionCode = CC_PAYLOAD_NOT_ENABLED;
        IPMI_DBG_PRINT_1 ("AppDevice+.c : ActivatePayload - Payload not enabled %x\n", Req->PayldType);
        return sizeof (*pRes);
    }

    /* Activate Payload */
    pSessInfo = getSessionInfo (SESSION_ID_INFO,&CurSesID, BMCInst);

    if (NULL == pSessInfo)
    {
        return  sizeof (*pRes); 
    }


    pChannelInfo = getChannelInfo (curchannel & 0xF, BMCInst);

    if (NULL == pChannelInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    pChUserInfo = getChUserIdInfo ((pSessInfo->UserId & 0x3F),
                                    &Index, 
                                    pChannelInfo->ChannelUserInfo, BMCInst);

    if (NULL == pChUserInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        return sizeof (*pRes); 
    }

    ActvtInst = getPayloadActiveInst (Req->PayldType, BMCInst);

    if (0 != (ActvtInst & (1 << (Req->PayldInst - 1))))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        IPMI_DBG_PRINT ("AppDevice+.c : ActivatePayload - Payload already active\n");
        Res->CompletionCode = CC_INST_ALREADY_ACTIVE;
        return  sizeof (*pRes);
    }

    /* Encryption can not be enabled, if encryption was not negotiated at the
       time of session activation */
    if ( (0 == pSessInfo->ConfidentialityAlgorithm) && (Req->AuxData[0] & ENCRYPTION_ONLY) )
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        Res->CompletionCode = CC_CANNOT_ACTIVATE_WITH_ENCR;
        return  sizeof (*pRes);
    }

    /* Authentication(Integrity) can not be enabled, if Integrity was not negotiated at the
       time of session activation */
    if ( (0 == pSessInfo->IntegrityAlgorithm) && (Req->AuxData[0] & AUTHENTICATION_ONLY) )
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        Res->CompletionCode = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    if (Req->PayldType == PAYLOAD_SOL)
    {
        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
        /*get information abt this channel*/
        pChannelInfo = getChannelInfo ((curchannel & 0xF), BMCInst);
        if(NULL == pChannelInfo)
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            Res->CompletionCode = CC_INV_DATA_FIELD;
            return  sizeof (*pRes);
        }
        /*get information abt this session*/
        pSessInfo = getSessionInfo (SESSION_ID_INFO,&CurSesID, BMCInst);
        if ((pSessInfo != NULL) && (pChannelInfo != NULL))
        {
            /*get information abt this user*/
            pChUserInfo = getChUserIdInfo (pSessInfo->UserId, &Index, pChannelInfo->ChannelUserInfo, BMCInst);

            if (pChUserInfo != NULL)
            {
                pChUserInfo->ActivatingSOL = TRUE;
            }
        }
        else
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            *pRes = CC_INV_DATA_FIELD;
            return sizeof (INT8U);
        }

        if((Req->AuxData[0] & (BIT4 | BIT0)) || ((Req->AuxData[0] & BIT3) && (Req->AuxData[0] & BIT2)))
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            Res->CompletionCode = CC_INV_DATA_FIELD;
            return  sizeof (*pRes);
        }

        if (0 == pBMCInfo->SOLCfg[EthIndex].SOLEnable)
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            Res->CompletionCode = CC_PAYLOAD_NOT_ENABLED;
            return  sizeof (*pRes); 
        }

        if (0 == (pChUserInfo->PayloadEnables [0]& 0x2))
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            Res->CompletionCode = CC_PAYLOAD_NOT_ENABLED;
            return  sizeof (*pRes);
        }
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

        /* If encryption is used, authentication must also be used */
        if ((Req->AuxData[0] & ENCRYPTION_ONLY) && !(Req->AuxData[0] & AUTHENTICATION_ONLY))
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            Res->CompletionCode = CC_INV_DATA_FIELD;
            return  sizeof (*pRes);
        }
        
        /* checked if forced encryption is enabled */
        if (ENCRYPTION_ONLY ==  (pBMCInfo->SOLCfg[EthIndex].SOLAuth & ENCRYPTION_ONLY ) )
        {
            /* Check for authentication and encryption activations */
            if (ENCRYPTION_ONLY != (Req->AuxData[0] & ENCRYPTION_ONLY))
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                Res->CompletionCode = CC_CANNOT_ACTIVATE_WITHOUT_ENCR;                  
                return  sizeof (*pRes); 
            }
        }
        
        /* checked if forced authentication is enabled */
        if (AUTHENTICATION_ONLY ==  (pBMCInfo->SOLCfg[EthIndex].SOLAuth & AUTHENTICATION_ONLY ) )
        {
            /* Check for authentication activations */
            if (AUTHENTICATION_ONLY != (Req->AuxData[0] & AUTHENTICATION_ONLY))
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                Res->CompletionCode = CC_CANNOT_ACTIVATE_WITHOUT_ENCR;                  
                return  sizeof (*pRes); 
            }
        }

        /* To check the SOL Priveilege */ 
        if(pSessInfo->Privilege  < (pBMCInfo->SOLCfg[EthIndex].SOLAuth & 0xF))
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            Res->CompletionCode = CC_INSUFFIENT_PRIVILEGE;
            return  sizeof (*pRes);
        }

        /* PDK to Check SOL connected from CLI*/
        if(g_PDKHandle[PDK_ISSOLCONNECTED] != NULL)
        {
            ((INT8U(*)(int)) g_PDKHandle[PDK_ISSOLCONNECTED])(BMCInst);
        }

        /*Check the sol session file. If its exist CIM might establish the SOL session using solssh, so return the completion code as 0x80.
         *Otherwise create the sol session file to prevent the CIM to establish the SOL session.
        */
        sprintf(solsessionfile,"%s%d",SOL_SESSION_FILE,BMCInst);

        if(stat(solsessionfile,&fp) != 0)
        {
            memset(solsessionfile,0,MAXFILESIZE);
            sprintf(solsessionfile,"touch %s%d",SOL_SESSION_FILE,BMCInst);
            safe_system(solsessionfile);
        }
        else
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
           *pRes = CC_INST_ALREADY_ACTIVE;
            return sizeof(INT8U);
        }
		
        pSessInfo->Activated = TRUE;

        if(pBMCInfo->IpmiConfig.SOLIfcSupport == 1)
        {
            BMC_GET_SHARED_MEM (BMCInst)->SOLSessID = pSessInfo->SessionID;
            pSessInfo->TimeOutValue = pBMCInfo->IpmiConfig.SOLSessionTimeOut;
            
            /* If SOL timeout value equals 0, then SOL will never timeout.. */
            if( pBMCInfo->IpmiConfig.SOLSessionTimeOut == 0)
                pSessInfo->TimeOutValue = pBMCInfo->IpmiConfig.SessionTimeOut;	        

            MsgPkt.Param    = START_SOL;
            MsgPkt.Size     = 0;
            if( 0 != PostMsg (&MsgPkt, SOL_IFC_Q,BMCInst))    
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                IPMI_WARNING ("AppDevice+.c : Error posting message to SOLIfc_Q\n");
                Res->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(*pRes);
            }

            MsgPkt.Param    = ACTIVATE_SOL;
            MsgPkt.Size     = 0;
            if( 0 != PostMsg (&MsgPkt, SOL_IFC_Q,BMCInst))    
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                IPMI_WARNING ("AppDevice+.c : Error posting message to SOLIfc_Q\n");
                Res->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(*pRes);
            }
        }
    }

    ActvtInst |= 1 << (Req->PayldInst - 1);

    pSessInfo->SessPyldInfo [Req->PayldType].ActivatedInst  = ActvtInst;
    pSessInfo->SessPyldInfo [Req->PayldType].Type           = Req->PayldType;
    _fmemcpy (pSessInfo->SessPyldInfo [Req->PayldType].AuxConfig, Req->AuxData, 4);

    /* Load Response */
    Res->CompletionCode     = CC_NORMAL;
    _fmemset (Res->AuxData, 0, 4);
    Res->InboundPayldSize   = (Req->PayldType == PAYLOAD_SOL)? htoipmi_u16 (MAX_SOL_IN_PAYLD_SIZE): htoipmi_u16 (MAX_IN_PAYLD_SIZE);
    Res->OutboundPayldSize  = (Req->PayldType == PAYLOAD_SOL)? htoipmi_u16 (MAX_SOL_OUT_PAYLD_SIZE): htoipmi_u16 (MAX_OUT_PAYLD_SIZE);
    Res->UDPPort            = htoipmi_u16(623);
    Res->VLANNo             = 0xFFFF;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);

    return sizeof (ActivatePayloadRes_T);
}


/*---------------------------------------
 * DeactivatePayload
 *---------------------------------------*/
int
DeactivatePayload (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    DeactivatePayloadReq_T* Req = (DeactivatePayloadReq_T*)pReq;
    int                 i;
    INT16U              ActvtInst = 0;
    INT32U  SolSessionID;
    MsgPkt_T            MsgPkt;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U		 EthIndex;
       ChannelInfo_T*        pChannelInfo=NULL;
       ChannelUserInfo_T*    pChUserInfo=NULL;
       SessionInfo_T*        pSessInfo=NULL;
    INT8U    Index;
    char solsessionfile[MAXFILESIZE] = {0};
    struct stat fp;
    INT32U CurSesID,curchannel;

    if((Req->PayldType & (BIT7 | BIT6)) || (Req->PayldInst & (BIT7 | BIT6 | BIT5 | BIT4)) 
        || (Req->PayldInst == 0) || (Req->PayldType == IPMI_PYLD_TYPE))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);
    if(0xff == EthIndex)
    {
         *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    /* Check Payload Type No */
    *pRes = CC_INV_DATA_FIELD;
    for (i = 0; i < MAX_PYLDS_SUPPORT; i++)
    {
        if ((0 != Req->PayldType) &&
            (Req->PayldType == pBMCInfo->RMCPPlus[EthIndex].PayloadInfo [i].Type))
        {
            break;
        }
    }

    /* Check if PayloadType exist */
    if (MAX_PYLDS_SUPPORT == i)
    {
        return sizeof (*pRes);
    }

    /* Check Payload Instance */
    if (Req->PayldInst > MAX_PAYLD_INST)
    {
        return  sizeof (*pRes);
    }
    if ((0 == pBMCInfo->SOLCfg[EthIndex].SOLEnable) && (1 == Req->PayldType))
    {
        *pRes = CC_PAYLOAD_NOT_ENABLED;
        return  sizeof (*pRes); 
    }


    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex, WAIT_INFINITE);
    SolSessionID =BMC_GET_SHARED_MEM (BMCInst)->SOLSessID ;
    /* Deactivate Payload */
    pSessInfo = getSessionInfo (SESSION_ID_INFO,&SolSessionID, BMCInst);
    if (NULL == pSessInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        IPMI_DBG_PRINT ("AppDevice+.c : Deactivate Payload - Invalid Session Id\n");
        *pRes = CC_INST_ALREADY_INACTIVE;
        return  sizeof (*pRes);
    }

    ActvtInst = pSessInfo->SessPyldInfo [Req->PayldType].ActivatedInst;
    if (0 == (ActvtInst & (1 << (Req->PayldInst - 1))))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        IPMI_DBG_PRINT ("AppDevice+.c : Deactivate Payload - Payload already deactivated\n");
        *pRes = CC_INST_ALREADY_INACTIVE;
        return  sizeof (*pRes);
    }
    
    ActvtInst &= ~(1 << (Req->PayldInst - 1));
    pSessInfo->SessPyldInfo [Req->PayldType].ActivatedInst = ActvtInst;
#if 0
    _fmemcpy (pSessInfo->SessPyldInfo [Req->PayldType].AuxConfig, Req->AuxData, 4);
    if(pBMCInfo->IpmiConfig.SOLIfcSupport == 1)
    {
        if (Req->PayldType == PAYLOAD_SOL)
        {
            pSessInfo->SessPyldInfo [Req->PayldType].Type = 0;
            pSessInfo->TimeOutValue = pBMCInfo->IpmiConfig.SessionTimeOut;
        }
    }
#endif    
    if(pBMCInfo->IpmiConfig.SOLIfcSupport == 1)
    {
        if (Req->PayldType == PAYLOAD_SOL)
        {
            /*Remove the solsession file while deactivating the SOL,so that CIM can establish the SOL session using solssh app*/
            sprintf(solsessionfile,"%s%d",SOL_SESSION_FILE,BMCInst);

            if(stat(solsessionfile,&fp) == 0)
            {
                unlink(solsessionfile);
            }

            /*get information abt this channel*/
            pChannelInfo = getChannelInfo ((curchannel & 0xF), BMCInst);
            if(NULL == pChannelInfo)
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                *pRes = CC_INV_DATA_FIELD;
                return	sizeof (*pRes);
            }

            OS_THREAD_TLS_GET(g_tls.CurSessionID,CurSesID);
            OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
            /*get information abt this session*/
            pSessInfo = getSessionInfo (SESSION_ID_INFO,&CurSesID, BMCInst);
            if ((pSessInfo != NULL) && (pChannelInfo != NULL))
            {
                /*get information abt this user*/
                pChUserInfo = getChUserIdInfo (pSessInfo->UserId, &Index, pChannelInfo->ChannelUserInfo, BMCInst);
                if (pChUserInfo != NULL)
                {
                    pChUserInfo->ActivatingSOL = FALSE;
                }
            }
            else
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                *pRes = CC_UNSPECIFIED_ERR;
                return	sizeof (*pRes);
            }
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            
            /* Deactivate SOL */
            MsgPkt.Param = DEACTIVATE_SOL;
            MsgPkt.Size = 0;
            if( 0 != PostMsg (&MsgPkt, SOL_IFC_Q,BMCInst))    
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                IPMI_WARNING ("AppDevice+.c : Error posting message to SOLIfc_Q\n");
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(*pRes);
            }

            /* PDK to Check SOL connected from CLI*/
            if(g_PDKHandle[PDK_ISSOLCONNECTED] != NULL)
            {
                ((INT8U(*)(int)) g_PDKHandle[PDK_ISSOLCONNECTED])(BMCInst);
            }
        }
        /*Required delay of 0.5 sec, when the sol session consecutively deactivate 
             and activate. If there is no delay means, sol send the Deactivated packet 
             to both the session unknowingly*/
        usleep (500000);   
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
    /* Load Response */
    *pRes   = CC_NORMAL;
    return sizeof (*pRes);
}


/*---------------------------------------
 * GetPayldActStatus
 *---------------------------------------*/
int
GetPayldActStatus (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetPayldActStatRes_T* Res = (GetPayldActStatRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int                           i;

    /* Check Payload Type No */
    Res->CompletionCode = CC_INV_DATA_FIELD;

    for (i = 0; i < MAX_PYLDS_SUPPORT; i++)
    {
        /* We dont know Which LAN channel we  need .so Returning default 0 . */
        if ((0 != pReq[0]) &&
            (pReq[0] == pBMCInfo->RMCPPlus[0].PayloadInfo [i].Type))
        {
            break;
        }
    }

    /* Check if PayloadType exist */
    if (MAX_PYLDS_SUPPORT == i && (IPMI_PYLD_TYPE != pReq[0]))
    {
        return  sizeof (*pRes);
    }

    Res->CompletionCode = CC_NORMAL;
    Res->InstCap        = MAX_PAYLD_INST;

    if(IPMI_PYLD_TYPE == pReq[0])
    {
        Res->ActivatedInst = 1;
    }
    else
    {
        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex, WAIT_INFINITE);
        Res->ActivatedInst = htoipmi_u16 (getPayloadActiveInst (*pReq, BMCInst));
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
    }
    return sizeof (GetPayldActStatRes_T);
}


/*---------------------------------------
 * GetPayldInstInfo
 *---------------------------------------*/
int
GetPayldInstInfo (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetPayldInstInfoReq_T* Req = (GetPayldInstInfoReq_T*)pReq;
    GetPayldInstInfoRes_T* Res = (GetPayldInstInfoRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int                            i;

    /* Check Payload Type No & Totally 16 payload instances can be supported */
    if ((Req->PayldInst == 0) || (Req->PayldType == IPMI_PYLD_TYPE)
        || (Req->PayldType >= UNSUPPORTED_PAYLOAD_TYPE) || (Req->PayldInst > MAX_PAYLD_INST)) 
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    for (i = 0; i < MAX_PYLDS_SUPPORT; i++)
    {
        // if ((0 != Req->PayldType) && 
        /* We dont know Which LAN channel we  need .so Returning default 0 . */
        if (Req->PayldType == pBMCInfo->RMCPPlus[0].PayloadInfo [i].Type)
        {
            break;
        }
    }

    /* Check if PayloadType exist */
    if (MAX_PYLDS_SUPPORT == i) { return sizeof (*pRes); }

    /* Load Response */
    _fmemset (Res, 0, sizeof (GetPayldInstInfoRes_T));
    Res->CompletionCode = CC_NORMAL;
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex, WAIT_INFINITE);
    Res->SessionID      = getPayloadInstInfo (Req->PayldType, Req->PayldInst, BMCInst);
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);

    if (Req->PayldType == PAYLOAD_SOL)
    {
        Res->SpecificInfo[0] = SYS_SERIAL_PORT_NUM;
    }

    return sizeof (GetPayldInstInfoRes_T);
}


/*---------------------------------------
 * SetUsrPayloadAccess
 *---------------------------------------*/
int
SetUsrPayloadAccess (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    SetUsrPayldAccReq_T* Req = (SetUsrPayldAccReq_T*)pReq;
      ChannelInfo_T*       pChannelInfo;
      ChannelUserInfo_T*   pChUserInfo;
      ChannelUserInfo_T*   pNVRChUserInfo;
    INT8U                        Index;
    int                          i,j;
    INT8U                        Operation;
    INT8U                        ChannelNum;
    INT8U	   EthIndex,curchannel;
    BMCInfo_t *pBMCInfo= &g_BMCInfo[BMCInst];
//    char ChFilename[MAX_CHFILE_NAME];

    /* Check Channel No */
    *pRes = CC_INV_DATA_FIELD;

    /* Check for Reserved Bits */
    if((0 != (Req->ChannelNum & 0xf0)) ||
        ((((Req->UserId & 0xc0) >> 6) != 0x00) && (((Req->UserId & 0xc0) >> 6) != 0x01)))
            return sizeof (*pRes);

    /*Check for Payload Reserved Bits */
    if((0 != (Req->PayloadEnables[0] & 0xfd)) ||
        ( 0 != (Req->PayloadEnables[1] & 0xff)) ||
        ( 0 != (Req->PayloadEnables[3] & 0xff)))
             return sizeof (*pRes);		

    ChannelNum = *pReq & 0x0F;
    if (0x0E == ChannelNum)
    {
        OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
        EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);
    }else
    {
        EthIndex= GetEthIndex(ChannelNum, BMCInst);
    }
    
   if(0xff == EthIndex)
   {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    /* Check UserID */
    if (0 == (Req->UserId & 0x3F))
    {
        return  sizeof (*pRes);
    }
    pChannelInfo = getChannelInfo (Req->ChannelNum & 0x0F, BMCInst);

    if (NULL == pChannelInfo)
    {
       *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pChUserInfo = getChUserIdInfo ((Req->UserId & 0x3F),
                                    &Index,
                                    pChannelInfo->ChannelUserInfo, BMCInst);

    if (NULL == pChUserInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        return sizeof (*pRes);
    }

    /*Check if the setting is for OEM/ Standard */
    Operation = Req->UserId >> 6;

    pNVRChUserInfo = GetNVRChUserConfigs(pChannelInfo,BMCInst);
     
    /* Set Access settings */
    for (i = 0; i < MAX_PLD_ENABLES; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if ((i == 0) && (j == 0)) { continue; } //For IPMI payload
            if (0 == (Req->PayloadEnables [i] & (1 << j)))
            {
                continue;
            }
            switch (Operation)
            {
                case 0: /* Enable */
                    pChUserInfo->PayloadEnables [i] |= (1 << j); 
                    pNVRChUserInfo[Index].PayloadEnables [i] |= (1 << j);
                    break;

                case 1: /* Disable */
                    pChUserInfo->PayloadEnables [i] &= ~(1 << j); 
                    pNVRChUserInfo[Index].PayloadEnables [i] &= ~(1 << j);
                    break;
            }
        }
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
    FlushChConfigs((INT8U*)GetNVRChConfigs(pChannelInfo,BMCInst),Req->ChannelNum,BMCInst);

    /* Load Response */
    *pRes = CC_NORMAL;

    return sizeof (*pRes);
}


/*---------------------------------------
 * GetUsrPayloadAccess
 *---------------------------------------*/
int
GetUsrPayloadAccess (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetUsrPayldAccReq_T* Req = (GetUsrPayldAccReq_T*)pReq;
    GetUsrPayldAccRes_T* Res = (GetUsrPayldAccRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
      ChannelInfo_T*       pChannelInfo;
      ChannelUserInfo_T*   pChUserInfo;
    INT8U                Index;
    INT8U                ChannelNum,curchannel;
    INT8U	   EthIndex;


    /* Check Channel No */
    Res->CompletionCode = CC_INV_DATA_FIELD;

    /* Check for reserved Bits */
    if((0 != (Req->ChannelNum & 0xf0)) || (0 != (Req->UserId & 0xc0)))
        return  sizeof (*pRes);

    ChannelNum = *pReq & 0x0F;
    if (0x0E == ChannelNum)
    {
        OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
        EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);
        ChannelNum = curchannel & 0xF;
    }else
    {
        EthIndex= GetEthIndex(ChannelNum, BMCInst);
    }

    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    /* Check UserID */
    if (0 == (Req->UserId & 0x3F))
    {
        IPMI_WARNING  ("Invalid User\n");
        return  sizeof (*pRes);
    }

    pChannelInfo = getChannelInfo (ChannelNum, BMCInst);

    if (NULL == pChannelInfo)
    {
        IPMI_WARNING  ("Unable to get Ch infor %d\n", ChannelNum);
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pChUserInfo  = getChUserIdInfo ((Req->UserId & 0x3F),
                                    &Index,
                                    pChannelInfo->ChannelUserInfo, BMCInst);

    if (NULL == pChUserInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        IPMI_DBG_PRINT_1 ("Unable to get Ch user infor %d \n", Req->UserId);
        return  sizeof (*pRes);
    }

    /* Get Access settings */
    _fmemcpy (Res->PayloadEnables, pChUserInfo->PayloadEnables, MAX_PLD_ENABLES);

    Res->CompletionCode = CC_NORMAL;

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    return sizeof (GetUsrPayldAccRes_T);
}


/*---------------------------------------
 * GetChPayloadSupport
 *---------------------------------------*/
int
GetChPayloadSupport (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetChPayldSupRes_T* Res = (GetChPayldSupRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U               ChannelNum,curchannel;
    INT8U   EthIndex;

    Res->CompletionCode = CC_INV_DATA_FIELD;

    if( *pReq & (BIT7 | BIT6 | BIT5 | BIT4))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    ChannelNum = *pReq;
    if (0x0E == ChannelNum)
    {
        OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
        EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);
    }else
    {
        EthIndex= GetEthIndex(ChannelNum, BMCInst);
    }

    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    LOCK_BMC_SHARED_MEM(BMCInst);
    /* Load Payload Supported  */
    _fmemcpy (&Res->StdPldtype1, (INT8U*)&pBMCInfo->RMCPPlus[EthIndex].PayloadSupport,
        sizeof (PayloadSupport_T));
    UNLOCK_BMC_SHARED_MEM(BMCInst);

    /* Load Response */
    Res->CompletionCode = CC_NORMAL;
    Res->Rsvd           = 0x0000;
    return sizeof (GetChPayldSupRes_T);
}


/*---------------------------------------
 * GetChPayloadVersion
 *---------------------------------------*/
int
GetChPayloadVersion (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetChPayldVerReq_T* Req = (GetChPayldVerReq_T*)pReq;
    GetChPayldVerRes_T* Res = (GetChPayldVerRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int                 i;
    INT8U               ChannelNum,curchannel;
    INT8U	   EthIndex;

    /* Check Channel No */
    Res->CompletionCode = CC_INV_DATA_FIELD;

    if( Req->ChannelNum & (BIT7 | BIT6 | BIT5 | BIT4))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    ChannelNum = *pReq & 0x0F;
    if (0x0E == ChannelNum)
    {
        OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
        EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);
    }else
    {
        EthIndex= GetEthIndex(ChannelNum, BMCInst);
    }

    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    /* Check Payload Type No */
    for (i = 0; i < MAX_PYLDS_SUPPORT; i++)
    {
        if (Req->PayloadNum == pBMCInfo->RMCPPlus[EthIndex].PayloadInfo [i].Type)
        {
            break;
        }
    }

    /* Check if PayloadType exist */
    if (MAX_PYLDS_SUPPORT == i)
    {
        IPMI_DBG_PRINT_1("AppDevice+.c : GetChPayloadVersion - Payload type (%x) not available on given channel. \n",Req->PayloadNum);
        Res->CompletionCode = CC_PAYLOAD_NOT_AVAILABLE;
        return sizeof (*pRes);
    }

    /* Load Payload Version */
    Res->CompletionCode = CC_NORMAL;
    Res->FormatVer      = pBMCInfo->RMCPPlus[EthIndex].PayloadInfo [i].Version;

    return sizeof (GetChPayldVerRes_T);
}


/*---------------------------------------
 * GetChOemPayloadInfo
 *---------------------------------------*/
int
GetChOemPayloadInfo (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetChOemPayldInfoReq_T* Req = (GetChOemPayldInfoReq_T*)pReq;
    GetChOemPayldInfoRes_T* Res = (GetChOemPayldInfoRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int                     i;
    INT8U               ChannelNum,curchannel;
    INT8U    EthIndex;

    /* Check Channel No */
    Res->CompletionCode = CC_INV_DATA_FIELD;

    if( Req->ChannelNum & (BIT7 | BIT6 | BIT5 | BIT4))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    ChannelNum = *pReq & 0x0F;
    if (0x0E == ChannelNum)
    {
        OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
        EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);
    }else
    {
        EthIndex= GetEthIndex(ChannelNum, BMCInst);
    }

    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    LOCK_BMC_SHARED_MEM(BMCInst);
    /* Check Payload Type No */
    for (i = 0; i < MAX_PYLDS_SUPPORT; i++)
    {
        if ((Req->PayloadNum == pBMCInfo->RMCPPlus[EthIndex].PayloadInfo [i].Type) &&
            (0 == _fmemcmp (&Req->OemIANA, pBMCInfo->RMCPPlus[EthIndex].PayloadInfo [i].OemPldIANA, sizeof (Req->OemIANA))) &&
            (0 == _fmemcmp (&Req->OemPyldId, pBMCInfo->RMCPPlus[EthIndex].PayloadInfo [i].OemPldID, sizeof (Req->OemPyldId))))
        {
            break;
        }
    }
    UNLOCK_BMC_SHARED_MEM(BMCInst);

    /* Check if PayloadType exist */
    if (MAX_PYLDS_SUPPORT == i)
    {
        *pRes = CC_PARAM_NOT_SUPPORTED;
        return sizeof (*pRes);
    }

    /* Load Payload Version */
    Res->CompletionCode = CC_NORMAL;
    LOCK_BMC_SHARED_MEM(BMCInst);
    _fmemcpy (&Res->PayloadNum, &pBMCInfo->RMCPPlus[EthIndex].PayloadInfo [i].Type, sizeof (PayloadInfo_T));
    UNLOCK_BMC_SHARED_MEM(BMCInst);

    return sizeof (GetChOemPayldInfoRes_T);
}


/*---------------------------------------
 * GetChCipherSuites
 *---------------------------------------*/
int
GetChCipherSuites (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetChCipherSuitesReq_T* Req = (GetChCipherSuitesReq_T*)pReq;
    GetChCipherSuitesRes_T* Res = (GetChCipherSuitesRes_T*)pRes;
    INT8U                   Channel, Ix, ResLen,curchannel;
    INT8U	  EthIndex;

    /* Check if the Channel No is valid */
    Channel = Req->ChannelNum;
    Res->CompletionCode = CC_INV_DATA_FIELD ;

    if((Req->ChannelNum & (BIT7 | BIT6 | BIT5 |BIT4)) || (Req->PayloadType & (BIT7 | BIT6)) || 
        (Req->ListIndex & BIT6))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    if (0x0E == Channel)
    {
        OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
        EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);   
    }else
    {
        EthIndex= GetEthIndex(Channel, BMCInst);
    }

    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    /* Check if the Payload is valid */
    if (Req->PayloadType >= MAX_PYLDS_SUPPORT)
    {
        return sizeof (*pRes);
    }

    if (Req->ListIndex & 0x80)
    {
        /* Get ListIndex */
        Ix = Req->ListIndex & 0x3F;

        /* Load Cipher Suites Response */
        Res->CompletionCode = CC_NORMAL;
        Res->ChannelNum     = GetLANChannel(EthIndex, BMCInst);

      if (Ix > (sizeof (g_CipherRec)/16))
        {
            return 2;
        }

        ResLen = sizeof (g_CipherRec) - (Ix * 16);
        ResLen = (ResLen >= 16) ? 16 : ResLen;
        _fmemcpy (Res + 1, &g_CipherRec [Ix * 16], ResLen);

    }
    else
    {
        /* Get ListIndex */
        Ix = Req->ListIndex & 0x3F;

        /* Load Cipher Suites Response */
        Res->CompletionCode = CC_NORMAL;
        Res->ChannelNum     = GetLANChannel(EthIndex, BMCInst);

        if (Ix <= 1)
        {
            ResLen = sizeof (m_SupportedAlgrothims);
            _fmemcpy (Res + 1, m_SupportedAlgrothims, ResLen);
        }
        else
        {
            ResLen = 0x00;
        }
    }

    return sizeof (GetChCipherSuitesRes_T) + ResLen;
}


/*---------------------------------------
 * SetChSecurityKeys
 *---------------------------------------*/
int
SetChSecurityKeys (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    SetChSecurityKeysReq_T* Req = (SetChSecurityKeysReq_T*)pReq;
    SetChSecurityKeysRes_T* Res = (SetChSecurityKeysRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
      INT8U*                  pKeys;
    int                     Keysize, ResLen;
    INT8U 	 EthIndex,curchannel;

	if(Req->ChannelNum & RESERVED_BITS_SETCHSECKEY)
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}
	
    /* Check Channel No */
    Res->CompletionCode = CC_INV_DATA_FIELD;
    if (0x0E == Req->ChannelNum)
    {
        OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
        EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);
    }else
    {
        EthIndex= GetEthIndex(Req->ChannelNum, BMCInst);
    }

    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    /* Check the Reserved Bit in  Request */
    if(Req->Operation > 2)
    {
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    /* Check Key Id */
    if (KEY_ID_KR == Req->KeyID)
    {
        pKeys = (INT8U*)pBMCInfo->RMCPPlus[EthIndex].PseudoGenKey;
    }
    else if (KEY_ID_KG == Req->KeyID)
    {
        pKeys = (INT8U*)pBMCInfo->RMCPPlus[EthIndex].KGHashKey;
    }
    else
    {
        /* Invalid ID */
        return  sizeof (*pRes);
    }

    Res->CompletionCode = CC_NORMAL;
    if (TRUE == pBMCInfo->RMCPPlus[EthIndex].LockKey [Req->KeyID])
    {
        Res->LockStatus     = 0x01;
    }
    else
    {
        Res->LockStatus     = 0x02;
    }

    ResLen = sizeof (SetChSecurityKeysRes_T);

    /* Perform Operation */
    switch (Req->Operation)
    {
        case SET_KEYS_OPER_READ:
            if (TRUE == pBMCInfo->RMCPPlus[EthIndex].LockKey [Req->KeyID])
            {
                Res->CompletionCode = CC_KEYS_LOCKED;
                return  sizeof (*pRes);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (pRes + sizeof (SetChSecurityKeysRes_T), pKeys, HASH_KEY_LEN);
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            ResLen = sizeof (SetChSecurityKeysRes_T) + HASH_KEY_LEN;
            break;


            case SET_KEYS_OPER_SET :
                Keysize  = ReqLen - sizeof (SetChSecurityKeysReq_T);
                if (Keysize > HASH_KEY_LEN)
                {
                    Res->CompletionCode = CC_TOO_MANY_KEY_BYTES;
                    return  sizeof (*pRes);
                }
                else if (Keysize < HASH_KEY_LEN)
                {
                    Res->CompletionCode = CC_INSUF_KEY_BYTES;
                    return  sizeof (*pRes);
                }

                if (TRUE == pBMCInfo->RMCPPlus[EthIndex].LockKey [Req->KeyID])
                {
                    Res->CompletionCode = CC_KEYS_LOCKED;
                    return  sizeof (*pRes);
                }

                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemset (pKeys, 0, HASH_KEY_LEN);
                _fmemcpy (pKeys, pReq + sizeof (SetChSecurityKeysReq_T), Keysize);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                break;

            case SET_KEYS_OPER_LOCK:
                if (Req->KeyID == KEY_ID_KG)
                {
                    Res->CompletionCode = CC_INV_DATA_FIELD;
                    ResLen = 1;
                }
                else
                {
                    pBMCInfo->RMCPPlus[EthIndex].LockKey [Req->KeyID] = TRUE;
                    Res->LockStatus = 0x01;
                }
                break;

            default:
                return sizeof (*pRes);
    }

    return ResLen;
}


/*---------------------------------------
 * SusResPayldEncrypt
 *---------------------------------------*/
int
SusResPayldEncrypt (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    SusResPayldEncryptReq_T* Req = (SusResPayldEncryptReq_T*)pReq;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int                      i;
    _FAR_	 SessionInfo_T*  pSessInfo;
    INT32U	  SessionID;
    INT8U	EthIndex,curchannel;

    if((Req->PayldType & (BIT7 | BIT6)) || (Req->PayldInst & (BIT7 | BIT6 | BIT5 | BIT4)) 
        || (Req->PayldInst == 0))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    if(Req->Operation & (BIT7 | BIT6 | BIT5 | BIT4))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    EthIndex= GetEthIndex(curchannel & 0xF, BMCInst);

    if(0xff== EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    if (0 == Req->PayldType)
    {
        /* Encryption can not be Suspended/Resumed for IPMI payload type */
        /* For IPMI payload type, The "Suspend/Resume Payload Encryption" command controls
        whether asynchronous (unrequested) messages from the BMC are encrypted or not */
        *pRes = CC_OP_NOT_SUPPORTED;
        return sizeof (*pRes);
    }
    /* Check Payload Type No */
    for (i = 0; i < MAX_PYLDS_SUPPORT; i++)
    {
        if (Req->PayldType == pBMCInfo->RMCPPlus[EthIndex].PayloadInfo [i].Type)
        {
            break;
        }
    }

    /* Check if PayloadType exist */
    if (MAX_PYLDS_SUPPORT == i)
    {
        *pRes = CC_OP_NOT_SUPPORTED;
        return sizeof (*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex, WAIT_INFINITE);
    if (0 == (htoipmi_u16 (getPayloadActiveInst (*pReq, BMCInst) & (1 << (Req->PayldInst - 1)))))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        *pRes = CC_INST_NOT_ACTIVE;
        return  sizeof (*pRes);
    }

    /* Check Payload Instance */
    if (Req->PayldInst > MAX_PAYLD_INST)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
        *pRes = CC_OP_NOT_ALLOWED;
        return  sizeof (*pRes);
    }

    SessionID = BMC_GET_SHARED_MEM (BMCInst)->SOLSessID;

    pSessInfo = getSessionInfo (SESSION_ID_INFO, &SessionID, BMCInst);
    *pRes = CC_NORMAL;
    switch (Req->Operation)
    {
        case 0: 
            /* Encryption can not be suspended if Force SOL Payload Encryption */
            if (pBMCInfo->SOLCfg[EthIndex].SOLAuth & ENCRYPTION_ONLY || (!(pSessInfo->SessPyldInfo[Req->PayldType].AuxConfig [0] & ENCRYPTION_ONLY)))
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                *pRes = CC_OP_NOT_ALLOWED;
                return  sizeof (*pRes);
            }
            pSessInfo->SessPyldInfo [Req->PayldType].AuxConfig [0] &= (~ENCRYPTION_ONLY);
            break;

        case 1: 
            if(pSessInfo->SessPyldInfo[Req->PayldType].AuxConfig [0] & ENCRYPTION_ONLY)
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                *pRes = CC_OP_NOT_ALLOWED;
                return sizeof (*pRes);
            }

            if ((pSessInfo->IntegrityAlgorithm) && (pSessInfo->ConfidentialityAlgorithm))
            {
                pSessInfo->SessPyldInfo [Req->PayldType].AuxConfig [0] |= ENCRYPTION_ONLY;
            }
            else
            {
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
                *pRes = CC_ENC_NOT_AVAILABLE;
                return  sizeof (*pRes);
            }
            break;

        case 2: 
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
            *pRes = CC_OP_NOT_SUPPORTED;
            return sizeof (*pRes);
            break;

        default:
            *pRes = CC_INV_DATA_FIELD;
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);
    return sizeof (*pRes);
}


/*---------------------------------------
 * GetSysIfcCaps
 *---------------------------------------*/
int
GetSysIfcCaps (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetSysIfcCapsRes_T* pCommonRes = (GetSysIfcCapsRes_T*)pRes;
    INT8U               ResLen;

    pCommonRes->CompletionCode = CC_NORMAL;
    pCommonRes->Rsrvd = 0;

    ResLen = sizeof (GetSysIfcCapsRes_T);

    if(*pReq & RESERVED_BITS_GETSYSIFCCAPS)
    {
        pCommonRes->CompletionCode=CC_INV_DATA_FIELD;
        return(*pRes);
    }

    switch (pReq [0] & 0x0F)
    {
        case 0: // SSIF ifc
            ResLen += sizeof (m_SSIfcData);
            _fmemcpy (pCommonRes + 1, m_SSIfcData, sizeof (m_SSIfcData));
            break;

        case 1: // KCS ifc
            ResLen += sizeof (m_KCSIfcData);
            _fmemcpy (pCommonRes + 1, m_KCSIfcData, sizeof (m_KCSIfcData));
            break;

        default:
            pCommonRes->CompletionCode = CC_INV_DATA_FIELD;
            ResLen  = sizeof (INT8U);
    }
    return ResLen;
}

#endif  /* APP_DEVICE */
