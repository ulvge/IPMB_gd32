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
* IPMI_AMIConf.h
* AMI specific configuration commands
*
* Author: Benson Chuang <bensonchuang@ami.com.tw>
*
******************************************************************/

#ifndef __IPMI_AMICONF_H__
#define __IPMI_AMICONF_H__

#include <Types.h>
//#include "nwcfg.h"
#include "Iptables.h"
//#include "IPMI_LANConfig.h"
#include "IPMI_AppDevice.h"
#include "IPMI_SDRRecord.h"



/* Network Interface EnableState */
#define  DISABLE_V4_V6 0x00
#define  ENABLE_V4     0x01
#define  ENABLE_V6     0x02
#define  ENABLE_V4_V6  0x03

#define  GET_CMD_MODE  0x00 
#define  SET_CMD_MODE  0x01 

#define AMI_IFACE_STATE_ETH             0x00
#define AMI_IFACE_STATE_BOND           0x01
#define AMI_IFACE_BOND_ENABLED      0x02
#define AMI_GET_IFACE_COUNT             0x03
#define AMI_GET_IFACE_CHANNEL           0x04
#define AMI_GET_IFACE_NAME              0x05
#define AMI_BOND_ACTIVE_SLAVE           0x06
#define AMI_BOND_VLAN_ENABLED           0x07
#define AMI_CURR_ACTIVE_IFACE_COUNT		0x08
#define MAX_SERVICES 			8
#define SERVICE_NAME_SIZE 16
#define MAX_LAN_CHANNEL             0x05
#define MAX_IFACE_NAME                16
#define FULL_SEL_ENTRIES     0xFF
#define PARTIAL_SEL_ENTRIES   0x00
#define SEL_EMPTY_REPOSITORY 0x00
#define THRESHOLD_RESERVED_BIT  0xC0
#define DISCRETE_RESERVED_BIT   0x80


#define AMI_DNS_CONF_HOST_NAME              0x01
#define AMI_DNS_CONF_REGISTER               0x02
#define AMI_DNS_CONF_DOMAIN_SETTINGS        0x03
#define AMI_DNS_CONF_DOMAIN_NAME            0x04
#define AMI_DNS_CONF_DNS_SETTING            0x05
#define AMI_DNS_CONF_DNS_IP                 0x06
#define AMI_DNS_CONF_DNS_RESTART            0x07
#define AMI_DNS_CONF_TSIG_UPLOAD            0x08
#define AMI_DNS_CONF_DNS_ENABLE             0x09

#define MAX_DNS_IP_ADDRESS                            0x03
#define MAX_DOMAIN_BLOCK_SIZE                      64
#define MAX_BLOCK                                               0x04

#define MAX_ROLE_GROUPS 5
#define PRIV_LEVEL_NO_ACCESS 0x0F
#define MAX_LDAP_IP_ADRRESS 256
#define MAX_LDAP_PASSWORD 48
#define MAX_LDAP_BIND_DN 64
#define MAX_LDAP_SEARCH_BASE 128
#define MAX_LDAP_ATTR_USER 8
#define MAX_GRP_NAME_DOMAIN_LEN 256
#define MAX_ACCESS_LEVEL 5
#define MAX_BLOCK_LEN   64
#define MAX_AD_DOMAIN_LEN       256
#define MAX_AD_FILTER_LEN       256
#define MAX_AD_USERNAME_LEN     65
#define MAX_AD_PASSWORD_LEN     128
#define MAX_AD_ROLE_NAME_LEN    256
#define MAX_AD_ROLE_DOMAIN_LEN  256
#define MOUNT_FAIL -4

#pragma pack (1)

#define MAX_BOND_IFACE_SLAVES       4
#define DEFAULT_MII_INTERVAL        100

//Host loack feature
#define HOSTUNLOCK_CMD        0x00
#define HOSTLOCK_CMD            0x01
#define   HLENABLED       0x1
#define   HLDISABLED     0x0


/*
 * Service related struct
 */
#define MAX_SERVICE_IFACE_NAME_SIZE 16

#define PECI_DATA_SIZE 20
#define FEEDBACK_LEN 3

#define KVM_MOUSE_MODE		0x00
#define KVM_KBD_LAYOUT			0x01
#define KVM_HOST_LOCK_STATUS	0x02
#define KVM_AUTO_LOCK_STATUS	0x03
#define KVM_ENCRYPTION		0x04
#define KEYBRD_LANG_SIZE	0x03
#define KVM_RETRY_COUNT		0X05
#define KVM_RETRY_INTERVAL		0X06


#define RIS_IMAGE_NAME	0
#define RIS_REMOTE_PATH		1
#define RIS_IP_ADDR		2
#define RIS_USER_NAME	3
#define RIS_PASSWORD	4
#define RIS_SHARE_TYPE	5
#define RIS_DOMAIN	6
#define RIS_START_MOUNT		7
#define RIS_MOUNT_STATUS	8
#define RIS_ERR_CODE		9
#define RIS_STATE	10
#define RIS_RESTART 11
#define RIS_READY 12
#define RIS_CLR_CONFIGS  13

// Media Redirection

#define MEDIA_NUM_OF_IMAGES		0x00
#define NUM_OF_CD_INSTANCES		0x01
#define NUM_OF_FD_INSTANCES		0x02
#define NUM_OF_HD_INSTANCES		0x03
#define MAX_REDIRECT_IMAGES		0x04
#define GET_REDIRECT_IMAGE_INFO	0x05
#define GET_REDIRECTED_IMAGE_INFO 0x06
#define GET_ALL_AVILABLE_IMAGE_INFO	0x07
#define CLEAR_MEDIA_TYPE_INDEX_ERROR 0x08

#define ADD_MEDIA_IMAGE          0x00
#define DELETE_MEDIA_IMAGE       0x01
#define UPDATE_MEDIA_IMAGE_LIST  0x02
#define CLEAR_MEDIA_IMAGE_ERROR  0x03


#define SHORT_IMG_NAME_FORMAT   0x00
#define LONG_IMG_NAME_FORMAT   0x01

#define Local_Media_Type       0x00

