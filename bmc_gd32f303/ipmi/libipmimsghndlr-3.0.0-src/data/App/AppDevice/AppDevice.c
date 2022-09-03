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
#define ENABLE_DEBUG_MACROS  0

#include "Types.h"
//#include "Debug.h"
#include "IPMI_Main.h"
//#include "SharedMem.h"
#include "Support.h"
#include "OSPort.h"
#include "Message.h"
#include "IPMIDefs.h"
#include "ipmi_common.h"
#include "MsgHndlr.h"
#include "IPMI_IPM.h"
#include "IPMI_AppDevice.h"
#include "AppDevice.h"
//#include "RMCP.h"
//#include "MD.h"
//#include "LANIfc.h"
//#include "WDT.h"
//#include "NVRAccess.h"
//#include "Util.h"
#include "libipmi_struct.h"
//#include "nwcfg.h"
//#include "Ethaddr.h"
#include "IPMIConf.h"
#include "IPMBIfc.h"
//#include "IPMI_KCS.h"
//#include "IPMI_BT.h"
//#include "ipmi_userifc.h"
//#include "Badpasswd.h"
//#include "hal_hw.h"
//#include "iniparser.h"
//#include "Session.h"
//#include "LANConfig.h"
//#include "userprivilege.h"
//#include "PDKBridgeMsg.h"
//#include "featuredef.h"
//#include "blowfish.h"
//#include "Encode.h"


#define USER_ID_ENABLED 	0x01
#define USER_ID_DISABLED 	0x02
#define OP_USERID_ONLY_LENGTH    2
#define OP_ENABLE_USER_ID    	 1
#define OP_DISABLE_USER_ID    	 0
#define BIT3_BIT0_MASK     0xf
#define GET_AUTH_TYPE_MASK  0xc0
#define AUTH_TYPE_V15	0x0
#define AUTH_TYPE_V20	0x40
#define AUTH_CODE_V15_MASK  0x0f
#define AUTH_CODE_V15_1  0x1
#define AUTH_CODE_V15_2  0x2
#define AUTH_CODE_V15_3  0x5
#define AUTH_CODE_V20_MASK  0x3f
#define MIN_AUTH_CODE_V20 0x04
#define MAX_AUTH_CODE_V20 0xc0
#define NULL_USER                 1
#define ZERO_SETSELECTOR 0x00
#define MAX_TYPE_OF_ENCODING 0x02
#define MAX_STRING_LENGTH_COPY 14
#define DISABLE_USR_LEVEL_AUTH 0x08

#define ASCII_LATIN1        0x00
#define UTF_8                     0x01
#define UNICODE                 0x02

/* Reserved bit macro definitions */
#define RESERVED_BITS_SENDMS 0x03 //(BIT1 | BIT0)

/* Auth code length */
#define HMAC_SHA1_96_LEN            12

#if APP_DEVICE == 1

#define COUNT_INCREASE  1	
#define COUNT_DECREASE -1
#define MAX_BT_PKT_LEN 64

//#define RESERVED_USERS_FILE "/etc/reservedusers"

/*macro definitions for set user password options*/
#define DISABLE_USER    0
#define ENABLE_USER     1
#define SET_PASSWORD    2
#define TEST_PASSWORD   3
#define MIN_REQ_LENTH   2
/*** Global variables ***/
_FAR_   INT8U   g_TmrRunning;
/*** Module variables ***/
static INT8U m_Set_ChReserveBit[] ={0xF0,0x0,0x30};

//extern IfcName_T Ifcnametable[MAX_LAN_CHANNELS];

/**
 * @fn CheckReservedUsers
 * @brief This function will checks for reserved users.
 * @param  Username - Username.
 * @retval availability of reserved users.
 */
static int CheckForReservedUsers(char *Username)
{

	return 0;
}

static void UpdateCurrentEnabledUserCount(int value, int bmcInstId)
{

}

static int IsPrivilegeAvailable(INT8U requestedPrivilege, INT8U channelNumber, int bmcInstId)  
{  

	
	//All bits are 0 that means privilege level is disabled  
	return 0;  
}


/*-------------------------------------
* ResetWDT
*-------------------------------------*/
int
ResetWDT (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    INT8U	u8ExpirationFlag;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;


    if (pBMCInfo->Msghndlr.TmrSet == FALSE)
    {
        *pRes = CC_ATTEMPT_TO_RESET_UNIN_WATCHDOG;
        return sizeof (*pRes);
    }

    // save the WDT expiration flag for later use
//    u8ExpirationFlag = g_WDTTmrMgr.WDTTmr.ExpirationFlag;


    /* Reset of Watchdog should not happen once
        once pretimeout interrupt interval is reached*/
    if(pBMCInfo->WDTPreTmtStat == TRUE)
    {
        *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
        return sizeof (*pRes);
    }

//    g_WDTTmrMgr.TmrPresent  = TRUE;
//    g_WDTTmrMgr.TmrInterval = pBMCInfo->WDTConfig.InitCountDown;
//    g_WDTTmrMgr.PreTimeOutInterval = SEC_TO_MS * pBMCInfo->WDTConfig.PreTimeOutInterval;

//    /* if the pre-timeout interrupt is not configured, adjust the pre-timeout interrupt
//        timeout value beyound the regular WDT timeout value so that it won't get triggered
//        before the WDT timeout. */
//    if ((pBMCInfo->WDTConfig.TmrActions & 0x70) == 0)
//    {
//        g_WDTTmrMgr.PreTimeOutInterval = g_WDTTmrMgr.TmrInterval+ 1;
//    }

//    _fmemcpy (&g_WDTTmrMgr.WDTTmr, &pBMCInfo->WDTConfig, sizeof (WDTConfig_T));

//    // restore the WDT expiration flag, don't use the one from the flash
//    g_WDTTmrMgr.WDTTmr.ExpirationFlag = u8ExpirationFlag;

//    // clear WDT sensor event history
//    if( g_corefeatures.internal_sensor == ENABLED )
//        RestartWD2Sensor(BMCInst);

//    if(g_corefeatures.wdt_flush_support == ENABLED )
//    {
//        FlushIPMI((INT8U*)&pBMCInfo->WDTConfig,(INT8U*)&pBMCInfo->WDTConfig,pBMCInfo->IPMIConfLoc.WDTDATAddr,
//                       sizeof(WDTConfig_T),BMCInst);
//    }

//    if(BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent == FALSE)
//    {
//        LOCK_BMC_SHARED_MEM(BMCInst);
//        BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning=TRUE;
//        BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent=TRUE;
//        UNLOCK_BMC_SHARED_MEM(BMCInst);
//        sem_post(&g_BMCInfo[BMCInst].WDTSem);
//    }
//    else
//    {
//        LOCK_BMC_SHARED_MEM(BMCInst);
//        BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning=TRUE;
//        BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent=TRUE;
//        UNLOCK_BMC_SHARED_MEM(BMCInst);
//        //Set SetWDTUpdated flag to reload initial countdown value.
//        g_BMCInfo[BMCInst].SetWDTUpdated = TRUE; 
//    }


    *pRes = CC_NORMAL;

    return sizeof (*pRes);
}



