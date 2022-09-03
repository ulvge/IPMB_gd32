/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2002-2012, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200, Norcross,        **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * HPMCmds.c
 * HPM Command Handler
 *
 * Author: Joey Chen <joeychen@ami.com.tw>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS     0

#include "Types.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "Debug.h"
#include "HPMFuncs.h"
#include "IPMI_HPM.h"
#include "IPMI_HPMCmds.h"
#include "IPMI_IPM.h"
#include "HPMConfig.h"
#include "IPMDevice.h"
#include "IPMIConf.h"
#include "Ethaddr.h"
#include "flashlib.h"
#include "flshfiles.h"

static BOOL first_time = TRUE;
static INT8U ExpectBlockNum = 0;
static BOOL FirstBlockUploaded = TRUE;
extern int CheckRollBackFirmwareExist(char *RunningImage);

    
static void 
InitBlkVar(void)
{
    ExpectBlockNum = 0;
    FirstBlockUploaded = FALSE;
}

#if PICMG_DEVICE == 1

#if GET_TARGET_UPLD_CAPABLITIES != UNIMPLEMENTED
/**
 * @brief Target upgrade properties command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
GetTargetUpgradeCapablities (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
    _NEAR_ GetTargetUpgradeCapablitiesRes_T*  pGetTargetUpgCapRes = (_NEAR_ GetTargetUpgradeCapablitiesRes_T*) pRes;
    
    pGetTargetUpgCapRes->CompletionCode        = CC_NORMAL;
    pGetTargetUpgCapRes->Identifier            = PICMG_IDENTIFIER;
    pGetTargetUpgCapRes->HPM1Ver               = HPM1_REV;
	
    if(first_time == TRUE)
    {
        ReadAllHPMConf();
        first_time = FALSE;
    }
    
    HPMConfTargetCap_T TargetCapConf;
    GetTargetCapConf(&TargetCapConf);
    
    pGetTargetUpgCapRes->IPMCGlobalCapablities = TargetCapConf.GlobalCap.ByteField;
    
    /* Timeout values (all in 5sec intervals), 0 if not supported */
    pGetTargetUpgCapRes->UpgradeTimeout        = TargetCapConf.UpgradeTimeout;
    pGetTargetUpgCapRes->SelfTestTimeout       = TargetCapConf.SelfTestTimeout;
    pGetTargetUpgCapRes->Rollbackimeout        = TargetCapConf.Rollbackimeout;
    pGetTargetUpgCapRes->InaccessiblityTimeout = TargetCapConf.InaccessiblityTimeout;
    pGetTargetUpgCapRes->ComponentsPresent     = TargetCapConf.ComponentsPresent;
    
    if(!IsFwUpSupportIfc(BMCInst))
    {
        pGetTargetUpgCapRes->CompletionCode	= CC_UPG_NOT_SUPPORTED_OVER_IFC;	
    }
        
    return sizeof(GetTargetUpgradeCapablitiesRes_T);    
}
#endif /* GET_TARGET_UPLD_CAPABLITIES != UNIMPLEMENTED */

