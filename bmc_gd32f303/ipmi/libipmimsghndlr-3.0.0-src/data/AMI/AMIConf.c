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
 *
 * AMIConf.c
 * AMI specific configuration commands related implementation.
 *
 * Author: Benson Chuang <bensonchuang@ami.com.tw>
 *****************************************************************/

#include "Debug.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "AMIRestoreDefaults.h"
#include "flshfiles.h"
#include "flashlib.h"
#include "PendTask.h"
#include "PDKDefs.h" 
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <linux/if.h>
#include "OSPort.h"
#include "NVRData.h"
#include "NVRAccess.h"
#include "PMConfig.h"
#include "IPMI_AMIConf.h"
#include "AMIConf.h"
#include "nwcfg.h"
#include "Ethaddr.h"
#include "MsgHndlr.h"
#include "ncml.h"
#include "IPMIConf.h"
#include "hostname.h"
#include <dlfcn.h>
#include "stunnel_cfg.h"
#include "unix.h"
#include "Sensor.h"
#include "SDR.h"
#include <flashlib.h>
#include "featuredef.h"
#include "SEL.h"
#include "blowfish.h"
#include "vmedia_cfg.h"
#include <netdb.h>
#include "AppDevice.h"
#include "userprivilege.h"
#include "Extendedpriv.h"
#include "Platform.h"
#include "safesystem.h"
#include "LANConfig.h"
#include "hal_hw.h"
#include "PDKCmdsAccess.h"
#include "racsessioninfo.h"
#include "validate.h"
#include  "Util.h"
#include "BMCInfo.h"


#define MAX_FULL_SEL_ENTRIES 0x4000
VMediaCfg_T gVMediaCfg;
//#define SNMP_LIBS "/usr/local/lib/libsnmpusers.so"
//#define RMS_SET  "rms_set"
#define DNS_PARAM_LENGTH 0x6

#define SESMNGT_LIB "/usr/local/lib/libracsessioninfo.so"

#define MAX_KVM_RECONNECT_INTERVAL     0x0F
#define MAX_KVM_RECONNECT_COUNT        0x03
#define MIN_KVM_RECONNECT_COUNT     0x01
#define MIN_KVM_RECONNECT_INTERVAL   0x05
/*
 * Service commands
 */
char *ServiceNameList[MAX_SERVICE_NUM] = { 
                                              WEB_SERVICE_NAME,    
                                              KVM_SERVICE_NAME,    
                                              CDMEDIA_SERVICE_NAME,
                                              FDMEDIA_SERVICE_NAME,
                                              HDMEDIA_SERVICE_NAME,
                                              SSH_SERVICE_NAME,    
                                              TELNET_SERVICE_NAME,
                                              SOLSSH_SERVICE_NAME,
                                          };

/*
 * @brief Network Interface Configuration request parameter length
 */
static const INT8U NwIfcStateConfigParamLenght [] = {
                            2, /*Eth Interface*/
                            7, /*Bond Interface*/
};

/*
 * @brief DNS configuration Request parameter length
 */
static const INT8U NwDNSConfigParamLength[] = {
                            66,
                            0xFF,
                            4,
                            64,
                            3,
};

/**
 * @brief UDS Session Information request parameter lengths
**/
static const INT8U UDSSessionInfoParamLength[]={
                                1,
                                5,
                                2,
                                2,
                                5,
                                2,
                                1,
};

extern IfcName_T Ifcnametable[MAX_LAN_CHANNELS];
/*
 * DNS related commands
 */

