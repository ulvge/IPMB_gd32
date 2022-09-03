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
 * Author: Muthuchamy Kumar <muthusamyk@amiinida.co.in>
 *
******************************************************************/
#include <sys/sysinfo.h>
#include <string.h>
#include <dlfcn.h>

#include "IPMI_AMIDevice.h"
#include "Types.h"
#include "OSPort.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "PDKAccess.h"
#include "IPMIConf.h"
#include "safesystem.h"
#include "UnifiedFwUpdate.h"



#define RESERVED_MEMORY     5 * 1024
#define MAX_TEMP_SIZE       1024

extern unsigned long CalculateChksum (char *data, unsigned long size);


/*
 * @Fn ProcessOpCodeReqData
 * @brief this function is used to validate opcode specific request data and get the filename to store the data.
 * @param pReq [in], PktLen [in], pRes[in], FilePath[out], BMCInst[in]
 * @return 0 on success, postive values on failure
 */
int ProcessUploadOpCodeReqData( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ INT8S *FilePath,_NEAR_ int BMCInst)
{
    AMIFileUploadReq_T *pAMIFileUploadReq = (AMIFileUploadReq_T *)pReq;
    AMIFileUploadRes_T *pAMIFileUploadRes = (AMIFileUploadRes_T *)pRes;
    INT8U *pOpCodeReq = pReq + sizeof(AMIFileUploadReq_T);
    int RetVal = 0;
    INT8U Filename[MAX_FILE_NAME] = {0};
    char TempPath[MAX_TEMP_SIZE] = {0};
    INT32U UniqueID = 0;
    pAMIFileUploadRes->CompletionCode = CC_NORMAL;
    pAMIFileUploadRes->TransID = 0x00;
    int (*ValidateUniqueID) (INT32U) = NULL;
    int (*CheckPendingUpdate)(void) = NULL;

    /*Call PDK hook to overwrite or Get the OEM specific File Path*/
    if (g_PDKHandle[PDK_PROCESS_FILE_UPLOAD_REQ_DATA] != NULL)
    {
        RetVal = ((int(*)(INT8U*, INT32U, INT8U*, INT8S*, int))(g_PDKHandle[PDK_PROCESS_FILE_UPLOAD_REQ_DATA]))(pReq, ReqLen, pRes, FilePath, BMCInst);
        if (RetVal == 0)
        {
            /*OpCode is overwrite by PDK*/
            return 0;
        }
        else if(RetVal > 0)
        {
            /*If the Retval is positive, then OpCode Specific Comletion Code is returned*/
            return RetVal;
        }
        /*RetVal == -1, Proceed with core implementation*/
    }

    switch (pAMIFileUploadReq->Hdr.OpCode)
    {
        case OPCODE_COMMON:

            memcpy(Filename,pOpCodeReq,pAMIFileUploadReq->Hdr.RqLen);

            if( 0 != GetFilePath(OPCODE_COMMON, &TempPath[0]))
            {
                *pRes = CC_ERR_FILE_PATH_NOT_PRESENT;
                return sizeof(INT8U);
            }
            snprintf(FilePath,MAX_FILE_PATH_SIZE,"%s/%s",TempPath,Filename);

        break;

        case OPCODE_BUNDLE:
            if(g_corefeatures.unified_firmware_update != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }

            if(pAMIFileUploadReq->Hdr.RqLen != 4)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            memcpy(&UniqueID, pOpCodeReq, pAMIFileUploadReq->Hdr.RqLen);
            /*validate opp code for bundle upload if bundle check for any firmware update is in progress or not?*/
            if ( access("/var/FWUpdateStatus", F_OK) != 0)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof(INT8U);
            }

            if(NULL == pCUILHandle)
            {
                IPMI_ERROR("Error While loading the libCompUpdateInterface library\n");
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            CheckPendingUpdate = dlsym(pCUILHandle,"CUIL_CheckPendingUpdate");
            if(NULL == CheckPendingUpdate)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            if(0 != CheckPendingUpdate())
            {
                IPMI_INFO("Previous Update is going on. Please wait or cancel the previous update");
                *pRes = CC_PREVIOUS_FIRMWARE_UPDATE_PENDING;
                return sizeof(INT8U);
            }

            ValidateUniqueID = dlsym(pCUILHandle,"CUIL_ValidateFWUniqueID");
            if(NULL == ValidateUniqueID)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            if ( 0 != ValidateUniqueID(UniqueID))
            {
                IPMI_INFO("Invalid FW Unuque ID %x",UniqueID);
                *pRes = CC_FIRMWARE_MODE_INVALID_UNIQUE_ID;
                return sizeof(INT8U);
            }


            if( 0 != GetFilePath(OPCODE_BUNDLE, &TempPath[0]))
            {
                *pRes = CC_ERR_FILE_PATH_NOT_PRESENT;
                return sizeof(INT8U);
            }
            snprintf(FilePath,MAX_FILE_PATH_SIZE,"%s/bundle.bdl",TempPath);

        break;

        case OPCODE_BIOSIMG:
            if(g_corefeatures.unified_firmware_update != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }

            /*TODO: Define the BIOS image path for SPI and eMMC */
        break;

        case OPCODE_BMCIMG:
            if(g_corefeatures.unified_firmware_update != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }

            if( 0 != GetFilePath(OPCODE_BMCIMG, &TempPath[0]))
            {
                *pRes = CC_ERR_FILE_PATH_NOT_PRESENT;
                return sizeof(INT8U);
            }
            snprintf(FilePath,MAX_FILE_PATH_SIZE,"%s/rom.ima",TempPath);

        break;

        default:
            *pRes = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
    }

return 0;
}

