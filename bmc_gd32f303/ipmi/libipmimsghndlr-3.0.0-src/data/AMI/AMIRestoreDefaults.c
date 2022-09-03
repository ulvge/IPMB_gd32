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
 * AMIRestoreDefaults.c
 * AMI Restore Factory settings related implementation.
 *
 * Author: Gokula Kannan. S <gokulakannans@amiindia.co.in>
 ******************************************************************/

#include "Debug.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "IPMI_AMIConf.h"
#include "AMIRestoreDefaults.h"
#include "flshfiles.h"
#include "flashlib.h"
#include "PendTask.h"
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <dlfcn.h>
#include <pthread.h>
#include "OSPort.h"
#include "PDKAccess.h"
#include "IPMIConf.h"
#include "libpreserveconf.h"
#include "featuredef.h"

#define RESTORE_PRESERVE_CMD "/usr/local/bin/preservecfg 1 &"
#define RESTORE_DEFAULTS_CMD "sh /etc/restoredefaults.sh restore &"
#define RESTORE_FACTORY_SETTINGS "touch /conf/restorefactory"

/*
 * @brief Get Backup Flag Request parameter length
 */

static const INT8U GetBackupFlagParamLength[] = {
                            0,
                            1,
};

/*
 * @brief DNS configuration Request parameter length
 */
static const INT8U ManageBMCConfigParamLength[] = {
                            0,
                            0,
                            63,
                            63,
};

int AMIRestoreDefaults(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    RestartService_T Service;
    INT8U curchannel;

    if(ReqLen > 0)
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(*pRes);
    }

    int retVal = PrepareFlashArea(FLSH_CMD_DUMMY_FLASH_AREA, g_corefeatures.dual_image_support);
    if (retVal != EXIT_SUCCESS)
    {
        IPMI_ERROR("Flash is going on currently");
        (*pRes) = CC_DEV_IN_FIRMWARE_UPDATE_MODE;
        goto end;
    }

    if(g_corefeatures.preserve_config == ENABLED)
    {
        system (RESTORE_PRESERVE_CMD);
    }
    else
    {
        system (RESTORE_DEFAULTS_CMD);
    }
    if(g_corefeatures.dual_image_support == ENABLED)
    {
        system(RESTORE_FACTORY_SETTINGS);
    }
    /* PDK Hook Restore Factory Setting*/
    if(g_PDKHandle[PDK_RESTOREFACTORYSETTINGS] != NULL)
    {
        ((int(*)(int)) g_PDKHandle[PDK_RESTOREFACTORYSETTINGS])(BMCInst);
    }
    /* PDK Module Post Set Reboot Cause*/
    if(g_PDKHandle[PDK_SETREBOOTCAUSE] != NULL)
    {
        ((INT8U(*)(INT8U,int)) g_PDKHandle[PDK_SETREBOOTCAUSE])(SETREBOOTCAUSE_IPMI_CMD_PROCESSING,BMCInst);
    }

    //Post BMC Reboot task to Pend task    
    Service.ServiceName = REBOOT;
    Service.SleepSeconds = 3;		// Sleep for 3 Seconds
    SetPendStatus(PEND_OP_RESTART_SERVICES, PEND_STATUS_PENDING);    
    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    PostPendTask(PEND_OP_RESTART_SERVICES, (INT8U *) &Service, sizeof(RestartService_T),curchannel & 0xF,BMCInst);

    (*pRes) = CC_SUCCESS;
end:
    return sizeof(*pRes);
}

