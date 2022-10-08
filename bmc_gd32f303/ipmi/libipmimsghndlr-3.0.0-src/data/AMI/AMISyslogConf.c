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
 * AMISyslogConf.c
 * AMI Syslog configuration related implementation.
 *
 * Author: Gokula Kannan. S <gokulakannans@amiindia.co.in>
 ******************************************************************/

#include "string.h"
#include "stdlib.h"
#include "Debug.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "syslogconf.h"
#include "AMISyslogConf.h"
#include "IPMI_AMISyslogConf.h"
#include "IPMIConf.h"

//#define RESV_EXTLOG_CONF_BITS 0xFE
//#define EXT_DEBUG_CONF_FILE(Instance,filename) sprintf(filename,"%s%d/%s",NV_DIR_PATH,Instance,".DebugConf")


/**
 * @fn CheckInputParams
 * @brief Check the input parameter and its length.
 * @param[in] pSetSyslogConf - pointer to the set syslog conf structure.
 * @param[in] ReqLen - Length of the request.
 * @retval  	CC_NORMAL, on success,
 * 				CC_REQ_INV_LEN, if the given length is not valid,
 * 				CC_INV_DATA_FIELD, if the given data is not valid.
 */
static INT8U CheckInputParams(AMISetLogConfReq_T *pSetSyslogConf, INT32U ReqLen)
{
    if(pSetSyslogConf->Cmd == AUDITLOG)
    {
        /* Request for Audit log */
        if(ReqLen != SIZE_AUDIT)
        {
            return CC_REQ_INV_LEN;
        }

        /* Audit log is supported to disable
        * or enable local but not remote. */
        if(pSetSyslogConf->Status != DISABLE &&
        pSetSyslogConf->Status != ENABLE_LOCAL)
        return CC_INV_DATA_FIELD;
    }
    else if(pSetSyslogConf->Cmd == SYSLOG)
    {
        /* Request for system log */
        switch(pSetSyslogConf->Status)
        {
            case DISABLE:
                if(ReqLen != SIZE_SYS_DISABLE)
                return CC_REQ_INV_LEN;
                break;

            case ENABLE_LOCAL:
                if(ReqLen != SIZE_SYS_LOCAL)
                return CC_REQ_INV_LEN;
                break;

            case ENABLE_REMOTE:
                if(ReqLen != SIZE_SYS_REMOTE)
                return CC_REQ_INV_LEN;
                break;

            default:
                return CC_INV_DATA_FIELD;
        }
    }
    else
    {
        return CC_INV_DATA_FIELD;
    }
    return CC_NORMAL;
}

/**
 * @fn AMIGetLogConf
 * @brief Get the log configuration file.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval  	CC_NORMAL, on success,
 * 				CC_UNSPECIFIED_ERR, if any unknown errors.
 */
int AMIGetLogConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	INT32U  size;
    AMIGetLogConfRes_T *pGetSyslogConf = (AMIGetLogConfRes_T *)pRes;
    TDBG("Inside AMIGetSyslogConf\n");

    memset(pGetSyslogConf, 0, sizeof(AMIGetLogConfRes_T));
    pGetSyslogConf->CompletionCode = CC_UNSPECIFIED_ERR;

    /* Get the status of the system log and audit log */
    GetSyslogConf(&(pGetSyslogConf->SysStatus), &(pGetSyslogConf->AuditStatus),
    pGetSyslogConf->Config.Remote.HostName);

    /* Get the logrotate and size if the system status is local */
    if(pGetSyslogConf->SysStatus == ENABLE_LOCAL)
    {
        GetLogRotate(&(pGetSyslogConf->Config.Local.Rotate), &size);
        pGetSyslogConf->Config.Local.Size = size;
        pGetSyslogConf->CompletionCode = CC_NORMAL;
        return sizeof(AMIGetLogConfRes_T) - (sizeof(pGetSyslogConf->Config) - sizeof(pGetSyslogConf->Config.Local));
    }

    pGetSyslogConf->CompletionCode = CC_NORMAL;

    return sizeof(AMIGetLogConfRes_T) - (sizeof(pGetSyslogConf->Config) - sizeof(pGetSyslogConf->Config.Remote));

}

/**
 * @fn AMISetLogConf
 * @brief Set the log configuration file.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval  	CC_NORMAL, on success,
 * 				CC_REQ_INV_LEN, if the given length is not valid,
 * 				CC_INV_DATA_FIELD, if the given data is not valid,
 * 				CC_UNSPECIFIED_ERR, if any unknown errors.
 */
int AMISetLogConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst)
{

    AMISetLogConfReq_T *pSetSyslogConf = (AMISetLogConfReq_T *)pReq;
    (*pRes) = CC_NORMAL;
    TDBG("Inside AMISetSyslogConf\n");

    /* Validate the input parameters */
    (*pRes) = CheckInputParams(pSetSyslogConf, ReqLen);
    if((*pRes) != CC_NORMAL)
        goto end;

        if(pSetSyslogConf->Cmd == AUDITLOG)
        {
            switch(pSetSyslogConf->Status)
            {
                case DISABLE:
                    /* Request for audit log disable */
                    if(AuditlogDisable() != 0)
                    *pRes = CC_UNSPECIFIED_ERR;
                    break;

                case ENABLE_LOCAL:
                    /* Request for audit log enable */
                    if(AuditlogEnable() != 0)
                    *pRes = CC_UNSPECIFIED_ERR;
                    break;

                default:
                    *pRes = CC_INV_DATA_FIELD;
            }
        }
        else
        {
            switch(pSetSyslogConf->Status)
            {
                case DISABLE:
                    /* Request for system log disable */
                    if(SyslogDisable() != 0)
                    {
                        (*pRes) = CC_UNSPECIFIED_ERR;
                    }
                    break;

                case ENABLE_LOCAL:
                    /* Request for system log enable local */
                    if((SyslogEnableLocal() != 0) || (SetLogRotate(pSetSyslogConf->Config.Local.Rotate,
                    pSetSyslogConf->Config.Local.Size) != 0))
                    {
                        (*pRes) = CC_UNSPECIFIED_ERR;
                    }
                    break;

            case ENABLE_REMOTE:
                /* Request for system log enable to log in remote syslog server */
                if(SyslogEnableRemote(pSetSyslogConf->Config.Remote.HostName) != 0)
                {
                    (*pRes) = CC_UNSPECIFIED_ERR;
                }
                break;

            default:
                *pRes = CC_INV_DATA_FIELD;
            }
        }

end:
    return sizeof(*pRes);
}

