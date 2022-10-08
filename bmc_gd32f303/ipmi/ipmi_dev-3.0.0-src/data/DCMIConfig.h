/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2011, American Megatrends Inc.        **
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
 * DCMIConfig.h
 * DCMI Configuration related commands.
 *
 * Author: Muthuchamy K <muthusamyk@amiindia.co.in>
 *
 *****************************************************************/
#ifndef DCMICONFIG_H
#define DCMICONFIG_H
#include "Types.h"

typedef union
{
    INT8U   DHCPActivate;
    INT8U   DHCPDiscovery;
    INT8U   DHCPTiming1;
    INT16U   DHCPTiming2;
    INT16U   DHCPTimimg3;
}DCMIConfig_T;

typedef struct
{
    INT8U   GroupExtnID;
    INT8U   ParamSelector;
    INT8U   SetSelector;
}PACKED GetDCMIConfigParmasReq_T;

typedef struct
{
    INT8U   CompletionCode;
    INT8U   GroupExtnID;
    INT8U   DCMI_Ver_Maj;
    INT8U   DCMI_Ver_Min;
    INT8U   Revision;
}PACKED CCRevConfig_T;

typedef struct
{
    CCRevConfig_T   CCRevision;
    DCMIConfig_T    Config;
}PACKED GetDCMIConfigParmasRes_T;

typedef struct
{
    INT8U   GroupExtnID;
    INT8U   ParamSelector;
    INT8U   SetSelector;
    DCMIConfig_T    Config;
}PACKED SetDCMIConfigParmasReq_T;

typedef struct
{
    INT8U   CompletionCode;
    INT8U   GroupExtnID;
}PACKED SetDCMIConfigParmasRes_T;

#define DCMI_PARAM_DHCP_ACTIVATE    0x01
#define DCMI_PARAM_DHCP_DISCOVERY   0x02
#define DCMI_PARAM_DHCP_TIMING1         0x03
#define DCMI_PARAM_DHCP_TIMING2         0x04
#define DCMI_PARAM_DHCP_TIMING3         0x05

#define DCMI_CONFIG_REVISION                0x01

#define DHCP_ACTIVATE                             0x01
#define DHCP_DISCOVERY_RESERVED         0x7C

#define DCMIDHCPCONFIG_FILE             "/conf/dcmi.conf"
#define DCMIDHCPCONFIG_DEF_FILE         "/etc/defconfig/dcmi.conf"
#define DHCP_DISCOVERY          "discovery"
#define DHCP_TIMING1                "timing1"
#define DHCP_TIMING2                "timing2"
#define DHCP_TIMING3                "timing3"

#define NETWORK_RESTART     "/etc/init.d/networking restart"

extern int GetDCMIConfigParameters(INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetDCMIConfigParameters(INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);


#endif /* DCMICONFIG_H */

