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
 ******************************************************************
 *
 * lanconfig.c
 * Lan Configuration functions.
 *
 *  Author: Bakka Ravinder Reddy <bakkar@ami.com>
 *
 ******************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "IPMIConf.h"
#include "LANConfig.h"
#include "MsgHndlr.h"
#include "Debug.h"
#include "Support.h"
#include "IPMI_LANConfig.h"
#include "PMConfig.h"
#include "SharedMem.h"
#include "IPMIDefs.h"
#include "NVRAccess.h"
#include "Util.h"
#include "Session.h"
#include "WDT.h"
#include "LANIfc.h"
#include "AppDevice.h"
#include "RMCP+.h"
#include "IPMI_Main.h"
#include "IPMI_LANConfig.h"
#include "nwcfg.h"
#include "PendTask.h"
#include "Ciphertable.h"
#include "Ethaddr.h"
#include "sendarp.h"
#include "PDKAccess.h"
#include "Message.h"
#include <linux/if.h>
#include "Badpasswd.h"
#include "libncsiconf.h"
#include <linux/ip.h>
#include <dlfcn.h>
#include <sys/prctl.h>
#include "featuredef.h"
#include <flashlib.h>
#include <libphyconf.h>
#include "safesystem.h"

/* Reserved bit macro definitions */
#define RESERVED_BITS_SUSPENDBMCARPS                    0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_GETIPUDPRMCPSTATS_CH              0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_GETIPUDPRMCPSTATS_CLRSTATE        0xFE //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1)

/*** Local definitions ***/
#define LAN_RESERVED                            0x00
#define CHANNEL_ID_MASK                         0x0f
#define SET_IN_PROGRESS_MASK                    0x03
#define PARAMETER_REVISION_MASK                 0x0f
#define DEST_ADDR_DATA2_ADDR_FORMAT_MASK        0xf0
#define PARAMETER_REVISION_FORMAT               0x11
#define GET_PARAMETER_REVISION_MASK             0x80
#define LAN_CALLBACK_AUTH_SUPPORT               0x17    /* MD2 & MD5 supported  */
#define LAN_USER_AUTH_SUPPORT                   0x17    /* MD2 & MD5 supported  */
#define LAN_OPERATOR_AUTH_SUPPORT               0x17    /* MD2 & MD5 supported  */
#define LAN_ADMIN_AUTH_SUPPORT                  0x17    /* MD2 & MD5 supported  */
#define BRD_CAST_BIT_MASK                       0xFF
#define LOOP_BACK_BIT_MASK                      0x7F
#define SUBNET_MASK_BIT_CHECK                   0x80

#define LAN_CONFIGURATION_SET_IN_PROGRESS       0x01
#define LAN_CONFIGURATION_SET_COMPLETE          0x00

#define GRATIUTOUS_ENABLE_MASK                  1
#define ENABLE_ARP_RESPONSES                    2
#define SUSPEND_ARP_RSVD_BIT_MASK               0xFC
#define ENABLE_ARPS                             0x03
#define SUSPEND_GRAT_ARP                        0x01
#define SUSPEND_ARP                             0x02

#define ARP_IGNORE_ON	8
#define ARP_IGNORE_OFF	0
#define DISABLE        0


/* Reserved Bits */
#define RESERVED_VALUE_70						0x70
#define RESERVED_VALUE_F0						0xF0

/**
*@fn NwInterfacePresenceCheck
*@brief This function is invoked to check network interface presence
*@param Interface - Char Pointer to buffer for which interface to check
*/
static int NwInterfacePresenceCheck (char * Interface);

/*** Module Variables ***/
//       INT8U  m_ArpSuspendReq;

char **explode(char separator, char *string);
int IPAddrCheck(INT8U *Addr,int params);
extern int GetLanAMIParamValue (INT8U* ParamSelect, INT8U* ImpType);

extern IfcName_T Ifcnametable[MAX_LAN_CHANNELS];
#define MAX_LAN_PARAMS_DATA  20
typedef struct
{
    INT8U	Params;
    INT8U	ReservedBits [MAX_LAN_PARAMS_DATA];
    INT8U	DataLen;

} LANCfgRsvdBits_T;

static LANCfgRsvdBits_T m_RsvdBitsCheck [] = {

    /* Param                 Reserved Bits                    Data Size   */
    { 0,	     			{ 0xFC}, 				 0x1 },	/* Set In progress  */
    { 1,				{ 0xC8 },					 0x1 }, 	 /* Authenication type */
    { 2,				{ 0xC8,0xC8,0xC8,0xC8,0xC8 }, 0x5}, 	 /* Authenication Support Enable  */
    { 4,				{ 0xF0 },					 0x1 }, 	 /* l */
    { 7,				{ 0x0,0x1F,0x01 },			 0x3 }, 	 /* l */
    { 0xA,				{ 0xFC },					 0x1 }, 	 /* l */
    { 0x11,			{ 0xF0 },					 0x1 }, 	 /* l */
    { 0x12,			{ 0xF0,0x78,0x0,0xF8 },		 0x4 }, //0x78
    { 0x13,			{ 0xF0,0x0F, 0xFE },		 0x3 },
    { 0x14,			{ 0x0,0x70},				 0x2 },
    { 0x15,			{ 0xF8 },					 0x1 },
    { 0x16,			{ 0xE0 },					 0x1 },
    { 0x17,			{ 0xFF },					 0x1 },
    { 0x18,			{ 0xFF },					 0x1 },
    { 0x19,			{ 0xF0,0x0F },				 0x2 },
    { 0x1A,			{ 0xFE },				 	 0x1 }
};

typedef struct
{
    INT8U OEMParam;
    INT8U Length;
}OEMParameterLength_T;

static OEMParameterLength_T m_OEMParamLen [] = {
        {LAN_PARAMS_AMI_OEM_IPV6_ENABLE,                    1 },
        {LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR_SOURCE,    1 },
        {LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR,                17 },
        {LAN_PARAMS_AMI_OEM_IPV6_PREFIX_LENGTH,      2},
        {LAN_PARAMS_AMI_OEM_IPV6_GATEWAY_IP,         16}
};

/**
 * @brief LAN configuration request parameter lengths
**/
static const INT8U LanconfigParameterLength [] = {
                    1,  /* Set in progress */
                    1,  /* Authentication type support */
                    5,  /* Authentication type enables */
                    4,  /* IP address */
                    1,  /* IP address source */
                    6,  /* MAC address */
                    4,  /* Subnet mask */
                    3,  /* IPv4 header parameters */
                    2,  /* Primary RMCP port number */
                    2,  /* Secondary RMCP port number */
                    1,  /* BMC generated ARP control */
                    1,  /* Gratuitous ARP */
                    4,  /* Default Gateway address */
                    6,  /* Default Gateway MAC address */
                    4,  /* Backup Gateway address */
                    6,  /* Backup Gateway MAC address */
                    18, /* Community string */
                    1,  /* Number of destinations */
                    4,  /* Destination type */
                    13, /* Destination address */
                    2,  /* VLAN ID */
                    1,  /* VLAN Priority */
                    1,  /* Cipher suite entry support */
                    17, /* Cipher suite entries */
                    9,  /* Cipher suite Privilege levels */
                    4,  /* VLAN tags destination address  */
                    6,  /* Bad Password Threshold */
                    (9+16) /* IPv6 Destination address */
};

/* A copy of ip_tos2prio with numeric format in "linux/net/ipv4/route.c" */ 
static const INT8U IP_TOS2PRIO[16] = {
    0,        /* TC_PRIO_BESTEFFORT,           */                     
    1,        /* ECN_OR_COST(FILLER),          */           
    0,        /* TC_PRIO_BESTEFFORT,           */           
    0,        /* ECN_OR_COST(BESTEFFORT),      */           
    2,        /* TC_PRIO_BULK,                 */           
    2,        /* ECN_OR_COST(BULK),            */          
    2,        /* TC_PRIO_BULK,                 */          
    2,        /* ECN_OR_COST(BULK),            */          
    6,        /* TC_PRIO_INTERACTIVE,          */           
    6,        /* ECN_OR_COST(INTERACTIVE),     */           
    6,        /* TC_PRIO_INTERACTIVE,          */           
    6,        /* ECN_OR_COST(INTERACTIVE),     */           
    4,        /* TC_PRIO_INTERACTIVE_BULK,     */           
    4,        /* ECN_OR_COST(INTERACTIVE_BULK),*/           
    4,        /* TC_PRIO_INTERACTIVE_BULK,     */           
    4         /* ECN_OR_COST(INTERACTIVE_BULK) */             
};
static BOOL enableSetMACAddr = FALSE;

/*-------------------------------------------------------
 * SetLanConfigParam
 *-------------------------------------------------------*/
int
SetLanConfigParam (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    SetLanConfigReq_T*  pSetLanReq = (SetLanConfigReq_T*) pReq;
    SetLanConfigRes_T*  pSetLanRes = (SetLanConfigRes_T*) pRes;
      BMCSharedMem_T*     pSharedMem = BMC_GET_SHARED_MEM (BMCInst);
      ChannelInfo_T*      ptrChannelInfo;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT      NWConfig6;
	RAipV6Addr_T        RAipV6Addr = {0};
	RAipV6Prefix_T      RAipV6Prefix = {0};
    INT32U GWIp,IPAddr;
    INT32U Subnetmask;
    int i,j=0, Num_of_Ports = 0;
    INT8U IsOemDefined = FALSE;
    INT8U EthIndex,netindex= 0xFF, currBmcGenArpCtrl;
    BMCArg *pLANArg=NULL; 
    INT8U ethcount=0;	
    char    VLANInterfaceName [32];
    INT16U vlanID=0;
    INT8U AddrFormat = 0;	
    int retValue=0,NIC_Count = 0;
    INT16U PriorityLevel[MAX_LAN_CHANNELS]= {0};
    INT8U  m_Lan_SetInProgress; /**< Contains setting LAN configuration status */
    ETHCFG_STRUCT PHYCfg;
    static INT8U macEthIndex = 0xFF;
    int pendStatus = PEND_STATUS_COMPLETED;
    char IfcName[16];     /* Eth interface name */
    int vLANMSB = 0, vLANLSB = 0;
    INT8U CipherSuitePrivilegelevels [N0_OF_CIPHER_SUITE_SUPPORTED + 1] = {0};
    INT8U CipherSuiteID = -1;
    INT8U InterfaceName[MAX_IPV6_INTERFACE][MAX_IFC_NAME]={{0}};
    int noofinterface=0;

    if ( ReqLen >= 2 )
    {
        ReqLen -= 2;
    }
    else
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof (INT8U);
    }

    if (1 == IsBMCNFSMode())
    {
        TINFO ("BMC is in NFS mode, Ignore Set Lan Parameter command\n");
        pSetLanRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
        return sizeof (*pRes);
    }

  /* Validation check for IPMI IPv6 Commands, version 1.1 */
   if((pSetLanReq->ParameterSelect >= LAN_PARAM_IPV6_IPV4_SUPPORT && pSetLanReq->ParameterSelect <=LAN_PARAMS_IPV6_SLAAC_TIMING_CONFIGURATION )
          && !( g_corefeatures.ipv6_compliance_support))        
   {
        pSetLanRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
        return sizeof(INT8U);
   }
   
   if(((pSetLanReq->ParameterSelect >= LAN_PARAMS_AMI_OEM_IPV6_ENABLE && pSetLanReq->ParameterSelect <=LAN_PARAMS_AMI_OEM_IPV6_GATEWAY_IP )) 
          && ( g_corefeatures.ipv6_compliance_support)) 
   {
        pSetLanRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
        return sizeof(INT8U);
   }


    EthIndex= GetEthIndex(pSetLanReq->ChannelNum & 0x0F, BMCInst);
    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    memset(IfcName,0,sizeof(IfcName));
    /*Get the EthIndex*/
    if(GetIfcName(EthIndex,IfcName, BMCInst) == -1)
    {
        TCRIT("Error in Getting Ifc name\n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
    {
        if(strcmp(Ifcnametable[i].Ifcname,IfcName) == 0)
        {
            netindex= Ifcnametable[i].Index;
            break;
        }
    }

    if(netindex == 0xFF)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }
    
    if((pSetLanReq->ParameterSelect >= MIN_LAN_OEM_CONF_PARAM) && 
            (pSetLanReq->ParameterSelect <= MAX_LAN_OEM_CONF_PARAM) )
    {
    	/* Converts OEM parameter value to equivalent AMI parameter value */
    	if (0 != GetLanAMIParamValue (&pSetLanReq->ParameterSelect, &IsOemDefined) )
    	{
            pSetLanRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
    	}

    	/* Hook for OEM to handle this parameter */
        if ( (IsOemDefined)  && (g_PDKHandle[PDK_SETLANOEMPARAM] != NULL) )
        {
			return ((int(*)(INT8U *, INT8U, INT8U *,int))(g_PDKHandle[PDK_SETLANOEMPARAM]))(pReq, ReqLen, pRes, BMCInst);
    	}
   	
    }    

    if(0x1b >pSetLanReq->ParameterSelect  )   //Max known Lan paramter
    {
        if (ReqLen != LanconfigParameterLength [pSetLanReq->ParameterSelect ])
        {
            *pRes = CC_REQ_INV_LEN;
            return sizeof (INT8U);
        }
    }

    /*Check the Request length for OEM parameter*/
    for(i=0;i<sizeof(m_OEMParamLen)/sizeof(OEMParameterLength_T);i++)
    {
        if(m_OEMParamLen[i].OEMParam == pSetLanReq->ParameterSelect)
        {
            if(ReqLen != m_OEMParamLen[i].Length)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }
        }
    }

    if(pSetLanReq->ChannelNum & RESERVED_VALUE_F0)
    {
        /* Alarm !!! Somebody is trying to set Reseved Bits */
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }    
 
    /* Check for Reserved Bits */
    for (i = 0; i < sizeof (m_RsvdBitsCheck)/ sizeof (m_RsvdBitsCheck[0]); i++)
    {
        /* Check if this Parameter Selector needs Reserved bit checking !! */
        if (m_RsvdBitsCheck[i].Params == pSetLanReq->ParameterSelect)
        {
            //IPMI_DBG_PRINT_2 ("Param - %x, DataLen - %x\n", pSetLanReq->ParameterSelect, m_RsvdBitsCheck[i].DataLen);
            for (j = 0; j < m_RsvdBitsCheck[i].DataLen; j++)
            {
                //	IPMI_DBG_PRINT_2 ("Cmp  %x,  %x\n", pReq[2+j], m_RsvdBitsCheck[i].ReservedBits[j]);
                if ( 0 != (pReq[2+j] & m_RsvdBitsCheck[i].ReservedBits[j]))
                {
                    /* Alarm !!! Somebody is trying to set Reseved Bits */
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (*pRes);
                }
            }
        }
     }

    ptrChannelInfo = getChannelInfo (pSetLanReq->ChannelNum & 0x0F, BMCInst);
    if(NULL == ptrChannelInfo)
    {
        *pRes = CC_INV_DATA_FIELD;
        return	sizeof(*pRes);
    }
	
    IPMI_DBG_PRINT_1 ("Parameter = %X\n", pSetLanReq->ParameterSelect);
    if (g_PDKHandle[PDK_BEFORESETLANPARM] != NULL )
    {
        retValue = ((int(*)(INT8U *, INT8U, INT8U *,int))(g_PDKHandle[PDK_BEFORESETLANPARM]))(pReq, ReqLen, pRes, BMCInst);
        if(retValue != 0)
        {
              return retValue;
        }
    }

    if((pSetLanReq->ParameterSelect == LAN_PARAM_SELECT_DEST_ADDR) || 
            (pSetLanReq->ParameterSelect == LAN_PARAMS_AMI_OEM_SNMPV6_DEST_ADDR))
    {
        /*Destination Address is allowed to configure only if DestType is LAN_PET_TRAP(SNMP alerts types)*/
        if(pBMCInfo->LANCfs[EthIndex].DestType [pSetLanReq->ConfigData.DestAddr.SetSelect - 1].DestType == LAN_OEM1_ALERT)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof (INT8U);
        }
    }
    switch (pSetLanReq->ParameterSelect)
    {
        case LAN_PARAM_SET_IN_PROGRESS:
            if((pSetLanReq->ConfigData.SetInProgress & SET_IN_PROGRESS_MASK) == SET_IN_PROGRESS_MASK)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (*pRes);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            m_Lan_SetInProgress = BMC_GET_SHARED_MEM(BMCInst)->m_Lan_SetInProgress;
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            /* Commit Write is optional and supported
             * only if rollback is supported */
            if ( (GetBits(pSetLanReq->ConfigData.SetInProgress, SET_IN_PROGRESS_MASK) !=
                    LAN_CONFIGURATION_SET_COMPLETE) &&
                 (GetBits(pSetLanReq->ConfigData.SetInProgress, SET_IN_PROGRESS_MASK) !=
                    LAN_CONFIGURATION_SET_IN_PROGRESS) )
            {
                pSetLanRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
                return sizeof(*pSetLanRes);
            }
            else if ((GetBits(m_Lan_SetInProgress, SET_IN_PROGRESS_MASK) ==
                        LAN_CONFIGURATION_SET_IN_PROGRESS) &&
                     (GetBits(pSetLanReq->ConfigData.SetInProgress, SET_IN_PROGRESS_MASK) ==
                        LAN_CONFIGURATION_SET_IN_PROGRESS))
            {
                pSetLanRes->CompletionCode = CC_SET_IN_PROGRESS;
                return sizeof(*pSetLanRes);
            }

            LOCK_BMC_SHARED_MEM(BMCInst);
            BMC_GET_SHARED_MEM(BMCInst)->m_Lan_SetInProgress = pSetLanReq->ConfigData.SetInProgress;
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case LAN_PARAM_AUTH_TYPE_SUPPORT:
            pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof(*pRes);

        case LAN_PARAM_AUTH_TYPE_ENABLES:
            for(i=0;i<5;i++)
            {
                 /* Check for Unsupported AuthType */
                if (pBMCInfo->LANCfs[EthIndex].AuthTypeSupport != (pBMCInfo->LANCfs[EthIndex].AuthTypeSupport  |pReq[2+i]))
                {
                      IPMI_DBG_PRINT_2("\n Alarm !!! Somebody is trying to Unsupported Bit :%d \t%d\n",pReq[2+i],i);
                      *pRes = CC_INV_DATA_FIELD;
                      return sizeof (*pRes);
                 }
            }

            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (&pBMCInfo->LANCfs[EthIndex].AuthTypeEnables,
                      &(pSetLanReq->ConfigData.AuthTypeEnables), sizeof(AuthTypeEnables_T));
            _fmemcpy (ptrChannelInfo->AuthType,
                      &(pSetLanReq->ConfigData.AuthTypeEnables), sizeof(AuthTypeEnables_T));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case LAN_PARAM_IP_ADDRESS:
            pendStatus = GetPendStatus(PEND_OP_SET_IP);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            //we need to do a read in hte pend task and not here
            // because if pend task is still working on setting the source for example t- by then we would have got the
            // next command which is ip address and then we would read back DHCP since nwcfg hasnt done its work yet etc. and all hell will breakloose.
            //nwReadNWCfg  (&NWConfig);

            if(pBMCInfo->LANCfs[EthIndex].IPAddrSrc == DHCP_IP_SOURCE)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

	    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
	    if(memcmp ( NWConfig.IPAddr, pSetLanReq->ConfigData.IPAddr, IP_ADDR_LEN ) == 0)
	    {
		TCRIT("Same IP address, do nothing\n");
		_fmemcpy (pBMCInfo->LANCfs[EthIndex].IPAddr, pSetLanReq->ConfigData.IPAddr, IP_ADDR_LEN);
		break;
	    }

            if(IPAddrCheck(pSetLanReq->ConfigData.IPAddr,LAN_PARAM_IP_ADDRESS))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }

            pendStatus = GetPendStatus(PEND_OP_SET_IP);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            
            _fmemcpy (NWConfig.IPAddr, pSetLanReq->ConfigData.IPAddr, IP_ADDR_LEN);
            SetPendStatus(PEND_OP_SET_IP,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_IP,(INT8U*)&NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F),BMCInst);
            _fmemcpy (pBMCInfo->LANCfs[EthIndex].IPAddr, pSetLanReq->ConfigData.IPAddr, IP_ADDR_LEN);
            //nwWriteNWCfg (&NWConfig);
            break;

        case LAN_PARAM_IP_ADDRESS_SOURCE:

            if ((pSetLanReq->ConfigData.IPAddrSrc > BMC_OTHER_SOURCE)
                ||(pSetLanReq->ConfigData.IPAddrSrc == UNSPECIFIED_IP_SOURCE))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            if ( pBMCInfo->LANCfs[EthIndex].IPAddrSrc == pSetLanReq->ConfigData.IPAddrSrc )
            {
                TCRIT("LAN or VLAN if current SRC is DHCP/Static and incoming SRC is DHCP/Static, do nothing\n");
                break;
            }
		if((g_corefeatures.delayed_lan_restart_support == ENABLED) && (pSetLanReq->ConfigData.IPAddrSrc == STATIC_IP_SOURCE ))
		{
			/* If the client is not configured the Static ip address 
			 * then the pBMCInfo->LANCfs[EthIndex].IPAddr is "0.0.0.0".
			 * So, At this time we are updating the Static ipv4 settings from 
			 * the Network Configuration file.
			 */
			if(pBMCInfo->LANCfs[EthIndex].IPAddr[0] == 0) 
			{
				nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
				_fmemcpy (pBMCInfo->LANCfs[EthIndex].IPAddr, NWConfig.IPAddr, IP_ADDR_LEN);
				_fmemcpy (pBMCInfo->LANCfs[EthIndex].DefaultGatewayIPAddr, NWConfig.Gateway, IP_ADDR_LEN);
				_fmemcpy (pBMCInfo->LANCfs[EthIndex].SubNetMask, NWConfig.Mask, IP_ADDR_LEN);
			}
		}
            if ( (pSetLanReq->ConfigData.IPAddrSrc == STATIC_IP_SOURCE ) || (pSetLanReq->ConfigData.IPAddrSrc == DHCP_IP_SOURCE ) )
            {
                pendStatus = GetPendStatus(PEND_OP_SET_SOURCE);
                if(pendStatus == PEND_STATUS_PENDING)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof (INT8U);
                }
		pBMCInfo->LANCfs[EthIndex].IPAddrSrc = pSetLanReq->ConfigData.IPAddrSrc ;
                NWConfig.CfgMethod = pSetLanReq->ConfigData.IPAddrSrc;
                SetPendStatus(PEND_OP_SET_SOURCE,PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_SOURCE,(INT8U*) &NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            }
            else if(pSetLanReq->ConfigData.IPAddrSrc == BIOS_IP_SOURCE)
            {
                /*Perform OEM action*/
                if(g_PDKHandle[PDK_BIOSIPSOURCE] != NULL)
                {
                	 retValue = ((int(*)(INT8U))g_PDKHandle[PDK_BIOSIPSOURCE]) (pSetLanReq->ChannelNum & CHANNEL_ID_MASK);
	                 if(retValue == 1)
	                 {
		                 *pRes = CC_INV_DATA_FIELD;
		                 return sizeof (*pRes);
	                 }
	                 pBMCInfo->LANCfs[EthIndex].IPAddrSrc = pSetLanReq->ConfigData.IPAddrSrc ;
                }
                else
                {
                    pendStatus = GetPendStatus(PEND_OP_SET_SOURCE);
                    if(pendStatus == PEND_STATUS_PENDING)
                    {
                        *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                        return sizeof (INT8U);
                    }
                    pBMCInfo->LANCfs[EthIndex].IPAddrSrc = pSetLanReq->ConfigData.IPAddrSrc ;
                    NWConfig.CfgMethod = STATIC_IP_SOURCE;
                    SetPendStatus(PEND_OP_SET_SOURCE,PEND_STATUS_PENDING);
                    PostPendTask(PEND_OP_SET_SOURCE, (INT8U*) &NWConfig,sizeof(NWConfig), (pSetLanReq->ChannelNum & CHANNEL_ID_MASK), BMCInst );
                }
            }
            else if(pSetLanReq->ConfigData.IPAddrSrc == BMC_OTHER_SOURCE)
            {
                /*Perform OEM action*/
                if(g_PDKHandle[PDK_BMCOTHERSOURCEIP] != NULL)
                {
                	retValue = ((int(*)(INT8U))g_PDKHandle[PDK_BMCOTHERSOURCEIP]) (pSetLanReq->ChannelNum & CHANNEL_ID_MASK);
                     if(retValue == 1)
                     {
		                 *pRes = CC_INV_DATA_FIELD;
		                 return sizeof (*pRes);
	                 }
                     pBMCInfo->LANCfs[EthIndex].IPAddrSrc = pSetLanReq->ConfigData.IPAddrSrc ;
                }
                else
                {	
                     *pRes = CC_INV_DATA_FIELD;
	                 return sizeof (*pRes);
                }
            }  
            break;

        case LAN_PARAM_MAC_ADDRESS:
#if 0
            nwReadNWCfg  (&NWConfig);
            printf ( "The MAC is %x %x %x %x %x %x \n", NWConfig.MAC [0],NWConfig.MAC [1],NWConfig.MAC [2],NWConfig.MAC [3],NWConfig.MAC [4],NWConfig.MAC [5] );

            _fmemcpy (NWConfig.MAC, pSetLanReq->ConfigData.MACAddr, MAC_ADDR_LEN);
    	printf ( "The MAC is %x %x %x %x %x %x \n", NWConfig.MAC [0],NWConfig.MAC [1],NWConfig.MAC [2],NWConfig.MAC [3],NWConfig.MAC [4],NWConfig.MAC [5] );
            nwWriteNWCfg (&NWConfig);
#else
            /* According to IPMI 2.0 Specification Revision 3, the MAC address can be read only parameter*/
           //*pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
           //return sizeof (*pRes);

			if (!enableSetMACAddr)
				pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
			else
			{
				EnableSetMACAddress_T macAddrEnabled;
				INT8U InvalidMac[MAC_ADDR_LEN] = {0};
				if((pSetLanReq->ConfigData.MACAddr[0]& BIT0)|| (memcmp(&InvalidMac,&pSetLanReq->ConfigData.MACAddr,MAC_ADDR_LEN) == 0))
				{
					pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			
				memset(&macAddrEnabled, 0, sizeof(EnableSetMACAddress_T));
				macAddrEnabled.EthIndex = macEthIndex;
				memcpy(&macAddrEnabled.MACAddress, &pSetLanReq->ConfigData.MACAddr, MAC_ADDR_LEN);
			
                pendStatus = GetPendStatus(PEND_OP_SET_MAC_ADDRESS);
                if(pendStatus == PEND_STATUS_PENDING)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof (INT8U);
                }
				SetPendStatus(PEND_OP_SET_MAC_ADDRESS, PEND_STATUS_PENDING);
				PostPendTask(PEND_OP_SET_MAC_ADDRESS, (INT8U*)&macAddrEnabled, sizeof(EnableSetMACAddress_T), (pSetLanReq->ChannelNum & 0x0F), BMCInst);
			
				enableSetMACAddr = FALSE;
				macEthIndex = 0xFF;
				
				pSetLanRes->CompletionCode = CC_NORMAL;
			}
			
			return sizeof(*pSetLanRes);

