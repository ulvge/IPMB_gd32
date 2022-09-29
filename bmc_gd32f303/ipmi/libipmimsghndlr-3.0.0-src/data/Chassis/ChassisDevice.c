/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2005-2006, American Megatrends Inc.        **
 ***                                                            **
 ***            All Rights Reserved.                            **
 ***                                                            **
 ***        6145-F, Northbelt Parkway, Norcross,                **
 ***                                                            **
 ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 ***                                                            **
 *****************************************************************
 *****************************************************************
 ******************************************************************
 *
 * ChassisDevice.c
 * Chassis commands
 *
 *  Author: Rama Bisa <ramab@ami.com>
 *
 ******************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "ChassisDevice.h"
//#include "Debug.h"
#include "Support.h"
#include "IPMI_ChassisDevice.h"
#include "IPMIDefs.h"
#include "MsgHndlr.h"
#include "Util.h"
#include "Platform.h"
#include "NVRAccess.h"
//#include "SharedMem.h"
#include "ChassisCtrl.h"
//#include "PDKAccess.h"
#include "IPMIConf.h"
//#include "featuredef.h"
#include "cpu/api_cpu.h"

/* Reserved bit macro definitions */
#define RESERVED_BITS_CHASSISCONTROL                    0xF0 //(BIT7 | BIT6 | BIT5 |BIT4)
#define RESERVED_BITS_GETCHASSISIDENTIFY                0xFE //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0)
#define RESERVED_BITS_SETCHASSISCAPS                    0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)
#define RESERVED_BITS_SETPOWERRESTOREPOLICY             0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)
#define RESERVED_BITS_SETFPBUTTONENABLES                0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)

#if CHASSIS_DEVICE == 1

/*** Local Definitions ***/
#define PRP_ALWAYS_POWEROFF_SUPPORT             0x01
#define PRP_LAST_STATE_SUPPORT                  0x02
#define PRP_ALWAYS_POWERON_SUPPORT              0x04


#define CHASSIS_AMI_OEM_PARAM                   96
#define CHASSIS_SET_INPROG                      0x00
#define CHASSIS_SERVICE_PART_SEL                0x01
#define CHASSIS_SERVICE_PART_SCAN               0x02
#define CHASSIS_BOOT_FLAG_VALID_BIT_CLEAR       0x03
#define CHASSIS_BOOT_INFO_ACK                   0x04
#define CHASSIS_BOOT_FLAGS                      0x05
#define CHASSIS_BOOT_INITIATOR_INFO             0x06
#define CHASSIS_BOOT_INITIATOR_MBOX             0x07
#define CHASSIS_AMI_OEM_PARAM                   96
#define SSICB_OEM_PARAM_BLK_SIZE_TBL            0x78
#define SSICB_BOOT_ORDER_TBL                    0x7D
#define SSICB_BOOT_DEV_SELECTOR                 0x7E
#define SSICB_SLOT_CONFIG_TBL                   0x7F

#define RESERVED_CHASSIS_SET_INPROG				 0x03

#define ROLLBACK_OPTION                         0x00
#define CHASSIS_DEFAULT_BRIDGE_DEV_ADDR         0x20
/* Mask Bits */
#define BIT5_BIT2_MASK				0x3C
#define BIT6_BIT5_MASK				0x60
#define BIT1_BIT0_MASK				0x03
       
/* Reserved Bits */
#define RESERVED_VALUE_03			0x03
#define RESERVED_VALUE_10			0x10
#define RESERVED_VALUE_20			0x20
#define RESERVED_VALUE_28			0x28
#define RESERVED_VALUE_30			0x30
#define RESERVED_VALUE_34			0x34
#define RESERVED_VALUE_38			0x38
#define RESERVED_VALUE_40			0x40
#define RESERVED_VALUE_60			0x60
#define RESERVED_VALUE_80			0x80

