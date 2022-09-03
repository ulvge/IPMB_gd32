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
 * AMISmtp.c
 * AMI SMTP extension Implementation
 *
 * Author: shivaranjanic <shivaranjanic@amiindia.co.in>
 *
 ******************************************************************/

#define ENABLE_DEBUG_MACROS    0

#include "Support.h"
#include "PMConfig.h"
#include "NVRData.h"
#include "NVRAccess.h"
#include "IPMIDefs.h"
#include "IPMI_AMISmtp.h"
#include "AMISmtp.h"
#include "Platform.h"
#include "Ethaddr.h"
#include <arpa/inet.h>
#include "IPMIConf.h"
#include "blowfish.h"
#include "Encode.h"
#include "featuredef.h"
#include "nwcfg.h"
#include "Util.h"

#define ENABLE_DISABLE_MASK 			0xFE

const INT8U g_SMTPConfigParameterLength [] = {
    1,                      /* Enable/disable */
    4,                      /* Server IP Address */
    0xFF,                   /* User Name */
    0xFF,           /* Passwd */
    0x1,                  /* No of destinations */
    1,                     /* SMTP user ID*/
    0xFF,                  /* Mail subject */
    0xFF ,                /* Message */
    0xFF,                 /*Sender Email Address */
    0xFF,                 /* Machine name */
    2,                  /* SMTP Port*/
    1,                   /*Enable/Disable SMTP Authentication */
    16,                                  /* IPV6 server Addess */
    1,                      /* Enable/disable */
    4,                      /* Server IP Address */
    0xFF,                   /* User Name */
    0XFF,           /* Passwd */
    0xFF,                    /*Sender Email Address */
    0xFF,                    /* Machine name */
    2,                  /* SMTP Port*/
    1,                   /*Enable/Disable SMTP Authentication */
    16,                                  /* IPV6 server Addess */
    1,                    /* SMTP_STARTTLS_SUPPORT   */
    1                     /* SMTP_2_STARTTLS_SUPPORT */
};

