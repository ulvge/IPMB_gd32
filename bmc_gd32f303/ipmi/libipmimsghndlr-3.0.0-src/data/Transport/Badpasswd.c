/******************************************************************
 ******************************************************************
 ***                                                                                                           **
 ***    (C)Copyright 2006-2009, American Megatrends Inc.                            **
 ***                                                                                                           **
 ***    All Rights Reserved.                                                                          **
 ***                                                                                                           **
 ***    5555 , Oakbrook Pkwy, Norcross,                                                       **
 ***                                                                                                           **
 ***    Georgia - 30093, USA. Phone-(770)-246-8600.                                  **
 ***                                                                                                           **
 ******************************************************************
 ******************************************************************
 ******************************************************************
 *
 * Badpasswd.c
 * Badpasswd related codes
 *
 *  Author: Winston <winstonv@amiindia.co.in>
 ******************************************************************/

#define ENABLE_DEBUG_MACROS 0

#include "Types.h"
#include "Debug.h"
#include "PMConfig.h"
#include "Session.h"
#include "MsgHndlr.h"
#include "Ethaddr.h"
#include "Badpasswd.h"
#include "NVRData.h"
#include "NVRAccess.h"
#include "SensorMonitor.h"
#include "SharedMem.h"
#include "IPMIConf.h"

