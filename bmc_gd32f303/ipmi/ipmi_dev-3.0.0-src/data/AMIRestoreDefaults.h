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
 ******************************************************************
 *
 * AmiRestoreDefaults.h
 * AMI Restore Defaults command Macros
 *
 * Author: Gokula Kannan. S	<gokulakannans@amiindia.co.in>
 *
 ******************************************************************/
#ifndef __AMIRESTOREDEF_H__ 
#define __AMIRESTOREDEF_H__

#include "Types.h"

#ifdef __GNUC__
#define PACKED __attribute__ ((packed))
#else
#define PACKED
#pragma pack( 1 )
#endif

#define GET_BACKUP_FLAG    1
#define GET_BACKUP_LIST    2

#define BACKUP_CONF  1
#define RESTORE_CONF 2
#define EXPORT_CONF  3
#define IMPORT_CONF  4
#define IP6_ADDR_LEN 16
#define SPI_PATH                 "/var"
#define BACKUP_CFG_LIST_AMI_FILE "/conf/backup_cfg_list-AMI.ini"
#define CONF_BACKUP_PATH_LENGTH  64
#define CONF_BACKUP_FOLDER_NAME  "/confbkup"
#define CONF_BACKUP_FILE_NAME    "/conf.bak"
#define MAX_IPADDR_LEN           64
#define MAX_CMD_LENGTH           80
#define MAX_FILENAME_LEN         64
#define MAX_BACKUP_FEATURE       16

#define EMMC_PATH                "/mnt/sdmmc0p"
#define LC_EVNT_LOG_DIR_NAME     "lifecyclelog"
#define LC_EVNT_LOG_FILE         "lcevtlog"
#define RESULT_LOG_FILE          "resultlcevtlog"

#define ALL_EVENT_LOGS           1
#define EVENT_LOGS_BY_TYPE       2
#define EVENT_LOGS_BY_COMPONENT  3
#define MAX_EVT_STRLEN           32

/* GetPreserveConfigRes_T */
typedef struct
{
	INT8U Selector;
} GetPreserveConfigReq_T;

/* GetPreserveConfigRes_T */
typedef struct
{
	INT8U   CompletionCode;
	INT8U	Status;
} GetPreserveConfigRes_T;

/* SetPreserveConfigReq_T */
typedef struct
{
    INT8U Selector;
    INT8U Status;
} SetPreserveConfigReq_T;

/* SetPreserveConfigRes_T */
typedef struct
{
    INT8U CompletionCode;
} SetPreserveConfigRes_T;

/* GetAllPreserveConfigRes_T */
typedef struct
{
    INT8U   CompletionCode;
    INT8U   Reserved;
    INT16U  Status;
    INT16U  EnabledStatus;
} PACKED GetAllPreserveConfigRes_T;

/* SetAllPreserveConfigReq_T */
typedef struct
{
    INT16U   Status;
} PACKED SetAllPreserveConfigReq_T;

/* SetAllPreserveConfigRes_T */
typedef struct
{
    INT8U   CompletionCode;
} PACKED SetAllPreserveConfigRes_T;

/* SetBackupFlagReq_T */
typedef struct
{
    INT16U BackupFlag;
} PACKED SetBackupFlagReq_T;

/* SetBackupFlagRes_T */
typedef struct
{
    INT8U CompletionCode;
} PACKED SetBackupFlagRes_T;

typedef struct
{
    INT8U Parameter;
    INT8U Selector;
} PACKED GetBackupConfigReq_T;

typedef struct
{
    INT8U Count;
    INT8U Selector[MAX_BACKUP_FEATURE];
}PACKED GetBackupFlag_T;

typedef struct {
    INT16U Selector;
    INT8U ConfigFile[MAX_FILENAME_LEN];
    INT16U BackupFlag;
}PACKED GetBackupList_T;

typedef union {
    GetBackupFlag_T GetBackupFlag;
    GetBackupList_T GetBackupList;
}GetBackupConfig_T;

typedef struct
{
    INT8U CompletionCode;
    GetBackupConfig_T GetBackupConfig;
} PACKED GetBackupConfigRes_T;

/* AMIManageBMCConfigReq_T */
typedef struct
{
    INT8U Parameter;
    INT8U IPAddress[IP6_ADDR_LEN+1];
}PACKED AMIManageBMCConfigReq_T;

/* AMIManageBMCConfigRes_T */
typedef struct
{
    INT8U CompletionCode;
} PACKED AMIManageBMCConfigRes_T;

typedef struct
{
    INT8U Parameter;
    INT8S ConfBackupFile[CONF_BACKUP_PATH_LENGTH];
    INT8S IPAddress[MAX_IPADDR_LEN];
} PACKED ManageBMCConfig_T;

typedef struct
{
    INT32U Operation;
} PACKED AMIGetPendStatusReq_T;

typedef struct
{
    INT8U  CompletionCode;
    INT8U  PendStatus;
    INT16U ErrorCode;
} PACKED AMIGetPendStatusRes_T;

typedef struct
{
    INT8U Parameter;
    INT8U Type;
} PACKED AMIGetLCEvtLogReq_T;

typedef struct
{
    INT8U CompletionCode;
} PACKED AMIGetLCEvtLogRes_T;

#ifdef __GNUC__
#else
#define PACKED
#pragma pack(  )
#endif

extern int AMIRestoreDefaults (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);

extern int AMIGetPreserveConfStatus(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);

extern int AMISetPreserveConfStatus(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);

extern int AMISetAllPreserveConfStatus(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);

extern int AMIGetAllPreserveConfStatus(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);

extern int AMISetBackupFlag(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);

extern int AMIGetBackupFlag(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);

extern int AMIManageBMCConfig(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);

extern int AMIGetPendStatus(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);

extern int AMIGetLifeCycleEvtLog(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);

#endif // __AMIRESTOREDEF_H__