#define IMAGE_TYPE_ALL     0x8

#define SHORT_IMG_NAME_LEN	20

#define BY_IMAGE_INDEX		0x00
#define BY_IMAGE_NAME		0x01
//Vmedia
#define VMEDIA_CD_ATTACH_MODE			0x00
#define VMEDIA_FD_ATTACH_MODE			0x01
#define VMEDIA_HD_ATTACH_MODE			0x02
#define VMEDIA_ENABLE_BOOT_ONCE			0x03
#define VMEDIA_NUM_CD					0x04
#define VMEDIA_NUM_FD					0x05
#define VMEDIA_NUM_HD					0x06
#define VMEDIA_LMEDIA_ENABLE_STATUS		0x07
#define VMEDIA_RMEDIA_ENABLE_STATUS		0x08
#define VMEDIA_SDMEDIA_ENABLE_STATUS	0x09
#define VMEDIA_RESTART					0x0a
#define VMEDIA_ENCRYPTION				0x0b
#define KVM_NUM_CD						0x0c
#define KVM_NUM_FD						0x0d
#define KVM_NUM_HD						0x0e

#define VMEDIA_REMOTE_ATTACH_MODE	0x00
#define VMEDIA_DEVICE_INSTANCE		0x01

// Get SSL Certificate status
#define CERTIFICATE_STATUS	0X00
#define CERIFICATE_INFO		0X01
#define PRIVATE_KEY_INFO	0X02

#define MAX_FILE_INFO_SIZE 126


#define DEFAULT_RADIUS_PORT 1812
#define DEFAULT_RADIUS_TIMEOUT 3
#define MAX_ACCESS_LEVEL 5
#define RADIUS_IPAddr_LEN 256
#define RADIUS_SECRET_LEN 32
#define RADIUS_VENDORData_LEN 128

//GET
typedef struct
{
    INT32U ServiceID;
} PACKED  AMIGetServiceConfReq_T;

typedef struct
{
    INT8U   CompletionCode;
    INT32U ServiceID;
    INT8U	 Enable;
           /* The following fields are meaningful only when the service is enabled */
    INT8S   InterfaceName[MAX_SERVICE_IFACE_NAME_SIZE + 1]; /* Interface name */
    INT32U  NonSecureAccessPort;               /* Non-secure access port number */
    INT32U  SecureAccessPort;                  /* Secure access port number */                        
    INTU    SessionInactivityTimeout;      	   /* Service session inactivity yimeout in seconds*/
    INT8U   MaxAllowSession;                   /* Maximum allowed simultaneous sessions */
    INT8U   CurrentActiveSession;              /* Number of current active sessions */
    INTU    MinSessionInactivityTimeout;	            /*Min Allowed value for Session timeout in sec*/
    INTU    MaxSessionInactivityTimeout;               /*Max Allowed value for Session timeout in sec*/
} PACKED  AMIGetServiceConfRes_T;

//SET
typedef struct
{
	INT32U ServiceID;
    INT8U	 Enable;
		    /* The following fields are meaningful only when the service is enabled */
	INT8S   InterfaceName[MAX_SERVICE_IFACE_NAME_SIZE + 1]; /* Interface name */
	INT32U  NonSecureAccessPort;               /* Non-secure access port number */
	INT32U  SecureAccessPort;                  /* Secure access port number */                        
	INTU    SessionInactivityTimeout;      	   /* Service session inactivity yimeout in seconds*/
	INT8U   MaxAllowSession;                   /* Maximum allowed simultaneous sessions */
	INT8U   CurrentActiveSession;              /* Number of current active sessions */
} PACKED  AMISetServiceConfReq_T;

typedef struct 
{
    INT8S   ServiceName [SERVICE_NAME_SIZE]; /* Service name */
    AMIGetServiceConfRes_T GetAllSeviceCfg;
}GetAllServiceConf_T;

typedef struct
{
    INT8U ServiceCnt;
    GetAllServiceConf_T ServiceInfo [MAX_SERVICES];
}GetServiceInfo_T;

typedef struct
{
    INT8U   CompletionCode;

} PACKED  AMISetServiceConfRes_T;


/*
 * DNS related struct
 */


typedef struct
{
    INT8U   DomainDHCP;
    INT8U   DomainIndex;
    INT8U   Domainpriority;
    INT8U   DomainLen;
}PACKED DomainSetting;

typedef struct
{
    INT8U   DNSDHCP;
    INT8U   DNSIndex;
    INT8U   IPPriority;
}PACKED DNSSetting;

//typedef union
//{
//    HOSTNAMECONF        HName;
//    INT8U               RegDNSConf[MAX_LAN_CHANNEL];
//    INT8U               PrivateKey[MAX_TSIG_PRIVKEY_SIZE];
//    DomainSetting       DomainConf;
//    INT8U               DomainName[MAX_DOMAIN_BLOCK_SIZE];
//    DNSSetting          DNSConf;
//    INT8U               DNSIPAddr[IP6_ADDR_LEN];
//    INT8U               DNSEnable;
//} DNSConf_T;

//typedef struct
//{
//    INT8U   ParamSelect;
//    INT8U   Blockselector;
//    DNSConf_T    DnsConfig;
//}PACKED AMISetDNSConfReq_T;

typedef struct
{
    INT8U   CompletionCode;
}PACKED AMISetDNSConfRes_T;

//typedef struct
//{
//    INT8U   CompletionCode;
//    DNSConf_T   DNSCfg;
//}PACKED AMIGetDNSConfRes_T;

typedef struct
{
    INT8U   Param;
    INT8U   Blockselect;
}PACKED AMIGetDNSConfReq_T;


/*
* LinkDown Resilent struct
*/
typedef struct
{
    INT8U LinkDownEnable;
}PACKED AMILinkDownResilentReq_T;

typedef struct
{
    INT8U CompletionCode;
    INT8U LinkEnableStatus;
}PACKED AMILinkDownResilentRes_T;

