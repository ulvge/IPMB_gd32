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
 ******************************************************************
 *
 * AMIResetPass.c
 * AMI REset password related implementation.
 *
 * Author: Gokula Kannan. S <gokulakannans@amiindia.co.in>
 ******************************************************************/

#include "Debug.h"
#include "Support.h"
#include "SharedMem.h"
#include "PMConfig.h"
#include "NVRData.h"
#include "NVRAccess.h"
#include "IPMIDefs.h"
#include "IPMI_AMIResetPass.h"
#include "AMIResetPass.h"
#include "AppDevice.h"
#include "IPMI_AppDevice.h"
#include "IPMI_AMIDevice.h"
#include "ipmi_userifc.h"
#include "smtpclient.h"
#include "nwcfg.h"
#include "Session.h"
#include "IPMI_AMISmtp.h"
#include "AMISmtp.h"
#include "Ethaddr.h"
#include "IPMIConf.h"
#include "userprivilege.h"
#include "featuredef.h"
#include "blowfish.h"
#include "Encode.h"

static int validate_email(const char *emailaddr)
{
    int        valid = 0;
    const char *pTemp, *domain;
    static char *special_char = "()<>@,;:\\\"[]";

    /* first we validate the name portion (name@domain) */
    for (pTemp= emailaddr;  *pTemp;  pTemp++) 
    {
        if (*pTemp == '\"' && (pTemp == emailaddr || *(pTemp - 1) == '.' || *(pTemp - 1) ==
        '\"')) {
        while (*++pTemp) {
        if (*pTemp == '\"') break;
        else if (*pTemp == '\\' && (*++pTemp == ' ')) continue;
        else if (*pTemp < ' ' || *pTemp >= 127) return 0;
        }
        if (!*pTemp++) return 0;
        else if (*pTemp == '@') break;
        else if (*pTemp != '.') return 0;
        continue;
        }
        else if (*pTemp == '@') break;
        else if (*pTemp <= ' ' || *pTemp >= 127) return 0;
        else if (strchr(special_char, *pTemp)) return 0;
    }

    if (pTemp == emailaddr || *(pTemp - 1) == '.') return 0;

    /* next we validate the domain portion (name@domain) */
    if (!*(domain = ++pTemp)) return 0;
    while (*pTemp)
    {
        if (*pTemp == '.') 
        {
            if (pTemp == domain || *(pTemp - 1) == '.') return 0;
            else if(!*(pTemp +1))return 0;
        }
        else if (*pTemp <= ' ' || *pTemp >= 127) return 0;
        else if (strchr(special_char, *pTemp)) return 0;
        pTemp++;
    };
    if(!*pTemp)
        valid++;
    return (valid >= 1);
}

