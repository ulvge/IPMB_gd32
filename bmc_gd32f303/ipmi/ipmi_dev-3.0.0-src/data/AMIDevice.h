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
 *****************************************************************
 *
 * AMICommands.h
 * AMI specific Commands
 *
 * Author: Basavaraj Astekar <basavaraja@ami.com>
 *
 *****************************************************************/
#ifndef AMI_DEVICE_H
#define AMI_DEVICE_H

#include "Types.h"

/**
 * @defgroup acf AMI Device commands
 * @ingroup acf
 * IPMI AMI-specific Command Handlers for
 * @{
 **/

#define SSH_TEMP_FILE   "/tmp/ssh_temp_key"


#define PAM_MODULE_LEN 64
#define PAM_BUF_LEN   128
#define  ERR_RETRN                  -1

#define CONNECTION_INFO_HOST            0x01
#define CONNECTION_INFO_FILE_PATH       0x02
#define CONNECTION_INFO_RETRY           0x03
#define FW_UPDATE_SELECT                0x04

#define ACTIVATE_FLASH_TIMEOUT 180

typedef struct
{
    INT8U  SqnceNo;
    INT8U  Enabled;
    char    PamModule[PAM_MODULE_LEN];
    char    ServiceName[PAM_MODULE_LEN];
}PamOrder_T;


#define PAM_IPMI   "pam_ipmi.so"
#define PAM_LDAP  "pam_custom_ldap.so"
#define PAM_AD      "pam_custom_ad.so"
#define PAM_RADIUS	"pam_custom_radius.so"
#define PAM_WOUNIX_FILE       "/conf/pam_wounix"
#define PAM_WITHUNIX_FILE   "/conf/pam_withunix"
#define PAM_NSSWITCH_FILE   "/conf/nsswitch.conf"
#define SRV_IPMI "ipmi"
#define SRV_LDAP "ldap"
#define SRV_AD "ad "
#define SRV_RADIUS "radius"
/*** Function Prototypes ***/
extern void FlashTimerTask (int BMCInst);
extern int SetDefaultTimezone (INT8U *TimeZone, int BMCInst);

/** @} */
#ifndef CONFIG_SPX_FEATURE_IPMI_NO_YAFU_SUPPORT
//int InitYafuVar();
extern int AMIYAFUSwitchFlashDevice ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst);
extern int AMIYAFUActivateFlashDevice ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst);
extern int AMIYAFURestoreFlashDevice ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUGetFlashInfo ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUGetFirmwareInfo ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUGetFMHInfo ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUGetStatus ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUActivateFlashMode ( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst );
extern int AMIYAFUDualImgSup(INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst);
extern int AMIYAFUAllocateMemory ( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst ); 
extern int AMIYAFUFreeMemory ( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst ); 
extern int AMIYAFUReadFlash ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUWriteFlash ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst);
extern int AMIYAFUEraseFlash ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUProtectFlash ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUEraseCopyFlash ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUGetECFStatus( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst);
extern int AMIYAFUGetVerifyStatus  ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst);
extern int AMIYAFUVerifyFlash ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUReadMemory ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUWriteMemory ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst);
extern int AMIYAFUCopyMemory ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUCompareMemory ( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst );
extern int AMIYAFUClearMemory ( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst ); 
extern int AMIYAFUGetBootConfig ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUSetBootConfig ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUGetBootVars ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUDeactivateFlash ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUResetDevice ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIYAFUFWSelectFlash ( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst );
extern int AMIYAFUMiscellaneousInfo ( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst );
#endif
extern int AMIFileUpload(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIFileDownload(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIDualImageSupport( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst );
extern int AMIGetNMChNum ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst); 
extern int AMIGetEthIndex ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst) ;
extern int AMIGetFruDetails( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst );
extern int AMIGetRootUserAccess (INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetRootPassword   (INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIGetUserShelltype   (INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMISetUserShelltype   (INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);

extern int AMISetTriggerEvent( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst );
extern int AMIGetTriggerEvent( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst );
extern int AMIGetSolConf( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst );
extern int AMISetLoginAuditConfig( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst );
extern int AMIGetLoginAuditConfig( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst );
extern int AMIGetAllIPv6Address ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst );
extern int AMISetPamOrder ( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIGetPamOrder ( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMISetPwdEncryptionKey (INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMISetUBootMemtest(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetUBootMemtestStatus(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMISetTimeZone (INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetTimeZone (INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMISetNTPCfg(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetNTPCfg(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIYAFUReplaceSignedImageKey(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIVirtualDeviceGetStatus(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIVirtualDeviceSetStatus(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);

int GetUBootParam (char*, char* );
int SetUBootParam (char*, char* );
//int GetAllUBootParam();
int DefaultSettingsForDualImageSupport(int BMCInst);
void CheckFirmwareChange(int BMCInst);

/**
 *    TFTP Firmware update 
 */
int AMIStartTFTPFwUpdate ( INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
int AMIGetTftpProgressStatus( INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
int AMISetFWCfg ( INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
int AMIGetFWCfg ( INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
int AMISetFWProtocol ( INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
int AMIGetFWProtocol ( INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
int CheckForBootOption(char bootoption);

extern int AMIGetLicenseValidity(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIAddLicenseKey(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetFeatureStatus(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);

#endif  /* AMI_CMDS_H */