typedef struct
{
    INT8U   EthIndex;
    INT8U   EnableState;
    /* EnableState - Bit 0 is represented IPv4 and Bit 1 is represented IPv6,
     *               this byte will be ignored if the command is the "get" command.
     *                  DISABLE_V4_V6  (0x00) - Disable IPv4 and IPv6     
     *                  ENABLE_V4         (0x01) - Enable IPv4 only   
     *                  ENABLE_V6         (0x02) - Enable IPv6 only is not allowed 
     *                  ENABLE_V4_V6    (0x03) - Enable IPv4 and IPv6
     */
}PACKED EthIfaceState;

typedef struct
{
    INT8U   Count;
    INT8U   EthIndex[MAX_LAN_CHANNEL];
}PACKED LANIfcCount_T;

typedef struct
{
    INT8U   Channel;
}PACKED GetIfcChannel_T;

typedef struct
{
    char   IfcName[MAX_IFACE_NAME];
}PACKED GetIfcName_T;

typedef struct
{
    INT8U   Enabled;
    INT8U   BondIndex;
}PACKED BondEnabled_T;

typedef struct
{
    INT8U BondIndex;
    INT8U ActiveIndex;
}PACKED ActiveSlave_T;

typedef struct
{
    INT8U Enabled;
}PACKED BondVLAN_T;

//typedef union
//{
//    EthIfaceState   EthIface;
//    BondIface        BondIface;
//    BondEnabled_T   BondEnable;
//    LANIfcCount_T     LANCount;
//    GetIfcChannel_T     IfcChannel;
//    GetIfcName_T        IfcName;
//    ActiveSlave_T       ActiveSlave;
//    BondVLAN_T          BondVLAN;
//}PACKED IfaceConfigFn;

/*
 * Network Interface Enable/Disable struct
 */
//typedef struct
//{
//    INT8U   Params;
//    IfaceConfigFn   ConfigData;
//}PACKED AMISetIfaceStateReq_T;

typedef struct
{
    INT8U   Params;
    INT8U   SetSelect;
    INT8U   BlockSelect;
}PACKED AMIGetIfaceStateReq_T;

typedef struct
{
    INT8U   CompletionCode;
}PACKED AMISetIfaceStateRes_T;

//typedef struct
//{
//    INT8U   CompletionCode;
//    IfaceConfigFn   ConfigData;
// 
//}PACKED AMIGetIfaceStateRes_T;


/*---------------- Function Definitions for Firewall Command Implementation ---------------------*/ 

typedef struct
{
	INT8U Param;
	INT8U State;
	FirewallConfUn_T CMD_INFO;
		
} PACKED AMISetFirewallReq_T;

typedef struct
{
	INT8U CompletionCode;
	
} PACKED AMISetFirewallRes_T;


typedef struct
{
	INT8U Param;
	INT8U EntryNo;
	
} PACKED AMIGetFirewallReq_T;


typedef struct
{
	INT8U CompletionCode;
    
}PACKED GetFWCC_T;

typedef union
{
    INT8U TotalCount;
    INT8U IsBlockAll;
    GetFirewallConf_T Info;
    
} GetFirewallConfUn_T;

typedef struct
{
    GetFWCC_T    CCParam;
    GetFirewallConfUn_T FWInfo;

}PACKED AMIGetFirewallRes_T;

typedef struct
{
    INT8U   CompletionCode;
    INT8U   IPMISessionTimeOut;
}PACKED AMIGetIPMISessionTimeOutRes_T;

/*------------------------------- End of Firewall Command Declarations -------------------------*/

//SNMP:
/*
 * SNMP related struct
 */

typedef struct
{
    INT8U   CompletionCode;
    INT8U UserID;
    INT8U snmp_enable;                          
    INT8U snmp_access_type;                     
    INT8U snmp_enc_type_1;
    INT8U snmp_enc_type_2;
    
}PACKED AMIGetSNMPConfRes_T;

typedef struct
{
    INT8U UserID;
   
}PACKED AMIGetSNMPConfReq_T;

typedef struct
{
    INT8U UserID;
    INT8U snmp_enable;                          
    INT8U snmp_access_type;                     
    INT8U snmp_enc_type_1;
    INT8U snmp_enc_type_2;
}PACKED AMISetSNMPConfReq_T;

typedef struct
{
    INT8U   CompletionCode;
    
}PACKED AMISetSNMPConfRes_T;

/*
 * AMIGetSELPolicyRes_T
 */
typedef struct
{
    INT8U   CompletionCode;
    INT8U   SELPolicy;

} PACKED AMIGetSELPolicyRes_T;

/*
 * AMISetSELPolicyReq_T
 */
typedef struct
{
    INT8U   SELPolicy;

} PACKED AMISetSELPolicyReq_T;

/*
 * AMISetSELPolicyRes_T
 */
typedef struct
{
    INT8U   CompletionCode;

} PACKED AMISetSELPolicyRes_T;

typedef struct
{
    INT8U ServiceName;
    INT8U SleepSeconds;
}PACKED RestartService_T;

typedef struct
{
    INT32U Noofentretrieved;
}PACKED AMIGetSELEntriesReq_T;

typedef struct
{
    INT8U   CompletionCode;
    INT32U   Noofentries;
    INT16U   LastRecID;
    INT8U   Status;
}PACKED AMIGetSELEntriesRes_T;

typedef struct
{
    INT8U   CompletionCode;
    INT16U   Noofentries;
}PACKED AMIGetSensorInfoRes_T;

typedef struct
{
    SDRRecHdr_T    hdr;
    INT8U               OwnerID;
    INT8U               OwnerLUN;
    INT8U               SensorNumber;
    INT8U               SensorReading;
    INT8U               MaxReading;
    INT8U               MinReading;
    INT8U               Flags;
    INT8U               ComparisonStatus;
    INT8U               OptionalStatus;
    INT8U               SensorTypeCode;
    INT8U               EventTypeCode;
    INT8U               Units1;
    INT8U               Units2;
    INT8U               Units3;
    INT8U               Linearization;
    INT8U               M_LSB;
    INT8U               M_MSB_Tolerance;
    INT8U               B_LSB;
    INT8U               B_MSB_Accuracy;
    INT8U               Accuracy_MSB_Exp;
    INT8U               RExp_BExp;
    INT8U               LowerNonCritical;
    INT8U               LowerCritical;
    INT8U               LowerNonRecoverable;
    INT8U               UpperNonCritical;
    INT8U               UpperCritical;
    INT8U               UpperNonRecoverable;
    INT8U               AssertionEventByte1;
    INT8U               AssertionEventByte2;
    INT8U               DeassertionEventByte1;
    INT8U               DeassertionEventByte2;
    INT8S               SensorName[MAX_ID_STR_LEN];
    INT16U              Settable_Readable_ThreshMask;    

}PACKED SenInfo_T;

