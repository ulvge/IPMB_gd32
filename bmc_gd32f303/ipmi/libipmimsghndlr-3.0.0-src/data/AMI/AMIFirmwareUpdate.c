/*******************************************************************
********************************************************************
****                                                              **
****    (C)Copyright 2013, American Megatrends Inc.               **
****                                                              **
****    All Rights Reserved.                                      **
****                                                              **
****    5555 , Oakbrook Pkwy, Norcross,                           **
****                                                              **
****    Georgia - 30093, USA. Phone-(770)-246-8600.               **
****                                                              **
********************************************************************
********************************************************************
********************************************************************
 *
 * AMIDataTransfer.c
 *
 * Author: Abhitesh <abhiteshk@amiindia.co.in>
 * 		   Valantina  A <valantinaa@amiindia.co.in> 
 *
******************************************************************/
#include "Support.h"
#include "AMIFirmwareUpdate.h"
#include "IPMI_AMIFirmwareUpdate.h" 
#include "IPMI_AMIDevice.h"
#include "AMIDevice.h"
#include "dbgout.h"
#include "IPMIDefs.h"
#include "PDKAccess.h"
#include "IPMIConf.h"
#include "safesystem.h"
#include "UnifiedFwUpdate.h"
#include <dlfcn.h>
#include <pthread.h>


static const INT8U m_SubCommandLen [] = {1,2,38,1,3,3,1};

#define SUCCESS     0
#define FAILURE     -1

#define STR_LEN_8   8
#define STR_LEN_16  16
#define STR_LEN_20  20
#define STR_LEN_32  32
#define STR_LEN_64  64

#define  DEVICE_IDEN_LEN 22
#define SET_NETWORK_SHARE_CONFIG_MIN_REQ_LEN 229


#if 0
int FillResponseData(ImageInfo *pImageInfo, INT8U *resData, int *RespLen)
// int FillResponseData(int *pImageInfo, INT8U *resData, int *RespLen)
{
    int offset=0;

    if(NULL == pImageInfo)
    {
        TCRIT("pImageInfo is null \n");
        return FAILURE;
    }

    resData[offset] = pImageInfo->edevicetype ;
    offset +=1;
    memcpy(resData+offset,pImageInfo->deviceName,STR_LEN_16);
    offset +=STR_LEN_16;
    memcpy(resData+offset,pImageInfo->deviceID,STR_LEN_16);
    offset +=STR_LEN_16;
#if 1
    memcpy(resData+offset,pImageInfo->vendorID,STR_LEN_16);
    offset +=STR_LEN_16;
    memcpy(resData+offset,pImageInfo->subfuncID,STR_LEN_16);
    offset +=STR_LEN_16;
    pImageInfo->fruID = resData[offset];
    offset ++;
    memcpy(resData+offset,pImageInfo->imageName,STR_LEN_16);
    offset +=STR_LEN_16;
    pImageInfo->imageFlashOrder = resData[offset];
    offset ++;
    memcpy(resData+offset,pImageInfo->releaseNoteFileName,STR_LEN_16);
    offset +=STR_LEN_16;
    memcpy(resData+offset,pImageInfo->modelNumber,STR_LEN_32);
    offset +=STR_LEN_32;
    memcpy(resData+offset,pImageInfo->subDeviceID,STR_LEN_16);
    offset +=STR_LEN_16;
    memcpy(resData+offset,pImageInfo->subVenderID,STR_LEN_16);
    offset +=STR_LEN_16;
    memcpy(resData+offset,pImageInfo->platform,STR_LEN_16);
    offset +=STR_LEN_16;
#endif
    (*RespLen) = offset;
    return SUCCESS;
}
#endif


