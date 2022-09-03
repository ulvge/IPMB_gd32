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
int InitYafuVar();
extern int AMIYAFUSwitchFlashDevice ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst);
extern int AMIYAFUActivateFlashDevice ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst);
extern int AMIYAFURestoreFlashDevice ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUGetFlashInfo ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUGetFirmwareInfo ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUGetFMHInfo ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUGetStatus ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUActivateFlashMode ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst );
extern int AMIYAFUDualImgSup(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst);
extern int AMIYAFUAllocateMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst ); 
extern int AMIYAFUFreeMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst ); 
extern int AMIYAFUReadFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUWriteFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst);
extern int AMIYAFUEraseFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUProtectFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUEraseCopyFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUGetECFStatus( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst);
extern int AMIYAFUGetVerifyStatus  ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst);
extern int AMIYAFUVerifyFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUReadMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUWriteMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst);
extern int AMIYAFUCopyMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUCompareMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst );
extern int AMIYAFUClearMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst ); 
extern int AMIYAFUGetBootConfig ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUSetBootConfig ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUGetBootVars ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUDeactivateFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUResetDevice ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIYAFUFWSelectFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst );
extern int AMIYAFUMiscellaneousInfo ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst );
#endif
extern int AMIFileUpload(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst);
extern int AMIFileDownload(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst);
extern int AMIDualImageSupport( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst );
extern int AMIGetNMChNum ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst); 
extern int AMIGetEthIndex ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst) ;
extern int AMIGetFruDetails( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst );
extern int AMIGetRootUserAccess (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMISetRootPassword   (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst);
extern int AMIGetUserShelltype   (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst);
extern int AMISetUserShelltype   (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst);

extern int AMISetTriggerEvent( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst );
extern int AMIGetTriggerEvent( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst );
extern int AMIGetSolConf( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst );
extern int AMISetLoginAuditConfig( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst );
extern int AMIGetLoginAuditConfig( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst );
extern int AMIGetAllIPv6Address ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst );
extern int AMISetPamOrder ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMIGetPamOrder ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMISetPwdEncryptionKey (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMISetUBootMemtest(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetUBootMemtestStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMISetTimeZone (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetTimeZone (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMISetNTPCfg(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetNTPCfg(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIYAFUReplaceSignedImageKey(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIVirtualDeviceGetStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIVirtualDeviceSetStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);

int GetUBootParam (char*, char* );
int SetUBootParam (char*, char* );
int GetAllUBootParam();
int DefaultSettingsForDualImageSupport(int BMCInst);
void CheckFirmwareChange(int BMCInst);

/**
 *    TFTP Firmware update 
 */
int AMIStartTFTPFwUpdate ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
int AMIGetTftpProgressStatus( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
int AMISetFWCfg ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
int AMIGetFWCfg ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
int AMISetFWProtocol ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
int AMIGetFWProtocol ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
int CheckForBootOption(char bootoption);

extern int AMIGetLicenseValidity(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIAddLicenseKey(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetFeatureStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst);

#endif  /* AMI_CMDS_H */