/* It contain Block selector length  and if Block selector length is zero then set selector also must be zero*/
const  INT8U  g_SMTPBlockSelectorLength[] ={
    0,                      /* Enable/disable */
    0,                      /* Server IP Address */
    0,                      /* User Name */
    0,                      /* Passwd */
    0,                                  /* No of destinations */
    0,                      /* SMTP user ID*/
    MAX_SUB_BLOCKS,               /* Mail subject */
    MAX_MSG_BLOCKS,               /* Message */
    MAX_EMAIL_ADDR_BLOCKS,                  /* Sender Address */
    MAX_SRV_NAME_BLOCKS,                /* Machine Name*/
    0,                      /*SMTP Port*/
    0,                     /* Enable/disable SMTP Authentication */
    16,                     /* IPV6 Server Address */
    0,                      /* Enable/disable */
    0,                      /* Server IP Address */
    0,                      /* User Name */
    0,                      /* Passwd */
    MAX_EMAIL_ADDR_BLOCKS,                  /* Sender Address */
    MAX_SRV_NAME_BLOCKS,                /* Machine Name*/
    0,                      /*SMTP Port*/
    0,                     /* Enable/disable SMTP Authentication */
    16                     /* IPV6 Server Address */
};
int GetSMTP_PrimaryServer(SMTP_STRUCT *mail, INT8U SetSelector, INT8U EthIndex, int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    Smtp_Config_T* pm_Smtp_Config     =   &pBMCInfo->SmtpConfig[EthIndex];
    int status = 0;
    INT8S decryptedSMTPPswd[MAX_SMTP_PASSWD_LEN] = {0}, EncryptedPswd[MAX_SMTP_PASSWD_LEN] = {0};
    INT8U pwdEncKey[MAX_SIZE_KEY] = {0};

    if(!pm_Smtp_Config->EnableDisableSMTP )
    {
        return OEM_SMTP_SERVER_DISABLED;
    }
    memset(mail, 0, sizeof(SMTP_STRUCT));
    mail->AuthEnable = pm_Smtp_Config->EnableDisableSmtpAuth;
    
    if(!mail->AuthEnable)
    {
        strcpy((char *)mail->username,"Administrator");
    }
    else
    {
        strcpy((char *)mail->username,(char *)pm_Smtp_Config->UserName );
    }
   if(IsFeatureEnabled("CONFIG_SPX_FEATURE_ENABLE_USERPSWD_ENCRYPTION"))
    {
        /* Get Encryption Key from the MBMCInfo_t structure */
        LOCK_BMC_SHARED_MEM(BMCInst);
            memcpy(pwdEncKey, &(g_MBMCInfo.PwdEncKey), MAX_SIZE_KEY);
        UNLOCK_BMC_SHARED_MEM(BMCInst);
        if(g_corefeatures.encrypted_pwd_in_hexa_format == ENABLED)
        {
            //Extract Hex Array content from string
            ConvertStrtoHex((char *)pm_Smtp_Config->EncryptedHexPswd, (char *)EncryptedPswd, MAX_SMTP_PASSWD_LEN);
        }
        else
        {
            Decode64( (char *)EncryptedPswd, (char *)pm_Smtp_Config->EncryptedHexPswd, MAX_SMTP_PASSWD_LEN);
        }

        if(DecryptPassword((INT8S *)EncryptedPswd, MAX_SMTP_PASSWD_LEN-1, decryptedSMTPPswd, MAX_SMTP_PASSWD_LEN-1, pwdEncKey))
        {
            TCRIT("\n Error in Decrypting the Primary SMTP Password. \n");
            return -1;
        }
        strncpy((char *)mail->password, decryptedSMTPPswd, MAX_SMTP_PASSWD_LEN);
        mail->password[MAX_SMTP_PASSWD_LEN - 1] = '\0';
    }
    else
    {
        strcpy((char *)mail->password, (char *)pm_Smtp_Config->Passwd);
    }
    strcpy((char *)mail->from_addr,(char *)pm_Smtp_Config->SenderAddr);
    
    /* check whether IPv4 Address present */
    if( memcmp( &(pm_Smtp_Config->ServerAddr[0]), &status , IP_ADDR_LEN) !=0 )
    {
        if ( inet_ntop( AF_INET, (char*)pm_Smtp_Config->ServerAddr, (char*) mail->smtp_server, 46) == NULL )
        {
            printf("Error converting smtp server address to presentation format");
        }
    }
    else
    {
        if ( inet_ntop( AF_INET6, (char*)pm_Smtp_Config->IP6_ServerAddr, (char*) mail->smtp_server, 46) == NULL )
        {
            printf("Error converting smtp server address to presentation format");
        }
    }
    
    strcpy((char *)mail->subject,  (char *)pm_Smtp_Config->Subject[SetSelector]);
    strcpy((char *)mail->message_body, (char *)pm_Smtp_Config->Msg[SetSelector]);
    mail->UserID = pm_Smtp_Config->UserID[SetSelector];
    strcpy((char *)mail->local_host,(char *)pm_Smtp_Config->Servername);
    mail->retryinterval = pBMCInfo->LANCfs[EthIndex].DestType[SetSelector - 1].AlertAckTimeout;
    mail->smtp_retries = pBMCInfo->LANCfs[EthIndex].DestType[SetSelector - 1].Retries;
    mail->smtp_portno = pm_Smtp_Config->SmtpPort;
    mail->resptimeout = 30;
	mail->smtp_enable_STARTTLS = pm_Smtp_Config->EnableSTARTTLSSupport;
	mail->smtp_server_flag = 0;  //0 - Primary Server, 1 - Secondary Server
    return 0;
	
}
int GetSMTP_SecondaryServer(SMTP_STRUCT *mail, INT8U SetSelector, INT8U EthIndex, int BMCInst)
{
    int status = 0;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    Smtp_Config_T* pm_Smtp_Config     =   &pBMCInfo->SmtpConfig[EthIndex];
    INT8S decryptedSMTPPswd[MAX_SMTP_PASSWD_LEN] = {0}, EncryptedPswd[MAX_SMTP_PASSWD_LEN];
    INT8U pwdEncKey[MAX_SIZE_KEY] = {0};

    if(!pm_Smtp_Config->EnableDisableSMTP2 )
    {
        return OEM_SMTP_SERVER_DISABLED;
    }
    memset(mail, 0, sizeof(SMTP_STRUCT));
    mail->AuthEnable = pm_Smtp_Config->EnableDisableSmtp2Auth;
    
    if(!mail->AuthEnable)
    {
        strcpy((char *)mail->username,"Administrator");
    }
    else
    {
        strcpy((char *)mail->username,(char *)pm_Smtp_Config->UserName2 );
    }
    if(IsFeatureEnabled("CONFIG_SPX_FEATURE_ENABLE_USERPSWD_ENCRYPTION"))
    {
        /* Get Encryption Key from the MBMCInfo_t structure */
        LOCK_BMC_SHARED_MEM(BMCInst);
            memcpy(pwdEncKey, &(g_MBMCInfo.PwdEncKey), MAX_SIZE_KEY);
        UNLOCK_BMC_SHARED_MEM(BMCInst);
        if(g_corefeatures.encrypted_pwd_in_hexa_format == ENABLED)
        {
            //Extract Hex Array content from string
            ConvertStrtoHex((char *)pm_Smtp_Config->EncryptedHexPswd2, (char *)EncryptedPswd, MAX_SMTP_PASSWD_LEN);
        }
        else
        {
            Decode64((char *)EncryptedPswd, (char *)pm_Smtp_Config->EncryptedHexPswd2, MAX_SMTP_PASSWD_LEN);
        }

        if(DecryptPassword((INT8S *)EncryptedPswd, MAX_SMTP_PASSWD_LEN, decryptedSMTPPswd, MAX_SMTP_PASSWD_LEN, pwdEncKey))
        {
            TCRIT("Error in Decrypting the Secondary SMTP Password. \n");
            return -1;
        }
        strncpy((char *)mail->password, decryptedSMTPPswd, MAX_SMTP_PASSWD_LEN);
        mail->password[MAX_SMTP_PASSWD_LEN - 1] = '\0';
    }
    else
    {
        strcpy((char *)mail->password, (char *)pm_Smtp_Config->Passwd2);
    }

    strcpy((char *)mail->from_addr,(char *)pm_Smtp_Config->Sender2Addr);
    
    /* check whether IPv4 Address present */
    if( memcmp( &(pm_Smtp_Config->Server2Addr[0]), &status , IP_ADDR_LEN) !=0 )
    {
        if ( inet_ntop( AF_INET, (char*)pm_Smtp_Config->Server2Addr, (char*) mail->smtp_server, 46) == NULL )
        {
            printf("Error converting smtp server address to presentation format");
        }
    }
    else if(strlen((char *)pm_Smtp_Config->IP6_Server2Addr) != 0)
    {
        if ( inet_ntop( AF_INET6, (char*)pm_Smtp_Config->IP6_Server2Addr, (char*) mail->smtp_server, 46) == NULL )
        {
            printf("Error converting smtp server address to presentation format");
        }
    }
    
    strcpy((char *)mail->subject,  (char *)pm_Smtp_Config->Subject[SetSelector]);
    strcpy((char *)mail->message_body, (char *)pm_Smtp_Config->Msg[SetSelector]);
    mail->UserID = pm_Smtp_Config->UserID[SetSelector];
    strcpy((char *)mail->local_host,(char *)pm_Smtp_Config->Server2name);
    mail->retryinterval = pBMCInfo->LANCfs[EthIndex].DestType[SetSelector - 1].AlertAckTimeout;
    mail->smtp_retries = pBMCInfo->LANCfs[EthIndex].DestType[SetSelector - 1].Retries;
    mail->smtp_portno = pm_Smtp_Config->Smtp2Port;
    mail->resptimeout = 30;
	mail->smtp_enable_STARTTLS = pm_Smtp_Config->Smtp2EnableSTARTTLSSupport;
	mail->smtp_server_flag = 1;  //0 - Primary Server, 1 - Secondary Server
    return 0;
}