INT8U CheckSlaveVLANInterface(INT8U Slaves, INT8U *VLANID, int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    char IfcName[16];
    int i, j, isFirstIndex = 1;
    
    if (VLANID == NULL)
        return (CC_UNSPECIFIED_ERR);
        
    (*VLANID) = 0;
    /*Check Slaves Entry*/
    for(i = 0; i < BOND_MAX_SLAVE; i++)
    {
        if( (Slaves >> i) & IFACE_ENABLED)
        {
            if(CheckIfcEntry( i, ETH_IFACE_TYPE) != 0)
            {
                IPMI_WARNING("Slave Entry %d is not presented in interface table\n",i);
                return (CC_INV_DATA_FIELD);
            }
            
            memset(IfcName, 0, sizeof(IfcName));
            sprintf(IfcName,"eth%d", i);
            
            for(j = 0; j < sizeof(pBMCInfo->LanIfcConfig)/sizeof(LANIFCConfig_T); j++)
            {
                if(strcmp(IfcName, pBMCInfo->LanIfcConfig[j].ifname) == 0)
                {
                    TDBG ("VLANID for %s : %d %d\n", IfcName, pBMCInfo->LANConfig.VLANID[j], (*VLANID));
                    if (isFirstIndex == 1)
                    {
                        (*VLANID) = pBMCInfo->LANConfig.VLANID[j];
                        isFirstIndex = 0;
                    }
                    else
                    {
                        if(pBMCInfo->LANConfig.VLANID[j] != (*VLANID))
                        {
                            return OEMCC_VLAN_ENABLED_ON_SLAVE;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/*
 @fn CheckVLANInterface
 @brief This function is used to check VLAN interface presence
 @params Ifcname[in] - interface name BMCInst[in] BMC instance
 @returns 1 on VLAN Enabled, 0 on VLAN Disabled
 */
int CheckVLANInterface(char* Ifcname,int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    int i;

    for(i=0;i<sizeof(pBMCInfo->LanIfcConfig)/sizeof(LANIFCConfig_T);i++)
    {
        if(strcmp(Ifcname,pBMCInfo->LanIfcConfig[i].ifname) == 0)
        {
            if(pBMCInfo->LANConfig.VLANID[i] != 0)
                return 1;
        }
    }

    return 0;
}


/*
 @fn ValidateDNSReqInfo
 @brief This function is used to validate the DNS Request params DomainIndex and Domainpriority depending on DomainDHCP
 @params DomainDHCP - Domain DHCP/static
 @params DomainIndex - Domain Index
 @params Domainpriority - Domain Prioriry IPV4/IPV6 
 @returns  0 on valid set DNS conf requested, others - error codes
 */
int ValidateDNSReqInfo(INT8U DomainDHCP, INT8U DomainIndex, INT8U Domainpriority)
{
	int Ifccount=0,retValue,ifcslaved=0,i;
    if(DomainDHCP)
    {   // filter domain priorities other then ipv4 and ipv6
        if((Domainpriority!=1)&&(Domainpriority!=2))
        {
            return CC_INV_DATA_FIELD;
        }
    }
    else
    {
        if(Domainpriority != 0 || DomainIndex != 0)
        {
            return CC_INV_DATA_FIELD;
        }
    }
    Ifccount = g_coremacros.global_nic_count;
    if(DomainIndex>Ifccount)
    {
        return CC_INV_DATA_FIELD;
    }
	if(DomainDHCP)
    {
	    if(g_corefeatures.bond_support == ENABLED)
	    {
	        //loop to check if bonding is configured
	        for(i = 0; i < Ifccount; i++)
	        {
	            retValue = -1;
	            retValue = CheckBondSlave(i);
	            if(retValue == -1)
	            {
	                return CC_UNSPECIFIED_ERR;
	            }
	            if(retValue == 1)
	            {
	                ifcslaved = 1;
	                if(i == DomainIndex)
	                {
	                    return CC_IFC_ALREADY_SLAVEDTO_BOND;
	                }
	                continue;
	             }
	             if(retValue != 0)
	             {
	                 return CC_UNSPECIFIED_ERR;
	             }
	        }
	    }
	    if(ifcslaved == BONDING_NOTACTIVE)
	    {
	        for(i=0;i<(sizeof(Ifcnametable)/sizeof(Ifcnametable[0]));i++)
	        {
	            if(strstr(Ifcnametable[i].Ifcname,"bond"))
	            {
	                if(Ifcnametable[i].Index == DomainIndex)
	                {
	                    return CC_INV_DATA_FIELD;
	                }
	            }
	        }
	    }
	}
    return 0;

}

int
AMIGetDNSConf( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
    AMIGetDNSConfReq_T* pAMIGetDNSConfReq = (AMIGetDNSConfReq_T*) pReq;
    AMIGetDNSConfRes_T*  pAMIGetDNSConfRes = (AMIGetDNSConfRes_T*) pRes;
    HOSTNAMECONF HostnameConfig;
    DOMAINCONF DomainConfig;
    DNSCONF DnsIPConfig;
    INT8U regBMC_FQDN[MAX_LAN_CHANNELS];
    INT8U Ifccount;

    memset(&HostnameConfig, 0, sizeof(HostnameConfig));
    memset(&DomainConfig, 0, sizeof(DomainConfig));
    memset(&DnsIPConfig, 0, sizeof(DnsIPConfig));
    memset(regBMC_FQDN, 0, sizeof(regBMC_FQDN));

    /*Validate the Block selector*/
    if( 0 != pAMIGetDNSConfReq->Blockselect 
                && (pAMIGetDNSConfReq->Param != AMI_DNS_CONF_DOMAIN_NAME )
                && (pAMIGetDNSConfReq->Param != AMI_DNS_CONF_DNS_IP))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    nwGetAllDNSConf( &HostnameConfig, &DomainConfig, &DnsIPConfig,regBMC_FQDN );
    pAMIGetDNSConfRes->CompletionCode = CC_SUCCESS;

    switch(pAMIGetDNSConfReq->Param)
    {
        case AMI_DNS_CONF_HOST_NAME:
            memset(&pAMIGetDNSConfRes->DNSCfg.HName,0,sizeof(HOSTNAMECONF));
            pAMIGetDNSConfRes->DNSCfg.HName.HostSetting = HostnameConfig.HostSetting;
            pAMIGetDNSConfRes->DNSCfg.HName.HostNameLen= HostnameConfig.HostNameLen;
            memcpy(pAMIGetDNSConfRes->DNSCfg.HName.HostName,HostnameConfig.HostName,HostnameConfig.HostNameLen);
            return sizeof(INT8U) + sizeof(HOSTNAMECONF);

            break;
        case AMI_DNS_CONF_REGISTER:
	    if(g_corefeatures.bond_support == ENABLED)
	    {
		 Ifccount = g_coremacros.global_nic_count + 1; //count for bond interface
	    }
	    else
	    {
		  Ifccount = g_coremacros.global_nic_count;
	    }

            memcpy(pAMIGetDNSConfRes->DNSCfg.RegDNSConf,regBMC_FQDN,Ifccount);
            return sizeof(INT8U) + Ifccount;

            break;
        case AMI_DNS_CONF_DOMAIN_SETTINGS:

            pAMIGetDNSConfRes->DNSCfg.DomainConf.DomainDHCP = DomainConfig.dhcpEnable;
            pAMIGetDNSConfRes->DNSCfg.DomainConf.DomainIndex = DomainConfig.EthIndex;
            pAMIGetDNSConfRes->DNSCfg.DomainConf.Domainpriority= DomainConfig.v4v6;
            pAMIGetDNSConfRes->DNSCfg.DomainConf.DomainLen= DomainConfig.domainnamelen;

            return sizeof(INT8U) + sizeof(DomainSetting);

            break;
        case AMI_DNS_CONF_DOMAIN_NAME:

            if(pAMIGetDNSConfReq->Blockselect > MAX_BLOCK || pAMIGetDNSConfReq->Blockselect == 0)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            memcpy(pAMIGetDNSConfRes->DNSCfg.DomainName,&DomainConfig.domainname[MAX_DOMAIN_BLOCK_SIZE * (pAMIGetDNSConfReq->Blockselect -1)],MAX_DOMAIN_BLOCK_SIZE);
            return sizeof(INT8U) + MAX_DOMAIN_BLOCK_SIZE;
            break;

        case AMI_DNS_CONF_DNS_SETTING:
            pAMIGetDNSConfRes->DNSCfg.DNSConf.IPPriority = DnsIPConfig.IPPriority;
            pAMIGetDNSConfRes->DNSCfg.DNSConf.DNSDHCP = DnsIPConfig.DNSDHCP;
            pAMIGetDNSConfRes->DNSCfg.DNSConf.DNSIndex = DnsIPConfig.DNSIndex;
            return sizeof(INT8U) + sizeof(DNSSetting);
            break;

        case AMI_DNS_CONF_DNS_IP:
            if(pAMIGetDNSConfReq->Blockselect == 1)
            {
                if(IN6_IS_ADDR_V4MAPPED(&DnsIPConfig.DNSIP1))
                {
                    memcpy(pAMIGetDNSConfRes->DNSCfg.DNSIPAddr,&DnsIPConfig.DNSIP1[IP6_ADDR_LEN - IP_ADDR_LEN],IP_ADDR_LEN);
                    return sizeof(INT8U) + IP_ADDR_LEN;
                }
                else
                {
                    memcpy(pAMIGetDNSConfRes->DNSCfg.DNSIPAddr,DnsIPConfig.DNSIP1,IP6_ADDR_LEN);
                    return sizeof(INT8U) + IP6_ADDR_LEN;
                }
            }
            else if(pAMIGetDNSConfReq->Blockselect == 2)
            {
                if(IN6_IS_ADDR_V4MAPPED(&DnsIPConfig.DNSIP2))
                {
                    memcpy(pAMIGetDNSConfRes->DNSCfg.DNSIPAddr,&DnsIPConfig.DNSIP2[IP6_ADDR_LEN - IP_ADDR_LEN],IP_ADDR_LEN);
                    return sizeof(INT8U) + IP_ADDR_LEN;
                }
                else
                {
                    memcpy(pAMIGetDNSConfRes->DNSCfg.DNSIPAddr,DnsIPConfig.DNSIP2,IP6_ADDR_LEN);
                    return sizeof(INT8U) + IP6_ADDR_LEN;
                }
            }
            else if(pAMIGetDNSConfReq->Blockselect == 3)
            {
                if(IN6_IS_ADDR_V4MAPPED(&DnsIPConfig.DNSIP3))
                {
                    memcpy(pAMIGetDNSConfRes->DNSCfg.DNSIPAddr,&DnsIPConfig.DNSIP3[IP6_ADDR_LEN - IP_ADDR_LEN],IP_ADDR_LEN);
                    return sizeof(INT8U) + IP_ADDR_LEN;
                }
                else
                {
                    memcpy(pAMIGetDNSConfRes->DNSCfg.DNSIPAddr,DnsIPConfig.DNSIP3,IP6_ADDR_LEN);
                    return sizeof(INT8U) + IP6_ADDR_LEN;
                }
            }
            else
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            break;
        case AMI_DNS_CONF_DNS_RESTART:
            *pRes = OEMCC_ATTEMPT_TO_GET_WO_PARAM;
            return sizeof(INT8U);
            break;

        case AMI_DNS_CONF_DNS_ENABLE:
            pAMIGetDNSConfRes->DNSCfg.DNSEnable = DnsIPConfig.DNSEnable;
            return sizeof(INT8U) + sizeof(INT8U);
            break;

        default:
            *pRes = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
            break;
    }

    return(sizeof(AMIGetDNSConfRes_T));
}


int
AMISetDNSConf( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
    AMISetDNSConfRes_T *pAMISetDNSConfRes = (AMISetDNSConfRes_T *)pRes;
    AMISetDNSConfReq_T *pAMISetDNSConfReq = (AMISetDNSConfReq_T*)pReq;
    int i,domainnamelength=0;
    INT8U Ifccount,ifcslaved=0,status=-1,curchannel;
    int retValue;
    char *ptr = NULL;

    UINT8 reservedBit = REG_BMC_RESERVED;
    int sizeWritten = 0;
    FILE *fp = NULL;
    char tsigprivate[255] = {0};

    if(ReqLen >= 2)
    {
        ReqLen -= 2;
    }
    else
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    if(DNS_PARAM_LENGTH > pAMISetDNSConfReq->ParamSelect)
    {
        if( (ReqLen != NwDNSConfigParamLength[pAMISetDNSConfReq->ParamSelect - 1])
                && ((NwDNSConfigParamLength[pAMISetDNSConfReq->ParamSelect - 1])!=0xFF) )   
        {
            TDBG("ReqLen %ld\n",ReqLen);
            *pRes = CC_REQ_INV_LEN;
            return sizeof(INT8U);
        }
    }

    /*Validate the Block selector*/
    if( 0 != pAMISetDNSConfReq->Blockselector
                && (pAMISetDNSConfReq->ParamSelect != AMI_DNS_CONF_DOMAIN_NAME )
                && (pAMISetDNSConfReq->ParamSelect != AMI_DNS_CONF_DNS_IP))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    switch(pAMISetDNSConfReq->ParamSelect)
    {
        case AMI_DNS_CONF_HOST_NAME:
            retValue = -1;
            /*Reserved bit checking*/
            if(pAMISetDNSConfReq->DnsConfig.HName.HostSetting > 0x1)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            /*Check the Maximum host name length i.e user can send 63 bytes and last one is null character*/
            if(pAMISetDNSConfReq->DnsConfig.HName.HostNameLen > MAX_HOSTNAME_LEN - 1)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            TDBG("HostName Setting %d and Length %d\n",pAMISetDNSConfReq->DnsConfig.HName.HostSetting,pAMISetDNSConfReq->DnsConfig.HName.HostNameLen);
            if(g_PDKHandle[PDK_SETDNSCONFIG] != NULL)
            {
                retValue = ((int(*)(INT8U *,INT8U, int))g_PDKHandle[PDK_SETDNSCONFIG])((INT8U*)&pAMISetDNSConfReq->DnsConfig.HName, AMI_DNS_CONF_HOST_NAME, BMCInst);
            }
            if(retValue == -1)
            {
                LOCK_BMC_SHARED_MEM(BMCInst);
                BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostSetting = pAMISetDNSConfReq->DnsConfig.HName.HostSetting;
                BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostNameLen = pAMISetDNSConfReq->DnsConfig.HName.HostNameLen;
                memcpy(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.HostName,pAMISetDNSConfReq->DnsConfig.HName.HostName,pAMISetDNSConfReq->DnsConfig.HName.HostNameLen);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
            }
            break;

        case AMI_DNS_CONF_REGISTER:
        	
            /*Added the validation to check the request data when the NIC count is 2. This check will be removed once we support 4 NIC*/	
            if((g_coremacros.global_nic_count < MAX_LAN_CHANNELS) && (ReqLen!=3))
            {
                 *pRes = CC_REQ_INV_LEN ;
                 return sizeof(INT8U);
            }

            // Allow setting this parameter only if DNS Service is enabled
            if(BMC_GET_SHARED_MEM(BMCInst)->DNSconf.DNSEnable == 0)
            {
                TCRIT("DNS is currently disabled. Dissallowing to set the dns configurations...\n");
                *pRes = CC_DNS_CURRENTLY_NOT_ENABLED;
                return sizeof(INT8U);
            }
           if(g_corefeatures.bond_support == ENABLED)
           {
                if(g_coremacros.global_nic_count < MAX_LAN_CHANNELS)
                Ifccount = g_coremacros.global_nic_count + 1; //count for bond interface
                else
                Ifccount = g_coremacros.global_nic_count; // for  4 NIC supported BMC

                for(i = 0; i < Ifccount; i++)
                {
                     retValue = -1;
                     retValue = CheckBondSlave(i);

                    if(retValue == -1)
                    {
                        *pRes = CC_UNSPECIFIED_ERR;
                        return sizeof(INT8U);
                    }
                    if(retValue == 1)
                    {
                        ifcslaved = 1;
                        if(pAMISetDNSConfReq->DnsConfig.RegDNSConf[i])
                        {
                             *pRes = CC_IFC_ALREADY_SLAVEDTO_BOND;
                             return sizeof(INT8U);
                        }
                    }
                    if(retValue != 0)
                        TDBG("Error in reading network configuration.\n");	  

                    if((i == (Ifccount-1)) && (ifcslaved == 0))
                    {
                         if( Ifccount != MAX_LAN_CHANNELS)
                         {
                             if (pAMISetDNSConfReq->DnsConfig.RegDNSConf[g_coremacros.global_nic_count])
                             {
                                 TDBG("Attempted to set DNS Conf for Bonding Interface, which is inactive\n");
                                 *pRes = CC_BOND_DISABLED_TOCONF_DNS;
                                 return sizeof(INT8U);
                             }
                         } 
                    }
                }
            }
            else
            {
                 Ifccount = g_coremacros.global_nic_count;
                 if(Ifccount < MAX_LAN_CHANNELS)
                 {
                      if (pAMISetDNSConfReq->DnsConfig.RegDNSConf[Ifccount])
                      {
                           TDBG("Attempted to set DNS Conf for Bonding Interface, which is inactive\n");
                           *pRes = CC_BOND_DISABLED_TOCONF_DNS;
                           return sizeof(INT8U);
                      }
                  }
            }

            if (g_corefeatures.tsig_support == ENABLED) {
                reservedBit = reservedBit & REG_BMC_RESERVED_TSIG;
            }
			
            if (g_corefeatures.mdns_support == ENABLED) {
            	reservedBit = reservedBit & REG_BMC_RESERVED_MDNS;
            }


            for(i = 0; i < Ifccount; i++)
            {
                if(pAMISetDNSConfReq->DnsConfig.RegDNSConf[i] & reservedBit)
                {
                     TDBG("Register value %d\n",pAMISetDNSConfReq->DnsConfig.RegDNSConf[i]);
                     *pRes = CC_INV_DATA_FIELD;
                     return sizeof(INT8U);
                }
                if (((pAMISetDNSConfReq->DnsConfig.RegDNSConf[i] & REG_BMC_FQDN) == REG_BMC_FQDN) &&
                    ((pAMISetDNSConfReq->DnsConfig.RegDNSConf[i] & REG_BMC_TSIG) == REG_BMC_TSIG))
                {
                    TCRIT("TSIG can't be enabled with FQDN support");
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }
                if(((pAMISetDNSConfReq->DnsConfig.RegDNSConf[i] & REG_BMC_FQDN) == REG_BMC_FQDN) &&
                    ((pAMISetDNSConfReq->DnsConfig.RegDNSConf[i] & REG_BMC_HOSTNAME) == REG_BMC_HOSTNAME))
                {
                    TCRIT("AMISetDNSConf(): Enabling both FQDN and Hostname options is not allowable as these are options of DHCP");
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }
            }

            TDBG("DNS Register Settings %d %d %d\n",pAMISetDNSConfReq->DnsConfig.RegDNSConf[0],pAMISetDNSConfReq->DnsConfig.RegDNSConf[1],pAMISetDNSConfReq->DnsConfig.RegDNSConf[2]);
            LOCK_BMC_SHARED_MEM(BMCInst);
            memcpy(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.RegisterBMC,pAMISetDNSConfReq->DnsConfig.RegDNSConf,Ifccount);
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case AMI_DNS_CONF_TSIG_UPLOAD:
            if (g_corefeatures.tsig_support == ENABLED) {
                /* Check Requested Length */
                if ((ReqLen > MAX_TSIG_PRIVKEY_SIZE) || (ReqLen <= 0))
                {
                    TCRIT("Invalid Request Length\n");
                    *pRes = CC_REQ_INV_LEN;
                    return sizeof(INT8U);
                }
                fp = fopen(TEMP_TSIG_PRIVATE_FILE, "wb");
                if (fp == NULL)
                {
                    TCRIT("Error in Opening File\n");
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                sizeWritten = fwrite(&pAMISetDNSConfReq->DnsConfig.PrivateKey[0], 1, ReqLen, fp);
                if (sizeWritten != ReqLen)
                {
                    TCRIT("Error in writing into the file\n");
                    fclose(fp);
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                fclose(fp);
                /* Now Open the File for validation */
                fp = fopen(TEMP_TSIG_PRIVATE_FILE, "rb");
                if(fp == NULL)
                {
                    TCRIT("Error in Opening File\n");
                    unlink(TEMP_TSIG_PRIVATE_FILE);
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                //HMAC-MD5 Algorithm type check
                ptr = fgets(tsigprivate, sizeof(tsigprivate)-1, fp);//read 1st line
                ptr = fgets(tsigprivate, sizeof(tsigprivate)-1, fp);

                if (strcmp(tsigprivate, TSIG_ALG_TYPE_HMAC_MD5) != 0)
                {
                    TCRIT("The private key's algorithm is not HMAC-MD5.");
                    fclose(fp);
                    unlink(TEMP_TSIG_PRIVATE_FILE);
                    *pRes = CC_TSIGPRIVATEKEY_VALIDATION_FAILED;
                    return sizeof(INT8U);
                }
                fclose(fp);
                if(moveFile(TEMP_TSIG_PRIVATE_FILE, CONF_TSIG_PRIVATE_FILE) != 0)
                {
                    TCRIT("Error in moving TSIG private key file.");
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
            }
            else
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }
        break;

        case AMI_DNS_CONF_DOMAIN_SETTINGS:

            /*Check the reserve bit*/
            if(pAMISetDNSConfReq->DnsConfig.DomainConf.DomainDHCP > 1)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            status = ValidateDNSReqInfo(pAMISetDNSConfReq->DnsConfig.DomainConf.DomainDHCP, pAMISetDNSConfReq->DnsConfig.DomainConf.DomainIndex,pAMISetDNSConfReq->DnsConfig.DomainConf.Domainpriority);
	    if(status !=0)
	    {
	        *pRes = status;
		return sizeof(INT8U);
	    }
            TDBG("Domain DHCP %d\nDomainIndex %d\nDomainLen %d\nDomainpriority%d\n",
                                                                                    pAMISetDNSConfReq->DnsConfig.DomainConf.DomainDHCP,
                                                                                    pAMISetDNSConfReq->DnsConfig.DomainConf.DomainIndex,
                                                                                    pAMISetDNSConfReq->DnsConfig.DomainConf.DomainLen,
                                                                                    pAMISetDNSConfReq->DnsConfig.DomainConf.Domainpriority);
            LOCK_BMC_SHARED_MEM(BMCInst);
            BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainDHCP = pAMISetDNSConfReq->DnsConfig.DomainConf.DomainDHCP;
            BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainIndex = pAMISetDNSConfReq->DnsConfig.DomainConf.DomainIndex;
            BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainLen = pAMISetDNSConfReq->DnsConfig.DomainConf.DomainLen;
            BMC_GET_SHARED_MEM (BMCInst)->DNSconf.Domainpriority = pAMISetDNSConfReq->DnsConfig.DomainConf.Domainpriority;
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            break;
        case AMI_DNS_CONF_DOMAIN_NAME:

            if( pAMISetDNSConfReq->Blockselector == 0x0 || pAMISetDNSConfReq->Blockselector > MAX_BLOCK)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
		    }
			ptr = malloc(sizeof(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName));
			domainnamelength = sizeof(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName);
			strncpy(ptr ,(char *)BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName, domainnamelength);
                        LOCK_BMC_SHARED_MEM(BMCInst);
			if(pAMISetDNSConfReq->Blockselector == 0x01)
				memset(&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName,0,(MAX_BLOCK * MAX_DOMAIN_BLOCK_SIZE));
		
                        memcpy(&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName[ MAX_DOMAIN_BLOCK_SIZE * (pAMISetDNSConfReq->Blockselector - 1)],pAMISetDNSConfReq->DnsConfig.DomainName,MAX_DOMAIN_BLOCK_SIZE);
                        // Validate the Domain Name if the domain name is of length DomainLen, which is set by user
			if(strlen((char *)BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName) >= BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainLen)
			{
				if((ValidateDomainName((char *)BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName)) == INVALID_DOMAIN_NAME)
				{
					strncpy((char *)BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName, ptr, domainnamelength);
					UNLOCK_BMC_SHARED_MEM(BMCInst);
					free(ptr);
					ptr = NULL;
					TDBG("Validating Domain name has fialed\n");
	                                *pRes = CC_INV_DOMAIN_NAME;
	                                return sizeof(INT8U);
				}
			}
			else if(strlen((char *)BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName) > BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainLen)
		        { 
     		            strncpy((char *)BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DomainName, ptr, domainnamelength);
			    UNLOCK_BMC_SHARED_MEM(BMCInst);
			    free(ptr);
			    ptr = NULL;
			    TDBG("Domain Name Length is Greater than the preset param DomainLen(set for Domain Name)\n");
                            *pRes = CC_INV_DATA_FIELD;
                            return sizeof(INT8U);
                        }
                        UNLOCK_BMC_SHARED_MEM(BMCInst);
			
			free(ptr);
			ptr = NULL;

            break;

        case AMI_DNS_CONF_DNS_SETTING:
            /*Check the reserve bits*/
            if((pAMISetDNSConfReq->DnsConfig.DNSConf.DNSDHCP > 1) || (pAMISetDNSConfReq->DnsConfig.DNSConf.IPPriority > 2))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            status = ValidateDNSReqInfo(pAMISetDNSConfReq->DnsConfig.DNSConf.DNSDHCP, pAMISetDNSConfReq->DnsConfig.DNSConf.DNSIndex, pAMISetDNSConfReq->DnsConfig.DNSConf.IPPriority);
	    if(status !=0)
	    {
		*pRes = status;
		return sizeof(INT8U);
	    }

            TDBG("IPPriority %d\n",pAMISetDNSConfReq->DnsConfig.DNSConf.IPPriority);
            LOCK_BMC_SHARED_MEM(BMCInst);
            BMC_GET_SHARED_MEM (BMCInst)->DNSconf.IPPriority = pAMISetDNSConfReq->DnsConfig.DNSConf.IPPriority;
            BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSDHCP = pAMISetDNSConfReq->DnsConfig.DNSConf.DNSDHCP;
            BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIndex = pAMISetDNSConfReq->DnsConfig.DNSConf.DNSIndex;
            UNLOCK_BMC_SHARED_MEM(BMCInst);


            break;
        case AMI_DNS_CONF_DNS_IP:
            if(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSDHCP)
            {
                *pRes = CC_DNS_DHCP_ENABLED;
                return sizeof(INT8U);
            }
            if( pAMISetDNSConfReq->Blockselector == 0x0 || pAMISetDNSConfReq->Blockselector > MAX_DNS_IP_ADDRESS)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            if((ReqLen != IP_ADDR_LEN) && (ReqLen != IP6_ADDR_LEN))
            {
                TDBG("Reqlen %ld\n",ReqLen);
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

			/*Validate the IPv6 address*/
            if(ReqLen == IP6_ADDR_LEN && 0 != IsValidGlobalIPv6Addr((struct in6_addr*)&pAMISetDNSConfReq->DnsConfig.DNSIPAddr))
            {
                TCRIT("Invalid Global IPv6 Address\n");
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            TDBG("Block Selector %d\n",pAMISetDNSConfReq->Blockselector);
            LOCK_BMC_SHARED_MEM(BMCInst);
            if(pAMISetDNSConfReq->Blockselector == 1)
            {
                memset(&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr1,0,IP6_ADDR_LEN);
                if(ReqLen == IP_ADDR_LEN)
                {
                    BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr1[10] = 0xFF;
                    BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr1[11] = 0xFF;
                    memcpy(&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr1[12],pAMISetDNSConfReq->DnsConfig.DNSIPAddr,ReqLen);
                    TDBG("DNS ipv4 address %d %d %d %d\n",pAMISetDNSConfReq->DnsConfig.DNSIPAddr[0],pAMISetDNSConfReq->DnsConfig.DNSIPAddr[1],
                                                                                    pAMISetDNSConfReq->DnsConfig.DNSIPAddr[2],pAMISetDNSConfReq->DnsConfig.DNSIPAddr[3]);
                }
                else
                {
                    memcpy(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr1,pAMISetDNSConfReq->DnsConfig.DNSIPAddr,ReqLen);
                }
            }
            else if(pAMISetDNSConfReq->Blockselector == 2)
            {
                memset(&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr2,0,IP6_ADDR_LEN);
                if(ReqLen == IP_ADDR_LEN)
                {
                    BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr2[10] = 0xFF;
                    BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr2[11] = 0xFF;
                    memcpy(&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr2[12],pAMISetDNSConfReq->DnsConfig.DNSIPAddr,ReqLen);
                }
                else
                {
                    memcpy(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr2,pAMISetDNSConfReq->DnsConfig.DNSIPAddr,ReqLen);
                }
            }
            else if(pAMISetDNSConfReq->Blockselector == 3)
            {
                memset(&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr3,0,IP6_ADDR_LEN);
                if(ReqLen == IP_ADDR_LEN)
                {
                    BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr3[10] = 0xFF;
                    BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr3[11] = 0xFF;
                    memcpy(&BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr3[12],pAMISetDNSConfReq->DnsConfig.DNSIPAddr,ReqLen);
                    TDBG("DNS ipv4 address %d %d %d %d\n",pAMISetDNSConfReq->DnsConfig.DNSIPAddr[0],pAMISetDNSConfReq->DnsConfig.DNSIPAddr[1],
                                                                                    pAMISetDNSConfReq->DnsConfig.DNSIPAddr[2],pAMISetDNSConfReq->DnsConfig.DNSIPAddr[3]);
                }
                else
                {
                    memcpy(BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSIPAddr3,pAMISetDNSConfReq->DnsConfig.DNSIPAddr,ReqLen);
                }
            }
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;
        case AMI_DNS_CONF_DNS_RESTART:

            if(ReqLen != 0)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            SetPendStatus(PEND_OP_SET_ALL_DNS_CFG,PEND_STATUS_PENDING);
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            PostPendTask(PEND_OP_SET_ALL_DNS_CFG, 0, 0,curchannel & 0xF,BMCInst);
            break;

        case AMI_DNS_CONF_DNS_ENABLE:
            /* Validate the request byte */
            if(pAMISetDNSConfReq->DnsConfig.DNSEnable > 1)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            /* Update the shared memory */
            LOCK_BMC_SHARED_MEM(BMCInst);
                BMC_GET_SHARED_MEM (BMCInst)->DNSconf.DNSEnable = pAMISetDNSConfReq->DnsConfig.DNSEnable;
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            /* If we disable the DNS service, disable the registrations of BMC hostname too... */
            if(pAMISetDNSConfReq->DnsConfig.DNSEnable == 0)
            {
                LOCK_BMC_SHARED_MEM(BMCInst);
                for(i = 0; i < MAX_LAN_CHANNELS; i++)
                    BMC_GET_SHARED_MEM(BMCInst)->DNSconf.RegisterBMC[i] = 0;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
            }
            break;

        default:
            *pRes = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
            break;
    }

    pAMISetDNSConfRes->CompletionCode = CC_SUCCESS;

    return(sizeof(AMISetDNSConfRes_T));
}



/*
*@fn RestartService
*@brief This function will restart the target service according to parameter service_bit, 
*@param service_bit - refer service id field for details
*@return Returns 0 on success    
*/
/* service id field
 * Bit31-Bit5 |Bit4  | Bit3 | Bit2 | Bit1 | Bit0
 * Reserved   |TELNET| SSH  | MEDIA| KVM  | WEB
 */

static int RestartService(INT8U service_bit,int BMCInst)
{
	struct stat buf;

    if ((service_bit > 0) && (service_bit <= HDMEDIA_SERVICE_ID_BIT))
    {
	      /* Update the stunnel configuration when ports change for kvm & media services */
        UpdateStunnelEntry(ServiceNameList[service_bit]);

        if(service_bit != KVM_SERVICE_ID_BIT)
        {
            if(send_signal_by_name("Adviserd", SIGALRM) != 0) 
            {
                TDBG ("Unable to send the signal to the requested process\n");
            }
        }
    }

    switch (service_bit)
    {
        case WEB_SERVICE_ID_BIT:
            if(stat("/etc/init.d/webgo.sh",&buf) == 0)
            {
                safe_system("/etc/init.d/webgo.sh restart &");
            }
            else if(stat("/etc/init.d/lighttpd.sh",&buf) == 0)
            {
                safe_system("/etc/init.d/lighttpd.sh restart &");
            }
            break;

        case KVM_SERVICE_ID_BIT:
            safe_system("/etc/init.d/adviserd.sh restart &");
            break;

        case CDMEDIA_SERVICE_ID_BIT:
            safe_system("/etc/init.d/cdserver restart &");
	    break;

        case FDMEDIA_SERVICE_ID_BIT:
            safe_system("/etc/init.d/fdserver restart &");
	    break;

        case HDMEDIA_SERVICE_ID_BIT:
            safe_system("/etc/init.d/hdserver restart &");
            break;

        case SSH_SERVICE_ID_BIT:
            safe_system("/etc/init.d/ssh stop");
            safe_system("/etc/init.d/ssh start &");
            if(g_corefeatures.session_management)
                On_clear_orphan_service(SESSION_TYPE_SSH);
            break;

        case TELNET_SERVICE_ID_BIT:
            safe_system("/etc/init.d/telnetd stop");
            safe_system("/etc/init.d/telnetd start &");
            if(g_corefeatures.session_management)
                On_clear_orphan_service(SESSION_TYPE_TELNET);
            break;

        default :
            break;
    }
        
    return 0;
}

/*
*@fn IsSinglePortEnable
*@brief This function will check singleport feature is enabled/disabled at runtime, 
*@return Returns ENABLED on success    
*/
int IsSinglePortEnable()
{
    int retval =ENABLED;
    int (*IsSinglePortEnabled) ();
    void    *singleport= NULL;

    if(g_corefeatures.runtime_singleport_support == ENABLED)
    {
        singleport = dlopen(SINGLE_PORT_APP_HOOKS_PATH, RTLD_LAZY);

        // This will call definition to update singleport status
        if (singleport)
        {
            IsSinglePortEnabled = dlsym(singleport, IS_SINGLE_PORT_ENABLED);
            if (IsSinglePortEnabled)
            {
                retval= IsSinglePortEnabled();
            }
            else
            {
                TEMERG("%s : dlsym failed  errno:%d  . \n",IS_SINGLE_PORT_ENABLED,errno);
            }
            dlclose(singleport);
        }
        else
        {
            TEMERG("%s : dlsym failed  errno:%d .\n",SINGLE_PORT_APP_HOOKS_PATH,errno);
        }
    }
    return retval;
}

int
AMIGetServiceConf( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{

    AMIGetServiceConfReq_T*  pAMIGetServiceConfReq = (AMIGetServiceConfReq_T*) pReq;
    AMIGetServiceConfRes_T*  pAMIGetServiceConfRes = (AMIGetServiceConfRes_T*) pRes;
    SERVICE_CONF_STRUCT conf;
    INT32U  Value = 0;
    INT8U  ServiceIDBit = 0;
    int ServiceErr = 0,ret=0;
    int IsSinglePortService = g_corefeatures.single_port_app;
    INT8U active_session_cnt=0;
    INT8U tempactivecount;
    SERVICE_CONF_STRUCT webconf;
    session_info_header_t hdr;
    /* Match the service_id byte with ServiceName */

    Value = (pAMIGetServiceConfReq->ServiceID);

    ServiceErr = GetINT32UBitsNum (Value, &ServiceIDBit);
    if(ServiceErr < 0)
    {
        TCRIT("Error in getting bit number from service_id\n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    } 

    if(ServiceNameList[ServiceIDBit] == NULL)
    {
        TCRIT("ServiceName is not found in ServiceNameList\n");
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof (INT8U);
    } 
    
    if( g_corefeatures.solssh_support != ENABLED && ServiceIDBit == SOLSSH_SERVICE_ID_BIT)
    {
    	 TDBG("SOLSSH is not available\n");
    	 *pRes = CC_SERVICE_ABSENT;
    	 return sizeof (INT8U);
    }
    
    ServiceErr = get_service_configurations(ServiceNameList[ServiceIDBit], &conf);
    if(ServiceErr < 0)
    {
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof (INT8U);
    }

    if((IsSinglePortService == ENABLED) && ((ServiceIDBit > 0) && (ServiceIDBit <= HDMEDIA_SERVICE_ID_BIT)))
    {
        if(g_corefeatures.runtime_singleport_support == ENABLED)
        {
            IsSinglePortService = IsSinglePortEnable();
        }
    }
    else
    {
        IsSinglePortService =0;
    }

    memset(pAMIGetServiceConfRes, 0, sizeof(AMIGetServiceConfRes_T)); 
    pAMIGetServiceConfRes->ServiceID =                pAMIGetServiceConfReq->ServiceID;
    pAMIGetServiceConfRes->Enable    =                conf.CurrentState;
    if(IsSinglePortService == ENABLED)
    {
        //As singleport is enabled .Get webservice information and show it for KVM,CD,FD,HD Media
        ServiceErr = get_service_configurations(WEB_SERVICE_NAME, &webconf);
        if(ServiceErr < 0)
        {
            *pRes = CC_UNSPECIFIED_ERR;
            return sizeof (INT8U);
        }
        strcpy(pAMIGetServiceConfRes->InterfaceName,      webconf.InterfaceName);
    }
    else
    {
        strcpy(pAMIGetServiceConfRes->InterfaceName,      conf.InterfaceName);
    }
    pAMIGetServiceConfRes->InterfaceName [MAX_SERVICE_IFACE_NAME_SIZE] = '\0';
    pAMIGetServiceConfRes->NonSecureAccessPort =      conf.NonSecureAccessPort;              
    pAMIGetServiceConfRes->SecureAccessPort =         conf.SecureAccessPort;                        
    pAMIGetServiceConfRes->SessionInactivityTimeout = conf.SessionInactivityTimeout;           
    pAMIGetServiceConfRes->MaxAllowSession =          conf.MaxAllowSession;                  
    //pAMIGetServiceConfRes->CurrentActiveSession =     conf.CurrentActiveSession;
    pAMIGetServiceConfRes->MaxSessionInactivityTimeout=	conf.MaxSessionInactivityTimeout;
    pAMIGetServiceConfRes->MinSessionInactivityTimeout=	conf.MinSessionInactivityTimeout;
    if(g_corefeatures.session_management != ENABLED)
    {	
		if(get_active_session(ServiceIDBit,&active_session_cnt) == 0)
		{
			if(active_session_cnt == 0xFF)
			{
				pAMIGetServiceConfRes->CurrentActiveSession = active_session_cnt;
			}
			else
			{
				getNotEditableMaskData((unsigned char *)&(active_session_cnt),sizeof(active_session_cnt),(unsigned char *)&(tempactivecount));
				pAMIGetServiceConfRes->CurrentActiveSession = tempactivecount;
			}
		}
		else
		{
			TCRIT("IPMI-NCML:Unable to get the active session count");
		}
    } else {
                if((ret=racsessinfo_getsessionhdr(&hdr)!=0))
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof (INT8U);
                }
                switch (ServiceIDBit) {
                case WEB_SERVICE_ID_BIT:
                                 active_session_cnt=hdr.sess_type_logins[SESSION_TYPE_WEB_HTTP];
                                 active_session_cnt+=hdr.sess_type_logins[SESSION_TYPE_WEB_HTTPS];
                        break;
                case KVM_SERVICE_ID_BIT:
                                 active_session_cnt=hdr.sess_type_logins[SESSION_TYPE_VKVM];
                        break;
                case CDMEDIA_SERVICE_ID_BIT:
                                 active_session_cnt=hdr.sess_type_logins[SESSION_TYPE_VMEDIA_CD];
                                 active_session_cnt+=hdr.sess_type_logins[SESSION_TYPE_VMEDIA_LOCAL_CD];
                        break;
                case FDMEDIA_SERVICE_ID_BIT:
                                 active_session_cnt=hdr.sess_type_logins[SESSION_TYPE_VMEDIA_FD];
                                 active_session_cnt+=hdr.sess_type_logins[SESSION_TYPE_VMEDIA_LOCAL_FD];
                        break;
                case HDMEDIA_SERVICE_ID_BIT:
                                 active_session_cnt=hdr.sess_type_logins[SESSION_TYPE_VMEDIA_HD];
                                 active_session_cnt+=hdr.sess_type_logins[SESSION_TYPE_VMEDIA_LOCAL_HD];
                        break;
                case SSH_SERVICE_ID_BIT:
                                 active_session_cnt=hdr.sess_type_logins[SESSION_TYPE_SSH];
                        break;
                case TELNET_SERVICE_ID_BIT:
                                 active_session_cnt=hdr.sess_type_logins[SESSION_TYPE_TELNET];
                        break;
                case SOLSSH_SERVICE_ID_BIT:
                	             active_session_cnt=hdr.sess_type_logins[SESSION_TYPE_SOLSSH];
                	    break;
                }
                getNotEditableMaskData((unsigned char *)&(active_session_cnt),sizeof(active_session_cnt),(unsigned char *)&(tempactivecount));
    	        pAMIGetServiceConfRes->CurrentActiveSession = tempactivecount;
    }
    TDBG( "AMIGETSERVICE: ServiceID=%lu, Enable=%d, Name=%s,\n"
            "port=%d, port=%d, timeout=%d, maxsession=%d, activesession=%d\n, "
        	"MaxSessionInactivityTimeout=%d, MinSessionInactivityTimeout=%d",
                pAMIGetServiceConfRes->ServiceID,
                pAMIGetServiceConfRes->Enable,
                pAMIGetServiceConfRes->InterfaceName,
                pAMIGetServiceConfRes->NonSecureAccessPort,               
                pAMIGetServiceConfRes->SecureAccessPort,                                     
                pAMIGetServiceConfRes->SessionInactivityTimeout,            
                pAMIGetServiceConfRes->MaxAllowSession,                  
                pAMIGetServiceConfRes->CurrentActiveSession,
            pAMIGetServiceConfRes->MaxSessionInactivityTimeout,
            pAMIGetServiceConfRes->MinSessionInactivityTimeout);


    pAMIGetServiceConfRes->CompletionCode = CC_SUCCESS;

    return(sizeof(AMIGetServiceConfRes_T));
}

#define IVTP_STOP_SESSION_IMMEDIATE (0x0008)
#define STOP_SESSION_WEBSERVER_RESTART (0x0B)
int IsKVMSessionRunning()
{
    INT8U  activecount = 0;
    session_info_header_t hdr;
    if(g_corefeatures.session_management != ENABLED)
    {
        if(get_active_session(KVM_SERVICE_ID_BIT,&activecount) != 0)
        {
            TCRIT("Not able to get the active count for KVM");
        }
    }
    else
    {
        if(racsessinfo_getsessionhdr(&hdr) == 0)
        {
            activecount=hdr.sess_type_logins[SESSION_TYPE_VKVM];
        }
    }
    if(activecount)
    {
        TINFO("KVM Running activecount:%d\n",activecount);
        return 1;
    }
    return 0;
}


int
AMISetServiceConf( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
    AMISetServiceConfReq_T*  pAMISetServiceConfReq = (AMISetServiceConfReq_T*) pReq;
    AMISetServiceConfRes_T*  pAMISetServiceConfRes = (AMISetServiceConfRes_T*) pRes;

    SERVICE_CONF_STRUCT conf,ReqConf;
    INT32U  Value = 0;
    INT8U   ServiceIDBit = 0;
    int     ServiceErr = 0,i,activeslaves;
    int	    RebootService = 0;
    int IsSinglePortService = g_corefeatures.single_port_app;
    SERVICE_CONF_STRUCT webconf;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    BondIface   bond;
    RestartService_T Service;
    INT8U curchannel;

    if ( '\0' != pAMISetServiceConfReq->InterfaceName[MAX_SERVICE_IFACE_NAME_SIZE])
    {
        TCRIT("Error : The last byte must be null in Interface name \n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    /* Match the service_id byte with ServiceName */         
    Value = (pAMISetServiceConfReq->ServiceID);
   
    ServiceErr = GetINT32UBitsNum (Value, &ServiceIDBit);
    if(ServiceErr < 0)
    {
        TCRIT("Error in Getting Bit Number from Service ID\n");
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof (INT8U);
    } 

    if(ServiceNameList[ServiceIDBit] == NULL)
    {
        TCRIT("Service Name Not Available for the Requested Service ID\n");
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof (INT8U);
    }
    if( (pAMISetServiceConfReq->Enable & 0xFE )  != 0x00 )
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }
    memset(&ReqConf,0,sizeof(SERVICE_CONF_STRUCT));

    memcpy(&ReqConf.ServiceName[0],ServiceNameList[ServiceIDBit],sizeof(ReqConf.ServiceName));
    memcpy(&ReqConf.InterfaceName[0],&pAMISetServiceConfReq->InterfaceName[0],sizeof(pAMISetServiceConfReq->InterfaceName));
    ReqConf.CurrentState = pAMISetServiceConfReq->Enable;
    ReqConf.MaxAllowSession = pAMISetServiceConfReq->MaxAllowSession;
    ReqConf.SessionInactivityTimeout = pAMISetServiceConfReq->SessionInactivityTimeout;
    ReqConf.SecureAccessPort = pAMISetServiceConfReq->SecureAccessPort;
    ReqConf.NonSecureAccessPort = pAMISetServiceConfReq->NonSecureAccessPort;
    ReqConf.CurrentActiveSession = pAMISetServiceConfReq->CurrentActiveSession;

    if((IsSinglePortService == ENABLED) && ((ServiceIDBit > 0) && (ServiceIDBit <= HDMEDIA_SERVICE_ID_BIT)))
    {
        if(g_corefeatures.runtime_singleport_support == ENABLED)
        {
            IsSinglePortService = IsSinglePortEnable();
        }
    }
    else
    {
        IsSinglePortService =0;
    }

    if(g_corefeatures.bond_support == ENABLED)
    {
         for(i = 0; i < sizeof(Ifcnametable)/sizeof(IfcName_T); i++)
         {
            if(strcmp(Ifcnametable[i].Ifcname,ReqConf.InterfaceName) == 0 )
            {
                if(CheckBondSlave(Ifcnametable[i].Index) == 1)
                {
                    *pRes = NCML_ERR_INVALID_INTERFACE_NAME;
                    return sizeof(INT8U);
                }
                else
                {
                    break;
                }
            }
         }
         if((IsBondingActive(BMCInst)==1)&&(strncmp(ReqConf.InterfaceName,"both",sizeof(ReqConf.InterfaceName))==0))
         {
             if (nwGetBondConf( &bond, pBMCInfo->BondConfig.BondIndex) != 0)
             {
                 *pRes = CC_UNSPECIFIED_ERR;
                 return sizeof(INT8U);
             }
             for(i=0,activeslaves=0;i<g_coremacros.global_nic_count;i++)
             {
                 if(bond.Slaves & (1<<i))
                 activeslaves++;
             }
             if(activeslaves==g_coremacros.global_nic_count)
             {
                 *pRes = NCML_ERR_INVALID_INTERFACE_NAME;
                 return sizeof(INT8U);
             }
         }
    }

    ServiceErr=Validate_SetServiceConfiguration(&ReqConf);
    if(ServiceErr !=CC_NORMAL)
    {
        *pRes=ServiceErr;
        TCRIT("Validate_SetServiceConfiguration Failed %d\n",ServiceErr);
        return sizeof(INT8U);
    }
    if(validate_active_session_count(ServiceIDBit,ReqConf.CurrentActiveSession,g_corefeatures.session_management)!=0)
    {
    	*pRes=NCML_ERR_NCMLCONFIG_NE;
        TCRIT("Setting Active session Not Applicable\n");
        return sizeof(INT8U);
    }
    ServiceErr = get_service_configurations(ReqConf.ServiceName, &conf);
    if (ServiceErr < 0)
    {
        TCRIT("Error in Getting the Configuration for the Requested Service\n");
	*pRes = CC_UNSPECIFIED_ERR;
	return sizeof (INT8U);
    }

    if(IsSinglePortService == ENABLED) 
    {
        ServiceErr = get_service_configurations(WEB_SERVICE_NAME, &webconf);
        if(ServiceErr < 0)
        {
            *pRes = CC_UNSPECIFIED_ERR;
            return sizeof (INT8U);
        }

        //Incoming interface name should be same as webservice interface name.
        if(memcmp(webconf.InterfaceName, ReqConf.InterfaceName, strlen(webconf.InterfaceName)) != 0)
        {
            TCRIT("Single Port service Interface not editable.\n");
            *pRes = NCML_ERR_SINGLEPORT_SERVICE_INTERFACE_NE;
            return sizeof (INT8U);
        }

        //Copy the serivce original interface name in Reqconf to avoid service restart
        memset(ReqConf.InterfaceName ,0,sizeof(ReqConf.InterfaceName));
        strcpy(ReqConf.InterfaceName,conf.InterfaceName);
        ReqConf.InterfaceName [MAX_SERVICE_IFACE_NAME_SIZE] = '\0';
    }

    if ((conf.CurrentState != ReqConf.CurrentState) ||
    	(memcmp(conf.InterfaceName, ReqConf.InterfaceName, strlen(conf.InterfaceName)) != 0) ||
   	(conf.NonSecureAccessPort != ReqConf.NonSecureAccessPort) ||
    	(conf.SecureAccessPort != ReqConf.SecureAccessPort) ||                        
    	(conf.SessionInactivityTimeout != ReqConf.SessionInactivityTimeout) ||           
    	(conf.MaxAllowSession != ReqConf.MaxAllowSession))
    {
        RebootService = 1;
    }

    memset(&conf, 0, sizeof(SERVICE_CONF_STRUCT));
    
    memcpy(&conf.ServiceName[0],&ReqConf.ServiceName[0],sizeof(ReqConf.ServiceName));
    conf.CurrentState =             ReqConf.CurrentState;
    strcpy(conf.InterfaceName,      ReqConf.InterfaceName);
    conf.NonSecureAccessPort =      ReqConf.NonSecureAccessPort;              
    conf.SecureAccessPort  =        ReqConf.SecureAccessPort;                        
    conf.SessionInactivityTimeout = ReqConf.SessionInactivityTimeout;           
    conf.MaxAllowSession =          ReqConf.MaxAllowSession;                  
    conf.CurrentActiveSession =     ReqConf.CurrentActiveSession;

    ServiceErr = set_service_configurations(ServiceNameList[ServiceIDBit], &conf,g_corefeatures.timeoutd_sess_timeout);
    if(ServiceErr < 0)
    {
        TCRIT("Error in Setting the Configuration for the Requested Service\n");
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof (INT8U);
    }

    if (RebootService) {
        //Close JViewer session if websession restart is going to happen.
        if((ServiceIDBit == WEB_SERVICE_ID_BIT) && IsKVMSessionRunning())
        {
            if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE] != NULL)
            {
                if( 0 > ((int(*)(int , int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE])(IVTP_STOP_SESSION_IMMEDIATE, STOP_SESSION_WEBSERVER_RESTART) )
                    {
                        IPMI_ERROR("Error in Writing data to Adviser\n");
                    }
            }
            Service.ServiceName = WEBSERVER;
            Service.SleepSeconds = 5;// Sleep for 5 Seconds
            SetPendStatus(PEND_OP_RESTART_SERVICES, PEND_STATUS_PENDING);
            OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            PostPendTask(PEND_OP_RESTART_SERVICES, (INT8U *) &Service, sizeof(RestartService_T),curchannel & 0xF,BMCInst);
        }
        else
        {
            RestartService(ServiceIDBit,BMCInst);
        }
        RebootService = 0;
    }

    pAMISetServiceConfRes->CompletionCode = CC_SUCCESS;

    return(sizeof(AMISetServiceConfRes_T));
}

int AMILinkDownResilent( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
    AMILinkDownResilentReq_T*  pAMILinkDownResilentReq = (AMILinkDownResilentReq_T*) pReq;
    AMILinkDownResilentRes_T* pAMILinkDownResilentRes = (AMILinkDownResilentRes_T *) pRes;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    pAMILinkDownResilentRes->CompletionCode = CC_SUCCESS;

    if(pAMILinkDownResilentReq->LinkDownEnable== 1)
    {
        pBMCInfo->IpmiConfig.LinkDownResilentSupport = 1;
        pAMILinkDownResilentRes->LinkEnableStatus = pBMCInfo->IpmiConfig.LinkDownResilentSupport;
    }
    else if(pAMILinkDownResilentReq->LinkDownEnable == 0)
    {
        pBMCInfo->IpmiConfig.LinkDownResilentSupport = 0;
        pAMILinkDownResilentRes->LinkEnableStatus =  pBMCInfo->IpmiConfig.LinkDownResilentSupport;
    }
    else if(pAMILinkDownResilentReq->LinkDownEnable == 0xFF)
    {
        pAMILinkDownResilentRes->LinkEnableStatus =  pBMCInfo->IpmiConfig.LinkDownResilentSupport;
    }
    else
    {
        *pRes = CC_INV_DATA_FIELD;
    }
    
    return sizeof(AMILinkDownResilentRes_T);
}

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
int ValidateSetIfaceStateBond (INT8U *pReq, INT8U *pRes, INT8U *VLANID, int CheckBondDisable, int BMCInst)
{
    AMISetIfaceStateReq_T *pAMIIfaceStateReq = (AMISetIfaceStateReq_T *)pReq;
    AMISetIfaceStateRes_T *pAMIIfaceStateRes = (AMISetIfaceStateRes_T *)pRes;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    int Ifccount = 0, i;
    char IfcName[16];

    if(g_corefeatures.bond_support != ENABLED)
    {
        *pRes = CC_PARAM_NOT_SUPPORTED;
        return sizeof(INT8U);
    }
    if((pAMIIfaceStateReq->ConfigData.BondIface.AutoConf & ENABLE_DISABLE_MASK)!= 0)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }
    if( pAMIIfaceStateReq->ConfigData.BondIface.Slaves == 0)
    {
        *pRes = CC_INSUFFICIENT_SLAVE_COUNT;
        return sizeof(INT8U);
    }
    
    /*Get the no of LAN interface count*/
    if(get_network_interface_count(&Ifccount) < 0)
    {
        pAMIIfaceStateRes->CompletionCode = CC_UNSPECIFIED_ERR;
        return sizeof(INT8U);
    }

    if(Ifccount <= 1)
    {
        *pRes = OEMCC_INSUFFIENT_LANIFC_COUNT;
        return sizeof(INT8U);
    }

    if(pAMIIfaceStateReq->ConfigData.BondIface.BondIndex != 0x0)
    {
        IPMI_WARNING("\nInvalid BondIndex number :%x", pAMIIfaceStateReq->ConfigData.BondIface.BondIndex);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    if (CheckBondDisable == 1)
    {
        if(g_corefeatures.delayed_lan_restart_support)
        {
            if((!pBMCInfo->BondConfig.Enable) && (pAMIIfaceStateReq->ConfigData.BondIface.Enable == IFACE_DISABLED))
            {
                *pRes = OEMCC_BOND_ALREADY_DISABLED;
                return sizeof(INT8U);
            }
        }
        else
        {
            GetNoofInterface();
    
            memset(IfcName,0,sizeof(IfcName));
            sprintf(IfcName,"bond%d",pAMIIfaceStateReq->ConfigData.BondIface.BondIndex);
            for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
            {
                if(Ifcnametable[i].Ifcname[0] == 0)
                    continue;
        
                if((strcmp(IfcName,Ifcnametable[i].Ifcname) == 0 ))
                {
                    if((Ifcnametable[i].Enabled == IFACE_DISABLED ) && (pAMIIfaceStateReq->ConfigData.BondIface.Enable == IFACE_DISABLED))
                    {
                        *pRes = OEMCC_BOND_ALREADY_DISABLED;
                        return sizeof(INT8U);
                    }
                    break;
                }
            }
    
            if(i == sizeof(Ifcnametable)/sizeof(IfcName_T))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }
        }
    }

    TDBG("Enable:%d\nBondIndex:%d\nBondMode:%d\nmiiinterval:%d\nSlave:%d\nAutoConf:%d\n",
                                                    pAMIIfaceStateReq->ConfigData.BondIface.Enable,
                                                    pAMIIfaceStateReq->ConfigData.BondIface.BondIndex,
                                                    pAMIIfaceStateReq->ConfigData.BondIface.BondMode,
                                                    pAMIIfaceStateReq->ConfigData.BondIface.MiiInterval,
                                                    pAMIIfaceStateReq->ConfigData.BondIface.Slaves,
                                                    pAMIIfaceStateReq->ConfigData.BondIface.AutoConf);
    /*Validate the bonding mode*/
    if( pAMIIfaceStateReq->ConfigData.BondIface.BondMode > MAX_BOND_MODE)
    {
        IPMI_WARNING("Bond Mode is exceeded\n");
        *pRes = CC_INV_DATA_FIELD;
        return (sizeof(INT8U));
    }

    /*Validate the supported bonding modes*/
    if( pAMIIfaceStateReq->ConfigData.BondIface.BondMode == MAX_BOND_MODE )
    {
        IPMI_WARNING("Bond Mode %d is not supported\n",pAMIIfaceStateReq->ConfigData.BondIface.BondMode);
        *pRes = OEMCC_UNSUPPORTED_BOND_MODE;
        return (sizeof(INT8U));
    }

    /*Validate the miiinterval*/
    if(pAMIIfaceStateReq->ConfigData.BondIface.MiiInterval < DEFAULT_MII_INTERVAL)
    {
        IPMI_WARNING("Mii interval should be greater than 100\n");
        *pRes = CC_INV_DATA_FIELD;
        return (sizeof(INT8U));
    }

    *pRes = CheckSlaveVLANInterface(pAMIIfaceStateReq->ConfigData.BondIface.Slaves, VLANID, BMCInst);
    if ((*pRes) != 0)
    {
        return (sizeof(INT8U));
    }
    return 0;
    
}


/**
 * @fn AMISetIfaceState
 * @brief This function sets the status of interfaces
 * @param Request message and BMC instance
 * @return success completion code 
 */
int AMISetIfaceState( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
    AMISetIfaceStateReq_T*  pAMIIfaceStateReq = (AMISetIfaceStateReq_T *) pReq;
    AMISetIfaceStateRes_T*  pAMIIfaceStateRes = (AMISetIfaceStateRes_T *) pRes;
    NWCFG_STRUCT cfg;
    NWCFG6_STRUCT cfg6;
    int retValue = 0;
    char IfcName[16];
    int EthIndex = 0,i,ethcount=0,Ethindex = 0;
    INT8U CurrentIfaceState = 0x00, VLANID = 0,curchannel;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    BondIface   bond;
    int Ifccount = 0;
    memset(&bond,0,sizeof(BondIface));

    if ( ReqLen >= 1 )
    {
        ReqLen -= 1;
    }
    else
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof (INT8U);
    }
    if(IsBMCNFSMode() != 0)
    {
         IPMI_WARNING("Card in NFS mode, Not safe to set any of interface configurations...\n");
         *pRes = CC_CMD_UNSUPPORTED_UNCONFIGURABLE;
         return sizeof(INT8U);
    }

    if( 0x02 >pAMIIfaceStateReq->Params)
    {
        if(ReqLen != NwIfcStateConfigParamLenght[pAMIIfaceStateReq->Params])
        {
            TCRIT("params:%d reqLen:%ld\n",pAMIIfaceStateReq->Params,ReqLen);
            *pRes = CC_REQ_INV_LEN;
            return sizeof(INT8U);
        }
    }
    
    if (g_PDKHandle[PDK_BEFORESETIFACESTATE] != NULL )
    {
        retValue = ((int(*)(INT8U *, INT8U, INT8U *,int))
                (g_PDKHandle[PDK_BEFORESETIFACESTATE]))(pReq, ReqLen, pRes, BMCInst);
        if(retValue != 0)
        {
              return retValue;
        }
    }

    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    switch(pAMIIfaceStateReq->Params)
    {
        case AMI_IFACE_STATE_ETH:

            if(pAMIIfaceStateReq->ConfigData.EthIface.EthIndex == 0xff)
            {
                IPMI_WARNING("\nInvalid EthIndex number :%x", pAMIIfaceStateReq->ConfigData.EthIface.EthIndex);
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }

            /*Ethindex to be enabled*/
            EthIndex = pAMIIfaceStateReq->ConfigData.EthIface.EthIndex;
            Ethindex = pAMIIfaceStateReq->ConfigData.EthIface.EthIndex; 
            if(pBMCInfo->BondConfig.Enable == 1)
            {
                snprintf(IfcName,sizeof(IfcName),"bond%d",pBMCInfo->BondConfig.BondIndex);
                if(strcmp(IfcName, pBMCInfo->LanIfcConfig[EthIndex].ifname) == 0)
                {
                    EthIndex = pBMCInfo->LanIfcConfig[EthIndex].Ethindex;
                }
            }

            if(g_corefeatures.ipv6_compliance_support == ENABLED)
            {
                /*If the ipmi ipv6 compliance support is enabled, this command is used to enable/disable the interface alone. 
                                IPv4 /IPv6 enable should be done via generic IPMI command
                                0x00 - Disable the interface
                                0x01 - Enable the interface*/
                if(pAMIIfaceStateReq->ConfigData.EthIface.EnableState > 1)
                {
                    IPMI_WARNING("\nInvalid EnableState :%x", pAMIIfaceStateReq->ConfigData.EthIface.EnableState);
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }

                if(pAMIIfaceStateReq->ConfigData.EthIface.EnableState == 1)
                {
                    /*Enable the IPv4/IPv6 based on generic LAN IPMI Command*/
                    if(pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable == 0x00)
                    {
                        pAMIIfaceStateReq->ConfigData.EthIface.EnableState = ENABLE_V4;
                    }
                    else if(pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable == 0x01)
                    {
                        pAMIIfaceStateReq->ConfigData.EthIface.EnableState = ENABLE_V6;
                    }
                    else if(pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable == 0x02)
                    {
                        pAMIIfaceStateReq->ConfigData.EthIface.EnableState = ENABLE_V4_V6;
                    }
                }
            }
            else
            {
                //Checking for valid EnableState
                if(  (pAMIIfaceStateReq->ConfigData.EthIface.EnableState != DISABLE_V4_V6)
                     && (pAMIIfaceStateReq->ConfigData.EthIface.EnableState != ENABLE_V4)
                     && (pAMIIfaceStateReq->ConfigData.EthIface.EnableState != ENABLE_V6)
                     && (pAMIIfaceStateReq->ConfigData.EthIface.EnableState != ENABLE_V4_V6)
                  )
                {
                    IPMI_WARNING("\nInvalid EnableState :%x", pAMIIfaceStateReq->ConfigData.EthIface.EnableState);
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (INT8U);
                }

                if(ENABLE_V6 == pAMIIfaceStateReq->ConfigData.EthIface.EnableState)
                {
                    if(g_corefeatures.global_ipv6  == ENABLED)
                    {
                        TDBG("IPv6 only enabled");
                    }
                    else
                    {
                       TCRIT("IPMI support for IPv6 is Disabled...\n");
                       *pRes = OEMCC_INV_IP4_NOT_ENABLED;
                       return sizeof (INT8U);
                    }
                }
            }

            /*Compare the bonding and eth interfaces */
            if(GetIfcNameByIndex(EthIndex, IfcName) != 0)
            {
                IPMI_WARNING("Invalid EthIndex number %d\n",EthIndex);
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }
            
            /*Update the interface status*/
            GetNoofInterface();
            for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
            {
                if(Ifcnametable[i].Ifcname[0] == 0)
                    continue;
            
                if(Ifcnametable[i].Enabled == 1)
                    ethcount++;
            }

            memset(&cfg, 0, sizeof(NWCFG_STRUCT));
            memset(&cfg6, 0, sizeof(NWCFG6_STRUCT));
            
            retValue = nwReadNWCfg_v4_v6(&cfg, &cfg6, Ethindex,g_corefeatures.global_ipv6);
            if(retValue != 0)
                TCRIT("Error in reading network configuration.\n");     
            
            CurrentIfaceState = ((cfg6.enable<<1) | cfg.enable);
            
            if(CurrentIfaceState == pAMIIfaceStateReq->ConfigData.EthIface.EnableState)
            {
                TCRIT("Iface state is the same, do nothing\n");         
                *pRes = CC_SUCCESS;
                return sizeof(AMISetIfaceStateRes_T); 
            }

            pBMCInfo->LANCfs[EthIndex].IPv6_Enable = (pAMIIfaceStateReq->ConfigData.EthIface.EnableState & ENABLE_V6) ? 1 : 0;
            pBMCInfo->LANCfs[EthIndex].IPv4_Enable= (pAMIIfaceStateReq->ConfigData.EthIface.EnableState & ENABLE_V4) ? 1 : 0;

            SetPendStatus(PEND_OP_SET_ETH_IFACE_STATE,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_ETH_IFACE_STATE, (INT8U *)&pAMIIfaceStateReq->ConfigData.EthIface, sizeof(EthIfaceState),curchannel & 0xF,BMCInst);
            
            FlushIPMI((INT8U*)&pBMCInfo->LANCfs[0], (INT8U*)&pBMCInfo->LANCfs[EthIndex], 
                    pBMCInfo->IPMIConfLoc.LANCfsAddr, sizeof(LANConfig_T),BMCInst);
            break;
        case AMI_IFACE_BOND_ENABLED:
            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (*pRes);

            break;
        case AMI_GET_IFACE_COUNT:
            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (*pRes);

            break;
        case AMI_GET_IFACE_CHANNEL:
            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (*pRes);

            break;
        case AMI_GET_IFACE_NAME:
            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (*pRes);

            break;
        case AMI_IFACE_STATE_BOND:
            
            retValue = ValidateSetIfaceStateBond (pReq, pRes, &VLANID, 1, BMCInst);
            if ((retValue != 0) && ((pAMIIfaceStateReq->ConfigData.BondIface.Enable == 1) || (retValue != OEMCC_VLAN_ENABLED_ON_SLAVE)))
            {
                return retValue;
            }
            if (pAMIIfaceStateReq->ConfigData.BondIface.Enable == 1)
            {
                memset(&cfg, 0, sizeof(NWCFG_STRUCT));
                memset(&cfg6, 0, sizeof(NWCFG6_STRUCT));
                memset(IfcName,0,sizeof(IfcName));

                sprintf(IfcName,"bond%d",pAMIIfaceStateReq->ConfigData.BondIface.BondIndex);
                for(i=0;i<MAX_LAN_CHANNELS;i++)
                {
                    if(strcmp(pBMCInfo->LanIfcConfig[i].ifname,IfcName) == 0)
                    {
                        EthIndex=pBMCInfo->LanIfcConfig[i].Ethindex;
                        break;
                    }
                }
                if(g_corefeatures.delayed_lan_restart_support)
                {
                    if((pBMCInfo->LANCfs[EthIndex].IPv4_Enable == 0) && (pBMCInfo->LANCfs[EthIndex].IPv6_Enable == 0))
                    {
                        *pRes = OEMCC_ETH_IFACE_DISABLED;
                        return sizeof(INT8U);
                    }
                }
                else
                {
                    retValue = nwReadNWCfg_v4_v6(&cfg, &cfg6,EthIndex,g_corefeatures.global_ipv6);
                    if(retValue != 0)
                        TCRIT("Error in reading network configuration.\n");

                    if ((cfg.enable == 0) && (cfg6.enable == 0))
                    {
                        *pRes = OEMCC_ETH_IFACE_DISABLED;
                        return sizeof(INT8U);
                    }
                }
            }
            _fmemcpy (&pBMCInfo->BondConfig,
                                  &(pAMIIfaceStateReq->ConfigData.BondIface), sizeof(BondIface));
            SetPendStatus(PEND_OP_SET_BOND_IFACE_STATE,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_BOND_IFACE_STATE, (INT8U *)&pAMIIfaceStateReq->ConfigData.BondIface, sizeof(BondIface),curchannel & 0xF,BMCInst);
            
            if (VLANID != 0)
            {
                cfg.VLANID = VLANID;
                SetPendStatus(PEND_OP_SET_VLAN_ID, PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_VLAN_ID, (INT8U*)&cfg, sizeof(cfg), (curchannel & 0x0F), BMCInst );
            }
            FlushIPMI((INT8U*)&pBMCInfo->BondConfig, (INT8U*)&pBMCInfo->BondConfig, 
                    pBMCInfo->IPMIConfLoc.BONDConfigAddr, sizeof(BondIface),BMCInst);
            break;
        case AMI_BOND_ACTIVE_SLAVE:
            if(g_corefeatures.bond_support != ENABLED)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }
            memset(IfcName,0,sizeof(IfcName));
            sprintf(IfcName,"bond%d",pAMIIfaceStateReq->ConfigData.ActiveSlave.BondIndex);
            for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
            {
                if((strcmp(IfcName,Ifcnametable[i].Ifcname) == 0 ) )
                {
                    break;
                }
            }

            /*Bond index is Invalid*/
            if(i == sizeof(Ifcnametable)/sizeof(IfcName_T))
            {
                *pRes = OEMCC_BOND_NOT_ENABLED;
                return sizeof(INT8U);
            }

            if(pAMIIfaceStateReq->ConfigData.ActiveSlave.ActiveIndex == 0x0)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            /*Index value 0xff will enable both eth0 and eth1 interface*/
            if(pAMIIfaceStateReq->ConfigData.ActiveSlave.ActiveIndex != 0xff )
            {
                for(i=0;i<BOND_MAX_SLAVE;i++)
                {
                    if((pAMIIfaceStateReq->ConfigData.ActiveSlave.ActiveIndex >> i) & IFACE_ENABLED)
                    {
                        Ifccount++;
                        /*Check Slave Entry presence*/
                        if(CheckIfcEntry(i,ETH_IFACE_TYPE) != 0)
                        {
                            *pRes = CC_INV_DATA_FIELD;
                            return sizeof(INT8U);
                        }
                    }
                }
            }

            if (nwGetBondConf( &bond, pAMIIfaceStateReq->ConfigData.ActiveSlave.BondIndex) != 0)
            {
                TDBG("Error in getting Bonding Configurations\n");
            }

            if((bond.BondMode == BOND_ACTIVE_BACKUP) && ((pAMIIfaceStateReq->ConfigData.ActiveSlave.ActiveIndex == 0xff) || (Ifccount > 1)))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            if(bond.BondMode == BOND_ACTIVE_BACKUP)
            {
                for(i=0;i<BOND_MAX_SLAVE;i++)
                {
                    if((pAMIIfaceStateReq->ConfigData.ActiveSlave.ActiveIndex >> i) & IFACE_ENABLED)
                    {
                        if(CheckIfcLinkStatus(i) != 1)
                        {
                            *pRes = OEMCC_ACTIVE_SLAVE_LINK_DOWN;
                            return sizeof(INT8U);
                        }
                        break;
                    }
                }
            }

            SetPendStatus(PEND_OP_SET_ACTIVE_SLAVE,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_ACTIVE_SLAVE, (INT8U *)&pAMIIfaceStateReq->ConfigData.ActiveSlave, sizeof(ActiveSlave_T),curchannel & 0xF,BMCInst);
            break;
        case AMI_BOND_VLAN_ENABLED:
            if(g_corefeatures.bond_support == ENABLED)
            {
                *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            }
            else
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
            }
            return sizeof (*pRes);
        case AMI_CURR_ACTIVE_IFACE_COUNT:
            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (INT8U);
        default:
            pAMIIfaceStateRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);

    }
    pAMIIfaceStateRes->CompletionCode = CC_SUCCESS;
    return sizeof(AMISetIfaceStateRes_T);
}

/**
 * @fn AMIGetIfaceState
 * @brief This function returns the status of interfaces
 * @param Request message and BMC instance
 * @return status of interfaces 
 */
int AMIGetIfaceState( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{

    AMIGetIfaceStateReq_T*  pAMIIfaceStateReq = (AMIGetIfaceStateReq_T *) pReq;
    AMIGetIfaceStateRes_T*  pAMIIfaceStateRes = (AMIGetIfaceStateRes_T *) pRes;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    NWCFG_STRUCT cfg;
    NWCFG6_STRUCT cfg6;
    BondIface   bond;
    char tmpstr[16];

    char IfcName[16],EthIfcname[MAX_ETHIFC_LEN * MAX_LAN_CHANNELS];
    int retValue = 0,i,Ifccount = 0,j,count=0;
    int EthIndex = 0,NIC_Count = 0,netindex = 0;
    INT8U CurrentIfaceState = 0x00;

    if(0 != pAMIIfaceStateReq->BlockSelect)
    {
        IPMI_WARNING("Invalid BlockSelect :%x\n", pAMIIfaceStateReq->BlockSelect);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }
    if (g_PDKHandle[PDK_BEFOREGETIFACESTATE] != NULL )
    {
        retValue = ((int(*)(INT8U *, INT8U, INT8U *,int))
                (g_PDKHandle[PDK_BEFOREGETIFACESTATE]))(pReq, ReqLen, pRes, BMCInst);
        if(retValue != 0)
        {
              return retValue;
        }
    }

    GetNoofInterface();
    switch(pAMIIfaceStateReq->Params)
    {
        case AMI_IFACE_STATE_ETH:
            if(pAMIIfaceStateReq->SetSelect== 0xff)
            {
                IPMI_WARNING("\nInvalid EthIndex number :%x", pAMIIfaceStateReq->SetSelect);
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            EthIndex = pAMIIfaceStateReq->SetSelect;

            if(GetIfcNameByIndex(EthIndex, IfcName) != 0)
            {
                IPMI_WARNING("Invalid EthIndex number %d\n",EthIndex);
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

/*            sprintf(tmpstr,"bond0");
            if(strcmp(tmpstr,IfcName) == 0)
            {
                IPMI_WARNING("Given EthIndex %d  is specific to bonding interface. Use specific parameter for bonding\n",EthIndex);
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }*/

            memset(&cfg, 0, sizeof(NWCFG_STRUCT));
            memset(&cfg6, 0, sizeof(NWCFG6_STRUCT));
               
            if((g_corefeatures.delayed_lan_restart_support))
            {
                netindex = EthIndex;
                if(pBMCInfo->BondConfig.Enable == 1)
                {
                    sprintf(tmpstr,"bond0");
                    for(i=0; i < MAX_LAN_CHANNELS; i++)
                    {
                        if(strcmp(tmpstr, pBMCInfo->LanIfcConfig[i].ifname) == 0)
                        {
                            netindex = pBMCInfo->LanIfcConfig[i].Ethindex;
                        }
                    }
                }
                cfg6.enable = pBMCInfo->LANCfs[netindex].IPv6_Enable;
                cfg.enable = pBMCInfo->LANCfs[netindex].IPv4_Enable;
            }
            else
            {
                retValue = nwReadNWCfg_v4_v6(&cfg, &cfg6, EthIndex,g_corefeatures.global_ipv6);
                if(retValue != 0)
                    TCRIT("Error in reading network configuration.\n"); 
            }
            pAMIIfaceStateRes->ConfigData.EthIface.EthIndex = EthIndex;
            if(g_corefeatures.ipv6_compliance_support == ENABLED)
            {
                /*The response should be either 0 or 1*/
                if((cfg6.enable == 1) || (cfg.enable == 1))
                {
                    /*Interface state is enabled either IPv4 or IPv6 is enabled*/
                    CurrentIfaceState = 1;
                }
                else
                {
                    CurrentIfaceState = 0;
                }
            }
            else
            {
                CurrentIfaceState = ((cfg6.enable<<1) | cfg.enable);
            }
            pAMIIfaceStateRes->ConfigData.EthIface.EnableState = CurrentIfaceState;
            pAMIIfaceStateRes->CompletionCode = CC_NORMAL;
            return sizeof(INT8U) + sizeof(EthIfaceState);
            break;
        case AMI_IFACE_STATE_BOND:
            if(g_corefeatures.bond_support == ENABLED)
            {
                if(pAMIIfaceStateReq->SetSelect== 0xff)
                {
                    IPMI_WARNING("\nInvalid EthIndex number :%x", pAMIIfaceStateReq->SetSelect);
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (INT8U);
                }
                EthIndex = pAMIIfaceStateReq->SetSelect;
    
                memset(&bond,0,sizeof(BondIface));
    
                if(g_corefeatures.delayed_lan_restart_support)
                {
                   if(EthIndex == pBMCInfo->BondConfig.BondIndex)
                   {
                        memcpy(&bond, &pBMCInfo->BondConfig, sizeof(BondIface)); 
                   }
                }
                else
                {
                    if (nwGetBondConf( &bond, EthIndex) != 0)
                    {
                        TDBG("Error in getting Bonding Configurations\n");
                    }
                }
                pAMIIfaceStateRes->CompletionCode = CC_NORMAL;
                pAMIIfaceStateRes->ConfigData.BondIface.Enable = bond.Enable;
                pAMIIfaceStateRes->ConfigData.BondIface.BondIndex = bond.BondIndex;
                pAMIIfaceStateRes->ConfigData.BondIface.BondMode = bond.BondMode;
                pAMIIfaceStateRes->ConfigData.BondIface.MiiInterval = bond.MiiInterval;
                pAMIIfaceStateRes->ConfigData.BondIface.Slaves = bond.Slaves;
                pAMIIfaceStateRes->ConfigData.BondIface.AutoConf = bond.AutoConf;
                return sizeof(INT8U) +sizeof(BondIface);
            }
            else
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }
            break;
        case AMI_IFACE_BOND_ENABLED:
             if(pAMIIfaceStateReq->SetSelect != 0)
            {
                IPMI_WARNING("\nInvalid BondIndex number :%x", pAMIIfaceStateReq->SetSelect);
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            memset(IfcName,0,sizeof(IfcName));
            sprintf(IfcName,"bond%d",pAMIIfaceStateReq->SetSelect);
            for(i=0;i<sizeof(pBMCInfo->LanIfcConfig)/sizeof(LANIFCConfig_T);i++)
            {
                if((strcmp(IfcName,pBMCInfo->LanIfcConfig[i].ifname) == 0 ) && 
                    (pBMCInfo->LanIfcConfig[i].Up_Status == 1))
                {
                    pAMIIfaceStateRes->ConfigData.BondEnable.Enabled = 1;
                    break;
                }
                else
                {
                    pAMIIfaceStateRes->ConfigData.BondEnable.Enabled = 0;
                    pAMIIfaceStateRes->ConfigData.BondEnable.BondIndex = 0;
                }
            }

            for(i=0;i< sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
            {
                if(Ifcnametable[i].Ifcname[0] == 0)
                    continue;

                if( (strcmp(IfcName,Ifcnametable[i].Ifcname) == 0) && 
                    (pAMIIfaceStateRes->ConfigData.BondEnable.Enabled == 1))
                {
                    pAMIIfaceStateRes->ConfigData.BondEnable.BondIndex = Ifcnametable[i].Index;
                    break;
                }
            }
            pAMIIfaceStateRes->CompletionCode = CC_NORMAL;
            return sizeof(INT8U)+sizeof(BondEnabled_T);
            break;
        case AMI_GET_IFACE_COUNT:
            /*Get the no of LAN interface count support for IPMI*/
            if( NULL != g_PDKHandle[PDK_GETIFACECOUNT])
            {
                if( 0xFF != (((INT8U(*)(INT8U *, INT8U *, int))(g_PDKHandle[PDK_GETIFACECOUNT]))
                        (pAMIIfaceStateRes->ConfigData.LANCount.EthIndex, 
                         &pAMIIfaceStateRes->ConfigData.LANCount.Count, BMCInst)))
                {
                    pAMIIfaceStateRes->CompletionCode = CC_NORMAL;
                    return sizeof (AMIGetIfaceStateRes_T);
                }
            }
            
            if(get_network_interface_count(&Ifccount) < 0)
            {
                pAMIIfaceStateRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }
            memset(EthIfcname,0,MAX_ETHIFC_LEN * MAX_LAN_CHANNELS);
            memset(pAMIIfaceStateRes->ConfigData.LANCount.EthIndex,0,MAX_LAN_CHANNEL);

            if(get_network_interfaces_name(EthIfcname,Ifccount) < 0)
            {
                pAMIIfaceStateRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }
            
            count = 0;
            /*Get the Index value of each LAN Interface*/
            for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
            {
                if(Ifcnametable[i].Ifcname[0] == 0)
                    continue;

                for(j=0;j <Ifccount;j++)
                {
                    if((strcmp(Ifcnametable[i].Ifcname,&EthIfcname[j*MAX_ETHIFC_LEN]) == 0)
                      && (strlen(Ifcnametable[i].Ifcname) != 0))
                        {
                            pAMIIfaceStateRes->ConfigData.LANCount.EthIndex[count] = Ifcnametable[i].Index;
                            count++;
                        }
                }
            }
            
            /* force count value to match CONFIG_SPX_FEATURE_GLOBAL_NIC_COUNT */
            NIC_Count =g_coremacros.global_nic_count;
            if(count > NIC_Count)
            {
                count = NIC_Count;
            }

            pAMIIfaceStateRes->CompletionCode = CC_NORMAL;
            pAMIIfaceStateRes->ConfigData.LANCount.Count = count;
            return sizeof(INT8U)+sizeof(LANIfcCount_T);
            break;
        case AMI_GET_IFACE_CHANNEL:
            pAMIIfaceStateRes->ConfigData.IfcChannel.Channel = 0;
            for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
            {
                if(Ifcnametable[i].Ifcname[0] == 0)
                    continue;

                if(Ifcnametable[i].Index== pAMIIfaceStateReq->SetSelect)
                {
                    for(j=0;j<MAX_LAN_CHANNELS;j++)
                    {
                        if(strcmp(Ifcnametable[i].Ifcname,pBMCInfo->LanIfcConfig[j].ifname) == 0)
                        {
                            pAMIIfaceStateRes->ConfigData.IfcChannel.Channel = pBMCInfo->LanIfcConfig[j].Chnum;
                            pAMIIfaceStateRes->CompletionCode = CC_SUCCESS;
                            return sizeof(INT8U)+sizeof(GetIfcChannel_T);
                        }
                    }
                }
            }
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(INT8U);
            break;
        case AMI_GET_IFACE_NAME:

            memset(&pAMIIfaceStateRes->ConfigData.IfcName.IfcName,0,sizeof(MAX_IFACE_NAME));
            for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
            {
                if(Ifcnametable[i].Ifcname[0] == 0)
                    continue;

                if(Ifcnametable[i].Index== pAMIIfaceStateReq->SetSelect)
                {
                    memcpy(&pAMIIfaceStateRes->ConfigData.IfcName.IfcName,&Ifcnametable[i].Ifcname,MAX_IFACE_NAME);
                    pAMIIfaceStateRes->CompletionCode = CC_SUCCESS;
                    return sizeof(INT8U)+sizeof(GetIfcName_T);
                }
            }
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(INT8U);
            break;
        case AMI_BOND_ACTIVE_SLAVE:
            if(g_corefeatures.bond_support == ENABLED)
            {
                memset(IfcName,0,sizeof(IfcName));
                sprintf(IfcName,"bond%d",pAMIIfaceStateReq->SetSelect);
                for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
                {
                    if(Ifcnametable[i].Ifcname[0] == 0)
                        continue;
    
                    if((strcmp(IfcName,Ifcnametable[i].Ifcname) == 0 ))
                    {
                        pAMIIfaceStateRes->ConfigData.ActiveSlave.BondIndex = pAMIIfaceStateReq->SetSelect;
                        break;
                    }
                }
    
                /*Bond index is Invalid*/
                if(i == sizeof(Ifcnametable)/sizeof(IfcName_T))
                {
                    *pRes = OEMCC_BOND_NOT_ENABLED;
                    return sizeof(INT8U);
                }
    
                /*Get the active slave configuration*/
                nwGetActiveSlave(pAMIIfaceStateReq->SetSelect,&pAMIIfaceStateRes->ConfigData.ActiveSlave.ActiveIndex);
                pAMIIfaceStateRes->CompletionCode = CC_NORMAL;
                return sizeof(INT8U)+sizeof(ActiveSlave_T);
            }
            else
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }
            break;
        case AMI_BOND_VLAN_ENABLED:
            if(g_corefeatures.bond_support == ENABLED)
            {
                memset(IfcName,0,sizeof(IfcName));
                sprintf(IfcName,"bond%d",pAMIIfaceStateReq->SetSelect);
    
                if(CheckVLANInterface(IfcName, BMCInst) == 1)
                    pAMIIfaceStateRes->ConfigData.BondVLAN.Enabled = 1;
                else
                    pAMIIfaceStateRes->ConfigData.BondVLAN.Enabled = 0;
    
                pAMIIfaceStateRes->CompletionCode = CC_SUCCESS;
                return sizeof(INT8U)+sizeof(BondVLAN_T);
            }
            else
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }
            break;
        case AMI_CURR_ACTIVE_IFACE_COUNT:
            if(get_network_interface_count(&Ifccount) < 0)
            {
                pAMIIfaceStateRes->CompletionCode = CC_UNSPECIFIED_ERR;
                return sizeof(INT8U);
            }
            count = 0;
            memset(pAMIIfaceStateRes->ConfigData.LANCount.EthIndex,0,MAX_LAN_CHANNEL);
            for(EthIndex=0;EthIndex <Ifccount;EthIndex++)
            {
                memset(&cfg, 0, sizeof(NWCFG_STRUCT));
                memset(&cfg6, 0, sizeof(NWCFG6_STRUCT));
                retValue = nwReadNWCfg_v4_v6(&cfg, &cfg6, EthIndex,g_corefeatures.global_ipv6);
                if(retValue != 0)
                {
                    TCRIT("Error in reading network configuration.\n");
                    pAMIIfaceStateRes->CompletionCode = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                if(cfg.enable | cfg6.enable)
                {
                    pAMIIfaceStateRes->ConfigData.LANCount.EthIndex[count] = EthIndex;
                    count++;
                }
            }
            pAMIIfaceStateRes->CompletionCode = CC_NORMAL;
            pAMIIfaceStateRes->ConfigData.LANCount.Count = count;
            return sizeof(INT8U)+sizeof(LANIfcCount_T);
        default:
            pAMIIfaceStateRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
    }
    pAMIIfaceStateRes->CompletionCode = CC_SUCCESS;
    return sizeof (AMIGetIfaceStateRes_T);
}

/**
 * @fn AMIGetSELPolicy
 * @brief This function returns current SEL policy.
 * @param Request message and BMC instance
 * @return Current SEL policy
 */
int
AMIGetSELPolicy (INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst)
{
    AMIGetSELPolicyRes_T* pAMIGetSELPolicyRes = (AMIGetSELPolicyRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    pAMIGetSELPolicyRes->CompletionCode = CC_NORMAL;
    pAMIGetSELPolicyRes->SELPolicy = pBMCInfo->AMIConfig.CircularSEL;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    return sizeof(AMIGetSELPolicyRes_T);
}


/**
 * @fn AMISetSELPolicy
 * @brief This function sets current SEL policy.
 * @param Request message and BMC instance
 * @return Success completion code
 */
int
AMISetSELPolicy (INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst)
{
    AMISetSELPolicyRes_T* pAMISetSELPolicyRes = (AMISetSELPolicyRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if(g_corefeatures.circular_sel == ENABLED)
    {
        AMISetSELPolicyReq_T* pAMISetSELPolicyReq = (AMISetSELPolicyReq_T*)pReq;

        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
        /* Validate SEL policy */
        if (pAMISetSELPolicyReq->SELPolicy != LINEAR_SEL && pAMISetSELPolicyReq->SELPolicy != CIRCULAR_SEL)
        {
            pAMISetSELPolicyRes->CompletionCode = CC_INV_DATA_FIELD;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return sizeof(*pRes);
        }
    
        SetSELPolicy(pAMISetSELPolicyReq->SELPolicy, BMCInst);
    
        pAMISetSELPolicyRes->CompletionCode = CC_NORMAL;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof(AMISetSELPolicyRes_T);
    }
    else
    {
        /* If Circular SEL feature is disabled in Project Configuration, return Not Support In Current State code. */
        pAMISetSELPolicyRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
        return sizeof(*pRes);
    }
}


/*
*@fn AMIGetSELEntires
*@param This function retrieves the SEL entries
*@return Returns CC_NORMAL
*/
int AMIGetSELEntires(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst)
{
    AMIGetSELEntriesReq_T *pAMIGetSelEntriesReq = (AMIGetSELEntriesReq_T *)pReq;
    AMIGetSELEntriesRes_T *pAMIGetSelEntiresRes = (AMIGetSELEntriesRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    SELRepository_T *m_sel = NULL;
    INT16U NumRecords = 0;

    if(g_corefeatures.del_sel_reclaim_support != ENABLED)
    {
         m_sel = (SELRepository_T*)GetSDRSELNVRAddr((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
         NumRecords = m_sel->NumRecords;
    }
    else
    {
         NumRecords = pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords;
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);

    if(NumRecords == SEL_EMPTY_REPOSITORY)
    {
        pAMIGetSelEntiresRes->CompletionCode = OEMCC_SEL_EMPTY_REPOSITORY;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof(*pRes);
    }
    else if (NumRecords < pAMIGetSelEntriesReq->Noofentretrieved)
    {
        pAMIGetSelEntiresRes->CompletionCode = OEMCC_SEL_CLEARED;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof(*pRes);
    }
    else if(((NumRecords - pAMIGetSelEntriesReq->Noofentretrieved) * sizeof(SELRec_T)) < MAX_FULL_SEL_ENTRIES)
    {
        pAMIGetSelEntiresRes->Status = FULL_SEL_ENTRIES;
        pAMIGetSelEntiresRes->Noofentries = NumRecords - pAMIGetSelEntriesReq->Noofentretrieved;
    }
    else
    {
        pAMIGetSelEntiresRes->Status = PARTIAL_SEL_ENTRIES;
        pAMIGetSelEntiresRes->Noofentries = ((MAX_FULL_SEL_ENTRIES - sizeof(AMIGetSELEntriesRes_T))/sizeof(SELRec_T));
    }


   if(g_corefeatures.del_sel_reclaim_support != ENABLED)
   {
        memcpy((INT8U*)(pAMIGetSelEntiresRes + 1),(INT8U*)&m_sel->SELRecord[pAMIGetSelEntriesReq->Noofentretrieved],
                  sizeof(SELRec_T)*pAMIGetSelEntiresRes->Noofentries);
   }
   else
   {

      CopySELReclaimEntries(&(SEL_RECLAIM_HEAD_NODE(BMCInst)),(INT8U *)(pAMIGetSelEntiresRes+1),
                                              pAMIGetSelEntriesReq->Noofentretrieved,pAMIGetSelEntiresRes->Noofentries,BMCInst);
   }

    pAMIGetSelEntiresRes->LastRecID = BMC_GET_SHARED_MEM(BMCInst)->m_LastRecID;

    pAMIGetSelEntiresRes->CompletionCode = CC_NORMAL;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    return sizeof(AMIGetSELEntriesRes_T) + (sizeof(SELRec_T) * pAMIGetSelEntiresRes->Noofentries);
}

/**
 * @fn AMIGetSensorInfo
 * @brief This function Gets all the Sensor Info needed for Web.
 * @param Request message and BMC instance
 * @return Proper completion code
 */
int AMIGetSenforInfo(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst)
{
    AMIGetSensorInfoRes_T *pAMIGetSensorInfoRes = (AMIGetSensorInfoRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U*   pValidSensor = NULL;
    INT16U       SensorIndex = 0;
    SensorInfo_T    pSensor ;
    SenInfo_T       SensorInfo;
    BOOL SensorIsSigned = FALSE;
    INT16U SensorReading = 0;
    SensorSharedMem_T*    pSMSharedMem;
     SDRRecHdr_T*     pSDRRec;
    FullSensorRec_T*      FullSDR;
    CompactSensorRec_T*   CompSDR;
    int i = 0;

    /* Get the Sensor Shared Memory */
    pSMSharedMem = (SensorSharedMem_T*)&pBMCInfo->SensorSharedMem;

    if(pBMCInfo->SenConfig.ValidSensorCnt == 0)
    {
        pAMIGetSensorInfoRes->CompletionCode = OEMCC_SENSOR_INFO_EMPTY;
        return sizeof(*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->m_hSMSharedMemMutex, SHARED_MEM_TIMEOUT);
    pValidSensor = (INT8U*)(pAMIGetSensorInfoRes+1);
    for(i = 0; i < pBMCInfo->SenConfig.ValidSensorCnt; i++)
    {
        SensorIndex = pBMCInfo->SenConfig.ValidSensorList[i];

        if ((g_corefeatures.node_manager == ENABLED) && (pBMCInfo->NMConfig.NMDevSlaveAddress == pBMCInfo->SenConfig.OwnerIDList[i] )){
            pSensor = pSMSharedMem->ME_SensorInfo [SensorIndex];
        }else{      
            pSensor = pSMSharedMem->SensorInfo [SensorIndex];
        }

        /*Copy the SDR Header*/
        memcpy(&SensorInfo.hdr,pSensor.SDRRec,sizeof(SDRRecHdr_T));

        SensorInfo.SensorNumber = pSensor.SensorNumber;
        SensorInfo.SensorTypeCode = pSensor.SensorTypeCode;
        SensorInfo.EventTypeCode = pSensor.EventTypeCode;

        SensorInfo.Units1 = pSensor.Units1;
        SensorInfo.Units2 = pSensor.Units2;
        SensorInfo.Units3 = pSensor.Units3;

        SensorInfo.M_LSB = pSensor.M_LSB;
        SensorInfo.M_MSB_Tolerance = pSensor.M_MSB_Tolerance;
        SensorInfo.B_LSB = pSensor.B_LSB;
        SensorInfo.B_MSB_Accuracy = pSensor.B_MSB_Accuracy;
        SensorInfo.Accuracy_MSB_Exp = pSensor.Accuracy_MSB_Exp;
        SensorInfo.RExp_BExp = pSensor.RExp_BExp;

        SensorInfo.LowerNonCritical = pSensor.LowerNonCritical;
        SensorInfo.LowerCritical = pSensor.LowerCritical;
        SensorInfo.LowerNonRecoverable = pSensor.LowerNonRecoverable;
        SensorInfo.UpperNonCritical = pSensor.UpperNonCritical;
        SensorInfo.UpperCritical = pSensor.UpperCritical;
        SensorInfo.UpperNonRecoverable= pSensor.UpperNonRecoverable;
        SensorInfo.Settable_Readable_ThreshMask= pSensor.SettableThreshMask;

        SensorInfo.Flags = pSensor.EventFlags & 0xe0;
        if((pSensor.EventFlags & BIT5) != 0)
        {
            SensorInfo.SensorReading = 0;
        }

        SensorReading = pSensor.SensorReading;
        SensorInfo.SensorReading = 0;
        SensorIsSigned = ( 0 != (pSensor.InternalFlags & BIT1));

        if (THRESHOLD_SENSOR_CLASS  == pSensor.EventTypeCode)
        {
            SensorInfo.SensorReading = (SensorReading & 0x00FF);
            SensorInfo.ComparisonStatus = 0;
            if((pSensor.DeassertionEventEnablesByte2 & BIT6) == BIT6 )
            {
                if (CompareValues(SensorIsSigned, SensorInfo.SensorReading, pSensor.UpperNonRecoverable) >= 0)
                {
                    SensorInfo.ComparisonStatus |= BIT5;
                }
            }
            if((pSensor.DeassertionEventEnablesByte2 & BIT5) == BIT5 )
            {
                if (CompareValues(SensorIsSigned, SensorInfo.SensorReading, pSensor.UpperCritical) >= 0)
                {
                    SensorInfo.ComparisonStatus |= BIT4;
                }
            }
            if((pSensor.DeassertionEventEnablesByte2 & BIT4) == BIT4 )
            {
                if (CompareValues(SensorIsSigned, SensorInfo.SensorReading, pSensor.UpperNonCritical) >= 0)
                {
                    SensorInfo.ComparisonStatus |= BIT3;
                }
            }
            if((pSensor.AssertionEventEnablesByte2 & BIT6) == BIT6 )
            {
                if (CompareValues(SensorIsSigned, SensorInfo.SensorReading, pSensor.LowerNonRecoverable) <= 0)
                {
                    SensorInfo.ComparisonStatus |= BIT2;
                }
            }
            if((pSensor.AssertionEventEnablesByte2 & BIT5) == BIT5 )
            {
                if (CompareValues(SensorIsSigned, SensorInfo.SensorReading, pSensor.LowerCritical) <= 0)
                {
                    SensorInfo.ComparisonStatus |= BIT1;
                }
            }
            if((pSensor.AssertionEventEnablesByte2 & BIT4) == BIT4 )
            {
                if (CompareValues(SensorIsSigned, SensorInfo.SensorReading, pSensor.LowerNonCritical) <= 0)
                {
                    SensorInfo.ComparisonStatus |= BIT0;
                }
            }

            SensorInfo.ComparisonStatus &= ((pSensor.SettableThreshMask >>  8) & 0xFF);
            SensorInfo.OptionalStatus = 0;
            // For Threshold sensor, [7:6] - reserved. Returned as 1b. Ignore on read.
            SensorInfo.ComparisonStatus |= THRESHOLD_RESERVED_BIT;
        }
        else
        {
        	SensorInfo.ComparisonStatus = (((INT8U) (SensorReading & 0x00FF)) & ((INT8U) (pSensor.SettableThreshMask & 0x00FF)) );
        	SensorInfo.OptionalStatus   = (((INT8U) (SensorReading >> 8)) & ((INT8U) (pSensor.SettableThreshMask >> 8)) );
            // For Discrete sensor, [7] - reserved. Returned as 1b. Ignore on read.
            SensorInfo.OptionalStatus  |= DISCRETE_RESERVED_BIT;
        }

        if((pSensor.EventFlags & BIT7) == 0)
        {
            SensorInfo.AssertionEventByte1 = 0;
            SensorInfo.AssertionEventByte2 = 0;
            SensorInfo.DeassertionEventByte1 = 0;
            SensorInfo.DeassertionEventByte2 = 0;
        }
        if((pSensor.SensorCaps & BIT6) == 0)
        {
             SensorInfo.AssertionEventByte1    = (pSensor.AssertionHistoryByte1 & pSensor.AssertionEventEnablesByte1);
             SensorInfo.AssertionEventByte2    = (pSensor.AssertionHistoryByte2 & pSensor.AssertionEventEnablesByte2);
             SensorInfo.DeassertionEventByte1    = (pSensor.DeassertionHistoryByte1 & pSensor.DeassertionEventEnablesByte1);
             SensorInfo.DeassertionEventByte2    = (pSensor.DeassertionHistoryByte2 & pSensor.DeassertionEventEnablesByte2);
        }
        else
        {
             SensorInfo.AssertionEventByte1    = (pSensor.AssertionEventOccuredByte1 & pSensor.AssertionEventEnablesByte1);
             SensorInfo.AssertionEventByte2    = (pSensor.AssertionEventOccuredByte2 & pSensor.AssertionEventEnablesByte2);
             SensorInfo.DeassertionEventByte1    = (pSensor.DeassertionEventOccuredByte1 & pSensor.DeassertionEventEnablesByte1);
             SensorInfo.DeassertionEventByte2    = (pSensor.DeassertionEventOccuredByte2 & pSensor.DeassertionEventEnablesByte2);
        }
        pSDRRec = GetSDRRec(pSensor.SDRRec->ID,BMCInst);
        if(pSensor.SDRRec->Type  == FULL_SDR_REC)       /*Full SDR*/
        {
            FullSDR = (FullSensorRec_T *)pSDRRec;
            SensorInfo.OwnerID = FullSDR->OwnerID;
            SensorInfo.OwnerLUN= FullSDR->OwnerLUN;
            SensorInfo.MaxReading = FullSDR->MaxReading;
            SensorInfo.MinReading = FullSDR->MinReading;
            SensorInfo.Linearization = FullSDR->Linearization;
            memset(SensorInfo.SensorName,0,MAX_ID_STR_LEN);
            strncpy(SensorInfo.SensorName,FullSDR->IDStr, MAX_ID_STR_LEN - (sizeof(FullSensorRec_T) - sizeof(SDRRecHdr_T) - FullSDR->hdr.Len));
        }
        else if(pSensor.SDRRec->Type == COMPACT_SDR_REC)   /*Compact SDR*/
        {
            CompSDR = (CompactSensorRec_T *)pSDRRec;
            SensorInfo.OwnerID = CompSDR->OwnerID;
            SensorInfo.OwnerLUN= CompSDR->OwnerLUN;
            SensorInfo.MaxReading = 0;
            SensorInfo.MinReading = 0;
            SensorInfo.Linearization = 0;
            memset(SensorInfo.SensorName,0,MAX_ID_STR_LEN);
            strncpy(SensorInfo.SensorName,CompSDR->IDStr, MAX_ID_STR_LEN - (sizeof(CompactSensorRec_T) - sizeof(SDRRecHdr_T) - CompSDR->hdr.Len));
        }
        memcpy(pValidSensor,(INT8U *)&SensorInfo,sizeof(SenInfo_T));
        pValidSensor = pValidSensor + sizeof(SenInfo_T);
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->m_hSMSharedMemMutex);

    pAMIGetSensorInfoRes->CompletionCode = CC_NORMAL;
    pAMIGetSensorInfoRes->Noofentries = pBMCInfo->SenConfig.ValidSensorCnt;

    return sizeof(AMIGetSensorInfoRes_T) + (sizeof(SenInfo_T) * pBMCInfo->SenConfig.ValidSensorCnt);

}

/**
 * @fn AMIGetIPMISessionTimeOut
 * @brief This function Gets IPMI session timeout.
 * @param Request message and BMC instance
 * @return Proper completion code
 */
int AMIGetIPMISessionTimeOut(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst)
{
    AMIGetIPMISessionTimeOutRes_T * pAMIGetIPMISessionTimeOutRes = (AMIGetIPMISessionTimeOutRes_T *)pRes;
    /* Chk request Length */
    if( ReqLen != 0 )
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof (INT8U);
    }
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    pAMIGetIPMISessionTimeOutRes->IPMISessionTimeOut = pBMCInfo->IpmiConfig.SessionTimeOut;
    pAMIGetIPMISessionTimeOutRes->CompletionCode = CC_NORMAL;
    return sizeof( AMIGetIPMISessionTimeOutRes_T );
}

/**
 * @fn AMIGetUDSInfo
 * @brief This function Gets UDS Info for Requesting IP's.
 * @param IPV4/IPV6 Address and BMC instance
 * @return Channel Number,BMC Instance and Completion Code
 */
int AMIGetUDSInfo(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst)
{
    AMIGetUDSInfoReq_T *pAMIGetUDSInfoReq = (AMIGetUDSInfoReq_T *)pReq;
    AMIGetUDSInfoRes_T *pAMIGetUDSInfoRes = (AMIGetUDSInfoRes_T *)pRes;
    int ChannelNum = -1;
    INT8U NoofBMC = 1;
      ChannelInfo_T*          pChannelInfo;

    for(NoofBMC = 1;NoofBMC <= MAX_NUM_BMC; NoofBMC++)
    {
         ChannelNum = GetChannelByAddr((char *)&pAMIGetUDSInfoReq->SessionIPAddr[0],NoofBMC);
         if(ChannelNum == -1)
         {
             if(g_corefeatures.global_ipv6  == ENABLED)
             {
                 ChannelNum = GetChannelByIPv6Addr((char *)&pAMIGetUDSInfoReq->SessionIPAddr[0], NoofBMC);
                 if(ChannelNum != -1)
                 {
                     TDBG("IPV6 Address Found and Channel Number is %x\n",ChannelNum);
                     break;
                 }
             }
         }
         else
         {
             TDBG("IPV4 Address Found and Channel Number is %d\n",ChannelNum);
             break;
         }
    }

    if(ChannelNum != -1)
    {
        pChannelInfo = getChannelInfo(ChannelNum,NoofBMC);
        if(NULL == pChannelInfo)
        {
            *pRes = CC_INV_DATA_FIELD;
            return  sizeof (*pRes);
        }

        pAMIGetUDSInfoRes->CompletionCode = CC_NORMAL;
        pAMIGetUDSInfoRes->ChannelNum = ChannelNum;
        pAMIGetUDSInfoRes->ChannelType = pChannelInfo->ChannelType;
        pAMIGetUDSInfoRes->BMCInstance = NoofBMC;
    }
    else 
    {    
        pAMIGetUDSInfoRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }        
    return sizeof(AMIGetUDSInfoRes_T);
}   

/**
 * @fn AMIGetUDSSessionInfo
 * @brief This function Gets UDS Session Info for Requesting Session Handle
 *        /Session Index/Session ID.
 * @param Parameter Selector and BMC instance
 * @return UDS Session Information and Completion Code
 */
int AMIGetUDSSessionInfo(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst)
{
    AMIGetUDSSessionInfoReq_T *pAMIGetUDSSessionInfoReq = (AMIGetUDSSessionInfoReq_T *)pReq;
    AMIGetUDSSessionInfoRes_T *pAMIGetUDSSessionInfoRes = ( AMIGetUDSSessionInfoRes_T *)pRes;
    UDSSessionTbl_T *pUDSSessionTblInfo = NULL;
    INT8U ArgType =0xFF;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U Index,ResLen;

    if( ReqLen != UDSSessionInfoParamLength[pAMIGetUDSSessionInfoReq->UDSSessionParam])
    {
        *pRes = CC_REQ_INV_LEN;
         return sizeof(*pRes);
    }

    pAMIGetUDSSessionInfoRes->CompletionCode = CC_NORMAL;

    switch(pAMIGetUDSSessionInfoReq->UDSSessionParam)
    {

        case UDS_SESSION_COUNT_INFO:
            pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSSessionCountInfo.MaxAllowedSession = pBMCInfo->IpmiConfig.MaxSession;
            pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSSessionCountInfo.ActiveSessionCount = pBMCInfo->UDSSessionTblInfo.SessionCount;
            return sizeof(INT8U )+sizeof(UDSSessionCount_T);

        case UDS_SESSION_ID_INFO:
            ArgType = UDS_SESSION_ID_INFO;
            break;

        case UDS_SESSION_HANDLE_INFO:
            ArgType = UDS_SESSION_HANDLE_INFO;
            break;

        case UDS_SESSION_INDEX_INFO:
            ArgType = UDS_SESSION_INDEX_INFO;
            break;

         case UDS_SOCKET_ID_INFO:
            ArgType = UDS_SOCKET_ID_INFO;
            break;

         case UDS_SESSION_PID_INFO:
             ArgType = UDS_SESSION_INDEX_INFO;
             pUDSSessionTblInfo = GetUDSSessionInfo(ArgType,(void *)&pAMIGetUDSSessionInfoReq->UDSSessionHandleOrIDOrIndex[0],BMCInst);
             if(NULL == pUDSSessionTblInfo)
             {
                 *pRes =CC_INV_DATA_FIELD;
                 return sizeof(*pRes);
             }
             pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSSessionPIDInfo.ProcessID = ipmitoh_u32(pUDSSessionTblInfo->ProcessID);
             pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSSessionPIDInfo.ThreadID = ipmitoh_u32(pUDSSessionTblInfo->ThreadID);
             return sizeof(INT8U )+sizeof(UDSSessionPIDInfo_T);
             break;

         case UDS_ACTIVE_SESSION_INDEX_LIST:
             pRes[0] = CC_NORMAL;
             for(Index = 0, ResLen = 1; Index < pBMCInfo->IpmiConfig.MaxSession; Index++)
             {
                 if(FALSE == pBMCInfo->UDSSessionTblInfo.UDSSessionTbl[Index].Activated)
                 {
                     continue;
                 }
                 pRes[ResLen] = Index;
                 ResLen++;
             }
             return ResLen;
             break;

         default:
            *pRes=CC_INV_DATA_FIELD;
            return sizeof (*pRes);

     }

     pUDSSessionTblInfo = GetUDSSessionInfo(ArgType,(void *)&pAMIGetUDSSessionInfoReq->UDSSessionHandleOrIDOrIndex[0],BMCInst);
     if(NULL == pUDSSessionTblInfo)
     {
         *pRes =CC_INV_DATA_FIELD;
         return sizeof(*pRes);
     }

     /* Filling up the required Session Related Response for the requested Session ID/Session Handle/Session Index */
     pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSLoggedInSessionInfo.LoggedInSessionID = pUDSSessionTblInfo->SessionID;
     pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSLoggedInSessionInfo.LoggedInSessionHandle = pUDSSessionTblInfo->LoggedInSessionHandle;
     pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSLoggedInSessionInfo.LoggedInTime = pUDSSessionTblInfo->LoggedInTime;
     pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSLoggedInSessionInfo.LoggedInUserID = pUDSSessionTblInfo->LoggedInUserID;
     pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSLoggedInSessionInfo.LoggedInPrivilege = pUDSSessionTblInfo->LoggedInPrivilege;
     pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSLoggedInSessionInfo.AuthenticationMechanism = pUDSSessionTblInfo->AuthenticationMechanism;
     pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSLoggedInSessionInfo.UDSChannelNum = pUDSSessionTblInfo->UDSChannelNum;
     pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSLoggedInSessionInfo.ChannelNum = pUDSSessionTblInfo->LoggedInChannel;
     pAMIGetUDSSessionInfoRes->UDSLoggedInInfo.UDSLoggedInSessionInfo.SessionTimeoutValue = pUDSSessionTblInfo->SessionTimeoutValue;

     return sizeof(INT8U)+sizeof(UDSLoggedInSessionInfo_T);
}

/**
* @fn RISServiceStatus
 * @brief This function Gets Remote Image service feature supported and enable informations
 */
int 
RISServiceStatus()
{
    VMediaCfg_T media_cfg;
     int ServiceErr = 0;

     /*Check for Feature is supported*/
    if(ENABLED != g_corefeatures.rmedia_support )
    {
         TCRIT("Remote Images Service is Not Supported\n");
         return CC_INV_CMD;
    }

    memset(&media_cfg, 0, sizeof(VMediaCfg_T));
    ServiceErr = GetVMediaCfg(&media_cfg,g_corefeatures.lmedia_support,g_corefeatures.rmedia_support,g_corefeatures.power_consumption_virtual_device_usb);
    if(ServiceErr < 0)
    {
        TCRIT("Error in Getting Vmedia Configuration\n");
        return CC_UNSPECIFIED_ERR;
    }
    /*Check for RIS Sevice is Enabled*/
    if(ENABLED != media_cfg.rmedia_enable )
    {
         TCRIT("Remote Images Service is Not Enabled\n");
         return CC_SERVICE_NOT_ENABLED;
    }
    return  CC_SUCCESS;

}

int LMediaServiceStatus()
{
    VMediaCfg_T media_cfg;
     int ServiceErr = 0;

     /*Check for Feature is supported*/
    if(ENABLED != g_corefeatures.lmedia_support )
    {
         TCRIT("Local Media Service is Not Supported\n");
         return CC_INV_CMD;
    }
    memset(&media_cfg, 0, sizeof(VMediaCfg_T));
    ServiceErr = GetVMediaCfg(&media_cfg,g_corefeatures.lmedia_support,g_corefeatures.rmedia_support,g_corefeatures.power_consumption_virtual_device_usb);
    if(ServiceErr < 0)
    {
        TCRIT("Error in Getting Vmedia Configuration\n");
        return CC_UNSPECIFIED_ERR;
    }
    /*Check for Lmedia Sevice is Enabled*/
    if(ENABLED != media_cfg.lmedia_enable )
    {
         TCRIT("Local Media Service is Not Supported\n");
         return CC_SERVICE_NOT_ENABLED;
    }
    return CC_SUCCESS;
}

/*
option : 1- get total image count ,0-get image list
*/
int RISGetAllImagesList(int option,int *TotalCnt,MediaImageCfg_T  **Image_list_buffer)
{
	int retval = -1;
	int CdCnt,FdCnt,HdCnt;
	MediaImageCfg_T *TempList = NULL;
	MediaImageCfg_T *CdImgList = NULL,*FdImgList = NULL,*HdImgList = NULL;
	int num_record = 0,index,alloc_size;


	if(( 0 != option) && ( 1 != option))
	{
		TCRIT("Invalid option parameter\n");
		return retval;
	}

	retval = GetlistImages(RMS_CD_IMG_LIST_FILE, &CdCnt,&CdImgList);
	if (0 != retval)
	{
		if(CdImgList != NULL)
			free(CdImgList);
		return retval;
	}
	retval = GetlistImages(RMS_FD_IMG_LIST_FILE, &FdCnt,&FdImgList);
	if (0 != retval)
	{
		if(FdImgList != NULL)
			free(FdImgList);
		return retval;
	}
	retval = GetlistImages(RMS_HD_IMG_LIST_FILE, &HdCnt,&HdImgList);
	if (0 != retval)
	{
		if(HdImgList != NULL)
			free(HdImgList);
		return retval;
	}

	//Get avaialble images count option
	if( 1 == option)
	{
		*Image_list_buffer = NULL;
		*TotalCnt = CdCnt + FdCnt + HdCnt;

		if(NULL != CdImgList)
			free(CdImgList);
		if(NULL != FdImgList)
			free(FdImgList);
		if(NULL != HdImgList)
			free(HdImgList);

		return 0;
	}

	alloc_size = (CdCnt + FdCnt + HdCnt)*sizeof(MediaImageCfg_T);
	if( (TempList = malloc((size_t)alloc_size)) == NULL )
	
	{
		TCRIT("Unable to allocate memory for Rmedia Image list \n");
		if(NULL != CdImgList)
			free(CdImgList);
		if(NULL != FdImgList)
			free(FdImgList);
		if(NULL != HdImgList)
			free(HdImgList);

		return errno;
	}

	for(index=0;index < CdCnt;index++)
	{
		memset(&TempList[num_record],0,sizeof(MediaImageCfg_T));
		strncpy(TempList[num_record].Image_name,CdImgList[index].Image_name,strlen(CdImgList[index].Image_name));
		TempList[num_record].Image_index = CdImgList[index].Image_index;
		TempList[num_record].Image_type = CdImgList[index].Image_type;
		num_record++;
	}

	for(index=0;index < FdCnt;index++)
	{
		memset(&TempList[num_record],0,sizeof(MediaImageCfg_T));
		strncpy(TempList[num_record].Image_name,FdImgList[index].Image_name,strlen(FdImgList[index].Image_name));
		TempList[num_record].Image_index = FdImgList[index].Image_index;
		TempList[num_record].Image_type = FdImgList[index].Image_type;
		num_record++;
	}

	for(index=0;index < HdCnt;index++)
	{
		memset(&TempList[num_record],0,sizeof(MediaImageCfg_T));
		strncpy(TempList[num_record].Image_name,HdImgList[index].Image_name,strlen(HdImgList[index].Image_name));
		TempList[num_record].Image_index = HdImgList[index].Image_index;
		TempList[num_record].Image_type = HdImgList[index].Image_type;
		num_record++;
	}

	if(NULL != CdImgList)
		free(CdImgList);
	if(NULL != FdImgList)
		free(FdImgList);
	if(NULL != HdImgList)
		free(HdImgList);

	*Image_list_buffer = TempList;
	*TotalCnt = num_record;

	return 0;
}
/*---------------------------------------
 * AMIMediaRedirectionStartStop
 *---------------------------------------*/
int
AMIMediaRedirectionStartStop( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
	AMIMediaRedirctionStartStopReq_T*  pAMIMediaRedirctionStartStopReq = (AMIMediaRedirctionStartStopReq_T*) pReq;
	AMIMediaRedirctionStartStopRes_T*  pAMIMediaRedirctionStartStopRes = (AMIMediaRedirctionStartStopRes_T*) pRes;
	int retval = -1,ServiceErr = 0;
	int Image_Type = 0,ImageIndex = 0, AppType = 0;
	char *ImageName= NULL;
	int datalen = -1;

	AppType = pAMIMediaRedirctionStartStopReq->AppType;
	if(pAMIMediaRedirctionStartStopReq->Param == BY_IMAGE_INDEX)
	{
		if(ReqLen != (sizeof(AMIMediaRedirctionStartStopReq_T) - sizeof(ImageInfo_T) + sizeof(INT8U)) )
		{
			*pRes = CC_REQ_INV_LEN;
			return sizeof(INT8U);
		}
	}
	else
	{
		datalen = ReqLen - (sizeof(AMIMediaRedirctionStartStopReq_T) - sizeof(ImageInfo_T));
		if((ReqLen > (sizeof(AMIMediaRedirctionStartStopReq_T) - sizeof(INT8U)) ) || (datalen <= 0))
		{
			*pRes = CC_REQ_INV_LEN;
			return sizeof(INT8U);
		}
	}

	*pRes = CC_SUCCESS;
	if(AppType == REMOTE_MEDIA_TYPE)
	{
		ServiceErr = RISServiceStatus();
		if(ServiceErr != 0)
		{
			*pRes = ServiceErr;
			return sizeof(INT8U);
		}
	}
	else if(AppType == LOCAL_MEDIA_TYPE)
	{
		ServiceErr = LMediaServiceStatus();
		if(ServiceErr != 0)
		{
			*pRes = ServiceErr;
			return sizeof(INT8U);
		}
	}
	else
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(INT8U);
	}
	Image_Type = pAMIMediaRedirctionStartStopReq->ImageType;
	if( (Image_Type != IMAGE_TYPE_CD) && (Image_Type !=IMAGE_TYPE_FD ) && ( Image_Type != IMAGE_TYPE_HD) )
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(INT8U);
	}

	switch(pAMIMediaRedirctionStartStopReq->Param)
		{
			case BY_IMAGE_INDEX:
				ImageIndex = pAMIMediaRedirctionStartStopReq->ImageInfo.Index;
				break;
			case BY_IMAGE_NAME:
				ImageIndex = -1;
				ImageName = (char *)malloc(REDIRECTED_IMAGE_LEN*sizeof(INT8U));
				memset(ImageName,0,REDIRECTED_IMAGE_LEN*sizeof(INT8U));
				if(ImageName == NULL)
				{
					*pRes = CC_UNSPECIFIED_ERR;
					return sizeof(INT8U);
				}
				strncpy(ImageName,(char *)pAMIMediaRedirctionStartStopReq->ImageInfo.Name,datalen);
				break;
			default:
				*pRes = CC_INV_PARAM;
				return sizeof(INT8U);
		}

	if( (pAMIMediaRedirctionStartStopReq->RedirectionState !=MEDIA_REDIRECTION_STOP ) &&
		(pAMIMediaRedirctionStartStopReq->RedirectionState !=MEDIA_REDIRECTION_START ))
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(INT8U);
	}

	if(AppType == LOCAL_MEDIA_TYPE)
	{
		if(0 < IsRequestInProgress(AppType,Image_Type,FILEAVAIL))
		{
			TINFO("Info : Media Redirection Request in Progress.Try after some time\n");
			*pRes = ERR_APP_REQ_IN_PROGRESS;
			return sizeof(INT8U);
		}
		else
		{
			IsRequestInProgress(AppType,Image_Type,TOUCH);
		}
	}

	retval = MediaExecAPI(AppType,ImageName,ImageIndex, Image_Type, pAMIMediaRedirctionStartStopReq->RedirectionState);
	if(retval != 0)
	{
		*pRes = retval;
		if(ImageName != NULL)
			free(ImageName);
		if(AppType == LOCAL_MEDIA_TYPE)
		{
			//Command is executed so Remove the file here 
			IsRequestInProgress(AppType,Image_Type,RMFILE);
		}
		return sizeof(INT8U);
	}

	if(ImageName != NULL)
		free(ImageName);
	pAMIMediaRedirctionStartStopRes->CompletionCode = CC_NORMAL;
	return sizeof(AMIMediaRedirctionStartStopRes_T);

}
/*---------------------------------------
 * AMIGetMediaInfo
 *---------------------------------------*/
int
AMIGetMediaInfo( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
	AMIGetMediaInfoReq_T*  pAMIGetMediaInfoReq = (AMIGetMediaInfoReq_T*) pReq;
	AMIGetMediaInfoRes_T*  pAMIGetMediaInfoRes = (AMIGetMediaInfoRes_T*) pRes;

	ShortImageInfo_T* ShortImageInfo = NULL;
	LongImageInfo_T*  LongImageInfo = NULL;
	RedirectedImageInfo_T* RedirectedImageInfo =NULL;
	INT8U MaxCount = 0, ImageName_Format = -1;
	ImageCfg_T *LMediaCfg = NULL;
	MediaImageCfg_T *MediaImageCfg = NULL;
	int ServiceErr ;
	int Image_Type = 0,ImageIndex = -1, AppType = 0,len = 0,i=0,j=0,ToatalAvaliableImages=0;
	char ImageName[256] = {0};
	int MaxCd=0,MaxFd=0,MaxHd=0;
	int retval = -1;
	char tmpstr[SHORT_IMG_NAME_LEN+1] = {0};

	AppType = pAMIGetMediaInfoReq->AppType;
	if(AppType == REMOTE_MEDIA_TYPE)
	{
		ServiceErr = RISServiceStatus();
		if(ServiceErr != 0)
		{
			*pRes = ServiceErr;
			return sizeof(INT8U);
		}
		MaxCd = g_coremacros.rmedia_multi_CD_Image_count;
		MaxFd = g_coremacros.rmedia_multi_FD_Image_count;
		MaxHd = g_coremacros.rmedia_multi_HD_Image_count;
	}
	else if(AppType == LOCAL_MEDIA_TYPE)
	{
		ServiceErr = LMediaServiceStatus();
		if(ServiceErr != 0)
		{
			*pRes = ServiceErr;
			return sizeof(INT8U);
		}
		MaxCd = g_coremacros.lmedia_multi_CD_Image_count;
		MaxFd = g_coremacros.lmedia_multi_FD_Image_count;
		MaxHd = g_coremacros.lmedia_multi_HD_Image_count;
	}
	else
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}
	if( (pAMIGetMediaInfoReq->Param <= MAX_REDIRECT_IMAGES) && (ReqLen != sizeof(INT16U)))
	{
		*pRes = CC_REQ_INV_LEN;
		return sizeof(*pRes);
	}
	MaxCount = MaxCd + MaxFd + MaxHd;
	pAMIGetMediaInfoRes->CompletionCode = CC_NORMAL;
	pAMIGetMediaInfoRes->AppType = AppType;

	switch(pAMIGetMediaInfoReq->Param)
	{
		case MEDIA_NUM_OF_IMAGES:
			if(AppType == LOCAL_MEDIA_TYPE)
				retval = GetlistImages(LMEDIA_IMG_LIST_FILE, &ToatalAvaliableImages,&MediaImageCfg);
			else if(AppType == REMOTE_MEDIA_TYPE)
				retval = RISGetAllImagesList(1, &ToatalAvaliableImages,&MediaImageCfg);
			if(retval != 0)
			{
				*pRes = (retval == 1)? CC_IMG_LIST_FILE_NOT_AVAIL: CC_UNSPECIFIED_ERR;
				if(MediaImageCfg != NULL)
					free(MediaImageCfg);
				return sizeof(INT8U);
			}
			pAMIGetMediaInfoRes->Num_of_images = ToatalAvaliableImages;
			if(MediaImageCfg != NULL)
				free(MediaImageCfg);
			break;
		case NUM_OF_CD_INSTANCES:
			pAMIGetMediaInfoRes->Num_of_images = MaxCd;
			break;
		case NUM_OF_FD_INSTANCES:
			pAMIGetMediaInfoRes->Num_of_images = MaxFd;
			break;
		case NUM_OF_HD_INSTANCES:
			pAMIGetMediaInfoRes->Num_of_images = MaxHd;
			break;
		 case MAX_REDIRECT_IMAGES:
			pAMIGetMediaInfoRes->Num_of_images = MaxCd + MaxFd + MaxHd;//MaxCount;
			break;
		case GET_REDIRECT_IMAGE_INFO:
		case GET_REDIRECTED_IMAGE_INFO:
			if(ReqLen != (sizeof(AMIGetMediaInfoReq_T)-1))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(*pRes);
			}
			Image_Type = pAMIGetMediaInfoReq->ImageType;

			switch(Image_Type)
			{
				case IMAGE_TYPE_CD:
					MaxCount = MaxCd;
					break;	
				case IMAGE_TYPE_FD:
					MaxCount = MaxFd;
					break;
				case IMAGE_TYPE_HD:
					MaxCount = MaxHd;
					break;
				case IMAGE_TYPE_ALL:
					MaxCount = MaxCd + MaxFd + MaxHd;
					break;
				default:
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
			}
			LMediaCfg = (ImageCfg_T*) calloc(MaxCount,sizeof(ImageCfg_T));
			if(LMediaCfg == NULL)
			{
				*pRes = CC_UNSPECIFIED_ERR;
				return sizeof(INT8U);
			}
			memset(LMediaCfg,0,sizeof(ImageCfg_T)*MaxCount);
			if(Image_Type == IMAGE_TYPE_ALL)
			{
				if(GetImageCfg(LMediaCfg,AppType) != 0)
				{
					*pRes = CC_UNSPECIFIED_ERR;
					if(LMediaCfg != NULL)
						free(LMediaCfg);
					return sizeof(INT8U);
				}
			}
			else if(GetImageCfgBySec(AppType, LMediaCfg,Image_Type,MaxCount,ImageIndex) != 0)
			{
				*pRes = CC_UNSPECIFIED_ERR;
				if(LMediaCfg != NULL)
					free(LMediaCfg);
				return sizeof(INT8U);
			}
			RedirectedImageInfo = (RedirectedImageInfo_T*)(pRes+2);
			for(i=0;i<MaxCount;i++)
			{
				if (pAMIGetMediaInfoReq->Param == GET_REDIRECTED_IMAGE_INFO)
				{
					if(LMediaCfg[i].IsRedirected)
					{
						RedirectedImageInfo[j].IsRedirected = LMediaCfg[i].IsRedirected;
						RedirectedImageInfo[j].MediaType= LMediaCfg[i].MediaType;
						RedirectedImageInfo[j].MediaIndex= LMediaCfg[i].MediaIndex;
						RedirectedImageInfo[j].SessionIndex= LMediaCfg[i].SessionIndex;
						strncpy((char *)RedirectedImageInfo[j].ImgName,(char *)LMediaCfg[i].ImgName,MAX_IMG_PATH);
						j++;
					}
					else
						continue;
				}
				else
				{
					RedirectedImageInfo[i].IsRedirected = LMediaCfg[i].IsRedirected;
					RedirectedImageInfo[i].MediaType= LMediaCfg[i].MediaType;
					RedirectedImageInfo[i].MediaIndex= LMediaCfg[i].MediaIndex;
					RedirectedImageInfo[i].SessionIndex= LMediaCfg[i].SessionIndex;
					strncpy((char *)RedirectedImageInfo[i].ImgName,(char *)LMediaCfg[i].ImgName,MAX_IMG_PATH);
				}
			}
			if(LMediaCfg != NULL)
				free(LMediaCfg);
			if(pAMIGetMediaInfoReq->Param == GET_REDIRECTED_IMAGE_INFO)
			{
				if(j== 0)
				{
					*pRes = CC_NO_IMAGE_IS_REDIRECTED;
					return sizeof(INT8U);
				}
				return ((j*sizeof(RedirectedImageInfo_T)) + (sizeof(AMIGetMediaInfoRes_T) - sizeof(INT8U)));
			}
			return ((MaxCount*sizeof(RedirectedImageInfo_T)) + (sizeof(AMIGetMediaInfoRes_T) - sizeof(INT8U)));
		case GET_ALL_AVILABLE_IMAGE_INFO:
			if(ReqLen != (sizeof(AMIGetMediaInfoReq_T) - 1))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(*pRes);
			}
			ImageName_Format = pAMIGetMediaInfoReq->ImageType;

			if( (ImageName_Format != SHORT_IMG_NAME_FORMAT) &&(ImageName_Format != LONG_IMG_NAME_FORMAT) )
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U); 
			}

			if(AppType == LOCAL_MEDIA_TYPE)
				retval = GetlistImages(LMEDIA_IMG_LIST_FILE, &ToatalAvaliableImages,&MediaImageCfg);
			else if(AppType == REMOTE_MEDIA_TYPE)
				retval = RISGetAllImagesList(0, &ToatalAvaliableImages,&MediaImageCfg) ;
			if(retval != 0)
			{
				*pRes = (retval == 1)? CC_IMG_LIST_FILE_NOT_AVAIL: CC_UNSPECIFIED_ERR;
				if(MediaImageCfg != NULL)
					free(MediaImageCfg);
				return sizeof(INT8U);
			}
			pAMIGetMediaInfoRes->CompletionCode = CC_NORMAL;
			if(ImageName_Format == SHORT_IMG_NAME_FORMAT)
			{
				ShortImageInfo = (ShortImageInfo_T*)(pRes+2);
				for(ImageIndex=0;ImageIndex < ToatalAvaliableImages;ImageIndex++)
				{
					ShortImageInfo[ImageIndex].Index = MediaImageCfg[ImageIndex].Image_index;
					ShortImageInfo[ImageIndex].ImageType = MediaImageCfg[ImageIndex].Image_type;
					memset(ImageName,0,MAX_IMG_PATH);
					strncpy(ImageName,MediaImageCfg[ImageIndex].Image_name,MAX_IMG_PATH );
					len = strlen(ImageName);
					if(len > SHORT_IMG_NAME_LEN)
					{
						memset(tmpstr,0,SHORT_IMG_NAME_LEN+1);
						strncpy(tmpstr,ImageName,SHORT_IMG_NAME_LEN - sizeof(INT32U));
						strcat(tmpstr,&ImageName[len-sizeof(INT32U)]);
						strncpy((char *)ShortImageInfo[ImageIndex].ImageName,tmpstr, SHORT_IMG_NAME_LEN);
					}
					else
						strncpy((char *)ShortImageInfo[ImageIndex].ImageName,MediaImageCfg[ImageIndex].Image_name, SHORT_IMG_NAME_LEN);
				}
				if(MediaImageCfg != NULL)
					free(MediaImageCfg);
				return (ToatalAvaliableImages *sizeof(ShortImageInfo_T)+ (sizeof(AMIGetMediaInfoRes_T) - sizeof(INT8U)));
			}
			else if(ImageName_Format == LONG_IMG_NAME_FORMAT)
			{
				LongImageInfo = (LongImageInfo_T*)(pRes+2);

				for(ImageIndex=0;ImageIndex < ToatalAvaliableImages;ImageIndex++)
				{
					LongImageInfo[ImageIndex].Index = MediaImageCfg[ImageIndex].Image_index;
					LongImageInfo[ImageIndex].ImageType = MediaImageCfg[ImageIndex].Image_type;
					strncpy((char *)LongImageInfo[ImageIndex].ImageName,MediaImageCfg[ImageIndex].Image_name, REDIRECTED_IMAGE_LEN);
				}
				if(MediaImageCfg != NULL)
					free(MediaImageCfg); 
				return (ToatalAvaliableImages * sizeof(LongImageInfo_T)+ (sizeof(AMIGetMediaInfoRes_T) - sizeof(INT8U)));
			}
			break;
		case CLEAR_MEDIA_TYPE_INDEX_ERROR:
			if(ReqLen != sizeof(AMIGetMediaInfoReq_T))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(*pRes);
			}
			Image_Type = pAMIGetMediaInfoReq->ImageType;
			ImageIndex = pAMIGetMediaInfoReq->ImageIndex;
			MaxCount = 1;
			LMediaCfg = (ImageCfg_T*) malloc(sizeof(ImageCfg_T));
			if(LMediaCfg == NULL)
			{
				*pRes = CC_UNSPECIFIED_ERR;
				return sizeof(INT8U);
			}
			memset(LMediaCfg,0,sizeof(ImageCfg_T));
			if(GetImageCfgBySec(AppType, LMediaCfg,Image_Type,MaxCount,ImageIndex) != 0)
			{
				*pRes = CC_UNSPECIFIED_ERR;
				return sizeof(INT8U);
			}

			if(LMediaCfg->IsRedirected ==1)
			{
				*pRes = ERR_REDIR_RUNNING;
				if(LMediaCfg != NULL)
					free(LMediaCfg);
				return sizeof(INT8U);
			}

			retval = ClearImgSection(AppType,LMediaCfg) ;
			if(retval != 0)
			{
				*pRes = retval;
				if(LMediaCfg != NULL)
					free(LMediaCfg);
				return sizeof(INT8U);
			}

			if(LMediaCfg != NULL)
				free(LMediaCfg);

			UpdateImageList( AppType, Image_Type);
			*pRes = CC_NORMAL ;
			return (sizeof(AMIGetMediaInfoRes_T) - sizeof(INT8U));
			break;
		default:
			*pRes = CC_PARAM_NOT_SUPPORTED;
			return sizeof(INT8U);
	}
	return sizeof(AMIGetMediaInfoRes_T);
}

int
AMISetMediaInfo( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
	AMISetMediaInfoReq_T*  pAMISetMediaInfoReq = (AMISetMediaInfoReq_T*) pReq;
	AMISetMediaInfoRes_T*  pAMISetMediaInfoRes = (AMISetMediaInfoRes_T*) pRes;
	INT8U ServiceErr = -1;
	int AppType = -1, ImageType = 0,MaxCount = 0, ImageIndex = -1;
	char *ImageName = NULL;
	ImageCfg_T *LMediaCfg = NULL;
	int RetVal = -1;

	AppType = pAMISetMediaInfoReq->AppType;

	if(AppType == REMOTE_MEDIA_TYPE)
	{
		ServiceErr = RISServiceStatus();
		if(ServiceErr != 0)
		{
			*pRes = ServiceErr;
			return sizeof(INT8U);
		}
	}
	else if(AppType == LOCAL_MEDIA_TYPE)
	{
		ServiceErr = LMediaServiceStatus();
		if(ServiceErr != 0)
		{
			*pRes = ServiceErr;
			return sizeof(INT8U);
		}
	}
	else
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(INT8U);
	}
	switch(pAMISetMediaInfoReq->Param)
	{
		case ADD_MEDIA_IMAGE:
			*pRes = CC_PARAM_NOT_SUPPORTED;
			return sizeof(INT8U);
		case DELETE_MEDIA_IMAGE:
			if(ReqLen < (sizeof(AMISetMediaInfoReq_T) - sizeof(SetOperations_T) + sizeof(INT8U)))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(INT8U);
			}
			if(pAMISetMediaInfoReq->Ops.Delete.ImageIndex == 0xFF)
			{
				ImageName = (char *)malloc(REDIRECTED_IMAGE_LEN*sizeof(INT8U));
				if(ImageName == NULL)
				{
					*pRes = CC_UNSPECIFIED_ERR;
					return sizeof(INT8U);
				}
				memset(ImageName,0,REDIRECTED_IMAGE_LEN);
				strncpy(ImageName,(char *)pAMISetMediaInfoReq->Ops.Delete.ImageName,REDIRECTED_IMAGE_LEN);
				ImageIndex = -1;
			}
			if((DeleteLMediaImage(ImageIndex, ImageName)) != 0)
			{
				*pRes = CC_UNSPECIFIED_ERR;
				if(ImageName != NULL)
					free(ImageName);
				return sizeof(INT8U);
			}
			if(ImageName != NULL)
				free(ImageName);
			break;
		case UPDATE_MEDIA_IMAGE_LIST:
			if(AppType == RMEDIA)
			{
				if(ReqLen != (sizeof(AMISetMediaInfoReq_T) - sizeof(SetOperations_T) + sizeof(Update_T)))
				{
					*pRes = CC_REQ_INV_LEN;
					return sizeof(INT8U);
				}
				ImageType = pAMISetMediaInfoReq->Ops.Update.ImageType;
				switch(ImageType)
				{
					case IMAGE_TYPE_CD:
					case IMAGE_TYPE_FD:
					case IMAGE_TYPE_HD:
						break;
					default:
						*pRes = CC_INV_DATA_FIELD;
						return sizeof(INT8U);
				}
			}
			else
			{
				if(ReqLen != (sizeof(AMISetMediaInfoReq_T) - sizeof(SetOperations_T) ))
				{
					*pRes = CC_REQ_INV_LEN;
					return sizeof(INT8U);
				}
				ImageType = IMAGE_TYPE_ALL;
			}

			if (UpdateImageList( AppType, ImageType) != 0 )
			{
				*pRes = CC_UNSPECIFIED_ERR;
				return sizeof(INT8U);
			}
			break;
		case CLEAR_MEDIA_IMAGE_ERROR:
			if(ReqLen != (sizeof(AMISetMediaInfoReq_T) -sizeof(SetOperations_T) + sizeof(Clear_T)))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(INT8U);
			}
			ImageType = pAMISetMediaInfoReq->Ops.Clear.ImageType;
			ImageIndex = pAMISetMediaInfoReq->Ops.Clear.ImageIndex;
			MaxCount = 1;

			LMediaCfg = (ImageCfg_T*) malloc(sizeof(ImageCfg_T));
			if(LMediaCfg == NULL)
			{
				*pRes = CC_UNSPECIFIED_ERR;
				return sizeof(INT8U);
			}
			memset(LMediaCfg,0,sizeof(ImageCfg_T));
			if(GetImageCfgBySec(AppType, LMediaCfg,ImageType,MaxCount,ImageIndex) != 0)
			{
				*pRes = CC_UNSPECIFIED_ERR;
				if(LMediaCfg != NULL)
					free(LMediaCfg);
				return sizeof(INT8U);
			}

			if(LMediaCfg->IsRedirected == 1)
			{
				*pRes = ERR_REDIR_RUNNING;
				if(LMediaCfg != NULL)
					free(LMediaCfg);
				return sizeof(INT8U);
			}

			RetVal = ClearImgSection(AppType,LMediaCfg) ;
			if(RetVal != 0)
			{
				*pRes = RetVal;
				if(LMediaCfg != NULL)
					free(LMediaCfg);
				return sizeof(INT8U);
			}
			if(LMediaCfg != NULL)
				free(LMediaCfg);

			UpdateImageList( AppType, ImageType);
			break;
		default:
			*pRes = CC_PARAM_NOT_SUPPORTED;
			return sizeof(INT8U);
	}
	pAMISetMediaInfoRes->CompletionCode = CC_SUCCESS;
	return sizeof(AMISetMediaInfoRes_T);
}

int AMIGetChannelType(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst)
{
    AMIGetChannelTypeReq_T *pAMIGetChTypeReq = (AMIGetChannelTypeReq_T*)pReq;
    AMIGetChannelTypeRes_T *pAMIGetChTypeRes = (AMIGetChannelTypeRes_T*)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
      ChannelInfo_T*  pChannelInfo;
    
    if((pAMIGetChTypeReq->ChannelNumber & 0xF0) || (CURRENT_CHANNEL_NUM == pAMIGetChTypeReq->ChannelNumber))
    {
        pAMIGetChTypeRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pChannelInfo = getChannelInfo(pAMIGetChTypeReq->ChannelNumber, BMCInst);
    if (NULL == pChannelInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        pAMIGetChTypeRes->CompletionCode = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    pAMIGetChTypeRes->CompletionCode = CC_NORMAL;
    pAMIGetChTypeRes->ChannelType = pChannelInfo->ChannelType;
    pAMIGetChTypeRes->ChannelNumber = pAMIGetChTypeReq->ChannelNumber;

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    return sizeof (AMIGetChannelTypeRes_T);
}


int AMIGetRemoteKVMCfg (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	AMIGetRemoteKVMCfgReq_T *pAMIGetRemoteKVMCfgReq = (AMIGetRemoteKVMCfgReq_T *)pReq;
	AMIGetRemoteKVMCfgRes_T *pAMIGetRemoteKVMCfgRes = (AMIGetRemoteKVMCfgRes_T *)pRes;
	AdviserConfig_T AdviserCfg;
	int retval,hostlockfeaturestatus,Runtime_Singleport_Status= 0;
	if(pAMIGetRemoteKVMCfgReq->ParameterSelect == KVM_ENCRYPTION)
	{
		if( (g_corefeatures.single_port_app == ENABLED) && (g_corefeatures.runtime_singleport_support != ENABLED) )
		{
			pAMIGetRemoteKVMCfgRes->CompletionCode = CC_INV_CMD;
			return sizeof(INT8U);
		}
		if( (g_corefeatures.runtime_singleport_support == ENABLED) && (g_PDKSinglePortHandle[PDKSINGLEPORT_ISFEATUREENABLED] != NULL) )
		{
			Runtime_Singleport_Status = ((int(*)(void ))g_PDKSinglePortHandle[PDKSINGLEPORT_ISFEATUREENABLED])();
			if(Runtime_Singleport_Status == ENABLED)
			{
				*pRes = CC_RUN_TIME_SINGLE_PORT_STATUS_ENABLED;
				return sizeof(INT8U);
			}
		} 
		if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_GETADVISERSECURESTATUS] == NULL)
		{
			pAMIGetRemoteKVMCfgRes->CompletionCode = CC_INV_CMD;
			return sizeof(INT8U);
		}
		retval = ((int(*)(void))g_PDKRemoteKVMHandle[PDKREMOTEKVM_GETADVISERSECURESTATUS])( );
		if((retval != 0) && (retval != 1))
		{
			pAMIGetRemoteKVMCfgRes->CompletionCode = CC_UNSPECIFIED_ERR;
			return sizeof(INT8U);
		}
		pAMIGetRemoteKVMCfgRes->CompletionCode = CC_NORMAL;
		pAMIGetRemoteKVMCfgRes->RemoteKVMCfg.secure_status = (INT8U)retval;
		return sizeof(AMIGetRemoteKVMCfgRes_T)-2;
	}
	else
	{
		if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_GETADVISERCFG] == NULL)
		{
			*pRes = CC_INV_CMD;
			return sizeof(INT8U);
		}
		retval = ((int(*)(AdviserConfig_T *))g_PDKRemoteKVMHandle[PDKREMOTEKVM_GETADVISERCFG])( &AdviserCfg );

		if(retval != 0)
		{
			TCRIT("Error in Getting the Remote KVM Configurations\n");
			*pRes = CC_INV_CMD;
			return sizeof(*pRes);
		}

		memset(pAMIGetRemoteKVMCfgRes,0,sizeof(AMIGetRemoteKVMCfgRes_T ));

		pAMIGetRemoteKVMCfgRes->CompletionCode = CC_NORMAL;

		switch(pAMIGetRemoteKVMCfgReq->ParameterSelect)
		{
		 case KVM_MOUSE_MODE:
			pAMIGetRemoteKVMCfgRes->RemoteKVMCfg.Mouse_Mode = AdviserCfg.Mouse_Mode;
			return sizeof(AMIGetRemoteKVMCfgRes_T)-2;
			break;
			
		 case KVM_RETRY_COUNT:
			if (g_corefeatures.kvm_reconnect_support != ENABLED)
			{
				pAMIGetRemoteKVMCfgRes->CompletionCode = CC_FEATURE_NOT_ENABLED;
				return sizeof(*pRes);
			}
		 	pAMIGetRemoteKVMCfgRes->RemoteKVMCfg.retry_count = AdviserCfg.retry_count;
		 	return sizeof(AMIGetRemoteKVMCfgRes_T)-2;
		 	break;

		 case KVM_RETRY_INTERVAL:
			if (g_corefeatures.kvm_reconnect_support != ENABLED)
			{
				pAMIGetRemoteKVMCfgRes->CompletionCode = CC_FEATURE_NOT_ENABLED;
				return sizeof(*pRes);
			}
			pAMIGetRemoteKVMCfgRes->RemoteKVMCfg.retry_interval = AdviserCfg.retry_interval;
			return sizeof(AMIGetRemoteKVMCfgRes_T)-2;
			break;

		 case KVM_KBD_LAYOUT:
			if (g_corefeatures.key_board_language_select != ENABLED)
			{
				pAMIGetRemoteKVMCfgRes->CompletionCode = CC_FEATURE_NOT_ENABLED;
				return sizeof(*pRes);
			}
			strncpy((char *)pAMIGetRemoteKVMCfgRes->RemoteKVMCfg.keyboard_layout,(char *)AdviserCfg.keyboard_layout,KEYBRD_LANG_SIZE-1);
			return sizeof(AMIGetRemoteKVMCfgRes_T)-1;
			break;

		 case KVM_HOST_LOCK_STATUS:
			if (g_corefeatures.host_lock_feature != ENABLED)
			{
				pAMIGetRemoteKVMCfgRes->CompletionCode = CC_FEATURE_NOT_ENABLED;
				return sizeof(*pRes);
			}
			hostlockfeaturestatus = ((INT8U )AdviserCfg.hostlock_value & (1<<0));
			pAMIGetRemoteKVMCfgRes->RemoteKVMCfg.hostlock_feature_status = hostlockfeaturestatus;

			return sizeof(AMIGetRemoteKVMCfgRes_T)-2;
			break;

		 case KVM_AUTO_LOCK_STATUS:
			if(g_corefeatures.host_auto_lock != ENABLED)
			{
				*pRes = CC_INV_CMD;
				return sizeof (INT8U);
			}
			if ( (AdviserCfg.hostlock_value & (1<<1)) == 1)
			{
				pAMIGetRemoteKVMCfgRes->RemoteKVMCfg.auto_lock_status = 1;
			}
			else
			{
				pAMIGetRemoteKVMCfgRes->RemoteKVMCfg.auto_lock_status = 0;
			}
			return sizeof(AMIGetRemoteKVMCfgRes_T)-2;

		 default:
			pAMIGetRemoteKVMCfgRes->CompletionCode = CC_INV_PARAM;
			return sizeof(*pRes);
			break;
		}
	}
	return sizeof(*pRes);
}

int AMISetRemoteKVMCfg (INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	AMISetRemoteKVMCfgReq_T *pAMISetRemoteKVMCfgReq = (AMISetRemoteKVMCfgReq_T *)pReq;
	AMISetRemoteKVMCfgRes_T *pAMISetRemoteKVMCfgRes = (AMISetRemoteKVMCfgRes_T *)pRes;
	AdviserConfig_T AdviserCfg;
	char Temp[KEYBRD_LANG_SIZE];
	int  retval,hostlockvalue = 0;
	int hostlock_feature_status = 0,prev_autolock_status = 0,prev_secure_status,Runtime_Singleport_Status = 0;
	if(pAMISetRemoteKVMCfgReq->ParameterSelect == KVM_ENCRYPTION)
	{
		if( (g_corefeatures.single_port_app == ENABLED) && (g_corefeatures.runtime_singleport_support != ENABLED) )
		{
			pAMISetRemoteKVMCfgRes->CompletionCode = CC_INV_CMD;
			return sizeof(INT8U);
		}
		if( (g_corefeatures.runtime_singleport_support == ENABLED) && (g_PDKSinglePortHandle[PDKSINGLEPORT_ISFEATUREENABLED] != NULL) )
		{
			Runtime_Singleport_Status = ((int(*)(void ))g_PDKSinglePortHandle[PDKSINGLEPORT_ISFEATUREENABLED])();
			if((Runtime_Singleport_Status == ENABLED) && (pAMISetRemoteKVMCfgReq->RemoteKVMcfg.secure_status == ENABLED))
			{
				*pRes = CC_RUN_TIME_SINGLE_PORT_STATUS_ENABLED;
				return sizeof(INT8U);
			}
		} 
		if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_GETADVISERSECURESTATUS] == NULL)
		{
			pAMISetRemoteKVMCfgRes->CompletionCode = CC_INV_CMD;
			return sizeof(INT8U);
		}
		if(ReqLen != 2)
		{
			*pRes = CC_REQ_INV_LEN;
			return sizeof(INT8U);
		}
		if((pAMISetRemoteKVMCfgReq->RemoteKVMcfg.secure_status & 0xFE) != 0)
		{
			*pRes = CC_INV_DATA_FIELD;
			return sizeof(*pRes);
		}

		retval = ((int(*)(void))g_PDKRemoteKVMHandle[PDKREMOTEKVM_GETADVISERSECURESTATUS])( );
		if(retval < 0)
		{
			pAMISetRemoteKVMCfgRes->CompletionCode = CC_UNSPECIFIED_ERR;
			return sizeof(INT8U);
		}
		prev_secure_status = retval;

		if(pAMISetRemoteKVMCfgReq->RemoteKVMcfg.secure_status == prev_secure_status)
		{
			*pRes = CC_SUCCESS;
			return sizeof(INT8U);
		}
		if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_SETADVISERSECURESTATUS] == NULL)
		{
			pAMISetRemoteKVMCfgRes->CompletionCode = CC_INV_CMD;
			return sizeof(INT8U);
		}
		retval = ((int(*)(int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_SETADVISERSECURESTATUS])(pAMISetRemoteKVMCfgReq->RemoteKVMcfg.secure_status );
		if(retval<0)
		{
			TCRIT("Error in Setting the KVM Secure Status\n");
			*pRes = CC_UNSPECIFIED_ERR;
			return sizeof(INT8U);
		}
	}
	else
	{
		if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_GETADVISERCFG] == NULL)
		{
			*pRes = CC_INV_CMD;
			return sizeof(*pRes);
		}
		retval = ((int(*)(AdviserConfig_T *))g_PDKRemoteKVMHandle[PDKREMOTEKVM_GETADVISERCFG])( &AdviserCfg );
		if(retval != 0)
		{
			TCRIT("Error in Getting the Remote KVM Configurations\n");
			*pRes = CC_UNSPECIFIED_ERR;
			return sizeof(*pRes);
		}
		pAMISetRemoteKVMCfgRes->CompletionCode = CC_NORMAL;

		switch(pAMISetRemoteKVMCfgReq->ParameterSelect)
		{

		case KVM_MOUSE_MODE:
			if(ReqLen != (sizeof(AMISetRemoteKVMCfgReq_T) - 2))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(*pRes);
			}
			if((pAMISetRemoteKVMCfgReq->RemoteKVMcfg.Mouse_Mode <= 0) ||(pAMISetRemoteKVMCfgReq->RemoteKVMcfg.Mouse_Mode > 0x03))
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(*pRes);
			}

			if(pAMISetRemoteKVMCfgReq->RemoteKVMcfg.Mouse_Mode == AdviserCfg.Mouse_Mode)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}
			else
				AdviserCfg.Mouse_Mode = pAMISetRemoteKVMCfgReq->RemoteKVMcfg.Mouse_Mode;
			break;

		case KVM_RETRY_COUNT:
			if(ReqLen != (sizeof(AMISetRemoteKVMCfgReq_T) - 2))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(*pRes);
			}

			if (g_corefeatures.kvm_reconnect_support != ENABLED)
			{
				pAMISetRemoteKVMCfgRes->CompletionCode = CC_FEATURE_NOT_ENABLED;
				return sizeof(*pRes);
			}

			if((pAMISetRemoteKVMCfgReq->RemoteKVMcfg.retry_count < MIN_KVM_RECONNECT_COUNT) ||(pAMISetRemoteKVMCfgReq->RemoteKVMcfg.retry_count > MAX_KVM_RECONNECT_COUNT))
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(*pRes);
			}

			AdviserCfg.retry_count = pAMISetRemoteKVMCfgReq->RemoteKVMcfg.retry_count;
			break;
		
		case KVM_RETRY_INTERVAL:
			if(ReqLen != (sizeof(AMISetRemoteKVMCfgReq_T) - 2))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(*pRes);
			}
			
			if (g_corefeatures.kvm_reconnect_support != ENABLED)
			{
				pAMISetRemoteKVMCfgRes->CompletionCode = CC_FEATURE_NOT_ENABLED;
				return sizeof(*pRes);
			}
			
			if((pAMISetRemoteKVMCfgReq->RemoteKVMcfg.retry_interval < MIN_KVM_RECONNECT_INTERVAL) ||(pAMISetRemoteKVMCfgReq->RemoteKVMcfg.retry_interval > MAX_KVM_RECONNECT_INTERVAL))
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(*pRes);
			}
			
			AdviserCfg.retry_interval = pAMISetRemoteKVMCfgReq->RemoteKVMcfg.retry_interval;
			break;

		case KVM_KBD_LAYOUT:
			if(ReqLen != KEYBRD_LANG_SIZE )
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(*pRes);
			}

			snprintf(Temp,KEYBRD_LANG_SIZE,"%s",pAMISetRemoteKVMCfgReq->RemoteKVMcfg.keyboard_layout);

			if (g_corefeatures.key_board_language_select != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(*pRes);
			}
			if (strncmp((char *)AdviserCfg.keyboard_layout,Temp,KEYBRD_LANG_SIZE-1) == 0)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}

			if (((int(*)(char *))g_PDKRemoteKVMHandle[PDKREMOTEKVM_VALIDKBDLANG])(Temp) == TRUE) // check to validate the input Kbd_language string
			{
				strncpy((char *)AdviserCfg.keyboard_layout,Temp,KEYBRD_LANG_SIZE-1);
			}
			else
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(*pRes);
			}
			break;

		case KVM_HOST_LOCK_STATUS:
			if(ReqLen != (sizeof(AMISetRemoteKVMCfgReq_T) - 2))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(*pRes);
			}
			// Check Host_Lock Feature is Enabled or Not in the PRJ
			if (g_corefeatures.host_lock_feature != ENABLED)
			{
				pAMISetRemoteKVMCfgRes->CompletionCode = CC_FEATURE_NOT_ENABLED;
				return sizeof(*pRes);
			}

			hostlockvalue = AdviserCfg.hostlock_value;

			hostlock_feature_status = hostlockvalue & (1<<0);

			//check incoming lockstatus to same as existing
			if(hostlock_feature_status == pAMISetRemoteKVMCfgReq->RemoteKVMcfg.hostlock_feature_status)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}

			if (pAMISetRemoteKVMCfgReq->RemoteKVMcfg.hostlock_feature_status == 1)
			{
				hostlockvalue |= (1<<0); // setting 0th bit for Host_lock Feature Enable
			}
			else if(pAMISetRemoteKVMCfgReq->RemoteKVMcfg.hostlock_feature_status == 0)
			{
				hostlockvalue &= ~(1<<0); // resetting 0th bit for Host_Lock feature Disable
			}
			else
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(*pRes);
			}
			AdviserCfg.hostlock_value = hostlockvalue;
			break;

		case KVM_AUTO_LOCK_STATUS:
			if(ReqLen != (sizeof(AMISetRemoteKVMCfgReq_T) - 2))
			{
				*pRes = CC_REQ_INV_LEN;
				return sizeof(*pRes);
			}

			if(g_corefeatures.host_auto_lock != ENABLED)
			{
				*pRes = CC_INV_CMD;
				return sizeof (INT8U);
			}

			// check incoming lockstatus to lock/unlock
			if ( (HOSTLOCK_CMD !=pAMISetRemoteKVMCfgReq->RemoteKVMcfg.auto_lock_status) && (HOSTUNLOCK_CMD !=pAMISetRemoteKVMCfgReq->RemoteKVMcfg.auto_lock_status) )
			{
				*pRes = CC_INV_LOCK_CMD;
				return sizeof (INT8U);
			}
			prev_autolock_status = hostlockvalue & (1<<1);

			// check incoming lockstatus to same as existing
			if ( prev_autolock_status == pAMISetRemoteKVMCfgReq->RemoteKVMcfg.auto_lock_status)
			{
				*pRes = CC_SUCCESS;
				return sizeof (INT8U);
			}

			if(pAMISetRemoteKVMCfgReq->RemoteKVMcfg.auto_lock_status == 1)
			{
				hostlockvalue |= (1<<1);
			}
			else if(pAMISetRemoteKVMCfgReq->RemoteKVMcfg.auto_lock_status == 0)
			{
				hostlockvalue &= ~(1 << 1);
			}
			else
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof (INT8U);
			}
			AdviserCfg.hostlock_value = hostlockvalue;
			break;

		default:
			pAMISetRemoteKVMCfgRes->CompletionCode = CC_INV_PARAM;
			return sizeof(*pRes);
		}

		if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_SETADVISERCFG] == NULL)
		{
			*pRes = CC_UNSPECIFIED_ERR;
			return sizeof(INT8U);
		}
		retval = ((int(*)(AdviserConfig_T *, int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_SETADVISERCFG])( &AdviserCfg, 1 );

		if(retval != 0)
		{
			TCRIT("Error in Setting Adviser configuration::%d\n", retval);
			*pRes = CC_UNSPECIFIED_ERR;
			return sizeof(*pRes);
		}
}
	pAMISetRemoteKVMCfgRes->CompletionCode = CC_NORMAL;
	return sizeof(*pRes);
}