/*
 * @Fn ProcessOpCodeReqData
 * @brief this function is used to validate opcode specific request data and get the filename to store the data.
 * @param pReq [in], PktLen [in], pRes[in], FilePath[out], BMCInst[in]
 * @return 0 on success, postive values on failure
 */
int ProcessDownloadOpCodeReqData( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, _NEAR_ INT8S *FilePath,_NEAR_ int BMCInst)
{
    AMIFileDownloadReq_T *pAMIFileDownloadReq = (AMIFileDownloadReq_T *)pReq;
    INT8U *pOpCodeReq = pReq + sizeof(AMIFileDownloadReq_T);
    int RetVal = 0;
    INT8U Identifier[DEV_IDENTIFIER_LEN] = {0};
    char Filename[MAX_FILE_NAME] = {0};
    char TempPath[MAX_TEMP_SIZE] = {0};
    int (*GetImageName)(INT8U, INT8U, INT8U*, INT8S*)=NULL;

    /*Call PDK hook to overwrite or Get the OEM specific File Path*/
    if (g_PDKHandle[PDK_PROCESS_FILE_DOWNLOAD_REQ_DATA] != NULL)
    {
        RetVal = ((int(*)(INT8U*, INT32U, INT8U*,INT8S*, int))(g_PDKHandle[PDK_PROCESS_FILE_DOWNLOAD_REQ_DATA]))(pReq, ReqLen, pRes, FilePath, BMCInst);
        if (RetVal == 0)
        {
            /*OpCode is overwrite by PDK*/
            return 0;
        }
        else if(RetVal > 0)
        {
            /*If the Retval is positive, then OpCode Specific Comletion Code is returned*/
            return RetVal;
        }
        /*RetVal == -1, Proceed with core implementation*/
    }

    switch (pAMIFileDownloadReq->OpCode)
    {
        case OPCODE_COMMON:
            /*Validate the OpCode Request Length*/
            if((ReqLen - sizeof(AMIFileDownloadReq_T)) > sizeof(OpCodeCommonReq_T))
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            memcpy(Filename,pOpCodeReq,(ReqLen - sizeof(AMIFileDownloadReq_T)));
            if( 0 != GetFilePath(OPCODE_COMMON, &TempPath[0]))
            {
                *pRes = CC_ERR_FILE_PATH_NOT_PRESENT;
                return sizeof(INT8U);
            }
            snprintf(FilePath,MAX_FILE_PATH_SIZE,"%s/%s",TempPath,Filename);

        break;

        case OPCODE_BUNDLE:
            if(g_corefeatures.unified_firmware_update != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }

            if( 0 != GetFilePath(OPCODE_BUNDLE, &TempPath[0]))
            {
                *pRes = CC_ERR_FILE_PATH_NOT_PRESENT;
                return sizeof(INT8U);
            }
            snprintf(FilePath,MAX_FILE_PATH_SIZE,"%s/bundle.bdl",TempPath);
        break;

        case OPCODE_BIOSIMG:
            if(g_corefeatures.unified_firmware_update != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }

            if((ReqLen - sizeof(AMIFileDownloadReq_T)) !=  sizeof(Identifier) + 1)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            if(pOpCodeReq[0] > 1)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            memcpy(Identifier,pOpCodeReq + 1,sizeof(Identifier));

            if(pOpCodeReq[0] == 0x00)
            {
                if(NULL == pCUILHandle)
                {
                    IPMI_ERROR("Error While loading the libCompUpdateInterface library\n");
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }

                GetImageName = dlsym(pCUILHandle,"CUIL_GetImageName");
                if(NULL == GetImageName)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }


                if ( 0 != GetImageName(DEVTYPE_BIOS, OPCODE_BIOSIMG, Identifier, &FilePath[0]))
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
            }

        break;

        case OPCODE_BMCIMG:
            if(g_corefeatures.unified_firmware_update != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }

            if( 0 != GetFilePath(OPCODE_BMCIMG, &TempPath[0]))
            {
                *pRes = CC_ERR_FILE_PATH_NOT_PRESENT;
                return sizeof(INT8U);
            }
            snprintf(FilePath,MAX_FILE_PATH_SIZE,"%s/rom.ima",TempPath);
        break;

        case OPCODE_MEZZCARDIMG:
            if(g_corefeatures.unified_firmware_update != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }

            if((ReqLen - sizeof(AMIFileDownloadReq_T)) !=  sizeof(Identifier))
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            memcpy(Identifier,pOpCodeReq,sizeof(Identifier));

            if(NULL == pCUILHandle)
            {
                IPMI_ERROR("Error While loading the libCompUpdateInterface library\n");
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            GetImageName = dlsym(pCUILHandle,"CUIL_GetImageName");
            if(NULL == GetImageName)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }


            if ( 0 != GetImageName(DEVTYPE_MEZZ, OPCODE_MEZZCARDIMG, Identifier, &FilePath[0]))
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

        break;
        case OPCODE_RAIDIMAGE:
            if(g_corefeatures.unified_firmware_update != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }

            if((ReqLen - sizeof(AMIFileDownloadReq_T)) !=  sizeof(Identifier))
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            memcpy(Identifier,pOpCodeReq,sizeof(Identifier));

            if(NULL == pCUILHandle)
            {
                IPMI_ERROR("Error While loading the libCompUpdateInterface library\n");
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            GetImageName = dlsym(pCUILHandle,"CUIL_GetImageName");
            if(NULL == GetImageName)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }


            if ( 0 != GetImageName(DEVTYPE_RAID, OPCODE_RAIDIMAGE, Identifier, &FilePath[0]))
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

        break;

        default:
        *pRes = CC_PARAM_NOT_SUPPORTED;
        return sizeof(INT8U);
    }

    return 0;
}