/*---------------------------------------
* SetWDT
*---------------------------------------*/
int
SetWDT (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_  SetWDTReq_T*    pSetWDTReq = (_NEAR_ SetWDTReq_T*)pReq;
#if GET_MSG_FLAGS != UNIMPLEMENTED
    GetMsgFlagsRes_T   GetMsgFlagsRes;
#endif
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo;


    //Check for Reserved bits
     if((pSetWDTReq->TmrUse & (BIT5 | BIT4 | BIT3)) || !(pSetWDTReq->TmrUse & (BIT2 | BIT1 | BIT0)) || ((pSetWDTReq->TmrUse & (BIT1 | BIT2)) == (BIT1 | BIT2)) || 
        (pSetWDTReq->TmrActions & (BIT7 |BIT6 | BIT3 | BIT2)) || (pSetWDTReq->ExpirationFlag & (BIT7 | BIT6 | BIT0)))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

#if NO_WDT_PRETIMEOUT_INTERRUPT == 1
    // do not support pre-timeout interrupt
    if (pSetWDTReq->TmrActions & 0x70)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }
#endif

//    pSetWDTReq->InitCountDown = htoipmi_u16(pSetWDTReq->InitCountDown);

//    // error out if the pre-timeout interrupt is greater than the initial countdown value
//    if (pSetWDTReq->InitCountDown < 10 * pSetWDTReq->PreTimeOutInterval)
//    {
//        *pRes = CC_INV_DATA_FIELD;
//        return sizeof(*pRes);
//    }

//    // only clear the memory version of the bit(s) when the input bit is set #31175
//    g_WDTTmrMgr.WDTTmr.ExpirationFlag &= ~pSetWDTReq->ExpirationFlag;
//    pSetWDTReq->ExpirationFlag = g_WDTTmrMgr.WDTTmr.ExpirationFlag;


//    /* Copy the Timer configuration in NVRAM */
//    LOCK_BMC_SHARED_MEM(BMCInst);
//    _fmemset ((_FAR_ INT8U*)&pBMCInfo->WDTConfig, 0, sizeof (WDTConfig_T));
//    _fmemcpy ((_FAR_ INT8U*)&pBMCInfo->WDTConfig, (_FAR_ INT8U*)pSetWDTReq, sizeof (SetWDTReq_T));
//    UNLOCK_BMC_SHARED_MEM(BMCInst);

//    if (TRUE ==BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning)
//    {
//        /* To check wheather Dont stop bit is set or not */
//        if (pSetWDTReq->TmrUse & 0x40)
//        {
//            /* Set the count down value to given value */
//            g_WDTTmrMgr.TmrPresent = TRUE;
//            LOCK_BMC_SHARED_MEM(BMCInst);
//            BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent =TRUE;
//            UNLOCK_BMC_SHARED_MEM(BMCInst);
//            g_WDTTmrMgr.TmrInterval= pSetWDTReq->InitCountDown;
//            g_WDTTmrMgr.PreTimeOutInterval = (SEC_TO_MS * pSetWDTReq->PreTimeOutInterval);

//            /* If PreTimeOutInt is set, clear it */
//            if (0 != (pSetWDTReq->TmrActions & 0x70))
//            {
//                pSetWDTReq->TmrActions &= ~0x70;
//            }
//            else
//            {
//                // if the pre-timeout interrupt is not configured, adjust the pre-timeout interrupt
//                // timeout value beyound the regular WDT timeout value so that it won't get triggered
//                // before the WDT timeout.
//                g_WDTTmrMgr.PreTimeOutInterval = pSetWDTReq->InitCountDown + 1;
//            }
//            _fmemcpy (&g_WDTTmrMgr.WDTTmr, pSetWDTReq, sizeof (WDTConfig_T ));