int
SetSMTPConfigParams (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst)
{

    SetSMTPConfigReq_T* pSetSMTPReq  =  ( SetSMTPConfigReq_T* ) pReq;
    SetSMTPConfigRes_T* pSetSMTPRes  =  ( SetSMTPConfigRes_T* ) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    Smtp_Config_T* pm_Smtp_Config;
    INT8S encryptedSMTPPswd[MAX_SMTP_PASSWD_LEN] = {0};
    INT8U pwdEncKey[MAX_SIZE_KEY] = {0};
    int retValue;

    if(0xff ==GetEthIndex(pSetSMTPReq->Channel, BMCInst) || ( pSetSMTPReq->SetSelector > MAX_EMAIL_DESTINATIONS ))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }
    else
    {
        pm_Smtp_Config	  =   &pBMCInfo->SmtpConfig[GetEthIndex(pSetSMTPReq->Channel, BMCInst)];
    }
    ReqLen -= 4;

    /* Check the Parameter Selector length */

    if ( 0xFF != g_SMTPConfigParameterLength [pSetSMTPReq->ParameterSelect] )
    {
        if (ReqLen != g_SMTPConfigParameterLength [pSetSMTPReq->ParameterSelect])
        {
            pSetSMTPRes->CompletionCode = CC_REQ_INV_LEN;
            return sizeof (INT8U);
        }
    }

    /* Check the Block Selector Value */

    if(pSetSMTPReq->BlockSelector != 0 )
    {
        if(g_SMTPBlockSelectorLength[pSetSMTPReq->ParameterSelect] <= pSetSMTPReq->BlockSelector )
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof (INT8U);
        }
    }else
    {
        /* If g_SMTPBlockSelectorLength[pSetSMTPReq->ParameterSelect] =0 means It doesnot need set selector also except for Paramter 5(OEM_PARAM_SMTP_USERID)*/
        /*UserId requires Set selector*/
        if((g_SMTPBlockSelectorLength[pSetSMTPReq->ParameterSelect] == 0) && (pSetSMTPReq->ParameterSelect != OEM_PARAM_SMTP_USERID) )
        {
            if(pSetSMTPReq->SetSelector  !=0)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
        }
    }
    if( pSetSMTPReq->ParameterSelect == OEM_PARAM_SMTP_USERID ||
            pSetSMTPReq->ParameterSelect == OEM_PARAM_SMTP_MSG ||
            pSetSMTPReq->ParameterSelect == OEM_PARAM_SMTP_SUBJECT)
    {
        /*UserID, Message and Subject is allowed to configure only if DestType is LAN_OEM1_ALERT(EMAIL) SMTP alerts types)*/
        if(pBMCInfo->LANCfs[GetEthIndex(pSetSMTPReq->Channel, BMCInst)].DestType [pSetSMTPReq->SetSelector - 1].DestType != LAN_OEM1_ALERT)
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof (INT8U);
        }
    }
    if (g_PDKHandle[PDK_BEFORESETSMTPCFG] != NULL )
    {
        retValue = ((int(*)(INT8U *, INT8U, INT8U *,int))(g_PDKHandle[PDK_BEFORESETSMTPCFG]))(pReq, ReqLen, pRes, BMCInst);
        if(retValue != 0)
        {
              return retValue;
        }
    }

    switch (pSetSMTPReq->ParameterSelect)
    {

        case  OEM_ENABLE_DISABLE_SMTP:
            if (0 !=  ( pSetSMTPReq-> ConfigData.EnableDisableSMTP  & ENABLE_DISABLE_MASK ) )
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            pm_Smtp_Config->EnableDisableSMTP = pSetSMTPReq->ConfigData.EnableDisableSMTP & 0x01;
            break;

        case OEM_PARAM_SMTP_SERVR_ADDR :
            if(ReqLen == IP_ADDR_LEN)       /* check for 4 bytes of server address length */
            {
                memset(&(pm_Smtp_Config->ServerAddr),0,IP_ADDR_LEN);
                memcpy ( &(pm_Smtp_Config->ServerAddr),pSetSMTPReq->ConfigData.ServerAddr ,IP_ADDR_LEN);
                 memset( &(pm_Smtp_Config->IP6_ServerAddr) , 0 , IP6_ADDR_LEN);
            }
            else
            {
                pSetSMTPRes->CompletionCode = CC_REQ_INV_LEN;
                return sizeof (INT8U);
            }
            break;

        case OEM_PARAM_SMTP_USER_NAME:
	        if (ReqLen > MAX_SMTP_USERNAME_LEN-1)
	        {
		        *pRes = CC_REQ_INV_LEN;
		         return sizeof (INT8U);
		    }
		 
            /*validate the user name*/
            if( IsValidUserName (pSetSMTPReq->ConfigData.UserName, sizeof(pSetSMTPReq->ConfigData.UserName)) ) 
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            memset ( &(pm_Smtp_Config->UserName),0,MAX_SMTP_USERNAME_LEN-1 );
            memcpy ( &(pm_Smtp_Config->UserName),pSetSMTPReq->ConfigData.UserName, ReqLen);
            break;

        case OEM_PARAM_SMTP_PASSWD :
            if (ReqLen > MAX_SMTP_PASSWD_LEN -1)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof (INT8U);
            }
            memset ( &(pm_Smtp_Config->Passwd),0, MAX_SMTP_PASSWD_LEN-1 );
            if(IsFeatureEnabled("CONFIG_SPX_FEATURE_ENABLE_USERPSWD_ENCRYPTION"))
            {
                /* Get Encryption Key from the MBMCInfo_t structure */
                LOCK_BMC_SHARED_MEM(BMCInst);
                    memcpy(pwdEncKey, &(g_MBMCInfo.PwdEncKey), MAX_SIZE_KEY);
                UNLOCK_BMC_SHARED_MEM(BMCInst);

                if(EncryptPassword((INT8S *)pSetSMTPReq->ConfigData.Passwd, MAX_SMTP_PASSWD_LEN-1, encryptedSMTPPswd, MAX_SMTP_PASSWD_LEN-1, pwdEncKey))
                {
                    TCRIT("Error in encrypting the Primary SMTP password.\n");
                    pSetSMTPRes->CompletionCode  = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                memcpy(&(pm_Smtp_Config->Passwd), encryptedSMTPPswd, MAX_SMTP_PASSWD_LEN-1);
                if(g_corefeatures.encrypted_pwd_in_hexa_format == ENABLED)
                {
                    /*Convert Hex Array Content to string*/
                    ConvertHextoStr((char *)&pm_Smtp_Config->Passwd[0], (char *)&pm_Smtp_Config->EncryptedHexPswd[0] , MAX_SMTP_PASSWD_LEN);
                }
                else
                {
                    Encode64nChar((char *)&pm_Smtp_Config->EncryptedHexPswd[0], (char *)&pm_Smtp_Config->Passwd[0], 2*MAX_SMTP_PASSWD_LEN, MAX_SMTP_PASSWD_LEN);
                }
            }
	    else
	    {
	        memcpy(&(pm_Smtp_Config->Passwd), pSetSMTPReq->ConfigData.Passwd, ReqLen);
            }
            break;

        case OEM_PARAM_SMTP_NO_OF_DESTINATIONS:
            pSetSMTPRes->CompletionCode  = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (INT8U);
            break;

        case OEM_PARAM_SMTP_USERID :
        	pm_Smtp_Config->UserID[pSetSMTPReq->SetSelector]=pSetSMTPReq->ConfigData.UserID;
            break;

        case OEM_PARAM_SMTP_MSG:
	    if (ReqLen > MAX_MSG_BLOCK_SIZE)
	    {
		*pRes = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	    }
            memset( &(pm_Smtp_Config->Msg[pSetSMTPReq->SetSelector][pSetSMTPReq->BlockSelector*MAX_MSG_BLOCK_SIZE]),0,MAX_MSG_BLOCK_SIZE);
            memcpy ( &(pm_Smtp_Config->Msg[pSetSMTPReq->SetSelector][pSetSMTPReq->BlockSelector*MAX_MSG_BLOCK_SIZE]),&(pSetSMTPReq->ConfigData.Msg),ReqLen  );
            break;

        case OEM_PARAM_SMTP_SUBJECT:
	    if (ReqLen > MAX_MSG_BLOCK_SIZE)
	    {
		*pRes = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	    }
            memset(&(pm_Smtp_Config->Subject[pSetSMTPReq->SetSelector][pSetSMTPReq->BlockSelector*MAX_SUB_BLOCK_SIZE]),0,MAX_SUB_BLOCK_SIZE);
            memcpy (&(pm_Smtp_Config->Subject[pSetSMTPReq->SetSelector][pSetSMTPReq->BlockSelector*MAX_SUB_BLOCK_SIZE]),&(pSetSMTPReq->ConfigData.Subject ),ReqLen );
            break;

        case OEM_PARAM_SMTP_SENDER_ADDR:
	    if (ReqLen > MAX_EMAIL_BLOCK_SIZE)
	    {
		*pRes = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	    }
            memset(&(pm_Smtp_Config->SenderAddr[pSetSMTPReq->BlockSelector*MAX_EMAIL_BLOCK_SIZE]),0 ,MAX_EMAIL_BLOCK_SIZE);
            memcpy ( &(pm_Smtp_Config->SenderAddr[pSetSMTPReq->BlockSelector*MAX_EMAIL_BLOCK_SIZE]),pSetSMTPReq->ConfigData.SenderAddr ,ReqLen);
            break;
        
        case  OEM_PARAM_SMTP_HOST_NAME:
	    if (ReqLen > MAX_SRV_NAME_BLOCK_SIZE)
	    {
		*pRes = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	    }
            memset ( &(pm_Smtp_Config->Servername[pSetSMTPReq->BlockSelector*MAX_SRV_NAME_BLOCK_SIZE]),0 ,MAX_SRV_NAME_BLOCK_SIZE);
            memcpy ( &(pm_Smtp_Config->Servername[pSetSMTPReq->BlockSelector*MAX_SRV_NAME_BLOCK_SIZE]),pSetSMTPReq->ConfigData.Servername ,ReqLen);
            break;

        case OEM_PARAM_SMTP_PORT :
            pm_Smtp_Config->SmtpPort= htoipmi_u16(pSetSMTPReq->ConfigData.SmtpPort);
            break;

        case  OEM_PARAM_ENABLE_SMTP_STARTTLS_SUPPORT:
            if (0 !=  ( pSetSMTPReq-> ConfigData.EnableSTARTTLSSupport  & ENABLE_DISABLE_MASK ) )
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            pm_Smtp_Config->EnableSTARTTLSSupport = pSetSMTPReq->ConfigData.EnableSTARTTLSSupport & 0x01;
            break;

        case OEM_ENABLE_DISABLE_SMTP_AUTH :
            pm_Smtp_Config->EnableDisableSmtpAuth = pSetSMTPReq->ConfigData.EnableDisableSmtpAuth;
            break;

        case OEM_PARAM_SMTP_IPV6_SERVR_ADDR :
            if( ReqLen  == IP6_ADDR_LEN )       /* check for total 16 bytes of IPV6 address length */
            {
				/*Validate the IPv6 address*/
	        	if(IsValidGlobalIPv6Addr((struct in6_addr*)&pSetSMTPReq->ConfigData.IP6_ServerAddr))
	         	{
	            	TCRIT("Invalid Global IPv6 Address\n");
	              	*pRes = CC_INV_DATA_FIELD;
	              	return sizeof(INT8U);
	         	}

                memcpy ( &(pm_Smtp_Config->IP6_ServerAddr),pSetSMTPReq->ConfigData.IP6_ServerAddr ,IP6_ADDR_LEN);
                memset( &(pm_Smtp_Config->ServerAddr) , 0 , IP_ADDR_LEN);
            }
            else            /* Invalid Request in length */
            {
                pSetSMTPRes->CompletionCode = CC_REQ_INV_LEN;
                return sizeof (INT8U);
            }
            break;

        case OEM_ENABLE_DISABLE_SMTP2 :
             if (0 !=  ( pSetSMTPReq-> ConfigData.EnableDisableSMTP2  & ENABLE_DISABLE_MASK ) )
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            pm_Smtp_Config->EnableDisableSMTP2 = pSetSMTPReq->ConfigData.EnableDisableSMTP2 & 0x01;
            break;

        case OEM_PARAM_SMTP_2_SERVR_ADDR :
            if(ReqLen == IP_ADDR_LEN)       /* check for 4 bytes of server address length */
            {
                memset(&(pm_Smtp_Config->Server2Addr),0,IP_ADDR_LEN);
                memcpy (&(pm_Smtp_Config->Server2Addr),pSetSMTPReq->ConfigData.Server2Addr ,IP_ADDR_LEN);
                 memset( &(pm_Smtp_Config->IP6_Server2Addr) , 0 , IP6_ADDR_LEN);
            }
            else
            {
                pSetSMTPRes->CompletionCode = CC_REQ_INV_LEN;
                return sizeof (INT8U);
            }
            break;

        case OEM_PARAM_SMTP_2_USER_NAME :
	    if (ReqLen > MAX_SMTP_USERNAME_LEN-1)
	    {
		*pRes = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	    }
            memset ( &(pm_Smtp_Config->UserName2),0,MAX_SMTP_USERNAME_LEN-1 );
            memcpy (&(pm_Smtp_Config->UserName2),pSetSMTPReq->ConfigData.UserName2, ReqLen);
            break;

        case OEM_PARAM_SMTP_2_PASSWD :
            if (ReqLen > MAX_SMTP_PASSWD_LEN -1)
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof (INT8U);
            }
            memset ( &(pm_Smtp_Config->Passwd2),0, MAX_SMTP_PASSWD_LEN-1 );
            if(IsFeatureEnabled("CONFIG_SPX_FEATURE_ENABLE_USERPSWD_ENCRYPTION"))
            {
                /* Get Encryption Key from the MBMCInfo_t structure */
                LOCK_BMC_SHARED_MEM(BMCInst);
                    memcpy(pwdEncKey, &(g_MBMCInfo.PwdEncKey), MAX_SIZE_KEY);
                UNLOCK_BMC_SHARED_MEM(BMCInst);

                if(EncryptPassword((INT8S *)pSetSMTPReq->ConfigData.Passwd2, MAX_SMTP_PASSWD_LEN-1, encryptedSMTPPswd, MAX_SMTP_PASSWD_LEN-1, pwdEncKey))
                {
                    TCRIT("Error in encrypting the Secondary SMTP password.\n");
                    pSetSMTPRes->CompletionCode  = CC_UNSPECIFIED_ERR;
                    return sizeof(INT8U);
                }
                memcpy(&(pm_Smtp_Config->Passwd2), encryptedSMTPPswd, MAX_SMTP_PASSWD_LEN-1);
                if(g_corefeatures.encrypted_pwd_in_hexa_format == ENABLED)
                {
                    /*Convert Hex Array Content to string*/
                    ConvertHextoStr((char *)&pm_Smtp_Config->Passwd2[0], (char *)&pm_Smtp_Config->EncryptedHexPswd2[0] , MAX_SMTP_PASSWD_LEN);
                }
                else
                {
                    Encode64nChar( (char *)&pm_Smtp_Config->EncryptedHexPswd2[0] , (char *)&pm_Smtp_Config->Passwd2[0], 2*MAX_SMTP_PASSWD_LEN, MAX_SMTP_PASSWD_LEN);
                }
            }
            else
            {
                memcpy ( &(pm_Smtp_Config->Passwd2),pSetSMTPReq->ConfigData.Passwd2, ReqLen );
            }
            break;

        case OEM_PARAM_SMTP_2_SENDER_ADDR :
	    if (ReqLen > MAX_EMAIL_BLOCK_SIZE)
	    {
		*pRes = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	    }
            memset(&(pm_Smtp_Config->Sender2Addr[pSetSMTPReq->BlockSelector*MAX_EMAIL_BLOCK_SIZE]),0 ,MAX_EMAIL_BLOCK_SIZE);
            memcpy (&(pm_Smtp_Config->Sender2Addr[pSetSMTPReq->BlockSelector*MAX_EMAIL_BLOCK_SIZE]),pSetSMTPReq->ConfigData.Sender2Addr ,ReqLen);
            break;

        case OEM_PARAM_SMTP_2_HOST_NAME :
	    if (ReqLen > MAX_SRV_NAME_BLOCK_SIZE)
	    {
		*pRes = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	    }
            memset (&(pm_Smtp_Config->Server2name[pSetSMTPReq->BlockSelector*MAX_SRV_NAME_BLOCK_SIZE]),0 ,MAX_SRV_NAME_BLOCK_SIZE);
            memcpy (&(pm_Smtp_Config->Server2name[pSetSMTPReq->BlockSelector*MAX_SRV_NAME_BLOCK_SIZE]),pSetSMTPReq->ConfigData.Server2name ,ReqLen);
            break;

        case OEM_PARAM_SMTP_2_PORT :
            pm_Smtp_Config->Smtp2Port= htoipmi_u16(pSetSMTPReq->ConfigData.Smtp2Port);
            break;

        case OEM_PARAM_ENABLE_SMTP_2_STARTTLS_SUPPORT :
             if (0 !=  ( pSetSMTPReq-> ConfigData.Smtp2EnableSTARTTLSSupport  & ENABLE_DISABLE_MASK ) )
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
            pm_Smtp_Config->Smtp2EnableSTARTTLSSupport = pSetSMTPReq->ConfigData.Smtp2EnableSTARTTLSSupport & 0x01;
            break;

        case OEM_ENABLE_DISABLE_SMTP_2_AUTH :
            pm_Smtp_Config->EnableDisableSmtp2Auth = pSetSMTPReq->ConfigData.EnableDisableSmtp2Auth;
            break;

        case OEM_PARAM_SMTP_2_IPV6_SERVR_ADDR :
            if( ReqLen  == IP6_ADDR_LEN )       /* check for total 16 bytes of IPV6 address length */
            {
				/*Validate the IPv6 address*/
	        	if(IsValidGlobalIPv6Addr((struct in6_addr*)&pSetSMTPReq->ConfigData.IP6_Server2Addr))
	         	{
	            	TCRIT("Invalid Global IPv6 Address\n");
	              	*pRes = CC_INV_DATA_FIELD;
	              	return sizeof(INT8U);
	         	}

                memcpy ( &(pm_Smtp_Config->IP6_Server2Addr),pSetSMTPReq->ConfigData.IP6_Server2Addr ,IP6_ADDR_LEN);
                memset( &(pm_Smtp_Config->Server2Addr) , 0 , IP_ADDR_LEN);
            }
            else            /* Invalid Request in length */
            {
                pSetSMTPRes->CompletionCode = CC_REQ_INV_LEN;
                return sizeof (INT8U);
            }
            break;
        default:
            pSetSMTPRes->CompletionCode  = CC_INV_DATA_FIELD;
            return sizeof (INT8U);

}
    pSetSMTPRes->CompletionCode  = CC_NORMAL ;
    FlushIPMI((INT8U*)&pBMCInfo->SmtpConfig[0],(INT8U*)&pBMCInfo->SmtpConfig[GetEthIndex(pSetSMTPReq->Channel, BMCInst)],
                      pBMCInfo->IPMIConfLoc.SmtpConfigAddr,sizeof(Smtp_Config_T),BMCInst);
    return sizeof (INT8U);

}