/*
 * @Fn ProcessUploadCompleteFileData
 * @brief this function is used to take the specific action after receiving the file completely.
 * @param FilePath [in], pReq [in], pRes[in], BMCInst[in]
 * @return 0 on success, postive values on failure
 */
int ProcessUploadCompleteFileData(INT8S* FilePath,INT8U* pReq, INT8U *pRes, int BMCInst)
{
    int RetVal = 0;
    struct stat fstat;
    AMIFileUploadReq_T *pAMIFileUploadReq = (AMIFileUploadReq_T *)pReq;
    int (*ValidateBundle)(INT8S*, INT8S*)=NULL;

    /*Call PDK hook to override or OEM specific operation*/
    if (g_PDKHandle[PDK_FILEEOPPROCESS] != NULL)
    {
        RetVal = ((int(*)(INT8S* ,INT8U *, INT8U *, int))(g_PDKHandle[PDK_FILEEOPPROCESS]))(FilePath, pReq, pRes, BMCInst);
        if (RetVal == 0)
        {
            /*OpCode is overwrite by PDK */
            return 0;
        }
        else if(RetVal > 0)
        {
            return RetVal;
        }
        /*RetVal == -1, proceed with core implementation*/
    }

    switch (pAMIFileUploadReq->Hdr.OpCode)
    {
        case OPCODE_COMMON:
            /*Don't need processing after File upload*/
        break;

        case OPCODE_BUNDLE:
            if(g_corefeatures.unified_firmware_update != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }

            if(NULL == pCUILHandle)
            {
                IPMI_ERROR("Error While loading the libCompUpdateInterface library\n");
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }
            
            ValidateBundle = dlsym(pCUILHandle,"CUIL_ValidateBundle");
            if(NULL == ValidateBundle)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

            if(0 != stat(BUNDLE_KEY_PATH,&fstat))
            {
                *pRes = CC_BUNDLE_KEY_NOT_PRESENT;
                return sizeof(INT8U);
            }

            RetVal = ValidateBundle(FilePath, BUNDLE_KEY_PATH);
            if ( -2 == RetVal)
            {
                *pRes = CC_BUNDLE_VALIDATION_FAILED;
                return sizeof(INT8U);
            }
            else if ( 0 != RetVal)
            {
                *pRes = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }

        break;

        case OPCODE_BIOSIMG:
            /*TODO: Add the AMI BIOS Image Verification */
        break;

        default:
        *pRes = CC_PARAM_NOT_SUPPORTED;
        return sizeof(INT8U);
    }

    return 0;
}