/*** Module Varibales ***/
static const INT8U            m_BootOptParamLen [] = /**< Boot Options parameter length */
{   
    0x01,                           /**< Set in progress byte length */
    0x01,                           /**< Service Partition selector length */
    0x01,                           /**< Service Partition scan length */
    0x01,                           /**< Boot flag valid bit length */
    sizeof(BootInfoAck_T),          /**< Boot info ack length */
    sizeof(BootFlags_T),            /**< Boot Flags valid length */ 
    sizeof(BootInitiatorInfo_T),    /**< Boot init info length */   
    sizeof(BootInitiatorMboxReq_T), /**< Boot init info length */   
};

static const INT8U            m_SSIBootOptParamLen [] = /**< SSI Boot Options parameter length */
{
    sizeof(OemParamBlkSizeTbl_T), /* OEM Parameter Block Size Table length */
    0x0,                          /* Reserved */
    0x0,                          /* Reserved */
    0x0,                          /* Reserved */
    0x0,                          /* Reserved */
    sizeof(BootOrderTblReq_T),    /* Boot Order Table length */
    sizeof(INT8U),                /* SSI Boot Device Selector length*/
    sizeof(SlotConfigTbl_T)       /* Slot Configuration Table length */
};

#define MAX_BOOT_PARAMS_DATA  20
typedef struct
{
    INT8U	Params;
    INT8U	ReservedBits [MAX_BOOT_PARAMS_DATA];
    INT8U	DataLen;

} BootCfgRsvdBits_T;

static BootCfgRsvdBits_T m_RsvdBitsCheck [] = {

        /* Param        Reserved Bits      Data Size   */
        { 0,	     		{ 0xFC }, 				    				 0x1 },	/* Set In progress  */
        { 2,				{ 0xFC }, 					 				 0x1 },
        { 3,				{ 0xE0 },									 0x1 },
        { 5,				{ 0x1F,0x00,0x00,0xF0,0xE0},			  	 0x5 },
        { 6,				{ 0xF0 },					 				 0x1 }
};

/*-------------------------------------
 * GetChassisCaps
 *-------------------------------------*/