int AMISetBackupFlag(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
	SetBackupFlagReq_T* pBackupFlagReq  =  ( SetBackupFlagReq_T* ) pReq;
	SetBackupFlagRes_T* pBackupFlagRes  =  ( SetBackupFlagRes_T* ) pRes;
	void *dl_handle = NULL;
	int ( *dl_func_SetBackupFlag)( unsigned long, char * ) = NULL;
	int ( *dl_func_IsValidBackupFlag)( unsigned long ) = NULL;
	int Flags = 0;

	if( ENABLED != g_corefeatures.backup_config_support )
	{
		// If Backup Conf feature is disabled in Project Configuration, return Not Support
		pBackupFlagRes->CompletionCode = CC_INV_CMD;
		return sizeof(SetBackupFlagRes_T);
	}
	if (pBackupFlagReq->BackupFlag >= BIT7)
	{
		pBackupFlagRes->CompletionCode = CC_INV_DATA_FIELD;
		return sizeof(SetBackupFlagRes_T);
	}

	dl_handle = dlopen ( "/usr/local/lib/libBackupConf.so", RTLD_NOW );
	if( NULL == dl_handle )
	{
	IPMI_ERROR("Error in loading libBackupConf.so library %s\n", dlerror() );
	pBackupFlagRes->CompletionCode = CC_UNSPECIFIED_ERR;
	return sizeof(SetBackupFlagRes_T);
	}

	memset (pBackupFlagRes, 0x00, sizeof (SetBackupFlagRes_T));
	pBackupFlagRes->CompletionCode = CC_NORMAL;
	
	dl_func_IsValidBackupFlag = dlsym (dl_handle, "IsValidBackupFlag");

	if ( NULL == dl_func_IsValidBackupFlag )
	{
		IPMI_ERROR("Error in getting symbol %s \n", dlerror());
		pBackupFlagRes->CompletionCode = CC_UNSPECIFIED_ERR;
		if (NULL != dl_handle)
			dlclose (dl_handle);

		return sizeof(SetBackupFlagRes_T);
	}

	Flags = dl_func_IsValidBackupFlag( pBackupFlagReq->BackupFlag);
	if(Flags == -1)
	{
		*pRes = CC_AUTHENTICATION_FEATURE_NOT_ENABLED;
		return sizeof(INT8U);
	}
	else if (Flags == -2)
	{
		*pRes = CC_NTP_FEATURE_NOT_ENABLED;
		return sizeof(INT8U);
	}
	else if (Flags == -3)
	{
		*pRes = CC_KVM_FEATURE_NOT_ENABLED;
		return sizeof(INT8U);
	}
	else if (Flags == -4)
	{
		*pRes = CC_SNMP_FEATURE_NOT_ENABLED;
		return sizeof(INT8U);
	}

	dl_func_SetBackupFlag = dlsym (dl_handle, "SetBackupFlag");
	if ( NULL == dl_func_SetBackupFlag )
	{
		IPMI_ERROR("Error in getting symbol %s \n", dlerror());
		pBackupFlagRes->CompletionCode = CC_UNSPECIFIED_ERR;
		if (NULL != dl_handle)
			dlclose (dl_handle);
		return sizeof(SetBackupFlagRes_T);
	}

	Flags = dl_func_SetBackupFlag( pBackupFlagReq->BackupFlag, BACKUP_CFG_LIST_AMI_FILE);
	if (-1 == Flags)
	{
		pBackupFlagRes->CompletionCode = CC_UNSPECIFIED_ERR;
	}
	else if (-2 == Flags)
	{
		pBackupFlagRes->CompletionCode = CC_CMD_UNSUPPORTED_UNCONFIGURABLE;
	}
	else
	{
		pBackupFlagRes->CompletionCode = CC_NORMAL;
	}

	if (NULL != dl_handle)
		dlclose (dl_handle);

	return sizeof(SetBackupFlagRes_T);
}
int AMIGetBackupFlag(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    GetBackupConfigRes_T* pGetBackupConfigRes = (GetBackupConfigRes_T*) pRes;
    GetBackupConfigReq_T* pGetBackupConfigReq = (GetBackupConfigReq_T*) pReq;
    GetBackupFlag_T getBackupFlag;
    GetBackupList_T getBackupList;
    INT8U returnval = 0;

    void *dl_handle = NULL;
    int( *dl_func1 )(GetBackupFlag_T*, char*) = NULL;
    int( *dl_func2 )(int, GetBackupList_T*, char*) = NULL;

    memset(&getBackupFlag, 0, sizeof(GetBackupFlag_T));
    memset(&getBackupList, 0, sizeof(GetBackupList_T));
    memset(pGetBackupConfigRes, 0, sizeof(GetBackupConfigRes_T));

    if (g_corefeatures.backup_config_support != ENABLED)
    {
        pGetBackupConfigRes->CompletionCode = CC_INV_CMD;
        return sizeof(INT8U);
    }

    if(pGetBackupConfigReq->Parameter < 1 || pGetBackupConfigReq->Parameter > 2)
    {
        pGetBackupConfigRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }
    ReqLen = ReqLen - 1;

    if( ReqLen != GetBackupFlagParamLength[pGetBackupConfigReq->Parameter - 1])
    {
        pGetBackupConfigRes->CompletionCode = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }
    dl_handle = dlopen ( "/usr/local/lib/libBackupConf.so", RTLD_NOW );
    if(NULL == dl_handle)
    {
        IPMI_ERROR("Error in loading libBackupConf.so library %s\n", dlerror() );
        pGetBackupConfigRes->CompletionCode = CC_UNSPECIFIED_ERR;
        return sizeof(INT8U);
    }
    pGetBackupConfigRes->CompletionCode = CC_NORMAL;

    switch(pGetBackupConfigReq->Parameter)
    {
    case GET_BACKUP_FLAG:

        dl_func1 = dlsym (dl_handle, "GetBackupFlag");
        if (NULL == dl_func1)
        {
            IPMI_ERROR("Error in getting symbol %s \n", dlerror());
            pGetBackupConfigRes->CompletionCode = CC_UNSPECIFIED_ERR;
            dlclose (dl_handle);
            return sizeof(INT8U);
        }
        returnval = dl_func1(&getBackupFlag, BACKUP_CFG_LIST_AMI_FILE );
        if (0 != returnval)
        {
            pGetBackupConfigRes->CompletionCode = CC_UNSPECIFIED_ERR;
            dlclose (dl_handle);
            return sizeof(INT8U);
        }
        pGetBackupConfigRes->GetBackupConfig.GetBackupFlag.Count = getBackupFlag.Count;
        memcpy(pGetBackupConfigRes->GetBackupConfig.GetBackupFlag.Selector, &getBackupFlag.Selector, MAX_BACKUP_FEATURE);
        dlclose (dl_handle);
        return sizeof(INT8U) + sizeof(GetBackupFlag_T);
        break;

    case GET_BACKUP_LIST:
        dl_func2 = dlsym (dl_handle, "GetBackupList");
        if ( NULL == dl_func2 )
        {
            IPMI_ERROR("Error in getting symbol %s \n", dlerror());
            pGetBackupConfigRes->CompletionCode = CC_UNSPECIFIED_ERR;
            dlclose (dl_handle);
            return sizeof(INT8U);
        }
        returnval = dl_func2(pGetBackupConfigReq->Selector, &getBackupList, BACKUP_CFG_LIST_AMI_FILE );
        if (-2 == returnval)
        {
            pGetBackupConfigRes->CompletionCode = CC_INV_DATA_FIELD;
            dlclose (dl_handle);
            return sizeof(INT8U);
        }
        if (0 != returnval)
        {
            pGetBackupConfigRes->CompletionCode = CC_UNSPECIFIED_ERR;
            dlclose (dl_handle);
            return sizeof(INT8U);
        }

        pGetBackupConfigRes->GetBackupConfig.GetBackupList.Selector = getBackupList.Selector;
        pGetBackupConfigRes->GetBackupConfig.GetBackupList.BackupFlag = getBackupList.BackupFlag;
        memcpy(pGetBackupConfigRes->GetBackupConfig.GetBackupList.ConfigFile, getBackupList.ConfigFile, strlen((char*)getBackupList.ConfigFile));
        dlclose (dl_handle);
        return sizeof(INT8U) + sizeof(GetBackupList_T);
        break;
    }
     pGetBackupConfigRes->CompletionCode = CC_SUCCESS;
     return sizeof(GetBackupConfigRes_T);
}