typedef struct
{
    INT8U SessionIPAddr[IP6_ADDR_LEN];
}PACKED AMIGetUDSInfoReq_T;

typedef struct
{
    INT8U CompletionCode;
    INT8U ChannelNum;
    INT8U ChannelType;
    INT8U BMCInstance;
}PACKED AMIGetUDSInfoRes_T;

typedef struct
{
    INT8U MaxAllowedSession;
    INT8U ActiveSessionCount;
}PACKED UDSSessionCount_T;

typedef struct
{
    INT32U LoggedInSessionID;
    INT8U LoggedInSessionHandle;
    INT8U LoggedInUserID;
    INT8U LoggedInPrivilege;
    INT32U LoggedInTime;
    INT8U UDSChannelNum;
    INT8U ChannelNum;
    INT32U SessionTimeoutValue;
    INT8U AuthenticationMechanism;
}PACKED UDSLoggedInSessionInfo_T;

typedef struct
{
    INT32U ProcessID;
    INT32U ThreadID;
} PACKED UDSSessionPIDInfo_T;

typedef union
{
    UDSSessionCount_T UDSSessionCountInfo;
    UDSLoggedInSessionInfo_T UDSLoggedInSessionInfo;
    UDSSessionPIDInfo_T UDSSessionPIDInfo;
}PACKED UDSLoggedInInfoUn_T;

typedef struct
{
    INT8U UDSSessionParam;
    INT8U UDSSessionHandleOrIDOrIndex[4];
} PACKED AMIGetUDSSessionInfoReq_T;

typedef struct
{
    INT8U CompletionCode;
    UDSLoggedInInfoUn_T UDSLoggedInInfo;
} PACKED AMIGetUDSSessionInfoRes_T;


/*
 * RIS related struct
 */
#define MAX_IMAGE_NAME_LEN		256
#define MAX_IMAGE_PATH_SIZE		256
#define MAX_IP_ADDR_LEN		63
#define MAX_RMS_USERNAME_LEN		256
#define MAX_RMS_PASSWORD_LEN		32
#define MAX_SHARE_TYPE_LEN		6
#define MAX_DOMAIN_LEN			256

#define REDIRECTED_IMAGE_LEN	128

//GET
typedef union 
{   
    INT8S   ImageName[MAX_IMAGE_NAME_LEN]; /* Image name */
    INT8S   MountPath[MAX_IMAGE_PATH_SIZE]; /* Mount Path */
    INT8S   RemoteIP[MAX_IP_ADDR_LEN+1]; /* Remote machine IP address */
    INT8S   UserName[MAX_RMS_USERNAME_LEN];/* Remote machine User Name */                        
    INT8S   Password[MAX_RMS_PASSWORD_LEN+1];	/* Remote machine Password*/
    INT8S   ShareType[MAX_SHARE_TYPE_LEN+1];/* Remote Share type*/
    INT8S   Domain[MAX_DOMAIN_LEN] ;    /* Remote machine Domain Name*/ 
    INT8U   ProgressBit;
    INT8U   RISstate;
    INT8U	Start_Mount;
    INT8U	Mount_Status;
    INT8U	Err_Code;
} RISConfig_T;

typedef struct
{
    INT8U MediaType;
    INT8U ParameterSelect;
} PACKED  AMIGetRISConfReq_T;

typedef struct
{
    INT8U   CompletionCode;
    INT8U   MediaType;
    INT8U   ParameterSelect;
    RISConfig_T Config;
} PACKED  AMIGetRISConfRes_T;

//SET
typedef struct
{
    INT8U MediaType;
    INT8U   ParameterSelect;
    INT8U   Blockselector;
    RISConfig_T  config;	
   
} PACKED  AMISetRISConfReq_T;

typedef struct
{
    INT8U   CompletionCode;

} PACKED  AMISetRISConfRes_T;

//START/STOP
typedef struct
{
    INT8U MediaType;
    INT8U   ParameterSelect;      
} PACKED  AMIRISStartStopReq_T;

typedef struct
{
    INT8U   CompletionCode;
} PACKED  AMIRISStartStopRes_T;

typedef struct 
{
	INT8U ImgName[256];
	INT8U MediaType;
	INT8U IsRedirected;
	INT8U MediaIndex;
	INT8U SessionIndex;
}PACKED RedirectedImageInfo_T;


typedef struct
{
	INT8U Index;
	INT8U ImageType;
	INT8U ImageName[20];
}PACKED ShortImageInfo_T;

typedef struct
{
	INT8U Index;
	INT8U ImageType;
	INT8U ImageName[256];
}PACKED LongImageInfo_T;

typedef union
{
	INT8U Index;
	INT8U Name[REDIRECTED_IMAGE_LEN];
}PACKED ImageInfo_T;

typedef struct
{
	INT8U Param;
	INT8U AppType;
	INT8U ImageType;
	INT8U RedirectionState;
	ImageInfo_T ImageInfo;
} PACKED AMIMediaRedirctionStartStopReq_T;

typedef struct
{
	INT8U CompletionCode;
} PACKED AMIMediaRedirctionStartStopRes_T;

typedef struct
{
	INT8U Param;
	INT8U AppType;
	INT8U ImageType;
	INT8U ImageIndex;
} PACKED AMIGetMediaInfoReq_T;

typedef struct
{
	INT8U CompletionCode;
	INT8U AppType;
	INT8U Num_of_images;
} PACKED AMIGetMediaInfoRes_T;