#if GET_COMPONENT_PROPERTIES != UNIMPLEMENTED
/**
 * @brief Get component properties command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
GetComponentProperties (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
    _NEAR_ GetComponentPropertiesReq_T* pGetCompPropReq = (_NEAR_ GetComponentPropertiesReq_T*) pReq;
    _NEAR_ GetCompPropGeneralRes_T* pGeneralRes  = (_NEAR_ GetCompPropGeneralRes_T*) pRes;
    _NEAR_ GetCompPropFWRes_T*      pFWRes       = (_NEAR_ GetCompPropFWRes_T*)      pRes;
    _NEAR_ GetCompPropDescStrRes_T* pDescStrRes  = (_NEAR_ GetCompPropDescStrRes_T*) pRes;
	int RetVal = 0;
    INT8U	ResLen = 0;
    static FirmwareVersion_T CurrFwVersion[MAX_COMPONENTS], RBFwVersion[MAX_COMPONENTS], DeferFwVersion[MAX_COMPONENTS];
    char DescString[DESC_STRING_LEN];
            
    if (!IsValidComponentID(pGetCompPropReq->ComponentID))
    {
        pGeneralRes->CompletionCode = CC_INV_COMPONENT_ID;   
        IPMI_DBG_PRINT("HPMCmds.c : Invalid Component.\n");
		return sizeof (INT8U);
    }
        
    pGeneralRes->CompletionCode = CC_NORMAL;
    pGeneralRes->Identifier = PICMG_IDENTIFIER;

    if(!IsFwUpSupportIfc(BMCInst))
    {
        pGeneralRes->CompletionCode	= CC_UPG_NOT_SUPPORTED_OVER_IFC;	
    }
    
    if(first_time == TRUE)
    {
        ReadAllHPMConf();
        first_time = FALSE;
	}
    
    HPMConfCompProp_T CompPropConf;
 
    switch (pGetCompPropReq->ComponentPropSelector)
	{
		case GENERAL_COMPONENT_PROPERTIES:
            GetCompPropConf(&CompPropConf, pGetCompPropReq->ComponentID);  
	        pGeneralRes->GeneralCompProp = CompPropConf.GeneralCompProp.ByteField;
	        ResLen = sizeof(GetCompPropGeneralRes_T);
			break;

		case CURRENT_VERSION:
            if(g_PDKHandle[PDK_HPMGETCURRENTFWVERSION] != NULL)
            {
                ((void(*)(INT8U *, INT8U *, INT32U *)) g_PDKHandle[PDK_HPMGETCURRENTFWVERSION] ) 
                        (&CurrFwVersion[pGetCompPropReq->ComponentID].FWRev1, &CurrFwVersion[pGetCompPropReq->ComponentID].FWRev2, &CurrFwVersion[pGetCompPropReq->ComponentID].AuxillaryFWRevInfo);
            } 
            if(!IsFwVersionCached(pGetCompPropReq->ComponentID, FWVER_TYPE_CURRENT))
            {
                GetCurrFirmwareVersion(pGetCompPropReq->ComponentID, &CurrFwVersion[pGetCompPropReq->ComponentID]);
                CacheFwVersion(pGetCompPropReq->ComponentID, FWVER_TYPE_CURRENT);
            }
            pFWRes->CurrFW.FWRev1          		= CurrFwVersion[pGetCompPropReq->ComponentID].FWRev1;
			pFWRes->CurrFW.FWRev2          		= CurrFwVersion[pGetCompPropReq->ComponentID].FWRev2;
			pFWRes->CurrFW.AuxillaryFWRevInfo	= CurrFwVersion[pGetCompPropReq->ComponentID].AuxillaryFWRevInfo;
			ResLen = sizeof(GetCompPropFWRes_T);
			break;
			
		case DESCRIPTION_STRING:
	        GetCompDescString(pGetCompPropReq->ComponentID, DescString, DESC_STRING_LEN);
            strncpy((char *)pDescStrRes->DescString, DescString, DESC_STRING_LEN);
	        ResLen = sizeof(GetCompPropDescStrRes_T);
			break;
            
		case ROLLBACK_FW_VERSION:
            if( (!IsAutoRollbackSupport(pGetCompPropReq->ComponentID)) && 
                (!IsManualRollbackSupport(pGetCompPropReq->ComponentID)) )
            {
                pGeneralRes->CompletionCode = CC_INV_COMPONENT_PROP_SELECTOR;
                ResLen = sizeof (INT8U);
                return (sizeof (INT8U));
            }
            if(g_PDKHandle[PDK_HPMGETROLLBACKFWVERSION] != NULL)
            {
                ((void(*)(INT8U *, INT8U *, INT32U *)) g_PDKHandle[PDK_HPMGETROLLBACKFWVERSION] ) 
                        (&RBFwVersion[pGetCompPropReq->ComponentID].FWRev1, &RBFwVersion[pGetCompPropReq->ComponentID].FWRev2, &RBFwVersion[pGetCompPropReq->ComponentID].AuxillaryFWRevInfo);
            }
            if(!IsFwVersionCached(pGetCompPropReq->ComponentID, FWVER_TYPE_ROLLBACK))
            {
                RetVal = GetRollbackFirmwareVersion(pGetCompPropReq->ComponentID, &RBFwVersion[pGetCompPropReq->ComponentID]);
                if(RetVal < 0)
                {
                    pGeneralRes->CompletionCode = CC_INV_COMPONENT_PROP_SELECTOR;
                    return (sizeof (INT8U));
                }
                CacheFwVersion(pGetCompPropReq->ComponentID, FWVER_TYPE_ROLLBACK);
            }
            pFWRes->CurrFW.FWRev1          		= RBFwVersion[pGetCompPropReq->ComponentID].FWRev1;
			pFWRes->CurrFW.FWRev2          		= RBFwVersion[pGetCompPropReq->ComponentID].FWRev2;
			pFWRes->CurrFW.AuxillaryFWRevInfo	= RBFwVersion[pGetCompPropReq->ComponentID].AuxillaryFWRevInfo;
			ResLen = sizeof(GetCompPropFWRes_T);
            break;
            
        case DEFERRED_UPG_FW_VERSION:
            if(g_PDKHandle[PDK_HPMGETDEFERREDFWVERSION] != NULL)
            {
                ((void(*)(INT8U *, INT8U *, INT32U *)) g_PDKHandle[PDK_HPMGETDEFERREDFWVERSION] ) 
                        (&DeferFwVersion[pGetCompPropReq->ComponentID].FWRev1, &DeferFwVersion[pGetCompPropReq->ComponentID].FWRev2, &DeferFwVersion[pGetCompPropReq->ComponentID].AuxillaryFWRevInfo);
            }        
            if(!IsFwVersionCached(pGetCompPropReq->ComponentID, FWVER_TYPE_DEFER))
            {
                RetVal = GetDeferUpgFirmwareVersion(pGetCompPropReq->ComponentID, &DeferFwVersion[pGetCompPropReq->ComponentID]);
                if(RetVal < 0)
                {
                    pGeneralRes->CompletionCode = CC_INV_COMPONENT_PROP_SELECTOR;
                    return (sizeof (INT8U));
                }
                CacheFwVersion(pGetCompPropReq->ComponentID, FWVER_TYPE_DEFER);
            }
	        pFWRes->CurrFW.FWRev1          		= DeferFwVersion[pGetCompPropReq->ComponentID].FWRev1;
			pFWRes->CurrFW.FWRev2          		= DeferFwVersion[pGetCompPropReq->ComponentID].FWRev2;
			pFWRes->CurrFW.AuxillaryFWRevInfo	= DeferFwVersion[pGetCompPropReq->ComponentID].AuxillaryFWRevInfo;
			ResLen = sizeof(GetCompPropFWRes_T);
            break;
               
		default:
	        pGeneralRes->CompletionCode = CC_INV_COMPONENT_PROP_SELECTOR;
			ResLen = sizeof (INT8U);
            break;
	}

    return ResLen;
}
#endif /* GET_COMPONENT_PROPERTIES != UNIMPLEMENTED */