int AMIGetSSLCertStatus(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	  AMIGetSSLCertStatusReq_T *pAMIGetSSLCertStatusReq = (AMIGetSSLCertStatusReq_T *)pReq;
	   AMIGetSSLCertStatusRes_T *pAMIGetSSLCertStatusRes = (AMIGetSSLCertStatusRes_T *)pRes;
	   char Temp[MAX_FILE_INFO_SIZE] = "Not Available";

	   struct stat CertStat, PrivkeyStat;
	   INT8U CertExists = 0;

	   if(pAMIGetSSLCertStatusReq->Param > 0x02)
	   {
		   *pRes = CC_INV_PARAM;
		   return sizeof(INT8U);
	   }
	   memset(pAMIGetSSLCertStatusRes,0,sizeof(AMIGetSSLCertStatusRes_T));

	   pAMIGetSSLCertStatusRes->CompletionCode = CC_NORMAL;

		if((stat(ACTUAL_CERTIFICATE_FILE, &CertStat) == 0) || (stat(DEFAULT_CERTIFICATE_FILE,&CertStat) == 0))
		{
			// user certificate available 
			CertExists = 1;
		}

		switch(pAMIGetSSLCertStatusReq->Param)
		{
		case CERTIFICATE_STATUS:
			pAMIGetSSLCertStatusRes->SSLCertInfo.Status = CertExists;
			return (sizeof(AMIGetSSLCertStatusRes_T) - sizeof(SSLCertInfo_T)+1);

		case CERIFICATE_INFO:
			if(CertExists)
			{
				sprintf((char *)pAMIGetSSLCertStatusRes->SSLCertInfo.CertInfo,"%s",ctime(&CertStat.st_mtime));
			}
			else
				strncpy((char *)pAMIGetSSLCertStatusRes->SSLCertInfo.CertInfo,Temp,strlen(Temp));
			return sizeof(AMIGetSSLCertStatusRes_T);

		case PRIVATE_KEY_INFO:
		if((stat(ACTUAL_PRIVATE_KEY_FILE, &PrivkeyStat) == 0) || (stat( DEFAULT_PRIVATE_KEY_FILE,&PrivkeyStat) == 0))
		{
			//user private key available 
			sprintf((char *)pAMIGetSSLCertStatusRes->SSLCertInfo.PrivateKeyInfo, "%s", ctime(&PrivkeyStat.st_mtime));
		}
		else 
			strncpy((char *)pAMIGetSSLCertStatusRes->SSLCertInfo.PrivateKeyInfo,Temp,strlen(Temp));
		return sizeof(AMIGetSSLCertStatusRes_T);
		}

		return sizeof(INT8U);
}

