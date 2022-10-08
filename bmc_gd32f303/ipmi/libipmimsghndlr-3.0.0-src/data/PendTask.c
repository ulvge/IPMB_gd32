/****************************************************************
****************************************************************
**                                                            **
****************************************************************
*****************************************************************
*
* PendTask.c
*      Any IPMI command operation which requires more
*      response time can be posted to this task.
*
* Author: 
*
*****************************************************************/
#define ENABLE_DEBUG_MACROS    0
#include "Types.h"
#include "OSPort.h"
#include "Debug.h"
#include "Support.h"
#include "Message.h"
#include "MsgHndlr.h"
#include "IPMI_Main.h"
#include "IPMIDefs.h"
#include "SharedMem.h"
#include "AMI.h"
#include "App.h"
#include "Bridge.h"
#include "Chassis.h"
#include "Storage.h"
#include "SensorEvent.h"
#include "DeviceConfig.h"
#include "ChassisDevice.h"
#include "WDT.h"
#include "SDR.h"
#include "SEL.h"
#include "FRU.h"
#include "Sensor.h"
#include "SensorMonitor.h"
#include "FFConfig.h"
#include "NVRAccess.h"
#include "Platform.h"
#include "ipmi_hal.h"
#include "ipmi_int.h"
#include "AMIDevice.h"
#include "PendTask.h"
#include "nwcfg.h"
#include <unistd.h>
#include <sys/reboot.h>
#include <linux/if.h>
#include <linux/reboot.h>
#include"Ethaddr.h"
#include "LANIfc.h"
#include "PDKAccess.h"
#include "IPMI_AMIConf.h"
#include "IPMI_AMIDevice.h"
#include "LANConfig.h"
#include "hostname.h"
#include <sys/reboot.h>
#include <linux/reboot.h>
#include<sys/prctl.h>
#include "flshdefs.h"
#include "flashlib.h"
#include "flshfiles.h"
#include "ncml.h"
#include "libncsiconf.h"
#include "featuredef.h"
#include <linux/ip.h>
#include <libphyconf.h>
#include <dlfcn.h>
#include "safesystem.h"
#include "LANConfig.h"
#include "PDKCmdsAccess.h"
#include "AMIRestoreDefaults.h"
#include "hal_hw.h"
#include "Util.h"

#define NTPCONF_LIB "/usr/local/lib/libntpconf.so"
/*--------------------------------------------------------------------
* Global Variables
*--------------------------------------------------------------------*/



/*-----------------------------------------------------------------------------
* Function Prototypes
*-----------------------------------------------------------------------------*/
static PendCmdHndlrTbl_T* GetPendTblEntry (INT32U Operation);
static int PendSetIPAddress (INT8U *pData, INT32U DataLen,INT8U EthIndex,int BMCInst);
static int PendSetSubnet (INT8U *pData, INT32U DataLen,INT8U EthIndex,int BMCInst);
static int PendSetGateway (INT8U *pData, INT32U DataLen,INT8U EthIndex,int BMCInst);
static int PendSetSource (INT8U *pData, INT32U DataLen,INT8U EthIndex,int BMCInst);
static int PendDelayedColdReset (INT8U *pData, INT32U DataLen,INT8U EthIndex,int BMCInst);
static int PendSendEmailAlert (INT8U *pData, INT32U DataLen,INT8U EthIndex,int BMCInst);
static int PendSetRAIPAddr(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);
static int PendSetRAPrefixLen(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);

void ConvertIPnumToStr(unsigned char *var, unsigned int len,unsigned char *string);

/**
*@fn PendSetVLANIfcID
*@brief This function is invoked to set the valn id for the specified interface
*@param pData -  Pointer to buffer where network configurations saved
*@param DataLen -  unsigned integer parameter to buffer where length of the input data specified
*@param EthIndex -  char value to bufferr where index for Ethernet channel is saved
*/
static int PendSetVLANIfcID(INT8U* pData,INT32U DataLen,INT8U EthIndex,int BMCInst);

/**
*@fn PendDeConfigVLANInterface
*@brief This function is invoked to de-configure vlan sockets
*@param pData -  Pointer to buffer where network configurations saved
*@param DataLen -  unsigned integer parameter to buffer where length of the input data specified
*@param EthIndex -  char value to bufferr where index for Ethernet channel is saved
*/
static int PendDeConfigVLANInterface(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst);


/**
*@fn PendSetIPv4Headers
*@brief This function is invoked to set IPv4 headers.
*@param pData -  Pointer to buffer where network configurations saved
*@param DataLen -  unsigned integer parameter to buffer where length of the input data specified
*@param EthIndex -  char value to bufferr where index for Ethernet channel is saved
*@return Returns 0 on success
*/
int PendSetIPv4Headers(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst);

int PendSetRMCPPort(INT8U* pData,INT32U Datalen,INT8U EthIndex,int BMCInst);

static int PendSetAllDNSCfg(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst);

static int PendSetIPv6Cfg(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst);

static int PendSetEthIfaceState(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst);

static int PendSetMACAddress(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);


static int PendSetIPv6Enable(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);

static int PendSetIPv6Source(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);

static int PendSetIPv6Address(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);

static int PendSetIPv6PrefixLength(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);

static int PendSetIPv6Gateway(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);

static int PendSetDHCPv6TimingConf(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);

static int PendSetSLAACTimingConf(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);

static int PendConfigBonding(INT8U* pData,INT32U DataLen,INT8U EthIndex,int BMCInst);

static int PendActiveSlave(INT8U* pData,INT32U DataLen,INT8U Ethindex,int BMCInst);