/*
*@fn CheckPasswordViolation
*@param SerialorLAN - Denotes the channnel number is Serial or LAN
*@param Ch - Channel Number 
*@return Returns 0
*/
int CheckPasswordViolation(INT8U SerialorLAN,INT8U Ch,int BMCInst)
{
    int j,EthIndex=0;
    INT8U Index=0,ThresholdVal=0;
    ChannelInfo_T*pChannelInfo=NULL;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    ChannelUserInfo_T*  pChUserInfo = NULL;
    INT16U AttemptResetInterval = 0,LockOutInterval =0;

    if(SerialorLAN == LAN_CHANNEL_BADP)
    {
        EthIndex = GetEthIndex(Ch,BMCInst);
        ThresholdVal = pBMCInfo->LANCfs[EthIndex].BadPasswd.ThreshNum;
        AttemptResetInterval = pBMCInfo->LANCfs[EthIndex].BadPasswd.ResetInterval;
        LockOutInterval = pBMCInfo->LANCfs[EthIndex].BadPasswd.LockoutInterval;
    }
    else if(SerialorLAN == SERIAL_CHANNEL_BADP)
    {
        ThresholdVal = pBMCInfo->SMConfig.BadPasswd.ThreshNum;
        AttemptResetInterval = pBMCInfo->SMConfig.BadPasswd.ResetInterval;
        LockOutInterval = pBMCInfo->SMConfig.BadPasswd.LockoutInterval;
    }
    else
    {
        /* Bad Password validation has to be done for only 
             Serial and LAN Interface */
        return 0;
    }

    if(ThresholdVal != 0)
    {
        pChannelInfo = getChannelInfo(Ch,BMCInst);
        if(NULL == pChannelInfo)
        {
            TDBG("Unable to get Channel Info to Check Password Violation for channel : %d \n",Ch);
            return 0;
        }

        if(pChannelInfo != NULL)
        {
            for(j=1;j<=g_BMCInfo[BMCInst].IpmiConfig.MaxUsers;j++)
            {
                pChUserInfo = getChUserIdInfo(j,&Index,pChannelInfo->ChannelUserInfo,BMCInst);
                if(pChUserInfo != NULL)
                {
                    if((pChUserInfo->LockedTime != 0) && (pChUserInfo->Lock == USER_LOCKED)
                        && (LockOutInterval != 0))
                    {
                        if((TimeUpdate() -  pChUserInfo->LockedTime) > (10 * LockOutInterval))
                        {
                            pChUserInfo->Lock = USER_UNLOCKED;
                            pChUserInfo->LockedTime = 0;
                            pChUserInfo->FailureAttempts = 0;
                        }
                    }

                    if((pChUserInfo->LockedTime != 0) && (pChUserInfo->Lock == USER_UNLOCKED)
                        && (AttemptResetInterval!= 0))
                    {
                        if((TimeUpdate() - pChUserInfo->LockedTime) > (10 * AttemptResetInterval))
                        {
                            pChUserInfo->LockedTime = 0;
                            pChUserInfo->FailureAttempts = 0;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

/*
*@fn MonitorPassword
*@brief This function monitors the invalid password attempts
*/
void MonitorPassword(int BMCInst)
{
    int i=0;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst]; 

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    for(i=0;i<MAX_NUM_CHANNELS;i++)
    {
        if(IsLANChannel(i,BMCInst))
        {
            CheckPasswordViolation(LAN_CHANNEL_BADP,i,BMCInst);
        }
        else if(pBMCInfo->IpmiConfig.SerialIfcSupport == 0x1 && pBMCInfo->SERIALch == i)
        {
            CheckPasswordViolation(SERIAL_CHANNEL_BADP,i,BMCInst);
        }
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
}


/*
*@fn FindUserLockStatus
*@param Userid - UserId to get the status
*@param Channel - Status of the user for the specified channel
*@return Returns 0 success
*        Returns -1 on failure
*/
int FindUserLockStatus(INT8U Userid,INT8U Channel,int BMCInst)
{
    INT8U Index=0;
    ChannelInfo_T*pChannelInfo = getChannelInfo(Channel,BMCInst);
    if(NULL == pChannelInfo)
    {
        TDBG("Unable to get Channel Info to Find User Lock Status for channel : %d \n",Channel);
        return 0;
    }

    if(pChannelInfo != NULL)
    {
        ChannelUserInfo_T *pChUserInfo = getChUserIdInfo (Userid , &Index, pChannelInfo->ChannelUserInfo,BMCInst);
        if(pChUserInfo != NULL)
        {
            if(pChUserInfo->Lock == USER_UNLOCKED)
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }

    /* Control doesn't reach here*/
    return 0;
}


/*
* @fn LockUser
* @param Userid - Password to be locked for Userid
* @param Channel - Channel Number
* @return Returns 0 on success
*/
int LockUser(INT8U Userid,INT8U Channel,int BMCInst)
{
    INT8U Index=0,EthIndex=0;
    ChannelInfo_T*pChannelInfo = getChannelInfo(Channel,BMCInst);
    if(NULL == pChannelInfo)
    {
        TDBG("Unable to get Channel Info to Lock User for channel : %d \n",Channel);
        return 0;
    }

    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst]; 
    INT8U ThresholdNum= 0xFF; 

    if(IsLANChannel(Channel,BMCInst))
    {
        EthIndex = GetEthIndex(Channel,BMCInst);
        ThresholdNum = pBMCInfo->LANCfs[EthIndex].BadPasswd.ThreshNum;
    }
    else if(pBMCInfo->IpmiConfig.SYSIfcSupport == 1 && pBMCInfo->SERIALch == Channel)
    {
        ThresholdNum = pBMCInfo->SMConfig.BadPasswd.ThreshNum; //TBD
    }
    else
    {
        /*If Channel no: does not belong to LAN or Serial
            Locking of User is discarded*/
        return 0;
    }

    ChannelUserInfo_T *pChUserInfo = getChUserIdInfo (Userid , &Index, pChannelInfo->ChannelUserInfo,BMCInst);

    if(ThresholdNum != 0 && pChUserInfo != NULL)
   {
         pChUserInfo->FailureAttempts++;
         pChUserInfo->LockedTime = TimeUpdate();
         if(pChUserInfo->FailureAttempts >= ThresholdNum)
         {
            pChUserInfo->Lock = USER_LOCKED;
            GenerateLockEvent(Channel,Userid,BMCInst);
         }
    }
    
    return 0;
}

/*
*@fn UnlockUser
*@param Userid -Password to be unlocked for Userid
*@param Channel - Channel Number
*@return Returns 0 on success
*/
int UnlockUser(INT8U Userid,INT8U Channel,int BMCInst)
{
    INT8U Index=0,EthIndex=0;
    ChannelInfo_T*pChannelInfo = getChannelInfo(Channel,BMCInst);
    if(NULL == pChannelInfo)
    {
        TDBG("Unable to get Channel Info to UnLock User for channel : %d \n",Channel);
        return 0;
    }

    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst]; 
    INT8U ThresholdNum= 0xFF; 

    if(pChannelInfo != NULL)
    {
        if(IsLANChannel(Channel,BMCInst))
        {
            EthIndex = GetEthIndex(Channel,BMCInst);
            ThresholdNum = pBMCInfo->LANCfs[EthIndex].BadPasswd.ThreshNum;
        }
        else if(pBMCInfo->IpmiConfig.SYSIfcSupport == 1 && pBMCInfo->SERIALch == Channel)
        {
            ThresholdNum = pBMCInfo->SMConfig.BadPasswd.ThreshNum; //TBD
        }
        else
        {
            /*If Channel no: does not belong to LAN or Serial
                Locking of User is discarded*/
            return 0;
        }

        ChannelUserInfo_T *pChUserInfo = getChUserIdInfo (Userid , &Index, pChannelInfo->ChannelUserInfo,BMCInst);

        if(ThresholdNum != 0 && pChUserInfo != NULL)
        {
            pChUserInfo->FailureAttempts=0;
            pChUserInfo->LockedTime = 0;
            pChUserInfo->Lock = USER_UNLOCKED;
        }
    }

    return 0;
}

/*
*@fn ClearUserLockAttempts
*/
int ClearUserLockAttempts(INT8U SerialorLAN,int BMCInst)
{
    int i=0,j=0;
    INT8U Index=0;//,EthIndex=0;
    ChannelInfo_T*pChannelInfo=NULL;
    ChannelUserInfo_T*  pChUserInfo = NULL;
     BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst]; 

    TDBG("Inside ClearUserlock \n");
    for(i=0;i<MAX_NUM_CHANNELS;i++)
    {
        if((IsLANChannel(i,BMCInst) && SerialorLAN == LAN_CHANNEL_BADP) || 
            ((pBMCInfo->IpmiConfig.SerialIfcSupport == 0x01) && (pBMCInfo->SERIALch == i) && (SerialorLAN == SERIAL_CHANNEL_BADP)))
        {
            pChannelInfo = getChannelInfo(i,BMCInst);
            if(pChannelInfo != NULL)
            {
                for(j=1;j<=g_BMCInfo[BMCInst].IpmiConfig.MaxChUsers;j++)
                {
                    pChUserInfo = getChUserIdInfo(j,&Index,pChannelInfo->ChannelUserInfo,BMCInst);
                    if(pChUserInfo != NULL)
                    {
                    	TDBG("Unlocking the user \n");
                        pChUserInfo->LockedTime = 0;
                        pChUserInfo->FailureAttempts = 0;
                    }
                }
            }
        }
    }

    return 0;
}


/*
*@fn GenerateLockEvent
*@param Event -Denotes whether event has to be generated
*@return Returns 0
*/
int GenerateLockEvent(INT8U Channel,INT8U UserID,int BMCInst)
{
    INT8U EventMsg[9],EthIndex=0,GenEvent=0;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst]; 

    if(IsLANChannel(Channel,BMCInst))
    {
        EthIndex = GetEthIndex(Channel,BMCInst);
        GenEvent = pBMCInfo->LANCfs[EthIndex].BadPasswd.GenEvent & 0x01;
    }
    else if(pBMCInfo->IpmiConfig.SYSIfcSupport == 1 && pBMCInfo->SERIALch == Channel)
    {
        GenEvent = pBMCInfo->SMConfig.BadPasswd.GenEvent & 0x01;
    }

    if(GenEvent)
    {
        EventMsg[0] = pBMCInfo->IpmiConfig.BMCSlaveAddr; /* Generator ID */
        EventMsg[1] = 0;                          /* Generator ID */
        EventMsg[2] = 4;                          /* EvM Rev */
        EventMsg[3] = 0x2A;                     /* Sensor Type */
        EventMsg[4] = 0;                          /* Sensor Number*/  //TBD
        EventMsg[5] = 0x6F;                     /* Event Dir | Event Type */
        EventMsg[6] = 0x03;                /* Event Data 1 */
        EventMsg[7] = UserID;                 /* Event Data 2 */
        EventMsg[8] = 0xFF;                     /* Event Data 3 */

        /* Post Event Message */
        if ( PostEventMessage(EventMsg,FALSE,sizeof(EventMsg),BMCInst) != 0)
        {
            TDBG("Generation of Event Message for User Lock failed \n");
        }
    }
    
    return 0;
}

