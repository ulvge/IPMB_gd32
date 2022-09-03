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
extern int AMIGetServiceConf(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int AMISetServiceConf(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int AMIGetDNSConf(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int AMISetDNSConf(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);

/* DNS v6 */
extern int AMIGetV6DNSConf(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int AMISetV6DNSConf(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int AMISetActiveSlave(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst );
extern int AMILinkDownResilent( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMISetIfaceState(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMIGetIfaceState(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMISetFirewall ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst );
extern int AMIGetFirewall ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst );
extern int AMISetSNMPConf( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMIGetSNMPConf( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);

extern int AMIGetSELPolicy (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMISetSELPolicy (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetSELEntires(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIAddExtendSelEntries(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIPartialAddExtendSelEntries(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIPartialGetExtendSelEntries(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGETExtendSelData(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetSenforInfo(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetIPMISessionTimeOut(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetUDSInfo(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetUDSSessionInfo(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetRISConf(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int AMISetRISConf(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int AMIRISStartStop(_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,int BMCInst);
extern int AMIMediaRedirectionStartStop( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMIGetMediaInfo( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMISetMediaInfo( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMISetExtendedPrivilege(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMIGetExtendedPrivilege(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMIGetHostLockFeatureStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMISetHostLockFeatureStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetHostAutoLockStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMISetHostAutoLockStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetChannelType(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIPECIWriteRead (_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetRemoteKVMCfg (_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMISetRemoteKVMCfg (_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetRadiusConf (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMISetRadiusConf (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMIGetSSLCertStatus(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetLDAPConf (_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMISetLDAPConf (_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetADConf(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMISetADConf (_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetAllActiveSessions (_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIActiveSessionClose (_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetVmediaCfg(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMISetVmediaCfg(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetVideoRcdConf(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMISetVideoRcdConf(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetRunTimeSinglePortStatus(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMISetRunTimeSinglePortStatus(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIRestartWebService(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetBMCInstanceCount(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMIGetUSBSwitchSetting(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMISetUSBSwitchSetting(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMISwitchMUX( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);
extern int AMIGetRAIDInfo(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst);

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