int AMIFirmwareCommand (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
    AMIFWCommandReq_T *pAMIFWCommandReq  = (AMIFWCommandReq_T* ) pReq;
    AMIFWCommandRes_T *pAMIFWCommandRes  = (AMIFWCommandRes_T*) pRes;
    int offset=0,status = FAILURE;
    int RespLen=0;
    int nRet = FAILURE;
    INT8U ListData[256];
    INT8U FWCount=0;
    INT8U ResLen=0;
    INT8U CancelStatus=1;
    INT32U RandNumber = 0, UniqueID = 0, NewUID = 0;
    INT8U bundlepath[STR_LEN_64] = {0};
    INT8U keypath[STR_LEN_32] = {0};
    INT8U bundlename[STR_LEN_16] = {0};
    offset = 0;
    char *pDevName = NULL;
    int (*GetUpdateCompforBIOS)(INT8U*, INT8U*, INT8U*) = NULL;
    int (*SetCompUpdateStatusByBIOS)(INT8U*) = NULL;
    int (*ReadImageInfo)(INT8U* ,INT32U , INT8U*) = NULL;
    int (*SetCompUpdate)(INT8U* ,INT32U , INT8U*) = NULL;
    int (*GetCompUpdateStatus)(INT8U* ,INT32U , INT8U*) = NULL;
    int (*SetNetworkShareConfig)(INT8U* ,INT32U , INT8U*) = NULL;
    int (*GetNetworkShareConfig)(INT8U* ,INT32U , INT8U*) = NULL;
    int (*GetNetworkShareOperationStatus)(INT8U* ,INT32U , INT8U*) = NULL;
    int (*NetworkShareOperation)(INT8U* ,INT32U , INT8U*) = NULL;
    int (*CancelComponentUpdate)(INT8U *, INT32U) = NULL;
    int (*ValidateBundle)(INT8U* , INT8U*) = NULL;
    int (*FirmwareUpdateMode) (int, INT32U*) = NULL;
    int (*RearmFirmwareTimer)(INT32U, INT32U*) = NULL;

    if(g_corefeatures.unified_firmware_update != ENABLED)
    {
        *pRes = CC_INV_CMD;
        return sizeof(INT8U);
    }

    /*Check the Firmware update Mode*/
    if(UnifiedUpdateMode == 0 || pCUILHandle == NULL)
    {
        IPMI_ERROR("Unified Firmware Update is not initalized properly\n");
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof(INT8U);
    }

    memset(&pAMIFWCommandRes->FWResData, 0x00, sizeof(pAMIFWCommandRes->FWResData));
    switch(pAMIFWCommandReq->FWSubCommand)
    {
        case IPMI_AMI_SET_UPDATE_MODE:
        {
            if(pReq[1] > 1)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            FirmwareUpdateMode = dlsym(pCUILHandle,"FirmwareUpdateMode");
            if(NULL == FirmwareUpdateMode)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            if( 0x01 == pReq[1])
            {
                if( ReqLen != 2)
                {
                    *pRes = CC_REQ_INV_LEN;
                    return sizeof(INT8U);
                }
                RandNumber = 0;
                nRet = FirmwareUpdateMode(1, &RandNumber);
                if(nRet == 1)
                {
                    *pRes = CC_ALREADY_IN_FW_UPDATEMODE;
                    return sizeof(INT8U);
                }
                else if(nRet == 2)
                {
                    *pRes = CC_PENDING_FW_UPDATE_IN_PROEGRESS;
                    return sizeof(INT8U);
                }
                else if (nRet != 0)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }

                UnifiedUpdateMode = pReq[1];
                *pRes = CC_SUCCESS;

                memcpy(pRes+1, &RandNumber, 4 );

                return 5;
            }
            else if (0x00 == pReq[1]) 
            {
                if(ReqLen != 6)
                {
                    *pRes = CC_REQ_INV_LEN;
                    return sizeof (INT8U);
                }
                RandNumber = 0;
                memcpy(&RandNumber, pReq+2, 4);

                nRet = FirmwareUpdateMode(0, &RandNumber);
                if(nRet == 3)
                {
                    *pRes = CC_INVALID_FW_UNIQUE_ID;
                    return sizeof(INT8U);
                }
                else if(nRet != 0)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT16U);
                }

                *pRes = CC_SUCCESS;
                return sizeof(INT8U);
            }
        }
            break;
        case IPMI_AMI_GET_IMAGES_INFO:
            ReadImageInfo = dlsym(pCUILHandle,"FW_Read_Image_Info");
            if(NULL == ReadImageInfo)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            if(((ReqLen - 2) % DEVICE_IDEN_LEN ) != 0)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            RespLen =  ReadImageInfo(pReq, ReqLen, pRes ) ;
            if( RespLen == -1)
            {
                pAMIFWCommandRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }
            else
            {
                pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
            }

            return RespLen;

            break;
        case IPMI_AMI_SET_NETWORK_SHARE_CONFIG:
        {
            SetNetworkShareConfig = dlsym(pCUILHandle,"CUIL_SetNetworkShareConfig");

            if(NULL == SetNetworkShareConfig){
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            /*if(ReqLen < SET_NETWORK_SHARE_CONFIG_MIN_REQ_LEN ){ 
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            } */

            nRet =  SetNetworkShareConfig(pReq, ReqLen, pRes ) ;
            if(nRet == 1)
            {
                *pRes = CC_INVALID_FW_UNIQUE_ID;
                return sizeof(INT8U);
            }
            else if( 0 != nRet){
                pAMIFWCommandRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
                
            }else{
                pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
            }

            return (2*sizeof(INT8U));

            break;
           
        }
        case IPMI_AMI_GET_NETWORK_SHARE_CONFIG:
        {

            GetNetworkShareConfig = dlsym(pCUILHandle,"CUIL_GetNetworkShareConfig");
            if(NULL == GetNetworkShareConfig)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            RespLen = GetNetworkShareConfig(pReq, ReqLen, pRes);
            
            if( (-1) == RespLen ){ //failure
                pAMIFWCommandRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
                
            }else if( 1 ==  RespLen){ //not configured
                TINFO(" GetNetworkShareConfig - Not configured ");
                pAMIFWCommandRes->CompletionCode = 0x80 ;
                return sizeof(INT8U);
            }
            else{ //success case
                TDBG(" GetNetworkShareConfig success %d ", RespLen);
                pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
            }

            return RespLen;

            break;
        }
        case IPMI_AMI_SET_NETWORK_SHARE_OPERATION:
        {
            if(ReqLen != 6){
                TCRIT("Invalid request data ");
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }
            if ( (pReq[5] ==5 ) || (pReq[5] == 6) )
            {
                GetNetworkShareOperationStatus = dlsym(pCUILHandle,"CUIL_GetNetworkShareOperationStatus");
                if(NULL == GetNetworkShareOperationStatus)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                ResLen = GetNetworkShareOperationStatus(pReq, ReqLen, pRes);
                if( (-1) == ResLen ){
                    TCRIT("Error in getting the status..\n");
                    return sizeof(INT8U);
                }
                else if (1 == ResLen)
                {
                    *pRes = CC_INVALID_FW_UNIQUE_ID;
                    return sizeof(INT8U);
                }
                return (2*sizeof(INT8U));
            }
            else
            {
                NetworkShareOperation = dlsym(pCUILHandle,"CUIL_NetworkShareOperation");
                if(NULL == NetworkShareOperation){
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                status = NetworkShareOperation(pReq, ReqLen, pRes);
                if(FAILURE == status)
                {
                    pAMIFWCommandRes->CompletionCode = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                else if (1 == status)
                {
                    pAMIFWCommandRes->CompletionCode = CC_INVALID_FW_UNIQUE_ID;
                    return sizeof(INT8U);
                }
                else if (2 == status)
                {
                    pAMIFWCommandRes->CompletionCode = CC_BUNDLE_KEY_NOT_PRESENT; 
                    return sizeof(INT8U);
                }
                else if (3 == status)
                {
                    pAMIFWCommandRes->CompletionCode = CC_BUNDLE_VALIDATION_FAILED;
                    return sizeof(INT8U);
                }
                else if (4 == status)
                {
                    pAMIFWCommandRes->CompletionCode = CC_PENDING_FW_UPDATE_IN_PROEGRESS;
                    return sizeof(INT8U);
                }
                else{
                    pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
                }
                return (2*sizeof(INT8U));
            }
            break;
        }
        case IPMI_AMI_SET_UPDATE_COMPONENT:

            SetCompUpdate = dlsym(pCUILHandle,"FW_Update_Component");
            if(NULL == SetCompUpdate)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            if(ReqLen < 7)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            if(((ReqLen - 7) % DEVICE_IDEN_LEN ) != 0)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            if( (pReq[6] * DEVICE_IDEN_LEN) != (ReqLen - 7))
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            if(g_PDKHandle[PDK_SET_UPDATE_VALIDATE_COMP] != NULL)
            {
                nRet = ((int(*)(INT8U *, int, INT8U*, int))g_PDKHandle[PDK_SET_UPDATE_VALIDATE_COMP])(pReq, ReqLen, pRes, BMCInst);
            }

            if(nRet != 0)
            {
                return sizeof(INT8U);
            }

            nRet =  SetCompUpdate(pReq, ReqLen, pRes);

            if(nRet == 1)
            {
                *pRes = CC_INVALID_FW_UNIQUE_ID;
                return sizeof(INT8U);
            }
            else if (nRet == 2)
            {
                *pRes = CC_PENDING_FW_UPDATE_IN_PROEGRESS;
                return sizeof(INT8U);
            }
            else if (nRet == 3)
            {
                *pRes = CC_UPDATE_COMP_NO_IMG_AVAILABLE;
                return sizeof(INT8U);
            }
            else if (nRet == 4)
            {
                *pRes = CC_UPDATE_COMP_INVALID_COMP_SELECTED;
                return sizeof(INT8U);
            }
            else if( FAILURE == nRet)
            {
                pAMIFWCommandRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }
            else
            {
                pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
            }

            return (2*sizeof(INT8U));

            break;
        case IPMI_AMI_GET_UPDATE_COMP_STATUS:
        {
            GetCompUpdateStatus = dlsym(pCUILHandle,"FW_Update_Component_Status");
            if(NULL == GetCompUpdateStatus)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            RespLen =  GetCompUpdateStatus(pReq, ReqLen, pRes);
            if( -1 == RespLen)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            return RespLen;

            break;
        }
        case IPMI_AMI_GET_UPDATE_COMP_FOR_BIOS:
        {
            memset(ListData,0,256);

            GetUpdateCompforBIOS = dlsym(pCUILHandle,"CUIL_GetListOfUpdateAvailableForBIOS");
            if(NULL == GetUpdateCompforBIOS)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            status = GetUpdateCompforBIOS(&FWCount, ListData,&ResLen);
            if(SUCCESS == status )
            {
                pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
                pAMIFWCommandRes->FWResData[offset] = FWCount;
                offset++;
                memcpy(pAMIFWCommandRes->FWResData +offset, ListData , ResLen);
                return ResLen + 2;
            }
            else
            {
                pAMIFWCommandRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }
            break;
        }
        case IPMI_AMI_SET_UPDATE_COMP_STATUS_BY_BIOS: 
        {
            SetCompUpdateStatusByBIOS = dlsym(pCUILHandle,"CUIL_SaveFWUpdateStatusByBIOS");
            if(NULL == SetCompUpdateStatusByBIOS)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            status = SetCompUpdateStatusByBIOS( pAMIFWCommandReq->FWCommandData);
            if(SUCCESS == status )
            {
                pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
                return sizeof(INT8U);
            }
            else
            {
                pAMIFWCommandRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U); 
            }
            break;
        }
        case IPMI_AMI_GET_COMPONENT_NAME:
            if (ReqLen != 2)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            pDevName = GetDevName(pReq[1]);
            if(pDevName == NULL)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
            memcpy(&pRes[1],pDevName,strlen(pDevName));
            return sizeof(INT8U) + strlen(pDevName);
            break;
        case IPMI_AMI_SET_CANCEL_COMPONENT_UPDATE:
        {
            CancelComponentUpdate = dlsym(pCUILHandle, "FW_CancelComponentUpdate");
            if(NULL == CancelComponentUpdate)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            if(ReqLen != 5)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }
            memcpy(&UniqueID, &pReq[1], sizeof(INT32U));
            nRet =  CancelComponentUpdate(&CancelStatus, UniqueID);
            if(nRet == SUCCESS)
                pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
            else if(nRet == 1)
                pAMIFWCommandRes->CompletionCode = 0x81;
            else if (nRet == 2)
                pAMIFWCommandRes->CompletionCode = 0x82;
            else
                pAMIFWCommandRes->CompletionCode = CC_UNSPECIFIED_ERR;


            return sizeof(INT8U);

            break;
        }
        case IPMI_AMI_GET_VALIDATE_BUNDLE:
        {
            if(ReqLen != 17)
            {
                TCRIT("Invalid request data ");
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }
            offset = 1;
            memcpy(bundlename, pReq+offset, STR_LEN_16);

            snprintf((char *)bundlepath,STR_LEN_64, "%s/%s", "/mnt/sdmmc0p5/componentupdate/", (char *)bundlename);
            memcpy((char *)keypath, "/conf/bdlpublic.pem", STR_LEN_32);

            ValidateBundle = dlsym(pCUILHandle, "CUIL_ValidateBundle");
            if (NULL == ValidateBundle)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            status = ValidateBundle(bundlepath, keypath);
            if (SUCCESS != status)
            {
                pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
                return (sizeof(INT8U));
            }
            else
                pAMIFWCommandRes->CompletionCode = CC_SUCCESS;

            return (sizeof(INT8U));
            break;

        }
        case IPMI_AMI_REARM_FIRMWARE_TIMER:
            if(ReqLen != 5)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            memcpy(&UniqueID, &pReq[1], sizeof(INT32U));
            RearmFirmwareTimer = dlsym(pCUILHandle,"CUIL_RearmFirmwareTimer");
            if(NULL == RearmFirmwareTimer)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            nRet= RearmFirmwareTimer( UniqueID, &NewUID);
            if(1 == nRet)
            {
            
                *pRes = CC_ALREADY_IN_FW_UPDATEMODE;
                return sizeof(INT8U);
            }
            else if (2 == nRet)
            {
                *pRes = CC_PENDING_FW_UPDATE_IN_PROEGRESS;
                return sizeof(INT8U);
            }
            else if(3 == nRet)
            {
                *pRes = CC_INVALID_FW_UNIQUE_ID;
                return sizeof(INT8U);
            }
            else if (0 != nRet)
            {
                pAMIFWCommandRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }
            pAMIFWCommandRes->CompletionCode = CC_SUCCESS;
            memcpy(&pRes[1],&NewUID,sizeof(NewUID));
            return sizeof(INT8U) + sizeof(NewUID);

            break;
        default:
            pAMIFWCommandRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
            break;
    }

    return 2*sizeof(INT8U);
}

