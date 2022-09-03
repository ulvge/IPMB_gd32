/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2012, American Megatrends Inc.        **
 ***                                                            **
 ***            All Rights Reserved.                            **
 ***                                                            **
 ***        6145-F, Northbelt Parkway, Norcross,                **
 ***                                                            **
 ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 ***                                                            **
 *****************************************************************
 *****************************************************************
 *
 * HPMConfig.h
 * HPM Configuration related commands.
 *
 * Author: Joey Chen <JoeyChen@ami.com.tw>
 *
 *****************************************************************/
#ifndef HPMCONFIG_H
#define HPMCONFIG_H

#include "Types.h"
#include "IPMI_HPMCmds.h"
#include "featuredef.h"
#include "fwinfo.h"

#define HPM_CONF_FILE "/conf/hpm.conf"
#define MAX_TEMP_ARRAY_SIZE         64

#define DEFAULT_MTD_DEVICE  "/dev/mtd0"
#define MAX_COMPONENTS          COMPONENT_ID_MAX
#define BOOT_COMPONENT_ID       COMPONENT_ID_0
#define APP_COMPONENT_ID        COMPONENT_ID_1

#define STR_SECTION_NAME_TARGETCAP "targetcap"

#define STR_UPGRADE_UNDERSIRABLE "upgrade_undesirable"
#define STR_ROLLBACK_OVERRIDE    "auto_rollback_override"
#define STR_IPMC_DEGRADED        "ipmc_degraded"
#define STR_DEFERRED_ACTIVATION  "deferred_activation"
#define STR_SERVICE_AFFECTED     "service_affected"
#define STR_MANUAL_ROLLBACK      "manual_rollback"
#define STR_AUTO_ROLLBACK        "auto_rollback"
#define STR_SELFTEST             "selftest"
#define STR_UPGRADE_TIMEOUT      "upgrade_timeout"
#define STR_SELFTEST_TIMEOUT     "selftest_timeout"
#define STR_ROLLBACK_TIMEOUT     "rollback_timeout"
#define STR_INACCESS_TIMEOUT     "inaccess_timeout"
#define STR_COMP_PRESENCE        "comp_presence"

#define STR_SECTION_NAME_COMPPROP_PREFIX "compprop"

#define STR_COLD_RESET_REQ       "cold_reset_req"
#define STR_COMPARISON           "comparison"
#define STR_PREPARATION          "preparation"
#define STR_ROLLBACK_BACKUP      "rollback_backup"   

#define HPM_IMAGE_1 1
#define HPM_IMAGE_2 2
extern CoreFeatures_T g_corefeatures;
extern char g_FlashingImage;

#pragma pack(1)

typedef struct 
{
    INT32U   Memoffset;
    INT32U   Flashoffset;  
    INT32U   Sizetocpy;
    INT32U   AllocateMemSize;
    INT8U    DescString[DESC_STRING_LEN]; 
} PACKED ComponentFlashInfo_T;

typedef struct
{
    union
    {
        unsigned char ByteField;
        struct
        {
            unsigned char SelfTest:1;
            unsigned char AutoRollback:1; 
            unsigned char ManualRollback:1;      
            unsigned char ServiceAffected:1;      
            unsigned char DeferredActivation:1;      
            unsigned char IPMCDegradedInUpg:1;        
            unsigned char AutoRollbackOverridden:1;        
            unsigned char FWUpgDesirable:1;       
        } PACKED BitField;
    } PACKED GlobalCap;                  
    INT8U UpgradeTimeout;
    INT8U SelfTestTimeout;
    INT8U Rollbackimeout;
    INT8U InaccessiblityTimeout;
    INT8U ComponentsPresent;
    
} PACKED HPMConfTargetCap_T;

typedef struct
{
    union
    {
        unsigned char ByteField;
        struct
        {
            unsigned char RollbackOrBackup         : 2;
            unsigned char Preparation              : 1;
            unsigned char Comparison               : 1;
            unsigned char DeferredActivation       : 1;
            unsigned char PayloadColdResetRequired : 1;
            unsigned char Reserved                 : 2;
        } PACKED BitField;
    } PACKED GeneralCompProp;
    
} PACKED HPMConfCompProp_T;

#pragma pack()

int IsValidComponentID(INT8U ComponentID);
BOOL IsIPMCComponentID(INT8U ComponentID);
BOOL IsContainIPMCComponents(INT8U Components);

BOOL IsValidComponents(INT8U Components);
BOOL IsOnlyOneComponent(INT8U Components);

int GetCompDescString(INT8U ComponentID, char *DescString, int StrSize);

BOOL IsFwVersionCached(INT8U ComponentID, FwVersionType_E FwVerType);
void ClearAllCachedFwVersion(void);
void CacheFwVersion(INT8U ComponentID, FwVersionType_E FwVerType);

int GetCurrFirmwareVersion(INT8U ComponentID, FirmwareVersion_T *FwVersion);
int GetRollbackFirmwareVersion(INT8U ComponentID, FirmwareVersion_T *FwVersion);
int GetDeferUpgFirmwareVersion(INT8U ComponentID, FirmwareVersion_T *FwVersion);

BOOL IsBackupComponentSupport(INT8U Components);
BOOL IsAutoRollbackSupport(INT8U ComponentID);
BOOL IsManualRollbackSupport(INT8U ComponentID);
BOOL IsDeferredActivationSupport(INT8U ComponentID);

int ReadAllHPMConf(void);
int GetTargetCapConf(HPMConfTargetCap_T *TargetCap);
int GetCompPropConf(HPMConfCompProp_T *CompProp, INT8U CompID);

int GetComponentFlashInfo(ComponentFlashInfo_T *ComponentFlashInfo, INT8U ComponentID);

#endif  /* HPMCONFIG_H */