#endif

        case LAN_PARAM_SUBNET_MASK:
            pendStatus = GetPendStatus(PEND_OP_SET_SUBNET);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            /*Returning valid completion code in case of attempt to set netmask in DHCP mode */
            if(pBMCInfo->LANCfs[EthIndex].IPAddrSrc == DHCP_IP_SOURCE)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
		
	    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
	    if(memcmp ( NWConfig.Mask, pSetLanReq->ConfigData.SubNetMask, IP_ADDR_LEN ) == 0)
	    {
		TCRIT("Same Mask, do nothing\n");
		_fmemcpy (pBMCInfo->LANCfs[EthIndex].SubNetMask, pSetLanReq->ConfigData.SubNetMask, IP_ADDR_LEN);
		break;
	    }

            if(IPAddrCheck(pSetLanReq->ConfigData.SubNetMask,LAN_PARAM_SUBNET_MASK))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }

                _fmemcpy (NWConfig.Mask, pSetLanReq->ConfigData.SubNetMask, IP_ADDR_LEN);
                SetPendStatus(PEND_OP_SET_SUBNET,PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_SUBNET,(INT8U*)&NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F) , BMCInst);
                _fmemcpy (pBMCInfo->LANCfs[EthIndex].SubNetMask, pSetLanReq->ConfigData.SubNetMask, IP_ADDR_LEN);

            break;

        case LAN_PARAM_IPv4_HEADER:
            pendStatus = GetPendStatus(PEND_OP_SET_IPV4_HEADERS);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            if(!pSetLanReq->ConfigData.Ipv4HdrParam.TimeToLive > 0)
            {
                IPMI_DBG_PRINT("The requested IPv4 header(TTL) to set is invalid.\n");
                *pRes = CC_PARAM_OUT_OF_RANGE;
                return sizeof(*pRes);
            }
            if(pSetLanReq->ConfigData.Ipv4HdrParam.IpHeaderFlags == 0x60) // Flags can be either of the values: DF(0x40) or MF(0x20)
            {
                IPMI_DBG_PRINT("The requested IPv4 header(Flags) to set is invalid.\n");
                *pRes = CC_PARAM_OUT_OF_RANGE;
                return sizeof(*pRes);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (&pBMCInfo->LANCfs[EthIndex].Ipv4HdrParam,
            &pSetLanReq->ConfigData.Ipv4HdrParam, sizeof(IPv4HdrParams_T));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            SetPendStatus(PEND_OP_SET_IPV4_HEADERS,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_IPV4_HEADERS, (INT8U*)&(pSetLanReq->ConfigData.Ipv4HdrParam),
            sizeof(pSetLanReq->ConfigData.Ipv4HdrParam),(pSetLanReq->ChannelNum & 0x0F),BMCInst);
            break;

        case LAN_PARAM_PRI_RMCP_PORT:
            pendStatus = GetPendStatus(PEND_RMCP_PORT_CHANGE);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            pBMCInfo->LANCfs[EthIndex].PrimaryRMCPPort = ipmitoh_u16 (pSetLanReq->ConfigData.PrimaryRMCPPort);
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            SetPendStatus(PEND_RMCP_PORT_CHANGE,PEND_STATUS_PENDING);
            PostPendTask(PEND_RMCP_PORT_CHANGE,(INT8U*)&(pSetLanReq->ConfigData.PrimaryRMCPPort),
                                    sizeof(pSetLanReq->ConfigData.PrimaryRMCPPort),(pSetLanReq->ChannelNum & 0x0F),BMCInst);
            break;

        case LAN_PARAM_SEC_RMCP_PORT:
            /* Returning Invalid error message */
            *pRes = CC_PARAM_NOT_SUPPORTED;
            return sizeof (INT8U);
            /*pPMConfig->LANConfig[EthIndex].SecondaryPort = ipmitoh_u16 (pSetLanReq->ConfigData.SecondaryPort);*/
            break;

        case LAN_PARAM_BMC_GENERATED_ARP_CONTROL:

        	currBmcGenArpCtrl = pBMCInfo->LANCfs[EthIndex].BMCGeneratedARPControl;
        	
			if(currBmcGenArpCtrl != pSetLanReq->ConfigData.BMCGeneratedARPControl)
				pBMCInfo->LANCfs[EthIndex].BMCGeneratedARPControl = pSetLanReq->ConfigData.BMCGeneratedARPControl;

			if((ENABLE_ARP_RESPONSES & currBmcGenArpCtrl) !=
			   (ENABLE_ARP_RESPONSES & pSetLanReq->ConfigData.BMCGeneratedARPControl))
			{
				UpdateArpStatus(EthIndex, BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning, BMCInst);
			}

			if(!(GRATIUTOUS_ENABLE_MASK & currBmcGenArpCtrl) &&
			   (GRATIUTOUS_ENABLE_MASK & pSetLanReq->ConfigData.BMCGeneratedARPControl))
			{
                /* Create a thread to Send Gratuitous ARP Packet */
                pLANArg = malloc(sizeof(BMCArg));
                pLANArg->BMCInst = BMCInst; 
                pLANArg->Len = sizeof(pSetLanReq->ChannelNum);
                pLANArg->Argument = malloc(pLANArg->Len);
                memcpy(pLANArg->Argument,(char *)&pSetLanReq->ChannelNum,pLANArg->Len);
               
                OS_CREATE_THREAD ((void *)GratuitousARPTask,(void *)pLANArg, NULL);
            }

            break;

        case LAN_PARAM_GRATITIOUS_ARP_INTERVAL:

            pBMCInfo->LANCfs[EthIndex].GratitousARPInterval =
                                    pSetLanReq->ConfigData.GratitousARPInterval;
            break;

        case LAN_PARAM_DEFAULT_GATEWAY_IP:
            pendStatus = GetPendStatus(PEND_OP_SET_GATEWAY);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
	    
            /*Returning valid completion code in case of attempt to set default gateway ip in DHCP mode */
            if(pBMCInfo->LANCfs[EthIndex].IPAddrSrc == DHCP_IP_SOURCE)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
	    if(memcmp ( NWConfig.Gateway, pSetLanReq->ConfigData.DefaultGatewayIPAddr, IP_ADDR_LEN ) == 0)
	    {
		TCRIT("Same Gateway, do nothing\n");
		_fmemcpy(pBMCInfo->LANCfs[EthIndex].DefaultGatewayIPAddr, pSetLanReq->ConfigData.DefaultGatewayIPAddr,IP_ADDR_LEN);
		break;
            }
            if(IPAddrCheck(pSetLanReq->ConfigData.DefaultGatewayIPAddr,LAN_PARAM_DEFAULT_GATEWAY_IP))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }

            _fmemcpy ((INT8U*)&GWIp,pSetLanReq->ConfigData.DefaultGatewayIPAddr, IP_ADDR_LEN);
            _fmemcpy ((INT8U*)&Subnetmask,&NWConfig.Mask[0],IP_ADDR_LEN);
            _fmemcpy ((INT8U*)&IPAddr,&NWConfig.IPAddr[0], IP_ADDR_LEN);
            /* Allowing  When the Default Gateway is Zero without validation to clear the Default Gateway */
            if(GWIp != 0)
            {
                _fmemcpy ((INT8U*)&Subnetmask,pBMCInfo->LANCfs[EthIndex].SubNetMask,IP_ADDR_LEN);
                _fmemcpy ((INT8U*)&IPAddr,pBMCInfo->LANCfs[EthIndex].IPAddr, IP_ADDR_LEN);
                if((IPAddr & Subnetmask ) != (GWIp & Subnetmask))
                {
                    IPMI_DBG_PRINT("\n Default GatewayIP to set is not valid \n");
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (INT8U);
                }
             }
            _fmemcpy (NWConfig.Gateway,pSetLanReq->ConfigData.DefaultGatewayIPAddr, IP_ADDR_LEN);
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy(pBMCInfo->LANCfs[EthIndex].DefaultGatewayIPAddr,
            pSetLanReq->ConfigData.DefaultGatewayIPAddr,IP_ADDR_LEN);
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            SetPendStatus(PEND_OP_SET_GATEWAY,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_GATEWAY,(INT8U*)&NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
             break;

        case LAN_PARAM_DEFAULT_GATEWAY_MAC:
            pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof(*pRes);
            break;

        case LAN_PARAM_BACKUP_GATEWAY_IP:

            nwReadNWCfg_v4_v6( &NWConfig,&NWConfig6, netindex,g_corefeatures.global_ipv6);
            _fmemcpy ((INT8U*)&GWIp,pSetLanReq->ConfigData.BackupGatewayIPAddr, IP_ADDR_LEN);
            _fmemcpy ((INT8U*)&Subnetmask,&NWConfig.Mask[0],IP_ADDR_LEN);
            _fmemcpy ((INT8U*)&IPAddr,&NWConfig.IPAddr[0], IP_ADDR_LEN);
            if(GWIp != 0)
            {
                if((IPAddr & Subnetmask ) != (GWIp & Subnetmask))
                {
                     IPMI_DBG_PRINT("\n Backup GatewayIP to set is not valid \n");
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (INT8U);
                }
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (pBMCInfo->LANCfs[EthIndex].BackupGatewayIPAddr,
            pSetLanReq->ConfigData.BackupGatewayIPAddr, IP_ADDR_LEN);
            nwSetBkupGWyAddr(pSetLanReq->ConfigData.BackupGatewayIPAddr,netindex);
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case LAN_PARAM_BACKUP_GATEWAY_MAC:

            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (pBMCInfo->LANCfs[EthIndex].BackupGatewayMACAddr,
                      pSetLanReq->ConfigData.BackupGatewayMACAddr, MAC_ADDR_LEN);
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case LAN_PARAM_COMMUNITY_STRING:
            if (g_PDKHandle[PDK_SETSNMPCOMMUNITYNAME] != NULL )
            {
                    if(((int(*)(INT8U *, INT8U,int))(g_PDKHandle[PDK_SETSNMPCOMMUNITYNAME]))(pSetLanReq->ConfigData.CommunityStr,MAX_COMM_STRING_SIZE, BMCInst)==0)
                        break;
            }
            OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->PefConfig.PEFSharedMemMutex, WAIT_INFINITE);
            _fmemcpy (pBMCInfo->LANCfs[EthIndex].CommunityStr,
                     pSetLanReq->ConfigData.CommunityStr, MAX_COMM_STRING_SIZE);
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
            break;

        case LAN_PARAM_DEST_NUM:

            pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof(INT8U);

        case LAN_PARAM_SELECT_DEST_TYPE:

            // if (pSetLanReq->ConfigData.DestType.SetSelect > NUM_LAN_DESTINATION)
            if (pSetLanReq->ConfigData.DestType.SetSelect > pBMCInfo->LANCfs[EthIndex].NumDest )
            {
                *pRes = CC_PARAM_OUT_OF_RANGE;
                return sizeof (*pRes);
            }

            if((pSetLanReq->ConfigData.DestType.DestType & 0x07) >= 1 && 
                (pSetLanReq->ConfigData.DestType.DestType & 0x07) <= 5)
            {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (INT8U);
            }

            if (0 == pSetLanReq->ConfigData.DestType.SetSelect)
            {
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (&pSharedMem->VolLANDestType[EthIndex],
                          &pSetLanReq->ConfigData.DestType, sizeof(LANDestType_T));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
            }
            else
            {
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (&pBMCInfo->LANCfs[EthIndex].DestType [pSetLanReq->ConfigData.DestType.SetSelect - 1],
                          &pSetLanReq->ConfigData.DestType, sizeof(LANDestType_T));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
            }
            break;

        case LAN_PARAM_SELECT_DEST_ADDR:
            pendStatus = GetPendStatus(PEND_OP_SET_GATEWAY);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            // if (pSetLanReq->ConfigData.DestAddr.SetSelect > NUM_LAN_DESTINATION)
            if (pSetLanReq->ConfigData.DestType.SetSelect > pBMCInfo->LANCfs[EthIndex].NumDest )
            {
                *pRes = CC_PARAM_OUT_OF_RANGE ;
                return sizeof (*pRes);
            }
            /*skip  the validate Ip if it is  0.0.0.0 as this is used for setting default  by PEF*/
            if (!((pSetLanReq->ConfigData.DestAddr.IPAddr[0] == 0) && (pSetLanReq->ConfigData.DestAddr.IPAddr[1] == 0) && (pSetLanReq->ConfigData.DestAddr.IPAddr[2] == 0) && (pSetLanReq->ConfigData.DestAddr.IPAddr[3] == 0)))
            {
                /*Validate the ip for reserved */
                if(IPAddrCheck(pSetLanReq->ConfigData.DestAddr.IPAddr, LAN_PARAM_SELECT_DEST_ADDR))
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }
            }
            if (0 == pSetLanReq->ConfigData.DestAddr.SetSelect)
            {
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (&pSharedMem->VolLANDest[EthIndex],
                          &pSetLanReq->ConfigData.DestAddr, sizeof(LANDestAddr_T));
               memset(pSharedMem->VolLANv6Dest,0,sizeof(LANDestv6Addr_T));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
            }
            else
            {
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (&pBMCInfo->LANCfs[EthIndex].DestAddr [pSetLanReq->ConfigData.DestAddr.SetSelect - 1],
                          &pSetLanReq->ConfigData.DestAddr, sizeof(LANDestAddr_T));
                memset( &pBMCInfo->LANCfs[EthIndex].Destv6Addr [pSetLanReq->ConfigData.Destv6Addr.SetSelect -1], 0 ,
                    sizeof(LANDestv6Addr_T));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
            }
            /* Setting BackupGw to DefaultGw as per request to send trap */
            if(pSetLanReq->ConfigData.DestAddr.GateWayUse == 1)
            {
                IPMI_DBG_PRINT("Setting Backupgw to Defaultgwip as per Request \n");
                nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(NWConfig.Gateway,pBMCInfo->LANCfs[EthIndex].BackupGatewayIPAddr,IP_ADDR_LEN);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                SetPendStatus(PEND_OP_SET_GATEWAY,PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_GATEWAY,(INT8U*)&NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F),BMCInst);

             }
            break;

        case LAN_PARAM_VLAN_ID:

            if( pBMCInfo->IpmiConfig.VLANIfcSupport == 1)
            {
                nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);

                vlanID = (pSetLanReq->ConfigData.VLANID & 0xfff);    /* get the vlan d from the data */
                
                if (vlanID > 4094)
                {
                	*pRes = CC_INV_DATA_FIELD ;
                	 return sizeof (*pRes);
                }
                if((pSetLanReq->ConfigData.VLANID & VLAN_MASK_BIT) == VLAN_MASK_BIT)    /* checks for VLAN enable bit*/
                {
                	if(vlanID==0)
                	{   		
                	   *pRes = CC_INV_DATA_FIELD ;
                	    return sizeof (*pRes);
                	                		                
                	}
                    pendStatus = GetPendStatus(PEND_OP_SET_VLAN_ID);
                    if(pendStatus == PEND_STATUS_PENDING)
                    {
                        *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                        return sizeof (INT8U);
                    }
                    

                    if ( NWConfig.VLANID != 0)     /* checks if vlan id already present */
                    {
                        if(NWConfig.VLANID == vlanID)
                        {
                            TCRIT("Currently configured vlan id and incoming set vlan id are same thus, do nothing\n");
                            break;
                        }
                        memset(IfcName,0,sizeof(IfcName));
                        if(GetIfcName(EthIndex, IfcName,BMCInst) != 0)
                        {
                            TCRIT("Error in getting Interface Name for the Lan Index :%d\n",EthIndex);
                            *pRes = CC_INV_DATA_FIELD ;
                            return sizeof (*pRes);
                        }
                        sprintf(VLANInterfaceName, "%s.%d", IfcName, (int)(NWConfig.VLANID));
                        if (0 == NwInterfacePresenceCheck (VLANInterfaceName))
                        {
                            pendStatus = GetPendStatus(PEND_OP_SET_VLAN_ID);
                            if(pendStatus == PEND_STATUS_PENDING)
                            {
                                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                                return sizeof (INT8U);
                            }
                            SetPendStatus(PEND_OP_DECONFIG_VLAN_IFC,PEND_STATUS_PENDING);
                            PostPendTask(PEND_OP_DECONFIG_VLAN_IFC,(INT8U*)&NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
                        }
                    }

                    NWConfig.VLANID=vlanID;
                    SetPendStatus(PEND_OP_SET_VLAN_ID,PEND_STATUS_PENDING);
                    PostPendTask(PEND_OP_SET_VLAN_ID,(INT8U*)&NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
                    pBMCInfo->LANCfs[EthIndex].VLANID = pSetLanReq->ConfigData.VLANID;
                }
                else        /* Vlan Bit is Disabled */
                {
                    if(NWConfig.VLANID==0)        /* Vlan id is disabled */
                    {
                        if((pSetLanReq->ConfigData.VLANID & 0xfff)!=0)
                        {
                            pBMCInfo->LANCfs[EthIndex].VLANID = pSetLanReq->ConfigData.VLANID;
                        }

                        if((pSetLanReq->ConfigData.VLANID & 0xfff)==0)
                        {
                            if((pBMCInfo->LANCfs[EthIndex].VLANID & 0xfff)!=0)
                            {
                                pBMCInfo->LANCfs[EthIndex].VLANID = pSetLanReq->ConfigData.VLANID;
                            }
                        }
                    }

                    else                /* Vlan ID is enable. so deconfigure it */
                    {
                        memset(IfcName,0,sizeof(IfcName));
                        if(GetIfcName(EthIndex, IfcName,BMCInst) != 0)
                        {
                            TCRIT("Error in getting Interface Name for the Lan Index :%d\n",EthIndex);
                            *pRes = CC_INV_DATA_FIELD ;
                            return sizeof (*pRes);
                        }
                        sprintf(VLANInterfaceName, "%s.%d", IfcName, (int)(NWConfig.VLANID));
                        if (0 == NwInterfacePresenceCheck (VLANInterfaceName))
                        {
                            pendStatus = GetPendStatus(PEND_OP_SET_VLAN_ID);
                            if(pendStatus == PEND_STATUS_PENDING)
                            {
                                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                                return sizeof (INT8U);
                            }
                            SetPendStatus(PEND_OP_DECONFIG_VLAN_IFC,PEND_STATUS_PENDING);
                            PostPendTask(PEND_OP_DECONFIG_VLAN_IFC,(INT8U*)&NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
                            pBMCInfo->LANCfs[EthIndex].VLANPriority =0;
                        }
                        pendStatus = GetPendStatus(PEND_OP_SET_SOURCE);
                        if(pendStatus == PEND_STATUS_PENDING)
                        {
                            *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                            return sizeof (INT8U);
                        }
                        //NWConfig.VLANID=0;
                        NWConfig.CfgMethod = pBMCInfo->LANCfs[EthIndex].IPAddrSrc;
                        SetPendStatus(PEND_OP_SET_SOURCE,PEND_STATUS_PENDING);
                        PostPendTask(PEND_OP_SET_SOURCE,(INT8U*) &NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
                        pBMCInfo->LANCfs[EthIndex].VLANID = pSetLanReq->ConfigData.VLANID;
                    }

                }
            }
            else
            {
                pSetLanRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof(INT8U);
            }
        break;

        case LAN_PARAM_VLAN_PRIORITY:

            if( pBMCInfo->IpmiConfig.VLANIfcSupport == 1)
            {
                if((pBMCInfo->LANCfs[EthIndex].VLANID & VLAN_MASK_BIT) != VLAN_MASK_BIT)    /* checks for VLAN enable bit*/
                {
                    if(g_corefeatures.vlan_priorityset == ENABLED)
                    {
                        if(pSetLanReq->ConfigData.VLANPriority > 7 )
                        {
                            TCRIT(" VLAN Priority value should be 0-7 \n");
                            *pRes = CC_INV_DATA_FIELD ;
                            return sizeof (*pRes);
                        }
                        if(ReadVLANFile(VLAN_PRIORITY_SETTING_STR, PriorityLevel) == -1)
                        {
                            return -1;
                        }
                        if(WriteVLANFile(VLAN_PRIORITY_SETTING_STR, PriorityLevel, netindex,pSetLanReq->ConfigData.VLANPriority) == -1)
                        {
                            return -1;
                        }
                        pBMCInfo->LANCfs[EthIndex].VLANPriority = pSetLanReq->ConfigData.VLANPriority;
                    }
                    else
                    {
                        TCRIT(" VLAN is not Configured \n");
                        *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                        return sizeof (*pRes);
                    }
                }
                else
                {
                    if(pSetLanReq->ConfigData.VLANPriority > 7 )
                    {
                        TCRIT(" VLAN Priority value should be 0-7 \n");
                        *pRes = CC_INV_DATA_FIELD ;
                        return sizeof (*pRes);
                    }

                    if(ReadVLANFile(VLAN_PRIORITY_SETTING_STR, PriorityLevel) == -1)
                    {
                        return -1;
                    }

                    if(WriteVLANFile(VLAN_PRIORITY_SETTING_STR, PriorityLevel, netindex,pSetLanReq->ConfigData.VLANPriority) == -1)
                    {
                        return -1;
                    }

                    memset(IfcName,0,sizeof(IfcName));
                    if(GetIfcName(EthIndex, IfcName,BMCInst) != 0)
                    {
                         TCRIT("Error in getting Interface Name for the Lan Index :%d\n",EthIndex);
                         *pRes = CC_INV_DATA_FIELD;
                         return sizeof(*pRes);
                    }
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                    pBMCInfo->LANCfs[EthIndex].VLANPriority = pSetLanReq->ConfigData.VLANPriority;
                    NWConfig.vlanpriority = pSetLanReq->ConfigData.VLANPriority;
                    SetPendStatus(PEND_OP_SET_VLAN_PRIORITY,PEND_STATUS_PENDING);
                    PostPendTask(PEND_OP_SET_VLAN_PRIORITY,(INT8U*)&NWConfig,sizeof(NWConfig),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
                }
            }
            else
            {
                 pSetLanRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                 return sizeof(INT8U);
            }
        break;


        case LAN_PARAM_CIPHER_SUITE_ENTRY_SUP:
            pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof(INT8U);

        case LAN_PARAM_CIPHER_SUITE_ENTRIES:
            pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof(INT8U);
            break;

        case LAN_PARAM_CIPHER_SUITE_PRIV_LEVELS:
            //According to IPMI spec
            //We can not support more than 16 cipher suites at a time
            //But these cipher suites ID can be 0 to BFh
            LOCK_BMC_SHARED_MEM(BMCInst);
            //Flush previous values, about to set new one
            _fmemset ( pBMCInfo->LANCfs[EthIndex].CipherSuitePrivLevels, 0,
                       MAX_NUM_CIPHER_SUITE_PRIV_LEVELS);

            //copy the reserve byte
            pBMCInfo->LANCfs[EthIndex].CipherSuitePrivLevels[0] = pSetLanReq->ConfigData.CipherSuitePrivLevels[0];

            //Retrieve the nibble wise privileges into bytes and store it in a temporary area
            for( i=1, j=1 ; i<= N0_OF_CIPHER_SUITE_SUPPORTED ; i=i+2, j++)
            {
                //Copy the higher nibble
                CipherSuitePrivilegelevels [i] = ((pSetLanReq->ConfigData.CipherSuitePrivLevels[j] >> 4) & 0x0f);
                //Copy the lower nibble
                CipherSuitePrivilegelevels [i-1] = (pSetLanReq->ConfigData.CipherSuitePrivLevels[j] & 0x0f);
            }

            //if maximum cipher suite supported are an even number
            //then last supported cipher suite will not be processed by the above loop
            //So, copy the privilege of the last supported cipher suite in the temporary area
            if( (i-2) != N0_OF_CIPHER_SUITE_SUPPORTED )
            {
                CipherSuitePrivilegelevels [i-1] = (pSetLanReq->ConfigData.CipherSuitePrivLevels[j] & 0x0f);
            }

            //Now copy the privilege bytes into BMC info nibble wise
            for( i=0; i<N0_OF_CIPHER_SUITE_SUPPORTED; i++)
            {
                //retrieve the cipher suite ID
                CipherSuiteID = g_CipherRec[(i * 5) + 1];
                //Copy in the BMC info nibble wise
                //if ID is even then copy in lower nibble
                //else copy in higher nibble
                pBMCInfo->LANCfs[EthIndex].CipherSuitePrivLevels [CipherSuiteID/2 + 1] = pBMCInfo->LANCfs[EthIndex].CipherSuitePrivLevels [CipherSuiteID/2 + 1] |
                           ( ((CipherSuiteID % 2) == 0) ? (CipherSuitePrivilegelevels [i] & 0x0f) : ((CipherSuitePrivilegelevels[i] << 4) & 0xf0) );
            }
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            break;

        case LAN_PARAM_VLAN_TAGS:
			AddrFormat = *(((INT8U*)&pSetLanReq->ConfigData) + 1)>> 4;
			TDBG("AddrFormat = %d\n", AddrFormat);
			if(AddrFormat > 0x01)
			{
				TDBG("Invalid Address Format\n");
				*pRes = CC_INV_DATA_FIELD;
				return sizeof(INT8U);
			}
			
			vlanID = *(((INT16U*)&pSetLanReq->ConfigData) + 1);
			// The VLANID obtained above is in the format specified in IPMIv2.0 spec - Table 23-4 LAN Configuration Parameters .
			// Parameter Name is "802.1q VLAN ID (12-bit)".
			vLANLSB = vlanID & 0xFF00;
			vLANMSB = vlanID & 0x00FF;
			vLANLSB = vLANLSB >> 8;
			vLANMSB = vLANMSB << 8;
			vlanID = vLANMSB | vLANLSB;
			TDBG("vlanID = %d, %x \n", vlanID, vlanID);

			/* check VLANID only when address format bit is 1h*/
			if(AddrFormat == 0x01)
			{
				
				// As per IPMIv2.0 spec, VLANID is of length 12-bits and ranges from 0 to 4095 
				if(vlanID == 0 || vlanID == 1 || vlanID == 4095 || vlanID > 4095) 
				{
					TDBG("Invalid VLAN ID\n");
					*pRes = CC_INV_DATA_FIELD;
					return sizeof(INT8U);
				}
			}
        	if (0 == pSetLanReq->ConfigData.DestAddr.SetSelect)
          {
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(&pSharedMem->VLANDestTag,
                         ((INT8U*)&pSetLanReq->ConfigData) + 1, sizeof(VLANDestTags_T));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
          }
          else
          {
                if (pSetLanReq->ConfigData.DestAddr.SetSelect > pBMCInfo->LANCfs[EthIndex].NumDest)
                {
                    pSetLanRes->CompletionCode = CC_PARAM_OUT_OF_RANGE;
                    return sizeof (INT8U);
                }
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(&pBMCInfo->LANCfs[EthIndex].VLANDestTags [pSetLanReq->ConfigData.DestAddr.SetSelect - 1],
                         ((INT8U*)&pSetLanReq->ConfigData) + 1, sizeof(VLANDestTags_T));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
          }
            break;

        case LAN_PARAMS_BAD_PASSWORD_THRESHOLD:
            ClearUserLockAttempts(LAN_CHANNEL_BADP,BMCInst);
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy(&pBMCInfo->LANCfs[EthIndex].BadPasswd,
                            &pSetLanReq->ConfigData.BadPasswd,sizeof(BadPassword_T));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case LAN_PARAMS_IPV6_DHCPV6_TIMING_CONF_SUPPORT:
            pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof(INT8U);

        case LAN_PARAMS_IPV6_DHCPV6_TIMING_CONFIGURATION:
            /*  Since the Timing Parameter is 1 byte, We cant give more than 255. So we will use below formula to calculate actual value.
                This will be doing in PendTask.
                Timing Parameter value=(Requested _value -1)*granularity+minimum_value[Refer IPMI SPec]
            */

            /* check IPv6/IPv4 Addressing enables is 0*/
            if (pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable == 0)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /*if DHCPv6 Dynamic Address configuration is not supported, this parameter should not be implemented.*/
            if((pBMCInfo->LANCfs[EthIndex].IPv6Status.SLAAC_DHCPv6Addressing & 0x01) == 0)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof (INT8U);
            }

            /*Check DHCPv6 timing configuration is supported*/
            if(pBMCInfo->LANCfs[EthIndex].IPv6_DHCPv6TimingConfSupport == 0x00)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof (INT8U);
            }

            /*Check the Request Length.[ReqLen=1(set selector)+1(block selector)+16(Timing parameters)]*/
            if ( ReqLen != (MAX_IPV6_BLOCK_SIZE+2) )
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof (INT8U);
            }

            /*Check PendStatus*/
            pendStatus = GetPendStatus(PEND_OP_SET_DHCPV6_TIMING_CONF);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /*If DHCPv6 Timing Configuration is "Global" then Set selector should be 0
            If it is "Per Interface" then Set selector should not exceed Number of IA(interface)*/
            if(pBMCInfo->LANCfs[EthIndex].IPv6_DHCPv6TimingConfSupport == 0x01)
            {
                if(pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.SetSelect != 0)
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }
            }
            else if(pBMCInfo->LANCfs[EthIndex].IPv6_DHCPv6TimingConfSupport == 0x02)
            {
                memset(InterfaceName,0,sizeof(InterfaceName));
                noofinterface=GetActiveIfcnameAndNoofIfc(InterfaceName);
                if(noofinterface == -1)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof (INT8U);
                }

                if(pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.SetSelect >= noofinterface)
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }
            }


            /*  In Block1, Maximum value of REQ_MAX_RC [ Max Request retry attempts]  is 100
                Maximum value of CNF_MAX_RT[ Max Confirm timeout] is 127
                Offset value of REQ_MAX_RC = 5
                Offset value of CNF_MAX_RT = 8

                All timing paramer value from offset 6 in Block2 should be 0
            */
            if(pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.BlockSelect == 0)
            {
                if((pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.TimingConf[5] > 100) ||
                    (pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.TimingConf[8] > 127))
                {
                      *pRes = CC_INV_DATA_FIELD ;
                      return sizeof (*pRes);
                }
            }
            else if(pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.BlockSelect == 1)
            {
                for(i=6;i<MAX_IPV6_BLOCK_SIZE;i++)
                {
                    if(pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.TimingConf[i] != 0)
                    {
                        *pRes = CC_INV_DATA_FIELD ;
                        return sizeof (*pRes);
                    }
                }
            }
            else
            {
                *pRes = CC_INV_DATA_FIELD ;
                return sizeof (*pRes);
            }

            memset(&pBMCInfo->LANCfs[EthIndex].DHCPv6TimingConf[pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.SetSelect][pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.BlockSelect*MAX_IPV6_BLOCK_SIZE],0,MAX_IPV6_BLOCK_SIZE);
            memcpy(&pBMCInfo->LANCfs[EthIndex].DHCPv6TimingConf[pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.SetSelect][pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.BlockSelect*MAX_IPV6_BLOCK_SIZE],
            &pSetLanReq->ConfigData.IPv6_DHCPv6TimingConf.TimingConf,MAX_IPV6_BLOCK_SIZE);

            SetPendStatus(PEND_OP_SET_DHCPV6_TIMING_CONF,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_DHCPV6_TIMING_CONF,NULL,0,(pSetLanReq->ChannelNum & 0x0F), BMCInst );
         break;


        case LAN_PARAMS_IPV6_SLAAC_TIMING_CONF_SUPPORT:
            pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof(INT8U);
            break;

        case LAN_PARAMS_IPV6_SLAAC_TIMING_CONFIGURATION:
            /*  Since the Timing Parameter is 1 byte, We cant give more than 255. So we will use below formula to calculate actual value.
                This will be doing in PendTask.
                Timing Parameter value=(Requested _value -1)*granularity+minimum_value[Refer IPMI SPec]
            */

            /* check IPv6/IPv4 Addressing enables is 0*/
            if (pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable == 0)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /*if SLAAC  Address configuration is not supported, this parameter should not be implemented.*/
            if(((pBMCInfo->LANCfs[EthIndex].IPv6Status.SLAAC_DHCPv6Addressing & 0x02) >> 1) == 0)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof (INT8U);
            }

            /* Check Neighbor Discovery / SLAAC Timing configuration is supported */
            if(pBMCInfo->LANCfs[EthIndex].IPv6_SLAACTimingConfSupport == 0x00)
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof (INT8U);
            }

            /*Check the Request Length.[ReqLen=1(set selector)+1(block selector)+16(Timing parameters)]*/
            if ( ReqLen != (MAX_IPV6_BLOCK_SIZE+2) )
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof (INT8U);
            }

            /*Check PendStatus*/
            pendStatus = GetPendStatus(PEND_OP_SET_SLAAC_TIMING_CONF);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /*If SLAAC Timing Configuration is "Global" then Set selector should be 0
            If it is "Per Interface" then Set selector should not exceed Number of IA(interface)*/
            if(pBMCInfo->LANCfs[EthIndex].IPv6_SLAACTimingConfSupport == 0x01)
            {
                if(pSetLanReq->ConfigData.IPv6_SLAACTimingConf.SetSelect != 0)
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }
            }
            else if(pBMCInfo->LANCfs[EthIndex].IPv6_SLAACTimingConfSupport == 0x02)
            {
                memset(InterfaceName,0,sizeof(InterfaceName));
                noofinterface=GetActiveIfcnameAndNoofIfc(InterfaceName);
                if(noofinterface == -1)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof (INT8U);
                }

                if(pSetLanReq->ConfigData.IPv6_SLAACTimingConf.SetSelect >= noofinterface)
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }
            }

            /*Only one block is supported*/
            if(pSetLanReq->ConfigData.IPv6_SLAACTimingConf.BlockSelect > 0)
            {
                *pRes = CC_INV_DATA_FIELD ;
                return sizeof (*pRes);
            }
            else
            {
                /*  Offset of MAX_RTR_SOLICITATIONS = 2
                    Offset of DupAddrDetectTransmits = 3
                    Offset of MAX_MULTICAST_SOLICIT = 4
                    Offset of MAX_UNICAST_SOLICIT = 5
                    These Parameters are counts and its maximum value is 100
                */
                for(i=2;i<=5;i++)
                {
                    if(pSetLanReq->ConfigData.IPv6_SLAACTimingConf.TimingConf[i] > 100)
                    {
                        *pRes = CC_INV_DATA_FIELD ;
                        return sizeof (*pRes);
                    }
                }

                /*  Parameter MAX_RANDOM_FACTOR (Offset= 11)and MIN_RANDOM_FACTOR(Offset=12) is optional
                    and currently not supported.and values should be 0 for unsupoprted parameters
                */
                for(i=11;i<MAX_IPV6_BLOCK_SIZE;i++)
                {
                    if(pSetLanReq->ConfigData.IPv6_SLAACTimingConf.TimingConf[i] != 0)
                    {
                        *pRes = CC_INV_DATA_FIELD ;
                        return sizeof (*pRes);
                    }
                }
            }

            memset(&pBMCInfo->LANCfs[EthIndex].SLAACTimingConf[pSetLanReq->ConfigData.IPv6_SLAACTimingConf.SetSelect][pSetLanReq->ConfigData.IPv6_SLAACTimingConf.BlockSelect*MAX_IPV6_BLOCK_SIZE],0,MAX_IPV6_BLOCK_SIZE);
            memcpy(&pBMCInfo->LANCfs[EthIndex].SLAACTimingConf[pSetLanReq->ConfigData.IPv6_SLAACTimingConf.SetSelect][pSetLanReq->ConfigData.IPv6_SLAACTimingConf.BlockSelect*MAX_IPV6_BLOCK_SIZE],
            &pSetLanReq->ConfigData.IPv6_SLAACTimingConf.TimingConf,MAX_IPV6_BLOCK_SIZE);

            SetPendStatus(PEND_OP_SET_SLAAC_TIMING_CONF,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_SLAAC_TIMING_CONF,NULL,0,(pSetLanReq->ChannelNum & 0x0F), BMCInst );
         break;


        case LAN_PARAM_IPV6_IPV4_SUPPORT:
            if(ReqLen>1)
            {
                pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                return sizeof(*pSetLanRes);
            }
            /* Read Only Parameter */
            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (INT8U);
            break;

        case LAN_PARAM_IPV6_HEADER_STATIC_TRAFFIC_CLASS:
            if((g_corefeatures.global_ipv6))
            {
                if(ReqLen>1)
                {
                    pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                    return sizeof(*pSetLanRes);
                }
                pendStatus = GetPendStatus(PEND_OP_SET_IPV6_HEADERS);
                if(pendStatus == PEND_STATUS_PENDING)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof (INT8U);
                }

                /*Check the reserved bit As per the RFC 2474 6th bit and 7th bit are reserved*/
                if((pSetLanReq->ConfigData.TrafficClass >> 6 & 0x01) ||
                    (pSetLanReq->ConfigData.TrafficClass >> 7 & 0x01))
                {
                    pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                    return sizeof(*pSetLanRes);
                }

                /*Check the previous data and requested are same*/
                if(pBMCInfo->LANCfs[EthIndex].TrafficClass ==pSetLanReq->ConfigData.TrafficClass)
                {
                    IPMI_DBG_PRINT("IPV6 Traffic Class: The Current and the previous traffic class is the same.\n");
                    break;
                }

                LOCK_BMC_SHARED_MEM(BMCInst);
                pBMCInfo->LANCfs[EthIndex].TrafficClass=pSetLanReq->ConfigData.TrafficClass;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                SetPendStatus(PEND_OP_SET_IPV6_HEADERS,PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_IPV6_HEADERS, (INT8U*)&(pSetLanReq->ConfigData.TrafficClass),
                sizeof(pSetLanReq->ConfigData.TrafficClass),(pSetLanReq->ChannelNum & 0x0F),BMCInst);
            }
            else
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            break;

        case LAN_PARAM_IPV6_HEADER_STATIC_HOP_LIMIT:
            if((g_corefeatures.global_ipv6))
            {
                if(ReqLen>1)
                {
                    pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                    return sizeof(*pSetLanRes);
                }
                if (pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable == 0)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof (INT8U);
                }


                pendStatus = GetPendStatus(PEND_OP_SET_IPV6_HEADERS);
                if(pendStatus == PEND_STATUS_PENDING)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof (INT8U);
                }

                /*Check the previous data and requested are same*/
                if(pBMCInfo->LANCfs[EthIndex].HopLimit==pSetLanReq->ConfigData.HopLimit)
                {
                    IPMI_DBG_PRINT("IPV6 Traffic Class: The Current and the previous traffic class is the same.\n");
                    break;
                }

                LOCK_BMC_SHARED_MEM(BMCInst);
                pBMCInfo->LANCfs[EthIndex].HopLimit=pSetLanReq->ConfigData.HopLimit;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                SetPendStatus(PEND_OP_SET_IPV6_HEADERS,PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_IPV6_HEADERS, (INT8U*)&(pSetLanReq->ConfigData.HopLimit),
                sizeof(pSetLanReq->ConfigData.HopLimit),(pSetLanReq->ChannelNum & 0x0F),BMCInst);

            }
            else
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            break;

        case LAN_PARAM_IPV6_HEADER_FLOW_LABEL:
            if(ReqLen>1)
            {
                pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                return sizeof(*pSetLanRes);
            }
            /* Not allowed to set since RFC 2460 , section-6 says, Flow label is in experimental stage*/
            *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
            return(sizeof (INT8U));
            break;

        case LAN_PARAM_IPV6_STATUS:
            /* Read Only Parameter */
            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (INT8U);
            break;

        case LAN_PARAM_IPV6_IPV4_ADDRESS_ENABLE:

            if((g_corefeatures.global_ipv6) )
            {
                if (0x1 != ReqLen)
                {
                    pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                    return sizeof(*pSetLanRes);
                }

                if (pSetLanReq->ConfigData.IPv6IPv4AddrEnable > 0x2)
                {
                    pSetLanRes->CompletionCode = CC_INV_DATA_FIELD; 
                    return sizeof(*pSetLanRes);
                }

                EthIndex= GetEthIndex(pSetLanReq->ChannelNum & 0x0F, BMCInst);
                if(0xff == EthIndex)
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (INT8U);
                }

                if(pBMCInfo->BondConfig.Enable == 1)
                {
                    snprintf(IfcName,sizeof(IfcName),"bond%d",pBMCInfo->BondConfig.BondIndex);
                    if(strcmp(IfcName, pBMCInfo->LanIfcConfig[EthIndex].ifname) == 0)
                    {
                        EthIndex = pBMCInfo->LanIfcConfig[EthIndex].Ethindex;
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

                memset(&NWConfig, 0, sizeof(NWCFG_STRUCT));
                memset(&NWConfig6, 0, sizeof(NWCFG6_STRUCT));
                retValue = nwReadNWCfg_v4_v6(&NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);
                if(retValue != 0)
                    TCRIT("Error in reading network configuration.\n");  

                if(EABLE_V6_DISABLE_V4 == pSetLanReq->ConfigData.IPv6IPv4AddrEnable)
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

                if(pSetLanReq->ConfigData.IPv6IPv4AddrEnable==pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable)
                {
                    TCRIT("Iface state is the same, do nothing\n");         
                    *pRes = CC_SUCCESS;
                }

                switch((pSetLanReq->ConfigData.IPv6IPv4AddrEnable) & (0x3))
                {
                    case DISABLE_V6:
                        pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable=0;
                        pBMCInfo->LANCfs[EthIndex].IPv4_Enable=1;
                        pBMCInfo->LANCfs[EthIndex].IPv6_Enable=0;
                        pBMCInfo->LANCfs[EthIndex].IPv6IPv4Support |= 0x02; // Setting BIT 1
                        break;
                    case EABLE_V6_DISABLE_V4:
                        pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable=0x1;
                        pBMCInfo->LANCfs[EthIndex].IPv4_Enable=0;
                        pBMCInfo->LANCfs[EthIndex].IPv6_Enable=1;

                        //[0] - 1b = Implementation can be configured to use IPv6 addresses only.
                        pBMCInfo->LANCfs[EthIndex].IPv6IPv4Support |= 0x01; // Setting BIT 0
                        pBMCInfo->LANCfs[EthIndex].IPv6IPv4Support &= 0xFD; // Unsetting BIT 1
                        break;
                    case ENABLE_V6_V4:
                        pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable=0x2;
                        pBMCInfo->LANCfs[EthIndex].IPv4_Enable=1;
                        pBMCInfo->LANCfs[EthIndex].IPv6_Enable=1;

                        //[1] - 1b = Implementation can be configured to use both IPv4 and IPv6 addresses simultaneously.
                        pBMCInfo->LANCfs[EthIndex].IPv6IPv4Support |= 0x02; // Setting BIT 1
                        pBMCInfo->LANCfs[EthIndex].IPv6IPv4Support &= 0xFE; // Unsetting BIT 0
                        break;
                }

                SetPendStatus(PEND_OP_SET_IPV6_IPV4_ADDR_ENABLE,PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_IPV6_IPV4_ADDR_ENABLE, (INT8U *)&pSetLanReq->ConfigData.IPv6IPv4AddrEnable, sizeof(pSetLanReq->ConfigData.IPv6IPv4AddrEnable),(pSetLanReq->ChannelNum & 0xF),BMCInst);

                FlushIPMI((INT8U*)&pBMCInfo->LANCfs[0], (INT8U*)&pBMCInfo->LANCfs[EthIndex], 
                pBMCInfo->IPMIConfLoc.LANCfsAddr, sizeof(LANConfig_T),BMCInst);
            }
            else
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            break;
        case  LAN_PARAM_IPV6_STATIC_ADDRESS:
            if((g_corefeatures.global_ipv6) )
            {

                if(19 != ReqLen)
                {
                    pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                    return sizeof(*pSetLanRes);
                }
                pendStatus = GetPendStatus(PEND_OP_SET_IPV6_STATIC_IP_ADDR);
                if (pendStatus == PEND_STATUS_PENDING)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof (INT8U);
                }

                if (( pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable == 0))
                {
                    TCRIT(" IPv6 is not enabled yet... \n");
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof (*pSetLanRes);
                    break;
                }
                /* Do Bond Mode Check*/
                if(g_corefeatures.bond_support == ENABLED)
                {
                    if((pBMCInfo->BondConfig.Enable) && ( pBMCInfo->BondConfig.BondMode != BOND_ACTIVE_BACKUP) )
                    {
                        TCRIT("Bonding Interface is not in Active Back_Up Mode...\n");
                        *pRes = CC_BOND_IFC_NOT_IN_ACTIVE_BACK_UP;
                        return sizeof (INT8U);
                    }
                }

                /*Validate the SET SELECTOR  value*/
                if(pSetLanReq->ConfigData.IPv6Addrs.SetSelector > 0x0)
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }

                if((pSetLanReq->ConfigData.IPv6Addrs.IPv6_AddrSrcType & 0x80))
                {
                    pBMCInfo->LANCfs[EthIndex].IPv6Addrs[0].IPv6_AddrSrcType=STATIC_IP_SOURCE;
                    pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc=STATIC_IP_SOURCE;
                }
                else
                {
                    pBMCInfo->LANCfs[EthIndex].IPv6Addrs[0].IPv6_AddrSrcType=DHCP_IP_SOURCE;
                    pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc=DHCP_IP_SOURCE;
                }

                /*validation for reserver bits*/
                if(pSetLanReq->ConfigData.IPv6Addrs.IPv6_AddrSrcType & 0x7F)
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }


                if(pSetLanReq->ConfigData.IPv6Addrs.IPv6_AddrSrcType == DHCP_IP_SOURCE )
                {
                    /*Validate the IPv6 address*/
                    if(0 != IsValidGlobalIPv6Addr((struct in6_addr*)&pSetLanReq->ConfigData.IPv6Addrs.IPv6_Address))
                    {
                        TCRIT("Invalid Global IPv6 Address\n");
                        *pRes = CC_INV_DATA_FIELD;
                        return sizeof(INT8U);
                    }
                }


                /*Validate the duplicate IPv6 Address*/
                nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                if(pSetLanReq->ConfigData.IPv6Addrs.IPv6_AddrSrcType == DHCP_IP_SOURCE )
                {

                    for(i=0;i<1;i++) //Cuurently supporting only one Static IPv6 Address
                    {
                        /*Use can set the same IPv6 address for same index. To satisfy the WebServer*/
                        if((memcmp(NWConfig6.GlobalIPAddr[i],pSetLanReq->ConfigData.IPv6Addrs.IPv6_Address,IP6_ADDR_LEN) == 0) && pSetLanReq->ConfigData.IPv6Addrs.SetSelector != i)
                        {
                            *pRes = CC_INV_DATA_FIELD;
                            return sizeof(INT8U);
                        }
                    }
                }

                NWConfig6.CfgMethod = pBMCInfo->LANCfs[EthIndex].IPv6Addrs[0].IPv6_AddrSrcType;
                if(pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc==STATIC_IP_SOURCE)
                {
                    _fmemcpy(&pBMCInfo->LANCfs[EthIndex].IPv6Addrs[pSetLanReq->ConfigData.IPv6Addrs.SetSelector].IPv6_Address,&pSetLanReq->ConfigData.IPv6Addrs.IPv6_Address, IP6_ADDR_LEN);
                    _fmemcpy(&pBMCInfo->LANCfs[EthIndex].IPv6_IPAddr[pSetLanReq->ConfigData.IPv6Addrs.SetSelector],&pSetLanReq->ConfigData.IPv6Addrs.IPv6_Address,IP6_ADDR_LEN);
                    pBMCInfo->LANCfs[EthIndex].IPv6Addrs[pSetLanReq->ConfigData.IPv6Addrs.SetSelector].IPv6_PrefixLength= pSetLanReq->ConfigData.IPv6Addrs.IPv6_PrefixLength;
                    pBMCInfo->LANCfs[EthIndex].IPv6_PrefixLen[pSetLanReq->ConfigData.IPv6Addrs.SetSelector] = pSetLanReq->ConfigData.IPv6Addrs.IPv6_PrefixLength;
                }

                SetPendStatus(PEND_OP_SET_IPV6_STATIC_IP_ADDR,PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_IPV6_STATIC_IP_ADDR, (INT8U*) &(pSetLanReq->ConfigData.IPv6Addrs),\
                sizeof (IPv6Addrs_T),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            }
            else
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            break;

        case LAN_PARAM_IPV6_DYNAMIC_ADDRESS:
            /* Read Only Parameter*/
            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (INT8U);

        case LAN_PARAMS_IPV6_ROUTER_ADDR_CONF_CNTL:
            if ((ReqLen != 1) ||
                (pSetLanReq->ConfigData.IPv6_RA_Conf_Cntl_Enable & 0xFC))
            {
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
                break;
            }

            /* check for IPv6 enable state */
            if ( 0x1 != pBMCInfo->LANCfs[EthIndex].IPv6_Enable)
            {
                TCRIT("IPv6 is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            LOCK_BMC_SHARED_MEM(BMCInst);
            pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable = 
                        pSetLanReq->ConfigData.IPv6_RA_Conf_Cntl_Enable;
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case LAN_PARAMS_IPV6_STATIC_ROUTER1_IPADDR:
            TDBG("Entered in LAN_PARAMS_IPV6_STATIC_ROUTER1_IPADDR \n");
            if ( ReqLen != IP6_ADDR_LEN )
            {
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
           	}

            pendStatus = GetPendStatus(PEND_OP_SET_RA_IPADDR);
            if (pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            
            /* check for Static Router Adv enable state */
            if (!( 0x1 & pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable))
            {
                TCRIT(" Static Router Adv is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /*Validate the IPv6 address*/
            if(0 != IsValidGlobalIPv6Addr((struct in6_addr*)&pSetLanReq->ConfigData.IPv6_Router1_IPAddr))
            {
                TCRIT("Invalid Global IPv6 Address\n");
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_Router1_IPAddr,
                        pSetLanReq->ConfigData.IPv6_Router1_IPAddr, IP6_ADDR_LEN );
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            RAipV6Addr.RAipV6_Cnt = 0x1;
            _fmemcpy(RAipV6Addr.RAipV6_IPAddr,
                        pSetLanReq->ConfigData.IPv6_Router1_IPAddr, IP6_ADDR_LEN );

            SetPendStatus(PEND_OP_SET_RA_IPADDR,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_RA_IPADDR, (INT8U*)&RAipV6Addr,
                                    sizeof(RAipV6Addr_T),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            break;

        case LAN_PARAMS_IPV6_STATIC_ROUTER1_MACADDR:
            if ( ReqLen != MAC_ADDR_LEN )
            {
            pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
            	return sizeof (INT8U);
            }

            /* check for Static Router Adv enable state */
            if (!( 0x1 & pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable))
            {
                TCRIT(" Static Router Adv is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_Router1_MACAddr,
                    pSetLanReq->ConfigData.IPv6_Router1_MACAddr, IP6_ADDR_LEN );
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            break;

        case LAN_PARAMS_IPV6_STATIC_ROUTER1_PREFIXLEN:
            if ( 0x1 != ReqLen )
            {
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }

            pendStatus = GetPendStatus(PEND_OP_SET_RA_PREFIXLEN);
            if (pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /* check for Static Router Adv enable state */
            if (!( 0x1 & pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable))
            {
                TCRIT(" Static Router Adv is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            LOCK_BMC_SHARED_MEM(BMCInst);
            pBMCInfo->LANCfs[EthIndex].IPv6_Router1_PrefixLen = 
                                    pSetLanReq->ConfigData.IPv6_Router1_PrefixLen;
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            RAipV6Prefix.RAipV6_Cnt = 0x1;
            RAipV6Prefix.RAipV6_PrefixLen = pSetLanReq->ConfigData.IPv6_Router1_PrefixLen;

            SetPendStatus(PEND_OP_SET_RA_PREFIXLEN,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_RA_PREFIXLEN, (INT8U*)&RAipV6Prefix,
                    sizeof(RAipV6Prefix_T),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            break;

        case LAN_PARAMS_IPV6_STATIC_ROUTER1_PREFIXVAL:
            if (ReqLen != 2)
            {
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            

            /* check for Static Router Adv enable state */
            if (!( 0x1 & pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable))
            {
                TCRIT(" Static Router Adv is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            //TODO: verify with Static Router Address

            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_Router1_PrefixValue,
                        pSetLanReq->ConfigData.IPv6_Router1_PrefixValue, IP6_PREFIX_MAXLEN);
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case LAN_PARAMS_IPV6_STATIC_ROUTER2_IPADDR:
            TDBG("Entered in LAN_PARAMS_IPV6_STATIC_ROUTER2_IPADDR \n");
            if ( ReqLen != IP6_ADDR_LEN )
            {
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }

            pendStatus = GetPendStatus(PEND_OP_SET_RA_IPADDR);
            if (pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /* check for Static Router Adv enable state */
            if (!( 0x1 & pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable))
            {
                TCRIT(" Static Router Adv is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /*Validate the IPv6 address*/
            if(0 != IsValidGlobalIPv6Addr((struct in6_addr*)&pSetLanReq->ConfigData.IPv6_Router1_IPAddr))
            {
                TCRIT("Invalid Global IPv6 Address\n");
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_Router2_IPAddr,
                        pSetLanReq->ConfigData.IPv6_Router2_IPAddr, IP6_ADDR_LEN );

            RAipV6Addr.RAipV6_Cnt = 0x2;
            _fmemcpy(RAipV6Addr.RAipV6_IPAddr, pSetLanReq->ConfigData.IPv6_Router2_IPAddr, IP6_ADDR_LEN );

            SetPendStatus(PEND_OP_SET_RA_IPADDR,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_RA_IPADDR, (INT8U*)&RAipV6Addr,
                        sizeof(RACFG_T),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            break;

        case LAN_PARAMS_IPV6_STATIC_ROUTER2_MACADDR:
            if ( ReqLen != MAC_ADDR_LEN )
            {
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            /* check for Static Router Adv enable state */
            if (!( 0x1 & pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable))
            {
                TCRIT(" Static Router Adv is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_Router2_MACAddr,
                        pSetLanReq->ConfigData.IPv6_Router2_MACAddr, IP6_ADDR_LEN );
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case LAN_PARAMS_IPV6_STATIC_ROUTER2_PREFIXLEN:
            if ( 0x1 != ReqLen )
            {
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }

            pendStatus = GetPendStatus(PEND_OP_SET_RA_PREFIXLEN);
            if ( pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /* check for Static Router Adv enable state */
            if (!( 0x1 & pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable))
            {
                TCRIT(" Static Router Adv is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            LOCK_BMC_SHARED_MEM(BMCInst);
            pBMCInfo->LANCfs[EthIndex].IPv6_Router2_PrefixLen= 
                        pSetLanReq->ConfigData.IPv6_Router2_PrefixLen;
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            RAipV6Prefix.RAipV6_Cnt = 0x2;
            RAipV6Prefix.RAipV6_PrefixLen = 
                        pSetLanReq->ConfigData.IPv6_Router2_PrefixLen;

            SetPendStatus(PEND_OP_SET_RA_PREFIXLEN,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_RA_PREFIXLEN, (INT8U*)&RAipV6Prefix,
                        sizeof(RAipV6Prefix_T),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            break;

        case LAN_PARAMS_IPV6_STATIC_ROUTER2_PREFIXVAL:
            if (ReqLen != 2)
            {
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            /* check for Static Router Adv enable state */
            if (!( 0x1 & pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable))
            {
                TCRIT(" Static Router Adv is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            //TODO: verify with Static Router Address

            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_Router2_PrefixValue,
                        pSetLanReq->ConfigData.IPv6_Router2_PrefixValue, IP6_PREFIX_MAXLEN );
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case LAN_PARAMS_AMI_OEM_SNMPV6_DEST_ADDR:

            if (pSetLanReq->ConfigData.DestType.SetSelect > pBMCInfo->LANCfs[EthIndex].NumDest )
            {
                TCRIT("Invalid data for SetSelect");
                *pRes = CC_INV_DATA_FIELD ;
                return sizeof (*pRes);
            }

            /*Validate the IPv6 address*/
            if(0 != IsValidGlobalIPv6Addr((struct in6_addr*)&pSetLanReq->ConfigData.Destv6Addr.IPAddr))
            {
                TCRIT("Invalid Global IPv6 Address\n");
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            if (0 == pSetLanReq->ConfigData.Destv6Addr.SetSelect)
            {
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (&pSharedMem->VolLANv6Dest,
                  &pSetLanReq->ConfigData.Destv6Addr, sizeof(LANDestv6Addr_T));

                memset(pSharedMem->VolLANDest,0,sizeof(LANDestAddr_T));

                UNLOCK_BMC_SHARED_MEM(BMCInst);
            }
            else
            {
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (&pBMCInfo->LANCfs[EthIndex].Destv6Addr[pSetLanReq->ConfigData.Destv6Addr.SetSelect - 1],
                  &pSetLanReq->ConfigData.Destv6Addr, sizeof(LANDestv6Addr_T));

                memset(&pBMCInfo->LANCfs[EthIndex].DestAddr[pSetLanReq->ConfigData.DestAddr.SetSelect - 1],0,sizeof(LANDestAddr_T));

                UNLOCK_BMC_SHARED_MEM(BMCInst);
            }

            TDBG("\n SetLanconfig: Setting SNMPv6 configuration done..\n");

            break;

        case LAN_PARAMS_AMI_OEM_ENABLE_SET_MAC:
            NIC_Count = g_coremacros.global_nic_count;
            if (ReqLen != 1)
                pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
            else if (pSetLanReq->ConfigData.EthIndex > ( NIC_Count- 1))
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
            else
            {
                enableSetMACAddr = TRUE;
                macEthIndex = pSetLanReq->ConfigData.EthIndex;
                pSetLanRes->CompletionCode = CC_NORMAL;
            }

            return sizeof(*pSetLanRes);
            break;


        case LAN_PARAMS_AMI_OEM_IPV6_ENABLE:
            pendStatus = GetPendStatus(PEND_OP_SET_IPV6_ENABLE);
            if (pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            
            TDBG("\n Entered in LAN_PARAMS_AMI_OEM_IPV6_ENABLE \n");         
            if ( pBMCInfo->LANCfs[EthIndex].IPv6_Enable == pSetLanReq->ConfigData.IPv6_Enable)
            {
                TCRIT("LAN or VLAN - the current state is the same \n");
                break;
            }
            pBMCInfo->LANCfs[EthIndex].IPv6_Enable= pSetLanReq->ConfigData.IPv6_Enable;
            
            NWConfig6.enable= pSetLanReq->ConfigData.IPv6_Enable;           
            SetPendStatus(PEND_OP_SET_IPV6_ENABLE,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_IPV6_ENABLE,(INT8U*) &NWConfig6,sizeof(NWConfig6),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            break;


        case LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR_SOURCE:
            pendStatus = GetPendStatus(PEND_OP_SET_IPV6_IP_ADDR_SOURCE);
            if (pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            
            TDBG("\n Entered in LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR_SOURCE \n");
             /* check for IPv6 enable state */
            if ( pBMCInfo->LANCfs[EthIndex].IPv6_Enable != 1)
            {
                TCRIT("IPv6 is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            
            if ((pSetLanReq->ConfigData.IPv6_IPAddrSrc > BMC_OTHER_SOURCE)
                ||(pSetLanReq->ConfigData.IPv6_IPAddrSrc == UNSPECIFIED_IP_SOURCE))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
           else if((pSetLanReq->ConfigData.IPv6_IPAddrSrc == BIOS_IP_SOURCE)
                       || (pSetLanReq->ConfigData.IPv6_IPAddrSrc == BMC_OTHER_SOURCE))
           {
                /* we only support DHCP and Static IP source */
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
           }
           
            if (pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc == pSetLanReq->ConfigData.IPv6_IPAddrSrc )
            {
                TCRIT("LAN or VLAN if current SRC is DHCP/Static and incoming SRC is DHCP/Static, do nothing\n");
                break;
            }
            
            pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc = pSetLanReq->ConfigData.IPv6_IPAddrSrc ;
            if ( (pSetLanReq->ConfigData.IPv6_IPAddrSrc == STATIC_IP_SOURCE ) || (pSetLanReq->ConfigData.IPv6_IPAddrSrc == DHCP_IP_SOURCE ) )
            {
                NWConfig6.CfgMethod = pSetLanReq->ConfigData.IPv6_IPAddrSrc;
                SetPendStatus(PEND_OP_SET_IPV6_IP_ADDR_SOURCE,PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_IPV6_IP_ADDR_SOURCE,(INT8U*) &NWConfig6,sizeof(NWConfig6),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            }
          break;            


        case LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR:
            pendStatus = GetPendStatus(PEND_OP_SET_IPV6_IP_ADDR);
            if (pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            
            TDBG("\n Entered in LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR \n");
            /* check for IPv6 enable state */
            if ( pBMCInfo->LANCfs[EthIndex].IPv6_Enable != 1)
            {
                TCRIT("IPv6 is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /* Do IP address source check */
            if( pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc == DHCP_IP_SOURCE)
            {
                TCRIT("IPv6 Address source is currently in DHCP \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            /* Do Bond Mode Check*/
            if(g_corefeatures.bond_support == ENABLED)
            {
            	if((pBMCInfo->BondConfig.Enable) && ( pBMCInfo->BondConfig.BondMode != BOND_ACTIVE_BACKUP) )
				{
					TCRIT("Bonding Interface is not in Active Back_Up Mode...\n");
					*pRes = CC_BOND_IFC_NOT_IN_ACTIVE_BACK_UP;
					return sizeof (INT8U);
				}
            }
            /*Validate the Index value*/
            if(pSetLanReq->ConfigData.IPv6Addr.IPv6_Cntr & 0xF0)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            /*Validate the IPv6 address*/
            if(0 != IsValidGlobalIPv6Addr((struct in6_addr*)&pSetLanReq->ConfigData.IPv6Addr.IPv6_IPAddr))
            {
                TCRIT("Invalid Global IPv6 Address\n");
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            /*Validate the duplicate IPv6 Address*/
            nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
            for(i=0;i<MAX_IPV6ADDRS;i++)
            {
                /*Use can set the same IPv6 address for same index. To satisfy the WebServer*/
                if((memcmp(NWConfig6.GlobalIPAddr[i],pSetLanReq->ConfigData.IPv6Addr.IPv6_IPAddr,IP6_ADDR_LEN) == 0) && pSetLanReq->ConfigData.IPv6Addr.IPv6_Cntr != i)
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }
            }

            _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_IPAddr[pSetLanReq->ConfigData.IPv6Addr.IPv6_Cntr], pSetLanReq->ConfigData.IPv6Addr.IPv6_IPAddr, IP6_ADDR_LEN );
            
            SetPendStatus(PEND_OP_SET_IPV6_IP_ADDR,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_IPV6_IP_ADDR, (INT8U*) &pSetLanReq->ConfigData.IPv6Addr,\
            sizeof (IPv6Addr_T),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            break;

        case LAN_PARAMS_AMI_OEM_IPV6_LINK_ADDR:
            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof(INT8U);

        case LAN_PARAMS_AMI_OEM_IPV6_PREFIX_LENGTH:
            pendStatus = GetPendStatus(PEND_OP_SET_IPV6_PREFIX_LENGTH);
            if (pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            
            TDBG("\n Entered in LAN_PARAMS_AMI_OEM_IPV6_PREFIX_LENGTH \n");
            /* check for IPv6 enable state */
            if ( pBMCInfo->LANCfs[EthIndex].IPv6_Enable != 1)
            {
                TCRIT("IPv6 is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /* Do IP address source check */
            if( pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc == DHCP_IP_SOURCE)
            {
                TCRIT("IPv6 Address source is currently in DHCP \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /*Validate the Index value*/
            if(pSetLanReq->ConfigData.IPv6Prefix.IPv6_Prepos & 0xF0)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            pBMCInfo->LANCfs[EthIndex].IPv6_PrefixLen[pSetLanReq->ConfigData.IPv6Prefix.IPv6_Prepos] = pSetLanReq->ConfigData.IPv6Prefix.IPv6_PrefixLen;
            
            SetPendStatus(PEND_OP_SET_IPV6_PREFIX_LENGTH,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_IPV6_PREFIX_LENGTH,(INT8U*) &pSetLanReq->ConfigData.IPv6Prefix, sizeof(IPv6Prefix_T),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            break;

        case LAN_PARAMS_AMI_OEM_IPV6_GATEWAY_IP:
            pendStatus = GetPendStatus(PEND_OP_SET_IPV6_GATEWAY);
            if (pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            
            TDBG("\n Entered in LAN_PARAMS_AMI_OEM_IPV6_GATEWAY_IP \n");
            /* check for IPv6 enable state */
            if ( pBMCInfo->LANCfs[EthIndex].IPv6_Enable != 1)
            {
                TCRIT("IPv6 is not enabled yet... \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

            /* Do IP address source check */
            if( pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc == DHCP_IP_SOURCE)
            {
                TCRIT("IPv6 Address source is currently in DHCP \n");
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }

			/*Validate the IPv6 address*/
            if(0 != IsValidGlobalIPv6Addr((struct in6_addr*)&pSetLanReq->ConfigData.IPv6_GatewayIPAddr))
            {
                TCRIT("Invalid Global IPv6 Address\n");
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }
            
            _fmemcpy( pBMCInfo->LANCfs[EthIndex].IPv6_GatewayIPAddr, pSetLanReq->ConfigData.IPv6_GatewayIPAddr, IP6_ADDR_LEN );
            
            _fmemcpy( NWConfig6.Gateway, pSetLanReq->ConfigData.IPv6_GatewayIPAddr, IP6_ADDR_LEN );
            SetPendStatus(PEND_OP_SET_IPV6_GATEWAY,PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_IPV6_GATEWAY,(INT8U*) &NWConfig6,sizeof(NWConfig6),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            break;

        case LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR_EUI64:

        /* The first byte is the index and it should contain atleast one byte of IPv6 address and it cannot exceed 8 bytes incase of EUI-64 algo */
        if((ReqLen < 2) || (ReqLen > 9)) 
        {
            *pRes = CC_REQ_INV_LEN;
            return sizeof(INT8U);
        }

        pendStatus = GetPendStatus(PEND_OP_SET_IPV6_IP_ADDR);
        if (pendStatus == PEND_STATUS_PENDING)
        {
            *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
            return sizeof (INT8U);
        }

        TDBG("\n Entered in LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR_EUI64 \n");

        /* Check for IPv6 enabled state */
        if(pBMCInfo->LANCfs[EthIndex].IPv6_Enable != 1)
        {
            TCRIT("IPv6 is not enabled yet... \n");
            *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
            return sizeof(INT8U);
        }

        /* Do IP address source check */
        if(pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc == DHCP_IP_SOURCE)
        {
            TCRIT("IPv6 Address source is currently in DHCP \n");
            *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
            return sizeof (INT8U);
        }

        /*Validate the Index value*/
        if(pSetLanReq->ConfigData.IPv6Addr.IPv6_Cntr & 0xF0)
        {
            TCRIT("Index of the given IPv6 Address is invalid. \n");
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(INT8U);
        }

        /* The request may be of variable length, so fill the network prefix by appending zeros. This is to avoid garbage values */
        for(i = ReqLen-1; i<8; i++)
        {
            pSetLanReq->ConfigData.IPv6Addr.IPv6_IPAddr[i] = 0;
        }

        /* Generate the IPv6 address using EUI-64 algo */
        if(nwFormIPv6Addr_EUI64((INT8U*) &pSetLanReq->ConfigData.IPv6Addr.IPv6_IPAddr, netindex, g_corefeatures.global_ipv6) != 0)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(INT8U);
        }

        /* Get the current network configuration settings */
        nwReadNWCfg_v4_v6(&NWConfig, &NWConfig6, netindex, g_corefeatures.global_ipv6);

        /*Validate the duplicate IPv6 Address*/
        for(i=0; i<MAX_IPV6ADDRS; i++)
        {
            /* User can set the same IPv6 address for same index to satisfy the WebServer */
            if((memcmp(NWConfig6.GlobalIPAddr[i], pSetLanReq->ConfigData.IPv6Addr.IPv6_IPAddr, IP6_ADDR_LEN) == 0) && pSetLanReq->ConfigData.IPv6Addr.IPv6_Cntr != i)
            {
                TCRIT("\n Error in setting the IPv6 address - Duplicate :( \n");
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }
        }

        _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_IPAddr[pSetLanReq->ConfigData.IPv6Addr.IPv6_Cntr], pSetLanReq->ConfigData.IPv6Addr.IPv6_IPAddr, IP6_ADDR_LEN);

        SetPendStatus(PEND_OP_SET_IPV6_IP_ADDR,PEND_STATUS_PENDING);
        PostPendTask(PEND_OP_SET_IPV6_IP_ADDR, (INT8U*) &pSetLanReq->ConfigData.IPv6Addr, sizeof(IPv6Addr_T), (pSetLanReq->ChannelNum & 0x0F), BMCInst);
        break;

        case LAN_PARAMS_AMI_OEM_PHY_SETTINGS:

        if(ReqLen != 4)
        {
            pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
            return sizeof(*pSetLanRes);
        }

        memset(IfcName, 0, sizeof(IfcName));

        if(GetIfcNameByIndex(EthIndex, IfcName) != 0)
        {
            TCRIT("Error in Getting Interface Name for the lan Index:%d\n",EthIndex);
            pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
            return sizeof(*pSetLanRes);
        }
        
        if((pSetLanReq->ConfigData.PHYConfig.AutoNegotiationEnable==DISABLE)&&(pSetLanReq->ConfigData.PHYConfig.Speed==1000))
		{
		   pSetLanRes->CompletionCode = CC_1GB_INVALID_AUTO_NEG;
		   return sizeof(*pSetLanRes);
		}

        if(nwGetEthInformation(&PHYCfg, IfcName) == 0)
        {
            if(((PHYCfg.autoneg == pSetLanReq->ConfigData.PHYConfig.AutoNegotiationEnable)  && (PHYCfg.speed == pSetLanReq->ConfigData.PHYConfig.Speed) 
                && (PHYCfg.duplex == pSetLanReq->ConfigData.PHYConfig.Duplex)))
            {
                TDBG("There is no change in the link mode, ignoring the request for setting the speed/duplex.\n");
                break;
            }
        }
        else
        {
            pSetLanRes->CompletionCode = CC_UNSPECIFIED_ERR;
            return sizeof(*pSetLanRes);
        }

        if(pSetLanReq->ConfigData.PHYConfig.Speed != 0xFFFF && pSetLanReq->ConfigData.PHYConfig.Duplex != 0xFF)
        {
            if(IsLinkModeSupported(&PHYCfg,pSetLanReq->ConfigData.PHYConfig.Speed,pSetLanReq->ConfigData.PHYConfig.Duplex) <= 0)
            {
                TCRIT("Unsupported link mode \n");
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof(*pSetLanRes);
            }
        }
        SetPendStatus(PEND_OP_SET_SPEED, PEND_STATUS_PENDING);
        PostPendTask(PEND_OP_SET_SPEED, (INT8U*) &(pSetLanReq->ConfigData.PHYConfig), sizeof(PHYConfig_T),(pSetLanReq->ChannelNum & 0x0F), BMCInst );

        break;
	
case LAN_PARAMS_AMI_OEM_MTU_SETTINGS:
            if(ReqLen != 2)
            {
                pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                return sizeof(*pSetLanRes);
            }

            /* Setting MTU values less than 1280 disables IPv6,
               RFC 1280 recommends MTU value should be greater than 1200 for IPV6.
               so restricting MTU value b/w 1280 to 1500.*/
        			
            if( (pSetLanReq->ConfigData.MTU_size < 1280) || (pSetLanReq->ConfigData.MTU_size > 1500) )
            {
                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                return sizeof(*pSetLanRes);
            }

            SetPendStatus(PEND_OP_SET_MTU_SIZE, PEND_STATUS_PENDING);
            PostPendTask(PEND_OP_SET_MTU_SIZE, (INT8U*) &(pSetLanReq->ConfigData.MTU_size), sizeof(INT16U),(pSetLanReq->ChannelNum & 0x0F), BMCInst );
            
            break;

    case LAN_PARAMS_SSI_OEM_2ND_PRI_ETH_MAC_ADDR:
        if(g_corefeatures.ssi_support == ENABLED)
        {
            pendStatus = GetPendStatus(PEND_OP_SET_MAC_ADDRESS);
            if(pendStatus == PEND_STATUS_PENDING)
            {
                *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                return sizeof (INT8U);
            }
            if (!enableSetMACAddr)
            {
                pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
                return sizeof(INT8U);
            }
            else
            {
                EnableSetMACAddress_T macAddrEnabled;
                memset(&macAddrEnabled, 0, sizeof(EnableSetMACAddress_T));
                macAddrEnabled.EthIndex = 0x1; /* Specify the 2nd interface */
                memcpy(&macAddrEnabled.MACAddress, pSetLanReq->ConfigData.SSI2ndPriEthMACAddr, MAC_ADDR_LEN);
                SetPendStatus(PEND_OP_SET_MAC_ADDRESS, PEND_STATUS_PENDING);
                PostPendTask(PEND_OP_SET_MAC_ADDRESS, (INT8U*)&macAddrEnabled, sizeof(EnableSetMACAddress_T), 0x1, BMCInst);
            }
            break;
          }
          else
          {
            pSetLanRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
          }

    case LAN_PARAMS_SSI_OEM_LINK_CTRL:
        if(g_corefeatures.ssi_support == ENABLED)
        {
            pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM; /* Read Only */
        }
        else
        {
            pSetLanRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
        }
            return sizeof(INT8U);

    case LAN_PARAMS_SSI_OEM_CMM_IP_ADDR:
        if(g_corefeatures.ssi_support == ENABLED)
        {
            _fmemcpy(pBMCInfo->SSIConfig.CMMIPAddr, pSetLanReq->ConfigData.CMMIPAddr, IP_ADDR_LEN);
            FlushIPMI((INT8U*)&pBMCInfo->SSIConfig, (INT8U*)&pBMCInfo->SSIConfig, pBMCInfo->IPMIConfLoc.SSIConfigAddr,
                      sizeof(SSIConfig_T), BMCInst);
            break;
        }
        else
        {
            pSetLanRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
        }

    default:
        if(g_corefeatures.ncsi_cmd_support == ENABLED)
        {
            switch (pSetLanReq->ParameterSelect)
            {
                case LAN_PARAMS_AMI_OEM_NCSI_CONFIG_NUM:
                    pSetLanRes->CompletionCode = CC_ATTEMPT_TO_SET_RO_PARAM;
                    return sizeof(*pSetLanRes);

                case LAN_PARAMS_AMI_OEM_NCSI_SETTINGS:
                    NIC_Count=g_coremacros.global_nic_count;
                    if (ReqLen != 3)
                        pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                    else if (pSetLanReq->ConfigData.NCSIPortConfig.Interface >= NIC_Count)
                        pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                    else
                    {
                        pendStatus = GetPendStatus(PEND_OP_SET_NCSI_CHANNEL_ID);
                        if(pendStatus == PEND_STATUS_PENDING)
                        {
                            *pRes = CC_NODE_BUSY;
                            return sizeof (INT8U);
                        }
                        
                        NCSIConfig_T configData;
                        char interfaceName[8];
                        memset(&configData, 0, sizeof(NCSIConfig_T));
                        memset(interfaceName, 0, sizeof(interfaceName));

                        snprintf(interfaceName, sizeof(interfaceName), "%s%d", "eth", pSetLanReq->ConfigData.NCSIPortConfig.Interface);

                        if (NCSIGetPortConfigByName(interfaceName, &configData) != 0)
                            pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                        else
                        {
                            configData.PackageId = pSetLanReq->ConfigData.NCSIPortConfig.PackageId;
                            configData.ChannelId = pSetLanReq->ConfigData.NCSIPortConfig.ChannelId;

                            if (NCSISetPortConfigByName(interfaceName, configData) != 0)
                                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                            else
                            {
                                SetPendStatus(PEND_OP_SET_NCSI_CHANNEL_ID, PEND_STATUS_PENDING);
                                PostPendTask(PEND_OP_SET_NCSI_CHANNEL_ID, NULL, 0, 0, BMCInst);
                                pSetLanRes->CompletionCode = CC_NORMAL;
                            }
                        }
                    }
                    return sizeof(*pSetLanRes);
                case LAN_PARAMS_AMI_OEM_NCSI_SETTINGS_BY_PORT_NUM:
                	NCSIGetTotalPorts(&Num_of_Ports);
                    int PortNum = pSetLanReq->ConfigData.NCSICfg.PortNum;
                    if (ReqLen != 4)
                        pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                    else if ((PortNum > Num_of_Ports) || (PortNum == 0))
                        pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                    else if (pSetLanReq->ConfigData.NCSICfg.InterfaceIndex != 0xFF)
                    	pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                    else
                    {
                        pendStatus = GetPendStatus(PEND_OP_SET_NCSI_CHANNEL_ID);
                        if(pendStatus == PEND_STATUS_PENDING)
                        {
                            *pRes = CC_NODE_BUSY;
                            return sizeof (INT8U);
                        }

                        NCSIConfig_T configData;
                        memset(&configData, 0, sizeof(NCSIConfig_T));

                        if (NCSIGetPortConfig(PortNum, &configData) != 0)
                            pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                        else
                        {
                            configData.PackageId = pSetLanReq->ConfigData.NCSICfg.PackageId;
                            configData.ChannelId = pSetLanReq->ConfigData.NCSICfg.ChannelId;

                            if (NCSISetPortConfig(PortNum, configData) != 0)
                                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                            else
                            {
                                SetPendStatus(PEND_OP_SET_NCSI_CHANNEL_ID, PEND_STATUS_PENDING);
                                PostPendTask(PEND_OP_SET_NCSI_CHANNEL_ID, NULL, 0, 0, BMCInst);
                                pSetLanRes->CompletionCode = CC_NORMAL;
                            }
                        }
                    }
                    return sizeof(*pSetLanRes);
                case LAN_PARAMS_AMI_OEM_NCSI_MODE_CHANGE:
                    NIC_Count=g_coremacros.global_nic_count;
                    if (ReqLen != 2)
                        pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                    else if (pSetLanReq->ConfigData.NCSIModeConfig.Interface >= NIC_Count)
                        pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                    else 
                    {
                        pendStatus = GetPendStatus(PEND_OP_SET_NCSI_MODE_CHANGE);
                        if(pendStatus == PEND_STATUS_PENDING)
                        {
                            *pRes = CC_NODE_BUSY;
                            return sizeof (INT8U);
                        }
                    
                        NCSIConfig_T configData;
                        char interfaceName[8];
                        memset(&configData, 0, sizeof(NCSIConfig_T));
                        memset(interfaceName, 0, sizeof(interfaceName));

                        snprintf(interfaceName, sizeof(interfaceName), "%s%d", "eth", pSetLanReq->ConfigData.NCSIModeConfig.Interface);
						
                        if (NCSIGetPortConfigByName(interfaceName, &configData) != 0)
                            pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                        else
                        {
                            configData.AutoSelect = pSetLanReq->ConfigData.NCSIModeConfig.NCSIMode;
                            if (NCSISetPortConfigByName(interfaceName, configData) != 0)
                                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                            else
                            {
                                SetPendStatus(PEND_OP_SET_NCSI_MODE_CHANGE, PEND_STATUS_PENDING);
                                PostPendTask(PEND_OP_SET_NCSI_MODE_CHANGE, NULL, 0, 0, BMCInst);
                                pSetLanRes->CompletionCode = CC_NORMAL;                    	
                            }
                        }
                    }
                    return sizeof(*pSetLanRes);
                case LAN_PARAMS_AMI_OEM_NCSI_EXTENSION:
                    if ((0x20 == pSetLanReq->ConfigData.NCSIPHYConfigSet.Command) && 
                        (g_corefeatures.ncsi_keep_phy_linkup_support == ENABLED))
                    {
                        NIC_Count=g_coremacros.global_nic_count;
                        if (ReqLen != 3)
                            pSetLanRes->CompletionCode = CC_REQ_INV_LEN;
                        else if (pSetLanReq->ConfigData.NCSIPHYConfigSet.Interface >= NIC_Count)
                            pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                        else 
                        {
                            pendStatus = GetPendStatus(PEND_OP_SET_NCSI_VETOBIT);
                            if(pendStatus == PEND_STATUS_PENDING)
                            {
                                *pRes = CC_NODE_BUSY;
                                return sizeof (INT8U);
                            }

                            NCSIConfig_T configData;
                            char interfaceName[8];
                            memset(&configData, 0, sizeof(NCSIConfig_T));
                            memset(interfaceName, 0, sizeof(interfaceName));

                            snprintf(interfaceName, sizeof(interfaceName), "%s%d", "eth", pSetLanReq->ConfigData.NCSIPHYConfigSet.Interface);

                            if (NCSIGetPortConfigByName(interfaceName, &configData) != 0)
                                pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                            else
                            {
                                configData.VetoBit= pSetLanReq->ConfigData.NCSIPHYConfigSet.VetoBit;
                                if (NCSISetPortConfigByName(interfaceName, configData) != 0)
                                    pSetLanRes->CompletionCode = CC_INV_DATA_FIELD;
                                else
                                {
                                    SetPendStatus(PEND_OP_SET_NCSI_VETOBIT, PEND_STATUS_PENDING);
                                    PostPendTask(PEND_OP_SET_NCSI_VETOBIT, NULL, 0, (pSetLanReq->ChannelNum & 0x0F), BMCInst);
                                    pSetLanRes->CompletionCode = CC_NORMAL;
                                }
                            }
                        }
                    }
                    else
                    {
                         pSetLanRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
                    }

                    return sizeof(*pSetLanRes);

                default:
                    TDBG("In Valid Option\n");
            }
        }
        pSetLanRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
        return sizeof(INT8U);
    }
    
    pSetLanRes->CompletionCode = CC_NORMAL;
    
    if(g_PDKHandle[PDK_POSTSETLANPARAM] != NULL)
    {
        ((int(*)(INT8U*, INT8U, int))(g_PDKHandle[PDK_POSTSETLANPARAM]))(pReq, ReqLen, BMCInst);
    }
    
    FlushIPMI((INT8U*)&pBMCInfo->LANCfs[0],(INT8U*)&pBMCInfo->LANCfs[EthIndex],pBMCInfo->IPMIConfLoc.LANCfsAddr,
                      sizeof(LANConfig_T),BMCInst);
    return sizeof(*pSetLanRes);
}

/*
 *@fn IPAddrCheck function
 *@brief It will validate the IP Address and net Mask
 *@param Addr - IP Address or net Mask
 *@param params - parameter data to validate
  */
int IPAddrCheck(INT8U *Addr,int params)
{
    int i,maskcount=0,bitmask =0,j,bitcheck=0;

    for(i=0;i< IP_ADDR_LEN;i++)
    {
        if(Addr[i] == BRD_CAST_BIT_MASK)
        {
            maskcount++;
        }
    }

    if(maskcount == IP_ADDR_LEN)
    {
        return 1;
    }

    if( params == LAN_PARAM_SELECT_DEST_ADDR )
    {
        if( Addr[0] == LAN_RESERVED  )
        {
            return 1;
        }
    }

    if(params == LAN_PARAM_SUBNET_MASK)
    {
        if(Addr[0] == BRD_CAST_BIT_MASK)
        {
            for(i=1;i< IP_ADDR_LEN;i++)
            {
                if(Addr[i-1] == 0)
                {
                    bitmask = 1;
                }
                if(Addr[i] != 0)
                {
                    for(j=0;j<8;j++)
                    {
                        if((Addr[i]<<j) & SUBNET_MASK_BIT_CHECK)
                        {
                            if(bitcheck == 1)
                            {
                                return 1;
                            }
                            continue;
                        }
                        bitcheck=1;
                    }
                    if((bitcheck == 1 && Addr[i-1] != BRD_CAST_BIT_MASK) || (Addr[i] > 0 && bitmask == 1))
                    {
                        return 1;
                    }
                }                       
            }
            return 0;
        }
        return 1;
    }

    if(Addr[0] == LOOP_BACK_BIT_MASK || Addr[0] == BRD_CAST_BIT_MASK)
    {
        return 1;
    }

    return 0;
}

/*----------------------------------------------
 * GratuitousARPTask
 *----------------------------------------------*/
void* GratuitousARPTask (INT8U *Addr)
{
    INT8U               IntervalInSec;
    INT8U               Status;
    int                 nRet;
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT       NWConfig6;
    INT8U               EthIndex,ChannelNum,netindex= 0xFF;
    int BMCInst;
    char IfcName[16];
    BMCArg *GratArpArgs = (BMCArg *)Addr;
    BMCInst = GratArpArgs->BMCInst;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

    memcpy(&ChannelNum,GratArpArgs->Argument,GratArpArgs->Len);

    memset(IfcName,0,sizeof(IfcName));

    if(GetChEthInfo(ChannelNum, &EthIndex, IfcName, &netindex,BMCInst) < 0)
    {
       TCRIT ("Error in Getting ethindex information\n");
       return 0;
    }

    if(Addr != NULL)
    {
     free(GratArpArgs->Argument);
     free(Addr);
    }

    TDBG ("Gratuitous ARP thread starts for ethindex : %x chnum : %x\n",EthIndex,ChannelNum);
    while (1)
    {
        if(EthIndex != GetEthIndex(ChannelNum & 0x0F, BMCInst))
        {
            memset(IfcName,0,sizeof(IfcName));

            if(GetChEthInfo(ChannelNum, &EthIndex, IfcName, &netindex,BMCInst) < 0)
            {
               TCRIT ("Error in Getting ethindex information\n");
               return 0;
            }
            TDBG ("Gratuitous ARP thread starts for ethindex : %x chnum : %x\n",EthIndex,ChannelNum);
        }
        /*Is Gratiutous Arp Enabled */
        if (0 == (pBMCInfo->LANCfs[EthIndex].BMCGeneratedARPControl & GRATIUTOUS_ENABLE_MASK))
        {
            TDBG("Gratuitous ARP thread exits : Disable BMC-generated Gratuitous ARPs invoked\n");
            break;
        }

        /*Is Gratiutous Arp Suspended */
        Status = BMC_GET_SHARED_MEM(BMCInst)->ArpSuspendStatus[EthIndex];

		if ((0 != (Status & GRATIUTOUS_ENABLE_MASK)) && 
			(BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning == TRUE))
        {
            // Gratuitous ARP Suspended.
            // sleep requested to access Shared memory for two different threads.
            usleep (20);
            continue;
        }

        nwReadNWCfg_v4_v6( &NWConfig,&NWConfig6, netindex,g_corefeatures.global_ipv6);
        if(GetIfcName(EthIndex, IfcName,BMCInst) != 0)
        {
            TCRIT("Error in getting Interface Name for the Lan Index :%d\n",EthIndex);
            
        }
        if (NWConfig.IFName[0] == 0)
            sprintf((char *)&NWConfig.IFName, "%s",IfcName);

        TDBG ( "MAC is %2x:%2x:%2x:%2x:%2x:%2x \n", NWConfig.MAC [0], NWConfig.MAC [1],
                NWConfig.MAC [2], NWConfig.MAC [3], NWConfig.MAC [4], NWConfig.MAC [5] );
        TDBG ( "IP is %d.%d.%d.%d\n", NWConfig.IPAddr[0], NWConfig.IPAddr[1],
                NWConfig.IPAddr[2], NWConfig.IPAddr[3]);
        TDBG ( "Device Name : %s\n", (char *)&NWConfig.IFName);

        nRet = SendGratuitousARPPacket((char *)&NWConfig.IFName, NWConfig.IPAddr, NWConfig.MAC);
        if (0 != nRet)
        {
            TCRIT("Unable to Send Gratuitous ARP packet\n");
        }
        TDBG ("Send Gratuitous Packet\n");

        if (0 == pBMCInfo->LANCfs[EthIndex].GratitousARPInterval)
        {
            IntervalInSec = 2; //Default 2 secs
        } else {
            // Gratuitous ARP interval in 500 millisecond increments.
            IntervalInSec = (pBMCInfo->LANCfs[EthIndex].GratitousARPInterval * 500)/1000;
        }
        usleep( IntervalInSec * 1000 * 1000 );
    }
    return 0;
}


/*---------------------------------------------------
 * GetLanConfigParam
 *---------------------------------------------------*/
int
GetLanConfigParam (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetLanConfigReq_T*  pGetLanReq = (GetLanConfigReq_T*) pReq;
    GetLanConfigRes_T*  pGetLanRes = (GetLanConfigRes_T*) pRes;
      BMCSharedMem_T*     pSharedMem = BMC_GET_SHARED_MEM (BMCInst);
    INT8U IsOemDefined = FALSE;
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT        NWConfig6;
//    V6DNS_CONFIG v6dnsconfig;
    INT8U EthIndex,netindex= 0xFF,i,j;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    int ncsiPortConfigNum = 0;
    RACFG_T raCfg = {0};
    ETHCFG_STRUCT PHYCfg;
    ChannelInfo_T* pChannelInfo = NULL;
       char tmpstr[16];		
	

    char IfcName[16];  /* Eth interface name */
    INT8U               ComStrLen=MAX_COMM_STRING_SIZE;
    int retValue = 0,NIC_Count = 0;
    INT8U CipherSuitePrivilegelevels [N0_OF_CIPHER_SUITE_SUPPORTED + 1] = {0};
    INT8U CipherSuiteID = -1, CipherSuitePrivilege = 0;

    INT8U InterfaceName[MAX_LAN_CHANNELS][MAX_IFC_NAME]={{0}};
    int noofinterface=0;

    pGetLanRes->CCParamRev.CompletionCode = CC_NORMAL;
    pGetLanRes->CCParamRev.ParamRevision  = PARAMETER_REVISION_FORMAT;

    if(pGetLanReq->ChannelNum & RESERVED_VALUE_70)
    {
        /* Alarm !!! Somebody is trying to set Reseved Bits */
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

       /* Validation check for IPMI IPv6 Commands, version 1.1 */
   if((pGetLanReq->ParameterSelect >= LAN_PARAM_IPV6_IPV4_SUPPORT && pGetLanReq->ParameterSelect <=LAN_PARAMS_IPV6_SLAAC_TIMING_CONFIGURATION )
          && !( g_corefeatures.ipv6_compliance_support))
   {
         pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_NOT_SUPPORTED;
        return sizeof(INT8U);
   }

   if(((pGetLanReq->ParameterSelect >= LAN_PARAMS_AMI_OEM_IPV6_ENABLE && pGetLanReq->ParameterSelect <=LAN_PARAMS_AMI_OEM_IPV6_GATEWAY_IP ))
          && ( g_corefeatures.ipv6_compliance_support))
   {
       if((pGetLanReq->ParameterSelect  != LAN_PARAMS_AMI_OEM_IPV6_LINK_ADDR ) ||(pGetLanReq->ParameterSelect != LAN_PARAMS_AMI_OEM_IPV6_LINK_ADDR_PREFIX))
       {
        pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_NOT_SUPPORTED;
        return sizeof(INT8U);
       }	
   }
	

    if((pGetLanReq->ParameterSelect >= MIN_LAN_OEM_CONF_PARAM) && 
            (pGetLanReq->ParameterSelect <= MAX_LAN_OEM_CONF_PARAM) )
    {
    	/* Converts OEM parameter value to equivalent AMI parameter value */
    	if (0 != GetLanAMIParamValue (&pGetLanReq->ParameterSelect, &IsOemDefined) )
    	{
            pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
    	}

    	/* Hook for OEM to handle this parameter */
        if ( (IsOemDefined)  && (g_PDKHandle[PDK_GETLANOEMPARAM] != NULL) )
        {
			return ((int(*)(INT8U *, INT8U, INT8U *,int))(g_PDKHandle[PDK_GETLANOEMPARAM]))(pReq, ReqLen, pRes, BMCInst);
    	}
   	
    }

    if (g_PDKHandle[PDK_BEFOREGETLANPARM] != NULL)
    {
        retValue = ((int(*)(INT8U *, INT8U, INT8U *,int))(g_PDKHandle[PDK_BEFOREGETLANPARM]))(pReq, ReqLen, pRes, BMCInst);
        if(retValue != 0)
        {
              return retValue;
        }
    }

    //! Validate the SetSelector value. 
    if (  (0x00 != pGetLanReq->SetSelect) &&  
          (pGetLanReq->ParameterSelect != LAN_PARAM_SELECT_DEST_TYPE) &&  
          (pGetLanReq->ParameterSelect != LAN_PARAM_SELECT_DEST_ADDR) &&  
          (pGetLanReq->ParameterSelect != LAN_PARAM_VLAN_TAGS) &&
          (pGetLanReq->ParameterSelect != LAN_PARAMS_AMI_OEM_SNMPV6_DEST_ADDR) && 
          (pGetLanReq->ParameterSelect != LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR) &&
          (pGetLanReq->ParameterSelect != LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR_EUI64) &&
          (pGetLanReq->ParameterSelect != LAN_PARAMS_AMI_OEM_IPV6_PREFIX_LENGTH) &&
          (pGetLanReq->ParameterSelect != LAN_PARAMS_IPV6_DHCPV6_TIMING_CONFIGURATION) &&
          (pGetLanReq->ParameterSelect != LAN_PARAMS_IPV6_SLAAC_TIMING_CONFIGURATION) &&
          (pGetLanReq->ParameterSelect !=LAN_PARAM_IPV6_STATIC_ADDRESS) &&
          (pGetLanReq->ParameterSelect !=LAN_PARAM_IPV6_DYNAMIC_ADDRESS) &&
          (pGetLanReq->ParameterSelect != LAN_PARAMS_IPV6_DYNAMIC_ROUTER_INFO_IPADDR) &&
          (pGetLanReq->ParameterSelect != LAN_PARAMS_IPV6_DYNAMIC_ROUTER_INFO_MACADDR) &&
          (pGetLanReq->ParameterSelect != LAN_PARAMS_IPV6_DYNAMIC_ROUTER_INFO_PREFIXLEN) &&
          (pGetLanReq->ParameterSelect != LAN_PARAMS_IPV6_DYNAMIC_ROUTER_INFO_PREFIXVAL))
    {
        if( g_corefeatures.ncsi_cmd_support == ENABLED )
        {
            if ((pGetLanReq->ParameterSelect != LAN_PARAMS_AMI_OEM_NCSI_SETTINGS) &&
				(pGetLanReq->ParameterSelect != LAN_PARAMS_AMI_OEM_NCSI_MODE_CHANGE) &&
				(pGetLanReq->ParameterSelect != LAN_PARAMS_AMI_OEM_NCSI_EXTENSION) &&
				(pGetLanReq->ParameterSelect != LAN_PARAMS_AMI_OEM_NCSI_SETTINGS_BY_PORT_NUM))
            {
                *pRes = CC_INV_DATA_FIELD;  
                return sizeof (*pRes);  
            }
        }
        else
        {
            *pRes = CC_INV_DATA_FIELD;  
            return sizeof (*pRes);  
        }
    }  

    //! Validate the BlockSelector value.  
    if( (0x00 != pGetLanReq->BlockSelect)  &&
        (pGetLanReq->ParameterSelect != LAN_PARAMS_IPV6_DHCPV6_TIMING_CONFIGURATION))
	{
        *pRes = CC_INV_DATA_FIELD;  
        return sizeof (*pRes);  
    }


    EthIndex= GetEthIndex(pGetLanReq->ChannelNum & 0x0F, BMCInst);
    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    /*Get the EthIndex*/
    if(GetIfcName(EthIndex,IfcName, BMCInst) != 0)
    {
        TCRIT("Error in Getting IfcName\n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
    {
        if(strcmp(Ifcnametable[i].Ifcname,IfcName) == 0)
        {
            netindex= Ifcnametable[i].Index;
            break;
        }
    }

    if(netindex == 0xFF)
    {
        TCRIT("Error in Getting netindex %d %s\n",netindex,IfcName);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }
    
    if ((pGetLanReq->ChannelNum & GET_PARAMETER_REVISION_MASK) != 0)
    {
        if((MAX_LAN_CONF_PARAM >= pGetLanReq->ParameterSelect) ||
               ((MIN_LAN_OEM_CONF_PARAM <= pGetLanReq->ParameterSelect) && (MAX_LAN_OEM_CONF_PARAM >= pGetLanReq->ParameterSelect)) ) 
        {
            return sizeof(GetLanCCRev_T);
        }
        else
        {
             *pRes = CC_PARAM_NOT_SUPPORTED;
             return sizeof (*pRes);  
        }

    }
    else
    {
            switch(pGetLanReq->ParameterSelect)
            {
            case LAN_PARAM_SET_IN_PROGRESS:

                LOCK_BMC_SHARED_MEM(BMCInst);
                pGetLanRes->ConfigData.SetInProgress = BMC_GET_SHARED_MEM(BMCInst)->m_Lan_SetInProgress;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                break;

            case LAN_PARAM_AUTH_TYPE_SUPPORT:

                pGetLanRes->ConfigData.AuthTypeSupport = pBMCInfo->LANCfs[EthIndex].AuthTypeSupport;
                break;

            case LAN_PARAM_AUTH_TYPE_ENABLES:

                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (&pGetLanRes->ConfigData.AuthTypeEnables,
                          &(pBMCInfo->LANCfs[EthIndex].AuthTypeEnables), sizeof(AuthTypeEnables_T));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                break;

            case LAN_PARAM_IP_ADDRESS:
                if((g_corefeatures.delayed_lan_restart_support) && (pBMCInfo->LANCfs[EthIndex].IPAddrSrc == STATIC_IP_SOURCE))
                {
                    if(pBMCInfo->LANCfs[EthIndex].IPv4_Enable)
                    {
                        _fmemcpy (NWConfig.IPAddr,pBMCInfo->LANCfs[EthIndex].IPAddr, IP_ADDR_LEN);
                    }
                    else
                    {
                        memset(&NWConfig, 0, sizeof(NWConfig));
                    }
                }
                else
                {
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                }
                _fmemcpy (pGetLanRes->ConfigData.IPAddr, NWConfig.IPAddr, IP_ADDR_LEN);


            break;

            case LAN_PARAM_IP_ADDRESS_SOURCE:
            if(g_corefeatures.delayed_lan_restart_support)
            {
                if(pBMCInfo->LANCfs[EthIndex].IPv4_Enable)
                {
                    NWConfig.CfgMethod = pBMCInfo->LANCfs[EthIndex].IPAddrSrc;
                }
                else
                {
                    memset(&NWConfig, 0, sizeof(NWConfig));
                }
            }
            else
            {
                nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
            }

            pGetLanRes->ConfigData.IPAddrSrc = NWConfig.CfgMethod;
              break;

            case LAN_PARAM_MAC_ADDRESS:
                nwGetNWInformations(&NWConfig,IfcName);
                if(IsBondingActive(BMCInst))
                {
                    /*If the bond is enabled and given channel number is 8, then read the MAC address from /proc/net/bonding/bond0*/
                    if(CheckBondSlave(EthIndex) == 1)
                    {
                        for(i=0; i<sizeof(pBMCInfo->LanIfcConfig)/sizeof(LANIFCConfig_T); i++)
                        {
                            if(pBMCInfo->LanIfcConfig[i].Ethindex == EthIndex)
                            {
                                nwGetMACAddrInBondConf(&NWConfig,pBMCInfo->LanIfcConfig[i].ifname);
                                break;
                            }
                        }
                    }
                }
                _fmemcpy (pGetLanRes->ConfigData.MACAddr, NWConfig.MAC, MAC_ADDR_LEN);

                break;

            case LAN_PARAM_SUBNET_MASK:
                if((g_corefeatures.delayed_lan_restart_support) && (pBMCInfo->LANCfs[EthIndex].IPAddrSrc == STATIC_IP_SOURCE))
                {
                    if(pBMCInfo->LANCfs[EthIndex].IPv4_Enable)
                    {
                        _fmemcpy (NWConfig.Mask,pBMCInfo->LANCfs[EthIndex].SubNetMask, IP_ADDR_LEN);
                    }
                    else
                    {
                        memset(&NWConfig, 0, sizeof(NWConfig));
                    }
                }
                else 
                {
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);	
                }
                _fmemcpy (pGetLanRes->ConfigData.SubNetMask, NWConfig.Mask, IP_ADDR_LEN);
                break;

            case LAN_PARAM_IPv4_HEADER:

                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (&pGetLanRes->ConfigData.Ipv4HdrParam,
                          &(pBMCInfo->LANCfs[EthIndex].Ipv4HdrParam), sizeof(IPv4HdrParams_T));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                break;

            case LAN_PARAM_PRI_RMCP_PORT:

                pGetLanRes->ConfigData.PrimaryRMCPPort = htoipmi_u16(pBMCInfo->LANCfs[EthIndex].PrimaryRMCPPort);
                break;

            case LAN_PARAM_SEC_RMCP_PORT:
                /* Returning Invalid error message */
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof (INT8U);
                /*pGetLanRes->ConfigData.SecondaryPort = htoipmi_u16(pPMConfig->LANConfig[EthIndex].SecondaryPort);*/
                break;

            case LAN_PARAM_BMC_GENERATED_ARP_CONTROL:

                pGetLanRes->ConfigData.BMCGeneratedARPControl = pBMCInfo->LANCfs[EthIndex].BMCGeneratedARPControl;
                break;
            case LAN_PARAM_GRATITIOUS_ARP_INTERVAL:
                pGetLanRes->ConfigData.GratitousARPInterval =
                        pBMCInfo->LANCfs[EthIndex].GratitousARPInterval;
                break;

            case LAN_PARAM_DEFAULT_GATEWAY_IP:
                if((g_corefeatures.delayed_lan_restart_support) && (pBMCInfo->LANCfs[EthIndex].IPAddrSrc == STATIC_IP_SOURCE))
                {
                    if(pBMCInfo->LANCfs[EthIndex].IPv4_Enable)
                    {
                        _fmemcpy (NWConfig.Gateway, pBMCInfo->LANCfs[EthIndex].DefaultGatewayIPAddr, IP_ADDR_LEN);
                    }
                    else
                    {
                        memset(&NWConfig, 0, sizeof(NWConfig));
                    }
                }
                else 
                {
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);	
                }
                _fmemcpy (pGetLanRes->ConfigData.DefaultGatewayIPAddr, NWConfig.Gateway, IP_ADDR_LEN);
                break;

            case LAN_PARAM_DEFAULT_GATEWAY_MAC:
                nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                nwGetSrcMacAddr(&NWConfig.Gateway[0],netindex, &pGetLanRes->ConfigData.DefaultGatewayMACAddr[0]);
                break;

            case LAN_PARAM_BACKUP_GATEWAY_IP:

                LOCK_BMC_SHARED_MEM(BMCInst);
                nwGetBkupGWyAddr(pGetLanRes->ConfigData.BackupGatewayIPAddr,netindex);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                break;

            case LAN_PARAM_BACKUP_GATEWAY_MAC:

                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (pGetLanRes->ConfigData.BackupGatewayMACAddr,
                          pBMCInfo->LANCfs[EthIndex].BackupGatewayMACAddr, MAC_ADDR_LEN);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                break;

            case LAN_PARAM_COMMUNITY_STRING:
                if (g_PDKHandle[PDK_GETSNMPCOMMUNITYNAME] != NULL )
                {
                    if(((int(*)(INT8U *, INT8U *,int))(g_PDKHandle[PDK_GETSNMPCOMMUNITYNAME]))(pGetLanRes->ConfigData.CommunityStr,&ComStrLen, BMCInst)==0)
                        break;
                }
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy (pGetLanRes->ConfigData.CommunityStr,
                          pBMCInfo->LANCfs[EthIndex].CommunityStr, MAX_COMM_STRING_SIZE);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                break;

            case LAN_PARAM_DEST_NUM:

                pGetLanRes->ConfigData.NumDest = pBMCInfo->LANCfs[EthIndex].NumDest;
                break;

            case LAN_PARAM_SELECT_DEST_TYPE:


                //if (pGetLanReq->SetSelect > NUM_LAN_DESTINATION)
                if ( pGetLanReq->SetSelect   > pBMCInfo->LANCfs[EthIndex].NumDest )
                {
                    *pRes = CC_PARAM_OUT_OF_RANGE ;
                    return sizeof (*pRes);
                }

                if (0 == pGetLanReq->SetSelect)
                {
                    LOCK_BMC_SHARED_MEM(BMCInst);
                    _fmemcpy (&pGetLanRes->ConfigData.DestType,
                              &pSharedMem->VolLANDestType[EthIndex], sizeof(LANDestType_T));
                    UNLOCK_BMC_SHARED_MEM(BMCInst);
                }
                else
                {
                    LOCK_BMC_SHARED_MEM(BMCInst);
                    _fmemcpy (&pGetLanRes->ConfigData.DestType,
                              &(pBMCInfo->LANCfs[EthIndex].DestType[pGetLanReq->SetSelect - 1]),
                              sizeof(LANDestType_T));
                    UNLOCK_BMC_SHARED_MEM(BMCInst);
                }
                pGetLanRes->ConfigData.DestAddr.SetSelect = pGetLanReq->SetSelect;
                break;

            case LAN_PARAM_SELECT_DEST_ADDR:

                //if (pGetLanReq->SetSelect > NUM_LAN_DESTINATION)
                if ( pGetLanReq->SetSelect   > pBMCInfo->LANCfs[EthIndex].NumDest )
                {
                    *pRes = CC_PARAM_OUT_OF_RANGE ;
                    return sizeof (*pRes);
                }

                if (0 == pGetLanReq->SetSelect)
                {
                    LOCK_BMC_SHARED_MEM(BMCInst);
                    _fmemcpy (&pGetLanRes->ConfigData.DestAddr,
                              &pSharedMem->VolLANDest[EthIndex], sizeof(LANDestAddr_T));
                    UNLOCK_BMC_SHARED_MEM(BMCInst);
                }
                else
                {
                    LOCK_BMC_SHARED_MEM(BMCInst);
                    _fmemcpy (&pGetLanRes->ConfigData.DestAddr,
                              &(pBMCInfo->LANCfs[EthIndex].DestAddr[pGetLanReq->SetSelect - 1]),
                              sizeof(LANDestAddr_T));
                    UNLOCK_BMC_SHARED_MEM(BMCInst);
                }
                pGetLanRes->ConfigData.DestAddr.SetSelect = pGetLanReq->SetSelect;
                break;

            case LAN_PARAM_VLAN_ID:

                if( pBMCInfo->IpmiConfig.VLANIfcSupport == 1)
                {
                    pGetLanRes->ConfigData.VLANID = pBMCInfo->LANCfs[EthIndex].VLANID;
                }
                break;

            case LAN_PARAM_VLAN_PRIORITY:

                pGetLanRes->ConfigData.VLANPriority = pBMCInfo->LANCfs[EthIndex].VLANPriority;
                break;

            case LAN_PARAM_CIPHER_SUITE_ENTRY_SUP:

                pGetLanRes->ConfigData.CipherSuiteSup = N0_OF_CIPHER_SUITE_SUPPORTED;
                break;

            case LAN_PARAM_CIPHER_SUITE_ENTRIES:
                {
                    int i;
                    _fmemset (pGetLanRes->ConfigData.CipherSuiteEntries, 0,
                              sizeof (pGetLanRes->ConfigData.CipherSuiteEntries));
                    for (i = 0; i < (N0_OF_CIPHER_SUITE_SUPPORTED); i++)
                    {
                        pGetLanRes->ConfigData.CipherSuiteEntries[i+1] = g_CipherRec[1 + i * 5];
                    }
                }
                break;

            case LAN_PARAM_CIPHER_SUITE_PRIV_LEVELS:
                //According to IPMI spec
                //We can not support more than 16 cipher suites at a time
                //But these cipher suites ID can be 0 to BFh
                //Initialize the response data
                _fmemset (pGetLanRes->ConfigData.CipherSuitePrivLevels, 0,
                          LanconfigParameterLength[pGetLanReq->ParameterSelect] );

                LOCK_BMC_SHARED_MEM(BMCInst);
                //copy the reserve byte in response
                pGetLanRes->ConfigData.CipherSuitePrivLevels [0] = pBMCInfo->LANCfs[EthIndex].CipherSuitePrivLevels [0];

                //Now retrieve the other privileges
                for( i=0; i<N0_OF_CIPHER_SUITE_SUPPORTED; i++)
                {
                    CipherSuiteID = g_CipherRec[(i * 5) + 1];
                    // The Cipher Suite ID is retrieved
                    // get the privilege level for the given cipher suite id Byte
                    CipherSuitePrivilege = pBMCInfo->LANCfs[EthIndex].CipherSuitePrivLevels [(CipherSuiteID/2)+1];
                    // if id is even its lower nibble else upper nibble 
                    CipherSuitePrivilege = ((CipherSuiteID % 2) == 0) ? (CipherSuitePrivilege & 0x0f) : ((CipherSuitePrivilege >> 4) & 0x0f) ;

                    //Copy the privileges in an temporary area
                    CipherSuitePrivilegelevels[i] = CipherSuitePrivilege;
                    TDBG("Retrieved CipherSuiteID : %x \t CipherSuitePrivilege : %x", CipherSuiteID, CipherSuitePrivilege);
                }
                UNLOCK_BMC_SHARED_MEM(BMCInst);

                //Now prepare the response
                for( i=1, j=1 ; i<= N0_OF_CIPHER_SUITE_SUPPORTED ; i=i+2, j++)
                {
                    // set the response privilege level according to supported cipher suite IDs
                    // Supported Cipher suite index (starting from 1) is decided from the cipher suite table g_CipherRec
                    // if index is even then its lower nibble else upper nibble 
                    pGetLanRes->ConfigData.CipherSuitePrivLevels [j] = 
                                ( ( (CipherSuitePrivilegelevels [i] << 4) & 0xf0) | CipherSuitePrivilegelevels [i-1] );
                }

                //if maximum cipher suite supported are an even number
                //then last supported cipher suite will not be processed by the above loop
                //So, copy the privilege of the last supported cipher suite in the response and pad upper nibble with 0
                if( (i-2) != N0_OF_CIPHER_SUITE_SUPPORTED)
                {
                    pGetLanRes->ConfigData.CipherSuitePrivLevels [j] = 
                           ( 0/*upper nibble doesn't exist*/ | CipherSuitePrivilegelevels [i-1] );
                }
                break;

            case LAN_PARAM_VLAN_TAGS:

                *((INT8U*)&pGetLanRes->ConfigData) = pGetLanReq->SetSelect;
                if (pGetLanReq->SetSelect > pBMCInfo->LANCfs[EthIndex].NumDest)
                {
                    pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_OUT_OF_RANGE;
                    return sizeof (GetLanCCRev_T);
                }
                if (0 == pGetLanReq->SetSelect)
                {
                    LOCK_BMC_SHARED_MEM(BMCInst);
                    _fmemcpy (((INT8U*) &pGetLanRes->ConfigData) + 1,&pSharedMem->VLANDestTag, sizeof(VLANDestTags_T));
                    UNLOCK_BMC_SHARED_MEM(BMCInst);
                }
                else
                {
                    if (pGetLanReq->SetSelect > pBMCInfo->LANCfs[EthIndex].NumDest)
                    {
                        pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_OUT_OF_RANGE;
                        return sizeof(GetLanCCRev_T);
                    }

                    LOCK_BMC_SHARED_MEM(BMCInst);
                    _fmemcpy (((INT8U*)&pGetLanRes->ConfigData) + 1,
                    &pBMCInfo->LANCfs[EthIndex].VLANDestTags[pGetLanReq->SetSelect - 1],
                    sizeof(VLANDestTags_T));
                    UNLOCK_BMC_SHARED_MEM(BMCInst);
                }
                break;

            case LAN_PARAMS_BAD_PASSWORD_THRESHOLD:
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(&pGetLanRes->ConfigData.BadPasswd,
                                &pBMCInfo->LANCfs[EthIndex].BadPasswd,sizeof(BadPassword_T));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                break;

            case LAN_PARAMS_IPV6_DHCPV6_TIMING_CONF_SUPPORT:
                pGetLanRes->ConfigData.IPv6_DHCPv6TimingConfSupport = pBMCInfo->LANCfs[EthIndex].IPv6_DHCPv6TimingConfSupport;
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);

            case LAN_PARAMS_IPV6_DHCPV6_TIMING_CONFIGURATION:
                /* check IPv6/IPv4 Addressing enables is 0*/
                if (pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable == 0)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof (INT8U);
                }

                /*if DHCPv6 Dynamic Address configuration is not supported, this parameter should not be implemented.*/
                if((pBMCInfo->LANCfs[EthIndex].IPv6Status.SLAAC_DHCPv6Addressing & 0x01) == 0)
                {
                    *pRes = CC_PARAM_NOT_SUPPORTED;
                    return sizeof (INT8U);
                }

                /*Check DHCPv6 timing configuration is supported*/
                if(pBMCInfo->LANCfs[EthIndex].IPv6_DHCPv6TimingConfSupport == 0x00)
                {
                    *pRes = CC_PARAM_NOT_SUPPORTED;
                    return sizeof (INT8U);
                }

                /*If DHCPv6 Timing Configuration is "Global" then Set selector should be 0 */
                if((pBMCInfo->LANCfs[EthIndex].IPv6_DHCPv6TimingConfSupport == 0x01) && (pGetLanReq->SetSelect != 0))
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }

                /*If DHCPv6 Timing Configuration is "Per Interface" then Set selector should not exceed Number of IA(interface)*/
                memset(InterfaceName,0,sizeof(InterfaceName));
                noofinterface=GetActiveIfcnameAndNoofIfc(InterfaceName);
                if(noofinterface == -1)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof (INT8U);
                }

                if(pGetLanReq->SetSelect >= noofinterface)
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }

                /*Maximum Supported Bloack Select is 2*/
                if(pGetLanReq->BlockSelect >= 2)
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }

                memcpy( &pGetLanRes->ConfigData.IPv6_DHCPv6TimingConf.TimingConf,
                        &pBMCInfo->LANCfs[EthIndex].DHCPv6TimingConf[ pGetLanReq->SetSelect][ pGetLanReq->BlockSelect*MAX_IPV6_BLOCK_SIZE],MAX_IPV6_BLOCK_SIZE);
                pGetLanRes->ConfigData.IPv6_DHCPv6TimingConf.SetSelect   = pGetLanReq->SetSelect;;
                pGetLanRes->ConfigData.IPv6_DHCPv6TimingConf.BlockSelect = pGetLanReq->BlockSelect;
                return sizeof(GetLanCCRev_T) + sizeof(IPv6_DHCPv6TimingConf_T);

            case LAN_PARAMS_IPV6_SLAAC_TIMING_CONF_SUPPORT:
                pGetLanRes->ConfigData.IPv6_SLAACTimingConfSupport = pBMCInfo->LANCfs[EthIndex].IPv6_SLAACTimingConfSupport;
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);

            case LAN_PARAMS_IPV6_SLAAC_TIMING_CONFIGURATION:
                /* check IPv6/IPv4 Addressing enables is 0*/
                if (pBMCInfo->LANCfs[EthIndex].IPv6IPv4AddrEnable == 0)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof (INT8U);
                }

                /*SLAAC Address configuration is not supported, this parameter should not be implemented.*/
                if(((pBMCInfo->LANCfs[EthIndex].IPv6Status.SLAAC_DHCPv6Addressing & 0x02) >> 1) == 0)
                {
                    *pRes = CC_PARAM_NOT_SUPPORTED;
                    return sizeof (INT8U);
                }

                /*Check SLAAC timing configuration is supported*/
                if(pBMCInfo->LANCfs[EthIndex].IPv6_SLAACTimingConfSupport == 0x00)
                {
                    *pRes = CC_PARAM_NOT_SUPPORTED;
                    return sizeof (INT8U);
                }

                /*If SLAAC Timing Configuration is "Global" then Set selector should be 0 */
                if((pBMCInfo->LANCfs[EthIndex].IPv6_SLAACTimingConfSupport == 0x01) && (pGetLanReq->SetSelect != 0))
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }

                /*If SLAAC Timing Configuration is "Per Interface" then Set selector should not exceed Number of IA(interface)*/
                memset(InterfaceName,0,sizeof(InterfaceName));
                noofinterface=GetActiveIfcnameAndNoofIfc(InterfaceName);
                if(noofinterface == -1)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return sizeof (INT8U);
                }

                if(pGetLanReq->SetSelect >= noofinterface)
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }

                memcpy( &pGetLanRes->ConfigData.IPv6_SLAACTimingConf.TimingConf,
                        &pBMCInfo->LANCfs[EthIndex].SLAACTimingConf[ pGetLanReq->SetSelect][ pGetLanReq->BlockSelect*MAX_IPV6_BLOCK_SIZE],MAX_IPV6_BLOCK_SIZE);

                pGetLanRes->ConfigData.IPv6_SLAACTimingConf.SetSelect   = pGetLanReq->SetSelect;;
                pGetLanRes->ConfigData.IPv6_SLAACTimingConf.BlockSelect = pGetLanReq->BlockSelect;

                return sizeof(GetLanCCRev_T) + sizeof(IPv6_SLAACTimingConf_T);
                break;

            case LAN_PARAM_IPV6_DYNAMIC_ADDRESS:

                if(pGetLanReq->SetSelect & 0xF0)
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }

                nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);


                pGetLanRes->ConfigData.IPv6_DynamicAddrs.SetSelector=	pGetLanReq->SetSelect;	
                pGetLanRes->ConfigData.IPv6_DynamicAddrs.IPv6_AddrSrcType=0x2; //DHCP Address
                pGetLanRes->ConfigData.IPv6_DynamicAddrs.IPv6_PrefixLength = NWConfig6.GlobalPrefix[(pGetLanReq->SetSelect  & 0x0F)]; 
                _fmemcpy (pGetLanRes->ConfigData.IPv6_DynamicAddrs.IPv6_DynamicAddress, NWConfig6.GlobalIPAddr[(pGetLanReq->SetSelect   & 0x0F)], IP6_ADDR_LEN);		
                pGetLanRes->ConfigData.IPv6_DynamicAddrs.IPv6_AddrStatus=0x0; //Active (In Use)
                return sizeof(GetLanCCRev_T) + sizeof (IPv6_DynamicAddrs_T);
                break;

            case LAN_PARAM_IPV6_IPV4_SUPPORT:
                /* Read Only Parameter */
                LOCK_BMC_SHARED_MEM(BMCInst);
                pGetLanRes->ConfigData.IPv6IPv4Support=pBMCInfo->LANCfs[EthIndex].IPv6IPv4Support;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                break;


            case LAN_PARAM_IPV6_IPV4_ADDRESS_ENABLE:
                EthIndex =GetEthIndex((pGetLanReq->ChannelNum & 0xf),BMCInst);
                if(GetIfcNameByIndex(EthIndex, IfcName) != 0)
                {
                    printf("Invalid EthIndex number %d\n",EthIndex);
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }

                memset(&NWConfig, 0, sizeof(NWCFG_STRUCT));
                memset(&NWConfig6, 0, sizeof(NWCFG6_STRUCT));

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
                    NWConfig6.enable = pBMCInfo->LANCfs[netindex].IPv6IPv4AddrEnable;
                    NWConfig.enable = pBMCInfo->LANCfs[netindex].IPv6IPv4AddrEnable;
                }
                else
                {
                    retValue = nwReadNWCfg_v4_v6(&NWConfig, &NWConfig6, EthIndex,g_corefeatures.global_ipv6);
                    if(retValue != 0)
                        TCRIT("Error in reading network configuration.\n"); 
                }

                switch(( pBMCInfo->LANCfs[netindex].IPv6IPv4AddrEnable) & (0x3))
                {
                    case DISABLE_V6:
                        pGetLanRes->ConfigData.IPv6IPv4AddrEnable=0x0;
                        break;
                    case EABLE_V6_DISABLE_V4:
                        pGetLanRes->ConfigData.IPv6IPv4AddrEnable=0x1;
                        break;
                    case ENABLE_V6_V4:
                        pGetLanRes->ConfigData.IPv6IPv4AddrEnable=0x2;
                        break;
                }
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                break;

            case LAN_PARAM_IPV6_HEADER_STATIC_TRAFFIC_CLASS:
                LOCK_BMC_SHARED_MEM(BMCInst);
                pGetLanRes->ConfigData.TrafficClass=pBMCInfo->LANCfs[EthIndex].TrafficClass;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                break;

            case LAN_PARAM_IPV6_HEADER_STATIC_HOP_LIMIT:
                LOCK_BMC_SHARED_MEM(BMCInst);
                pGetLanRes->ConfigData.HopLimit=pBMCInfo->LANCfs[EthIndex].HopLimit;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                break;

            case LAN_PARAM_IPV6_HEADER_FLOW_LABEL:
               /* Not allowed to set since RFC 2460 , section-6 says, Flow label is in experimental stage*/
               *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
               return(sizeof (INT8U));

            case LAN_PARAM_IPV6_STATUS:
                /* Read Only Parameter */
                pGetLanRes->ConfigData.IPv6Status.MaxStaticIPv6Addr=pBMCInfo->LANCfs[EthIndex].IPv6Status.MaxStaticIPv6Addr;
                pGetLanRes->ConfigData.IPv6Status.MaxDynamicIPv6Addr=pBMCInfo->LANCfs[EthIndex].IPv6Status.MaxDynamicIPv6Addr;
                pGetLanRes->ConfigData.IPv6Status.SLAAC_DHCPv6Addressing=pBMCInfo->LANCfs[EthIndex].IPv6Status.SLAAC_DHCPv6Addressing;
                return sizeof(GetLanCCRev_T) + sizeof(IPv6Status_T);
                break;
#if 0
                case LAN_PARAM_IPV6_STATIC_DUID_STORAGE_LEN:
                   LOCK_BMC_SHARED_MEM(BMCInst);
                pGetLanRes->ConfigData.StaticDUIDLength=pBMCInfo->LANCfs[EthIndex].StaticDUIDLength;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                   return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                break;

                case LAN_PARAM_IPV6_DYNAMIC_DUID_STORAGE_LEN:
                 LOCK_BMC_SHARED_MEM(BMCInst);
                pGetLanRes->ConfigData.StaticDUIDLength=pBMCInfo->LANCfs[EthIndex].DynamicDUIDLength;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                   return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                break;
#endif

            case LAN_PARAM_IPV6_STATIC_ADDRESS:

                if(pGetLanReq->SetSelect & 0xF0)
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }

                if((g_corefeatures.delayed_lan_restart_support) && (pBMCInfo->LANCfs[EthIndex].IPv6Addrs[(pGetLanReq->SetSelect & 0xF)].IPv6_AddrSrcType == STATIC_IP_SOURCE))
                {
                    if((pBMCInfo->LANCfs[EthIndex].IPv6_Enable))
                    {
                        NWConfig6.CfgMethod =  pBMCInfo->LANCfs[EthIndex].IPv6Addrs[(pGetLanReq->SetSelect & 0xF)].IPv6_AddrSrcType;
                        NWConfig6.GlobalPrefix[(pGetLanReq->SetSelect & 0x0F)] = pBMCInfo->LANCfs[EthIndex].IPv6Addrs[(pGetLanReq->SetSelect   & 0x0F)].IPv6_PrefixLength;
                        for(i=0;i<15;i++)			
                        _fmemcpy (NWConfig6.GlobalIPAddr,&pBMCInfo->LANCfs[EthIndex].IPv6Addrs[(pGetLanReq->SetSelect & 0xF)].IPv6_Address,IP6_ADDR_LEN);
                    }
                    else
                    {
                        memset(&NWConfig6, 0, sizeof(NWConfig6));
                    }
                }
                else
                {
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                }

                pGetLanRes->ConfigData.IPv6Addrs.SetSelector=	pGetLanReq->SetSelect;	

                if( NWConfig6.CfgMethod==DHCP_IP_SOURCE)
                {
                    pGetLanRes->ConfigData.IPv6Addrs.IPv6_AddrSrcType= 0x0; //Dynamic IPv6 Source
                }
                else if(NWConfig6.CfgMethod==STATIC_IP_SOURCE)
                {
                    pGetLanRes->ConfigData.IPv6Addrs.IPv6_AddrSrcType= 0x80; // Static IPv6 Source	
                }

                pGetLanRes->ConfigData.IPv6Addrs.IPv6_PrefixLength = NWConfig6.GlobalPrefix[(pGetLanReq->SetSelect  & 0x0F)];

                _fmemcpy (pGetLanRes->ConfigData.IPv6Addrs.IPv6_Address, NWConfig6.GlobalIPAddr[(pGetLanReq->SetSelect   & 0x0F)], IP6_ADDR_LEN);

                pGetLanRes->ConfigData.IPv6Addrs.IPv6_AddrStatus=0x0; //Active (In Use)

                return sizeof(GetLanCCRev_T) + sizeof (IPv6Addrs_T) ;
                break;

            case LAN_PARAMS_IPV6_ROUTER_ADDR_CONF_CNTL:
                LOCK_BMC_SHARED_MEM(BMCInst);
                pGetLanRes->ConfigData.IPv6_RA_Conf_Cntl_Enable =
                        pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);

            case LAN_PARAMS_IPV6_STATIC_ROUTER1_IPADDR:
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(&pGetLanRes->ConfigData.IPv6_Router1_IPAddr,
                                &pBMCInfo->LANCfs[EthIndex].IPv6_Router1_IPAddr,IP6_ADDR_LEN);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + IP6_ADDR_LEN;

            case LAN_PARAMS_IPV6_STATIC_ROUTER1_MACADDR:
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(&pGetLanRes->ConfigData.IPv6_Router1_MACAddr,
                                &pBMCInfo->LANCfs[EthIndex].IPv6_Router1_MACAddr,MAC_ADDR_LEN);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + MAC_ADDR_LEN;

            case LAN_PARAMS_IPV6_STATIC_ROUTER1_PREFIXLEN:
                LOCK_BMC_SHARED_MEM(BMCInst);
                pGetLanRes->ConfigData.IPv6_Router1_PrefixLen =
                        pBMCInfo->LANCfs[EthIndex].IPv6_Router1_PrefixLen;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);

            case LAN_PARAMS_IPV6_STATIC_ROUTER1_PREFIXVAL:
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(&pGetLanRes->ConfigData.IPv6_Router1_PrefixValue,
                                &pBMCInfo->LANCfs[EthIndex].IPv6_Router1_PrefixValue,
                                sizeof(pGetLanRes->ConfigData.IPv6_Router1_PrefixValue));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + sizeof(pGetLanRes->ConfigData.IPv6_Router1_PrefixValue);

            case LAN_PARAMS_IPV6_STATIC_ROUTER2_IPADDR:
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(&pGetLanRes->ConfigData.IPv6_Router2_IPAddr,
                                &pBMCInfo->LANCfs[EthIndex].IPv6_Router2_IPAddr,IP6_ADDR_LEN);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + IP6_ADDR_LEN;

            case LAN_PARAMS_IPV6_STATIC_ROUTER2_MACADDR:
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(&pGetLanRes->ConfigData.IPv6_Router2_MACAddr,
                                &pBMCInfo->LANCfs[EthIndex].IPv6_Router2_MACAddr,MAC_ADDR_LEN);
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + MAC_ADDR_LEN;

            case LAN_PARAMS_IPV6_STATIC_ROUTER2_PREFIXLEN:
                LOCK_BMC_SHARED_MEM(BMCInst);
                pGetLanRes->ConfigData.IPv6_Router2_PrefixLen =
                        pBMCInfo->LANCfs[EthIndex].IPv6_Router2_PrefixLen;
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);

            case LAN_PARAMS_IPV6_STATIC_ROUTER2_PREFIXVAL:
                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(&pGetLanRes->ConfigData.IPv6_Router2_PrefixValue,
                                &pBMCInfo->LANCfs[EthIndex].IPv6_Router2_PrefixValue,
                                sizeof(pGetLanRes->ConfigData.IPv6_Router1_PrefixValue));
                UNLOCK_BMC_SHARED_MEM(BMCInst);
                return sizeof(GetLanCCRev_T) + sizeof(pGetLanRes->ConfigData.IPv6_Router1_PrefixValue);

            case LAN_PARAMS_IPV6_NUM_DYNAMIC_ROUTER_INFO_SETS:
                /* if dynamic Router Address information entries are not supported */
                if (!( 0x2 & pBMCInfo->LANCfs[EthIndex].IPv6_RA_Conf_Cntl_Enable))
                {
                    pGetLanRes->ConfigData.IPv6_RA_Dynamic_InfoSetNum = 0x0;
                    return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                }
                nwReadRACfg(&raCfg,netindex);

                pGetLanRes->ConfigData.IPv6_RA_Dynamic_InfoSetNum = 
                                        raCfg.IPv6_RA_Dynamic_InfoSetNum;

                LOCK_BMC_SHARED_MEM(BMCInst);
                pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_InfoSetNum = 
                                            raCfg.IPv6_RA_Dynamic_InfoSetNum;
                UNLOCK_BMC_SHARED_MEM(BMCInst);

                return sizeof(GetLanCCRev_T) + sizeof(INT8U);

            case LAN_PARAMS_IPV6_DYNAMIC_ROUTER_INFO_IPADDR:

                if( pGetLanReq->SetSelect & 0xf0 )
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }

                if ( 0 == pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_InfoSetNum)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                }

                nwReadRACfg(&raCfg,netindex);

                _fmemcpy(pGetLanRes->ConfigData.IPv6_RA_Dynamic_InfoIPAddr,
                                raCfg.IPv6_RA_Dynamic_InfoIPAddr[pGetLanReq->SetSelect],
                                IP6_ADDR_LEN);

                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_InfoIPAddr,
                                 raCfg.IPv6_RA_Dynamic_InfoIPAddr,
                                 (MAX_IPV6ADDRS * IP6_ADDR_LEN));
                UNLOCK_BMC_SHARED_MEM(BMCInst);

                return sizeof(GetLanCCRev_T) + IP6_ADDR_LEN;

            case LAN_PARAMS_IPV6_DYNAMIC_ROUTER_INFO_MACADDR:
            //TODO: HOW to get the MAC Address
                if(	pGetLanReq->SetSelect & 0xf0 )
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }

                if ( 0 == pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_InfoSetNum)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                }

                _fmemcpy(pGetLanRes->ConfigData.IPv6_RA_Dynamic_InfoMACAddr,
                                &pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_InfoMACAddr,
                                (MAX_IPV6ADDRS * MAC_ADDR_LEN));

                return sizeof(GetLanCCRev_T) + MAC_ADDR_LEN;

            case LAN_PARAMS_IPV6_DYNAMIC_ROUTER_INFO_PREFIXLEN:
                if(pGetLanReq->SetSelect & 0xf0 )
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }

                if ( 0 == pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_InfoSetNum)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                }

                nwReadRACfg(&raCfg,netindex);

                pGetLanRes->ConfigData.IPv6_RA_Dynamic_InfoPrefixLen[0] =
                                raCfg.IPv6_RA_Dynamic_InfoPrefixLen[pGetLanReq->SetSelect];

                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_InfoPrefixLen,
                                raCfg.IPv6_RA_Dynamic_InfoPrefixLen,
                                MAX_IPV6ADDRS );
                UNLOCK_BMC_SHARED_MEM(BMCInst);

                return sizeof(GetLanCCRev_T) + sizeof(INT8U);

            case LAN_PARAMS_IPV6_DYNAMIC_ROUTER_INFO_PREFIXVAL:
                if(pGetLanReq->SetSelect & 0xf0 )
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    return sizeof (*pRes);
                }

                if ( 0 == pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_InfoSetNum)
                {
                    *pRes = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                    return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                }

                nwReadRACfg(&raCfg,netindex);

                _fmemcpy(pGetLanRes->ConfigData.IPv6_RA_Dynamic_InfoPrefixValue,
                                raCfg.IPv6_RA_Dynamic_InfoPrefixValue[pGetLanReq->SetSelect],
                                IP6_ADDR_LEN);

                LOCK_BMC_SHARED_MEM(BMCInst);
                _fmemcpy(pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_InfoPrefixValue,
                                raCfg.IPv6_RA_Dynamic_InfoPrefixValue,
                                (MAX_IPV6ADDRS * IP6_PREFIX_MAXLEN));
                UNLOCK_BMC_SHARED_MEM(BMCInst);

                return sizeof(GetLanCCRev_T) + IP6_ADDR_LEN;

            case LAN_PARAMS_IPV6_DYNAMIC_ROUTER_RECV_HOPLIMIT:
                //TODO:  How to get the HOPLIMIT
                pGetLanRes->ConfigData.IPv6_RA_Dynamic_RecvHopLimit =
                        pBMCInfo->LANCfs[EthIndex].IPv6_RA_Dynamic_RecvHopLimit;
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);

            case LAN_PARAMS_AMI_OEM_SNMPV6_DEST_ADDR:

                if ( pGetLanReq->SetSelect   > pBMCInfo->LANCfs[EthIndex].NumDest )
                {
                    *pRes = CC_PARAM_OUT_OF_RANGE ;
                    return sizeof (*pRes);
                }

                if (0 == pGetLanReq->SetSelect)
                {
                    LOCK_BMC_SHARED_MEM(BMCInst);
                    _fmemcpy (&pGetLanRes->ConfigData.Destv6Addr,
                              &pSharedMem->VolLANv6Dest[EthIndex], sizeof(LANDestv6Addr_T));
                    UNLOCK_BMC_SHARED_MEM(BMCInst);
                }
                else
                {
                    LOCK_BMC_SHARED_MEM(BMCInst);
                    _fmemcpy (&pGetLanRes->ConfigData.Destv6Addr,
                              &(pBMCInfo->LANCfs[EthIndex].Destv6Addr[pGetLanReq->SetSelect - 1]),
                              sizeof(LANDestv6Addr_T));
                    UNLOCK_BMC_SHARED_MEM(BMCInst);
                }

                TDBG("\n GetLanconfig: Getting SNMPv6 configuration done..\n");

                return sizeof(GetLanConfigRes_T);

                break;

            case LAN_PARAMS_AMI_OEM_ENABLE_SET_MAC:

            	pGetLanRes->ConfigData.ChangeMACEnabled = enableSetMACAddr;
            	return sizeof(GetLanCCRev_T) + sizeof(INT8U);

            case LAN_PARAMS_AMI_OEM_IPV6_ENABLE:
                if(g_corefeatures.delayed_lan_restart_support) 
                {
                    NWConfig6.enable = pBMCInfo->LANCfs[EthIndex].IPv6_Enable;
                }
                else
                {
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                }
                pGetLanRes->ConfigData.IPv6_Enable = NWConfig6.enable;
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                
                break;

            case LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR_SOURCE:
                if(g_corefeatures.delayed_lan_restart_support) 
                {
                    if((pBMCInfo->LANCfs[EthIndex].IPv6_Enable))
                    {
                        NWConfig6.CfgMethod = pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc;
                    }
                    else
                    {
                        memset(&NWConfig6, 0, sizeof(NWConfig6));
                    }
                }
                else
                {
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                }
                pGetLanRes->ConfigData.IPv6_IPAddrSrc = NWConfig6.CfgMethod;
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                
                break;

            case LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR:
            case LAN_PARAMS_AMI_OEM_IPV6_IP_ADDR_EUI64:
                if(pGetLanReq->SetSelect & 0xF0)
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }
               
                if((g_corefeatures.delayed_lan_restart_support) && (pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc == STATIC_IP_SOURCE))
                {
                    if((pBMCInfo->LANCfs[EthIndex].IPv6_Enable))
                    {
                        _fmemcpy (NWConfig6.GlobalIPAddr,pBMCInfo->LANCfs[EthIndex].IPv6_IPAddr, IP6_ADDR_LEN*MAX_IPV6ADDRS);
                    }
                    else
                    {
                        memset(&NWConfig6, 0, sizeof(NWConfig6));
                    }
                }
                else
                {
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                }
                _fmemcpy (pGetLanRes->ConfigData.IPv6_LinkAddr, NWConfig6.GlobalIPAddr[(pGetLanReq->SetSelect & 0x0F)], IP6_ADDR_LEN);
                return sizeof(GetLanCCRev_T) + IP6_ADDR_LEN;
                
                break;
            case LAN_PARAMS_AMI_OEM_IPV6_LINK_ADDR:
                nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                _fmemcpy (pGetLanRes->ConfigData.IPv6_LinkAddr, NWConfig6.LinkIPAddr, IP6_ADDR_LEN);
                return sizeof(GetLanCCRev_T) + IP6_ADDR_LEN;

            case LAN_PARAMS_AMI_OEM_IPV6_PREFIX_LENGTH:

                if(pGetLanReq->SetSelect & 0xF0)
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }

                if((g_corefeatures.delayed_lan_restart_support) && (pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc == STATIC_IP_SOURCE))
                {
                    if((pBMCInfo->LANCfs[EthIndex].IPv6_Enable))
                    {
                        NWConfig6.GlobalPrefix[(pGetLanReq->SetSelect & 0x0F)] = pBMCInfo->LANCfs[EthIndex].IPv6_PrefixLen[(pGetLanReq->SetSelect & 0x0F)];
                    }
                    else
                    {
                        memset(&NWConfig6, 0, sizeof(NWConfig6));
                    }
                }
                else
                {
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                }
                pGetLanRes->ConfigData.IPv6_LinkAddrPrefix = NWConfig6.GlobalPrefix[(pGetLanReq->SetSelect & 0x0F)];
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                
                break;

            case LAN_PARAMS_AMI_OEM_IPV6_LINK_ADDR_PREFIX:
                nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                pGetLanRes->ConfigData.IPv6_LinkAddrPrefix = NWConfig6.LinkPrefix;
                return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                
                break;

            case LAN_PARAMS_AMI_OEM_IPV6_GATEWAY_IP:
                if((g_corefeatures.delayed_lan_restart_support) && (pBMCInfo->LANCfs[EthIndex].IPv6_IPAddrSrc == STATIC_IP_SOURCE))
                {
                    if((pBMCInfo->LANCfs[EthIndex].IPv6_Enable))
                    {
                        _fmemcpy (NWConfig6.Gateway,pBMCInfo->LANCfs[EthIndex].IPv6_GatewayIPAddr, IP6_ADDR_LEN);
                    }
                    else
                    {
                        memset(&NWConfig6, 0, sizeof(NWConfig6));
                    }
                }
                else
                {
                    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
                }
                _fmemcpy (pGetLanRes->ConfigData.IPv6_GatewayIPAddr, NWConfig6.Gateway, IP6_ADDR_LEN);
                return sizeof(GetLanCCRev_T) + IP6_ADDR_LEN;
                
                break;

            case LAN_PARAMS_AMI_OEM_PHY_SETTINGS:
                memset(IfcName, 0, sizeof(IfcName));
                if(GetIfcNameByIndex(EthIndex, IfcName) != 0)
                {
                    TCRIT("Error in Getting Interface Name for the lan Index:%d\n",EthIndex);
                }
                if(nwGetEthInformation(&PHYCfg, IfcName) !=0)
                {
                    pGetLanRes->CCParamRev.CompletionCode = CC_UNSPECIFIED_ERR;
                    return sizeof(GetLanCCRev_T);
                }
                pGetLanRes->ConfigData.PHYConfig.Interface = EthIndex;
                pGetLanRes->ConfigData.PHYConfig.AutoNegotiationEnable = PHYCfg.autoneg;
                pGetLanRes->ConfigData.PHYConfig.Speed = PHYCfg.speed;
                pGetLanRes->ConfigData.PHYConfig.Duplex = PHYCfg.duplex;
                pGetLanRes->ConfigData.PHYConfig.CapabilitiesSupported = PHYCfg.supported;

                return sizeof(GetLanCCRev_T) + sizeof(PHYConfig_T);

            case LAN_PARAMS_AMI_OEM_MTU_SETTINGS:
                        memset(IfcName,0,sizeof(IfcName));
                        if(GetIfcNameByIndex(EthIndex, IfcName) != 0)
                        {
                            TCRIT("Error in Getting Interface Name for the lan Index:%d\n",EthIndex);
                            pGetLanRes->CCParamRev.CompletionCode = CC_UNSPECIFIED_ERR;
                            return sizeof(GetLanCCRev_T);

                        }
                        if(nwGetEthInformation(&PHYCfg, IfcName) !=0)
                        {
                            pGetLanRes->CCParamRev.CompletionCode = CC_UNSPECIFIED_ERR;
                            return sizeof(GetLanCCRev_T);
                        }
					
                        pGetLanRes->ConfigData.MTU_size = PHYCfg.maxtxpkt;
                        return sizeof(GetLanCCRev_T) + sizeof(INT16U);
				
        case LAN_PARAMS_SSI_OEM_2ND_PRI_ETH_MAC_ADDR:
            if(g_corefeatures.ssi_support == ENABLED)
            {
                netindex = 0x1; /* Specify the 2nd interface */
		EthIndex = 0x1; /* Specify the 2nd interface */

                if ( IsBondingActive (BMCInst) )
		{
			for ( i = 0; i < sizeof(pBMCInfo->LanIfcConfig)/sizeof(LANIFCConfig_T); i++)
			{
			        if(pBMCInfo->LanIfcConfig[i].Ethindex == EthIndex )
			        {
					 nwGetMACAddrInBondConf(&NWConfig,pBMCInfo->LanIfcConfig[i].ifname);
					 break;
			        }
    			}
		}
                else
	                nwReadNWCfg_v4_v6(&NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);

                _fmemcpy(pGetLanRes->ConfigData.SSI2ndPriEthMACAddr, NWConfig.MAC, MAC_ADDR_LEN);
                return sizeof(GetLanCCRev_T) + sizeof(pGetLanRes->ConfigData.SSI2ndPriEthMACAddr);
             }
             else
             {
                pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
             }
            break;

        case LAN_PARAMS_SSI_OEM_LINK_CTRL:
            if(g_corefeatures.ssi_support == ENABLED)
            {
                pGetLanRes->ConfigData.SSILinkControl = 0;

                pChannelInfo = getChannelInfo(pBMCInfo->RMCPLAN1Ch, BMCInst);
                if(NULL == pChannelInfo)
                {
                    *pRes = CC_UNSPECIFIED_ERR;
                    return	sizeof (*pRes);
                }

                if (pChannelInfo->AccessMode == 0x02)		/* If 1st channal is available */
                    pGetLanRes->ConfigData.SSILinkControl |= BIT0;
                NIC_Count = g_coremacros.global_nic_count;
                if (NIC_Count == 2)
                {
                    pChannelInfo = getChannelInfo(pBMCInfo->RMCPLAN2Ch, BMCInst);
                    if(NULL == pChannelInfo)
                    {
                        *pRes = CC_UNSPECIFIED_ERR;
                        return	sizeof (*pRes);
                    }

                    if (pChannelInfo->AccessMode == 0x02)	/* If 2nd channal is available */
                        pGetLanRes->ConfigData.SSILinkControl |= BIT1;
                }
                return sizeof(GetLanCCRev_T) + sizeof(pGetLanRes->ConfigData.SSILinkControl);
            }
            else
            {
                pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }
            break;

        case LAN_PARAMS_SSI_OEM_CMM_IP_ADDR:
            if(g_corefeatures.ssi_support == ENABLED)
            {
                _fmemcpy (pGetLanRes->ConfigData.CMMIPAddr, pBMCInfo->SSIConfig.CMMIPAddr, IP_ADDR_LEN);
                return sizeof(GetLanCCRev_T) + sizeof(pGetLanRes->ConfigData.CMMIPAddr);
            }
            else
            {
                pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }
            break;

        default:
            if(g_corefeatures.ncsi_cmd_support == ENABLED)
            {
                switch(pGetLanReq->ParameterSelect)
                {
                    case LAN_PARAMS_AMI_OEM_NCSI_CONFIG_NUM:
                        NCSIGetTotalPorts(&ncsiPortConfigNum);
                        
                        if (ncsiPortConfigNum >= 0xFF)
                        {
                            pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                            return sizeof(GetLanCCRev_T);
                        }
                        else
                        {
                            pGetLanRes->ConfigData.NumNCSIPortConfigs = ncsiPortConfigNum;
                            return sizeof(GetLanCCRev_T) + sizeof(INT8U);
                        }
                    break;
                                
                    case LAN_PARAMS_AMI_OEM_NCSI_SETTINGS:
                    {
                        NIC_Count=g_coremacros.global_nic_count;

                        if (pGetLanReq->SetSelect >= NIC_Count)
                        {
                            pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                            return sizeof(GetLanCCRev_T);
                        }
                        NCSIConfig_T configData;
                        char interfaceName[8];

                        memset(&configData, 0, sizeof(NCSIConfig_T));
                        memset(interfaceName, 0, sizeof(interfaceName));

                        snprintf(interfaceName, sizeof(interfaceName), "%s%d", "eth", pGetLanReq->SetSelect);
                        
                        if (NCSIGetPortConfigByName(interfaceName, &configData) != 0)
                        {
                            pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                            return sizeof(GetLanCCRev_T);
                        }

                        if (configData.PackageId >= 0xFF || configData.ChannelId >= 0xFF)
                        {
                            pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                            return sizeof(GetLanCCRev_T);
                        }

                        pGetLanRes->ConfigData.NCSIPortConfig.Interface = pGetLanReq->SetSelect;
                        pGetLanRes->ConfigData.NCSIPortConfig.PackageId = configData.PackageId;
                        pGetLanRes->ConfigData.NCSIPortConfig.ChannelId = configData.ChannelId;

                        return sizeof(GetLanCCRev_T) + sizeof(NCSIPortConfig_T);
                    }
                    break;

                    case LAN_PARAMS_AMI_OEM_NCSI_MODE_CHANGE:
                    {
                        NIC_Count=g_coremacros.global_nic_count;
                        if (pGetLanReq->SetSelect >= NIC_Count)
                        {
                            pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                            return sizeof(GetLanCCRev_T);
                        }

                        NCSIConfig_T configData;
                        char interfaceName[8];
                        memset(&configData, 0, sizeof(NCSIConfig_T));
                        memset(interfaceName, 0, sizeof(interfaceName));

                        snprintf(interfaceName, sizeof(interfaceName), "%s%d", "eth", pGetLanReq->SetSelect);

                        if (NCSIGetPortConfigByName(interfaceName, &configData) != 0)
                        {
                            pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                            return sizeof(GetLanCCRev_T);
                        }

                        if (configData.AutoSelect>= 0xFF)
                        {
                            pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                            return sizeof(GetLanCCRev_T);
                        }

                        pGetLanRes->ConfigData.NCSIModeConfig.Interface = pGetLanReq->SetSelect;
                        pGetLanRes->ConfigData.NCSIModeConfig.NCSIMode = configData.AutoSelect;

                        return sizeof(GetLanCCRev_T) + sizeof(NCSIModeConfig_T);
                    }
                    break;
                    case LAN_PARAMS_AMI_OEM_NCSI_SETTINGS_BY_PORT_NUM:
                    {
                    	NCSIGetTotalPorts(&ncsiPortConfigNum);
                        if ((pGetLanReq->SetSelect > ncsiPortConfigNum) || (pGetLanReq->SetSelect == 0))
                        {
                            pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                            return sizeof(GetLanCCRev_T);
                        }

                        NCSIConfig_T configData;
                        int PortNum = pGetLanReq->SetSelect;
                        if (NCSIGetPortConfig(PortNum,&configData) != 0)
                        {
                        	pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                        	return sizeof(GetLanCCRev_T);
                        }

                        pGetLanRes->ConfigData.NCSICfg.PortNum = pGetLanReq->SetSelect;
                        pGetLanRes->ConfigData.NCSICfg.InterfaceIndex = atoi(&configData.InterfaceName[3]);
                        pGetLanRes->ConfigData.NCSICfg.PackageId = configData.PackageId;
                        pGetLanRes->ConfigData.NCSICfg.ChannelId = configData.ChannelId;
                        return sizeof(GetLanCCRev_T) + sizeof(NCSICfg_T);
                        break;
                    }

                    case LAN_PARAMS_AMI_OEM_NCSI_EXTENSION:
                    {
                        char ncsi_default_ifc[IFNAMSIZ + 1] = "eth1";
                        
                        if ((pGetLanReq->SetSelect == 0x20) && 
                            (g_corefeatures.ncsi_keep_phy_linkup_support == ENABLED))
                        {
                            NIC_Count=g_coremacros.global_nic_count;
                            NCSIConfig_T configData;
                            char interfaceName[8];
                            memset(&configData, 0, sizeof(NCSIConfig_T));
                            memset(interfaceName, 0, sizeof(interfaceName));

                            GetMacrodefine_string("CONFIG_SPX_FEATURE_NCSI_DEFAULT_INTERFACE", ncsi_default_ifc);
                            snprintf(interfaceName, sizeof(interfaceName), "%s", ncsi_default_ifc);

                            if (NCSIGetPortConfigByName(interfaceName, &configData) != 0)
                            {
                                pGetLanRes->CCParamRev.CompletionCode = CC_INV_DATA_FIELD;
                                return sizeof(GetLanCCRev_T);
                            }

                            char Interface[2];
                            snprintf(Interface, sizeof(Interface), "%c", interfaceName[3]);
                            pGetLanRes->ConfigData.NCSIPHYConfigGet.Interface = atoi(Interface);
                            pGetLanRes->ConfigData.NCSIPHYConfigGet.VetoBit = configData.VetoBit;

                            return sizeof(GetLanCCRev_T) + sizeof(NCSIPHYConfigGet_T);
                    }
                    else
                    {
                        pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_NOT_SUPPORTED;
                        return sizeof(GetLanCCRev_T);
                    }
                	}
                    break;
					
                    default:
                        TDBG("In Valid Option\n");
                        
                }
            }
            pGetLanRes->CCParamRev.CompletionCode = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
        }

    }

    
    return sizeof(GetLanCCRev_T) + LanconfigParameterLength[pGetLanReq->ParameterSelect];
}


/*---------------------------------------------------
 * SuspendBMCArps
 *---------------------------------------------------*/
int
SuspendBMCArps (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    SuspendBMCArpsReq_T*    pArpReq = (SuspendBMCArpsReq_T*) pReq;
    SuspendBMCArpsRes_T*    pArpRes = (SuspendBMCArpsRes_T*) pRes;
    INT8U EthIndex;

    /* Verify Channel ID */
    pArpRes->CompletionCode = CC_INV_DATA_FIELD;

    if(pArpReq->ChannelNo & RESERVED_BITS_SUSPENDBMCARPS) return sizeof(*pRes);

    if (!IsLANChannel(pArpReq->ChannelNo & CHANNEL_ID_MASK, BMCInst) )
    {
        return sizeof (*pRes);
    }

    EthIndex= GetEthIndex(pArpReq->ChannelNo & 0x0F,BMCInst);
    if(0xff == EthIndex)
    {
        return sizeof (*pRes);
    }

    /* Reserved Bits Validation */
	if (0 != (SUSPEND_ARP_RSVD_BIT_MASK & pArpReq->ArpSuspend))
    {
        return sizeof (*pRes);
    }

	if (BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning == TRUE)
		BMC_GET_SHARED_MEM(BMCInst)->ArpSuspendStatus[EthIndex] = pArpReq->ArpSuspend;

    /* Update Response */
    pArpRes->CompletionCode   = CC_NORMAL;
    pArpRes->ArpSuspendStatus = UpdateArpStatus(EthIndex, BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning, BMCInst);

    return sizeof (SuspendBMCArpsRes_T);
}



/*---------------------------------------------------
 * GetIPUDPRMCPStats
 *---------------------------------------------------*/
int
GetIPUDPRMCPStats (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetIPUDPRMCPStatsReq_T*    pGetIPUDPRMCPStatsReq = (GetIPUDPRMCPStatsReq_T*) pReq;
    GetIPUDPRMCPStatsRes_T*    pGetIPUDPRMCPStatsRes = (GetIPUDPRMCPStatsRes_T*) pRes;
    _FAR_	BMCSharedMem_T* 	pSharedMem = BMC_GET_SHARED_MEM (BMCInst);

    FILE *fptr;
    char FDRead[512], FSRead[512];
    char *result = NULL;
    char **StrArray;
    int count = 0;
    int cStrings =0;

    if(pGetIPUDPRMCPStatsReq->ChannelNo & RESERVED_BITS_GETIPUDPRMCPSTATS_CH)
    {
    	*pRes = CC_INV_DATA_FIELD;
    	return sizeof(*pRes);
    }

    if(pGetIPUDPRMCPStatsReq->ClearStatus & RESERVED_BITS_GETIPUDPRMCPSTATS_CLRSTATE)
    {
    	*pRes = CC_INV_DATA_FIELD;
    	return sizeof(*pRes);
    }

    //Channel number [3:0] is valid.
    if ( !IsLANChannel(pGetIPUDPRMCPStatsReq->ChannelNo & CHANNEL_ID_MASK, BMCInst) )
    {
        pGetIPUDPRMCPStatsRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    //Clear the statistics if 1 is given in request.
    if ( pGetIPUDPRMCPStatsReq->ClearStatus & 0x01 )
    {
        /* <TBD BalaT>
        * Have to clear the already stored statistics when clear req is 1.
        */
        pGetIPUDPRMCPStatsRes->CompletionCode = CC_NORMAL;
        return sizeof (INT8U);
    }

    //All Statistics values are initialised to 0.
    memset (pGetIPUDPRMCPStatsRes, 0, sizeof(GetIPUDPRMCPStatsRes_T) );

    //All the datas are taken from /proc/net/snmp file
    fptr = fopen ("/proc/net/snmp","r+");
    if (fptr == NULL)
    {
        pGetIPUDPRMCPStatsRes->CompletionCode = CC_COULD_NOT_PROVIDE_RESP;
        return sizeof (INT8U);
    }

    while( NULL != fgets(FDRead,512,fptr) )
    {
       if ( NULL != strstr (FDRead, "Ip: ") )
       {
          count++;
          if (count == 2)
          {
             //To find the no. of valid strings in a line.
             strcpy (FSRead, FDRead);
             result = strtok( FSRead, " " );
             while( result != NULL )
            {
                cStrings++;
                result = strtok( NULL, " " );
            }

            //Condition to check so that explode doesnt try to read the strings from unknown location.
            if ( cStrings == 20)
            {
                StrArray = explode (' ', FDRead);
            }
            else
            {
                pGetIPUDPRMCPStatsRes->CompletionCode = CC_NORMAL;
                return sizeof (INT8U);
             }

             //All Statistics stops acumulating at FFFFh unless otherwise noted.
             //IP packets received.
            if ( atol((char *)StrArray[3]) > 0xffff)
                pGetIPUDPRMCPStatsRes->IPPacketsRecv = ( ( atol((char *)StrArray[3]) ) % 65535);
            else
                pGetIPUDPRMCPStatsRes->IPPacketsRecv = atoi((char *)StrArray[3]);

            //IP Header Error.
            pGetIPUDPRMCPStatsRes->IPHeaderErr = atoi((char *)StrArray[4]);

             //IP Address Error.
             if ( atol((char *)StrArray[5]) > 0xffff)
                pGetIPUDPRMCPStatsRes->IPAddrErr = ( ( atol((char *)StrArray[5]) ) % 65535);
             else
                pGetIPUDPRMCPStatsRes->IPAddrErr = atoi((char *)StrArray[5]);

            //Fragmented IP Packets received.
            if ( atol((char *)StrArray[17]) > 0xffff)
                pGetIPUDPRMCPStatsRes->FragIPPacketsRecv = ( ( atol((char *)StrArray[17]) ) % 65535);
            else
                pGetIPUDPRMCPStatsRes->FragIPPacketsRecv = atoi((char *)StrArray[17]);

            //IP packets Transmitted.
            if ( atol((char *)StrArray[10]) > 0xffff)
                pGetIPUDPRMCPStatsRes->IPPacketsTrans = ( ( atol((char *)StrArray[10]) ) % 65535);
            else
                 pGetIPUDPRMCPStatsRes->IPPacketsTrans = atoi((char *)StrArray[10]);

             count = 0;
          }
       }
       
        if ( NULL != strstr (FDRead, "Udp: ") )
        {
            count++;
            if (count == 2)
            {
                //To find the no. of valid strings in a line.
                cStrings = 0;
                strcpy (FSRead, FDRead);
                result = strtok( FSRead, " " );
                while( result != NULL )
                {
                    cStrings++;
                    result = strtok( NULL, " " );
                }

                //Condition to check so that explode doesnt try to read the strings beyond the valid location.
                if ( cStrings == 5)
                {
                    StrArray = explode (' ', FDRead);
                }
                else
                {
                    pGetIPUDPRMCPStatsRes->CompletionCode = CC_NORMAL;
                    return sizeof (INT8U);
                }

                //UDP packets received.
                if ( atol((char *)StrArray[1]) > 0xffff)
                    pGetIPUDPRMCPStatsRes->UDPPacketsRecv = ( ( atol((char *)StrArray[1]) ) % 65535);
                else
                    pGetIPUDPRMCPStatsRes->UDPPacketsRecv = atoi((char *)StrArray[1]);

                count = 0;
            }
        }
    }
    fclose(fptr);

    //Valid RMCP packets received.
    pGetIPUDPRMCPStatsRes->ValidRMCPPackets = pSharedMem->gIPUDPRMCPStats;

    //<TBD BalaT>To store the statistics across the system reset and power cycles

    pGetIPUDPRMCPStatsRes->CompletionCode = CC_NORMAL;
    return sizeof (GetIPUDPRMCPStatsRes_T);

}

/*-----------------------------------------------------
 * explode
 * Funntion to split the strings and store in an array
 *-----------------------------------------------------*/

char **explode(char separator, char *string)
{
    int start = 0, i, k = 1, count = 2;
    char **strarr;

    for (i = 0; string[i] != '\0'; i++)
        /* how many rows do we need for our array? */
        if (string[i] == separator)
            count++;

    /* count is at least 2 to make room for the entire string
 *      * and the ending NULL */
    strarr = malloc(count * sizeof(char*));
    i = 0;

    while (*string++ != '\0')
    {
        if (*string == separator)
        {
            strarr[i] = malloc(k - start + 2);
            strncpy(strarr[i], string - k + start, k - start + 1);
            strarr[i][k - start + 1] = '\0'; /* guarantee null termination */
            start = k;
            i++;
        }
        k++;
    }
    /* copy the last part of the string after the last separator */
    strarr[i] = malloc(k - start);
    strncpy(strarr[i], string - k + start, k - start - 1);
    strarr[i][k - start - 1] = '\0'; /* guarantee null termination */
    strarr[++i] = NULL;

    return strarr;
}

/*----------------------------------------------
 * UpdateArpStatus
 *----------------------------------------------*/
INT8U
UpdateArpStatus (INT8U EthIndex,BOOL IsTimerRunning, int BMCInst)
{
    INT8U GratArpSuspend;
    INT8U ArpSuspend;
    INT8U Status;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    char Cmds[50]={0}; // command string to perform BMC-generated ARP
    char IfcName[16];

    IPMI_DBG_PRINT_1 ("Timer - %x", IsTimerRunning);
    GratArpSuspend = ArpSuspend = 1;

    // Check Gratuitous ARP is Enabled
    if (0 == (pBMCInfo->LANCfs[EthIndex].BMCGeneratedARPControl & GRATIUTOUS_ENABLE_MASK))
    {
        GratArpSuspend = 0;
    }

    // Check ARP Response is Enabled
    if (0 == (pBMCInfo->LANCfs[EthIndex].BMCGeneratedARPControl & ENABLE_ARP_RESPONSES))
    {
        ArpSuspend = 0;
    }

    /*Disable ARP */
    if (TRUE == IsTimerRunning)
    {
		/* WDT is running, check and suspend ARP if necessary */	
		if( (0 != (BMC_GET_SHARED_MEM(BMCInst)->ArpSuspendStatus[EthIndex] & SUSPEND_GRAT_ARP)) &&
			(0 < GratArpSuspend) )
        {
            GratArpSuspend--;
        }

		if( (0 != (BMC_GET_SHARED_MEM(BMCInst)->ArpSuspendStatus[EthIndex] & SUSPEND_ARP)) &&
            (0 < ArpSuspend) )
        {
            ArpSuspend--;
        }
    }
    
    memset(IfcName,0,sizeof(IfcName));
    if(GetIfcName(EthIndex, IfcName, BMCInst) != 0)
    {
        TCRIT("Error in getting Interface Name for the Lan Index :%d\n", EthIndex);
    }
    else
    {
    	/* Perform commands for BMC-generated Arp */
    	memset(Cmds, 0, sizeof(Cmds));
    	sprintf(Cmds, "/usr/local/bin/ArpSwitch.sh %s %d", IfcName, (!ArpSuspend) ? ARP_IGNORE_ON : ARP_IGNORE_OFF);
    	safe_system (Cmds);	
    	/* Perform commands for BMC-generated Arp ends */
    }
    
    /* Update Status */
    Status = ArpSuspend << 1;
    Status = Status | GratArpSuspend;

    return Status;
}


/**
*@fn NwInterfacePresenceCheck
*@brief This function is invoked to check network interface presence
*@param Interface - Char Pointer to Interface for which interface to check
*@return Returns 0 on success
*/
static int
NwInterfacePresenceCheck (char * Interface)
{
    int r;
    int sockdes;
    struct ifreq Ifreq;
    unsigned char MAC[MAC_ADDR_LEN];

    IPMI_DBG_PRINT_1 ("Checking the presence of %s\n", Interface);

    sockdes = socket(PF_INET, SOCK_DGRAM, 0 );
    if ( sockdes < 0 )
    {
        IPMI_ERROR("can't open socket: %s\n",strerror(errno));
        return -1;
    }

    /* Get MAC address */
    memset(&Ifreq,0,sizeof(struct ifreq));
    memset(MAC, 0, MAC_ADDR_LEN);
    strcpy(Ifreq.ifr_name, Interface);
    Ifreq.ifr_hwaddr.sa_family = AF_INET;
    r = ioctl(sockdes, SIOCGIFHWADDR, &Ifreq);
    close (sockdes);
    if ( r < 0 )
    {
        //IPMI_ERROR("IOCTL to get MAC failed: %d\n",r);
        return -1;
    }
    IPMI_DBG_PRINT_1 (" %s Interface is present\n", Interface);

    return 0;
}