typedef struct
{
	INT8U ImageIndex;
	INT8U ImageName[REDIRECTED_IMAGE_LEN];
}PACKED Delete_T;

typedef struct
{
	INT8U ImageType;
}PACKED Update_T;

typedef struct
{
	INT8U ImageType;
	INT8U ImageIndex;
}PACKED Clear_T;

typedef union
{
	Delete_T Delete;
	Update_T Update;
	Clear_T Clear;
}SetOperations_T;

typedef struct
{
	INT8U Param;
	INT8U AppType;
	SetOperations_T Ops;
} PACKED AMISetMediaInfoReq_T;

typedef struct
{
	INT8U CompletionCode;
} PACKED AMISetMediaInfoRes_T;


/**
 * @struct AMIGetExtendedPrivReq_T
 * @brief  get user extended previlege value
 */
typedef struct
{
    INT8U UserID;
}PACKED AMIGetExtendedPrivReq_T;

/**
 * @struct AMIGetExtendedPrivRes_T
 * @brief  get user extended privilege field
 */
typedef struct
{
    INT8U CompletionCode;
    INT32U Extendedpriv;    
}PACKED AMIGetExtendedPrivRes_T;


/**
 * @struct AMISetExtendedPrivReq_T
 * @brief  set user extended privilege field 
 */
typedef struct
{
    INT8U UserID;
    INT32U Extendedpriv;
}PACKED AMISetExtendedPrivReq_T;
/**
 * @struct AMIGetHostLockStatusRes_T
 * @brief  get Host Monitor Lock Status
 */
typedef struct
{
    INT8U CompletionCode;
    INT8U LockStatus;
}PACKED AMIGetHostAutoLockStatusRes_T;

/**
 * @struct AMISetHostLockStatusReq_T
 * @brief  set Host Monitor Lock Status
 */
typedef struct
{
    INT8U LockStatus;
}PACKED AMISetHostAutoLockStatusReq_T;

/**
 * @struct AMISetHostLockStatusReq_T
 * @brief  set Host Monitor Lock Status
 */
typedef struct
{
	INT8U CompletionCode;
}PACKED AMISetHostAutoLockStatusRes_T;

/**
 * @struct AMIGetHostLockFeatureStatusRes_T
 * @brief  get Host Monitor Lock Feature Status
 */
typedef struct
{
    INT8U CompletionCode;
    INT8U LockFeatureStatus;
}PACKED AMIGetHostLockFeatureStatusRes_T;

/**
 * @struct AMISetHostLockFeatureStatusReq_T
 * @brief  set Host Monitor Lock Feature Status
 */
typedef struct
{
    INT8U LockFeatureStatus;
}PACKED AMISetHostLockFeatureStatusReq_T;

/**
 * @struct AMISetHostLockStatusReq_T
 * @brief  set Host Monitor Lock Status
 */
typedef struct
{
	INT8U CompletionCode;
}PACKED AMISetHostLockFeatureStatusRes_T;


typedef struct
{
    INT8U ChannelNumber;
}PACKED AMIGetChannelTypeReq_T;

typedef struct
{
    INT8U CompletionCode;
    INT8U ChannelNumber;
    INT8U ChannelType;
}PACKED AMIGetChannelTypeRes_T;

typedef struct
{
    INT8U ParameterSelector;
    INT8U PECIDevID;
    INT8U PECISlaveAddress;
    INT8U Awfcs;
    INT8U Domain;
    INT8U Data[PECI_DATA_SIZE-FEEDBACK_LEN+2];
}AMIPECIReq_T;

typedef struct
{
    INT8U CompletionCode;
    INT8U Data[PECI_DATA_SIZE];
}AMIPECIRes_T;

typedef struct
{
	INT8U ParameterSelect;
} PACKED AMIGetRemoteKVMCfgReq_T;

typedef union
{
    INT8U	Mouse_Mode;
    INT8U	keyboard_layout[KEYBRD_LANG_SIZE];
    INT8U	hostlock_feature_status;
    INT8U	auto_lock_status;
    INT8U	secure_status;
    INT8U   retry_count;
    INT8U   retry_interval;
} PACKED RemoteKVMCfg_T;

typedef struct
{
	INT8U CompletionCode;
	RemoteKVMCfg_T RemoteKVMCfg;
} PACKED AMIGetRemoteKVMCfgRes_T;

typedef struct
{
	INT8U   ParameterSelect;
	RemoteKVMCfg_T RemoteKVMcfg;
} PACKED AMISetRemoteKVMCfgReq_T;

typedef struct
{
	INT8U CompletionCode;
} PACKED AMISetRemoteKVMCfgRes_T;

typedef struct
{
	INT8U Param;
} PACKED AMIGetSSLCertStatusReq_T;

typedef union
{
	INT8U Status;
	INT8U CertInfo[MAX_FILE_INFO_SIZE];
	INT8U PrivateKeyInfo[MAX_FILE_INFO_SIZE];
}SSLCertInfo_T;

typedef struct
{
	INT8U CompletionCode;
	SSLCertInfo_T SSLCertInfo;
} PACKED AMIGetSSLCertStatusRes_T;

typedef struct {
        INT8U Enable;
	INT8U EncryptedEnable;
        INT16U PortNum;
        INT16U PortNumSecondary;
        INT8S IPAddr[MAX_LDAP_IP_ADRRESS];
        INT8S IPAddrSecondary[MAX_LDAP_IP_ADRRESS];
        INT8S Password[MAX_LDAP_PASSWORD];
        INT8S BindDN[MAX_LDAP_BIND_DN];
        INT8S SearchBase[MAX_LDAP_SEARCH_BASE];
        INT8S AttributeOfUserLogin[MAX_LDAP_ATTR_USER];
        INT8U DefaultRole;
	INT8U FQDNEnable;
}PACKED IPMILDAPCONFIG;

