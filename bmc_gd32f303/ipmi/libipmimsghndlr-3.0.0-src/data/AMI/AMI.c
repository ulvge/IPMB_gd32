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
 ****************************************************************
 *
 * AMI.c
 * AMI specific Command Handler
 *
 * Author: Basavaraj Astekar <basavaraja@ami.com>
 *
 *****************************************************************/
#define AMI_UNIMPLEMENTED_AS_FUNC

#include "Types.h"
#include "MsgHndlr.h"
#include "IPMI_AMI.h"
#include "AMIDevice.h"
#include "IPMI_AMIDevice.h"
#include "AMIFirmwareUpdate.h"
#include "IPMI_AMIFirmwareUpdate.h"
#include "AMISmtp.h"
#include "IPMI_AMISmtp.h"
#include "IPMI_AMIResetPass.h"
#include "AMIResetPass.h"
#include "AMIRestoreDefaults.h"
#include "AMISyslogConf.h"
#include "IPMI_AMISyslogConf.h"
#include "Support.h"
#include "AMIConf.h"
#include "IPMI_AMIConf.h"
#include "IPMI_AMIBiosCode.h"
#include "AMIBiosCode.h"
#include "versionmgt.h"
#include "flashlib.h"

/*** Global Variables ***/
ExCmdHndlrMap_T g_AMI_CmdHndlr [MAX_CMD_LIMIT] =
{
    /* Cmd */                                /*Privilege*/       /*Cmdhndlr*/                         /*ReqLen*/                              /*FFConfig*/ /*IfcSupport*/
#if AMI_DEVICE == 1
#ifndef CONFIG_SPX_FEATURE_IPMI_NO_YAFU_SUPPORT
    //Yafu related commands
    { CMD_AMI_YAFU_SWITCH_FLASH_DEVICE,        PRIV_ADMIN,        AMI_YAFU_SWITCH_FLASH_DEVICE,        sizeof(AMIYAFUSwitchFlashDeviceReq_T),     0xAAAA, AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_ACTIVATE_FLASH_DEVICE,      PRIV_ADMIN,        AMI_YAFU_ACTIVATE_FLASH_DEVICE,      sizeof(AMIYAFUActivateFlashDeviceReq_T),   0xAAAA, AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_RESTORE_FLASH_DEVICE,       PRIV_ADMIN,        AMI_YAFU_RESTORE_FLASH_DEVICE,       sizeof(AMIYAFUSwitchFlashDeviceReq_T),     0xAAAA, AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_GET_FLASH_INFO,             PRIV_ADMIN,        AMI_YAFU_GET_FLASH_INFO,             sizeof(AMIYAFUGetFlashInfoReq_T),          0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_GET_FIRMWARE_INFO,          PRIV_ADMIN,        AMI_YAFU_GET_FIRMWARE_INFO,          sizeof(AMIYAFUGetFirmwareInfoReq_T),       0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_GET_FMH_INFO,               PRIV_ADMIN,        AMI_YAFU_GET_FMH_INFO,               sizeof(AMIYAFUGetFMHInfoReq_T),            0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_GET_STATUS,                 PRIV_ADMIN,        AMI_YAFU_GET_STATUS,                 sizeof(AMIYAFUGetStatusReq_T),             0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_ACTIVATE_FLASH,             PRIV_ADMIN,        AMI_YAFU_ACTIVATE_FLASH,             sizeof(AMIYAFUActivateFlashModeReq_T),     0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_ALLOCATE_MEMORY,            PRIV_ADMIN,        AMI_YAFU_ALLOCATE_MEMORY,            sizeof(AMIYAFUAllocateMemoryReq_T),        0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_FREE_MEMORY,                PRIV_ADMIN,        AMI_YAFU_FREE_MEMORY,                sizeof(AMIYAFUFreeMemoryReq_T),            0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_READ_FLASH,                 PRIV_ADMIN,        AMI_YAFU_READ_FLASH,                 0xff,                                      0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_WRITE_FLASH,                PRIV_ADMIN,        AMI_YAFU_WRITE_FLASH,                0xff,                                      0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_ERASE_FLASH,                PRIV_ADMIN,        AMI_YAFU_ERASE_FLASH,                sizeof(AMIYAFUErashFlashReq_T),            0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_PROTECT_FLASH,              PRIV_ADMIN,        AMI_YAFU_PROTECT_FLASH,              sizeof(AMIYAFUProtectFlashReq_T),          0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_ERASE_COPY_FLASH,           PRIV_ADMIN,        AMI_YAFU_ERASE_COPY_FLASH,           sizeof(AMIYAFUEraseCopyFlashReq_T),        0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_VERIFY_FLASH,               PRIV_ADMIN,        AMI_YAFU_VERIFY_FLASH,               sizeof(AMIYAFUVerifyFlashReq_T),           0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_READ_MEMORY,                PRIV_ADMIN,        AMI_YAFU_READ_MEMORY,                sizeof(AMIYAFUReadMemoryReq_T),            0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_WRITE_MEMORY,               PRIV_ADMIN,        AMI_YAFU_WRITE_MEMORY,               0xff,                                      0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_COPY_MEMORY,                PRIV_ADMIN,        AMI_YAFU_COPY_MEMORY,                sizeof(AMIYAFUCopyMemoryReq_T),            0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_COMPARE_MEMORY,             PRIV_ADMIN,        AMI_YAFU_COMPARE_MEMORY,             sizeof(AMIYAFUCompareMemoryReq_T),         0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_CLEAR_MEMORY,               PRIV_ADMIN,        AMI_YAFU_CLEAR_MEMORY,               sizeof(AMIYAFUClearMemoryReq_T),           0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_GET_BOOT_CONFIG,            PRIV_ADMIN,        AMI_YAFU_GET_BOOT_CONFIG,            sizeof(AMIYAFUGetBootConfigReq_T),         0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_SET_BOOT_CONFIG,            PRIV_ADMIN,        AMI_YAFU_SET_BOOT_CONFIG,            0xff ,                                     0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_GET_BOOT_VARS,              PRIV_ADMIN,        AMI_YAFU_GET_BOOT_VARS,              0xff ,                                     0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_DEACTIVATE_FLASH_MODE,      PRIV_ADMIN,        AMI_YAFU_DEACTIVATE_FLASH_MODE,      sizeof(AMIYAFUDeactivateFlashReq_T),       0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_RESET_DEVICE,               PRIV_ADMIN,        AMI_YAFU_RESET_DEVICE ,              sizeof(AMIYAFUResetDeviceReq_T),           0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_GET_ECF_STATUS,             PRIV_ADMIN,        AMI_YAFU_GET_ECF_STATUS,             sizeof(AMIYAFUGetECFStatusReq_T),          0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_GET_VERIFY_STATUS,          PRIV_ADMIN,        AMI_YAFU_GET_VERIFY_STATUS,          sizeof(AMIYAFUGetVerifyStatusReq_T),       0xAAAA ,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_DUAL_IMAGE_SUP,             PRIV_ADMIN,        AMI_YAFU_DUAL_IMG_SUP,               sizeof(AMIYAFUDualImgSupReq_T),            0xAAAA,AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_FIRMWARE_SELECT_FLASH,      PRIV_ADMIN,        AMI_YAFU_FIRMWARE_SELECT_FLASH,      sizeof(AMIYAFUFWSelectFlashModeReq_T),     0xAAAA, AMI_DEVICE_SUP},
    { CMD_AMI_YAFU_MISCELLANEOUS_INFO,         PRIV_ADMIN,        AMI_YAFU_MISCELLANEOUS_INFO,         sizeof(AMIYAFUMiscellaneousReq_T),         0xAAAA, AMI_DEVICE_SUP},
#endif
    { CMD_AMI_FILE_UPLOAD,		PRIV_ADMIN,		AMI_FILE_UPLOAD,	0xff,		0xAAAA, AMI_DEVICE_SUP },
    { CMD_AMI_FILE_DOWNLOAD,		PRIV_ADMIN,		AMI_FILE_DOWNLOAD,	0xff,		0xAAAA, AMI_DEVICE_SUP },
    { CMD_AMI_GET_CHANNEL_NUM,              PRIV_USER,       AMI_GET_NM_CHANNEL_NUM,                       0x00,                                   0xAAAA ,0xFFFF},
    { CMD_AMI_GET_ETH_INDEX,                   PRIV_USER,         AMI_GET_ETH_INDEX,                   sizeof(AMIGetEthIndexReq_T),               0xAAAA ,0xFFFF},


    /*-------------------- AMI Smtp Commands -----------------------*/

   { CMD_SET_SMTP_CONFIG_PARAMS ,              PRIV_USER ,        SET_SMTP_CONFIG_PARAMS ,             0xFF,                                     0xAAAA ,0xFFFF},
   { CMD_GET_SMTP_CONFIG_PARAMS ,              PRIV_USER ,        GET_SMTP_CONFIG_PARAMS ,             sizeof (GetSMTPConfigReq_T ),             0xAAAA ,0xFFFF},

   { CMD_AMI_GET_EMAIL_USER,                   PRIV_USER,         AMI_GET_EMAIL_USER,                  sizeof(INT8U),                            0xAAAA, 0xFFFF },
   { CMD_AMI_SET_EMAIL_USER,                   PRIV_USER,         AMI_SET_EMAIL_USER,                  sizeof(AMISetUserEmailReq_T),             0xAAAA, 0xFFFF },
   { CMD_AMI_GET_EMAILFORMAT_USER,             PRIV_USER,         AMI_GET_EMAILFORMAT_USER,            sizeof(INT8U),                            0xAAAA, 0xFFFF },
   { CMD_AMI_SET_EMAILFORMAT_USER,             PRIV_USER,         AMI_SET_EMAILFORMAT_USER,            sizeof(AMISetUserEmailFormatReq_T),       0xAAAA, 0xFFFF },
   { CMD_AMI_RESET_PASS,                       PRIV_USER,         AMI_RESET_PASS,                      sizeof(AMIResetPasswordReq_T),            0xAAAA, 0xFFFF },
   { CMD_AMI_RESTORE_DEF,                      PRIV_ADMIN,        AMI_RESTORE_DEF,                     0xff,                                     0xAAAA, 0xFFFF },
   { CMD_AMI_GET_LOG_CONF,                     PRIV_USER,         AMI_GET_LOG_CONF,                    0x00,                                     0xAAAA, 0xFFFF },
   { CMD_AMI_SET_LOG_CONF,                     PRIV_ADMIN,        AMI_SET_LOG_CONF,                    0xff,                                     0xAAAA, 0xFFFF },
   
   /*-------------------- AMI Get Bios Code Commands -----------------------------------------*/
   { CMD_AMI_GET_BIOS_CODE,                    PRIV_USER,         AMI_GET_BIOS_CODE,                   sizeof (AMIGetBiosCodeReq_T),             0xAAAA, 0xFFFF },
   { CMD_AMI_SEND_TO_BIOS,                     PRIV_USER,         AMI_SEND_TO_BIOS,                    0xff,                                     0xAAAA, 0xFFFF },
   { CMD_AMI_GET_BIOS_COMMAND,                 PRIV_LOCAL,         AMI_GET_BIOS_COMMAND,               0x00,                                     0xAAAA, 0xFFFF },
   { CMD_AMI_SET_BIOS_RESPONSE,                PRIV_LOCAL,         AMI_SET_BIOS_RESPONSE,              0xff,                                     0xAAAA, 0xFFFF },
   { CMD_AMI_GET_BIOS_RESPONSE,                PRIV_USER,         AMI_GET_BIOS_RESPONSE,               sizeof (AMIGetBiosReponseSReq_T),         0xAAAA, 0xFFFF },
   { CMD_AMI_SET_BIOS_FLAG,                    PRIV_USER,         AMI_SET_BIOS_FLAG,                   sizeof (AMISetBiosFlagReq_T),             0xAAAA, 0xFFFF },
   { CMD_AMI_GET_BIOS_FLAG,                    PRIV_USER,         AMI_GET_BIOS_FLAG,                   0x00,                                     0xAAAA, 0xFFFF },

  /*---------------------AMI Firmware Command ------------------------------------------------*/
   {  CMD_AMI_GETRELEASENOTE,                  PRIV_USER,         AMI_GETRELEASENOTE,                  sizeof(AMIGetReleaseNoteReq_T),            0xAAAA, 0xFFFF},
   {  CMD_AMI_FIRMWAREUPDATE,                  PRIV_USER,         AMI_FIRMWAREUPDATE,                  0xff,                                      0xAAAA, 0xFFFF},


   /*-------------------- AMI SERVICE Commands -----------------------------------------*/
   { CMD_AMI_GET_SERVICE_CONF,                 PRIV_USER,         AMI_GET_SERVICE_CONF,                sizeof(AMIGetServiceConfReq_T),           0xAAAA  },
   { CMD_AMI_SET_SERVICE_CONF,                 PRIV_ADMIN,        AMI_SET_SERVICE_CONF,                sizeof(AMISetServiceConfReq_T),           0xAAAA  },
   { CMD_AMI_LINK_DOWN_RESILENT,               PRIV_ADMIN,        AMI_LINK_DOWN_RESILENT ,             sizeof(AMILinkDownResilentReq_T),         0xAAAA},
   
    /*-------------------- AMI DNS Commands -----------------------------------------*/
   { CMD_AMI_GET_DNS_CONF,                     PRIV_USER,         AMI_GET_DNS_CONF,                    sizeof(AMIGetDNSConfReq_T),               0xAAAA  },
   { CMD_AMI_SET_DNS_CONF,                     PRIV_ADMIN,        AMI_SET_DNS_CONF,                    0xff,                                     0xAAAA  },

   /*-------------------- AMI Iface State Commands -----------------------------------------*/
   { CMD_AMI_GET_IFACE_STATE,                  PRIV_OPERATOR,     AMI_GET_IFACE_STATE,                 sizeof(AMIGetIfaceStateReq_T),            0xAAAA },
   { CMD_AMI_SET_IFACE_STATE,                  PRIV_ADMIN,        AMI_SET_IFACE_STATE,                 0xff,                                     0xAAAA },
   
   { CMD_AMI_GET_FRU_DETAILS,                  PRIV_USER,         AMI_GET_FRU_DETAILS,                 sizeof(AMIGetFruDetailReq_T),             0xAAAA },
   { CMD_AMI_GET_ROOT_USER_ACCESS,             PRIV_OPERATOR,     AMI_GET_ROOT_USER_ACCESS,            0x00,                                     0xAAAA, 0xFFFF},
   { CMD_AMI_SET_ROOT_PASSWORD,                PRIV_ADMIN,        AMI_SET_ROOT_PASSWORD,               0xff,                                     0xAAAA, 0xFFFF},
   { CMD_AMI_GET_USER_SHELLTYPE,               PRIV_OPERATOR,     AMI_GET_USER_SHELLTYPE  ,            sizeof(AMIGetUserShelltypeReq_T)  ,       0xAAAA, 0xFFFF},
   { CMD_AMI_SET_USER_SHELLTYPE,               PRIV_ADMIN,        AMI_SET_USER_SHELLTYPE  ,            sizeof(AMISetUserShelltypeReq_T)  ,       0xAAAA, 0xFFFF},
   { CMD_AMI_SET_TRIGGER_EVT,                  PRIV_ADMIN,        AMI_SET_TRIGGER_EVT,                 0xFF,0xAAAA,0xFFFF},
   { CMD_AMI_GET_TRIGGER_EVT,                  PRIV_OPERATOR,     AMI_GET_TRIGGER_EVT,                 sizeof(AMIGetTriggerEventReq_T),          0xAAAA,0xFFFF},
   /*-------------------- AMI SOL Commands -----------------------------------------*/
   { CMD_AMI_GET_SOL_CONFIG_PARAMS,            PRIV_USER,         AMI_GET_SOL_CONF,                    0x00,                                     0xAAAA, 0xFFFF  },
   
   { CMD_AMI_SET_LOGIN_AUDIT_CFG,              PRIV_ADMIN,        AMI_SET_LOGIN_AUDIT_CFG,             sizeof(AMISetLoginAuditCfgReq_T),         0xAAAA, 0xFFFF    },
   { CMD_AMI_GET_LOGIN_AUDIT_CFG,              PRIV_USER,         AMI_GET_LOGIN_AUDIT_CFG,             0x00,                                     0xAAAA, 0xFFFF    },
   { CMD_AMI_GET_IPV6_ADDRESS,                 PRIV_ADMIN,        AMI_GET_IPV6_ADDRESS,                sizeof(AMIGetIPv6AddrReq_T),              0xAAAA, 0xFFFF    },
   { CMD_AMI_GET_CHANNEL_TYPE,                 PRIV_USER,         AMI_GET_CHANNEL_TYPE,                sizeof(AMIGetChannelTypeReq_T),           0xAAAA, 0xFFFF},
 
    /*------------------------------------------------- AMI SEL Commands -------------------------------------------------*/
   { CMD_AMI_GET_SEL_POLICY,                   PRIV_USER,         AMI_GET_SEL_POLICY,                  0x00,                                     0xAAAA, 0xFFFF },
   { CMD_AMI_SET_SEL_POLICY,                   PRIV_ADMIN,        AMI_SET_SEL_POLICY,                  sizeof(AMISetSELPolicyReq_T),             0xAAAA, 0xFFFF },
   { CMD_AMI_GET_SEL_ENTIRES,                  PRIV_USER,         AMI_GET_SEL_ENTRIES,                 sizeof(AMIGetSELEntriesReq_T) ,           0xAAAA, 0xFFFF},

    /*---------------------- AMI Sensor Info Commands ------------------------------------------------------*/
   { CMD_AMI_GET_SENSOR_INFO,                  PRIV_USER,         AMI_GET_SENSOR_INFO,                 0x00 ,                                    0xAAAA, 0xFFFF},

    /*---------------------- AMI TFTP Firmware Update Commands ------------------*/
   { CMD_AMI_GET_IPMI_SESSION_TIMEOUT,         PRIV_USER,         AMI_GET_IPMI_SESSION_TIMEOUT,        0x00,                                     0xAAAA, 0xFFFF},
   { CMD_AMI_GET_UDS_CHANNEL_INFO,             PRIV_USER,         AMI_GET_UDS_CHANNEL_INFO,            sizeof(AMIGetUDSInfoReq_T),               0xAAAA, 0xFFFF},
   { CMD_AMI_GET_UDS_SESSION_INFO,             PRIV_USER,         AMI_GET_UDS_SESSION_INFO,            0xFF,                                     0xAAAA, 0xFFFF},
   
    /*                                       AMI Dual Image Support  command                                       */
   { CMD_AMI_SET_UBOOT_MEMTEST,                 PRIV_ADMIN,             AMI_SET_UBOOT_MEMTEST,     sizeof(AMISetUBootMemtestReq_T),     0xAAAA, 0xFFFF  },
   { CMD_AMI_GET_UBOOT_MEMTEST_STATUS,      PRIV_ADMIN,             AMI_GET_UBOOT_MEMTEST_STATUS,           0x00,                0xAAAA, 0xFFFF  },

/*-------------------- AMI Remote Image server configuration Commands -----------------------------------------*/
   
   { CMD_AMI_MEDIA_REDIRECTION_START_STOP,		PRIV_ADMIN,		AMI_MEDIA_REDIRECTION_START_STOP,	0xFF/*sizeof(AMIMediaRedirctionStartStopReq_T)*/,	0xAAAA, 0xFFFF  },
   { CMD_AMI_GET_MEDIA_INFO,		PRIV_USER,		AMI_GET_MEDIA_INFO,		0xFF,	0xAAAA, 0xFFFF  },
   { CMD_AMI_SET_MEDIA_INFO,		PRIV_ADMIN,		AMI_SET_MEDIA_INFO,		0xFF,	0xAAAA, 0xFFFF  },
     /*----------------- AMI Control Debug Messages Commands--------------------------------------------------*/
    {CMD_AMI_YAFU_SIGNIMAGEKEY_REPLACE,PRIV_ADMIN, AMI_YAFU_SIGNIMAGEKEY_REPLACE,0xFF,0xAAAA,0xFFFF },

    { CMD_AMI_GET_REMOTEKVM_CONF,	PRIV_USER,		AMI_GET_REMOTEKVM_CONF,	sizeof(AMIGetRemoteKVMCfgReq_T),	0xAAAA, 0xFFFF  },
    { CMD_AMI_SET_REMOTEKVM_CONF,	PRIV_ADMIN,		AMI_SET_REMOTEKVM_CONF,	0xFF,								0xAAAA, 0xFFFF  },
    { CMD_AMI_GET_SSL_CERT_STATUS,	PRIV_USER,		AMI_GET_SSL_CERT_STATUS, sizeof(AMIGetSSLCertStatusReq_T)	,0xAAAA, 0xFFFF  },
    { CMD_AMI_GET_VMEDIA_CONF,	PRIV_USER,		AMI_GET_VMEDIA_CONF, sizeof(AMIGetVmediaCfgReq_T)	,0xAAAA, 0xFFFF  },
    { CMD_AMI_SET_VMEDIA_CONF,	PRIV_ADMIN,		AMI_SET_VMEDIA_CONF, sizeof(AMISetVmediaCfgReq_T)	,0xAAAA, 0xFFFF  },
    //{ CMD_AMI_GET_VIDEO_RCD_CONF,	PRIV_USER,	AMI_GET_VIDEO_RCD_CONF,	sizeof(AMIGetVideoRcdReq_T),	0xAAAA, 0xFFFF  },
    //{ CMD_AMI_SET_VIDEO_RCD_CONF,	PRIV_ADMIN,	AMI_SET_VIDEO_RCD_CONF,	0xFF,	0xAAAA, 0xFFFF  },
    { CMD_AMI_GET_FW_VERSION,PRIV_ADMIN,AMI_GET_FW_VERSION,sizeof(AMIGetFwVersionReq_T),0xAAAA, 0xFFFF},
    { CMD_AMI_GET_FEATURE_STATUS,	PRIV_ADMIN,		AMI_GET_FEATURE_STATUS,	0xff,	0xAAAA, 0xFFFF  },
    //{ CMD_AMI_SET_EXTLOG_CONF,    PRIV_ADMIN,     AMI_SET_EXTLOG_CONF,   sizeof(AMISetLogConfReq_T),    0xAAAA, 0xFFFF  },
   // { CMD_AMI_GET_EXTLOG_CONF,    PRIV_USER,      AMI_GET_EXTLOG_CONF,   0x0,    0xAAAA, 0xFFFF  },

    /*-------------------- AMI BMC Config Backup-Restore Commands -----------------------------------------*/
    { CMD_AMI_SET_BACKUP_FLAG,      PRIV_ADMIN, AMI_SET_BACKUP_FLAG,    sizeof(SetBackupFlagReq_T), 0xAAAA, 0xFFFF },
    { CMD_AMI_GET_BACKUP_FLAG,      PRIV_USER,  AMI_GET_BACKUP_FLAG,    0xFF,                       0xAAAA, 0xFFFF },
    { CMD_AMI_MANAGE_BMC_CONFIG,    PRIV_ADMIN, AMI_MANAGE_BMC_CONFIG,  0xFF,                       0xAAAA, 0xFFFF },
    { CMD_AMI_RESTART_WEB_SERVICE,     PRIV_ADMIN,             AMI_RESTART_WEB_SERVICE,      0x0,   0xAAAA, 0xFFFF  },
    { CMD_AMI_GET_PEND_STATUS,      PRIV_OPERATOR,  AMI_GET_PEND_STATUS,    sizeof(AMIGetPendStatusReq_T),    0xAAAA, 0xFFFF },
    { CMD_AMI_MUX_SWITCHING,     PRIV_ADMIN,             AMI_MUX_SWITCHING,     sizeof(AMISwitchMUXReq_T),   0xAAAA, 0xFFFF  },
#if defined(CONFIG_SPX_FEATURE_BMCCOMPANIONDEVICE_AST1070)
    /*-------------------------- AMI SMBMC Commands ----------------------------------------------*/
    { CMD_AMI_GET_BMC_INSTANCE_COUNT,    PRIV_ADMIN, AMI_GET_BMC_INSTANCE_COUNT,  0x00,   0xAAAA, 0xFFFF },
    { CMD_AMI_GET_USB_SWITCH_SETTING,    PRIV_ADMIN, AMI_GET_USB_SWITCH_SETTING,  0x00,   0xAAAA, 0xFFFF },
    { CMD_AMI_SET_USB_SWITCH_SETTING,    PRIV_ADMIN, AMI_SET_USB_SWITCH_SETTING,  sizeof(AMISetUSBSwitchSettingReq_T),   0xAAAA, 0xFFFF },

#endif
    { 0 , 0 , 0 , 0, 0 ,0}, // End of Table
#else
    { 0x00,                 0x00,           0x00,              0x00,                              0x0000 ,     0x0000}
#endif
};