#if INITIATE_UPGRADE_ACTION != UNIMPLEMENTED
/**
 * @brief Initiate Upgrade Action command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
InitiateUpgradeAction (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
    _NEAR_ InitiateUpgActionReq_T* pInitiateUpgActionReq = (_NEAR_ InitiateUpgActionReq_T*) pReq;
    _NEAR_ InitiateUpgActionRes_T* pInitiateUpgActionRes = (_NEAR_ InitiateUpgActionRes_T*) pRes;
    
    pInitiateUpgActionRes->CompletionCode = CC_CMD_INPROGRESS;
    pInitiateUpgActionRes->Identifier      = PICMG_IDENTIFIER;
 
    if(first_time == TRUE)
    {
        ReadAllHPMConf();
        first_time = FALSE;
    }
       
    if (!IsValidComponents(pInitiateUpgActionReq->Components))
    {
        pInitiateUpgActionRes->CompletionCode = CC_INV_COMPONENT;   
        IPMI_DBG_PRINT("HPMCmds.c : Invalid Component.\n");
        UpdateHPMStatus(CC_INV_COMPONENT, CMD_INITIATE_UPG_ACTION, HPM_LONG_DURATION_CMD);
		return sizeof(InitiateUpgActionRes_T);
    }
    
    UpdateHPMStatus(CC_CMD_INPROGRESS, CMD_INITIATE_UPG_ACTION, HPM_LONG_DURATION_CMD);
    
    switch (pInitiateUpgActionReq->UpgradeAction)
    {
        case INIT_BACKUP_COMPONENTS:
            if(!IsBackupComponentSupport(pInitiateUpgActionReq->Components))
            {
                pInitiateUpgActionRes->CompletionCode = CC_INV_DATA_FIELD;
                UpdateHPMStatus(CC_INV_DATA_FIELD, CMD_INITIATE_UPG_ACTION, HPM_LONG_DURATION_CMD);
                return sizeof(InitiateUpgActionRes_T);
            }	     
            UpdateHPMStatus(CC_CMD_INPROGRESS, CMD_INITIATE_UPG_ACTION, HPM_LONG_DURATION_CMD);
            InitBackupComponents(pInitiateUpgActionReq->Components);
            break;
        case INIT_PREPARE_COMPONENTS:	 
            /* we will received all components mask here */
            InitPreComponents(pInitiateUpgActionReq->Components);
            pInitiateUpgActionRes->CompletionCode = CC_NORMAL;
            UpdateHPMStatus(CC_NORMAL, CMD_INITIATE_UPG_ACTION, HPM_LONG_DURATION_CMD);
	     break;            
        case INIT_UPLOAD_FOR_UPGRADE:
        case INIT_UPLOAD_FOR_COMPARE:	     
            /* only one commponent at one times, we know which one will be upgraded from now on. */           
            if(!IsOnlyOneComponent(pInitiateUpgActionReq->Components))
            {
                pInitiateUpgActionRes->CompletionCode = CC_INV_COMPONENT;
                UpdateHPMStatus(CC_INV_COMPONENT, CMD_INITIATE_UPG_ACTION, HPM_LONG_DURATION_CMD);
            }
            else
                InitUpload(pInitiateUpgActionReq->UpgradeAction, pInitiateUpgActionReq->Components);
            InitBlkVar();
            break;
         
        default:
            pInitiateUpgActionRes->CompletionCode = CC_INV_DATA_FIELD;
            UpdateHPMStatus(CC_INV_DATA_FIELD, CMD_INITIATE_UPG_ACTION, HPM_LONG_DURATION_CMD);
            break;
    }
    
    ClearAllCachedFwVersion();
    
    return sizeof(InitiateUpgActionRes_T);
}
#endif /* INITIATE_UPGRADE_ACTION != UNIMPLEMENTED */