typedef struct __IPMItag_LDAPCONFIG
{
        INT8U LDAPRoleGroupNameStrlen;
        INT8S LDAPRoleGroupNameStr[MAX_GRP_NAME_DOMAIN_LEN];
        INT8U LDAPRoleGroupDomainStrlen;
        INT8S LDAPRoleGroupDomainStr[MAX_GRP_NAME_DOMAIN_LEN];
        INT32U LDAPRoleGroupPrivilege;
        INT32U LDAPRoleGroupExtendedPrivilege;
}PACKED IPMI_LDAP_Config_T;

typedef struct
{
    INT8U GroupRolecfg;
    union
    {
       INT8U UserID;
       INT8S LDAPRolestr[MAX_BLOCK_LEN];
       INT32U LDAPRolePriv;
    }LDAPRole;
}PACKED IPMILDAPROLE;

typedef union
{
       INT8U Enable;
       INT8U EncryptedEnable;
       INT16U PortNum;
       INT16U PortNumSecondary;
       INT8S IPAddr[MAX_LDAP_IP_ADRRESS];
       INT8S IPAddrSecondary[MAX_LDAP_IP_ADRRESS];
       INT8S Password[MAX_LDAP_PASSWORD];
       INT8S BindDN[MAX_LDAP_BIND_DN];
       INT8S SearchBase[MAX_LDAP_SEARCH_BASE];
       INT8S AttributeOfUserLogin[MAX_LDAP_ATTR_USER];
       INT8U DefaultRole;
       INT8U FQDNEnable;
       INT8U Progressbit;
       INT8S LDAPRoleGroupNameStr[MAX_GRP_NAME_DOMAIN_LEN];
       INT8S LDAPRoleGroupDomainStr[MAX_GRP_NAME_DOMAIN_LEN];
       INT32U LDAPRoleGroupPrivilege;
       INT32U LDAPRoleGroupExtendedPrivilege;
       IPMILDAPROLE SetRoleData;
} LDAPConfig_T;

typedef struct
{
       INT8U ParameterSelector;
       INT8U UserID;
       INT8U GroupRoleconf;
}PACKED AMIGetLDAPReq_T;

typedef struct
{
       INT8U CompletionCode;
       LDAPConfig_T Config;
}PACKED AMIGetLDAPRes_T;

typedef struct
{
       INT8U ParameterSelector;
       INT8U Blockselector;
       LDAPConfig_T Config;
}PACKED AMISetLDAPReq_T;

typedef struct
{
       INT8U CompletionCode;
}PACKED AMISetLDAPRes_T;

typedef enum  {
        AMI_AD_ENABLE = 0,
        AMI_AD_SSL_ENABLE,
        AMI_AD_TIME_OUT,
        AMI_AD_RAC_DOMAIN,
        AMI_AD_TYPE,
        AMI_AD_DC_FILTER1,
        AMI_AD_DC_FILTER2,
        AMI_AD_DC_FILTER3,
        AMI_AD_RAC_USERNAME,
        AMI_AD_RAC_PASSWORD,
        AMI_AD_GROUPROLE_CONFIG,
}AMIADOPERATION;

typedef enum  {
        AMI_AD_GROUP_USERID=0,
        AMI_AD_GROUP_NAME,
        AMI_AD_GROUP_DOMAIN,
        AMI_AD_GROUP_PRIVILEGE,
        AMI_AD_GROUP_EXTENDED_PRIVILEGE,
} AMIADGROUPOPERATION;

typedef struct __IPMI_tag_ADCONFIG
{
    INT8U ADEnable;
    INT8U SSLEnable;
    INT8U Reserved1;// to avoid alignment trap
    INT8U Reserved2;// to avoid alignment trap
    INT32U ADTimeout;
    INT8U ADRACDomainStrlen;
    INT8U ADRACDomainStr[MAX_AD_DOMAIN_LEN];
    INT8U ADType; // 1 for extended and 2 for std, we are not using extended, only value is 2.
    INT8U ADDCFilter1Len;
    INT8U ADDCFilter1[MAX_AD_FILTER_LEN];
    INT8U ADDCFilter2Len;
    INT8U ADDCFilter2[MAX_AD_FILTER_LEN];
    INT8U ADDCFilter3Len;
    INT8U ADDCFilter3[MAX_AD_FILTER_LEN];
    INT8U ADRACUserNameStrlen;
    INT8U ADRACUserName[MAX_AD_USERNAME_LEN];
    INT8U ADRACPasswordStrlen;
    INT8U ADRACPassword[MAX_AD_PASSWORD_LEN];
}PACKED IPMI_AD_Config_T;

typedef struct __IPMI_tag_SSADCONFIG
{
    INT8U SSADRoleGroupNameStrlen;
    INT8U SSADRoleGroupNameStr[MAX_AD_ROLE_NAME_LEN];
    INT8U SSADRoleGroupDomainStrlen;
    INT8U SSADRoleGroupDomainStr[MAX_AD_ROLE_DOMAIN_LEN];
    INT32U SSADRoleGroupPrivilege;
    INT32U ExtendedPrivilege;
}PACKED IPMI_SSAD_Config_T;

typedef struct
{
    INT8U GroupRolecfg;
    union
    {
       INT8U UserID;
       INT8S ADRolestr[MAX_AD_ROLE_NAME_LEN];
       INT32U ADRolePriv;
    }ADRole;
}PACKED IPMIADROLE;

typedef union
{
    INT8U ADEnable;
    INT8U SSLEnable;
    INT32U ADTimeout;
    INT8U ADRACDomainStr[MAX_AD_DOMAIN_LEN];
    INT8U ADType; // 1 for extended and 2 for std, we are not using extended, only value is 2.
    INT8U ADDCFilter1[MAX_AD_FILTER_LEN];
    INT8U ADDCFilter2[MAX_AD_FILTER_LEN];
    INT8U ADDCFilter3[MAX_AD_FILTER_LEN];
    INT8U ADRACUserName[MAX_AD_USERNAME_LEN];
    INT8U ADRACPassword[MAX_AD_PASSWORD_LEN];
    INT8U Progressbit;
    INT8U SSADRoleGroupNameStr[MAX_AD_ROLE_NAME_LEN];
    INT8U SSADRoleGroupDomainStr[MAX_AD_ROLE_DOMAIN_LEN];
    INT32U SSADRoleGroupPrivilege;
    INT32U ExtendedPrivilege;
    IPMIADROLE SetRoleData;
} ADCONFIG_T;
typedef struct
{
    INT8U ParameterSelector;
    INT8U UserID;
    INT8U GroupRoleconf;
}PACKED AMIGetADReq_T;