int AMIGetVmediaCfg(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	AMIGetVmediaCfgReq_T *pAMIGetVmediaCfgReq = (AMIGetVmediaCfgReq_T *)pReq;
	AMIGetVmediaCfgRes_T *pAMIGetVmediaCfgRes = (AMIGetVmediaCfgRes_T *)pRes;
	int retval,Runtime_Singleport_Status= 0;
	VMediaCfg_T VMediaCfg;

	if(pAMIGetVmediaCfgReq->Param == VMEDIA_ENCRYPTION)
	{
		if( (g_corefeatures.single_port_app == ENABLED) && (g_corefeatures.runtime_singleport_support != ENABLED) )
		{
			*pRes = CC_INV_CMD;
			return sizeof(INT8U);
		}
		if( (g_corefeatures.runtime_singleport_support == ENABLED) && (g_PDKSinglePortHandle[PDKSINGLEPORT_ISFEATUREENABLED] != NULL) )
		{
			Runtime_Singleport_Status = ((int(*)(void ))g_PDKSinglePortHandle[PDKSINGLEPORT_ISFEATUREENABLED])();
			if(Runtime_Singleport_Status == ENABLED)
			{
				*pRes = CC_RUN_TIME_SINGLE_PORT_STATUS_ENABLED;
				return sizeof(INT8U);
			}
		}
		if(g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIASECURESTATUS] == NULL)
		{
			*pRes = CC_INV_CMD;
			return sizeof(INT8U);
		}
		retval = ((int(*)(void))g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIASECURESTATUS])();
		if(retval<0)
		{
			*pRes = CC_UNSPECIFIED_ERR;
			return sizeof(INT8U);
		}
		pAMIGetVmediaCfgRes->VMediaConfig.secure_status = retval;
		return sizeof(AMIGetVmediaCfgRes_T);
	}
	else
	{
		if(g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIACFG] == NULL)
		{
			*pRes = CC_INV_CMD;
			return sizeof(INT8U);
		}
		retval = ((int(*)(VMediaCfg_T *,int,int,int))g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIACFG])( &VMediaCfg,g_corefeatures.lmedia_support, g_corefeatures.rmedia_support,g_corefeatures.power_consumption_virtual_device_usb);
		if(retval != 0)
		{
			TCRIT("Error in Getting the Vmedia Configuration Command :: retval = %x\n",retval);
			*pRes = CC_INV_CMD;
			return sizeof(INT8U);
		}

		pAMIGetVmediaCfgRes->CompletionCode = CC_NORMAL;

		switch(pAMIGetVmediaCfgReq->Param)
		{
		case VMEDIA_CD_ATTACH_MODE:
			
			pAMIGetVmediaCfgRes->VMediaConfig.attach_cd = VMediaCfg.attach_cd;
			return sizeof(AMIGetVmediaCfgRes_T);

		case VMEDIA_FD_ATTACH_MODE:
			
			pAMIGetVmediaCfgRes->VMediaConfig.attach_fd = VMediaCfg.attach_fd;
			return sizeof(AMIGetVmediaCfgRes_T);

		case VMEDIA_HD_ATTACH_MODE:
			
			pAMIGetVmediaCfgRes->VMediaConfig.attach_hd = VMediaCfg.attach_hd;
			return sizeof(AMIGetVmediaCfgRes_T);

		case VMEDIA_ENABLE_BOOT_ONCE:
			if( g_corefeatures.vmedia_enable_boot_once != ENABLED )
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			
			pAMIGetVmediaCfgRes->VMediaConfig.enable_boot_once = VMediaCfg.enable_boot_once;
			return sizeof(AMIGetVmediaCfgRes_T);

		case VMEDIA_NUM_CD:
			
			pAMIGetVmediaCfgRes->VMediaConfig.num_cd = VMediaCfg.num_cd;
			return sizeof(AMIGetVmediaCfgRes_T);

		case VMEDIA_NUM_FD:
			
			pAMIGetVmediaCfgRes->VMediaConfig.num_fd = VMediaCfg.num_fd;
			return sizeof(AMIGetVmediaCfgRes_T);

		case VMEDIA_NUM_HD:
			
			pAMIGetVmediaCfgRes->VMediaConfig.num_hd = VMediaCfg.num_hd;
			return sizeof(AMIGetVmediaCfgRes_T);

		case VMEDIA_LMEDIA_ENABLE_STATUS:
			if(g_corefeatures.lmedia_support != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(*pRes);
			}
			pAMIGetVmediaCfgRes->VMediaConfig.lmedia_enable = VMediaCfg.lmedia_enable;
			return sizeof(AMIGetVmediaCfgRes_T);

		case VMEDIA_RMEDIA_ENABLE_STATUS:
			if(g_corefeatures.rmedia_support != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			pAMIGetVmediaCfgRes->VMediaConfig.rmedia_enable = VMediaCfg.rmedia_enable;
			return sizeof(AMIGetVmediaCfgRes_T);

		case VMEDIA_SDMEDIA_ENABLE_STATUS: 
			if(g_corefeatures.sd_server_support != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			pAMIGetVmediaCfgRes->VMediaConfig.sdmedia_enable = VMediaCfg.sdmedia_enable;
			return sizeof(AMIGetVmediaCfgRes_T);
		case KVM_NUM_CD: 
			if(g_corefeatures.kvm_media_count != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			pAMIGetVmediaCfgRes->VMediaConfig.kvm_cd= VMediaCfg.kvm_cd;
			return sizeof(AMIGetVmediaCfgRes_T);
		case KVM_NUM_FD:
			if(g_corefeatures.kvm_media_count != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			pAMIGetVmediaCfgRes->VMediaConfig.kvm_fd = VMediaCfg.kvm_fd;
			return sizeof(AMIGetVmediaCfgRes_T);
		case KVM_NUM_HD:
			if(g_corefeatures.kvm_media_count != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			pAMIGetVmediaCfgRes->VMediaConfig.kvm_hd = VMediaCfg.kvm_hd;
			return sizeof(AMIGetVmediaCfgRes_T);
		default:
			*pRes = CC_INV_PARAM;
			return sizeof(INT8U);
		}
	}
	return sizeof(AMIGetVmediaCfgRes_T);
}


#define RESTART_NOT_REQ	0
#define VM_RESTART_REQ		0x001
#define LMEDIA_RESTART_REQ	0x010
#define RMEDIA_RESTART_REQ	0x011
#define ADVISER_CONF_RESTART_REQ	0x100

int FindChangeToRestart(VMediaCfg_T VMediaCfg)
{
	int restart = RESTART_NOT_REQ;
	int retval = 0;
	VMediaCfg_T OldVMediaCfg;

	if(g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIASECURESTATUS] == NULL)
	{
		return VM_RESTART_REQ;
	}
	retval = ((int(*)(VMediaCfg_T *,int,int,int))g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIACFG])( &OldVMediaCfg,g_corefeatures.lmedia_support, g_corefeatures.rmedia_support,g_corefeatures.power_consumption_virtual_device_usb);
	if(retval != 0)
	{
		TCRIT("Error in Getting the Vmedia Configuration Command :: retval = %x\n",retval);
		return VM_RESTART_REQ;
	}

	if( (VMediaCfg.attach_cd != OldVMediaCfg.attach_cd) ||
		(VMediaCfg.attach_fd != OldVMediaCfg.attach_fd) ||
		(VMediaCfg.attach_hd != OldVMediaCfg.attach_hd) ||
		(VMediaCfg.enable_boot_once != OldVMediaCfg.enable_boot_once) ||
		(VMediaCfg.num_cd != OldVMediaCfg.num_cd) ||
		(VMediaCfg.num_fd != OldVMediaCfg.num_fd) ||
		(VMediaCfg.num_hd != OldVMediaCfg.num_hd) ||
		(VMediaCfg.sdmedia_enable != OldVMediaCfg.sdmedia_enable))
	{
		restart = VM_RESTART_REQ;
	}

	//If dedicated media instance feature is enabled restart vmapp
	if (VMediaCfg.lmedia_enable != OldVMediaCfg.lmedia_enable) 
		restart |=(g_corefeatures.dedicated_device_lmedia_rmedia) ? VM_RESTART_REQ : LMEDIA_RESTART_REQ  ;
	if (VMediaCfg.rmedia_enable != OldVMediaCfg.rmedia_enable) 
		restart |= (g_corefeatures.dedicated_device_lmedia_rmedia) ? VM_RESTART_REQ : RMEDIA_RESTART_REQ;

	if( (VMediaCfg.kvm_cd != OldVMediaCfg.kvm_cd) ||
		(VMediaCfg.kvm_fd != OldVMediaCfg.kvm_fd) ||
		(VMediaCfg.kvm_hd != OldVMediaCfg.kvm_hd))
	{
		restart |= (g_corefeatures.kvm_media_count) ? ADVISER_CONF_RESTART_REQ : RESTART_NOT_REQ;
	}

	return restart;

}

/*
 @fn CheckMediaRedirectionRunningState
 @brief This function is used to check whether any media redirection running or not. 
 @params [in] - nothing
 @params [in]  -1 : on any error
                0 : media redirection state idle 
                1 : media redirection running.
 */

int CheckMediaRedirectionRunningState()
{
    session_info_record_t	*session_tbl;
    session_info_header_t	hdr;
    uint32	total_sessions=0;
    void *dl_handle = NULL;
    int (*GetSessionCount)(session_info_record_t **,session_info_header_t *,uint32 *);	
    int i=0;

    //no session management feature follow old behavior 
    if(g_corefeatures.session_management != ENABLED)
        return 0;
	
    dl_handle = dlopen(SESMNGT_LIB, RTLD_NOW);
    if(dl_handle == NULL)
    {
    	TDBG("CheckMediaRedirectionRunningState unable to access library %s \n",SESMNGT_LIB);    			
    	return 0;
    }

    GetSessionCount = dlsym(dl_handle,"racsessinfo_getallrecords");

    if(GetSessionCount)
    {
    	if( GetSessionCount(&session_tbl, &hdr, &total_sessions) != 0 ) 
    	{
    		return -1;
    	}    	
    }	
    		
	if(total_sessions > 0)
	{
		//if any active sessions present check for media sessions
		for(i=0;i<total_sessions;i++)
		{			
			if( session_tbl[i].session_type == SESSION_TYPE_VMEDIA_CD || 
				session_tbl[i].session_type == SESSION_TYPE_VMEDIA_LOCAL_CD ||
				session_tbl[i].session_type == SESSION_TYPE_VMEDIA_FD || 
				session_tbl[i].session_type == SESSION_TYPE_VMEDIA_LOCAL_FD ||
				session_tbl[i].session_type == SESSION_TYPE_VMEDIA_HD || 
				session_tbl[i].session_type == SESSION_TYPE_VMEDIA_LOCAL_HD
			  )
			{				
				free(session_tbl);
				return 1;
			}
		}
		free(session_tbl);
	}
	
return 0;
}


int AMISetVmediaCfg(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	AMISetVmediaCfgReq_T *pAMISetVmediaCfgReq = (AMISetVmediaCfgReq_T *)pReq;
	AMISetVmediaCfgRes_T *pAMISetVmediaCfgRes = (AMISetVmediaCfgRes_T *)pRes;
	INT8U num_cd = 0,num_fd = 0,num_hd = 0;
	static INT8U InitializeVmediaCfg = TRUE;
	int retval = 0,prev_secure_status,Runtime_Singleport_Status= 0;
	VMediaCfg_T VMediaCfg;
	int restart = 0,type =-1,redirectionState=0;

	if(pAMISetVmediaCfgReq->Param == VMEDIA_ENCRYPTION)
	{
		if( (g_corefeatures.single_port_app == ENABLED) && (g_corefeatures.runtime_singleport_support != ENABLED) )
		{
			*pRes = CC_INV_CMD;
			return sizeof(INT8U);
		}
		if( (g_corefeatures.runtime_singleport_support == ENABLED) && (g_PDKSinglePortHandle[PDKSINGLEPORT_ISFEATUREENABLED] != NULL) )
		{
			Runtime_Singleport_Status = ((int(*)(void ))g_PDKSinglePortHandle[PDKSINGLEPORT_ISFEATUREENABLED])();
			if((Runtime_Singleport_Status == ENABLED) && (pAMISetVmediaCfgReq->VMediaConfig.secure_status == ENABLED))
			{
				*pRes = CC_RUN_TIME_SINGLE_PORT_STATUS_ENABLED;
				return sizeof(INT8U);
			}
		}
		if(g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIASECURESTATUS] == NULL)
		{
			*pRes = CC_INV_CMD;
			return sizeof(INT8U);
		}
		prev_secure_status = ((int(*)(void))g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIASECURESTATUS])();
		if(retval<0)
		{
			*pRes = CC_UNSPECIFIED_ERR;
			return sizeof(INT8U);
		}
		if(pAMISetVmediaCfgReq->VMediaConfig.secure_status == prev_secure_status)
		{
			*pRes = CC_SUCCESS;
			return sizeof(INT8U);
		}
		if(g_PDKVMediaHandle[PDKVMEDIA_SETVMEDIASECURESTATUS] == NULL)
		{
			*pRes = CC_INV_CMD;
			return sizeof(INT8U);
		}
		retval = ((int(*)(int))g_PDKVMediaHandle[PDKVMEDIA_SETVMEDIASECURESTATUS])(pAMISetVmediaCfgReq->VMediaConfig.secure_status);
		if(retval<0)
		{
			TCRIT("Error in Setting the Vmedia Secure Status\n");
			*pRes = CC_UNSPECIFIED_ERR;
			return sizeof(INT8U);
		}
	}
	else
	{
                redirectionState = CheckMediaRedirectionRunningState();
                if(redirectionState == -1)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                else if(redirectionState == 1)
                {
                       *pRes = CC_MEDIA_REDIRECTION_IN_PROGRESS;
                       return sizeof(INT8U);
                }

		if(InitializeVmediaCfg == TRUE)
		{
			if(g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIACFG] == NULL)
			{
				*pRes = CC_INV_CMD;
				return sizeof(INT8U);
			}
			retval = ((int(*)(VMediaCfg_T *,int,int,int))g_PDKVMediaHandle[PDKVMEDIA_GETVMEDIACFG])( &gVMediaCfg,g_corefeatures.lmedia_support, g_corefeatures.rmedia_support,g_corefeatures.power_consumption_virtual_device_usb);			if(retval != 0)
			{
				TCRIT("Error in Getting the Vmedia Configuration Command :: retval = %x\n",retval);
				*pRes = CC_INV_CMD;
				return sizeof(INT8U);
			}
			InitializeVmediaCfg = FALSE;
		}
		switch(pAMISetVmediaCfgReq->Param)
		{
		case VMEDIA_CD_ATTACH_MODE:
			if (gVMediaCfg.attach_cd == pAMISetVmediaCfgReq->VMediaConfig.attach_cd)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}

			if (((pAMISetVmediaCfgReq->VMediaConfig.attach_cd) & 0xFE) != 0x00)
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}
			gVMediaCfg.attach_cd = pAMISetVmediaCfgReq->VMediaConfig.attach_cd;
			break;
		case VMEDIA_FD_ATTACH_MODE:
			if (gVMediaCfg.attach_fd == pAMISetVmediaCfgReq->VMediaConfig.attach_fd)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}

			if (((pAMISetVmediaCfgReq->VMediaConfig.attach_fd) & 0xFE) != 0x00)
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}
			gVMediaCfg.attach_fd = pAMISetVmediaCfgReq->VMediaConfig.attach_fd;
			break;
		case VMEDIA_HD_ATTACH_MODE:
			if (gVMediaCfg.attach_hd == pAMISetVmediaCfgReq->VMediaConfig.attach_hd)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}

			if (((pAMISetVmediaCfgReq->VMediaConfig.attach_hd) & 0xFE) != 0x00)
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}
			gVMediaCfg.attach_hd = pAMISetVmediaCfgReq->VMediaConfig.attach_hd;
			break;
		case VMEDIA_ENABLE_BOOT_ONCE:
			if( g_corefeatures.vmedia_enable_boot_once != ENABLED )
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			if (((pAMISetVmediaCfgReq->VMediaConfig.enable_boot_once) & 0xFE) != 0x00)
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}

			if (gVMediaCfg.enable_boot_once == pAMISetVmediaCfgReq->VMediaConfig.enable_boot_once)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}
			gVMediaCfg.enable_boot_once = pAMISetVmediaCfgReq->VMediaConfig.enable_boot_once;
			break;
		case VMEDIA_NUM_CD:		
			num_cd = pAMISetVmediaCfgReq->VMediaConfig.num_cd;
			if(g_corefeatures.dedicated_device_lmedia_rmedia == ENABLED) //if Feature is enabled
			{
				if((num_cd != 0x01) && (num_cd != 0x02)) //  Allow the num of CD Instances either 1 or 2
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			else
			{
				if((num_cd > 0x04) || (num_cd < 0x00)) // Allow the num of CD Instances from  0 to 4
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			if (gVMediaCfg.num_cd == pAMISetVmediaCfgReq->VMediaConfig.num_cd)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}
			gVMediaCfg.num_cd = pAMISetVmediaCfgReq->VMediaConfig.num_cd;

			if(g_corefeatures.kvm_media_count == ENABLED)
			{
				//if KVM  device value is greater then VM device do the below
				if(gVMediaCfg.kvm_cd > gVMediaCfg.num_cd)
					gVMediaCfg.kvm_cd = gVMediaCfg.num_cd ;
			}
			break;
		case VMEDIA_NUM_FD:
			
			num_fd = pAMISetVmediaCfgReq->VMediaConfig.num_fd;
			if(g_corefeatures.dedicated_device_lmedia_rmedia == ENABLED) //If feature is enabled.
			{
				if((num_fd != 0x01) && (num_fd != 0x02))  // Allow the num of FD Instances either 1 or 2
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			else
			{
				if((num_fd > 0x04) || (num_fd < 0x00)) // Allow the num of FD Instances from 0 to 4
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			if (gVMediaCfg.num_fd == pAMISetVmediaCfgReq->VMediaConfig.num_fd)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}
			gVMediaCfg.num_fd = pAMISetVmediaCfgReq->VMediaConfig.num_fd;

			if(g_corefeatures.kvm_media_count == ENABLED)
			{
				//if KVM  device value is greater then VM device do the below
				if(gVMediaCfg.kvm_fd > gVMediaCfg.num_fd)
					gVMediaCfg.kvm_fd = gVMediaCfg.num_fd ;
			}
			break;
		case VMEDIA_NUM_HD:
			
			num_hd = pAMISetVmediaCfgReq->VMediaConfig.num_hd;
			if(g_corefeatures.dedicated_device_lmedia_rmedia == ENABLED) // If this Feature is Enabled
			{
				if((num_hd != 0x01) && (num_hd != 0x02)) // Allow the num of HD Instances either 1 or 2
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			else
			{
				if((num_hd > 0x04) || (num_hd < 0x00)) // Allow the num of HD Instances from 0 to 4
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			if (gVMediaCfg.num_hd == pAMISetVmediaCfgReq->VMediaConfig.num_hd)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}
			gVMediaCfg.num_hd = pAMISetVmediaCfgReq->VMediaConfig.num_hd;

			if(g_corefeatures.kvm_media_count == ENABLED)
			{
				//if KVM  device value is greater then VM device do the below
				if(gVMediaCfg.kvm_hd > gVMediaCfg.num_hd)
					gVMediaCfg.kvm_hd = gVMediaCfg.num_hd ;
			}
			break;
		case VMEDIA_LMEDIA_ENABLE_STATUS:
			if (g_corefeatures.lmedia_support != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			if (gVMediaCfg.lmedia_enable == pAMISetVmediaCfgReq->VMediaConfig.lmedia_enable)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}

			if (((pAMISetVmediaCfgReq->VMediaConfig.lmedia_enable) & 0xFE) != 0x00)
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}
			gVMediaCfg.lmedia_enable = pAMISetVmediaCfgReq->VMediaConfig.lmedia_enable;
			break;
		case VMEDIA_RMEDIA_ENABLE_STATUS:
			if (g_corefeatures.rmedia_support != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			if (gVMediaCfg.rmedia_enable == pAMISetVmediaCfgReq->VMediaConfig.rmedia_enable)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}

			if (((pAMISetVmediaCfgReq->VMediaConfig.rmedia_enable) & 0xFE) != 0x00)
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}
			gVMediaCfg.rmedia_enable = pAMISetVmediaCfgReq->VMediaConfig.rmedia_enable;
			break;
		case VMEDIA_SDMEDIA_ENABLE_STATUS:
			if (g_corefeatures.sd_server_support != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}
			if (gVMediaCfg.sdmedia_enable == pAMISetVmediaCfgReq->VMediaConfig.sdmedia_enable)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}
			if (((pAMISetVmediaCfgReq->VMediaConfig.sdmedia_enable) & 0xFE) != 0x00)
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}
			gVMediaCfg.sdmedia_enable = pAMISetVmediaCfgReq->VMediaConfig.sdmedia_enable;
			break;
		case VMEDIA_RESTART:
			if (pAMISetVmediaCfgReq->VMediaConfig.Vmedia_restart != 0x01)
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}
			memcpy(&VMediaCfg,&gVMediaCfg, sizeof(VMediaCfg_T));

			restart = FindChangeToRestart(VMediaCfg);

			if(( VM_RESTART_REQ == (restart & VM_RESTART_REQ)) ||
				( LMEDIA_RESTART_REQ == (restart & LMEDIA_RESTART_REQ)))
			{
				if((ENABLED == g_corefeatures.lmedia_support) && (ENABLED == g_corefeatures.lmedia_medium_type_sd))
				{
					if(ENABLED == VMediaCfg.lmedia_enable)
					{
						((int(*)())g_PDKVMediaHandle[PDKVMEDIA_SETSDMOUNTSTATE])(MOUNT_IN_PROGRESS);
					}
				}
			}

			if( VM_RESTART_REQ == (restart & VM_RESTART_REQ))
			{
				if(g_PDKVMediaHandle[PDKVMEDIA_SETVMEDIACFG] == NULL)
				{
					*pRes = CC_INV_CMD;
					return sizeof(INT8U);
				}
				retval = ((int(*)(VMediaCfg_T *, int,int,int))g_PDKVMediaHandle[PDKVMEDIA_SETVMEDIACFG])( &VMediaCfg,g_corefeatures.lmedia_support, g_corefeatures.rmedia_support,g_corefeatures.power_consumption_virtual_device_usb);
			}
			else if(( LMEDIA_RESTART_REQ == (restart & LMEDIA_RESTART_REQ)) ||
				( RMEDIA_RESTART_REQ == (restart & RMEDIA_RESTART_REQ)))
			{
				if(g_PDKVMediaHandle[PDKVMEDIA_SETLMEDIARMEDIASTATE] == NULL)
				{
					*pRes = CC_INV_CMD;
					return sizeof(INT8U);
				}

				if(( LMEDIA_RESTART_REQ == (restart & LMEDIA_RESTART_REQ)) &&
				( RMEDIA_RESTART_REQ == (restart & RMEDIA_RESTART_REQ)))
					type = LMEDIA_RMEDIA;
				else if( LMEDIA_RESTART_REQ == (restart & LMEDIA_RESTART_REQ))
					type = LMEDIA;
				else if( RMEDIA_RESTART_REQ == (restart & RMEDIA_RESTART_REQ))
					type = RMEDIA;
				retval = ((int(*)(int,VMediaCfg_T ))g_PDKVMediaHandle[PDKVMEDIA_SETLMEDIARMEDIASTATE])( type,VMediaCfg);
			}
			else if( ADVISER_CONF_RESTART_REQ == (restart & ADVISER_CONF_RESTART_REQ))
			{

				if(g_PDKVMediaHandle[PDKVMEDIA_SETKVMDEVICECOUNT] == NULL)
				{
					*pRes = CC_INV_CMD;
					return sizeof(INT8U);
				}
				retval = ((int(*)(VMediaCfg_T *))g_PDKVMediaHandle[PDKVMEDIA_SETKVMDEVICECOUNT])( &VMediaCfg);
			}
			else
			{
				retval = 0;
			}
			if(retval != 0)
			{
				TCRIT("Error in Setting the Vmedia Configurations :: retval = %x\n",retval);
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}
			InitializeVmediaCfg  = TRUE;
			break;
		case KVM_NUM_CD:

			if(g_corefeatures.kvm_media_count != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}

			num_cd = pAMISetVmediaCfgReq->VMediaConfig.kvm_cd;
			if(num_cd > gVMediaCfg.num_cd)// should not be greater then num_cd
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}

			if(g_corefeatures.dedicated_device_lmedia_rmedia == ENABLED) // If this Feature is Enabled
			{
				if((num_cd != 0x01) && (num_cd != 0x02)) // Allow the num of HD Instances either 1 or 2
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			else
			{
				if((num_cd > 0x04) || (num_cd < 0x00)) // Allow the num of HD Instances from 0 to 4
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			if (gVMediaCfg.kvm_cd == pAMISetVmediaCfgReq->VMediaConfig.kvm_cd)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}
			gVMediaCfg.kvm_cd = pAMISetVmediaCfgReq->VMediaConfig.kvm_cd;
			break;
		case KVM_NUM_FD:

			if(g_corefeatures.kvm_media_count != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}

			num_fd = pAMISetVmediaCfgReq->VMediaConfig.kvm_fd;
			if(num_fd > gVMediaCfg.num_fd) // should not be greater  then num_fd
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}

			if(g_corefeatures.dedicated_device_lmedia_rmedia == ENABLED) // If this Feature is Enabled
			{
				if((num_fd != 0x01) && (num_fd != 0x02)) // Allow the num of HD Instances either 1 or 2
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			else
			{
				if((num_fd > 0x04) || (num_fd < 0x00)) // Allow the num of HD Instances from 0 to 4
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			if (gVMediaCfg.kvm_fd == pAMISetVmediaCfgReq->VMediaConfig.kvm_fd)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}
			gVMediaCfg.kvm_fd = pAMISetVmediaCfgReq->VMediaConfig.kvm_fd;
			break;
		case KVM_NUM_HD:

			if(g_corefeatures.kvm_media_count != ENABLED)
			{
				*pRes = CC_FEATURE_NOT_ENABLED;
				return sizeof(INT8U);
			}

			num_hd = pAMISetVmediaCfgReq->VMediaConfig.kvm_hd;
			if(num_hd > gVMediaCfg.num_hd) //should not be greater then num_hd
			{
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}

			if(g_corefeatures.dedicated_device_lmedia_rmedia == ENABLED) // If this Feature is Enabled
			{
				if((num_hd != 0x01) && (num_hd != 0x02)) // Allow the num of HD Instances either 1 or 2
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			else
			{
				if((num_hd > 0x04) || (num_hd < 0x00)) // Allow the num of HD Instances from 0 to 4
				{
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
			if (gVMediaCfg.kvm_hd == pAMISetVmediaCfgReq->VMediaConfig.kvm_hd)
			{
				*pRes = CC_SUCCESS;
				return sizeof(INT8U);
			}
			gVMediaCfg.kvm_hd = pAMISetVmediaCfgReq->VMediaConfig.kvm_hd;
			break;
		default:
			*pRes = CC_INV_PARAM;
			return sizeof(INT8U);
		}
	}
	pAMISetVmediaCfgRes->CompletionCode = CC_NORMAL;
	return sizeof(AMISetVmediaCfgRes_T);
}

int AMIRestartWebService(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	AMIRestartWebServiceRes_T  *pAMIRestartWebServiceRes = (AMIRestartWebServiceRes_T *)pRes;
	int retval = 0;
	RestartService_T Service;
	INT8U curchannel;

	Service.ServiceName = WEBSERVER;
	SetPendStatus(PEND_OP_RESTART_SERVICES, PEND_STATUS_PENDING);	
	OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
	PostPendTask(PEND_OP_RESTART_SERVICES, (INT8U *) &Service, 1, curchannel & 0xF,BMCInst);

	pAMIRestartWebServiceRes->Enable = retval;
	pAMIRestartWebServiceRes->CompletionCode = CC_NORMAL;
	return sizeof(INT8U);
}

int AMIGetBMCInstanceCount(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	AMIGetBMCInstanceCountRes_T  *pAMIGetBMCInstanceCountRes = (AMIGetBMCInstanceCountRes_T *)pRes;

#if defined(CONFIG_SPX_FEATURE_BMCCOMPANIONDEVICE_AST1070)
	int BMCInstCount = 0;
	BMCInstCount = GetBMCConfInfo();
	if( -1 != BMCInstCount )
	{
		pAMIGetBMCInstanceCountRes->CompletionCode = CC_NORMAL;
		pAMIGetBMCInstanceCountRes->BMCInstanceCount = BMCInstCount;
	}
	else
		pAMIGetBMCInstanceCountRes->CompletionCode = CC_NODE_BUSY;
#else
	pAMIGetBMCInstanceCountRes->CompletionCode = CC_OP_NOT_SUPPORTED;
#endif
	return sizeof(AMIGetBMCInstanceCountRes_T);
}

int AMIGetUSBSwitchSetting(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	AMIGetUSBSwitchSettingRes_T  *pAMIGetUSBSwitchSettingRes = (AMIGetUSBSwitchSettingRes_T *)pRes;

#if defined(CONFIG_SPX_FEATURE_BMCCOMPANIONDEVICE_AST1070)
	int retval = 0;
	if( g_HALUSBSwitchSettingHandle[HAL_USBSWITCHSETTING_GET_SET] != NULL )
	{
		retval = ((int(*)(unsigned char, unsigned char *))g_HALUSBSwitchSettingHandle[HAL_USBSWITCHSETTING_GET_SET]) (0, (unsigned char *)&(pAMIGetUSBSwitchSettingRes->USBSwitchSetting));
		if(retval != 0)
		{
			TCRIT("Error in Getting the USB Switch Setting :: retval = %x\n",retval);
			pAMIGetUSBSwitchSettingRes->CompletionCode = retval;
		}
		else
			pAMIGetUSBSwitchSettingRes->CompletionCode = CC_NORMAL;
	}
	else
		pAMIGetUSBSwitchSettingRes->CompletionCode = CC_OP_NOT_SUPPORTED;
#else
	pAMIGetUSBSwitchSettingRes->CompletionCode = CC_OP_NOT_SUPPORTED;
#endif

	return sizeof(AMIGetUSBSwitchSettingRes_T);
}

int AMISetUSBSwitchSetting(INT8U *pReq,INT32U ReqLen, INT8U* pRes,int BMCInst)
{
	AMISetUSBSwitchSettingRes_T  *pAMISetUSBSwitchSettingRes = (AMISetUSBSwitchSettingRes_T *)pRes;

#if defined(CONFIG_SPX_FEATURE_BMCCOMPANIONDEVICE_AST1070)
	int retval = 0;
	AMISetUSBSwitchSettingReq_T  *pAMISetUSBSwitchSettingReq = (AMISetUSBSwitchSettingReq_T *)pReq;
	
	if( g_HALUSBSwitchSettingHandle[HAL_USBSWITCHSETTING_GET_SET] != NULL )
	{
		retval = ((int(*)(unsigned char, unsigned char *))g_HALUSBSwitchSettingHandle[HAL_USBSWITCHSETTING_GET_SET]) (1, (unsigned char *)&(pAMISetUSBSwitchSettingReq->USBSwitchSetting));
		if(retval != 0)
		{
			TCRIT("Error in Setting the USB Switch Setting :: retval = %x\n",retval);
			pAMISetUSBSwitchSettingRes->CompletionCode = retval;
		}
		else
			pAMISetUSBSwitchSettingRes->CompletionCode = CC_NORMAL;
	}
	else
		pAMISetUSBSwitchSettingRes->CompletionCode = CC_OP_NOT_SUPPORTED;
#else
	pAMISetUSBSwitchSettingRes->CompletionCode = CC_OP_NOT_SUPPORTED;
#endif

	return sizeof(AMISetUSBSwitchSettingRes_T);
}

int AMISwitchMUX( INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
        AMISwitchMUXReq_T *pSwitchMUXReq = (AMISwitchMUXReq_T*)pReq;
        INT8U currSession;

        OS_THREAD_TLS_GET(g_tls.CurSessionType, currSession);
        if(UDS_SESSION_TYPE != currSession)
        {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
        }
        if(MUX_2_RAC < pSwitchMUXReq->direction)
        {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
        }

        if(g_PDKHandle[PDK_SWITCHEMPMUX] != NULL)
        {
                ((void(*)(INT8U,int))g_PDKHandle[PDK_SWITCHEMPMUX]) (pSwitchMUXReq->direction,BMCInst);
        }

        BMC_GET_SHARED_MEM (BMCInst)->SerialMUXMode = pSwitchMUXReq->direction;

        *pRes = CC_NORMAL;
        return sizeof(INT8U);
}