//        }
//        else
//        {
//            /* Stop the timer */
//            g_WDTTmrMgr.TmrPresent = FALSE;
//            LOCK_BMC_SHARED_MEM(BMCInst);
//            BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning=FALSE;
//            BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent =FALSE;
//            UNLOCK_BMC_SHARED_MEM(BMCInst);
//            g_WDTTmrMgr.TmrInterval= pSetWDTReq->InitCountDown;
//            g_WDTTmrMgr.PreTimeOutInterval = SEC_TO_MS * pSetWDTReq->PreTimeOutInterval;
//            // clear WDT sensor event history
//            if( g_corefeatures.internal_sensor == ENABLED)
//                RestartWD2Sensor(BMCInst);
//        }

//        /* Clear the  pre-timeout interupt flag */
//        LOCK_BMC_SHARED_MEM(BMCInst);
//        pBMCInfo->WDTConfig.PreTimeoutActionTaken = 0x00;
//        BMC_GET_SHARED_MEM (BMCInst)->MsgFlags &= ~0x08; /* Clear the flag */
//#if GET_MSG_FLAGS != UNIMPLEMENTED
//        // Clear SMS_ATN bit if and only if the Get Message Flag return 0 in byte 2.
//        GetMsgFlags (NULL, 0, (INT8U *)&GetMsgFlagsRes,BMCInst);
//        TDBG("GetMsgFlagsRes.CompletionCode : %X, GetMsgFlagsRes.MsgFlags : %X\n",
//                GetMsgFlagsRes.CompletionCode, GetMsgFlagsRes.MsgFlags);
//        if (GetMsgFlagsRes.CompletionCode == CC_NORMAL && GetMsgFlagsRes.MsgFlags == 0)
//#else
//        if((BMC_GET_SHARED_MEM(BMCInst)->MsgFlags & BIT3_BIT0_MASK) == 0)
//#endif
//        {
//            /* Clear the SMS_ATN bit */
//            if (pBMCInfo->IpmiConfig.KCS1IfcSupport == 1)
//            {
//                CLEAR_SMS_ATN (0, BMCInst);
//            }
//            if (pBMCInfo->IpmiConfig.KCS2IfcSupport == 1)
//            {
//                CLEAR_SMS_ATN (1, BMCInst);
//            }
//            if (pBMCInfo->IpmiConfig.KCS3IfcSuppport == 1)
//            {
//                CLEAR_SMS_ATN (2, BMCInst);
//            }
//            if(pBMCInfo->IpmiConfig.BTIfcSupport == 1 )
//            {
//            	CLEAR_BT_SMS_ATN(0, BMCInst);
//            }
//        }
//            UNLOCK_BMC_SHARED_MEM(BMCInst);

//        }
//        else
//        {
//            g_WDTTmrMgr.TmrInterval = pSetWDTReq->InitCountDown;
//            g_WDTTmrMgr.TmrPresent = FALSE;
//            LOCK_BMC_SHARED_MEM(BMCInst);
//            BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent =FALSE;
//            UNLOCK_BMC_SHARED_MEM(BMCInst);
//            // clear WDT sensor event history
//            if( g_corefeatures.internal_sensor == ENABLED)
//                RestartWD2Sensor(BMCInst);
//        }

//    // Modify ARP status to resume the thread
//    // after receiving set Watchdog Timer command
//    //BMC_GET_SHARED_MEM(BMCInst)->GratArpStatus = RESUME_ARPS;
//    
//    int i = 0; 

//    for (i = 0; i < MAX_LAN_CHANNELS; i++)	
//    {
//        if((pBMCInfo->LanIfcConfig[i].Enabled == TRUE)
//                && (pBMCInfo->LanIfcConfig[i].Up_Status == LAN_IFC_UP))
//        {
//            BMC_GET_SHARED_MEM(BMCInst)->ArpSuspendStatus[i] = RESUME_ARPS; 	
//            UpdateArpStatus(pBMCInfo->LanIfcConfig[i].Ethindex, BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning, BMCInst);  
//        }
//    }
//    if(g_corefeatures.wdt_flush_support == ENABLED )
//    {
//        FlushIPMI((INT8U*)&pBMCInfo->WDTConfig,(INT8U*)&pBMCInfo->WDTConfig,pBMCInfo->IPMIConfLoc.WDTDATAddr,
//              sizeof(WDTConfig_T),BMCInst);
//    }
//    // set the "Don't Log" bit
//    g_WDTTmrMgr.WDTTmr.TmrUse &= 0x7F;
//    g_WDTTmrMgr.WDTTmr.TmrUse |= (pSetWDTReq->TmrUse & 0x80);

//    g_BMCInfo[BMCInst].SetWDTUpdated = TRUE;
//    g_BMCInfo[BMCInst].Msghndlr.TmrSet = TRUE;
//    pBMCInfo->WDTPreTmtStat = FALSE;
    *pRes = CC_NORMAL;

    return sizeof (*pRes);
}