typedef struct
{
   INT8U CompletionCode;
   ADCONFIG_T Config;
}PACKED AMIGetADRes_T;
typedef struct
{
   INT8U ParameterSelector;
   INT8U Blockselector;
   ADCONFIG_T Configdata;
}PACKED AMISetADReq_T;

typedef struct
{
   INT8U CompletionCode;
}PACKED AMISetADRes_T;

typedef enum  {
        AMI_LDAP_STATUS = 0,
	AMI_LDAP_ENCRYPTEDENABLE,
        AMI_LDAP_PORT_NUM,
        AMI_LDAP_PORT_NUM_SEC,
        AMI_LDAP_IP_ADDR,
        AMI_LDAP_IP_ADDR_SEC,
        AMI_LDAP_PASSWORD,
        AMI_LDAP_BIND_DN,
        AMI_LDAP_SEARCH_BASE,
        AMI_LDAP_ATTRIBUTE_USER_LOGIN,
        AMI_LDAP_DEFAULT_ROLE,
        AMI_LDAP_GROUPROLE_CONFIG,
        AMI_LDAP_FQDN_ENABLE,
}AMILDAPOPERATION;

typedef enum  {
        AMI_LDAP_GROUP_USERID=0,
        AMI_LDAP_GROUP_NAME,
        AMI_LDAP_GROUP_DOMAIN,
        AMI_LDAP_GROUP_PRIVILEGE,
        AMI_LDAP_GROUP_EXTENDED_PRIVILEGE,
} AMILDAPGROUPOPERATION;

typedef enum  {
        AMI_RADIUS_STATUS = 0, 
        AMI_RADIUS_IP_ADDR,
        AMI_RADIUS_PORT_NUM,
        AMI_RADIUS_SECRET,
        AMI_RADIUS_TIMEOUT,
        AMI_RADIUS_PRIVILEGE,
        AMI_RADIUS_EXT_PRIVILEGE,
        AMI_RADIUS_VENDOR_DATA,
}AMIRADIUSOPERATION;

typedef struct {
	INT8U Enable;
	INT8S IPAddr[RADIUS_IPAddr_LEN];
	INT16U PortNum;
	INT8S Secret[RADIUS_SECRET_LEN];
	INT32U Timeout;
	INT32U ExtendedPrivilege;
	// additional setting for IPMI LAN privilege
	INT32U Privilege;
}PACKED IPMIRADIUSCONFIG;

typedef struct tag_IPMIRadiusPriv
{
	INT32U AccessLevel;
	INT8U VendorData[RADIUS_VENDORData_LEN];
}PACKED IPMIRADIUSPRIVCONFIG;

typedef struct tag_IPMISetRadiusPriv
{
	INT8U AccessLevel;
	INT8U VendorData[RADIUS_VENDORData_LEN];
}PACKED IPMISetRADIUSPRIVCONFIG;

typedef union 
{
    INT8U Status;
    INT8S IPAddr[RADIUS_IPAddr_LEN];
    INT16U PortNum;
    INT8S Secret[RADIUS_SECRET_LEN];
    INT32U Timeout;
    INT32U ExtendedPrivilege;
    // additional setting for IPMI LAN privilege
    INT32U Privilege;
    IPMIRADIUSPRIVCONFIG Vendordata;
    INT8U Progressbit;
    INT8S Get_Vendordata[RADIUS_VENDORData_LEN];
    IPMISetRADIUSPRIVCONFIG VendorData;
}PACKED RadiusConfig_T;

typedef struct
{
    INT8U ParameterSelector;
    INT8U Accesslevel;
}PACKED AMIGetRadiusReq_T;

typedef struct
{
    INT8U CompletionCode;
    RadiusConfig_T Config;
}PACKED AMIGetRadiusRes_T;

typedef struct
{
    INT8U ParameterSelector;
    INT8U Blockselector;
    RadiusConfig_T ConfigData;
}PACKED AMISetRadiusReq_T;

typedef struct
{
    INT8U CompletionCode;
}PACKED AMISetRadiusRes_T;
typedef struct
{
    INT8U CompletionCode;
    INT8U totalrecord;
    INT32U record[20];
    
}AMIActiveSessionRes_T;
typedef struct
{
	INT8U Session_type;
	
}AMIActiveSessionReq_T;
typedef struct
{
    INT32U racsessionid;
    
}AMIActivedcloseSessionReq_T;

typedef struct
{
	INT8U CompletionCode;    
}AMIActivedcloseSessionRes_T;

typedef struct
 {
 	INT8U Param;
 } PACKED AMIGetVmediaCfgReq_T;

typedef union {
	INT8U attach_cd;			/** Attach mode for CD */
	INT8U attach_fd;			/** Attach mode for FD */
	INT8U attach_hd;			/** Attach mode for HD */
	INT8U enable_boot_once;		/** Enable boot once or not */
	INT8U num_cd;			/** Number of CD Instances */
	INT8U num_fd;			/** Number of FD Instances */
	INT8U num_hd;			/** Number of HD Instances */
	INT8U lmedia_enable;		/** Enable the LMedia Feature */
	INT8U rmedia_enable;		/** Enable the RMedia Feature */
	INT8U sdmedia_enable;
	INT8U secure_status;
	INT8U Vmedia_restart;
	INT8U kvm_cd;			/** Number of CD Instances for vmedia */
	INT8U kvm_fd;			/** Number of FD Instances for vmedia*/
	INT8U kvm_hd;			/** Number of HD Instances for vmedia*/
} PACKED VMediaConfig_T;

typedef struct
{
	INT8U CompletionCode;
	VMediaConfig_T VMediaConfig;
} PACKED AMIGetVmediaCfgRes_T;

typedef struct
{
	INT8U Param;
	VMediaConfig_T VMediaConfig;
} PACKED AMISetVmediaCfgReq_T;