/*
 * @Fn AMIFileUpload
 * @brief this function is used to upload the file to BMC
 * @param pReq [in], ReqLen [in], pRes[in], BMCInst[in]
 * @return 0 on success, postive values on failure
 */
int AMIFileUpload ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
    AMIFileUploadReq_T *pAMIFileUploadReq = (AMIFileUploadReq_T *) pReq;
    AMIFileUploadRes_T *pAMIFileUploadRes = (AMIFileUploadRes_T *) pRes;
    INT8U Reserved[3] = {0};
    struct sysinfo s_info;
    struct stat buf;
    INT32U PktLen = 0;
    FILE *fp = NULL;
    int Err,RetVal = 0;
    FileTrans_T *Track = NULL;
    INT8S TempPath[MAX_FILE_PATH_SIZE] = {0};
    INT8S FilePath[MAX_FILE_PATH_SIZE] = {0};
    INT8S cmd[MAX_DATA_CMD_LEN] = {0};

    if(g_corefeatures.data_transfer_cmd_support != ENABLED)
    {
        *pRes = CC_INV_CMD;
        return sizeof(INT8U);
    }

    /* Validate the Request length */
    if(ReqLen < sizeof (AMIFileUploadReq_T))
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    if((ReqLen - sizeof(AMIFileUploadReq_T)) != pAMIFileUploadReq->Hdr.PktLen)
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    /*return as invalid length if data section is missing in the packet*/
    if((ReqLen - sizeof(AMIFileUploadReq_T)) == 0)
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    if(pAMIFileUploadReq->Hdr.PktLen > MAX_PKT_LEN)
    {
        IPMI_DBG_PRINT_1("Pktlen is invalid %d\n",pAMIFileUploadReq->Hdr.PktLen);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /* Check the Reserved Bit */
    if(pAMIFileUploadReq->Hdr.PktType & 0xFC)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /* Validate the Operation Code */
    if((pAMIFileUploadReq->Hdr.OpCode < OPCODE_OEM) && (pAMIFileUploadReq->Hdr.OpCode > OPCODE_RAIDIMAGE))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /* Check ther Reserved Byte */
    if((pAMIFileUploadReq->Hdr.Reserved1 & 0xFFFFFFFF) || memcmp(&pAMIFileUploadReq->Hdr.Reserved,&Reserved,sizeof(Reserved)) != 0)
    {
       *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /* Validate the Offset */
    if((pAMIFileUploadReq->Hdr.PktType & SOP_BIT) && ((pAMIFileUploadReq->Hdr.Offset != 0x00) || pAMIFileUploadReq->Hdr.TransID != 0x00))
    {
        IPMI_DBG_PRINT_2("pkt type and offset fail !! pkt ypte %d and offset %d\n",pAMIFileUploadReq->Hdr.PktType,pAMIFileUploadReq->Hdr.Offset);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    if(!(pAMIFileUploadReq->Hdr.PktType & SOP_BIT) && ((pAMIFileUploadReq->Hdr.Offset == 0x00) || pAMIFileUploadReq->Hdr.TransID == 0x00))
    {
        IPMI_DBG_PRINT("Offset or TransID should not be 0x0 other than SOP packet\n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /*Validate the OpCode Request Length field*/
    if((pAMIFileUploadReq->Hdr.RqLen != 0) && ((pAMIFileUploadReq->Hdr.PktType & SOP_BIT) != SOP_BIT))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    if((ReqLen - sizeof(AMIFileUploadReq_T)) < pAMIFileUploadReq->Hdr.RqLen)
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    /* Validate the Checksum Field */
    if(CalculateChksum((char *)&pReq[sizeof(AMIFileUploadReq_T)],pAMIFileUploadReq->Hdr.PktLen) != pAMIFileUploadReq->Hdr.CheckSum)
    {
        *pRes = CC_INVALID_CHECKSUM;
        return sizeof(INT8U);
    }

    Err=sysinfo(&s_info);
    if(Err !=0)
    {
        fprintf(stderr,"\nError in getting free RAM Memory using Sysinfo system call\n Error Code:%d\n",Err);
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof(INT8U);
    }

    OS_THREAD_MUTEX_ACQUIRE(&g_MBMCInfo.FileTransMutex,WAIT_INFINITE);

    Track = GetFileUpTrackInfo( pAMIFileUploadReq->Hdr.TransID );
    if(Track == NULL)
    {
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        *pRes = CC_DATA_UP_TRANS_LIMIT_EXIST;
        return sizeof(INT8U);
    }

    /*Verify the RAM memory before storing the packet. Reserve 5KB for other use*/
    if(pAMIFileUploadReq->Hdr.PktLen > (s_info.freeram - RESERVED_MEMORY))
    {
        *pRes = CC_OUT_OF_MEMORY;
        if(Track->TransID != 0 && Track->FilePath[0] != 0)
        {
            if(Track->fp)
                fclose(Track->fp);
            /* Remove the file if out of memory*/
            unlink(Track->FilePath);
        }

        /* Reset the values to stop the monitoring*/
        memset(Track,0,sizeof(FileTrans_T));
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        return sizeof(INT8U);
    }

    /*free the kernel cache memory for every 2MB of transfer*/
    if ((pAMIFileUploadReq->Hdr.Offset % (2 * 1024 * 1024)) >= ((pAMIFileUploadReq->Hdr.Offset + pAMIFileUploadReq->Hdr.PktLen) % (2 * 1024 * 1024)))
    {
        safe_system("sync;echo 1 > /proc/sys/vm/drop_caches");
    }

    /* Process the OpCode Request data and upload the packet */
    if(pAMIFileUploadReq->Hdr.PktType & SOP_BIT)
    {
        RetVal = ProcessUploadOpCodeReqData(pReq,ReqLen,pRes,FilePath,BMCInst);
        if(RetVal != 0)
        {
            OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
            return RetVal;
        }
        
        snprintf(TempPath,MAX_FILE_PATH_SIZE,"%s_tmp",FilePath);
        
        /* Return if the same file is already in progress*/
        if(0 == stat(TempPath,&buf))
        {
            OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
            *pRes = CC_ERR_FILE_ALREADY_EXIT;
            return sizeof(INT8U);
        }

        fp = fopen(TempPath,"wb");
        if(fp == NULL)
        {
            OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
            *pRes = CC_ERR_FILE_NOT_CREATED;
            return sizeof(INT8U);
        }

        if(0 != fseek(fp,0,SEEK_SET))
        {
            fclose(fp);
            unlink(TempPath);
            OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
            *pRes = CC_ERR_FILE_NOT_CREATED;
            return sizeof(INT8U);
        }

        /* Get the Actual data offset and calculate the packet length to write*/
        PktLen = (pAMIFileUploadReq->Hdr.PktLen - pAMIFileUploadReq->Hdr.RqLen);

        if( PktLen != fwrite(&pReq[ sizeof(AMIFileUploadReq_T) + pAMIFileUploadReq->Hdr.RqLen], 1,PktLen,fp))
        {
            fclose(fp);
            unlink(TempPath);
            IPMI_DBG_PRINT("Error while writing the bytes to file\n");
            OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
            *pRes = CC_ERR_FILE_NOT_CREATED;
            return sizeof(INT8U);
        }

        memset(Track->FilePath,0,MAX_FILE_PATH_SIZE);
        memcpy(Track->FilePath,TempPath,MAX_FILE_PATH_SIZE);

        /* Get the File Transfer ID*/
        Track->TransID = GetTransID();
        pAMIFileUploadRes->TransID = Track->TransID;
        Track->fp = fp;

    }
    else
    {
        if((pAMIFileUploadReq->Hdr.TransID != Track->TransID) || (pAMIFileUploadReq->Hdr.TransID == 0))
        {
            IPMI_DBG_PRINT_1("Trans ID is invalid %d\n",pAMIFileUploadReq->Hdr.TransID);
            OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
            *pRes = CC_INVALID_TRANS_ID;
            return sizeof(INT8U);
        }

        pAMIFileUploadRes->TransID = pAMIFileUploadReq->Hdr.TransID;

        if( 0 != fseek(Track->fp,pAMIFileUploadReq->Hdr.Offset,SEEK_SET))
        {
            *pRes = CC_UNSPECIFIED_ERR;
            OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
            return sizeof(INT8U);
        }

        if( (pAMIFileUploadReq->Hdr.PktLen) != fwrite(&pReq[ sizeof(AMIFileUploadReq_T)] , 1, pAMIFileUploadReq->Hdr.PktLen,Track->fp))
        {
            IPMI_DBG_PRINT_1("Error while writing the file Pkt Lenght %d\n",pAMIFileUploadReq->Hdr.PktLen);
            *pRes = CC_UNSPECIFIED_ERR;
            OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
            return sizeof(INT8U);
        }
        fflush(Track->fp);
    }

    Track->Inprogess = FILE_TRANSFER_INPROGRESS;
    sem_post(&g_MBMCInfo.FileTransSem);

    if (pAMIFileUploadReq->Hdr.PktType & EOP_BIT )
    {
        fclose(Track->fp);
        RetVal = ProcessUploadCompleteFileData(Track->FilePath,pReq,pRes,BMCInst);
        if(RetVal != 0)
        {
            OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
            unlink(Track->FilePath);
            Track->TransID = 0;
            memset(Track->FilePath,0,sizeof(Track->FilePath));
            Track->Inprogess = FILE_TRANSFER_COMPLETED;
            return RetVal;
        }

        /*Rename the file once all the data are transfered.*/
        memset(TempPath,0,sizeof(TempPath));
        strncpy(TempPath,Track->FilePath,strlen(Track->FilePath) - 4);
        snprintf(cmd,MAX_DATA_CMD_LEN,"mv \"%s\" \"%s\"",Track->FilePath,TempPath);
        safe_system(cmd);

        Track->TransID = 0;
        memset(Track->FilePath,0,sizeof(Track->FilePath));
        Track->Inprogess = FILE_TRANSFER_COMPLETED;
        /*free the kernel caches*/
        safe_system("sync;echo 1 > /proc/sys/vm/drop_caches");
    }

    OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
    pAMIFileUploadRes->CompletionCode = CC_NORMAL;

    return sizeof(AMIFileUploadRes_T);
}

/*
 * @Fn AMIFileDownload
 * @brief this function is used to download the file from BMC
 * @param pReq [in], ReqLen [in], pRes[in], BMCInst[in]
 * @return 0 on success, postive values on failure
 */
int AMIFileDownload ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
    AMIFileDownloadReq_T *pAMIFileDownloadReq = (AMIFileDownloadReq_T *) pReq;
    AMIFileDownloadRes_T *pAMIFileDownloadRes = (AMIFileDownloadRes_T *) pRes;
    struct stat buf;
    FileTrans_T *Track = NULL;
    FILE *fp = NULL;
    int RetVal = 0;
    INT8S FilePath[MAX_FILE_PATH_SIZE] = {0};

    if(g_corefeatures.data_transfer_cmd_support != ENABLED)
    {
        *pRes = CC_INV_CMD;
        return sizeof(INT8U);
    }

    if(ReqLen < sizeof(AMIFileDownloadReq_T))
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    /* Validate the Operation Code */
    if((pAMIFileDownloadReq->OpCode < OPCODE_OEM) &&(pAMIFileDownloadReq->OpCode > OPCODE_RAIDIMAGE))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /*Check the Packet Length*/
    if(pAMIFileDownloadReq->PktLen > MAX_PKT_LEN)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /* If Trans ID is 0x00, offset should be 0x0*/
    if(pAMIFileDownloadReq->TransID == 0x00 && (pAMIFileDownloadReq->Offset != 0x0))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    OS_THREAD_MUTEX_ACQUIRE(&g_MBMCInfo.FileTransMutex,WAIT_INFINITE);

    /*Get the Track Info*/
    Track = GetFileDownTrackInfo(pAMIFileDownloadReq->TransID);
    if(Track == NULL)
    {
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        *pRes = CC_DATA_DOWN_TRANS_LIMIT_EXIST;
        return sizeof(INT8U);
    }

    if((pAMIFileDownloadReq->TransID == 0x0) && (pAMIFileDownloadReq->Offset == 0x0))
    {
        RetVal = ProcessDownloadOpCodeReqData(pReq,ReqLen,pRes,FilePath,BMCInst);
        if(RetVal != 0)
        {
        printf("Error while processing the opcode request data\n");
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        return RetVal;
        }

        /*Update the filepath to TrackInfo*/
        memcpy(Track->FilePath,FilePath,MAX_FILE_PATH_SIZE);

        Track->TransID = GetTransID();
        IPMI_DBG_PRINT_1("TransID %d\n",Track->TransID);
        pAMIFileDownloadRes->Hdr.PktType = SOP_BIT;
    }
    else if(ReqLen != sizeof(AMIFileDownloadReq_T))
    {
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    if(0 != stat(Track->FilePath,&buf))
    {
        printf("File Path is not present %s\n",Track->FilePath);
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        *pRes = CC_FILE_NOT_PRESENT;
        return sizeof(INT8U);
    }

    if(pAMIFileDownloadReq->Offset > buf.st_size)
    {
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        *pRes = CC_FILE_OFFSET_OUT_OF_RANGE;
        return sizeof(INT8U);
    }

    Track->Inprogess = FILE_TRANSFER_INPROGRESS;

    pAMIFileDownloadRes->Hdr.TransID = Track->TransID;
    /*Update the Next Offset Field*/
    pAMIFileDownloadRes->Hdr.Offset = pAMIFileDownloadReq->Offset + pAMIFileDownloadReq->PktLen;

    if((pAMIFileDownloadReq->Offset + pAMIFileDownloadReq->PktLen) >= buf.st_size)
    {
        pAMIFileDownloadRes->Hdr.PktType |= EOP_BIT;
        /*If request packet size is greater than file size, return file size */
        pAMIFileDownloadReq->PktLen = (buf.st_size - pAMIFileDownloadReq->Offset);
        /*Reset the TransID once Last packet is transfered */
        Track->Inprogess = FILE_TRANSFER_COMPLETED;
        Track->TransID = 0x00;
        pAMIFileDownloadRes->Hdr.Offset = 0x00;
    }

    fp = fopen(Track->FilePath,"rb");
    if(fp == NULL)
    {
        printf("Error while opening the file %s and Transfer ID %d\n",Track->FilePath, Track->TransID);
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        *pRes = CC_ERR_READING_FILE;
        return sizeof(INT8U);
    }

    if(0 != fseek(fp,pAMIFileDownloadReq->Offset,SEEK_SET))
    {
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        *pRes = CC_ERR_READING_FILE;
        return sizeof(INT8U);
    }

    if(pAMIFileDownloadReq->PktLen != fread(&pRes[sizeof(AMIFileDownloadRes_T)],1,pAMIFileDownloadReq->PktLen,fp))
    {
        OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);
        *pRes = CC_ERR_READING_FILE;
        return sizeof(INT8U);
    }

    fclose(fp);

    OS_THREAD_MUTEX_RELEASE(&g_MBMCInfo.FileTransMutex);

    sem_post(&g_MBMCInfo.FileTransSem);

    /*Update Response Packet length*/
    pAMIFileDownloadRes->Hdr.OpCode = pAMIFileDownloadReq->OpCode;
    pAMIFileDownloadRes->Hdr.PktLen = pAMIFileDownloadReq->PktLen;
    pAMIFileDownloadRes->Hdr.CheckSum = CalculateChksum((char *)&pRes[sizeof(AMIFileDownloadRes_T)],pAMIFileDownloadReq->PktLen);
    pAMIFileDownloadRes->CompletionCode = CC_NORMAL;

    return sizeof(AMIFileDownloadRes_T) + pAMIFileDownloadReq->PktLen;
}