#if IPM_DEVICE == 1
#if QUERY_SELF_TEST_RESULTS != UNIMPLEMENTED
/*---------------------------------------
 * QuerySelfTestResults
 *---------------------------------------*/
int
QuerySelfTestResults (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
	_NEAR_ QuerySelfTestResultsRes_T* 	pQuerySelfTestRes = (_NEAR_ QuerySelfTestResultsRes_T*) pRes;

    pQuerySelfTestRes->CompletionCode  	= CC_NORMAL;
	pQuerySelfTestRes->Identifier		= PICMG_IDENTIFIER;
    
    pQuerySelfTestRes->SelfTestResult1 = GetSelfTestResultByte(1, BMCInst);
    pQuerySelfTestRes->SelfTestResult2 = GetSelfTestResultByte(2, BMCInst);

    if(!IsFwUpSupportIfc(BMCInst))
    {
        pQuerySelfTestRes->CompletionCode	= CC_UPG_NOT_SUPPORTED_OVER_IFC;
	return sizeof (QuerySelfTestResultsRes_T);
    }

    /* if self test fialed */
   	if(pQuerySelfTestRes->SelfTestResult1 != GST_NO_ERROR)
   	{   	
    		HPMConfTargetCap_T TargetCapConf;
    
   	  
       	 if(first_time == TRUE)
    		{
        		ReadAllHPMConf();
        		first_time = FALSE;
    		}
            
		GetTargetCapConf(&TargetCapConf);		

		/*check if autorollback support */
		if (TargetCapConf.GlobalCap.BitField.AutoRollback==1)
		{ 
		        INT8U ActivatedComponents = GetHPMActCompsFromBootParam();
				
			if(ActivatedComponents!=0)
			{
				
        			HandleManualRollback();
        			ClearAllCachedFwVersion();
			}
		} 
   	}
	
    return sizeof (QuerySelfTestResultsRes_T);
}
#endif /* QUERY_SELF_TEST_RESULTS != UNIMPLEMENTED */
#endif /* IPM DEVICE */