typedef struct
{
	INT8U CompletionCode;
} PACKED AMISetVmediaCfgRes_T;

typedef struct
{
	INT8U CompletionCode;
	INT8U RuntimeSinglePort;
} PACKED AMIGetRunTimeSinglePortStatusRes_T;

typedef struct
{
	INT8U RuntimeSinglePort;
} PACKED AMISetRunTimeSinglePortStatusReq_T;

typedef struct
{
	INT8U CompletionCode;
	INT8U Enable;
} PACKED AMIRestartWebServiceRes_T;


typedef struct {
	INT32U attach_cd;			
	INT32U attach_fd;			
	INT32U attach_hd;			
	INT32U enable_boot_once;		
	INT32U num_cd;			
	INT32U num_fd;			
	INT32U num_hd;			
	INT32U lmedia_enable;		
	INT32U rmedia_enable;		
	INT32U sdmedia_enable;
	INT32U power_consumption_enable;
	INT32U kvm_cd;
	INT32U kvm_fd;
	INT32U kvm_hd;
} VMediaCfgParam_T;

typedef struct {
    char path[MAX_IMAGE_PATH_SIZE];
    unsigned int max_time;
    unsigned int max_size;
    unsigned int max_dumps;
    unsigned int remote_path_support;
    char ip_addr[MAX_IP_ADDR_LEN];
    char username[MAX_RMS_USERNAME_LEN];
    char password[MAX_RMS_PASSWORD_LEN];
    char share_type[MAX_SHARE_TYPE_LEN];
    char domain[MAX_DOMAIN_LEN];
    unsigned int mnt_successful;
} PACKED IPMI_AutoRecordCfg_T;

typedef struct
{
    //Continuous Recording parameters
    unsigned char vdo_duration;
    unsigned char vdo_quality;
    unsigned char vdo_compression_mode;
    unsigned char frame_count;
} PACKED IPMI_RecordBootCrashCfg_T;

typedef enum  {
        AMI_VIDEO_RCD_REMOTE_PATH_SUPPORT=0,
        AMI_VIDEO_RCD_PATH,
        AMI_VIDEO_RCD_MAX_TIME,
        AMI_VIDEO_RCD_MAX_SIZE,
        AMI_VIDEO_RCD_MAX_DUMP,
        AMI_VIDEO_RCD_IP_ADDR,
        AMI_VIDEO_RCD_USERNAME,
        AMI_VIDEO_RCD_PASSWORD,
        AMI_VIDEO_RCD_SHARE_TYPE,
        AMI_VIDEO_RCD_DOMAIN,
        AMI_VIDEO_RCD_MNT_SUCCESS,
        AMI_VIDEO_RCD_RST,
        AMI_CONT_VIDEO_RCD_CONF,
        AMI_VIDEO_RCD_PATH_TEST_MOUNT,
        AMI_VIDEO_RCD_STATUS,
        AMI_VIDEO_RCD_PARAM_MAX 
}AMIVideoRcdOPERATION;

typedef union {
    INT8S path[MAX_IMAGE_PATH_SIZE];
    INT32U max_time;
    INT32U max_size;
    INT32U max_dumps;
    INT32U remote_path_support;
    INT8S ip_addr[MAX_IP_ADDR_LEN];
    INT8S username[MAX_RMS_USERNAME_LEN];
    INT8S password[MAX_RMS_PASSWORD_LEN];
    INT8S share_type[MAX_SHARE_TYPE_LEN];
    INT8S domain[MAX_DOMAIN_LEN];
    INT32U mnt_successful;
    INT8U Progressbit;
    //Continuous video recording struct
    IPMI_RecordBootCrashCfg_T ContRcdCfg;
} PACKED VideoRcd_Config_T;

typedef struct
{
    INT8U ParameterSelector;
}PACKED AMIGetVideoRcdReq_T;

typedef struct
{
    INT8U CompletionCode;
    VideoRcd_Config_T Config;
}PACKED AMIGetVideoRcdRes_T;

typedef struct
{
    INT8U ParameterSelector;
    INT8U Blockselector;
    VideoRcd_Config_T Config;
}PACKED AMISetVideoRcdReq_T;

typedef struct
{
    INT8U CompletionCode;
}PACKED AMISetVideoRcdRes_T;
typedef struct
{
	char Image_Name[MAX_IMAGE_NAME_LEN];
	char Image_Path[MAX_IMAGE_PATH_SIZE];
	char Ip_Addr[MAX_IP_ADDR_LEN];
	char Username[MAX_RMS_USERNAME_LEN];
	char Password[MAX_RMS_PASSWORD_LEN];
	char Share_Type[MAX_SHARE_TYPE_LEN];
	char Domain[MAX_DOMAIN_LEN];
	unsigned int  Start_Mount;
	unsigned int Mount_Successful;
	unsigned int Err_Code;
} PACKED MediaConfig_T;

/* AMIGetBMCInstanceCountRes_T */
typedef struct {
    INT8U   CompletionCode;
    INT8U   BMCInstanceCount;
} PACKED  AMIGetBMCInstanceCountRes_T;

/* AMIGetUSBSwitchSettingRes_T */
typedef struct {
    INT8U   CompletionCode;
    INT8U   USBSwitchSetting;
} PACKED  AMIGetUSBSwitchSettingRes_T;

/* AMISetUSBSwitchSettingReq_T */
typedef struct {
    INT8U   USBSwitchSetting;
} PACKED  AMISetUSBSwitchSettingReq_T;

/* AMISetUSBSwitchSettingRes_T */
typedef struct {
    INT8U   CompletionCode;
} PACKED  AMISetUSBSwitchSettingRes_T;

#pragma pack ()

typedef struct {
	unsigned int Mouse_Mode;
	char   keyboard_layout[KEYBRD_LANG_SIZE];
	unsigned int hostlock_value;
    unsigned int retry_count;
    unsigned int retry_interval;
} AdviserConfig_T;

typedef struct { 
	INT8U direction; 
}AMISwitchMUXReq_T;

#endif //__IPMI_AMICONF_H__