int
GetChassisCaps (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    _NEAR_  GetChassisCapabilitiesRes_T*    pGetChassisCapsRes = 
        (_NEAR_ GetChassisCapabilitiesRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    
    IPMI_DBG_PRINT ("\nGET Chassis CAPABILITIES\n");

    pGetChassisCapsRes->CompletionCode = CC_NORMAL;

    
    return sizeof(GetChassisCapabilitiesRes_T);
}


/*-------------------------------------
 * GetChassisStatus
 *-------------------------------------*/
int
GetChassisStatus (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    INT8U                   PSGood=0;
    _NEAR_  GetChassisStatusRes_T*  pGetChassisStatusRes =
        (_NEAR_ GetChassisStatusRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    IPMI_DBG_PRINT ("\nGET Chassis STATUS\n");

    pGetChassisStatusRes->CompletionCode = CC_NORMAL;


    return sizeof (GetChassisStatusRes_T) ; 

}


extern xQueueHandle g_chassisCtrl_Queue;
extern xQueueHandle FTUartWrite_Queue;
/*-------------------------------------
 * ChassisControl
 *-------------------------------------*/
int
ChassisControl ( _NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst )
{
    _NEAR_  ChassisControlReq_T*    pChassisControlReq = 
        (_NEAR_  ChassisControlReq_T*) pReq;
    _NEAR_  ChassisControlRes_T*    pChassisControlRes = 
        (_NEAR_  ChassisControlRes_T*) pRes;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo;
    SamllMsgPkt_T Msg;
    BaseType_t err = pdFALSE;

    pChassisControlRes->CompletionCode = CC_NORMAL;

    /* Check for the reserved bytes should b zero */

    if  ( 0 !=  (pChassisControlReq->ChassisControl & RESERVED_BITS_CHASSISCONTROL ) )
    {
        pChassisControlRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(INT8U);	
    }

    Msg.Cmd  = pChassisControlReq->ChassisControl; // CHASSIS_CMD_CTRL
    Msg.Size = 1;
    err = xQueueSend(g_chassisCtrl_Queue, (char *)&Msg, 10);
    if (err == pdFALSE)
    {
        LOG_E("g_chassisCtrl_Queue send msg ERR!");
    }

    return sizeof (ChassisControlRes_T);
}


/*-------------------------------------
 * GetChassisIdentify
 *-------------------------------------*/
int
GetChassisIdentify (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    _NEAR_ ChassisIdentifyReq_T*    pChassisIdentifyReq =
        (_NEAR_ ChassisIdentifyReq_T*) pReq;
    _NEAR_ ChassisIdentifyRes_T*    pChassisIdentifyRes =
        (_NEAR_ ChassisIdentifyRes_T*) pRes;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo;

    IPMI_DBG_PRINT ("GET Chassis IDENTIFY\n");

    pChassisIdentifyRes->CompletionCode = CC_INV_DATA_FIELD;

    return sizeof(INT8U);
    // return sizeof(ChassisIdentifyRes_T);
}


/*-------------------------------------
 * SetChassisCaps
 *-------------------------------------*/
int
SetChassisCaps (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    _NEAR_  SetChassisCapabilitiesReq_T*    pSetChassisCapsReq =
        (_NEAR_ SetChassisCapabilitiesReq_T*) pReq;
    _NEAR_  SetChassisCapabilitiesRes_T*    pSetChassisCapsRes =
        (_NEAR_ SetChassisCapabilitiesRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    IPMI_DBG_PRINT ("SET Chassis CAPABILITIES\n");


    pSetChassisCapsRes->CompletionCode = CC_INV_DATA_FIELD;
    return sizeof(INT8U);	

    // return sizeof (SetChassisCapabilitiesRes_T);
}


/*-------------------------------------
 * SetPowerRestorePolicy
 *-------------------------------------*/
int
SetPowerRestorePolicy (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    
    _NEAR_  SetPowerRestorePolicyReq_T* pSetPowerRestorePolicyReq =
        (_NEAR_ SetPowerRestorePolicyReq_T*) pReq;
    _NEAR_  SetPowerRestorePolicyRes_T* pSetPowerRestorePolicyRes =
        (_NEAR_ SetPowerRestorePolicyRes_T*) pRes;  
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    
    IPMI_DBG_PRINT ("\nSET POWER RESORE POLICY\n");
    

    pSetPowerRestorePolicyRes->CompletionCode = CC_INV_DATA_FIELD;
    return sizeof (INT8U);

    // return  sizeof(SetPowerRestorePolicyRes_T);
}


/*-------------------------------------
 * GetSysRestartCause
 *-------------------------------------*/
int
GetSysRestartCause (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{    
    _NEAR_  GetSystemRestartCauseRes_T* pGetSysRestartCauseRes =
        (_NEAR_ GetSystemRestartCauseRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    INT8U curchannel;

    IPMI_DBG_PRINT ("GET SYSTEM RESTART CAUSE\n");

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChassisMutex, WAIT_INFINITE);
    pGetSysRestartCauseRes->CompletionCode  = CC_NORMAL;
    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    pGetSysRestartCauseRes->ChannelID       = curchannel & 0xF;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChassisMutex);

    return sizeof(GetSystemRestartCauseRes_T);
}



/*-------------------------------------
 * GetPOHCounter
 *-------------------------------------*/
int
GetPOHCounter (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    _NEAR_  GetPOHCounterRes_T* pGetPOHCounterRes = (_NEAR_ GetPOHCounterRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    IPMI_DBG_PRINT ("GET POH COUNTER\n");
    pGetPOHCounterRes->CompletionCode    = CC_NORMAL;
    pGetPOHCounterRes->MinutesPerCount   = POH_MINS_PER_COUNT;

    return sizeof(GetPOHCounterRes_T);
}


/*-------------------------------------
 * SetSysBOOTOptions
 *-------------------------------------*/
int
SetSysBOOTOptions (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    _NEAR_ SetBootOptionsReq_T* pBootOptReq = (_NEAR_ SetBootOptionsReq_T*) pReq;
    _NEAR_ SetBootOptionsRes_T* pBootOptRes = (_NEAR_ SetBootOptionsRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    INT8U                Parameter;
    BootOptions_T*       pBootOptions;
    INT8U				u8SetInProgress;
    INT8U				u8TempData,SSIComputeBladeSupport;
    int i,j=0;


    pBootOptRes->CompletionCode = CC_INV_DATA_FIELD;
    return sizeof (INT8U);

    // pBootOptRes->CompletionCode = CC_NORMAL;
    // return sizeof(SetBootOptionsRes_T);
}


/*-------------------------------------
 * GetSysBOOTOptions
 *-------------------------------------*/
int 
GetSysBOOTOptions (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    _NEAR_  GetBootOptionsReq_T* pGetBootOptReq = (_NEAR_ GetBootOptionsReq_T*) pReq;
    _NEAR_  GetBootOptionsRes_T* pGetBootOptRes = (_NEAR_ GetBootOptionsRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    INT8U                ParamSel,SSIComputeBladeSupport;
    int                  ResponseLength;
    BootOptions_T        sBootOptions;


    IPMI_DBG_PRINT ("GET SYSTEM BOOT OPTIONS\n");


    /* Alarm !!! Somebody is trying to set Reseved Bits */
    *pRes = CC_INV_DATA_FIELD;
    return sizeof (*pRes);
}

/*-------------------------------------
 * SetFPButtonEnables
 *-------------------------------------*/
int
SetFPButtonEnables (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
#if 0 //FQLI
    _FAR_   PMConfig_T*          pPMConfig;
    bool                         bRet;
    SetFPBtnEnablesRes_T*        pFPBtnEnablesRes = (SetFPBtnEnablesRes_T*)pRes;
    SetFPBtnEnablesReq_T*   pFPBtnEnablesReq = (SetFPBtnEnablesReq_T*)pReq;

    IPMI_DBG_PRINT ("PwrCtrl - SetFrontPanelEnables\n");
    pFPBtnEnablesRes->CompletionCode = CC_INV_CMD;

    if (pReq)
    {
        pFPBtnEnablesRes->CompletionCode = CC_NORMAL;

        // retrieve chassis status from the NVStore
        pPMConfig = (_FAR_ PMConfig_T*)GetNVRAddr(NVRH_PMCONFIG);

        // store the front panel enables to the NVStore
        pPMConfig->ChassisConfig.ChassisPowerState.FPBtnEnables = pFPBtnEnablesReq->ButtonEnables;

        // enable / disable front panel buttons. do the set before store the value in case the
        // action fails. if failed to set the passthrough buttons, the current reading will be
        // used to be the current settings.
        bRet = PDK_SetFPEnable(&pFPBtnEnablesReq->ButtonEnables);
        if (bRet != TRUE)
        {
            pFPBtnEnablesRes->CompletionCode = CC_UNSPECIFIED_ERR;
        }

      	FlushPMC(&pPMConfig->ChassisConfig.ChassisPowerState.sFrontPanelButton, sizeof(ChassisPowerState_T));
    }

    return sizeof (SetFPBtnEnablesRes_T);

#else //AMI

    _NEAR_  SetFPBtnEnablesReq_T* pFPBtnEnablesReq = (_NEAR_ SetFPBtnEnablesReq_T*)pReq;
    _NEAR_  SetFPBtnEnablesRes_T* pFPBtnEnablesRes = (_NEAR_ SetFPBtnEnablesRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    IPMI_DBG_PRINT ("Set FP Button Enables\n");

    /* Check for the reserved bytes should b zero */
    pFPBtnEnablesRes->CompletionCode = CC_INV_DATA_FIELD;
    return sizeof(INT8U);	

#endif //FQLI
}


/*-------------------------------------
 * SetPowerCycleInterval
 *-------------------------------------*/
int
SetPowerCycleInterval (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{    
    _NEAR_  SetPowerCycleIntervalReq_T* pSetPowerCycleInterval =
        (_NEAR_ SetPowerCycleIntervalReq_T*) pReq;
    _NEAR_  SetPowerCycleIntervalRes_T* pSetPowerCycleIntervalRes =
        (_NEAR_ SetPowerCycleIntervalRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    pSetPowerCycleIntervalRes->CompletionCode  = CC_NORMAL;

    return sizeof(SetPowerCycleIntervalRes_T);
}

#endif /* CHASSIS_DEVICE */