int AMIGetReleaseNote (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{

    AMIGetReleaseNoteReq_T *pAMIGetReleaseNoteReq = (AMIGetReleaseNoteReq_T *)pReq;
    AMIGetReleaseNoteRes_T *pAMIGetReleaseNoteRes = (AMIGetReleaseNoteRes_T *)pRes;
    int status = -1;
    int (*GetReleaseNote)(INT8U, INT16U, INT16U*, INT16U*, INT8U*) = NULL;

    if(g_corefeatures.unified_firmware_update != ENABLED)
    {
        *pRes = CC_INV_CMD;
        return sizeof(INT8U);
    }

    if(pCUILHandle == NULL)
    {
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof(INT8U);
    }

    GetReleaseNote = dlsym(pCUILHandle,"CUIL_GetReleaseNote");
    if(NULL == GetReleaseNote)
    {
        IPMI_ERROR("Error while Getting the symbol %d\n",dlerror());
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof(INT8U);
    }

    pAMIGetReleaseNoteRes->CompletionCode = CC_SUCCESS;

    status = GetReleaseNote(pAMIGetReleaseNoteReq->DeviceType,pAMIGetReleaseNoteReq->DataOffset,
        &pAMIGetReleaseNoteRes->NextOffset,&pAMIGetReleaseNoteRes->DataLen,pAMIGetReleaseNoteRes->ChunkData);
    if(!status)
    printf("Error in Reading Realese Note\n");

    return sizeof(AMIGetReleaseNoteRes_T);
}