#if ABORT_FIRMWARE_UPGRADE != UNIMPLEMENTED
/**
 * @brief Abort Firmware upgrade command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
AbortFirmwareUpgrade (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
    _NEAR_ AbortFirmwareUpgradeRes_T* pAbortFWUpgradeRes = (_NEAR_ AbortFirmwareUpgradeRes_T*) pRes;

    pAbortFWUpgradeRes->Identifier = PICMG_IDENTIFIER;
    pAbortFWUpgradeRes->CompletionCode = HandleAbortFirmwareUpgrade();
    
    if(pAbortFWUpgradeRes->CompletionCode != CC_UPG_NOT_ABORTED_AT_THIS_MOMENT)
    {
        InitBlkVar();
        UpdateHPMStatus(pAbortFWUpgradeRes->CompletionCode, CMD_ABORT_FIRMWARE_UPGRADE, HPM_SHORT_DURATION_CMD);
    }
    
    return sizeof(AbortFirmwareUpgradeRes_T);
}
#endif /* ABORT_FIRMWARE_UPGRADE != UNIMPLEMENTED */

#if UPLOAD_FIRMWARE_BLOCK != UNIMPLEMENTED
/**
 * @brief Upload firmware block command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
UploadFirmwareBlock (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
  
    _NEAR_ UploadFirmwareBlkReq_T* pUploadFWBlkReq = (_NEAR_ UploadFirmwareBlkReq_T*) pReq;
    _NEAR_ UploadFirmwareBlkRes_T* pUploadFWBlkRes = (_NEAR_ UploadFirmwareBlkRes_T*) pRes;
    INT8U DataLen = 0;
    int RetVal = 0;
    
    DataLen = ReqLen - 2; /* minus Identify and Block number bytes */
    
    pUploadFWBlkRes->CompletionCode = CC_CMD_INPROGRESS;
    pUploadFWBlkRes->Identifier = PICMG_IDENTIFIER;

    /*reset timer every time the function get called*/ 
    SetHPMFlashStatus(HPM_UPLOAD_BLK_INPROGRESS);
    SetHPMTimerCnt(HPM_UPLOAD_BLK_TIMER_COUNT);
    
    if(FirstBlockUploaded == FALSE)
    {
        /* check state */
        if(!IsLastHPMCmdCorrect(CMD_INITIATE_UPG_ACTION))
        {
            pUploadFWBlkRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
            UpdateHPMStatus(CC_PARAM_NOT_SUP_IN_CUR_STATE, CMD_UPLOAD_FIRMWARE_BLOCK, HPM_LONG_DURATION_CMD);
            return sizeof(UploadFirmwareBlkRes_T);  
        }

        if((ExpectBlockNum == 0) && (pUploadFWBlkReq->BlkNumber == 0))
        {
            FirstBlockUploaded = TRUE;
	    /*we create timer thread when receive first blk*/		
	     OS_CREATE_THREAD(HPMTimerTask, NULL, NULL);		
        }            
    }
    
    UpdateHPMStatus(CC_CMD_INPROGRESS, CMD_UPLOAD_FIRMWARE_BLOCK, HPM_LONG_DURATION_CMD);
        
    if(pUploadFWBlkReq->BlkNumber != ExpectBlockNum)
    {
        TDBG("out of order, Expect block number is [%d] and received block number is [%d]\n", ExpectBlockNum, pUploadFWBlkReq->BlkNumber);
        pUploadFWBlkRes->CompletionCode = CC_INV_DATA_FIELD; 
        UpdateHPMStatus(CC_INV_DATA_FIELD, CMD_UPLOAD_FIRMWARE_BLOCK, HPM_LONG_DURATION_CMD);
        return sizeof(UploadFirmwareBlkRes_T);        
    }
    
    if((!DataLen) && (!FirstBlockUploaded))
    {
        pUploadFWBlkRes->CompletionCode = CC_INV_DATA_FIELD; 
        UpdateHPMStatus(CC_INV_DATA_FIELD, CMD_UPLOAD_FIRMWARE_BLOCK, HPM_LONG_DURATION_CMD);
        return sizeof(UploadFirmwareBlkRes_T);
    } 
    else
        RetVal = HandleFirmwareBlock(pUploadFWBlkReq->BlkNumber, pUploadFWBlkReq->FWData, DataLen);
    
    if(RetVal < 0)
    {
        pUploadFWBlkRes->CompletionCode = CC_INV_FW_CHECKSUM;
        InitBlkVar();
    }
    else
    {
        pUploadFWBlkRes->CompletionCode = CC_NORMAL;
        /* increase ExpectBlockNum */        
        if(ExpectBlockNum == 0xFF)
            ExpectBlockNum = 0;
        else    
            ExpectBlockNum++;
    }
  
    UpdateHPMStatus(pUploadFWBlkRes->CompletionCode, CMD_UPLOAD_FIRMWARE_BLOCK, HPM_LONG_DURATION_CMD);
    
    return sizeof(UploadFirmwareBlkRes_T);
}
#endif /* UPLOAD_FIRMWARE_BLOCK != UNIMPLEMENTED */