int AMISetEmailForUser(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{

    AMISetUserEmailReq_T *pSetUserEmail = (AMISetUserEmailReq_T *)pReq;
     UserInfo_T* pUserInfo;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    TDBG("Inside AMISetEmailForUser %d \n", pSetUserEmail->UserID);

    if ( (0 != pSetUserEmail->EMailID[0]) && (0 == validate_email(pSetUserEmail->EMailID)) )
    {
        IPMI_WARNING ("Invalid Email address \n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pUserInfo = getUserIdInfo (pSetUserEmail->UserID, BMCInst);
    if ( (NULL ==  pUserInfo)  || (pUserInfo->ID != USER_ID) )
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        IPMI_WARNING ("Invalid User Id \n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

     _fmemcpy (pUserInfo->UserEMailID, pSetUserEmail->EMailID, EMAIL_ADDR_SIZE);

    FlushIPMI((INT8U*)&pBMCInfo->UserInfo[0],(INT8U*)&pBMCInfo->UserInfo[0],pBMCInfo->IPMIConfLoc.UserInfoAddr,
                      sizeof(UserInfo_T)*MAX_USER_CFG_MDS,BMCInst);
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    *pRes = CC_NORMAL;
    return sizeof(*pRes);

}

int AMIGetEmailForUser(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
    AMIGetUserEmailRes_T*  pAMIGetUserEmailRes = (AMIGetUserEmailRes_T *) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int UserID = *(INT8U *)pReq;
    UserInfo_T* pUserInfo;
    TDBG("Inside AMIGetEmailForUser %d \n", UserID);

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pUserInfo = getUserIdInfo (UserID, BMCInst);
    if ( (NULL ==  pUserInfo)  || (pUserInfo->ID != USER_ID))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        IPMI_WARNING ("Invalid User Id \n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }
    memset(pAMIGetUserEmailRes,0, sizeof(AMIGetUserEmailRes_T));
    pAMIGetUserEmailRes->CompletionCode = CC_NORMAL;
    if( pUserInfo->UserEMailID[0] != 0 )
    {
        _fmemcpy (pAMIGetUserEmailRes->EMailID, pUserInfo->UserEMailID, EMAIL_ADDR_SIZE);
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
    return sizeof(AMIGetUserEmailRes_T);
}

int AMIResetPassword(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
    AMIResetPasswordReq_T *pCmdReq  = (AMIResetPasswordReq_T *)pReq;
    char eMailAdd[EMAIL_ADDR_SIZE];
    char userName[MAX_USERNAME_LEN];
    SetUserPswdReq_T ReqSetUserPswd;
    SetUserPswdRes_T ResSetUserPswd;
    char ClearPswd[MAX_PASSWORD_LEN];
    UserInfo_T* pUserInfo = NULL;
    char OldPswd[MAX_PASSWORD_LEN];
    int ret;
    int smtpServerFlag=0;// 0 - primary 1- secondary
    SMTP_STRUCT mail;
    int i, retVal = 0, ethIndex = 0;
    char letters_numbers[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char PassLength = 0;
    INT8S DecryptedUserPswd[MAX_PASSWORD_LEN] = {0};
    char  EncryptedPswd[MAX_ENCRYPTED_PSWD_LEN + 1] = {0};
    INT8U PwdEncKey[MAX_SIZE_KEY] = {0};
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    /* Get the username and E-Mail address */
    pUserInfo = getUserIdInfo (pCmdReq->UserID, BMCInst);
    if ( (NULL ==  pUserInfo)  || (pUserInfo->ID != USER_ID))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        IPMI_WARNING ("Invalid User Id \n");
        *(pRes) = CC_INV_DATA_FIELD;
        goto end;
    }

    if( pUserInfo->UserEMailID[0] == 0 )
    {
        IPMI_ERROR("Email not configured for the user\n");
        *(pRes) = OEMCC_EMAIL_NOT_CONFIGURED;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        goto end;
    }

    _fmemcpy (eMailAdd, pUserInfo->UserEMailID, EMAIL_ADDR_SIZE);
    _fmemcpy (userName, pUserInfo->UserName, MAX_USERNAME_LEN);

    if (g_corefeatures.userpswd_encryption == ENABLED)
    {
        /* Get Encryption Key from the MBMCInfo_t structure */
            memcpy(PwdEncKey, &(g_MBMCInfo.PwdEncKey), MAX_SIZE_KEY);
            if(g_corefeatures.encrypted_pwd_in_hexa_format == ENABLED)
            {
                //Extract Hex Array content from string
                ConvertStrtoHex((char *)pBMCInfo->EncryptedUserInfo[pCmdReq->UserID - 1].EncryptedHexPswd, (char *)EncryptedPswd, MAX_ENCRYPTED_PSWD_LEN);
            }
            else
            {
                Decode64( (char *)EncryptedPswd, (char *)pBMCInfo->EncryptedUserInfo[pCmdReq->UserID - 1].EncryptedHexPswd, MAX_ENCRYPTED_PSWD_LEN);
            }
        if(DecryptPassword((INT8S *)EncryptedPswd, MAX_PASSWORD_LEN, DecryptedUserPswd, MAX_PASSWORD_LEN, PwdEncKey))
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            TCRIT("Error in decrypting the IPMI User password for user with ID:%d.\n", pCmdReq->UserID);
            *pRes = CC_UNSPECIFIED_ERR;
            goto end;
        }
        memcpy(OldPswd, DecryptedUserPswd, MAX_PASSWORD_LEN);
    }
    else
    {
        _fmemcpy (OldPswd, pUserInfo->UserPassword, MAX_PASSWORD_LEN);
    }

    ethIndex = GetEthIndex(pCmdReq->Channel, BMCInst);
    if (ethIndex == 0xff)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        *(pRes) = CC_INV_DATA_FIELD;
        goto end;
    }
    if((ret=GetSMTP_PrimaryServer(&mail,0,ethIndex,BMCInst))<0)
    {
        smtpServerFlag=1;
        if(GetSMTP_SecondaryServer(&mail,0,ethIndex,BMCInst)<0)
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            *(pRes) = OEMCC_SMTP_DISABLED;
            goto end;
        }
    }
    /* Generate password */
    TDBG("\n The previous user length %d \n",pUserInfo->MaxPasswordSize);
    if (pUserInfo->MaxPasswordSize == MAX_PASSWORD_LEN)
    {
        ReqSetUserPswd.UserID = pCmdReq->UserID | 0x80 ;
        PassLength = IPMI_20_PASSWORD_LEN;
    }
    else
    {
    	ReqSetUserPswd.UserID = pCmdReq->UserID;
    	PassLength = IPMI_15_PASSWORD_LEN;
    } 
    ReqSetUserPswd.Operation = 0x02;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    memset(&ClearPswd, 0, MAX_PASSWORD_LEN);

    srand(time(0));
    i = (rand() % (MAX_PASSWORD_LEN-13)) + 8;
    TINFO("Random Number Length for Password : %d\n", i);
    for (i--; i >= 0; i--)
    {
        ClearPswd[i] = letters_numbers[(rand() % (sizeof(letters_numbers ) - 1))];
    }

    strncpy((char*)ReqSetUserPswd.Password, (char*)ClearPswd, MAX_PASSWORD_LEN);
     retVal = smtp_test(&mail);
	if(smtpServerFlag == 0)
	{
		if (retVal == ERR_UNABLE_TO_CONNECT_SMTPSERVER)
		{
			if(GetSMTP_SecondaryServer(&mail,0,ethIndex,BMCInst)<0)
			{
				*(pRes) = OEMCC_UNABLE_TO_CONNECT_SMTPSERVER;
			 	 goto end;
			}
			retVal = smtp_test(&mail);
		}
	}
     if ((retVal) != EMAIL_UNKNOWN)
    {
    	  switch (retVal)
    	  {
    	      case EMAIL_NO_SOCKET:
    	      case EMAIL_NO_CONNECT:
    	      case EMAIL_NO_HELO_RESPONSE:
    	          *(pRes) = OEMCC_UNABLE_TO_CONNECT_SMTPSERVER;
    	          break;
    	      case EMAIL_AUTH_TYPE_UNSUPPORTED:
    	          *(pRes) = OEMCC_UNSUPPORTED_AUTH_TYPE;
    	          break;
    	      case EMAIL_AUTH_REQUIRED:
    	      case EMAIL_AUTH_FAILED:
    	      case EMAIL_AUTH_NOT_SUPPORTED:
    	          *(pRes) = OEMCC_SEND_EMAIL_AUTH_FAILED;
    	          break;
            default:
    	          *(pRes) = OEMCC_SEND_EMAIL_FAILED;
    	  }
        IPMI_ERROR("Test mail Server failed (%s) : %d\n", mail.smtp_server, retVal);
        goto end;
    }
    strcpy((char *)mail.to_addr, eMailAdd);
    sprintf((char *)mail.subject, "The %s Login Password has been changed",mail.local_host);
    sprintf((char *)mail.message_body, "The password has been reset for %s (host)\nUserName : %s\nPassword : %s\n",
                mail.local_host, userName, ClearPswd);


    TDBG("UserName : %s, Password : %s, To : %s, From : %s\n", mail.username,
            mail.password, mail.to_addr, mail.from_addr);
    TDBG("Subject : %s\n", mail.subject);
    TDBG("Mail : %s\n", mail.message_body);
    TDBG("SMTP Server : %s\n", mail.smtp_server);
    TDBG("The %s Login Password has been changed\n", mail.local_host);
    TDBG("The password has been reset for %s (host)\n\tUserName : %s\n\tPassword : %s\n",
            mail.local_host, userName, ClearPswd);

    /* Set user password */
    SetUserPassword((INT8U *)&ReqSetUserPswd, PassLength, (INT8U *)&ResSetUserPswd,BMCInst);
    if(ResSetUserPswd.CompletionCode != CC_NORMAL)
    {
        *(pRes) = ResSetUserPswd.CompletionCode;
        goto end;
    }

    /* Send mail */
    if ((retVal = smtp_mail(&mail)) != EMAIL_UNKNOWN)
    {
    	  switch (retVal)
    	  {
    	      case EMAIL_NO_SOCKET:
    	      case EMAIL_NO_CONNECT:
    	      case EMAIL_NO_HELO_RESPONSE:
    	          *(pRes) = OEMCC_UNABLE_TO_CONNECT_SMTPSERVER;
    	          break;
    	      case EMAIL_AUTH_TYPE_UNSUPPORTED:
    	          *(pRes) = OEMCC_UNSUPPORTED_AUTH_TYPE;
    	          break;
    	      case EMAIL_AUTH_REQUIRED:
    	      case EMAIL_AUTH_FAILED:
    	      case EMAIL_AUTH_NOT_SUPPORTED:
    	          *(pRes) = OEMCC_SEND_EMAIL_AUTH_FAILED;
    	          break;
            default:
    	          *(pRes) = OEMCC_SEND_EMAIL_FAILED;
    	  }

        strncpy((char*)ReqSetUserPswd.Password, (char*)OldPswd, MAX_PASSWORD_LEN);
        /* Preserve Old password */
        SetUserPassword((INT8U *)&ReqSetUserPswd, PassLength, (INT8U *)&ResSetUserPswd,BMCInst);
        if(ResSetUserPswd.CompletionCode != CC_NORMAL)
        {
            *(pRes) = ResSetUserPswd.CompletionCode;
            goto end;
        }

        IPMI_ERROR("Send mail failed with the server (%s) : %d\n", mail.smtp_server, retVal);
    }
    else
    {
        *(pRes) = CC_NORMAL;
    }

end:
    return sizeof(*(pRes));


}

int AMISetEmailFormatUser(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{

    AMISetUserEmailFormatReq_T *pSetUserEmail = (AMISetUserEmailFormatReq_T *)pReq;
     UserInfo_T* pUserInfo;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int cnt=0;
    TDBG("Inside AMISetEmailFormatUser %d \n", pSetUserEmail->UserID);

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pUserInfo = getUserIdInfo (pSetUserEmail->UserID, BMCInst);
    if ( (NULL ==  pUserInfo)  || (pUserInfo->ID != USER_ID) )
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        IPMI_WARNING ("Invalid User Id \n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }
    
    for(cnt=0;cnt< EmailformatCount;cnt++)
    {
        if(memcmp(g_PDKEmailFormat[cnt].EmailFormat,pSetUserEmail->EMailFormat,EMAIL_FORMAT_SIZE)==0)
        {
            break;
        }
    }
    if(cnt==EmailformatCount )
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        IPMI_WARNING ("Invalid Email Format \n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

     _fmemcpy (pUserInfo->EmailFormat, pSetUserEmail->EMailFormat, EMAIL_FORMAT_SIZE);
    FlushIPMI((INT8U*)&pBMCInfo->UserInfo[0],(INT8U*)&pBMCInfo->UserInfo[0],pBMCInfo->IPMIConfLoc.UserInfoAddr,
                      sizeof(UserInfo_T)*MAX_USER_CFG_MDS,BMCInst);

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    *pRes = CC_NORMAL;
    return sizeof(*pRes);

}

int AMIGetEmailFormatUser(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int UserID = *(INT8U *)pReq;
    UserInfo_T* pUserInfo;
    TDBG("Inside AMIGetEmailFormatUser %d \n", UserID);

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pUserInfo = getUserIdInfo (UserID, BMCInst);
    if ( (NULL ==  pUserInfo)  || (pUserInfo->ID != USER_ID))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        IPMI_WARNING ("Invalid User Id \n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    *pRes = CC_NORMAL;
    if( pUserInfo->EmailFormat[0] != 0 )
    {
        _fmemcpy ((pRes+1), pUserInfo->EmailFormat, EMAIL_FORMAT_SIZE);
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        return EMAIL_FORMAT_SIZE+1;
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    return sizeof(*pRes);

}

/*
*@fn AMIGetRootUserAccess
*@brief This command helps to get linux root user access state 
*		@param pReq   - Request for the command   
*		@param ReqLen - Request length for the command
*		@param pRes   - Respose for the command
*		@param BMCInst- BMC Instance
*		@return Returns size of AMISetRootPasswordRes_T
*/
int AMIGetRootUserAccess (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
	AMIGetRootUserAccessRes_T*     pGetRootUserAccessRes = (AMIGetRootUserAccessRes_T*)pRes;

	INT8U    UserId,UserAccessState;
	INT16S   Err = 0;

	UserId    = LINUX_ROOT_USER_UID;

	pGetRootUserAccessRes->CompletionCode = CC_NORMAL;

	Err = LINUX_GetUserAccess (UserId,&UserAccessState);
	if (0 != Err)
	{
		IPMI_ERROR ("AMIResetPass.c : Setting password to Linux database failed\n");
		*pRes = abs (Err);
		return sizeof (*pRes);
	}
	else
		pGetRootUserAccessRes->CurrentUserIDState =  UserAccessState  ;

return sizeof (AMIGetRootUserAccessRes_T);
}
/*
*@fn SetRootPassword
*@brief This command helps to set linux root user password,enabling root user, 
*       disabling root user.
*		@param pReq   - Request for the command   
*		@param ReqLen - Request length for the command
*		@param pRes   - Respose for the command
*		@param BMCInst- BMC Instance
*		@return Returns size of AMISetRootPasswordRes_T
*/

int AMISetRootPassword (INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{

    AMISetRootPasswordReq_T*     pSetRootPswdReq = (AMISetRootPasswordReq_T*)pReq;
    INT16S  Err = 0;     

    if(ReqLen < OP_ONLY_CMD_LENGTH)     //Checking for minimum command Request length
    {
        *pRes = CC_REQ_INV_LEN;
        return  sizeof (*pRes);
    }
	
    //Checking for Valid operation
    if (ReqLen == OP_ONLY_CMD_LENGTH)
    {
        if ((pSetRootPswdReq->Operation != OP_ENABLE_USER_ID) && (pSetRootPswdReq->Operation != OP_DISABLE_USER_ID))
        {
            if(pSetRootPswdReq->Operation == OP_SET_ROOT_PASSWD)
                *pRes = CC_REQ_INV_LEN;
            else	
                *pRes = CC_INV_DATA_FIELD;
                return  sizeof (*pRes); //Invalid operation.
        }
    }

    if(ReqLen > OP_ONLY_CMD_LENGTH)    //Checking for passwd length
    {
        if( ( (ReqLen-1) < LINUX_USER_MIN_PASSWORD_LEN) ||  ( (ReqLen-1) > LINUX_USER_MAX_PASSWORD_LEN) ) 
        {
            *pRes = CC_REQ_INV_LEN;
            return  sizeof (*pRes);
        }
        pSetRootPswdReq->Password[ReqLen-1] = '\0';
    }

    *pRes = CC_NORMAL;

    /* Returning completion code as 0x80 when NULL password is set*/
    if((pSetRootPswdReq->Operation == OP_SET_ROOT_PASSWD) && (strlen((char *)pSetRootPswdReq->Password))  == 0)
    {
        *pRes = CC_NULL_PASSWORD;
        return sizeof(*pRes);
    }

    Err = LINUX_SetRootPassword( pSetRootPswdReq->Operation, pSetRootPswdReq->Password);

    if (0 != Err)
    {
    IPMI_ERROR ("AMIResetPass.c : Setting password to Linux database failed\n");
    *pRes = abs (Err);
    return sizeof (*pRes);
    }

return sizeof (*pRes);
}


/*
*@fn AMIGetUserShelltype
*@brief This command helps to get users shell 
*		@param pReq   - Request for the command   
*		@param ReqLen - Request length for the command
*		@param pRes   - Respose for the command
*		@param BMCInst- BMC Instance
*		@return Returns size of AMIGetUserShelltypeRes_T
*/
int AMIGetUserShelltype(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
    AMIGetUserShelltypeReq_T*       pGetUserShellReq = (AMIGetUserShelltypeReq_T*)pReq;
    AMIGetUserShelltypeRes_T*       pGetUserShellRes = (AMIGetUserShelltypeRes_T*)pRes;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
      UserInfo_T*             pUserInfo = NULL;
    INT8U   UserId=0;

    UserId = pGetUserShellReq->UserID & 0x3F;

    /*  if user ID exceeded the Max limit */
    if (pGetUserShellReq->UserID > pBMCInfo->IpmiConfig.MaxUsers)
    {
        pGetUserShellRes->CompletionCode = CC_INV_DATA_FIELD ;
        return  sizeof (*pRes);/* Invalied user id */
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pUserInfo = getUserIdInfo(UserId, BMCInst);

    /* User with given ID is not created */
    if ( NULL == pUserInfo ) 
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    pGetUserShellRes->CompletionCode = CC_NORMAL;	
    pGetUserShellRes->Shelltype = pUserInfo->UserShell;

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

return sizeof (AMIGetUserShelltypeRes_T);
}


/*
*@fn AMISetUserShelltype
*@brief This command helps to set users shell 
*		@param pReq   - Request for the command   
*		@param ReqLen - Request length for the command
*		@param pRes   - Respose for the command
*		@param BMCInst- BMC Instance
*		@return Returns size of AMISetUserShelltypeRes_T
*/

int AMISetUserShelltype(INT8U *pReq, INT32U ReqLen, INT8U *pRes,int BMCInst)
{
    AMISetUserShelltypeReq_T*       pSetUserShellReq = (AMISetUserShelltypeReq_T*)pReq;
       UserInfo_T*             pUserInfo = NULL;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U   UserId=0, ShellType;
    INT8U oldshell = -1, newshell = -1;

    UserId    = pSetUserShellReq->UserID & 0x3F;
    ShellType     = pSetUserShellReq->Shelltype;
	
    /*  if user ID exceeded the Max limit */
    if (pSetUserShellReq->UserID > pBMCInfo->IpmiConfig.MaxUsers)
    {
        *pRes = CC_INV_DATA_FIELD ;
        return  sizeof (*pRes);/* Invalied user id */
    }

    if((ShellType < 0) || (ShellType > MAX_SHELL_TYPES-1))
    {
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes); //Invalid shell type
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pUserInfo = getUserIdInfo(UserId, BMCInst);
    /* if User is not created */
    if ( NULL == pUserInfo )
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }   
    
    /*if the user is fixed user */
    if (((pUserInfo != NULL) && (pUserInfo->FixedUser == TRUE)))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        *pRes = CC_INV_DATA_FIELD ;
        return  sizeof (*pRes);
    }

    newshell = ShellType;
    oldshell = pUserInfo->UserShell;
    pUserInfo->UserShell = newshell;

    AddIPMIUsrtoShellGrp((char *)pUserInfo->UserName, oldshell, newshell);
    FlushIPMI((INT8U*)&pBMCInfo->UserInfo[0],(INT8U*)&pBMCInfo->UserInfo[0],pBMCInfo->IPMIConfLoc.UserInfoAddr,
                      sizeof(UserInfo_T)*MAX_USER_CFG_MDS,BMCInst);
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    *pRes = CC_NORMAL;
    return sizeof (*pRes);
}