static int PendRestartServices(INT8U * pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int PendStartFwUpdate_Tftp(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int PendSetNCSIChannelID(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int PendSetNCSIModeChange(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int PendSetNCSIVetoBit(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);
static int PendSetSpeed(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int PendSetMTUSize(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int PendSetNWPHYRegister(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int PendSetBlockAll(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int PendSetBlockAllTimeout(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int PendManageBMCBkupConfig(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int TriggerDelayedLANTimeout(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

static int ForcefulLANRestart(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

int PendSetVLANPriority(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst);

static int PendSetNCSIDetect(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);

void GetUpdatedCFG(DELAYEDLANRESTART_NWDATA *pDelyedLAN_NwCfgs, INT8U Ethindex, INT8U Flag_SetDNS, int BMCInst);

static int  PendSetIPv6StaticAddress (INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst);
static int PendSetIPv6IPv4AddrEnable(INT8U* pData, INT32U DataLen, INT8U EthIndex,int BMCInst);
static int PendSetIPv6Headers(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst);
static int PendSetNTPState(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst);


/*------------------ Pending Operation Table --------------------------*/
PendCmdHndlrTbl_T m_PendOperationTbl [2 * PEND_OP_MAX_HANDLE] = {};

CorePendCmdHndlrTbl_T m_CorePendCmdHndlrTbl [ PEND_OP_MAX_HANDLE ] = 
{
    /* Operation                    Handler      */
    { PEND_OP_SET_IP,               PendSetIPAddress            },
    { PEND_OP_SET_SUBNET,           PendSetSubnet               },
    { PEND_OP_SET_GATEWAY,          PendSetGateway              },
    { PEND_OP_SET_SOURCE,           PendSetSource               },
    { PEND_OP_DELAYED_COLD_RESET,   PendDelayedColdReset        },
    { PEND_OP_SEND_EMAIL_ALERT,     PendSendEmailAlert          },
    { PEND_OP_SET_VLAN_ID,          PendSetVLANIfcID            },
    { PEND_OP_DECONFIG_VLAN_IFC,    PendDeConfigVLANInterface   },
    { PEND_OP_SET_VLAN_PRIORITY,    PendSetVLANPriority         },
    { PEND_OP_SET_IPV4_HEADERS,     PendSetIPv4Headers          },
    { PEND_RMCP_PORT_CHANGE,        PendSetRMCPPort             },
    { PEND_OP_SET_ALL_DNS_CFG,      PendSetAllDNSCfg            },
    { PEND_OP_SET_IPV6_CFG,         PendSetIPv6Cfg              },
    { PEND_OP_SET_ETH_IFACE_STATE,      PendSetEthIfaceState    },
    { PEND_OP_SET_MAC_ADDRESS,		PendSetMACAddress	},
    { PEND_OP_SET_IPV6_ENABLE,      PendSetIPv6Enable   	},
    { PEND_OP_SET_IPV6_IP_ADDR_SOURCE,      PendSetIPv6Source   },
    { PEND_OP_SET_IPV6_IP_ADDR,     PendSetIPv6Address    	},
    { PEND_OP_SET_IPV6_PREFIX_LENGTH,     PendSetIPv6PrefixLength  },
    { PEND_OP_SET_DHCPV6_TIMING_CONF, PendSetDHCPv6TimingConf  },
    { PEND_OP_SET_SLAAC_TIMING_CONF,  PendSetSLAACTimingConf   },
    { PEND_OP_SET_IPV6_GATEWAY,     PendSetIPv6Gateway    	},
    { PEND_OP_SET_BOND_IFACE_STATE,       PendConfigBonding	},
    { PEND_OP_SET_ACTIVE_SLAVE,     PendActiveSlave		},
    { PEND_OP_RESTART_SERVICES,     PendRestartServices		},
    { PEND_OP_START_FW_UPDATE_TFTP, PendStartFwUpdate_Tftp	},
    { PEND_OP_SET_NCSI_CHANNEL_ID,  PendSetNCSIChannelID	},   
    { PEND_OP_SET_NCSI_MODE_CHANGE, PendSetNCSIModeChange	},
    { PEND_OP_SET_NCSI_VETOBIT,     PendSetNCSIVetoBit	},
    { PEND_OP_SET_SPEED,            PendSetSpeed },
    { PEND_OP_SET_MTU_SIZE,         PendSetMTUSize },
    { PEND_OP_SET_NW_PHY_REGISTER,  PendSetNWPHYRegister },
    { PEND_OP_SET_BLOCK_ALL,  PendSetBlockAll },
    { PEND_OP_SET_BLOCK_ALL_TIMEOUT,  PendSetBlockAllTimeout },
    { PEND_OP_MANAGE_BMC_BKUPCONFIG,    PendManageBMCBkupConfig },
    { TRIGGER_DELAYED_LAN_TIMEOUT,  TriggerDelayedLANTimeout },
    { FORCEFUL_LAN_RESTART,         ForcefulLANRestart },
    { PEND_OP_SET_NCSI_DETECT,      PendSetNCSIDetect },
    {PEND_OP_SET_IPV6_STATIC_IP_ADDR, PendSetIPv6StaticAddress},
    {PEND_OP_SET_IPV6_HEADERS, PendSetIPv6Headers},
    {PEND_OP_SET_IPV6_IPV4_ADDR_ENABLE,PendSetIPv6IPv4AddrEnable},
    { PEND_OP_SET_RA_IPADDR,    PendSetRAIPAddr },
    { PEND_OP_SET_RA_PREFIXLEN,    PendSetRAPrefixLen },
    { PEND_OP_SET_NTP_NTPSTATE,    PendSetNTPState }
};

PendCmdStatus_T m_PendStatusTbl [2 * PEND_OP_MAX_HANDLE] = {};

CorePendCmdStatus_T m_CorePendCmdStatus [ PEND_OP_MAX_HANDLE ] = 
{
    /* Operation                    Status */
    { PEND_OP_SET_IP ,              PEND_STATUS_COMPLETED },
    { PEND_OP_SET_SUBNET ,          PEND_STATUS_COMPLETED },
    { PEND_OP_SET_GATEWAY ,         PEND_STATUS_COMPLETED },
    { PEND_OP_SET_SOURCE ,          PEND_STATUS_COMPLETED },
    { PEND_OP_DELAYED_COLD_RESET ,  PEND_STATUS_COMPLETED },
    { PEND_OP_SEND_EMAIL_ALERT ,    PEND_STATUS_COMPLETED },
    { PEND_OP_SET_VLAN_ID ,         PEND_STATUS_COMPLETED },
    { PEND_OP_DECONFIG_VLAN_IFC,    PEND_STATUS_COMPLETED },
    { PEND_OP_SET_VLAN_PRIORITY,    PEND_STATUS_COMPLETED },
    { PEND_OP_SET_IPV4_HEADERS,     PEND_STATUS_COMPLETED },
    { PEND_RMCP_PORT_CHANGE,        PEND_STATUS_COMPLETED },
    { PEND_OP_SET_ALL_DNS_CFG,      PEND_STATUS_COMPLETED },
    { PEND_OP_SET_ETH_IFACE_STATE,      PEND_STATUS_COMPLETED },
    { PEND_OP_SET_MAC_ADDRESS,		PEND_STATUS_COMPLETED },
    { PEND_OP_SET_IPV6_ENABLE,		PEND_STATUS_COMPLETED },
    { PEND_OP_SET_IPV6_IP_ADDR_SOURCE,		PEND_STATUS_COMPLETED },
    { PEND_OP_SET_IPV6_IP_ADDR,		PEND_STATUS_COMPLETED },  
    { PEND_OP_SET_IPV6_PREFIX_LENGTH,		PEND_STATUS_COMPLETED },
    { PEND_OP_SET_IPV6_GATEWAY,		PEND_STATUS_COMPLETED },
    { PEND_OP_SET_DHCPV6_TIMING_CONF, PEND_STATUS_COMPLETED },
    { PEND_OP_SET_SLAAC_TIMING_CONF,  PEND_STATUS_COMPLETED },
    { PEND_OP_SET_BOND_IFACE_STATE,               PEND_STATUS_COMPLETED},
    { PEND_OP_SET_ACTIVE_SLAVE,        PEND_STATUS_COMPLETED},
    { PEND_OP_RESTART_SERVICES,		PEND_STATUS_COMPLETED},
    { PEND_OP_START_FW_UPDATE_TFTP,	PEND_STATUS_COMPLETED},
    { PEND_OP_SET_NCSI_CHANNEL_ID,	PEND_STATUS_COMPLETED},
    { PEND_OP_SET_NCSI_MODE_CHANGE,	PEND_STATUS_COMPLETED},
    { PEND_OP_SET_NCSI_VETOBIT,	PEND_STATUS_COMPLETED},
    { PEND_OP_SET_SPEED,            PEND_STATUS_COMPLETED },
    { PEND_OP_SET_MTU_SIZE,         PEND_STATUS_COMPLETED },
    { PEND_OP_SET_NW_PHY_REGISTER,  PEND_STATUS_COMPLETED },
    { PEND_OP_SET_BLOCK_ALL, PEND_STATUS_COMPLETED},
    { PEND_OP_MANAGE_BMC_BKUPCONFIG,    PEND_STATUS_COMPLETED },
    { TRIGGER_DELAYED_LAN_TIMEOUT,  PEND_STATUS_COMPLETED },
    { FORCEFUL_LAN_RESTART,         PEND_STATUS_COMPLETED },
    { PEND_OP_SET_NCSI_DETECT,      PEND_STATUS_COMPLETED },
    { PEND_OP_SET_IPV6_STATIC_IP_ADDR,      PEND_STATUS_COMPLETED },
    {PEND_OP_SET_IPV6_HEADERS,PEND_STATUS_COMPLETED},
    {PEND_OP_SET_IPV6_IPV4_ADDR_ENABLE,PEND_STATUS_COMPLETED},
    { PEND_OP_SET_RA_IPADDR,               PEND_STATUS_COMPLETED },
    { PEND_OP_SET_RA_PREFIXLEN,            PEND_STATUS_COMPLETED },
    { PEND_OP_SET_NTP_NTPSTATE,            PEND_STATUS_COMPLETED }
};

extern IfcName_T Ifcnametable[MAX_LAN_CHANNELS];
extern char *RestartServices[MAX_RESTART_SERVICE];
extern char *ModifyServiceNameList[MAX_SERVICES];
extern BondConf     m_NwBondInfo[MAX_BOND];

unsigned char g_NwRestartPending = 0; 
unsigned long ElapsedTime = 0;
static int g_CmdHndlrCnt = 0;
static int g_PendCmdStatusCnt = 0;

MONSETLANPARAM_STRUCT
	monitor_set_lan_parameter[] = {         {PEND_OP_SET_IP,				1},
                                            {PEND_OP_SET_SUBNET,			1},
                                            {PEND_OP_SET_GATEWAY,			1},
                                            {PEND_OP_SET_SOURCE,			1},
                                            {PEND_OP_SET_VLAN_ID,			1},
                                            {PEND_OP_DECONFIG_VLAN_IFC,		1},
                                            {PEND_OP_SET_VLAN_PRIORITY,		0},
                                            {PEND_OP_SET_BOND_IFACE_STATE,	1},
                                            {PEND_OP_SET_IPV6_ENABLE,		1},
                                            {PEND_OP_SET_IPV6_IP_ADDR_SOURCE,1},
                                            {PEND_OP_SET_IPV6_IP_ADDR,		1},
                                            {PEND_OP_SET_IPV6_PREFIX_LENGTH,1},
                                            {PEND_OP_SET_IPV6_GATEWAY,		1},
                                            {PEND_OP_SET_ETH_IFACE_STATE,	1},
                                            {PEND_OP_SET_ALL_DNS_CFG,		1},
                                            {PEND_OP_SET_SPEED,				0},
                                            {PEND_OP_SET_MTU_SIZE,			0},
                                            {PEND_OP_SET_NW_PHY_REGISTER,	0},
                                            {TRIGGER_DELAYED_LAN_TIMEOUT,	1},
                                            {PEND_OP_SET_IPV6_IPV4_ADDR_ENABLE,1},
                                            {PEND_OP_SET_IPV6_STATIC_IP_ADDR,1}
//TODO:                                     {PEND_OP_SET_RA_IPADDR, 0},
//TODO:                                     {PEND_OP_SET_RA_PREFIXLEN, 0},
                                            };

#define SET_SLAAC_TIMING_CONF(timing_param,granularity,initval,offset,value)                \
do                                                                                          \
{                                                                                           \
    if((timing_param == 0) || (offset == 2)||(offset == 3)||(offset == 4)||(offset == 5)||(offset == 7))   \
        value = timing_param;                                                               \
    else                                                                                    \
        value =((timing_param - 1) * granularity+initval) * HZ_JIFFIES;                     \
} while (0)

#define SET_DHCPV6_TIMING_CONF(timing_param,granularity,initval,offset,value)               \
do                                                                                          \
{                                                                                           \
    if((timing_param == 0) || (offset == 5) || (offset == 18) || (offset == 20) || (offset == 21))         \
        value = timing_param;                                                               \
    else                                                                                    \
        value =((timing_param - 1) * granularity+initval) * MILLI_SECS;                     \
} while (0)

int GetPHYConfig(char* ifcName, PHYConfig_T* phyCfg, unsigned short* mtuSize)
{
    void *pHandle = NULL;
    int (*pGetPHYConfig) (char*, PHYConfigFile_T*);
    PHYConfigFile_T tempPhyCfg;

    if (IsFeatureEnabled("CONFIG_SPX_FEATURE_PHY_SUPPORT") == ENABLED)
    {
    	memset(&tempPhyCfg, 0 ,sizeof(PHYConfigFile_T));

        pHandle = dlopen("libphyconf.so", RTLD_NOW);
        if (pHandle == NULL) return -1;
        
    	pGetPHYConfig = dlsym(pHandle, "getPHYConfigByName");

    	if (pGetPHYConfig == NULL || 
    		(pGetPHYConfig != NULL && pGetPHYConfig(ifcName, &tempPhyCfg)))
    	{
    		dlclose(pHandle);
    		return -1;
    	}

    	dlclose(pHandle);

        phyCfg->AutoNegotiationEnable = (INT8U)(tempPhyCfg.autoNegotiationEnable & 0x000000FF);
        phyCfg->Speed = (INT16U)(tempPhyCfg.speed & 0x0000FFFF);
        phyCfg->Duplex = (INT8U)(tempPhyCfg.duplex & 0x000000FF);
        *mtuSize = (INT16U)(tempPhyCfg.mtu & 0x0000FFFF);
    }

    return 0;
}

void GetUpdatedCFG(DELAYEDLANRESTART_NWDATA *pDelyedLAN_NwCfgs, INT8U Ethindex, INT8U Flag_SetDNS, int BMCInst)
{
	BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
	char IfcName[16],IfName[16];
	int i,j,k;
	INT8U netindex = 0xFF,EthIndex =0xff;
	NWCFG_STRUCT CFGIPv4,*pCfgIPv4;
	NWCFG6_STRUCT CFGIPv6,*pCfgIPv6;
	PHYConfig_T *pPHYConfig; 
	BondIface *pBondConfig;
	unsigned short * pMTUSize;
	HOSTNAMECONF *pHostnameConfig;
	DOMAINCONF *pDomainConfig;
	DNSCONF *pDnsIPConfig;
	INT8U *pRegBMC_FQDN;

	pCfgIPv4 = &pDelyedLAN_NwCfgs->cfgIPv4[0];
	pCfgIPv6 = &pDelyedLAN_NwCfgs->cfgIPv6[0];
	pBondConfig = &pDelyedLAN_NwCfgs->BondConfig;
	pPHYConfig = &pDelyedLAN_NwCfgs->PHYConfig[0];
	pMTUSize = &pDelyedLAN_NwCfgs->mtu_size[0];
	pHostnameConfig = &pDelyedLAN_NwCfgs->HostnameConfig;
	pDomainConfig = &pDelyedLAN_NwCfgs->DomainConfig;
	pDnsIPConfig = &pDelyedLAN_NwCfgs->DnsIPConfig;
	pRegBMC_FQDN = pDelyedLAN_NwCfgs->regBMC_FQDN;

	nwReadNWCfg_v4_v6(&CFGIPv4, &CFGIPv6, Ethindex, g_corefeatures.global_ipv6);
	GetNwStruct_v4_v6(pCfgIPv4, pCfgIPv6);
	unsigned char IPv6Addr[IP6_ADDR_LEN], IPv4Addr[IP_ADDR_LEN];
	memset (IPv6Addr, 0, IP6_ADDR_LEN);
	memset (IPv4Addr, 0, IP_ADDR_LEN);
	memcpy(pBondConfig, &pBMCInfo->BondConfig, sizeof(BondIface));
	
	if(Flag_SetDNS)
	{
		LOCK_BMC_SHARED_MEM(BMCInst);

		pDnsIPConfig->DNSEnable = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSEnable;
		pDnsIPConfig->DNSIndex = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIndex;
		pDnsIPConfig->DNSDHCP = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSDHCP;
		pDnsIPConfig->IPPriority = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.IPPriority;

		if(pDnsIPConfig->DNSDHCP == 1)
		{
			memset(pDnsIPConfig->DNSIP1,'\0',IP6_ADDR_LEN);
			memset(pDnsIPConfig->DNSIP2,'\0',IP6_ADDR_LEN);
			memset(pDnsIPConfig->DNSIP3,'\0',IP6_ADDR_LEN);
		}
		else
		{
			memcpy(pDnsIPConfig->DNSIP1,BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr1,IP6_ADDR_LEN);
			memcpy(pDnsIPConfig->DNSIP2,BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr2,IP6_ADDR_LEN);
			memcpy(pDnsIPConfig->DNSIP3,BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr3,IP6_ADDR_LEN);
		}

		//DOMAIN NAME
		pDomainConfig->EthIndex = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainIndex;
		pDomainConfig->v4v6 = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.Domainpriority; //To support both IPv4 and IPv6 in DNS_CONFIG.
		pDomainConfig->dhcpEnable = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainDHCP;
		if(pDomainConfig->dhcpEnable == 1)
		{
			pDomainConfig->domainnamelen = 0;
			memset(pDomainConfig->domainname,'\0',DNSCFG_MAX_DOMAIN_NAME_LEN);
		}
		else
		{
			pDomainConfig->domainnamelen = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainLen;
			if(pDomainConfig->domainnamelen <= MAX_DOMAIN_NAME_STRING_SIZE)
			{
				memset(&pDomainConfig->domainname , '\0', sizeof(pDomainConfig->domainname));
				strncpy((char *)&pDomainConfig->domainname, (char *)&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName, pDomainConfig->domainnamelen);
			}
		}

		//Register BMC Flag
		memcpy(pRegBMC_FQDN, BMC_GET_SHARED_MEM (BMCInst)->DNSconf.RegisterBMC, MAX_LAN_CHANNELS);

		UNLOCK_BMC_SHARED_MEM(BMCInst);
	}
	
    LOCK_BMC_SHARED_MEM(BMCInst);
    if(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostNameLen <= MAX_HOST_NAME_STRING_SIZE)
    {
        pHostnameConfig->HostNameLen = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostNameLen;
    }
    else
    {
        pHostnameConfig->HostNameLen = MAX_HOST_NAME_STRING_SIZE;
    }
    pHostnameConfig->HostSetting = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostSetting;

    memset(&pHostnameConfig->HostName , '\0', sizeof(pHostnameConfig->HostName));
    strncpy((char *)&pHostnameConfig->HostName, (char *)&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostName, pHostnameConfig->HostNameLen);
    UNLOCK_BMC_SHARED_MEM(BMCInst);
	
	sprintf(IfName,"bond%d",pBondConfig->BondIndex);
	unsigned short mtuSize = 0;

	for(i = 0 ;i < MAX_LAN_CHANNELS; i++)
	{
		if(strcmp(IfName, pBMCInfo->LanIfcConfig[i].ifname) == 0)
			continue;
		if((pBondConfig->Enable == 1) && ((1 << i) & (pBondConfig->Slaves)))
			continue;
		memcpy(IfcName, pBMCInfo->LanIfcConfig[i].ifname, sizeof(IfcName));
		if(IfcName[0] == 0)
			continue;

		for(j=0; j< sizeof(Ifcnametable)/sizeof(IfcName_T); j++)
		{
			if(strcmp(Ifcnametable[j].Ifcname, IfcName) == 0)
			{
				netindex = Ifcnametable[j].Index;
			}
			else
				continue;

			pPHYConfig[netindex].AutoNegotiationEnable = 1;
			pPHYConfig[netindex].Speed = 0xFFFF;
			pPHYConfig[netindex].Duplex = 0xFF;
			pMTUSize[netindex] = 1500;

			if (GetPHYConfig(IfcName, &pPHYConfig[netindex], &mtuSize) == 0)
			{
				pMTUSize[netindex] = mtuSize;
				mtuSize = 0;
			}

			pDelyedLAN_NwCfgs->TypeOfService[netindex] = pBMCInfo->LANCfs[i].Ipv4HdrParam.TypeOfService;
			if((pBMCInfo->LANCfs[i].IPAddrSrc == STATIC_IP_SOURCE) || (pBMCInfo->LANCfs[i].IPAddrSrc == DHCP_IP_SOURCE) ||
					(pBMCInfo->LANCfs[i].IPAddrSrc == BIOS_IP_SOURCE))
			{
				if(pBMCInfo->LANCfs[i].IPAddrSrc == BIOS_IP_SOURCE)
					pCfgIPv4[netindex].CfgMethod = STATIC_IP_SOURCE;
				else
					pCfgIPv4[netindex].CfgMethod = pBMCInfo->LANCfs[i].IPAddrSrc;
				if(pCfgIPv4[netindex].CfgMethod == STATIC_IP_SOURCE)
				{
					if(0 != memcmp(IPv4Addr, pBMCInfo->LANCfs[i].IPAddr, IP_ADDR_LEN))
					{
						memcpy (pCfgIPv4[netindex].IPAddr, pBMCInfo->LANCfs[i].IPAddr, IP_ADDR_LEN);
					}	
					if(0 != memcmp(IPv4Addr, pBMCInfo->LANCfs[i].SubNetMask, IP_ADDR_LEN))
					{
						memcpy (pCfgIPv4[netindex].Mask, pBMCInfo->LANCfs[i].SubNetMask, IP_ADDR_LEN);
					}
					if(0 != memcmp(IPv4Addr, pBMCInfo->LANCfs[i].DefaultGatewayIPAddr, IP_ADDR_LEN))
					{
						memcpy (pCfgIPv4[netindex].Gateway, pBMCInfo->LANCfs[i].DefaultGatewayIPAddr, IP_ADDR_LEN);
					}
				}
				pCfgIPv4[netindex].enable = pBMCInfo->LANCfs[i].IPv4_Enable;
				if((pBMCInfo->LANCfs[i].VLANID & VLAN_MASK_BIT) == VLAN_MASK_BIT)
				{
					pCfgIPv4[netindex].VLANID = pBMCInfo->LANCfs[i].VLANID & 0xfff;
					pCfgIPv4[netindex].vlanpriority = pBMCInfo->LANCfs[i].VLANPriority;
				}
				else
				{
					pCfgIPv4[netindex].VLANID = 0;
					pCfgIPv4[netindex].vlanpriority = 0;
				}
				pBMCInfo->LANConfig.VLANID[i] = pCfgIPv4[netindex].VLANID;
			}
			if((pBMCInfo->LANCfs[i].IPv6_IPAddrSrc == STATIC_IP_SOURCE) || (pBMCInfo->LANCfs[i].IPv6_IPAddrSrc == DHCP_IP_SOURCE))
			{
				
				if(pBMCInfo->LANCfs[i].IPv6_IPAddrSrc == STATIC_IP_SOURCE)
				{
					/*Validate the IPv6 address*/
					if(0 != memcmp(IPv6Addr, pBMCInfo->LANCfs[i].IPv6_IPAddr[0], IP6_ADDR_LEN))
					{
						memcpy(pCfgIPv6[netindex].GlobalIPAddr[0], pBMCInfo->LANCfs[i].IPv6_IPAddr[0], IP6_ADDR_LEN);
						pCfgIPv6[netindex].GlobalPrefix[0] = pBMCInfo->LANCfs[i].IPv6_PrefixLen[0];
					}
						
					/*Validate the IPv6 Gateway*/
					if(0 != memcmp(IPv6Addr, pBMCInfo->LANCfs[i].IPv6_GatewayIPAddr, IP6_ADDR_LEN))
					{
						memcpy(pCfgIPv6[netindex].Gateway, pBMCInfo->LANCfs[i].IPv6_GatewayIPAddr, IP6_ADDR_LEN);
					}

					for(k=1; k<16; k++)
					{
						if(g_corefeatures.ipv6_compliance_support)
					    {
					       	memcpy(pCfgIPv6[netindex].GlobalIPAddr[k],  pBMCInfo->LANCfs[i].IPv6Addrs[k].IPv6_Address, IP6_ADDR_LEN);
						    pCfgIPv6[netindex].GlobalPrefix[k] =  pBMCInfo->LANCfs[i].IPv6Addrs[k].IPv6_PrefixLength;
					    }
						else
						{
							memcpy(pCfgIPv6[netindex].GlobalIPAddr[k], pBMCInfo->LANCfs[i].IPv6_IPAddr[k], IP6_ADDR_LEN);
							pCfgIPv6[netindex].GlobalPrefix[k] = pBMCInfo->LANCfs[i].IPv6Addrs[k].IPv6_PrefixLength;
						}
					}
				}
				pCfgIPv6[netindex].CfgMethod = pBMCInfo->LANCfs[i].IPv6_IPAddrSrc;
				pCfgIPv6[netindex].enable = pBMCInfo->LANCfs[i].IPv6_Enable;
			}
			break;
		}
	}
	nwGetBondCfg();

	if((pBondConfig->Enable) || ((m_NwBondInfo[pBMCInfo->BondConfig.BondIndex].Enable) && !(pBMCInfo->BondConfig.Enable)))
	{
		sprintf(IfcName,"bond%d",pBondConfig->BondIndex);
		for(j=0;j<MAX_LAN_CHANNELS;j++)
		{
			if((strcmp(IfcName,pBMCInfo->LanIfcConfig[j].ifname) == 0))
			{
				EthIndex = pBMCInfo->LanIfcConfig[j].Ethindex;
				break;
			}
		}
		for(j=0;j<MAX_LAN_CHANNELS;j++)
		{
			if((strcmp(IfcName,Ifcnametable[j].Ifcname) == 0))
			{
				netindex = Ifcnametable[j].Index;
				break;
			}
		}
		pDelyedLAN_NwCfgs->TypeOfService[netindex] = pBMCInfo->LANCfs[EthIndex].Ipv4HdrParam.TypeOfService;
		pCfgIPv4[netindex].CfgMethod = pBMCInfo->LANCfs[EthIndex].IPAddrSrc;
		pCfgIPv4[netindex].enable = pBMCInfo->LANCfs[EthIndex].IPv4_Enable;
		pCfgIPv6[netindex].enable = pBMCInfo->LANCfs[EthIndex].IPv6_Enable;
		pCfgIPv6[netindex].CfgMethod = pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc;
		if(pCfgIPv4[netindex].CfgMethod == CFGMETHOD_STATIC)
		{
			if((pBMCInfo->LANCfs[EthIndex].IPAddr[0] != 0) || (pBMCInfo->LANCfs[EthIndex].IPAddr[1] != 0) || (pBMCInfo->LANCfs[EthIndex].IPAddr[2] != 0) || (pBMCInfo->LANCfs[EthIndex].IPAddr[3] != 0))
			{
				memcpy (pCfgIPv4[netindex].IPAddr, pBMCInfo->LANCfs[EthIndex].IPAddr, IP_ADDR_LEN);
			}	
			if((pBMCInfo->LANCfs[EthIndex].SubNetMask[0] != 0) || (pBMCInfo->LANCfs[EthIndex].SubNetMask[1] != 0) || (pBMCInfo->LANCfs[EthIndex].SubNetMask[2] != 0) || (pBMCInfo->LANCfs[EthIndex].SubNetMask[3] != 0))
			{
				memcpy (pCfgIPv4[netindex].Mask, pBMCInfo->LANCfs[EthIndex].SubNetMask, IP_ADDR_LEN);
			}
			if((pBMCInfo->LANCfs[EthIndex].DefaultGatewayIPAddr[0] != 0) || (pBMCInfo->LANCfs[EthIndex].DefaultGatewayIPAddr[1] != 0) || (pBMCInfo->LANCfs[EthIndex].DefaultGatewayIPAddr[2] != 0) || (pBMCInfo->LANCfs[EthIndex].DefaultGatewayIPAddr[3] != 0))
			{
				memcpy (pCfgIPv4[netindex].Gateway, pBMCInfo->LANCfs[EthIndex].DefaultGatewayIPAddr, IP_ADDR_LEN);
			}
		}
		if(pCfgIPv6[netindex].CfgMethod == CFGMETHOD_STATIC)
		{
			memcpy(pCfgIPv6[netindex].GlobalIPAddr[0], &pBMCInfo->LANCfs[EthIndex].IPv6_IPAddr[0], (MAX_IPV6ADDRS * IP6_ADDR_LEN));
			memcpy(pCfgIPv6[netindex].GlobalPrefix, pBMCInfo->LANCfs[EthIndex].IPv6_PrefixLen, MAX_IPV6ADDRS );
			memcpy(pCfgIPv6[netindex].Gateway, pBMCInfo->LANCfs[EthIndex].IPv6_GatewayIPAddr, IP6_ADDR_LEN);
		}
		if((pBMCInfo->LANCfs[EthIndex].VLANID & VLAN_MASK_BIT) == VLAN_MASK_BIT)
		{
			pCfgIPv4[netindex].VLANID = pBMCInfo->LANCfs[EthIndex].VLANID & 0xfff;
			pCfgIPv4[netindex].vlanpriority = pBMCInfo->LANCfs[EthIndex].VLANPriority;
		}
		else
		{
			pCfgIPv4[netindex].VLANID = 0;
			pCfgIPv4[netindex].vlanpriority = 0;
		}
		pBMCInfo->LANConfig.VLANID[netindex] = pCfgIPv4[netindex].VLANID;
	}
	return;
}

int InitPendCmdHndlrTable()
{
    int i;
    static int g_InitPendCmdHndlr;

    if(g_InitPendCmdHndlr)
        return 0;

    for(i = 0; i < sizeof(m_CorePendCmdHndlrTbl)/sizeof(m_CorePendCmdHndlrTbl[0]); i++)
    {
        m_PendOperationTbl[g_CmdHndlrCnt].Operation = m_CorePendCmdHndlrTbl[i].Operation;
        m_PendOperationTbl[g_CmdHndlrCnt].PendHndlr = m_CorePendCmdHndlrTbl[i].PendHndlr;
        g_CmdHndlrCnt++;
    }

    g_InitPendCmdHndlr = 1;
    return 0;
}

int InitPendCmdStatus()
{
    int i;
    static int g_InitPendCmdStatus = 0;

    if(g_InitPendCmdStatus)
        return 0;

    for(i = 0; i < sizeof(m_CorePendCmdStatus) / sizeof(m_CorePendCmdStatus[0]); i++)
    {
        m_PendStatusTbl[g_PendCmdStatusCnt].Action = m_CorePendCmdStatus[i].Operation;
        m_PendStatusTbl[g_PendCmdStatusCnt].PendStatus = m_CorePendCmdStatus[i].PendStatus;
        g_PendCmdStatusCnt++;
    }

    g_InitPendCmdStatus = 1;
    return 0;
}

int UpdateOEMCmdHndlr(INT32U Operation, pPendCmdHndlr_T PendHndlr)
{

    /*Initialize the core Pend Task table*/
    InitPendCmdHndlrTable();

    m_PendOperationTbl[g_CmdHndlrCnt].Operation = Operation;
    m_PendOperationTbl[g_CmdHndlrCnt].PendHndlr = PendHndlr;
    g_CmdHndlrCnt++;

    return 0;
}

int UpdateOEMPendStatus(INT32U Operation, int PendStatus)
{
    /*Initialize the Core Pend Status Table*/
    InitPendCmdStatus();

    m_PendStatusTbl[g_PendCmdStatusCnt].Action = Operation;
    m_PendStatusTbl[g_PendCmdStatusCnt].PendStatus = PendStatus;
    g_PendCmdStatusCnt++;

    return 0;
}

/**
 * PendCmdTask
 *
 * @brief IPMI Command Pending Task.
**/
void* PendCmdTask (void *pArg)
{
    int *inst = (int*) pArg;
    int BMCInst = *inst;
    MsgPkt_T                MsgPkt;
    PendCmdHndlrTbl_T       *pPendTblEntry = NULL;
    INT8U EthIndex,netindex = 0xFF,Ethindex = 0xff,i,Channel = 1,Flag_SetDNS = 0;
    char IfcName[16];
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    unsigned long configured_timeout = 0;
    unsigned char IsLANCmd = 0;
    INT8S   Ifcname[IFNAMSIZ];
    int retval, retValue, j;
    prctl(PR_SET_NAME,__FUNCTION__,0,0,0);
	DELAYEDLANRESTART_NWDATA DelyedLAN_NwCfgs;
    IPMI_DBG_PRINT("Pending Task Started\n");
    if(g_corefeatures.delayed_lan_restart_support)
    {
        configured_timeout = g_coremacros.delayed_lan_restart_timeout;
    }

    InitPendCmdHndlrTable();
    InitPendCmdStatus();

    /*Update OEM Pend Task Table*/
    if( g_PDKCmdsHandle[PDKCMDS_OEMPENDTASKINIT] != NULL)
    {
        ((int(*)())(g_PDKCmdsHandle[PDKCMDS_OEMPENDTASKINIT]))();
    }

    /* Process Task from PendTask Q */
    while (1)
    {
        if((g_corefeatures.delayed_lan_restart_support) && g_NwRestartPending)
        {
            retval = GetMsg (&MsgPkt, PEND_TASK_Q, 1, BMCInst);
            if(ElapsedTime >= configured_timeout)
            {
                /* Included for Debugging and it will be removed after one or two releases.*/
                TINFO ("*******************LAN Task Timeout happens***************");
                for(j = 0; j < MAX_LAN_CHANNELS; j++)
                {
                    if ((pBMCInfo->LanIfcConfig[j].ifname[0] == 0) || (NULL != strstr(pBMCInfo->LanIfcConfig[j].ifname, "bond")))
                        continue;
                    
                    TINFO(" %s : ipv4_enable %d, ipv6_enable %d, ipaddr %d.%d.%d.%d, source %d, "
                            "LANCFs VLANID: %x, LANCONFIG VLANID : %d", pBMCInfo->LanIfcConfig[j].ifname,
                            pBMCInfo->LANCfs[j].IPv4_Enable, pBMCInfo->LANCfs[j].IPv6_Enable,
                            pBMCInfo->LANCfs[j].IPAddr[0], pBMCInfo->LANCfs[j].IPAddr[1], 
                            pBMCInfo->LANCfs[j].IPAddr[2], pBMCInfo->LANCfs[j].IPAddr[3],
                            pBMCInfo->LANCfs[j].IPAddrSrc,  pBMCInfo->LANCfs[j].VLANID, 
                            pBMCInfo->LANConfig.VLANID[j]);
                }
                TINFO ("Bond Enable : %d", pBMCInfo->BondConfig.Enable);
                memset(&DelyedLAN_NwCfgs, 0, sizeof(DELAYEDLANRESTART_NWDATA));
                GetUpdatedCFG(&DelyedLAN_NwCfgs, netindex, Flag_SetDNS, BMCInst);
                memset(Ifcname,0,sizeof(Ifcname));
                sprintf(Ifcname,"bond%d",pBMCInfo->BondConfig.BondIndex);
                for(i=0;i<MAX_LAN_CHANNELS;i++)
                {
                    if(strcmp(pBMCInfo->LanIfcConfig[i].ifname,Ifcname) == 0)
                    {
                        Channel=pBMCInfo->LanIfcConfig[i].Chnum;
                    }
                }
                Ethindex = GetEthIndex(Channel, BMCInst);
                HandleDelayedNwRestart(&DelyedLAN_NwCfgs, Flag_SetDNS,Ethindex);
                if (g_PDKHandle[PDK_AFTERDELAYEDLANRESTART] != NULL )
                {
                    retValue = ((int(*)(int))(g_PDKHandle[PDK_AFTERDELAYEDLANRESTART]))(BMCInst);
                    if(retValue != 0)
                    {
                        TCRIT("Error setting eth information retValue %d \n",retValue);
                    }
                }
                InitDNSConfiguration(BMCInst);
                g_NwRestartPending = 0;
                Flag_SetDNS = 0;
            }
            if(retval != 0)
                continue;
        }
        else
        {
            /* Wait for any Pending Operation */
            if (0 != GetMsg (&MsgPkt, PEND_TASK_Q, WAIT_INFINITE, BMCInst))
            {
                IPMI_WARNING ("PendTask.c : Error fetching messages from PenTaskQ\n");
                continue;
            }
        }
        if(g_corefeatures.delayed_lan_restart_support)
        {
            for(j=0;j<sizeof(monitor_set_lan_parameter)/sizeof(MONSETLANPARAM_STRUCT);j++)
            {
                if(MsgPkt.Param == monitor_set_lan_parameter[j].param)
                {
                    IsLANCmd = 1;	
                    if(1 == monitor_set_lan_parameter[j].NwRestartNeeded)
                    {
                        ElapsedTime = 0;
                        g_NwRestartPending = 1;
                    }
                    if(MsgPkt.Param == PEND_OP_SET_ALL_DNS_CFG)
                        Flag_SetDNS = 1;
                    break;
                }
            }
        }
        /* Get appropriate Pending Table Entry */
        pPendTblEntry = GetPendTblEntry (MsgPkt.Param);

        /* Check if match found  */
        if (pPendTblEntry == NULL)
        {
            /* If not in core it could be an handler in PDK */
            //pPendTblEntry = PDK_GetPendTblEntry (MsgPkt.Param);

            /* If not present in PDK then unknown operation */
            if (pPendTblEntry == NULL)
            {
                IPMI_WARNING ("PendTask.c : Unknown Pending Operation\n");
                continue;
            }
        }

        /* Hanlde Operation */
        if (pPendTblEntry->PendHndlr == NULL)
        {
            IPMI_WARNING ("PendTask.c : Unknown Operation Handler\n");
            continue;
        }

        EthIndex=GetEthIndex(MsgPkt.Channel, BMCInst);

        /*we should not pass this index value to libnetwork library.
        we can use this index value which is related to ipmi. 
        It will be useful in bonding mode*/
        pBMCInfo->LANConfig.g_ethindex = EthIndex;

        /*Get the EthIndex*/
        if(GetIfcName(EthIndex,IfcName, BMCInst) == -1)
        {
            TCRIT("Error in Getting Ethindex\n");
        }

        for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
        {
            if(strcmp(Ifcnametable[i].Ifcname,IfcName) == 0)
            {
                netindex= Ifcnametable[i].Index;
                break;
            }
        }

        /*if(netindex == 0xFF)
        {
            TCRIT("Error in getting Ethindex\n");
            continue;
        }*/
        /* Operation Done !! */
        if((g_corefeatures.delayed_lan_restart_support) && (IsLANCmd) && g_NwRestartPending)
        {
            SetPendStatus(MsgPkt.Param, PEND_STATUS_COMPLETED);
            IsLANCmd = 0;
        }
        else
        {
            pPendTblEntry->PendHndlr (MsgPkt.Data, MsgPkt.Size,netindex,BMCInst);
        }
    }

}

/**
 * PostPendTask
 *
 * @brief Post A Msg to Pending Task Table.
**/
int
PostPendTask (PendTaskOperation_E Operation, INT8U *pData, INT32U DataLen,INT8U Channel,int BMCInst)
{
    MsgPkt_T    MsgPkt;

    /* Fill Operation information */
    MsgPkt.Param = Operation;
    memcpy (MsgPkt.Data, pData, DataLen);
    MsgPkt.Size = DataLen;
    MsgPkt.Channel= Channel;


    /* Post to Pending Task Q */
    if (0 != PostMsg (&MsgPkt, PEND_TASK_Q,BMCInst))
    {
        IPMI_WARNING ("PendTask.c : Unable to post to PendTask Q\n");
        return -1;
    }

    return 0;
}



/**
 * GetPendTblEntry
 *
 * @brief Fetches an handler from Pending Table.
**/
PendCmdHndlrTbl_T*
GetPendTblEntry (PendTaskOperation_E Operation)
{
    int i = 0;

    /*  Search through the pending table and find a match */
    for (i = 0; i < sizeof (m_PendOperationTbl)/ sizeof (m_PendOperationTbl[0]); i++)
    {
        if (m_PendOperationTbl [i].Operation == Operation)
        {
            return (PendCmdHndlrTbl_T*)&m_PendOperationTbl[i];
        }
    }

    /* Match not found !!! */
    return NULL;
}

/*
* @fn SetPendStatus
* @brief This function updates the status of Pend Task Operation
* @param Action -  Pend Task Operation to update
* @param Status - Status of the Pend Task Operation
* @return Returns 0 on success
*             Returns -1 on failure
*/
int SetPendStatus (PendTaskOperation_E Action, int Status)
{
    int i = 0;

    for(i=0;i < sizeof(m_PendStatusTbl)/ sizeof(m_PendStatusTbl[0]);i++)
    {
        if(m_PendStatusTbl[i].Action == Action)
        {
            m_PendStatusTbl [i].PendStatus = Status;
            return 0;
        }
    }

    /* Match not found !!!*/
    return -1;
}

/*
* @fn SetPendStatusError
* @brief This function updates the status of Pend Task Operation
* @param Action -  Pend Task Operation to update
* @param ErrorCode  - Error code of the PendTask
* @return Returns 0 on success
*             Returns -1 on failure
*/
int SetPendStatusError(PendTaskOperation_E Action, int ErrorCode)
{
    int i = 0;

    for(i = 0; i < sizeof(m_PendStatusTbl)/ sizeof(m_PendStatusTbl[0]);i++)
    {
        if(m_PendStatusTbl[i].Action == Action)
        {
            m_PendStatusTbl [i].PendStatus = ErrorCode << 8 | PEND_STATUS_ERROR;
            return 0;
        }
    }

    /* Match not found !!!*/
    return -1;
}

/*
* @fn GetPendStatus
* @brief This function retrives the status of Pend Task Operation
* @param Action -  Pend Task Operation to update
* @return Returns Pend Task Operation Status
*             Returns -1 on failure
*/
int GetPendStatus (PendTaskOperation_E Action)
{
    int i=0;

    for(i=0;i < sizeof(m_PendStatusTbl)/ sizeof(m_PendStatusTbl[0]);i++)
    {
        if(m_PendStatusTbl[i].Action == Action)
        {
            return m_PendStatusTbl[i].PendStatus;
        }
    }

    /* Match not found*/
    return -1;
}

/**
 * PendSetIPAddress
 *
 * @brief Set IP Address.
**/
int PendSetIPAddress (INT8U *pData, INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;

    //nwReadNWCfg(&NWConfig,EthIndex);
    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);
    memcpy (NWConfig.IPAddr, ((NWCFG_STRUCT*)pData)->IPAddr, IP_ADDR_LEN);
    //nwWriteNWCfg (&NWConfig,EthIndex);
    nwWriteNWCfg_ipv4_v6 ( &NWConfig, &NWConfig6, EthIndex);
    SetPendStatus (PEND_OP_SET_IP,PEND_STATUS_COMPLETED);

    return 0;
}


int PendSetSubnet(INT8U* pData,INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;

    //nwReadNWCfg(&NWConfig,EthIndex);
    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);
    memcpy (NWConfig.Mask,  ((NWCFG_STRUCT*)pData)->Mask, IP_ADDR_LEN);
    //nwWriteNWCfg(&NWConfig,EthIndex);
    nwWriteNWCfg_ipv4_v6 ( &NWConfig, &NWConfig6, EthIndex);
    SetPendStatus (PEND_OP_SET_SUBNET,PEND_STATUS_COMPLETED);

    return 0;
}

int PendSetGateway(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    NWCFG_STRUCT    NWConfig;
    NWCFG6_STRUCT        NWConfig6;    
    unsigned char  NullGateway[4];
    INT32U GwIP;

    memset(NullGateway,0,IP_ADDR_LEN);

    //nwReadNWCfg(&NWConfig,EthIndex);
    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);    
    if(memcmp(((NWCFG_STRUCT*)pData)->Gateway,NullGateway,IP_ADDR_LEN)== 0)
    {
        nwGetBkupGWyAddr(((NWCFG_STRUCT*)pData)->Gateway,EthIndex);
    }
    memcpy (NWConfig.Gateway,((NWCFG_STRUCT*)pData)->Gateway, IP_ADDR_LEN);
    if(NWConfig.CfgMethod == CFGMETHOD_DHCP)
    {
       nwDelExistingGateway(EthIndex);
       memcpy((INT8U*)&GwIP,NWConfig.Gateway,IP_ADDR_LEN);
       nwSetGateway((INT8U*)&NWConfig.Gateway,EthIndex);
    }
    else
    {
        //nwWriteNWCfg(&NWConfig,EthIndex);
        nwWriteNWCfg_ipv4_v6 ( &NWConfig, &NWConfig6, EthIndex);
    }

    SetPendStatus (PEND_OP_SET_GATEWAY,PEND_STATUS_COMPLETED);

    return 0;
}

int PendSetSource(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;        
    DOMAINCONF      DomainCfg;
    DNSCONF     DNS;
    INT8U               regBMC_FQDN[MAX_LAN_CHANNELS];
    int i;

    memset(&DomainCfg,0,sizeof(DOMAINCONF));
    memset(&DNS,0,sizeof(DNSCONF));
    memset(regBMC_FQDN,0,sizeof(regBMC_FQDN));

    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);    
    NWConfig.CfgMethod = ((NWCFG_STRUCT*)pData)->CfgMethod;
    if(NWConfig.CfgMethod == CFGMETHOD_STATIC)
    {
        ReadDNSConfFile(&DomainCfg, &DNS,regBMC_FQDN);

        if(DomainCfg.EthIndex == EthIndex)
        {
            if(DomainCfg.v4v6 == 1)
                DomainCfg.dhcpEnable= 0;

            if(DNS.DNSDHCP == 1)
                DNS.DNSDHCP = 0;

            for(i=0;i<MAX_LAN_CHANNELS;i++)
            {
                if((regBMC_FQDN[i] & 0x10) == 0x10)
                    regBMC_FQDN[i] |=0x00;
            }
        }
        else
        {
            regBMC_FQDN[EthIndex] |= 0x0;
        }

        WriteDNSConfFile(&DomainCfg, &DNS, regBMC_FQDN);

        /*If the state changed to static, get the OEM configured static address from the Hook*/
        if(g_PDKHandle[PDK_GETSTATICNWCFG] != NULL)
        {
            ((int(*)(INT8U*, INT8U*, INT8U, int))g_PDKHandle[PDK_GETSTATICNWCFG]) ((INT8U *)&NWConfig, (INT8U *)&NWConfig6, EthIndex, BMCInst);
        }
    }
    nwWriteNWCfg_ipv4_v6 ( &NWConfig, &NWConfig6, EthIndex);    
    SetPendStatus (PEND_OP_SET_SOURCE,PEND_STATUS_COMPLETED);

    return 0;
}


int PendDelayedColdReset (INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    OS_TIME_DELAY_HMSM (0, 0, 5, 0);
    /* PDK Module Post Set Reboot Cause*/
    if(g_PDKHandle[PDK_SETREBOOTCAUSE] != NULL)
    {
	((INT8U(*)(INT8U,int)) g_PDKHandle[PDK_SETREBOOTCAUSE])(SETREBOOTCAUSE_IPMI_CMD_PROCESSING,BMCInst);
    }
	
    reboot (LINUX_REBOOT_CMD_RESTART);
    SetPendStatus (PEND_OP_DELAYED_COLD_RESET,PEND_STATUS_COMPLETED);
    return 0;
}
 int  PendSendEmailAlert (INT8U *pData, INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    /* Send the Email Alert */
    SetExtCfgReq_T* pSetCfgReq  =  ( SetExtCfgReq_T* ) pData;

    IPMI_DBG_PRINT_1 ("Send Email Alert  - %d \n",pSetCfgReq->Index);

    /* Since the Token Starts by 1 but the indexing of the array will be zero based.*/
    if(g_PDKHandle[PDK_FRAMEANDSENDMAIL] != NULL)
    {
         ((int(*)(INT8U,INT8U,int))g_PDKHandle[PDK_FRAMEANDSENDMAIL]) (pSetCfgReq->Index -1,EthIndex,BMCInst);
    }
    SetPendStatus (PEND_OP_SEND_EMAIL_ALERT,PEND_STATUS_COMPLETED);

    return 0;
}

/**
*@fn PendSetVLANIfcID
*@brief This function is invoked to set the valn id for the specified interface
*@param pData -  Pointer to buffer where network configurations saved
*@param DataLen -  unsigned integer parameter to buffer where length of the input data specified
*@param EthIndex -  char value to bufferr where index for Ethernet channel is saved
*@return Returns 0 on success
*/
int PendSetVLANIfcID(INT8U* pData,INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;    
    char CmdsetVconfig[32], IfcName[16];
    INT16U vlanID[MAX_LAN_CHANNELS]={0};
    INT16U PriorityLevel[MAX_LAN_CHANNELS]= {0};
    int retValue = 0,Index=0;
    char Ifcname[16];
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    SERVICE_CONF_STRUCT g_serviceconf;

    memset(Ifcname,0,sizeof(Ifcname));

    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);
    NWConfig.VLANID =  ((NWCFG_STRUCT*)pData)->VLANID;

    TCRIT("\n %s:VLAN ID request received... VLANID = %d \n",__FUNCTION__,NWConfig.VLANID);

    /* Read VLAN IDs from the /conf/vlansetting.conf */
    if(ReadVLANFile(VLAN_ID_SETTING_STR, vlanID) == -1)
    {
        SetPendStatus (PEND_OP_SET_VLAN_ID,PEND_STATUS_COMPLETED);
        return -1;
    }

    /* Writing VLAN ID to VLAN setting file */
    if(WriteVLANFile(VLAN_ID_SETTING_STR, vlanID, EthIndex, NWConfig.VLANID) == -1)
    {
        SetPendStatus (PEND_OP_SET_VLAN_ID,PEND_STATUS_COMPLETED);
        return -1;
    }

    if(GetIfcName(pBMCInfo->LANConfig.g_ethindex,Ifcname,BMCInst) == -1)
    {
        SetPendStatus (PEND_OP_SET_VLAN_ID,PEND_STATUS_COMPLETED);
        return -1;
    }

    sprintf(CmdsetVconfig,"vconfig add %s %d",Ifcname,NWConfig.VLANID );

    if(((retValue = safe_system(CmdsetVconfig)) < 0))
    {
        TCRIT("ERROR %d: %s failed\n",retValue,IFUP_BIN_PATH);
    }

    sleep(2);

    for(Index=0;Index<sizeof(pBMCInfo->LanIfcConfig)/sizeof(LANIFCConfig_T);Index++)
    {
        if(strlen(pBMCInfo->LanIfcConfig[Index].ifname) != 0)
        {
            ReadIfcVLANID (VLAN_ID_SETTING_STR, pBMCInfo->LanIfcConfig[Index].ifname, &pBMCInfo->LANConfig.VLANID[Index]);
        }
    }

    for(Index = 0; Index < MAX_SERVICE; Index++)
    {
        if(get_service_configurations(ModifyServiceNameList[Index],&g_serviceconf) != -1)
	{
        if(strcmp(g_serviceconf.InterfaceName,Ifcname) == 0)
        {
            sprintf(g_serviceconf.InterfaceName,"%s.%d",Ifcname,NWConfig.VLANID);
            if(set_service_configurations(ModifyServiceNameList[Index],&g_serviceconf,g_corefeatures.timeoutd_sess_timeout) !=0)
            {
                TCRIT("Error in Setting service configuration for the service %s\n",ModifyServiceNameList[Index]);
            }
        }
	}
    }

    nwWriteNWCfg_ipv4_v6 ( &NWConfig, &NWConfig6, EthIndex);

    memset(IfcName, 0, sizeof(IfcName));
    if(GetIfcName(EthIndex, IfcName,BMCInst) != 0)
    {
        TCRIT("Error in getting Interface Name for the Lan Index :%d\n",EthIndex);
    }
    if(ReadVLANFile(VLAN_PRIORITY_SETTING_STR, PriorityLevel) == -1)
    {
        SetPendStatus (PEND_OP_SET_VLAN_ID,PEND_STATUS_COMPLETED);
        return -1;
    }
    if(WriteVLANFile(VLAN_PRIORITY_SETTING_STR, PriorityLevel, EthIndex, pBMCInfo->LANCfs[EthIndex].VLANPriority) == -1)
    {
        SetPendStatus (PEND_OP_SET_VLAN_ID,PEND_STATUS_COMPLETED);
        return -1;
    }
    SetVLANPriority(NWConfig.VLANID, pBMCInfo->LANCfs[EthIndex].VLANPriority, pBMCInfo->LANCfs[EthIndex].Ipv4HdrParam.TypeOfService, IfcName);

    /*Restart the service to effect the changes*/
    for(Index = 0; Index < MAX_RESTART_SERVICE; Index++)
    {
        safe_system(RestartServices[Index]);
    }

    SetPendStatus (PEND_OP_SET_VLAN_ID,PEND_STATUS_COMPLETED);
    return 0;
}


/**
*@fn PendDeConfigVLANInterface
*@brief This function is invoked to de-configure vlan sockets
*@param pData -  Pointer to buffer where network configurations saved
*@param DataLen -  unsigned integer parameter to buffer where length of the input data specified
*@param EthIndex -  char value to buffer where index for Ethernet channel is saved
*@return Returns 0 on success
*/
int PendDeConfigVLANInterface(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    int retValue = 0,Index=0;
    INT16U vlanID[MAX_LAN_CHANNELS]={0};
    INT16U VLANPriorityLevel[MAX_LAN_CHANNELS];
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6 ;
    char str[40]=VLAN_NETWORK_DECONFIG_FILE;
    char Ifcname[16] = {0};
    char tmpIfcname[16] = {0};
    SERVICE_CONF_STRUCT g_serviceconf;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    //nwReadNWCfg(&NWConfig, EthIndex);
    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);

    if(GetIfcName(pBMCInfo->LANConfig.g_ethindex,Ifcname,BMCInst) == -1)
    {
        SetPendStatus (PEND_OP_DECONFIG_VLAN_IFC,PEND_STATUS_COMPLETED);
        return -1;
    }

    sprintf(str,"%s%d",str,EthIndex);
    if(((retValue = safe_system(str)) < 0))
    {
        TCRIT("ERROR %d: %s failed\n",retValue,IFUP_BIN_PATH);
    }
    /* Reset vlan setting files */
    if(ReadVLANFile(VLAN_ID_SETTING_STR, vlanID) == -1)
    {
        SetPendStatus (PEND_OP_DECONFIG_VLAN_IFC,PEND_STATUS_COMPLETED);
        return -1;
    }

    if(WriteVLANFile(VLAN_ID_SETTING_STR, vlanID, EthIndex, 0) == -1)
    {
        SetPendStatus (PEND_OP_DECONFIG_VLAN_IFC,PEND_STATUS_COMPLETED);
        return -1;
    }

    if(ReadVLANFile(VLAN_PRIORITY_SETTING_STR, VLANPriorityLevel) == -1)
    {
        SetPendStatus (PEND_OP_DECONFIG_VLAN_IFC,PEND_STATUS_COMPLETED);
        return -1;
    }

    if(WriteVLANFile(VLAN_PRIORITY_SETTING_STR, VLANPriorityLevel, EthIndex, 0) == -1)
    {
        SetPendStatus (PEND_OP_DECONFIG_VLAN_IFC,PEND_STATUS_COMPLETED);
        return -1;
    }

    sync();

    sleep(2);

    for(Index=0;Index<sizeof(pBMCInfo->LanIfcConfig)/sizeof(LANIFCConfig_T);Index++)
    {
        if(strlen(pBMCInfo->LanIfcConfig[Index].ifname) != 0)
        {
            ReadIfcVLANID (VLAN_ID_SETTING_STR, pBMCInfo->LanIfcConfig[Index].ifname, &pBMCInfo->LANConfig.VLANID[Index]);
        }
    }

    for(Index = 0; Index < MAX_SERVICE; Index++)
    {
        if(get_service_configurations(ModifyServiceNameList[Index],&g_serviceconf) != -1)
	{
        sprintf(tmpIfcname,"%s.%d",Ifcname,NWConfig.VLANID);
        if(strcmp(g_serviceconf.InterfaceName,tmpIfcname) == 0)
        {
            sprintf(g_serviceconf.InterfaceName,"%s",Ifcname);
            if(set_service_configurations(ModifyServiceNameList[Index],&g_serviceconf,g_corefeatures.timeoutd_sess_timeout) !=0)
            {
                TCRIT("Error in Setting service configuration for the service %s\n",ModifyServiceNameList[Index]);
            }
        }
	}
    }

    /*Reset the VLANID*/
    NWConfig.VLANID=0;

    nwWriteNWCfg_ipv4_v6 ( &NWConfig, &NWConfig6, EthIndex);

    /*Restart the service to effect the changes*/
    for(Index = 0; Index < MAX_RESTART_SERVICE; Index++)
    {
        safe_system(RestartServices[Index]);
    }

    SetPendStatus (PEND_OP_DECONFIG_VLAN_IFC,PEND_STATUS_COMPLETED);
    return 0;
}
/**
*@fn PendSetVLANPriority
*@brief This function is invoked to configure vlan Priority
*@param pData -  Pointer to buffer where network configurations saved
*@param DataLen -  unsigned integer parameter to buffer where length of the input data specified
*@param EthIndex -  char value to buffer where index for Ethernet channel is saved
*@return Returns 0 on success
*/
int PendSetVLANPriority(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    INT8U VLANPriority, TOS;
    INT16U vlanID=0;
    char IfcName[16];
    memset(IfcName,0,sizeof(IfcName));
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    NWCFG_STRUCT* NWConfig = (NWCFG_STRUCT *)pData;
    if(GetIfcName(EthIndex, IfcName,BMCInst) != 0)
    {
        TCRIT("Error in getting Interface Name for the Lan Index :%d\n",EthIndex);
        SetPendStatus (PEND_OP_SET_VLAN_PRIORITY, PEND_STATUS_COMPLETED);
        return -1;
    }
    VLANPriority = NWConfig->vlanpriority;
    vlanID = (pBMCInfo->LANCfs[EthIndex].VLANID & 0xfff);
    TOS = pBMCInfo->LANCfs[EthIndex].Ipv4HdrParam.TypeOfService;
    SetVLANPriority(vlanID, VLANPriority, TOS, IfcName);
    SetPendStatus (PEND_OP_SET_VLAN_PRIORITY, PEND_STATUS_COMPLETED);
    return 0;

}

/**
*@fn PendSetIPv4Headers
*@brief This function is invoked to set IPv4 headers.
*@param pData -  Pointer to buffer where network configurations saved
*@param DataLen -  unsigned integer parameter to buffer where length of the input data specified
*@param EthIndex -  char value to bufferr where index for Ethernet channel is saved
*@return Returns 0 on success
*/
int PendSetIPv4Headers(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    MsgPkt_T MsgPkt;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    /* Send a Mesg Request to LANIFC task to reinitialize the LAN sockets after new VLAN IFC*/
    MsgPkt.Param =LAN_CONFIG_IPV4_HEADER;
    MsgPkt.Channel = pBMCInfo->LANConfig.g_ethindex;

    if(pBMCInfo->LANConfig.UDPSocket[EthIndex] != -1)
    {
        MsgPkt.Socket = pBMCInfo->LANConfig.UDPSocket[EthIndex];
    }

    if((pBMCInfo->IpmiConfig.VLANIfcSupport == 1) && (pBMCInfo->LANConfig.VLANID[EthIndex] != 0))
    {
        if(pBMCInfo->LANConfig.VLANUDPSocket[EthIndex] != -1)
        {
            MsgPkt.Socket = pBMCInfo->LANConfig.VLANUDPSocket[EthIndex];
        }
    }

    /* Post the request packet to LAN Interface Queue */
    if (0 != PostMsg (&MsgPkt, LAN_IFC_Q, BMCInst))
    {
        IPMI_WARNING ("LANIfc.c : Error posting message to LANIfc Q\n");
    }

    SetPendStatus (PEND_OP_SET_IPV4_HEADERS,PEND_STATUS_COMPLETED);
    return 0;
}

/*
 *@fn PendSetRMCPPort
 *@brief This function is invoked to change RMCP port number
 *@param pData -  Pointer to buffer which hold the data to be posted
 *@param DataLen -  Specifies the length of the message to be posted
 *@param EthIndex -  Ethernet Index
 */
int PendSetRMCPPort(INT8U* pData,INT32U Datalen,INT8U EthIndex,int BMCInst)
{
    MsgPkt_T MsgPkt;

    MsgPkt.Param = LAN_RMCP_PORT_CHANGE;

    /* Post the request packet to LAN Interface Queue */
    if(0 != PostMsg(&MsgPkt, LAN_IFC_Q,BMCInst))
    {
        IPMI_WARNING("LANIfc.c : Error posting message to LANIfc Q\n");
    }
    SetPendStatus (PEND_RMCP_PORT_CHANGE,PEND_STATUS_COMPLETED);
    return 0;
}

/*
 *@fn PendSetAllDNSCfg
 *@brief This function is invoked to set all dns configuration
 *@param pData -  Pointer to buffer which hold the data to be posted
 *@param DataLen -  Specifies the length of the message to be posted
 *@param EthIndex -  Ethernet Index
 */
int PendSetAllDNSCfg(INT8U* pData, INT32U DataLen, INT8U EthIndex,int BMCInst)
{

    HOSTNAMECONF HostnameConfig;
    DOMAINCONF DomainConfig;
    DNSCONF DnsIPConfig;
    INT8U regBMC_FQDN[MAX_LAN_CHANNELS];

    memset(&HostnameConfig, 0, sizeof(HostnameConfig));
    memset(&DomainConfig, 0, sizeof(DomainConfig));
    memset(&DnsIPConfig, 0, sizeof(DnsIPConfig));
    memset(regBMC_FQDN, 0, sizeof(regBMC_FQDN));

    nwGetAllDNSConf( &HostnameConfig, &DomainConfig, &DnsIPConfig,regBMC_FQDN );

    LOCK_BMC_SHARED_MEM(BMCInst);

    /*
     * Start of converting data
     */
    //DNS SERVER IP
    DnsIPConfig.DNSEnable = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSEnable;
    DnsIPConfig.DNSIndex = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIndex;
    DnsIPConfig.DNSDHCP = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSDHCP;
    DnsIPConfig.IPPriority = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.IPPriority;

    if(DnsIPConfig.DNSDHCP== 1)
    {
        memset(DnsIPConfig.DNSIP1,'\0',IP6_ADDR_LEN);
        memset(DnsIPConfig.DNSIP2,'\0',IP6_ADDR_LEN);
        memset(DnsIPConfig.DNSIP3,'\0',IP6_ADDR_LEN);
    }
    else
    {
        memcpy(DnsIPConfig.DNSIP1,BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr1,IP6_ADDR_LEN);
        memcpy(DnsIPConfig.DNSIP2,BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr2,IP6_ADDR_LEN);
        memcpy(DnsIPConfig.DNSIP3,BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr3,IP6_ADDR_LEN);
    }

    //DOMAIN NAME
    DomainConfig.EthIndex = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainIndex;
    DomainConfig.v4v6 = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.Domainpriority; //To support both IPv4 and IPv6 in DNS_CONFIG.
    DomainConfig.dhcpEnable = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainDHCP;
    if(DomainConfig.dhcpEnable == 1)
    {
        DomainConfig.domainnamelen = 0;
        memset(DomainConfig.domainname,'\0',DNSCFG_MAX_DOMAIN_NAME_LEN);
    }
    else
    {
        DomainConfig.domainnamelen = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainLen;
        if(DomainConfig.domainnamelen <= MAX_DOMAIN_NAME_STRING_SIZE)
        {
            memset(&DomainConfig.domainname , '\0', sizeof(DomainConfig.domainname));
            strncpy((char *)&DomainConfig.domainname, (char *)&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName, DomainConfig.domainnamelen);
        }
    }

    //Register BMC Flag
    memcpy(regBMC_FQDN, BMC_GET_SHARED_MEM (BMCInst)->DNSconf.RegisterBMC, MAX_LAN_CHANNELS);

    if(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostNameLen <= MAX_HOST_NAME_STRING_SIZE)
    {
        HostnameConfig.HostNameLen = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostNameLen;
    }
    else
    {
        HostnameConfig.HostNameLen = MAX_HOST_NAME_STRING_SIZE;
    }
    HostnameConfig.HostSetting = BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostSetting;

    memset(&HostnameConfig.HostName , '\0', sizeof(HostnameConfig.HostName));
    strncpy((char *)&HostnameConfig.HostName, (char *)&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostName, HostnameConfig.HostNameLen);

    UNLOCK_BMC_SHARED_MEM(BMCInst);
    /* End of converting data */

    nwSetAllDNSConf( &HostnameConfig, &DomainConfig, &DnsIPConfig, regBMC_FQDN);

    SetPendStatus (PEND_OP_SET_ALL_DNS_CFG, PEND_STATUS_COMPLETED);
    return 0;
}

static int PendSetIPv6Cfg(INT8U* pData, INT32U DataLen, INT8U EthIndex,int BMCInst)
{
    INT8U ethIndex = 0, curchannel;

    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    ethIndex = GetEthIndex( curchannel & 0xF,BMCInst);
    if (0xff == ethIndex) ethIndex = 0;

    NWCFG_STRUCT cfgIPv4;
    NWCFG6_STRUCT* newIPv6Cfg = (NWCFG6_STRUCT*)pData;

    nwReadNWCfg_v4_v6(&cfgIPv4, NULL, ethIndex,g_corefeatures.global_ipv6);
    nwWriteNWCfg_ipv4_v6(&cfgIPv4, newIPv6Cfg, ethIndex);

    SetPendStatus (PEND_OP_SET_IPV6_CFG, PEND_STATUS_COMPLETED);

    return 0;
}

/*
 *@fn PendSetIfaceState
 *@brief This function is invoked to set network interface state to enable or disable
 *@param pData -  Pointer to buffer which hold the data to be posted
 *@param DataLen -  Specifies the length of the message to be posted
 *@param EthIndex -  Ethernet Index
 */
int PendSetEthIfaceState(INT8U* pData, INT32U DataLen, INT8U EthIndex,int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    EthIfaceState* pReq = (EthIfaceState*)pData;
    NWCFG_STRUCT cfg;
    NWCFG6_STRUCT cfg6;
    int retValue = 0;
    int eth_index = 0;

    eth_index = pReq->EthIndex;

    retValue = nwReadNWCfg_v4_v6(&cfg, &cfg6, eth_index,g_corefeatures.global_ipv6);
    if(retValue != 0)
        TCRIT("Error in reading network configuration.\n");

    switch(pReq->EnableState)
    {
        case DISABLE_V4_V6:
            cfg.enable = 0;
            cfg6.enable = 0;
            break;

        case ENABLE_V4:
            cfg.enable = 1;
            cfg6.enable = 0;
            break;

        case ENABLE_V4_V6:
            cfg.enable = 1;
            cfg6.enable = 1;
            break;

        case ENABLE_V6:
            cfg.enable  = 0;
            cfg6.enable = 1;
            break;

        default:
            SetPendStatus (PEND_OP_SET_ETH_IFACE_STATE, PEND_STATUS_COMPLETED);
            return -1;
            break;
    }

    cfg.CfgMethod = pBMCInfo->LANCfs[eth_index].IPAddrSrc;
    cfg6.CfgMethod = pBMCInfo->LANCfs[eth_index].IPv6_IPAddrSrc;

    if(cfg.CfgMethod == STATIC_IP_SOURCE)
    {
        //Set IP address
        _fmemcpy (&cfg.IPAddr, pBMCInfo->LANCfs[eth_index].IPAddr, IP_ADDR_LEN);
        //Set subnet mask
        _fmemcpy (&cfg.Mask, pBMCInfo->LANCfs[eth_index].SubNetMask, IP_ADDR_LEN);
        //Set default gateway
        _fmemcpy (&cfg.Gateway, pBMCInfo->LANCfs[eth_index].DefaultGatewayIPAddr, IP_ADDR_LEN);
    }

    if(cfg6.CfgMethod == STATIC_IP_SOURCE)
    {
        _fmemcpy(cfg6.GlobalIPAddr,pBMCInfo->LANCfs[eth_index].IPv6_IPAddr[0],IP6_ADDR_LEN*MAX_IPV6ADDRS);
        _fmemcpy(&cfg6.Gateway,pBMCInfo->LANCfs[eth_index].IPv6_GatewayIPAddr,IP6_ADDR_LEN);
        _fmemcpy(cfg6.GlobalPrefix, pBMCInfo->LANCfs[eth_index].IPv6_PrefixLen,MAX_IPV6ADDRS);
    }

    if(cfg6.enable == 0)
        retValue = nwWriteNWCfg_ipv4_v6(&cfg, NULL, eth_index);
    else
        retValue = nwWriteNWCfg_ipv4_v6(&cfg, &cfg6, eth_index);

    if(retValue != 0)
        TCRIT("Error in writing network configuration.\n");

    SetPendStatus (PEND_OP_SET_ETH_IFACE_STATE, PEND_STATUS_COMPLETED);

    return 0;
}

static int PendSetMACAddress(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
	EnableSetMACAddress_T* data = (EnableSetMACAddress_T*)pData;
	char oldMACAddrString[18], newMACAddrString[18];
	char ethName[15];
	int result = 0;
	int SockFD = 0;
	struct ifreq IfReq;
	char *ParseMac = NULL;
	int Index = 0;
	unsigned long ConvMac = 0;
	int InterfaceEnabled = 0;
	char Cmd[128] = "/etc/init.d/networking restart";

	memset(oldMACAddrString, 0, sizeof(oldMACAddrString));
	memset(newMACAddrString, 0, sizeof(newMACAddrString));
	memset(ethName, 0, sizeof(ethName));

	// Frame the MAC Address and the Interface name
	sprintf(newMACAddrString, "%02x:%02x:%02x:%02x:%02x:%02x", 
			data->MACAddress[0], data->MACAddress[1], 
			data->MACAddress[2], data->MACAddress[3], 
			data->MACAddress[4], data->MACAddress[5]);

	if (data->EthIndex > 0 && data->EthIndex != 0xFF)
		sprintf(ethName, "eth%daddr", data->EthIndex);
	else
		strcpy(ethName, "ethaddr");

	/* ------- This section sets the MAC Address in U-Boot Environment --------- */

	// Get the old MAC Address from the U-Boot Environment
	GetUBootParam(ethName, oldMACAddrString);

	// Compare with our new MAC to confirm if it is indeed a change of MAC
	if (memcmp(oldMACAddrString, newMACAddrString, strlen(newMACAddrString)) == 0)
	{
		result = 0;
		goto clear_exit;
	}

	// Update the U-Boot environment with the updated MAC Address
	result = SetUBootParam(ethName, newMACAddrString);
	if (result != 0)
	{
		result = -1;
		goto clear_exit;
	}	

	/* --------------------- End of U-Boot section --------------------------*/

/*-----------------------------------------------------------------------
 *
 * for g_coremacros.maceeprom_addr_len == 2 (16-bit EEPROM address) offset is
 *   0x000nxxxx for EEPROM address selectors at n, offset xxxx in EEPROM.
 *
 * for g_coremacros.maceeprom_addr_len == 1 (8-bit EEPROM page address) offset is
 *   0x00000nxx for EEPROM address selectors and page number at n.
 -----------------------------------------------------------------------*/
	/* Set MAC into EEPROM */
    if(g_corefeatures.maceeprom_support == ENABLED)
    {
        INT8U WriteBuf[8]={0},WriteCount=0,offsetTmp;
        char EEPROMBusName[64];
        u8 eeprom_addr;
        int retry;

        if(g_coremacros.maceeprom_addr_len==1)
        {
            if (data->EthIndex > 0 && data->EthIndex != 0xFF)
			{
				offsetTmp = (g_coremacros.eeprom_mac1addr_offset>>8) & 0xFF;
				WriteBuf[0] = g_coremacros.eeprom_mac1addr_offset & 0xFF;
			}
			else
			{
				offsetTmp = (g_coremacros.eeprom_macaddr_offset>>8) & 0xFF;
				WriteBuf[0] = g_coremacros.eeprom_macaddr_offset & 0xFF;
			}
            WriteCount=7;
			eeprom_addr = g_coremacros.maceeprom_addr | offsetTmp;
			memcpy(&WriteBuf[1],&data->MACAddress[0], MAC_ADDR_LEN);
        }
        else
        {
			if (data->EthIndex > 0 && data->EthIndex != 0xFF)
			{
				WriteBuf[0] = (g_coremacros.eeprom_mac1addr_offset>>8) & 0xFF;
				WriteBuf[1] = g_coremacros.eeprom_mac1addr_offset & 0xFF;
			}
			else
			{
				WriteBuf[0] = (g_coremacros.eeprom_macaddr_offset>>8) & 0xFF;
				WriteBuf[1] = g_coremacros.eeprom_macaddr_offset & 0xFF;
			}
			WriteCount=8;
			eeprom_addr = g_coremacros.maceeprom_addr;
			memcpy(&WriteBuf[2],&data->MACAddress[0], MAC_ADDR_LEN);
        }

		sprintf(EEPROMBusName, "/dev/i2c%d",g_coremacros.maceeprom_channel_id);
        //printf("PendSetMACAddress : set %d to EEPROM data 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",WriteCount,WriteBuf[0],WriteBuf[1],WriteBuf[2],WriteBuf[3],WriteBuf[4],WriteBuf[5],WriteBuf[6],WriteBuf[7]);
		for(retry=1;retry<4;retry++)
	    {
		    if(0 >((ssize_t(*)(char *,u8,u8 *,size_t))g_HALI2CHandle[HAL_I2C_MW]) (EEPROMBusName,eeprom_addr,WriteBuf,WriteCount))
	        {
			    /* Erron in reading */
			    printf("Set MAC to EEPROM error %d time\n",retry);
		    }
			else
			{
			    break;
			}
	    }
	}
	/* End of Set MAC into EEPROM */

        /*If the bond interface is enabled, reboot is needed*/
        if(CheckBondSlave(data->EthIndex) == 1)
        {
	     TDBG("Given index is slave of bond interface\n");
	     SetPendStatus(PEND_OP_SET_MAC_ADDRESS, PEND_STATUS_COMPLETED);
	     /* PDK Module Post Set Reboot Cause*/
	     if(g_PDKHandle[PDK_SETREBOOTCAUSE] != NULL)
	     {
 		((INT8U(*)(INT8U,int)) g_PDKHandle[PDK_SETREBOOTCAUSE])(SETREBOOTCAUSE_IPMI_CMD_PROCESSING,BMCInst);
 	     }
	     reboot(LINUX_REBOOT_CMD_RESTART);
	     return 0;
        }

	/* ------- This section sets the MAC Address Temporarily in Linux environment --------- */
	/* ------- Upon next reboot, the new mac will take effect permanently 	      --------- */

	// Frame the Interface name based on the EthIndex
	if (data->EthIndex != 0xFF)
	{
		sprintf(ethName, "eth%d", data->EthIndex);
	}	

	// Set the Interface name and its family
	sprintf(IfReq.ifr_name, "%s", ethName);
    	IfReq.ifr_hwaddr.sa_family = AF_INET;

	// open a socket to the interface
	SockFD = socket(AF_INET, SOCK_STREAM, 0);
	if(SockFD < 0)
	{
		goto clear_exit;
	}

	// Read the Interface status and identify it is already UP
	result = ioctl(SockFD, SIOCGIFFLAGS, (char *)&IfReq);
	if (result < 0)
	{
		goto clear_exit;
	}		

	if (IfReq.ifr_flags & IFF_UP)
	{
		InterfaceEnabled = 1;
	}

	// If Interface is already UP, then disable the interface temporarily
	if (InterfaceEnabled)	
	{
		// disable the interface by setting the flag
		IfReq.ifr_flags &= ~(IFF_UP);

  		// apply the flag by using the ioctl
  		result = ioctl(SockFD, SIOCSIFFLAGS, (char *)&IfReq);
		if (result < 0) 
		{
			goto clear_exit;
		}
	}

	// Getting the Current MAC Address from the interface
	// This also fills the structure with proper data for all fields
	result = ioctl(SockFD, SIOCGIFHWADDR, (char *)&IfReq);
	if(result < 0)
	{
		goto clear_exit;
	}

	// Just replace the Data field of the structure with our new MAC Address
	// Have to convert the string to Data happy format
	ParseMac = strtok(newMACAddrString, ":");
  	do
  	{
        	if (ParseMac)
        	{
               		sscanf(ParseMac, "%lx", &ConvMac);
                	IfReq.ifr_hwaddr.sa_data[Index] = (char)(ConvMac);
			ParseMac = NULL;
        	}
		else
		{
			break;
		}

        	Index++;
        	ParseMac = strtok(NULL, ":");
	} while (ParseMac != NULL);

	if ((ParseMac == NULL) && (Index < IFHWADDRLEN))
	{
		result = -1;
		goto clear_exit;
	}

	// Set the MAC Address for the interface
  	result = ioctl(SockFD, SIOCSIFHWADDR, (char *)&IfReq);
  	if (result < 0) 
	{
		goto clear_exit;
  	}

	// If interface was enabled before, then bring back up the interface again
	// Else, we need not bring back up the interface, as it was already down
	if (InterfaceEnabled)
	{
		// enable the interface by setting the flag
		IfReq.ifr_flags |= (IFF_UP);

		// set the new flags for interface
  		result = ioctl(SockFD, SIOCSIFFLAGS, (char *)&IfReq);
  		if (result < 0) 
		{
  	     	 	goto clear_exit;
  		}
	
		safe_system(Cmd);
	    	OS_TIME_DELAY_HMSM (0, 0, 2, 0);
	}
	
	/* --------------------- End of Linux section --------------------------*/
		
clear_exit:
	// Close the socket if existing
	if (SockFD > 0)
		close(SockFD);

	// Set the status to COMPLETED before returning
	SetPendStatus(PEND_OP_SET_MAC_ADDRESS, PEND_STATUS_COMPLETED);
	
	return result;
}

static int PendSetIPv6Enable(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
     NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);
    NWConfig6.enable = ((NWCFG6_STRUCT*)pData)->enable;
    /*Update the IP Source*/
    NWConfig6.CfgMethod = pBMCInfo->LANCfs[pBMCInfo->LANConfig.g_ethindex].IPv6_IPAddrSrc;
    if(NWConfig6.CfgMethod == STATIC_IP_SOURCE)
    {
        _fmemcpy(NWConfig6.GlobalIPAddr, pBMCInfo->LANCfs[pBMCInfo->LANConfig.g_ethindex].IPv6_IPAddr,IP6_ADDR_LEN*MAX_IPV6ADDRS);
        _fmemcpy(&NWConfig6.Gateway,pBMCInfo->LANCfs[pBMCInfo->LANConfig.g_ethindex].IPv6_GatewayIPAddr,IP6_ADDR_LEN);
        _fmemcpy(NWConfig6.GlobalPrefix, pBMCInfo->LANCfs[pBMCInfo->LANConfig.g_ethindex].IPv6_PrefixLen,MAX_IPV6ADDRS);
    }
    nwWriteNWCfg_ipv4_v6( &NWConfig, &NWConfig6,EthIndex);
    SetPendStatus (PEND_OP_SET_IPV6_ENABLE,PEND_STATUS_COMPLETED);

    return 0;

}

static int PendSetIPv6Source(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;
    DOMAINCONF      DomainCfg;
    DNSCONF     DNS;
    INT8U               regBMC_FQDN[MAX_LAN_CHANNELS];

    memset(&DomainCfg,0,sizeof(DOMAINCONF));
    memset(&DNS,0,sizeof(DNSCONF));
    memset(regBMC_FQDN,0,sizeof(regBMC_FQDN));

    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);
    NWConfig6.CfgMethod= ((NWCFG6_STRUCT*)pData)->CfgMethod;

    if(NWConfig6.CfgMethod == CFGMETHOD_STATIC)
    {
        ReadDNSConfFile(&DomainCfg, &DNS, regBMC_FQDN);

        if(DomainCfg.v4v6 == 2)
            DomainCfg.v4v6 = 0;

        if(DNS.DNSDHCP== 1)
            DNS.DNSDHCP= 0;

        WriteDNSConfFile(&DomainCfg, &DNS, regBMC_FQDN);
    }

    nwWriteNWCfg_ipv4_v6( &NWConfig, &NWConfig6,EthIndex);
    SetPendStatus (PEND_OP_SET_IPV6_IP_ADDR_SOURCE,PEND_STATUS_COMPLETED);

    return 0;

}

static int PendSetIPv6Address(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;
    IPv6Addr_T                  *NewIPv6Addr = (IPv6Addr_T *)pData;

    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);

    memcpy( NWConfig6.GlobalIPAddr[(NewIPv6Addr->IPv6_Cntr & 0x0F)], NewIPv6Addr->IPv6_IPAddr, IP6_ADDR_LEN );
    nwWriteNWCfg_ipv4_v6( &NWConfig, &NWConfig6,EthIndex);
    SetPendStatus (PEND_OP_SET_IPV6_IP_ADDR,PEND_STATUS_COMPLETED);

    return 0;

}

static int PendSetIPv6PrefixLength(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;
    IPv6Prefix_T       *NewIPv6Prefix = (IPv6Prefix_T *)pData;
    
    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);
    NWConfig6.GlobalPrefix[(NewIPv6Prefix->IPv6_Prepos & 0x0F)] = NewIPv6Prefix->IPv6_PrefixLen;
    nwWriteNWCfg_ipv4_v6( &NWConfig, &NWConfig6,EthIndex);
    SetPendStatus (PEND_OP_SET_IPV6_PREFIX_LENGTH,PEND_STATUS_COMPLETED);

    return 0;

}

static int PendSetIPv6Gateway(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;

    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);

    memcpy( NWConfig6.Gateway, ((NWCFG6_STRUCT*)pData)->Gateway, IP6_ADDR_LEN );

    nwWriteNWCfg_ipv4_v6( &NWConfig, &NWConfig6,EthIndex);
    SetPendStatus (PEND_OP_SET_IPV6_GATEWAY,PEND_STATUS_COMPLETED);

    return 0;

}

static int PendSetDHCPv6TimingConf(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    INT8U Parameter[MAX_SUPPORTED_DHCPV6_TIMING_PARAMS][MAX_DHCPV6_TIMING_PARAM_SIZE]=
                    {
                        "SOL_MAX_DELAY",
                        "SOL_TIMEOUT",
                        "SOL_MAX_RT",
                        "REQ_TIMEOUT",
                        "REQ_MAX_RT",
                        "REQ_MAX_RC",
                        "CNF_MAX_DELAY",
                        "CNF_TIMEOUT",
                        "CNF_MAX_RT",
                        "CNF_MAX_RD",
                        "REN_TIMEOUT",
                        "REN_MAX_RT",
                        "REB_TIMEOUT",
                        "REB_MAX_RT",
                        "INF_MAX_DELAY",
                        "INF_TIMEOUT",
                        "INF_MAX_RT",
                        "REL_TIMEOUT",
                        "REL_MAX_RC",
                        "DEC_TIMEOUT",
                        "DEC_MAX_RC",
                        "HOP_COUNT_LIMIT"
                    };

    float DHCPv6_Timing_Param[2*MAX_SUPPORTED_DHCPV6_TIMING_PARAMS]=
    {
    /*----Granularity----InitialValue*/
            0.5,          0.5,
            0.5,          0.5,
            30,           30,
            0.5,          0.5,
            0.5,          15,
            1,            0,
            0.5,          0.5,
            0.5,          0.5,
            1,            1,
            2,            2,
            2,            2,
            10,           10,
            2,            2,
            10,           10,
            0.5,          0.5,
            0.5,          0.5,
            30,           30,
            0.5,          0.5,
            1,            1,
            0.5,          0.5,
            1,            1,
            1,            1
    };



    FILE *fp_ipv6timingconf=NULL;
    INT8U InterfaceName[MAX_IPV6_INTERFACE][MAX_IFC_NAME];
    int noofinterface=0;
    int i=0,SetSelector=0;
    int offset=0;
    int value;

    /*  Create a file in conf to store all the requested DHCPv6 Timing configurations
        These informations will be used by dhcp6c client 
    */
    fp_ipv6timingconf=fopen(DHCPV6_TIMING_CONF,"w");

    if(fp_ipv6timingconf == NULL)
    {
        TCRIT("ERROR: Cannot open DHCP6C Timing configuration file!\n");
        return -1;
    }

    if(pBMCInfo->LANCfs->IPv6_DHCPv6TimingConfSupport == 0x01)
        SetSelector = 0;
    else if(pBMCInfo->LANCfs->IPv6_DHCPv6TimingConfSupport == 0x02)
        SetSelector = 1;

    memset(InterfaceName,0,sizeof(InterfaceName));

    noofinterface=GetActiveIfcnameAndNoofIfc(InterfaceName);

    for(i=0; i < noofinterface; i++)
    {
        fprintf(fp_ipv6timingconf,"[ %s ]\n",InterfaceName[i]);

        for(offset = 0; offset < MAX_SUPPORTED_DHCPV6_TIMING_PARAMS; offset++)
        {
            SET_DHCPV6_TIMING_CONF(pBMCInfo->LANCfs->DHCPv6TimingConf[i*SetSelector][offset],
                                   DHCPv6_Timing_Param[offset*2] , DHCPv6_Timing_Param[offset*2+1],offset,value);
            fprintf(fp_ipv6timingconf,"\t%-24s%d\n",Parameter[offset],value);
        }

        fprintf(fp_ipv6timingconf,"\n");
    }

    fclose(fp_ipv6timingconf);

    SetPendStatus (PEND_OP_SET_DHCPV6_TIMING_CONF,PEND_STATUS_COMPLETED);

    return 0;
}

static int PendSetSLAACTimingConf(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{

    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    INT8U Parameter[MAX_SUPPORTED_SLAAC_TIMING_PARAMS][MAX_SLAAC_TIMING_PARAM_SIZE]=
                    {
                        "MAX_RTR_SOLICITATION_DELAY",
                        "RTR_SOLICITATION_INTERVAL",
                        "MAX_RTR_SOLICITATIONS",
                        "DupAddrDetectTransmits",
                        "MAX_MULTICAST_SOLICIT",
                        "MAX_UNICAST_SOLICIT",
                        "MAX_ANYCAST_DELAY_TIME",
                        "MAX_NEIGHBOR_ADVERTISEMENT",
                        "REACHABLE_TIME",
                        "RETRANS_TIMER",
                        "DELAY_FIRST_PROBE_TIME",
                        "MAX_RANDOM_FACTOR",
                        "MIN_RANDOM_FACTOR"
                    };

    float SLAAC_Timing_Param[2*MAX_SUPPORTED_SLAAC_TIMING_PARAMS]=
    {
    /*----Granularity----InitialValue*/
            0.25,           0.25,
            0.5,            0.5,
            1,              1,
            1,              0,
            1,              1,
            1,              1,
            0.25,           0.25,
            1,              1,
            2,              2,
            0.25,           0.25,
            0.5,            0.5,
            0.125,          1,
            0.125,          0.125
    };

    FILE *fp_ipv6timingconf=NULL;
    INT8U InterfaceName[MAX_IPV6_INTERFACE][MAX_IFC_NAME];
    int noofinterface=0;
    int i=0,SetSelector=0;
    int offset=0;
    int value;

    /*  Create a file in conf to store all the requested SLAAC Timing configurations
        These information is used to update the respective proc files
    */
    fp_ipv6timingconf=fopen(SLAAC_TIMING_CONF,"w");

    if(fp_ipv6timingconf == NULL)
    {
        TCRIT("ERROR: Cannot open SLAAC Timing configuration file!\n");
        return -1;
    }

    if(pBMCInfo->LANCfs->IPv6_SLAACTimingConfSupport == 0x01)
        SetSelector = 0;
    else
        SetSelector = 1;

    memset(InterfaceName,0,sizeof(InterfaceName));

    noofinterface=GetActiveIfcnameAndNoofIfc(InterfaceName);

    for(i=0; i < noofinterface; i++)
    {
        fprintf(fp_ipv6timingconf,"[ %s ]\n",InterfaceName[i]);

        for(offset=0; offset < MAX_SUPPORTED_SLAAC_TIMING_PARAMS; offset++)
        {
            SET_SLAAC_TIMING_CONF(pBMCInfo->LANCfs->SLAACTimingConf[i*SetSelector][offset],
                                  SLAAC_Timing_Param[2*offset],SLAAC_Timing_Param[2*offset+1],offset,value);
            fprintf(fp_ipv6timingconf,"\t%-32s%d\n",Parameter[offset],value);
        }

        fprintf(fp_ipv6timingconf,"\n");
    }

    fclose(fp_ipv6timingconf);

    SetPendStatus (PEND_OP_SET_SLAAC_TIMING_CONF,PEND_STATUS_COMPLETED);

    return 0;
}


/*
 *@fn PendConfigBonding
 *@brief This function is invoked to Enable/Disable the Bonding Support
 *@param pData -  Pointer to buffer which hold the data to be posted
 *@param DataLen -  Specifies the length of the message to be posted
 *@param EthIndex -  Ethernet Index
 */
static int PendConfigBonding(INT8U * pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
    BondIface*pConfigBond = (BondIface*)pData;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8S   Ifcname[IFNAMSIZ];
    INT8U i,Channel=0,Ethindex=0xff;
    INT16U vlanID[MAX_LAN_CHANNELS]={0};
    INT16U VLANPriorityLevel[MAX_LAN_CHANNELS];


    memset(Ifcname,0,sizeof(Ifcname));
    sprintf(Ifcname,"bond%d",pConfigBond->BondIndex);

    for(i=0;i<MAX_LAN_CHANNELS;i++)
    {
        if(strcmp(pBMCInfo->LanIfcConfig[i].ifname,Ifcname) == 0)
        {
            Channel=pBMCInfo->LanIfcConfig[i].Chnum;
        }
    }

    Ethindex=GetEthIndex(Channel, BMCInst);
    if(Ethindex == 0xff)
    {
        TCRIT("Error in getting Ethindex");
        SetPendStatus (PEND_OP_SET_BOND_IFACE_STATE,PEND_STATUS_COMPLETED);
        return 0;
    }

    /* Read VLAN ID for bond interface */
    if(ReadVLANFile(VLAN_ID_SETTING_STR, vlanID) == -1)
    {
        //return -1;
    }

    nwConfigureBonding(pConfigBond,Ethindex,g_corefeatures.timeoutd_sess_timeout,g_corefeatures.global_ipv6);

    /*Disable the VLAN interface properly before disabling bond interface*/
    if(pConfigBond->Enable == 0 && vlanID[EthIndex] != 0)
    {
        if(WriteVLANFile(VLAN_ID_SETTING_STR, vlanID, EthIndex, 0) == -1)
        {
            SetPendStatus (PEND_OP_SET_BOND_IFACE_STATE,PEND_STATUS_COMPLETED);
            return -1;
        }

        if(ReadVLANFile(VLAN_PRIORITY_SETTING_STR, VLANPriorityLevel) == -1)
        {
            SetPendStatus (PEND_OP_SET_BOND_IFACE_STATE,PEND_STATUS_COMPLETED);
            return -1;
        }

        if(WriteVLANFile(VLAN_PRIORITY_SETTING_STR, VLANPriorityLevel, EthIndex, 0) == -1)
        {
            SetPendStatus (PEND_OP_SET_BOND_IFACE_STATE,PEND_STATUS_COMPLETED);
            return -1;
        }

        if(ReadVLANFile(VLAN_ID_SETTING_STR, pBMCInfo->LANConfig.VLANID) == -1)
        {
            //return -1;
        }

        /*Update the NVRAM Configuration*/
        pBMCInfo->LANCfs[Ethindex].VLANID = 0;
        FlushIPMI((INT8U*)&pBMCInfo->LANCfs[0],(INT8U*)&pBMCInfo->LANCfs[Ethindex],pBMCInfo->IPMIConfLoc.LANCfsAddr,
                      sizeof(LANConfig_T),BMCInst);
    }

    SetPendStatus (PEND_OP_SET_BOND_IFACE_STATE,PEND_STATUS_COMPLETED);

    return 0;
}

static int PendActiveSlave(INT8U * pData, INT32U DataLen, INT8U Ethindex, int BMCInst)
{
    ActiveSlave_T * pReq= (ActiveSlave_T *)pData;

    nwActiveSlave(pReq->BondIndex,pReq->ActiveIndex);
    SetPendStatus(PEND_OP_SET_ACTIVE_SLAVE,PEND_STATUS_COMPLETED);
    return 0;
}

static int PendRestartServices(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{
	BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
	RestartService_T *pReq = (RestartService_T *) pData;
	int i;
	struct stat buf;
	
	if(pReq->ServiceName == IPMI)
	{
		close(pBMCInfo->UDSConfig.UDSSocket);	

		for(i=0;i<MAX_LAN_CHANNELS;i++)
		{		
			if (-1 != pBMCInfo->LANConfig.TCPSocket[i]) {
				shutdown(pBMCInfo->LANConfig.TCPSocket[i],SHUT_RDWR);
				close(pBMCInfo->LANConfig.TCPSocket[i]);
				pBMCInfo->LANConfig.TCPSocket[i] = 0;
			}
		}

		safe_system("/etc/init.d/ipmistack restart");
	}
	else if(pReq->ServiceName == WEBSERVER)
	{
		sleep(pReq->SleepSeconds);
		//remove tmp/sdr_data file, to reflect the appropriate SDR change in webUI
		unlink("/var/tmp/sdr_data");

		//restart the webserver to avoid the websession hang
		if(stat("/etc/init.d/webgo.sh",&buf) == 0)
		{
			safe_system("/etc/init.d/webgo.sh restart &");
		}
		else if(stat("/etc/init.d/lighttpd.sh",&buf) == 0)
		{
			safe_system("/etc/init.d/lighttpd.sh restart &");
		}

	}
	else if(pReq->ServiceName == REBOOT && access("/var/yafu_bios_update_selection", F_OK) != 0)
	{	
		sleep(pReq->SleepSeconds);
		
		//reboot the BMC stack
		reboot (LINUX_REBOOT_CMD_RESTART);
	}
	else if(pReq->ServiceName == ADVISER_CONF)
	{
		sleep(pReq->SleepSeconds);
		if(stat("/etc/init.d/adviserd.sh",&buf) == 0)
		{
			safe_system("/etc/init.d/adviserd.sh restart_conf &");
		}
	}
	SetPendStatus(PEND_OP_RESTART_SERVICES,PEND_STATUS_COMPLETED);
	return 0;
}

static int PendStartFwUpdate_Tftp(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{
    FirmwareConfig_T *FwConfig = (FirmwareConfig_T*)pData;
    unsigned char PreserveCfg = 0;
    unsigned char ResetBMC = 1;
    int ProgressState = 0;
    unsigned char ProtocolType, flashType=FLASH_REQUIRED;
    ImageVerificationInfo VeriInfo;
    STRUCTURED_FLASH_PROGRESS flprog;
    STRUCTURED_DOWNLOAD_PROGRESS dlprog;
    unsigned char  retval;

    PreserveCfg = FwConfig->Protocol.tftp.PreserveCfg;
    ProtocolType = FwConfig->ProtocolType;
    memset(&VeriInfo,0,sizeof(ImageVerificationInfo));
    memset(&flprog, 0, sizeof(STRUCTURED_FLASH_PROGRESS));
    memset(&dlprog, 0, sizeof(STRUCTURED_DOWNLOAD_PROGRESS));

    if(access("/var/bios_update_selection", F_OK) != 0)
    {

        TINFO("PrepareFlashArea...\n");
        retval =PrepareFlashArea(FLSH_CMD_PREP_TFTP_FLASH_AREA, g_corefeatures.dual_image_support);
        if (retval != 0)
        {
          goto FlashError;   
        }

        sleep(2);
        TINFO("DownloadFwImage...\n");
        retval = DownloadFwImage(ProtocolType);
        if (retval != 0)
        {
          goto FlashError;
        }
        do {
           sleep(2);
           ProgressState = GetDownloadProgress(&dlprog);
           TDBG("DL progress - %s , %s, %d, %d\n", dlprog.SubAction, dlprog.Progress, dlprog.State, ProgressState);
        } while(!ProgressState);

        if(dlprog.State != DLSTATE_COMPLETE)
        {
            SetPendStatus(PEND_OP_START_FW_UPDATE_TFTP, PEND_STATUS_COMPLETED);
            return 0;
        }
    
        sleep(2);
        TDBG("VerifyFirmwareImage\n");
        retval = VerifyFirmwareImage(&VeriInfo,NULL,NULL);
        if (retval != 0)
        {
          goto FlashError;
        }
    	if(g_corefeatures.dual_image_support == ENABLED)
    	{
            /* Need to flash both firmware  images if uboot version changes */
            if ( (VeriInfo.Status & BOOT_VERSION_CHANGE) == BOOT_VERSION_CHANGE )
            {
                flashType = FLASH_TYPE_BOTH_IMAGES;
                TINFO("uboot versions mismatched going to flash both firmware images..\n");
            }
    	}
		
        TDBG("StartImageFlash\n");
        StartImageFlash(PreserveCfg, ResetBMC,flashType,NULL,0);
        do {
            sleep(2);
            ProgressState = GetFlashProgress(&flprog);
         
            if((flprog.State == FLSTATE_DOING) || (flprog.State == FLSTATE_TOBEDONE)) {
        	continue;
            }
        
            printf("FL progress - %s , %s, %d, %d\n", flprog.SubAction, flprog.Progress, flprog.State, ProgressState);
        } while(!ProgressState);

        if(flprog.State == FLSTATE_COMPLETE)
           TINFO("FLASH Complete...\n");

        RestartDeviceWithNewFirmware();
        FlashError: 
 	           AbortFlash(g_corefeatures.online_flashing_support);
        
    }
    else
    {
    TINFO("PrepareFlashArea...\n");
    if(PrepareFlashArea(FLSH_CMD_PREP_TFTP_FLASH_AREA, 0) == 0 )
    {
        sleep(2);
        TWARN("DownloadFwImage...\n");
        if(DownloadFwImage(ProtocolType) == 0)
        {
            do {
                sleep(2);
                ProgressState = GetDownloadProgress(&dlprog);
                TDBG("DL progress - %s , %s, %d, %d\n", dlprog.SubAction, dlprog.Progress, dlprog.State, ProgressState);
            } while(!ProgressState);

            if(dlprog.State == DLSTATE_ERROR)
            {
                DeactivateBiosFlashMode();
                SetPendStatus(PEND_OP_START_FW_UPDATE_TFTP, PEND_STATUS_COMPLETED);
                return 0;
            }

            sleep(2);

            TDBG("VerifyFirmwareImage\n");
            if(VerifyFirmwareImage(&VeriInfo, NULL, NULL) == 0)
            {
                TDBG("StartImageFlash\n");
                if(StartImageFlash(PreserveCfg, ResetBMC, FLASH_REQUIRED, NULL, 0) == 0)
                {
                    do {
                        sleep(2);
                        ProgressState = GetFlashProgress(&flprog);
                        if((flprog.State == FLSTATE_DOING) || (flprog.State == FLSTATE_TOBEDONE)) {
                            continue;
                        }
                    printf("FL progress - %s , %s, %d, %d\n", flprog.SubAction, flprog.Progress, flprog.State, ProgressState);
                    } while(!ProgressState);
                    SetBiosResetSetting();
                }
            }
        }
    }
    }
    SetPendStatus(PEND_OP_START_FW_UPDATE_TFTP, PEND_STATUS_COMPLETED);
    return 0;
}

static int PendSetNCSIChannelID(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{	
	int ret = 0;
	
	if(safe_system("/usr/local/bin/ncsicfg; sleep 5")<0)
	{
		ret = -1;
	}	
	
	SetPendStatus(PEND_OP_SET_NCSI_CHANNEL_ID, PEND_STATUS_COMPLETED);

	return ret;
}

static int PendSetNCSIModeChange(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{	
	int ret = 0;
	
	if(safe_system("/usr/local/bin/ncsicfg; sleep 5")<0)
	{
		ret = -1;
	}	
	
	SetPendStatus(PEND_OP_SET_NCSI_MODE_CHANGE, PEND_STATUS_COMPLETED);

	return ret;
}

static int PendSetNCSIVetoBit(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{	
	int ret = 0;
	
	if(safe_system("/usr/local/bin/ncsicfg 1")<0)
	{
		ret = -1;
	}	
	
	SetPendStatus(PEND_OP_SET_NCSI_VETOBIT, PEND_STATUS_COMPLETED);

	return ret;
}

static int PendSetSpeed(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{
    PHYConfig_T         *PHYConfig = (PHYConfig_T *)pData;
    char Ifcname[16] = {0};
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    BondIface   bond;
    unsigned char Upslave;
    
    if(IsBondingActive ( BMCInst ) && (pBMCInfo->BondConfig.BondMode == BOND_ACTIVE_BACKUP))
    {
        nwGetBondConf(&bond, pBMCInfo->BondConfig.BondIndex);
        nwGetBondActiveSlave(pBMCInfo->BondConfig.BondIndex, &Upslave);
    }

    TDBG ("PendSetSpeed Invoked : %x %x %x\n", PHYConfig->AutoNegotiationEnable, PHYConfig->Duplex, PHYConfig->Speed);
    
    if(GetIfcNameByIndex(pBMCInfo->LANConfig.g_ethindex, Ifcname) == -1)
    {
        SetPendStatus(PEND_OP_SET_SPEED, PEND_STATUS_COMPLETED);
        return -1;
    }
    SetNWSpeed(PHYConfig, Ifcname);

    SetPendStatus(PEND_OP_SET_SPEED, PEND_STATUS_COMPLETED);
    if(IsBondingActive ( BMCInst ) && (bond.BondMode == BOND_ACTIVE_BACKUP))
    {
        sleep(5);
        nwActiveSlave(pBMCInfo->BondConfig.BondIndex, Upslave);
    }
    return 0;
}

static int PendSetMTUSize(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{
    INT16U *MTU_size = (INT16U *)pData;
    char Ifcname[16] = {0};
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    if(GetIfcName(pBMCInfo->LANConfig.g_ethindex, Ifcname, BMCInst) == -1)
    {
        SetPendStatus(PEND_OP_SET_MTU_SIZE, PEND_STATUS_COMPLETED);
        return -1;
    }
    SetNWMTUSize(*MTU_size, Ifcname);

    SetPendStatus(PEND_OP_SET_MTU_SIZE, PEND_STATUS_COMPLETED);
    return 0;
}

static int PendSetNWPHYRegister(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{
    SetPHYReg_T *PHYReg = (SetPHYReg_T *)pData;
    char Ifcname[16] = {0};
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT16S RegValue = 0;

    if(GetIfcName(pBMCInfo->LANConfig.g_ethindex, Ifcname, BMCInst) == -1)
    {
        SetPendStatus(PEND_OP_SET_NW_PHY_REGISTER, PEND_STATUS_COMPLETED);
        return -1;
    }
    
    if (PHYReg->IsReplace == 0)
    {
        RegValue = (INT16S)nwGetPHYRegister(PHYReg->RegNumber, Ifcname); /* Restart Autoneg RMW (Read Modify Write)  */
        if(RegValue == -1 || RegValue == 0)
        {
            SetPendStatus(PEND_OP_SET_NW_PHY_REGISTER, PEND_STATUS_COMPLETED);
            return -1;
        }
        RegValue |= PHYReg->RegValue;
    }
    else
    {
        RegValue = PHYReg->RegValue;
    }
    
    if(nwSetPHYRegister(RegValue, PHYReg->RegNumber, Ifcname) == -1)
    {
        SetPendStatus(PEND_OP_SET_NW_PHY_REGISTER, PEND_STATUS_COMPLETED);
        return -1;
    }
    
    SetPendStatus(PEND_OP_SET_NW_PHY_REGISTER, PEND_STATUS_COMPLETED);
    return 0;
}

static int PendSetBlockAll(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{
    INT8U Block = (INT8U)*pData;
    void (*dl_block)( void*);
    void *dl_handle = NULL;

    dl_handle = dlopen ( "/usr/local/lib/libiptables.so", RTLD_NOW ); 
    if(NULL == dl_handle)
    {
        IPMI_ERROR("Error in loading libiptables.so library %s\n", dlerror() );
        SetPendStatus(PEND_OP_SET_BLOCK_ALL, PEND_STATUS_COMPLETED);
        return 0;
    }

    dl_block = dlsym(dl_handle,"block_all");
    if(NULL == dl_block)
    {
        IPMI_ERROR("Error un getting symbol %s\n",dlerror());
        dlclose(dl_handle);
        SetPendStatus(PEND_OP_SET_BLOCK_ALL, PEND_STATUS_COMPLETED);
        return 0;
    }

    dl_block((void *)&Block);

    dlclose (dl_handle);
    SetPendStatus(PEND_OP_SET_BLOCK_ALL, PEND_STATUS_COMPLETED);
    return 0;
}


static int PendSetBlockAllTimeout(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{  
    void (*dl_block)( void*);
    void *dl_handle = NULL;

    dl_handle = dlopen ( "/usr/local/lib/libiptables.so", RTLD_NOW ); 
    if(NULL == dl_handle)
    {
        IPMI_ERROR("Error in loading libiptables.so library %s\n", dlerror() );
        SetPendStatus(PEND_OP_SET_BLOCK_ALL_TIMEOUT, PEND_STATUS_COMPLETED);
        return 0;
    }

    dl_block = dlsym(dl_handle,"block_all_timeout");
    if(NULL == dl_block)
    {
        IPMI_ERROR("Error un getting symbol %s\n",dlerror());
        dlclose(dl_handle);
        SetPendStatus(PEND_OP_SET_BLOCK_ALL_TIMEOUT, PEND_STATUS_COMPLETED);
        return 0;
    }
    dl_block((void *)pData);

    dlclose (dl_handle);
    SetPendStatus(PEND_OP_SET_BLOCK_ALL_TIMEOUT, PEND_STATUS_COMPLETED);
    return 0;
}

static int PendManageBMCBkupConfig(INT8U *pData, INT32U DataLen, INT8U Ethindex, int BMCInst)
{
    ManageBMCConfig_T *pManageBMCConfig = (ManageBMCConfig_T *)pData;
    int status = -1;
    void *dl_handle = NULL;
    int ( *dl_func1 )(char *) = NULL;
    int ( *dl_func2 )(char *, char *) = NULL;
    struct stat buf;

    dl_handle = dlopen ("/usr/local/lib/libBackupConf.so", RTLD_NOW );
    if(NULL == dl_handle)
    {
        IPMI_ERROR("Error in loading libBackupConf.so library %s\n", dlerror());
        SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_UNSPECIFIED_ERR);
        return 0;
    }

    switch(pManageBMCConfig->Parameter)
    {
    case BACKUP_CONF:

        dl_func1 = dlsym (dl_handle, "BackupConfFile");
        if ( NULL == dl_func1 )
        {
            IPMI_ERROR("Error in getting symbol %s \n", dlerror());
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_UNSPECIFIED_ERR);
            break;
        }

        status = dl_func1(pManageBMCConfig->ConfBackupFile);
        if (status)
        {
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_UNSPECIFIED_ERR);
        }
        else
        {
            SetPendStatus(PEND_OP_MANAGE_BMC_BKUPCONFIG, PEND_STATUS_COMPLETED);
        }
        break;

    case RESTORE_CONF:

        dl_func1 = dlsym (dl_handle, "RestoreConfFile");
        if ( NULL == dl_func1 )
        {
            IPMI_ERROR("Error in getting symbol %s \n", dlerror());
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_UNSPECIFIED_ERR);
            break;
        }
        if(!stat(pManageBMCConfig->ConfBackupFile, &buf))
        {
            status = dl_func1(pManageBMCConfig->ConfBackupFile);
        }
        else
        {
            TCRIT("%s file is not found");
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_FILE_NOT_FOUND);
            break;
        }

        if (!status)
        {
            SetPendStatus(PEND_OP_MANAGE_BMC_BKUPCONFIG, PEND_STATUS_COMPLETED);
            sleep(3);
            //reboot the BMC stack
            reboot(LINUX_REBOOT_CMD_RESTART);
        }
        else
        {
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_UNSPECIFIED_ERR);
        }
        break;

    case EXPORT_CONF:

        dl_func2 = dlsym (dl_handle, "ExportConfFile");
        if ( NULL == dl_func2 )
        {
            IPMI_ERROR("Error in getting symbol %s \n", dlerror());
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_UNSPECIFIED_ERR);
            break;
        }

        if(!stat(pManageBMCConfig->ConfBackupFile, &buf))
        {
            status = dl_func2(pManageBMCConfig->ConfBackupFile, pManageBMCConfig->IPAddress);
        }
        else
        {
            TCRIT("%s file is not found");
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_FILE_NOT_FOUND);
            break;
        }

        if (-1 == status)
        {
            TCRIT("Tftp connection error. File is not downloaded.");
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_TFTP_CONNECTION_FAILED);
        }
        else
        {
            SetPendStatus(PEND_OP_MANAGE_BMC_BKUPCONFIG, PEND_STATUS_COMPLETED);
        }
        break;

    case IMPORT_CONF:

        dl_func2 = dlsym (dl_handle, "ImportConfFile");
        if ( NULL == dl_func2 )
        {
            IPMI_ERROR("Error in getting symbol %s \n", dlerror());
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_UNSPECIFIED_ERR);
            break;
        }
        status = dl_func2(pManageBMCConfig->ConfBackupFile, pManageBMCConfig->IPAddress);

        if (-1 == status)
        {
            TCRIT("Tftp connection error. File is not uploaded.");
            SetPendStatusError(PEND_OP_MANAGE_BMC_BKUPCONFIG, CC_TFTP_CONNECTION_FAILED);
        }
        else
        {
            SetPendStatus(PEND_OP_MANAGE_BMC_BKUPCONFIG, PEND_STATUS_COMPLETED);
        }
        break;
    }
    if (NULL != dl_handle)
        dlclose(dl_handle);

    return 0;
}

static int TriggerDelayedLANTimeout(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{
    SetPendStatus(TRIGGER_DELAYED_LAN_TIMEOUT, PEND_STATUS_COMPLETED);
    return 0;
}

static int ForcefulLANRestart(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{
    int retValue;
    UINT8 j;
    UINT8 EthIndex = Ethindex;
    char IFname[16];
    char cmd[64];
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    if(1 == pBMCInfo->BondConfig.Enable)
    {
        memset(IFname, 0, sizeof(IFname));
        snprintf(IFname,sizeof(IFname),"bond%d",pBMCInfo->BondConfig.BondIndex);
        for(j=0;j<MAX_LAN_CHANNELS;j++)
        {
           
            //considering eth0 vlan configutions when ever bond available
            Ethindex = 0; 
		
            //checking real net eth index for bond
            if(strncmp(IFname,Ifcnametable[j].Ifcname,sizeof(IFname)) == 0)
            {
                   EthIndex= Ifcnametable[j].Index;
                   break;
            }    

        }
    }
    if(((pBMCInfo->LANCfs[Ethindex].VLANID & VLAN_MASK_BIT) == VLAN_MASK_BIT))
    {
        snprintf(cmd,sizeof(cmd),"%s%d",VLAN_NETWORK_DECONFIG_FILE,EthIndex);
        safe_system(cmd);
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd,sizeof(cmd),"%s%d",VLAN_NETWORK_CONFIG_FILE,EthIndex);
        safe_system(cmd);
        safe_system("ifdown lo; ifup lo");
    }
    else
    {
        safe_system("/etc/init.d/networking restart");
    }
    
    if ((g_corefeatures.delayed_lan_restart_support) && (g_PDKHandle[PDK_AFTERDELAYEDLANRESTART] != NULL ))
    {
        retValue = ((int(*)(int))(g_PDKHandle[PDK_AFTERDELAYEDLANRESTART]))(BMCInst);
        if(retValue != 0)
        {
            TCRIT("Error setting eth information retValue %d \n",retValue);
        }
    }
    SetPendStatus(FORCEFUL_LAN_RESTART, PEND_STATUS_COMPLETED);
    
    return 0;
}



static int PendSetNCSIDetect(INT8U *pData, INT32U DataLen, INT8U Ethindex,int BMCInst)
{
    safe_system("echo 1 > /proc/sys/ractrends/ncsi/Detect; sleep 5");
    SetPendStatus(PEND_OP_SET_NCSI_DETECT, PEND_STATUS_COMPLETED);
    return 0;
}


/**
*@fn PendSetIPv6Headers
*@brief This function is invoked to set IPv6 headers.
*@param pData -  Pointer to buffer where network configurations saved
*@param DataLen -  unsigned integer parameter to buffer where length of the input data specified
*@param EthIndex -  char value to bufferr where index for Ethernet channel is saved
*@return Returns 0 on success
*/
int PendSetIPv6Headers(INT8U* pData, INT32U DataLen,INT8U EthIndex,int BMCInst)
{
    MsgPkt_T MsgPkt;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    MsgPkt.Param =LAN_CONFIG_IPV6_HEADER;
    MsgPkt.Channel = pBMCInfo->LANConfig.g_ethindex;

    if(pBMCInfo->LANConfig.UDPSocket[EthIndex] != -1)
    {
        MsgPkt.Socket = pBMCInfo->LANConfig.UDPSocket[EthIndex];
    }

    /* Post the request packet to LAN Interface Queue */
    if (0 != PostMsg (&MsgPkt, LAN_IFC_Q, BMCInst))
    {
        IPMI_WARNING ("LANIfc.c : Error posting message to LANIfc Q\n");
    }

    SetPendStatus (PEND_OP_SET_IPV6_HEADERS,PEND_STATUS_COMPLETED);
    return 0;
}




static int  PendSetIPv6StaticAddress (INT8U *pData, INT32U DataLen, INT8U EthIndex,int BMCInst)
{

     NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;
    DOMAINCONF      DomainCfg;
    DNSCONF     DNS;
    INT8U               regBMC_FQDN[MAX_LAN_CHANNELS];
    IPv6Addrs_T                  *NewIPv6Addr = (IPv6Addrs_T *)pData;

    memset(&DomainCfg,0,sizeof(DOMAINCONF));
    memset(&DNS,0,sizeof(DNSCONF));
    memset(regBMC_FQDN,0,sizeof(regBMC_FQDN));

  
    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);
    NWConfig6.CfgMethod= ((NWCFG6_STRUCT*)pData)->CfgMethod;

    if(NWConfig6.CfgMethod == CFGMETHOD_STATIC)
    {
        ReadDNSConfFile(&DomainCfg, &DNS, regBMC_FQDN);

        if(DomainCfg.v4v6 == 2)
            DomainCfg.v4v6 = 0;

        if(DNS.DNSDHCP== 1)
            DNS.DNSDHCP= 0;

        WriteDNSConfFile(&DomainCfg, &DNS, regBMC_FQDN);
    }

    memcpy( NWConfig6.GlobalIPAddr[(NewIPv6Addr->SetSelector & 0x0F)], NewIPv6Addr->IPv6_Address, IP6_ADDR_LEN );
    NWConfig6.GlobalPrefix[(NewIPv6Addr->SetSelector & 0x0F)] = NewIPv6Addr->IPv6_PrefixLength;

   
    nwWriteNWCfg_ipv4_v6( &NWConfig, &NWConfig6,EthIndex);
    SetPendStatus (PEND_OP_SET_IPV6_STATIC_IP_ADDR,PEND_STATUS_COMPLETED);
   return 0;
}

/*
 *@fn PendSetIfaceState
 *@brief This function is invoked to set network interface state to enable or disable
 *@param pData -  Pointer to buffer which hold the data to be posted
 *@param DataLen -  Specifies the length of the message to be posted
 *@param EthIndex -  Ethernet Index
 */
int PendSetIPv6IPv4AddrEnable(INT8U* pData, INT32U DataLen, INT8U EthIndex,int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U IPv6IPv4AddrEnable = *pData;
    NWCFG_STRUCT cfg;
    NWCFG6_STRUCT cfg6;
    int retValue = 0;
    int eth_index = 0;

    eth_index = EthIndex;
 
    retValue = nwReadNWCfg_v4_v6(&cfg, &cfg6, eth_index,g_corefeatures.global_ipv6);
    if(retValue != 0)
        TCRIT("Error in reading network configuration.\n");
 
    switch(IPv6IPv4AddrEnable)
    {
        case DISABLE_V6:
            cfg.enable = 1;
            cfg6.enable = 0;
            break;

        case EABLE_V6_DISABLE_V4:
            cfg.enable = 0;
            cfg6.enable = 1;
            break;

        case ENABLE_V6_V4:
            cfg.enable = 1;
            cfg6.enable = 1;
            break;
			
       default:
            SetPendStatus (PEND_OP_SET_IPV6_IPV4_ADDR_ENABLE, PEND_STATUS_COMPLETED);
            return -1;
            break;
    }

    cfg.CfgMethod = pBMCInfo->LANCfs[eth_index].IPAddrSrc;
    cfg6.CfgMethod = pBMCInfo->LANCfs[eth_index].IPv6_IPAddrSrc;

    if(cfg.CfgMethod == STATIC_IP_SOURCE)
    {
        //Set IP address
        _fmemcpy (&cfg.IPAddr, pBMCInfo->LANCfs[eth_index].IPAddr, IP_ADDR_LEN);
        //Set subnet mask
        _fmemcpy (&cfg.Mask, pBMCInfo->LANCfs[eth_index].SubNetMask, IP_ADDR_LEN);
        //Set default gateway
        _fmemcpy (&cfg.Gateway, pBMCInfo->LANCfs[eth_index].DefaultGatewayIPAddr, IP_ADDR_LEN);
    }

    if(cfg6.CfgMethod == STATIC_IP_SOURCE)
    {
        _fmemcpy(cfg6.GlobalIPAddr,pBMCInfo->LANCfs[eth_index].IPv6_IPAddr[0],IP6_ADDR_LEN*MAX_IPV6ADDRS);
        _fmemcpy(&cfg6.Gateway,pBMCInfo->LANCfs[eth_index].IPv6_GatewayIPAddr,IP6_ADDR_LEN);
        _fmemcpy(cfg6.GlobalPrefix, pBMCInfo->LANCfs[eth_index].IPv6_PrefixLen,MAX_IPV6ADDRS);
    }
  
    if(cfg6.enable == 0)
        retValue = nwWriteNWCfg_ipv4_v6(&cfg, NULL, eth_index);
    else
        retValue = nwWriteNWCfg_ipv4_v6(&cfg, &cfg6, eth_index);

    if(retValue != 0)
    TCRIT("Error in writing network configuration.\n");

    SetPendStatus (PEND_OP_SET_IPV6_IPV4_ADDR_ENABLE, PEND_STATUS_COMPLETED);

    return 0;
}
 
static int PendSetRAIPAddr(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
    RACFG_T	rCfg = {0};
    RAipV6Addr_T *RAipV6Addr = (RAipV6Addr_T *)pData;
    nwReadRACfg(&rCfg,EthIndex);
    rCfg.IPv6_RA_Conf_Cntl_Enable |= 0x1;
    if ( 0x1 == RAipV6Addr->RAipV6_Cnt)
        _fmemcpy( &rCfg.IPv6_Router1_IPAddr, RAipV6Addr->RAipV6_IPAddr, IP6_ADDR_LEN );

    else if ( 0x2 == RAipV6Addr->RAipV6_Cnt)
        _fmemcpy( &rCfg.IPv6_Router2_IPAddr, RAipV6Addr->RAipV6_IPAddr, IP6_ADDR_LEN );

    nwWriteRACfg(&rCfg,EthIndex);
    SetPendStatus (PEND_OP_SET_RA_IPADDR, PEND_STATUS_COMPLETED);
    return 0;

}

static int PendSetRAPrefixLen(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
    RACFG_T  rCfg = {0};
    RAipV6Prefix_T *RAipV6Prefix = (RAipV6Prefix_T *)pData;
    nwReadRACfg(&rCfg,EthIndex);
    if ( 0x1 == RAipV6Prefix->RAipV6_Cnt)
        rCfg.IPv6_Router1_PrefixLen = RAipV6Prefix->RAipV6_PrefixLen;

    else if ( 0x2 == RAipV6Prefix->RAipV6_Cnt)
        rCfg.IPv6_Router2_PrefixLen = RAipV6Prefix->RAipV6_PrefixLen;

    nwWriteRACfg(&rCfg,EthIndex);
    SetPendStatus (PEND_OP_SET_RA_PREFIXLEN, PEND_STATUS_COMPLETED);
    return 0;
}

static int PendSetNTPState(INT8U* pData, INT32U DataLen, INT8U EthIndex, int BMCInst)
{
    INT8U PrimServer[MAX_SERVER_LEN]={0};
    INT8U SecServer[MAX_SERVER_LEN]={0};
    INT8U Status[MAX_STATUS_LEN]={0};
    INT8U update_ntp[256]={0};
    int ret = 0;
    void *dl_handle = NULL;	
    int (*libami_getntpServer)(char *, char *, unsigned int);
    int (*libami_setntpStatus)(char *);
    strncpy((char*)Status, "Auto", MAX_STATUS_LEN);
    dl_handle = dlopen((char *)NTPCONF_LIB, RTLD_NOW);
    if(!dl_handle)
    {
        syslog(LOG_ERR, "Error in loading ntpconf library");
        SetPendStatus (PEND_OP_SET_NTP_NTPSTATE, PEND_STATUS_ERROR);
        goto ret;
    }
    libami_getntpServer = dlsym(dl_handle,"libami_getntpServer");
    if (libami_getntpServer)
    {
        if(libami_getntpServer((char*)PrimServer,(char*)SecServer, MAX_SERVER_LEN))
        {
            IPMI_ERROR("\n Error in getting primary and secondary ntp server\n");
            dlclose(dl_handle);
            SetPendStatus (PEND_OP_SET_NTP_NTPSTATE, PEND_STATUS_ERROR);
            goto ret;
        }
    }
    snprintf((char*)update_ntp,sizeof(update_ntp), "ntpdate -b -s -u %s", PrimServer);
    ret = safe_system((char*)update_ntp);
    if(0 != ret)
    {
        IPMI_ERROR("\n NTP update failure in primary server::%d\n", ret);
        snprintf((char*)update_ntp,sizeof(update_ntp), "ntpdate -b -s -u %s", SecServer);
        ret = safe_system((char*)update_ntp);
        if(0 != ret)
        {
            IPMI_ERROR("\n NTP update failure in secondary server::%d\n", ret);
            strncpy((char*)Status, "Failure", MAX_STATUS_LEN);
        }
    }
    libami_setntpStatus = dlsym(dl_handle,"libami_setntpStatus");
    if (libami_setntpStatus)
    {
        if(libami_setntpStatus((char*)Status))
        {
            IPMI_ERROR("\n Error in Setting NTP status\n");
            dlclose(dl_handle);
            SetPendStatus (PEND_OP_SET_NTP_NTPSTATE, PEND_STATUS_ERROR);
            goto ret;
        }
    }
    dlclose(dl_handle);
    if(0 != RestartDaemonByForce("crond", "cron", "/var/run/crond.reboot", SIGKILL))
    {
        SetPendStatus (PEND_OP_SET_NTP_NTPSTATE, PEND_STATUS_ERROR);
        goto ret;
    }
    SetPendStatus (PEND_OP_SET_NTP_NTPSTATE, PEND_STATUS_COMPLETED);
ret:return 0;
}