int AMIManageBMCConfig(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst)
{
    AMIManageBMCConfigReq_T* pAMIManageBMCConfigReq  =  ( AMIManageBMCConfigReq_T* ) pReq;
    AMIManageBMCConfigRes_T* pAMIManageBMCConfigRes  =  ( AMIManageBMCConfigRes_T* ) pRes;
    INT8U IPV4[sizeof(struct in_addr)];
    char ConfBackupFilePath[CONF_BACKUP_PATH_LENGTH] = {0};
    char ConfBackupFile[CONF_BACKUP_PATH_LENGTH]={0};
    char Command[MAX_CMD_LENGTH];
    ManageBMCConfig_T ManageBMCConfig;
    INT8U curchannel;
    struct stat buf;
    int PendTaskStatus = 0;

    memset(pAMIManageBMCConfigRes, 0, sizeof(AMIManageBMCConfigRes_T));
    pAMIManageBMCConfigRes->CompletionCode = CC_NORMAL;

    PendTaskStatus = GetPendStatus(PEND_OP_MANAGE_BMC_BKUPCONFIG);
    if(PendTaskStatus == PEND_STATUS_PENDING)
    {
        TINFO("Pend Task for ManageBMCConfig is already running...");
        return sizeof(AMIManageBMCConfigRes_T);
    }

    if(g_corefeatures.backup_config_support == ENABLED)
    {
        if(pAMIManageBMCConfigReq->Parameter < 1 || pAMIManageBMCConfigReq->Parameter > 4)
        {
            pAMIManageBMCConfigRes->CompletionCode = CC_INV_DATA_FIELD;
            return sizeof(AMIManageBMCConfigRes_T);
        }

        if(ReqLen >= 1)
        {
            ReqLen -= 1;
        }
        else
        {
            pAMIManageBMCConfigRes->CompletionCode = CC_REQ_INV_LEN;
            return sizeof(AMIManageBMCConfigRes_T);
        }

        if( ReqLen > ManageBMCConfigParamLength[pAMIManageBMCConfigReq->Parameter - 1])
        {
            pAMIManageBMCConfigRes->CompletionCode = CC_REQ_INV_LEN;
            return sizeof(AMIManageBMCConfigRes_T);
        }

        pAMIManageBMCConfigRes->CompletionCode = CC_NORMAL;

        if(g_corefeatures.backup_config_sd_emmc_support == ENABLED)
        {
            snprintf(ConfBackupFilePath, CONF_BACKUP_PATH_LENGTH, "%s%d%s", EMMCPATH, g_coremacros.backup_config_partition_num, CONF_BACKUP_FOLDER_NAME);
        }
        else
        {
            snprintf(ConfBackupFilePath, CONF_BACKUP_PATH_LENGTH, "%s%s", SPI_PATH, CONF_BACKUP_FOLDER_NAME);
            if(stat(ConfBackupFilePath, &buf))
            {
                memset(Command, 0, MAX_CMD_LENGTH);
                snprintf(Command, MAX_CMD_LENGTH, "mkdir %s", ConfBackupFilePath);
                system(Command);
            }
        }
        snprintf(ConfBackupFile, CONF_BACKUP_PATH_LENGTH, "%s%s", ConfBackupFilePath, CONF_BACKUP_FILE_NAME);
        TINFO("ConfBackupFile : %s", ConfBackupFile);
        switch(pAMIManageBMCConfigReq->Parameter)
        {
        case BACKUP_CONF:
        case RESTORE_CONF:

            memset(&ManageBMCConfig, 0, sizeof(ManageBMCConfig_T));
            ManageBMCConfig.Parameter = pAMIManageBMCConfigReq->Parameter;
            strncpy(ManageBMCConfig.ConfBackupFile, ConfBackupFile, strlen(ConfBackupFile));
            SetPendStatus(PEND_OP_MANAGE_BMC_BKUPCONFIG, PEND_STATUS_PENDING);
            OS_THREAD_TLS_GET(g_tls.CurChannel, curchannel);
            PostPendTask(PEND_OP_MANAGE_BMC_BKUPCONFIG, (INT8U *) &ManageBMCConfig, sizeof(ManageBMCConfig_T),curchannel & 0xF,BMCInst);
            break;

        case EXPORT_CONF:
        case IMPORT_CONF:

            memset(&ManageBMCConfig, 0, sizeof(ManageBMCConfig_T));
            ManageBMCConfig.Parameter = pAMIManageBMCConfigReq->Parameter;
            strncpy(ManageBMCConfig.ConfBackupFile, ConfBackupFile, strlen(ConfBackupFile));

            if((ReqLen != IP_ADDR_LEN) && (ReqLen != IP6_ADDR_LEN))
            {
                TDBG("Reqlen %ld\n", ReqLen);
                pAMIManageBMCConfigRes->CompletionCode = CC_REQ_INV_LEN;
                return sizeof(AMIManageBMCConfigRes_T);
            }

            if(ReqLen == IP_ADDR_LEN)
            {
                inet_ntop(AF_INET, &(pAMIManageBMCConfigReq->IPAddress[0]), ManageBMCConfig.IPAddress, sizeof (ManageBMCConfig.IPAddress));

                /* Validate IPV4 Address */
                if (!inet_pton (AF_INET, ManageBMCConfig.IPAddress, (char*)IPV4))
                {
                    TCRIT("Invalid IPv4 Address");
                    pAMIManageBMCConfigRes->CompletionCode = CC_INV_DATA_FIELD;
                    return sizeof(AMIManageBMCConfigRes_T);
                }
            }
            else
            {
                /* Validate IPV6 Address */
                if(0 != IsValidGlobalIPv6Addr((struct in6_addr*)&pAMIManageBMCConfigReq->IPAddress))
                {
                    TCRIT("Invalid IPv6 Address");
                    pAMIManageBMCConfigRes->CompletionCode = CC_INV_DATA_FIELD;
                    return sizeof(AMIManageBMCConfigRes_T);
                }
                inet_ntop(AF_INET6, &(pAMIManageBMCConfigReq->IPAddress[0]), ManageBMCConfig.IPAddress, sizeof (ManageBMCConfig.IPAddress));
            }
            SetPendStatus(PEND_OP_MANAGE_BMC_BKUPCONFIG, PEND_STATUS_PENDING);
            OS_THREAD_TLS_GET(g_tls.CurChannel, curchannel);
            PostPendTask(PEND_OP_MANAGE_BMC_BKUPCONFIG, (INT8U *) &ManageBMCConfig, sizeof(ManageBMCConfig_T),curchannel & 0xF,BMCInst);

            break;
        }
        return sizeof(AMIManageBMCConfigRes_T);
    }
    else
    {
        // If Backup Conf feature is disabled in Project Configuration, return Not Support
        pAMIManageBMCConfigRes->CompletionCode = CC_INV_CMD;
        return sizeof(AMIManageBMCConfigRes_T);
    }
}