#if FINISH_FIRMWARE_UPLOAD != UNIMPLEMENTED
/**
 * @brief Finish Firmware Upload command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
FinishFirmwareUpload (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{

    _NEAR_ FinishFWUploadReq_T* pFinishFWUploadReq = (_NEAR_ FinishFWUploadReq_T*) pReq;
    _NEAR_ FinishFWUploadRes_T* pFinishFWUploadRes = (_NEAR_ FinishFWUploadRes_T*) pRes;
            
    pFinishFWUploadRes->CompletionCode = CC_CMD_INPROGRESS;
    pFinishFWUploadRes->Identifier = PICMG_IDENTIFIER;
    
    InitBlkVar();
    
    if(!IsLastHPMCmdCorrect(CMD_UPLOAD_FIRMWARE_BLOCK))
    {
        pFinishFWUploadRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
        UpdateHPMStatus(CC_PARAM_NOT_SUP_IN_CUR_STATE, CMD_FINISH_FIRMWARE_UPLOAD, HPM_LONG_DURATION_CMD);
        return sizeof(FinishFWUploadRes_T);    
    }
        
    if(!VerifyImageLength(pFinishFWUploadReq->ImageSize))
    {
        RevertToInitState(pFinishFWUploadReq->Component);
        pFinishFWUploadRes->CompletionCode = CC_IMAGE_LEN_MISMATCH;
        UpdateHPMStatus(CC_IMAGE_LEN_MISMATCH, CMD_FINISH_FIRMWARE_UPLOAD, HPM_LONG_DURATION_CMD);
        return sizeof(FinishFWUploadRes_T);
    }
        
    UpdateHPMStatus(CC_CMD_INPROGRESS, CMD_FINISH_FIRMWARE_UPLOAD, HPM_LONG_DURATION_CMD);

    if(!IsCachedComponentID(pFinishFWUploadReq->Component))
    {
        pFinishFWUploadRes->CompletionCode = CC_INV_DATA_FIELD;
        UpdateHPMStatus(CC_INV_DATA_FIELD, CMD_FINISH_FIRMWARE_UPLOAD, HPM_LONG_DURATION_CMD);
        return sizeof(FinishFWUploadRes_T);
    }

    HandleUploadedFirmware(); 

    ClearAllCachedFwVersion();
    
    return sizeof(FinishFWUploadRes_T);
}
#endif /* FINISH_FIRMWARE_UPLOAD != UNIMPLEMENTED */

#if GET_UPGRADE_STATUS != UNIMPLEMENTED
/**
 * @brief Get upgrade status command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
GetUpgradeStatus (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
    _NEAR_ GetUpgradeStatusRes_T*  pGetUpgradeStatus = (_NEAR_ GetUpgradeStatusRes_T*) pRes;
    HPMCmdStatus_T HPMCmdStatus;

    GetHPMStatus(&HPMCmdStatus);
    
    /* if Command in Progress is long Duration command */
    if (HPM_LONG_DURATION_CMD == HPMCmdStatus.CmdDuration)
    {
        pGetUpgradeStatus->CmdInProgress = HPMCmdStatus.CmdInProgress;
        pGetUpgradeStatus->LastCmdCC = HPMCmdStatus.CmdCC;
    }
    else /* command in progress is this command */
    {
        pGetUpgradeStatus->CmdInProgress = CMD_GET_UPGRADE_STATUS;
        pGetUpgradeStatus->LastCmdCC = CC_NORMAL;
    }
    
    pGetUpgradeStatus->Identifier       = PICMG_IDENTIFIER;
    pGetUpgradeStatus->CompletionCode   = CC_NORMAL;
    
    return sizeof(GetUpgradeStatusRes_T);
}
#endif /* GET_UPGRADE_STATUS != UNIMPLEMENTED */

