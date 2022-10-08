/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2010, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
*
* AMIConf.h
* AMI specific extended configuration commands 
*
* Author: Benson Chuang <bensonchuang@ami.com.tw>
*           
******************************************************************/

#ifndef __AMICONF_H__ 
#define __AMICONF_H__

#include "Types.h"
#include "ncml.h"


#define RIS_STOP		0
#define RIS_START		1
#define MAX_SERVICE_COUNT	3
#define SET_COMPLETE			0x00
#define SET_IN_PROGRESS 		0x01
#define WRITE_TO_FILE 		0x01

#define ENABLE_DISABLE_MASK 			0xFE

#define BONDING_ACTIVE	1
#define BONDING_NOTACTIVE 0

#define INVALID_PRIV_REQUEST 0xFFFFFFFC
#define INVALID_DOMAIN_NAME	0

extern char *ServiceNameList[MAX_SERVICE_NUM];
extern int AMIGetServiceConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetServiceConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetDNSConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetDNSConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);

/* DNS v6 */
extern int AMIGetV6DNSConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetV6DNSConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetActiveSlave(INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst );
extern int AMILinkDownResilent( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMISetIfaceState(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIGetIfaceState(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMISetFirewall ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst );
extern int AMIGetFirewall ( INT8U *pReq, INT32U ReqLen, INT8U *pRes ,int BMCInst );
extern int AMISetSNMPConf( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIGetSNMPConf( INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);

extern int AMIGetSELPolicy (INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMISetSELPolicy (INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetSELEntires(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIAddExtendSelEntries(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIPartialAddExtendSelEntries(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIPartialGetExtendSelEntries(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGETExtendSelData(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetSenforInfo(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetIPMISessionTimeOut(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetUDSInfo(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetUDSSessionInfo(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetRISConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetRISConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIRISStartStop(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIMediaRedirectionStartStop( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIGetMediaInfo( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMISetMediaInfo( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMISetExtendedPrivilege(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIGetExtendedPrivilege(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIGetHostLockFeatureStatus(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMISetHostLockFeatureStatus(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetHostAutoLockStatus(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMISetHostAutoLockStatus(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGetChannelType(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIPECIWriteRead (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetRemoteKVMCfg (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetRemoteKVMCfg (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetRadiusConf (INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMISetRadiusConf (INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIGetSSLCertStatus(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetLDAPConf (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetLDAPConf (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetADConf(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMISetADConf (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetAllActiveSessions (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIActiveSessionClose (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetVmediaCfg(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetVmediaCfg(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetVideoRcdConf(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetVideoRcdConf(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetRunTimeSinglePortStatus(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetRunTimeSinglePortStatus(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIRestartWebService(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetBMCInstanceCount(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMIGetUSBSwitchSetting(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISetUSBSwitchSetting(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst);
extern int AMISwitchMUX( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);
extern int AMIGetRAIDInfo(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst);

/*
 * @fn ValidateSetIfaceStateBond
 * @brief Validate the SetIface State Bond Command inupt
 * @param[in] pReq - Request Structure.
 * @param[out] pRes - Response Structure.
 * @param[out] VLANID - VLANID of the slave interfaces (if any, otherwise return 0).
 * @param[in] CheckBondDisable - Validate the Bond disable feature.
 * @param[in] BMCInst - Instance of the BMC.
 * @return 
 */
extern int ValidateSetIfaceStateBond (INT8U *pReq, INT8U *pRes, INT8U *VLANID, int CheckBondDisable, int BMCInst);

#endif /* __AMICONF_H__ */