int AMIGetPendStatus(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    AMIGetPendStatusReq_T* pAMIGetPendStatusReq = ( AMIGetPendStatusReq_T* ) pReq;
    AMIGetPendStatusRes_T* pAMIGetPendStatusRes = ( AMIGetPendStatusRes_T* ) pRes;
    INT32U Status = 0;

    memset(pAMIGetPendStatusRes, 0, sizeof(AMIGetPendStatusRes_T));
    pAMIGetPendStatusRes->CompletionCode = CC_NORMAL;

    Status = GetPendStatus(pAMIGetPendStatusReq->Operation);

    if(Status == -1)
    {
        pAMIGetPendStatusRes->CompletionCode = CC_INVALID_OPERATION_CODE;
        return sizeof(AMIGetPendStatusRes_T);
    }

    pAMIGetPendStatusRes->PendStatus = Status & 0xFF;
    if(pAMIGetPendStatusRes->PendStatus == PEND_STATUS_COMPLETED || pAMIGetPendStatusRes->PendStatus == PEND_STATUS_PENDING)
    {
        return sizeof(INT16U);
    }
    else
    {
        pAMIGetPendStatusRes->ErrorCode = (Status >> 8) & 0xFFFF;
    }
    return sizeof(AMIGetPendStatusRes_T);
}