#if ACTIVATE_FIRMWARE != UNIMPLEMENTED
/**
 * @brief Activate Firmware command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
ActivateFirmware (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
    _NEAR_ ActivateFWRes_T* pActivateFWRes = (_NEAR_ ActivateFWRes_T*) pRes;
   
    UpdateHPMStatus(CC_CMD_INPROGRESS, CMD_ACTIVATE_FIRMWARE, HPM_LONG_DURATION_CMD);
    
	pActivateFWRes->CompletionCode = CC_CMD_INPROGRESS;
    pActivateFWRes->Identifier = PICMG_IDENTIFIER;
    
    if(FALSE == HandleActivateFirmware())
    {
        pActivateFWRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
        UpdateHPMStatus(CC_PARAM_NOT_SUP_IN_CUR_STATE, CMD_ACTIVATE_FIRMWARE, HPM_LONG_DURATION_CMD);
    }
  
    return sizeof(ActivateFWRes_T);
}
#endif /* ACTIVATE_FIRMWARE != UNIMPLEMENTED */

#if QUERY_ROLLBACK_STATUS != UNIMPLEMENTED
/**
 * @brief Query Rollback Status command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
QueryRollbackStatus (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
    _NEAR_ QueryRollbackStatusRes_T* pQueryRollbackStatusRes = (_NEAR_ QueryRollbackStatusRes_T*) pRes;

    pQueryRollbackStatusRes->Identifier = PICMG_IDENTIFIER;
    pQueryRollbackStatusRes->RollbackStatus = 0;
    HPMCmdStatus_T HPMCmdStatus;
    
    if((!IsAutoRollbackSupport(INVALID_COMPONENT_ID)) && (!IsManualRollbackSupport(INVALID_COMPONENT_ID)))
    {
        pQueryRollbackStatusRes->CompletionCode = CC_INV_CMD;
        return (sizeof (INT8U));
    }
   
    GetHPMStatus(&HPMCmdStatus);
    /* if Command in Progress is long Duration command */
    if ((HPM_LONG_DURATION_CMD == HPMCmdStatus.CmdDuration) &&
        (CMD_INITIATE_MANUAL_ROLLBACK == HPMCmdStatus.CmdInProgress))
    {

        pQueryRollbackStatusRes->CompletionCode = HPMCmdStatus.CmdCC;
        if(CC_CMD_INPROGRESS == HPMCmdStatus.CmdCC)
        {    
            pQueryRollbackStatusRes->RollbackStatus = 0x00;
        }
        else
        {
            pQueryRollbackStatusRes->RollbackStatus = GetRollbackComponents();
        }
    }
    else /* command in progress is this command */
    {
        pQueryRollbackStatusRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
    }
    
    return sizeof(QueryRollbackStatusRes_T);
}
#endif /* QUERY_ROLLBACK_STATUS != UNIMPLEMENTED */

#if INITIATE_MANUAL_ROLLBACK != UNIMPLEMENTED
/**
 * @brief Initiate Manual Rollback command.
 * @param Req - Request message.
 * @param ReqLen - Request length.
 * @param Res - Response message.
 * @return The response length.
**/
int
InitiateManualRollback (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, _NEAR_ int BMCInst)
{
    _NEAR_ InitiateManualRollbackRes_T* pInitiateManualRollbackRes = (_NEAR_ InitiateManualRollbackRes_T*) pRes;
    char RunningImage = HPM_IMAGE_1;

    pInitiateManualRollbackRes->Identifier = PICMG_IDENTIFIER;
    
    if(first_time == TRUE)
    {
        ReadAllHPMConf();
        first_time = FALSE;
    }
    
    if(!IsManualRollbackSupport(INVALID_COMPONENT_ID))
    {
        pInitiateManualRollbackRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
        UpdateHPMStatus(CC_PARAM_NOT_SUP_IN_CUR_STATE, CMD_INITIATE_MANUAL_ROLLBACK, HPM_LONG_DURATION_CMD);
        return sizeof(InitiateManualRollbackRes_T);
    }
    if (-1 == CheckRollBackFirmwareExist(&RunningImage))
    {
        pInitiateManualRollbackRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
        return sizeof(InitiateManualRollbackRes_T);
        
    }
    
	pInitiateManualRollbackRes->CompletionCode = CC_CMD_INPROGRESS;
   
    
    SetHPMFlashStatus(HPM_FLASH_FW_INPROGRESS);    
    /*Create timer thread*/		
    OS_CREATE_THREAD(HPMTimerTask, NULL, NULL);		
    HandleManualRollback();
    
    ClearAllCachedFwVersion();
    return sizeof(InitiateManualRollbackRes_T);
}
#endif /* INITIATE_MANUAL_ROLLBACK != UNIMPLEMENTED */

#endif	/* PICMG DEVICE */