/*---------------------------------------
* GetWDT
*---------------------------------------*/
int
GetWDT (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_  GetWDTRes_T*    pGetWDTRes = (_NEAR_ GetWDTRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    /* Copy the current settings from the NVRAM */
//    LOCK_BMC_SHARED_MEM(BMCInst);
//    _fmemcpy ((_FAR_ INT8U*)&pGetWDTRes->CurrentSettings,
//            (_FAR_ INT8U*)&pBMCInfo->WDTConfig, sizeof (WDTConfig_T));
//    UNLOCK_BMC_SHARED_MEM(BMCInst);

//    // get the WDT expiration from the global veriable in memory, not from the flash
//    pGetWDTRes->CurrentSettings.ExpirationFlag = g_WDTTmrMgr.WDTTmr.ExpirationFlag;

//    // get the current "Don't Log" bit
//    pGetWDTRes->CurrentSettings.TmrUse &= 0x7F;
//    pGetWDTRes->CurrentSettings.TmrUse |= (g_WDTTmrMgr.WDTTmr.TmrUse & 0x80);
//    if (TRUE == BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent)
//    {
//         // set the WDT running bit #30235/30467
//        pGetWDTRes->CurrentSettings.TmrUse |= 0x40;
//        /* Present count down in 1/100 of second */
//    }
//    else
//    {
//         // clear the WDT running bit #30235/30467  for Timer Use (ie) WatchDog Timer status
//        pGetWDTRes->CurrentSettings.TmrUse &= ~0x40;
//        pGetWDTRes->CurrentSettings.ExpirationFlag = (pGetWDTRes->CurrentSettings.ExpirationFlag) & 0x3E;
//    }

//    pGetWDTRes->PresentCountDown			   = g_WDTTmrMgr.TmrInterval;
//    pGetWDTRes->CurrentSettings.InitCountDown = htoipmi_u16(pGetWDTRes->CurrentSettings.InitCountDown);
    pGetWDTRes->CompletionCode                = CC_NORMAL;

    return sizeof (GetWDTRes_T);
}


/*---------------------------------------
* SetBMCGlobalEnables
*---------------------------------------*/
int
SetBMCGlobalEnables (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    INT8U GblEnblByte = *pReq;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    MsgPkt_T MsgPkt;

//    _fmemset (&MsgPkt, 0, sizeof (MsgPkt_T));

//    /* Check For the reserved bit 4 */
//    if ( GblEnblByte & BIT4)
//      {
//         *pRes = CC_INV_DATA_FIELD;
//          return sizeof (*pRes);
//      }

//    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->BMCMsgMutex,WAIT_INFINITE);
//    if (((BMC_GET_SHARED_MEM (BMCInst)->GlobalEnables ^ GblEnblByte)) & 0x20) 
//    {
//        /* OEM 0 puts us in ICTS compatibility mode for IPMIv2,
//         * Send a message to lan process so it can change behavior
//         */
//        MsgPkt.Channel    = GetLANChannel(0, BMCInst);
//        MsgPkt.Param      = LAN_ICTS_MODE;
//        MsgPkt.Privilege  = PRIV_LOCAL;
//        if (GblEnblByte & 0x20)
//            MsgPkt.Cmd = 1;
//        else
//            MsgPkt.Cmd = 0;
//        PostMsg(&MsgPkt,LAN_IFC_Q,BMCInst);
//    }

//    BMC_GET_SHARED_MEM (BMCInst)->GlobalEnables = GblEnblByte;
    *pRes = CC_NORMAL;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->BMCMsgMutex);

    return sizeof (*pRes);
}


/*---------------------------------------
* GetBMCGlobalEnables
*---------------------------------------*/
int
GetBMCGlobalEnables (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_  GetBMCGblEnblRes_T* pGetBMCGblEnblRes = (_NEAR_ GetBMCGblEnblRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

//    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->BMCMsgMutex,WAIT_INFINITE);
    pGetBMCGblEnblRes->CompletionCode = CC_NORMAL;
//    pGetBMCGblEnblRes->BMCGblEnblByte = BMC_GET_SHARED_MEM (BMCInst)->GlobalEnables;
//    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->BMCMsgMutex);

    return sizeof (GetBMCGblEnblRes_T);
}


/*---------------------------------------
* ClrMsgFlags
*---------------------------------------*/
int
ClrMsgFlags (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ ClearMsgsFlagReq_T* pClearMsgsFlagReq = (_NEAR_ ClearMsgsFlagReq_T*)pReq;
    INT8U kcsifcnum;
#if GET_MSG_FLAGS != UNIMPLEMENTED
    GetMsgFlagsRes_T   GetMsgFlagsRes;
#endif
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo;

    //Check for Reserved bits
    if(pClearMsgsFlagReq->Flag & (BIT4 | BIT2))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

//    OS_THREAD_TLS_GET(g_tls.CurKCSIfcNum,kcsifcnum);
//    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->BMCMsgMutex,WAIT_INFINITE);
//    /* Flush Receive Message Queue */
//    if (0 != (pClearMsgsFlagReq->Flag & 0x01))
//    {
//        while (0 == GetMsg (&m_MsgPkt, &g_RcvMsgQ[kcsifcnum][0], WAIT_NONE,BMCInst))
//        {
//            BMC_GET_SHARED_MEM (BMCInst)->NumRcvMsg[kcsifcnum]--;
//        }

//        BMC_GET_SHARED_MEM (BMCInst)->MsgFlags &= ~0x01; /* Clear the flag */
//    }

//    /* Flush Event Message Buffer */
//    if (0 != (pClearMsgsFlagReq->Flag & 0x02))
//    {
//        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->EventMutex,WAIT_INFINITE);
//        while (0 == GetMsg (&m_MsgPkt, EVT_MSG_Q, WAIT_NONE,BMCInst))
//        {
//            BMC_GET_SHARED_MEM (BMCInst)->NumEvtMsg--;
//        }
//        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->EventMutex);
//        BMC_GET_SHARED_MEM (BMCInst)->MsgFlags &= ~0x02; /* Clear the flag */
//    }

//    /* Clear WatchdogTimer Interrupt*/
//    if (0 != (pClearMsgsFlagReq->Flag & 0x08))
//    {
//        /* Clear the  pre-timeout interupt flag */
//        pBMCInfo->WDTConfig.PreTimeoutActionTaken = 0x00;
//        BMC_GET_SHARED_MEM (BMCInst)->MsgFlags &= ~0x08; /* Clear the flag */
//	if(g_corefeatures.wdt_flush_support == ENABLED )
//    	{
//            FlushIPMI((INT8U*)&pBMCInfo->WDTConfig,(INT8U*)&pBMCInfo->WDTConfig,pBMCInfo->IPMIConfLoc.WDTDATAddr,
//                           sizeof(WDTConfig_T),BMCInst);
//       }

//    }

//#if GET_MSG_FLAGS != UNIMPLEMENTED
//    // Clear SMS_ATN bit if and only if the Get Message Flag return 0 in byte 2.
//    GetMsgFlags (NULL, 0, (INT8U *)&GetMsgFlagsRes,BMCInst);
//    TDBG("GetMsgFlagsRes.CompletionCode : %X, GetMsgFlagsRes.MsgFlags : %X\n",
//            GetMsgFlagsRes.CompletionCode, GetMsgFlagsRes.MsgFlags);
//    if (GetMsgFlagsRes.CompletionCode == CC_NORMAL && GetMsgFlagsRes.MsgFlags == 0)
//#else
//    if((BMC_GET_SHARED_MEM(BMCInst)->MsgFlags & BIT3_BIT0_MASK) == 0)
//#endif
//    {
//        /* Clear the SMS_ATN bit */
//        if (pBMCInfo->IpmiConfig.KCS1IfcSupport == 1)
//        {
//            CLEAR_SMS_ATN (0, BMCInst);
//        }
//        if (pBMCInfo->IpmiConfig.KCS2IfcSupport == 1)
//        {
//            CLEAR_SMS_ATN (1, BMCInst);
//        }
//        if (pBMCInfo->IpmiConfig.KCS3IfcSuppport == 1)
//        {
//            CLEAR_SMS_ATN (2, BMCInst);
//        }
//        if(pBMCInfo->IpmiConfig.BTIfcSupport == 1 )
//        {
//            CLEAR_BT_SMS_ATN(0, BMCInst);
//        }
//    }

    *pRes = CC_NORMAL;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->BMCMsgMutex);

    return sizeof (*pRes);
}