int
GetSMTPConfigParams (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst)
{
    INT8U ResLen = 0;
    GetSMTPConfigReq_T* pGetSMTPReq  =  ( GetSMTPConfigReq_T* ) pReq;
    GetSMTPConfigRes_T* pGetSMTPRes  =  ( GetSMTPConfigRes_T* ) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    Smtp_Config_T* pm_Smtp_Config;
    INT16U SessionType;
    INT8S decryptedSMTPPswd[MAX_SMTP_PASSWD_LEN] = {0};
    INT8U pwdEncKey[MAX_SIZE_KEY] = {0};

    if(0xff ==GetEthIndex(pGetSMTPReq->Channel, BMCInst) || (pGetSMTPReq->SetSelector > MAX_EMAIL_DESTINATIONS))
    {
        pGetSMTPRes->CompletionCode  = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }else
    {
        pm_Smtp_Config	= &pBMCInfo->SmtpConfig[GetEthIndex(pGetSMTPReq->Channel, BMCInst)];
    }

    /* Check the Block Selector Value */

    if(pGetSMTPReq->BlockSelector != 0 )
    {
        if(g_SMTPBlockSelectorLength[pGetSMTPReq->ParameterSelect] <= pGetSMTPReq->BlockSelector )
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof (INT8U);
        }
    }else
    {
       /* If g_SMTPBlockSelectorLength[pSetSMTPReq->ParameterSelect] =0 means It doesnot need set selector also except for Paramter 5(OEM_PARAM_SMTP_USERID)*/
        /*UserId requires Set selector*/
        if((g_SMTPBlockSelectorLength[pGetSMTPReq->ParameterSelect] == 0) && (pGetSMTPReq->ParameterSelect != OEM_PARAM_SMTP_USERID) )        
        {
            if(pGetSMTPReq->SetSelector  !=0)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }
        }
    }

    switch (pGetSMTPReq->ParameterSelect)
    {

        case  OEM_ENABLE_DISABLE_SMTP:
            pGetSMTPRes->ConfigData.EnableDisableSMTP  = pm_Smtp_Config->EnableDisableSMTP;
            ResLen = 1;
            break;


        case OEM_PARAM_SMTP_SERVR_ADDR :
            memcpy ( pGetSMTPRes->ConfigData.ServerAddr ,&(pm_Smtp_Config->ServerAddr),IP_ADDR_LEN);
            ResLen = IP_ADDR_LEN;
            break;

        case OEM_PARAM_SMTP_USER_NAME:
            memcpy ( pGetSMTPRes->ConfigData.UserName , &(pm_Smtp_Config->UserName),MAX_SMTP_USERNAME_LEN-1);
            ResLen = MAX_SMTP_USERNAME_LEN-1;
            break;

        case OEM_PARAM_SMTP_PASSWD :
            /* It Write only Parameter Only Allowed for UDS Session*/
            OS_THREAD_TLS_GET(g_tls.CurSessionType,SessionType);
            if(UDS_SESSION_TYPE == SessionType)
            {
                if(g_corefeatures.userpswd_encryption == ENABLED)
                {
                     /* Get Encryption Key from the MBMCInfo_t structure */
                    LOCK_BMC_SHARED_MEM(BMCInst);
                       memcpy(pwdEncKey, &(g_MBMCInfo.PwdEncKey), MAX_SIZE_KEY);
                    UNLOCK_BMC_SHARED_MEM(BMCInst);

                    if(DecryptPassword((INT8S *)pm_Smtp_Config->Passwd, MAX_SMTP_PASSWD_LEN-1, decryptedSMTPPswd, MAX_SMTP_PASSWD_LEN-1, pwdEncKey))
                    {
                        TCRIT("\n Error in Decrypting the Primary SMTP Password. \n");
                        *pRes  = CC_UNSPECIFIED_ERR;
                        return sizeof(INT8U);
                    }
                    strncpy((char *)pGetSMTPRes->ConfigData.Passwd, decryptedSMTPPswd, MAX_SMTP_PASSWD_LEN);
                    pGetSMTPRes->ConfigData.Passwd[MAX_SMTP_PASSWD_LEN - 1] = '\0';
                 }
                 else
                 {
                    strcpy((char *)pGetSMTPRes->ConfigData.Passwd, (char *)pm_Smtp_Config->Passwd);
                 }
                 ResLen = MAX_SMTP_PASSWD_LEN-1;
                 break;
            }
            else
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }

        case OEM_PARAM_SMTP_NO_OF_DESTINATIONS:
            pGetSMTPRes->ConfigData.NoofDestinations  = MAX_EMAIL_DESTINATIONS;
            ResLen = sizeof (INT8U);
            break;

        case OEM_PARAM_SMTP_USERID:
      	    pGetSMTPRes->ConfigData.UserID=pm_Smtp_Config->UserID[pGetSMTPReq->SetSelector];
            ResLen = sizeof (INT8U);
            break;

        case OEM_PARAM_SMTP_MSG:
            memcpy(&(pGetSMTPRes->ConfigData.Msg) ,&(pm_Smtp_Config->Msg[pGetSMTPReq->SetSelector][pGetSMTPReq->BlockSelector*MAX_MSG_BLOCK_SIZE]),MAX_MSG_BLOCK_SIZE  );
            ResLen = MAX_MSG_BLOCK_SIZE;
            break;

        case OEM_PARAM_SMTP_SUBJECT:
            memcpy ( &(pGetSMTPRes->ConfigData.Subject), &( pm_Smtp_Config->Subject[pGetSMTPReq->SetSelector][pGetSMTPReq->BlockSelector * MAX_SUB_BLOCK_SIZE]),MAX_SUB_BLOCK_SIZE);
            ResLen = MAX_SUB_BLOCK_SIZE;
            break;

        case OEM_PARAM_SMTP_SENDER_ADDR:
            memcpy ( &(pGetSMTPRes->ConfigData.SenderAddr), &( pm_Smtp_Config->SenderAddr[pGetSMTPReq->BlockSelector *MAX_EMAIL_BLOCK_SIZE]),MAX_EMAIL_BLOCK_SIZE);
            ResLen = MAX_EMAIL_BLOCK_SIZE  ;
            break;

        case  OEM_PARAM_SMTP_HOST_NAME:
            memcpy ( &(pGetSMTPRes->ConfigData.Servername), &( pm_Smtp_Config->Servername[pGetSMTPReq->BlockSelector *MAX_SRV_NAME_BLOCK_SIZE]),MAX_SRV_NAME_BLOCK_SIZE);
            ResLen = MAX_SRV_NAME_BLOCK_SIZE;
            break;

        case  OEM_PARAM_SMTP_PORT:
            pGetSMTPRes->ConfigData.SmtpPort  = pm_Smtp_Config->SmtpPort;
            ResLen = sizeof(INT16U);
            break;

        case  OEM_PARAM_ENABLE_SMTP_STARTTLS_SUPPORT:
            pGetSMTPRes->ConfigData.EnableSTARTTLSSupport  = pm_Smtp_Config->EnableSTARTTLSSupport;
            ResLen = 1;
            break;

        case  OEM_ENABLE_DISABLE_SMTP_AUTH:
            pGetSMTPRes->ConfigData.EnableDisableSmtpAuth  = pm_Smtp_Config->EnableDisableSmtpAuth;
            ResLen = 1;
            break;

        case OEM_PARAM_SMTP_IPV6_SERVR_ADDR :
            memcpy ( pGetSMTPRes->ConfigData.IP6_ServerAddr , &(pm_Smtp_Config->IP6_ServerAddr), IP6_ADDR_LEN);
            ResLen =IP6_ADDR_LEN;
            break;

        case OEM_ENABLE_DISABLE_SMTP2 :
            pGetSMTPRes->ConfigData.EnableDisableSMTP2  = pm_Smtp_Config->EnableDisableSMTP2;
            ResLen = 1;
            break;

        case OEM_PARAM_SMTP_2_SERVR_ADDR :
            memcpy ( pGetSMTPRes->ConfigData.Server2Addr ,&(pm_Smtp_Config->Server2Addr),IP_ADDR_LEN);
            ResLen = IP_ADDR_LEN;
            break;

        case OEM_PARAM_SMTP_2_USER_NAME :
            memcpy ( pGetSMTPRes->ConfigData.UserName2 , &(pm_Smtp_Config->UserName2),MAX_SMTP_USERNAME_LEN-1 );
            ResLen = MAX_SMTP_USERNAME_LEN-1;
            break;

        case OEM_PARAM_SMTP_2_PASSWD :
            /* It Write only Parameter Only Allowed for UDS Session*/
            OS_THREAD_TLS_GET(g_tls.CurSessionType,SessionType);
            if(UDS_SESSION_TYPE == SessionType)
            {
                if(g_corefeatures.userpswd_encryption == ENABLED)
                {
                     /* Get Encryption Key from the MBMCInfo_t structure */
                    LOCK_BMC_SHARED_MEM(BMCInst);
                       memcpy(pwdEncKey, &(g_MBMCInfo.PwdEncKey), MAX_SIZE_KEY);
                    UNLOCK_BMC_SHARED_MEM(BMCInst);

                    if(DecryptPassword((INT8S *)pm_Smtp_Config->Passwd2, MAX_SMTP_PASSWD_LEN-1, decryptedSMTPPswd, MAX_SMTP_PASSWD_LEN-1, pwdEncKey))
                    {
                        TCRIT("\n Error in Decrypting the Primary SMTP Password. \n");
                        *pRes  = CC_UNSPECIFIED_ERR;
                        return sizeof(INT8U);
                    }
                    strncpy((char *)pGetSMTPRes->ConfigData.Passwd2, decryptedSMTPPswd, MAX_SMTP_PASSWD_LEN);
                    pGetSMTPRes->ConfigData.Passwd2[MAX_SMTP_PASSWD_LEN - 1] = '\0';
                 }
                 else
                 {
                    strcpy((char *)pGetSMTPRes->ConfigData.Passwd2, (char *)pm_Smtp_Config->Passwd2);
                 }
                 ResLen = MAX_SMTP_PASSWD_LEN-1;
                 break;
            }
            else
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (INT8U);
            }

        case OEM_PARAM_SMTP_2_SENDER_ADDR :
            memcpy ( &(pGetSMTPRes->ConfigData.Sender2Addr), &(pm_Smtp_Config->Sender2Addr[pGetSMTPReq->BlockSelector *MAX_EMAIL_BLOCK_SIZE]),MAX_EMAIL_BLOCK_SIZE);
            ResLen = MAX_EMAIL_BLOCK_SIZE  ;
            break;

        case OEM_PARAM_SMTP_2_HOST_NAME :
            memcpy ( &(pGetSMTPRes->ConfigData.Server2name), &(pm_Smtp_Config->Server2name[pGetSMTPReq->BlockSelector *MAX_SRV_NAME_BLOCK_SIZE]),MAX_SRV_NAME_BLOCK_SIZE);
            ResLen = MAX_SRV_NAME_BLOCK_SIZE;
            break;

        case OEM_PARAM_SMTP_2_PORT :
            pGetSMTPRes->ConfigData.Smtp2Port  = pm_Smtp_Config->Smtp2Port;
            ResLen = sizeof(INT16U);
            break;

        case OEM_PARAM_ENABLE_SMTP_2_STARTTLS_SUPPORT :
            pGetSMTPRes->ConfigData.Smtp2EnableSTARTTLSSupport  = pm_Smtp_Config->Smtp2EnableSTARTTLSSupport;
            ResLen = 1;
            break;

        case OEM_ENABLE_DISABLE_SMTP_2_AUTH :
            pGetSMTPRes->ConfigData.EnableDisableSmtp2Auth  = pm_Smtp_Config->EnableDisableSmtp2Auth;
            ResLen = 1;
            break;

        case OEM_PARAM_SMTP_2_IPV6_SERVR_ADDR :
            memcpy ( pGetSMTPRes->ConfigData.IP6_Server2Addr , &(pm_Smtp_Config->IP6_Server2Addr), IP6_ADDR_LEN);
            ResLen =IP6_ADDR_LEN;
            break;
        default:
            pGetSMTPRes->CompletionCode  = CC_INV_DATA_FIELD;
            return sizeof (INT8U);
    }

    pGetSMTPRes->CompletionCode  = 0 ;
    ResLen++;	// byte for Completion Code
    return ResLen;

}