/*---------------------------------------
 GetMsgFlags
---------------------------------------*/
int
GetMsgFlags (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_  GetMsgFlagsRes_T*   pGetMsgFlagsRes = (_NEAR_ GetMsgFlagsRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    INT8U kcsifcnum;

//    OS_THREAD_TLS_GET(g_tls.CurKCSIfcNum,kcsifcnum);
//    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->BMCMsgMutex,WAIT_INFINITE);
//    /* get the message flags */
//    pGetMsgFlagsRes->MsgFlags = BMC_GET_SHARED_MEM (BMCInst)->MsgFlags;

//    if (BMC_GET_SHARED_MEM (BMCInst)->NumEvtMsg >= EVT_MSG_BUF_SIZE)
//    {
//        /* If Event MessageBuffer is Full set the BIT */
//        pGetMsgFlagsRes->MsgFlags |= 0x02;
//    }
//    else
//    {
//        /* else reset the Flag */
//        pGetMsgFlagsRes->MsgFlags &= ~0x02;
//    }

//    if(kcsifcnum !=0xFF && 0 != BMC_GET_SHARED_MEM (BMCInst)->NumRcvMsg[kcsifcnum])
//    {
//        /* if any Message in ReceiveMsgQ set the Flag */
//        pGetMsgFlagsRes->MsgFlags |= 0x01;
//    }
//    else
//    {
//        /* else reset the Flag */
//        pGetMsgFlagsRes->MsgFlags &= ~0x01;
//    }

//    /* get the  Pre-Timeout Bits Value & Set it to Response Data */
//    //PRETIMEOUT BIT is 3rd bit so changed accordingly
//    pGetMsgFlagsRes->MsgFlags |= (pBMCInfo->WDTConfig.PreTimeoutActionTaken & 0x08);

//    /* Update the Message flags in shared Mem */
//    BMC_GET_SHARED_MEM (BMCInst)->MsgFlags |=  pGetMsgFlagsRes->MsgFlags;
//    pGetMsgFlagsRes->CompletionCode = CC_NORMAL;
//    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->BMCMsgMutex);

    return sizeof (GetMsgFlagsRes_T);
}


/*---------------------------------------
* EnblMsgChannelRcv
*---------------------------------------*/
int
EnblMsgChannelRcv (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    return sizeof (EnblMsgChRcvRes_T);
}


/*---------------------------------------
* GetMessage
*---------------------------------------*/
int
GetMessage (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    return 0;
    // return  ((m_MsgPkt.Size-1));      /*+ 2 for completion code & channel No. */
}


/*---------------------------------------
* SendMessage
*---------------------------------------*/
extern xQueueHandle ResponseDatMsg_Queue;
extern xQueueHandle RecvForwardI2CDatMsg_Queue;
int
SendMessage (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    char buff[sizeof(MsgPkt_T)];
    _NEAR_  SendMsgReq_T* pSendMsgReq = (_NEAR_ SendMsgReq_T*)pReq;
    _NEAR_  SendMsgRes_T* pSendMsgRes = (_NEAR_ SendMsgRes_T*)pRes;
    _NEAR_  IPMIMsgHdr_T* pIPMIMsgHdr;
    INT8U         Tracking;
    INT8U         Channel=0,resaddr=0;
    INT8U         ResLen = 1;
    INT8U         RetVal = 0;
    INT8U         SeqNum = 0;

    MsgPkt_T    MsgReq;
    MsgPkt_T*   MsgRes = (MsgPkt_T*)buff; 
    BaseType_t err = pdFALSE;

    int i;

    if (ReqLen < 1)
    {
        *pRes = CC_REQ_INV_LEN;
        return  sizeof (*pRes);
    }

    if(pSendMsgReq->ChNoTrackReq == 0xC0)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }
        /* Get the channel number */
    Channel = pSendMsgReq->ChNoTrackReq & 0x0F;

    /* Get Tracking field */
    Tracking = pSendMsgReq->ChNoTrackReq >> 6;

    if (Tracking == RESERVED_BITS_SENDMS)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    // m_MsgPkt.Param    = BRIDGING_REQUEST;
    MsgReq.Param    = IPMI_REQUEST;
    MsgReq.Channel  = Channel;
    MsgReq.Size     = ReqLen - 1; /* -1 to skip channel num */

    /* Copy the message data */
    _fmemcpy (MsgReq.Data, &pReq[1], MsgReq.Size);
    // LOG_RAW("\r\rRecv bridge dat: ");
    // for (i = 0; i < MsgReq.Size; i++)
    // {
    // 		LOG_RAW("%02x ", MsgReq.Data[i]);
    // }
    // LOG_RAW("\r\n");
		
	if(CheckMsgValidation(MsgReq.Data, MsgReq.Size) == FALSE)
    {
        LOG_E("bridge msg checksum err!");
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    // send msg ipmb
    err = xQueueSend(ResponseDatMsg_Queue, (char*)&MsgReq, 50);
    if(err == pdFALSE){
        LOG_E("sendmessage send failed.");
         pRes[0] = CC_NO_ACK_FROM_SLAVE;
         return 1;
    }
    err = xQueueReceive(RecvForwardI2CDatMsg_Queue, buff, 50);
    if(err == pdFALSE){
        LOG_E("sendmessage recv failed.");
        pRes[0] = CC_NO_ACK_FROM_SLAVE;
        return 1;
    }

     pRes[0] = CC_NORMAL;
    _fmemcpy (&pRes[1], MsgRes->Data, MsgRes->Size);
    return MsgRes->Size + 1;
}


/*---------------------------------------
* ReadEvtMsgBuffer
*---------------------------------------*/
int
ReadEvtMsgBuffer (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    return sizeof (ReadEvtMsgBufRes_T);
}


/*---------------------------------------
* GetBTIfcCap
*---------------------------------------*/
int
GetBTIfcCap (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_  GetBTIfcCapRes_T* pGetBTIfcCapRes = (_NEAR_ GetBTIfcCapRes_T*)pRes;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo;
    
    if(pBMCInfo->IpmiConfig.BTIfcSupport ==1)
    {
		pGetBTIfcCapRes->CompletionCode     = CC_NORMAL;
		pGetBTIfcCapRes->NumReqSupported    = 2;
		pGetBTIfcCapRes->InputBufSize       = MAX_BT_PKT_LEN;
		pGetBTIfcCapRes->OutputBufSize      = MAX_BT_PKT_LEN;
		pGetBTIfcCapRes->RespTime           = 1;
		pGetBTIfcCapRes->Retries            = 0;
    }
    else
    {
		pGetBTIfcCapRes->CompletionCode = CC_INV_CMD;
    }
       return sizeof (GetBTIfcCapRes_T);
}


/*---------------------------------------
* GetSystemGUID
*---------------------------------------*/
int
GetSystemGUID (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetSysGUIDRes_T* pGetSysGUIDRes = (_NEAR_ GetSysGUIDRes_T*)pRes;

    pGetSysGUIDRes->CompletionCode  = CC_NORMAL;

    return sizeof (GetSysGUIDRes_T);
}


#define SUPPORT_IPMI20  0x02
#define SUPPORT_IPMI15  0x01
/*---------------------------------------
* GetChAuthCap
*---------------------------------------*/
int
GetChAuthCap (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetChAuthCapReq_T*   pGetChAuthCapReq = (_NEAR_ GetChAuthCapReq_T*)pReq;
    _NEAR_ GetChAuthCapRes_T*   pGetChAuthCapRes = (_NEAR_ GetChAuthCapRes_T*)pRes;
    _FAR_  ChannelUserInfo_T*   pChUserInfo;
    _FAR_  UserInfo_T*          pUserInfo;
    _FAR_  ChannelInfo_T*       pChannelInfo;
    INT8U                       ChannelNum, Index,curchannel;
    INT8U                       i;
    INT8U 			EthIndex=0;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo;
    INT8U   UserPswd[16] = {0};
    INT8U EncryptionEnabled = 0;
    INT8U    PwdEncKey[MAX_SIZE_KEY + 1] = {0};
    char  EncryptedPswd[MAX_ENCRYPTED_PSWD_LEN + 1] = {0};

    ChannelNum  = pGetChAuthCapReq->ChannelNum & 0x0F;
    memset (pGetChAuthCapRes, 0, sizeof (GetChAuthCapRes_T));

    //Check for Reserved bits
    if((pGetChAuthCapReq->ChannelNum & (BIT6 | BIT5 | BIT4)) || (pGetChAuthCapReq->PrivLevel == AUTHTYPE_NONE) || (pGetChAuthCapReq->PrivLevel > AUTHTYPE_OEM_PROPRIETARY))
    {
        pGetChAuthCapRes->CompletionCode = CC_INV_DATA_FIELD;   /*Reserved bits   */
        return sizeof (*pRes);
    }

    /* Information for this Channel */
    // if (CURRENT_CHANNEL_NUM == ChannelNum)
    // {
    //     ChannelNum = curchannel & 0xF;
    // }
    
    /* For KCS and IPMB interfaces   */
    if((ChannelNum==0x0f)||(ChannelNum==0x00))
    {
        pGetChAuthCapRes->CompletionCode=CC_INV_DATA_FIELD;
   	    return sizeof (*pRes);	  	
    }	
    
    // if (0 != IsLANChannel(ChannelNum,BMCInst))
    // {
    //     EthIndex= GetEthIndex(ChannelNum,BMCInst);
    //     if(0xff == EthIndex)
    //     {
    //         *pRes = CC_INV_DATA_FIELD;
    //         return sizeof (*pRes);
    //     }
    // }

    // pChannelInfo = getChannelInfo (ChannelNum, BMCInst);
    // if (NULL == pChannelInfo)
    // {
    //     /* Invalid channel  */
    //     pGetChAuthCapRes->CompletionCode = CC_INV_DATA_FIELD;   /*Invalid Channel   */
    //     return sizeof (*pRes);
    // }

    pGetChAuthCapRes->CompletionCode    = CC_NORMAL; /* completion code */
    pGetChAuthCapRes->ChannelNum        = ChannelNum; /* channel No */
    pGetChAuthCapRes->AuthType = 0x01;

    return sizeof (GetChAuthCapRes_T);
}


/*---------------------------------------
* GetSessionChallenge
*---------------------------------------*/
int
GetSessionChallenge (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetSesChallengeReq_T* pGetSesChalReq = (_NEAR_ GetSesChallengeReq_T*)pReq;
    _NEAR_ GetSesChallengeRes_T* pGetSesChalRes = (_NEAR_ GetSesChallengeRes_T*)pRes;

    pGetSesChalRes->CompletionCode = CC_NORMAL;
    pGetSesChalRes->TempSessionID  = 0x01;
    return sizeof (GetSesChallengeRes_T);
}

/*---------------------------------------
* ActivateSession
*---------------------------------------*/
int
ActivateSession (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_  ActivateSesReq_T*   pAcvtSesReq = (_NEAR_ ActivateSesReq_T*)pReq;
    _NEAR_  ActivateSesRes_T*   pAcvtSesRes = (_NEAR_ ActivateSesRes_T*)pRes;

        /* Initial Outbound Sequence Number cannot be null */  
    if (pAcvtSesReq->OutboundSeq == 0)  
    {  
        pAcvtSesRes->CompletionCode = CC_ACTIVATE_SESS_SEQ_OUT_OF_RANGE;  
        return sizeof(*pRes);  
    }
    pAcvtSesRes->AuthType = 0x00;
    pAcvtSesRes->SessionID = 1;
    //increasing the inbound sequence to keep sync with the actual inbound sequence sent to client application
    pAcvtSesRes->InboundSeq = 0 + 1;
    pAcvtSesRes->Privilege = 0x04;
    

    pAcvtSesRes->CompletionCode = CC_NORMAL;

    return sizeof (ActivateSesRes_T);
}


/*---------------------------------------
* SetSessionPrivLevel
*---------------------------------------*/
int
SetSessionPrivLevel (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_  SetSesPrivLevelReq_T*   pSetSesPrivLevelReq = (_NEAR_ SetSesPrivLevelReq_T*)pReq;
    _NEAR_  SetSesPrivLevelRes_T*   pSetSesPrivLevelRes = (_NEAR_ SetSesPrivLevelRes_T*)pRes;

    /* set the privilege for the session */
    pSetSesPrivLevelRes->CompletionCode = CC_NORMAL;
    pSetSesPrivLevelRes->Privilege      = pSetSesPrivLevelReq->Privilege;

    return sizeof (SetSesPrivLevelRes_T);
}


/*---------------------------------------
* CloseSession
*---------------------------------------*/
int
CloseSession (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
 
    *pRes = CC_NORMAL;
    return sizeof (*pRes);
}

#if GET_SESSION_INFO != UNIMPLEMENTED
/*---------------------------------------
* GetSessionInfo
*---------------------------------------*/
int
GetSessionInfo (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

	return (sizeof (GetSesInfoRes_T) - sizeof (SessionInfoRes_T));
}
#endif


/**
 * @fn GetAuthCodeForTypeV15
 * @brief This function will use the encryption technique supported 
 * 			in IPMI V1.5 in order to produce Auth Code.
 * @param[in] pUserInfo - Pointer to User info structure.
 * @param[in] pGetAuthCodeReq - Pointer to the structure of request data
 * @param[out] pGetAuthCodeRes - Pointer to the resultant data.
 * @retval size of the result data.
 */
static int
GetAuthCodeForTypeV15 (UserInfo_T* pUserInfo,
        GetAuthCodeReq_T* pGetAuthCodeReq,
        GetAuthCodeRes_T* pGetAuthCodeRes,int BMCInst)
{
 
    // IPMI V1.5 AuthCode is only 16 byte.
    return (sizeof (GetAuthCodeRes_T) - 4);
}

/**
 * @fn GetAuthCodeForTypeV20
 * @brief This function will use the encryption technique supported 
 * 			in IPMI V2.0 in order to produce Auth Code.
 * @param[in] pUserInfo - Pointer to User info structure.
 * @param[in] pGetAuthCodeReq - Pointer to the structure of request data
 * @param[out] pGetAuthCodeRes - Pointer to the resultant data.
 * @retval size of the result data.
 */
static int
GetAuthCodeForTypeV20 (UserInfo_T* pUserInfo,
        GetAuthCodeReq_T* pGetAuthCodeReq,
        GetAuthCodeRes_T* pGetAuthCodeRes,int BMCInst)
{
 
		return sizeof(GetAuthCodeRes_T);
}

/**
 * @fn GetAuthCode
 * @brief This function will encrypt the given 16 byte data with
 *          the algorithm given and return Auth Code.
 * @param[in] pReq - Request data.
 * @param[in] ReqLen - Length of the request data.
 * @param[out] pRes - Result data
 * @retval size of the result data.
 */
int
GetAuthCode (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{

    return 0;
}


/*---------------------------------------
* SetChAccess
*---------------------------------------*/
int
SetChAccess (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    *pRes = CC_NORMAL;

    return sizeof (*pRes);
}


/*---------------------------------------
* GetChAccess
*---------------------------------------*/
int
GetChAccess (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
 
    return sizeof (GetChAccessRes_T);
}


/*---------------------------------------
* GetChInfo
*---------------------------------------*/
int
GetChInfo (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    return sizeof (GetChInfoRes_T);
}

/*---------------------------------------
* IsChannelSuppGroups
*---------------------------------------*/
INT8U  IsChannelSuppGroups(INT8U ChannelNum,int BMCInst)
{
 
		return 0;
}
/*---------------------------------------
* ModifyUsrGRP
*---------------------------------------*/
int
ModifyUsrGrp(char * UserName,INT8U ChannelNum,INT8U OldAccessLimit, INT8U NewAccessLimit )
{

    return 0;
}

/*---------------------------------------
* SetUserAccess
*---------------------------------------*/
int
SetUserAccess (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    *pRes = CC_NORMAL;

    return sizeof (*pRes);
}


/*---------------------------------------
* GetUserAccess
*---------------------------------------*/
int
GetUserAccess (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{


    return sizeof (GetUserAccessRes_T);
}


/*---------------------------------------
* SetUserName
*---------------------------------------*/
int
SetUserName (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
        *pRes = CC_NORMAL;
        return sizeof (*pRes);
}


/*---------------------------------------
* GetUserName
*---------------------------------------*/
int
GetUserName (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    return sizeof (GetUserNameRes_T);
}


/*---------------------------------------
* SetUserPassword
*---------------------------------------*/
int
SetUserPassword (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    return sizeof (*pRes);
}


/*---------------------------------------
* MasterWriteRead
*---------------------------------------*/
int
MasterWriteRead (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ MasterWriteReadReq_T* pMasterWriteReadReq = (_NEAR_ MasterWriteReadReq_T*)pReq;
    _NEAR_ MasterWriteReadRes_T* pMasterWriteReadRes = (_NEAR_ MasterWriteReadRes_T*)pRes;

    pMasterWriteReadRes->CompletionCode = CC_NORMAL;

    return (sizeof (*pRes));
}


#if 0
/*-------------------------------------------
* ComputeAuthCode
*-------------------------------------------*/
void
ComputeAuthCode (_FAR_ INT8U* pPassword, _NEAR_ SessionHdr_T* pSessionHdr,
                _NEAR_ IPMIMsgHdr_T* pIPMIMsg, _NEAR_ INT8U* pAuthCode,
                INT8U ChannelType)
{
    if (AUTH_TYPE_PASSWORD == pSessionHdr->AuthType)
    {
        _fmemcpy (pAuthCode, pPassword, MAX_PASSWORD_LEN);
    }
    else
    {
        INT8U   AuthBuf [MAX_AUTH_PARAM_SIZE];
        INT16U  AuthBufLen = 0;
        INT8U   IPMIMsgLen = *((_NEAR_ INT8U*) pIPMIMsg - 1);

        /* Password */
        _fmemcpy (AuthBuf, pPassword, MAX_PASSWORD_LEN);
        AuthBufLen += MAX_PASSWORD_LEN;
        /* Session ID */
        _fmemcpy (AuthBuf + AuthBufLen, &pSessionHdr->SessionID, sizeof (INT32U));
        AuthBufLen += sizeof (INT32U);
        /* IPMI Response Message */
        _fmemcpy (AuthBuf + AuthBufLen, pIPMIMsg, IPMIMsgLen);
        AuthBufLen += IPMIMsgLen;

        if (ChannelType == MULTI_SESSION_CHANNEL)
        {
            /* Session Sequence Number */
            _fmemcpy (AuthBuf + AuthBufLen,
                    &pSessionHdr->SessionSeqNum, sizeof (INT32U));
            AuthBufLen += sizeof (INT32U);
        }
        /* Password */
        _fmemcpy (AuthBuf + AuthBufLen, pPassword, MAX_PASSWORD_LEN);
        AuthBufLen += MAX_PASSWORD_LEN;

        switch (pSessionHdr->AuthType)
        {
        case AUTH_TYPE_MD2 :
            AuthCodeCalMD2 (AuthBuf, pAuthCode, AuthBufLen);
            break;

        case AUTH_TYPE_MD5 :
            AuthCodeCalMD5 (AuthBuf, pAuthCode, AuthBufLen);
            break;

        default  :
            TDBG ("RMCP: Invalid Authentication Type \n");
        }
    }
}
#endif

/*---------------------------------------
* GetSystemInfoParam
*---------------------------------------*/
int
GetSystemInfoParam (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    /* return the size of the response */
    return 0;
}

int validatestring(INT8U* String,int len,INT8U TOE)
{
    int i,delimit = 0,strlen = 0;

    for(i=0;i<len;i++)
    {
        if(String[i] == 0)
        {
            delimit++;
        }
        else
        {
            if(delimit != 0)
            {
                if(TOE == UNICODE)
                {
                    if(delimit == 2)
                    {
                        strlen = strlen + 2;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else
                {
                    if(delimit == 1)
                    {
                        strlen = strlen + 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                delimit = 0;
            }
            strlen++;
        }
    }

    return strlen;
}

/*---------------------------------------
* SetSystemInfoParam
*---------------------------------------*/
int
SetSystemInfoParam (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    return sizeof(SetSystemInfoParamRes_T);
}

#endif  /* APP_DEVICE */
