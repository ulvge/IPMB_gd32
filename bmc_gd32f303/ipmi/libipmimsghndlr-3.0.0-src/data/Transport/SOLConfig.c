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
 ****************************************************************
 *
 * SOLConfig.c
 * SOL configuration 
 *
 *  Author: Govind kothandapani <govindk@ami.com>
 ****************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "Debug.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "SharedMem.h"
#include "PMConfig.h"
#include "IPMI_SOLConfig.h"
#include "NVRAccess.h"
#include "PDKDefs.h"
#include "PDKAccess.h"
#include"Ethaddr.h"
#include "IPMIConf.h"

/*** Local Definitions ***/
#define SOL_SET_IN_PROGRESS                 0
#define SOL_ENABLE_PARAM                    1
#define SOL_AUTHENTICATION_PARAM            2
#define SOL_ACCUM_THRESHOLD_PARAM           3
#define SOL_RETRY_PARAM                     4
#define SOL_NVOL_BIT_RATE_PARAM             5
#define SOL_VOL_BIT_RATE_PARAM              6
#define SOL_PAYLD_CHANNEL_PARAM             7
#define SOL_PAYLD_PORT_NUM_PARAM            8

#define SET_IN_PROGRESS                     0x01
#define SET_COMPLETE                        0x00
#define SOL_SET_IN_PROGRESS_MASK 0x03
#define PARAM_REVISION                      0x11
#define GET_PARAM_REV_ONLY_MASK             0x80

#define BAUD_RATE_9600          			6
#define BAUD_RATE_19200         			7
#define BAUD_RATE_38400         			8
#define BAUD_RATE_57600         			9
#define BAUD_RATE_115200        			10




/* Reserved Bits table */
#define MAX_SOL_PARAMS_DATA  9
#define RESERVED_VALUE_70					0x70
#define RESERVED_VALUE_F0					0xF0
typedef struct
{
    INT8U	Params;
    INT8U	ReservedBits [MAX_SOL_PARAMS_DATA];
    INT8U	DataLen;

} SOLCfgRsvdBits_T;

static SOLCfgRsvdBits_T m_RsvdBitsCheck [] = {

    /* Param        Reserved Bits      Data Size   */
    { 0,	       { 0xFC }, 			0x1 },		
    { 1,		{ 0xFE }, 			0x1 },		
    { 2,		{ 0x30 },				0x1 },
    { 4,		 { 0xF8 },			 0x1 },	
    { 5,		 { 0xF0 },			 0x1 }

};




/*** Module Variables ***/

/**
 * @brief SOL Configuration Parameter Lengths
**/
static const INT8U  SOLConfigParamLengths [] = { 1,1,1,2,2,1,1,1,2 };

/**
 * IsBaudRateValid
 **/
static int IsBaudRateValid (INT8U BaudRate);

// Function to check if valid privileges
static int IsValidPrivilege(INT8U authtype);

#if SOL_ACTIVATING_COMMAND != UNIMPLEMENTED
/*---------------------------------------
 * SOLActivating
 *---------------------------------------*/
int
SOLActivating (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    /**
     * Send SOL activating Message to Remote Application connected to
     * Connected to Serial port
    **/

    *pRes = CC_NORMAL;
    return sizeof (*pRes);
}
#endif /* SOL_ACTIVATING_COMMAND */


#if GET_SOL_CONFIGURATION != UNIMPLEMENTED
/*---------------------------------------
 * GetSOLConfig
 *---------------------------------------*/
int
GetSOLConfig (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetSOLConfigReq_T*   GetReq = (_NEAR_ GetSOLConfigReq_T*) pReq;
    _NEAR_ GetSOLConfigRes_T*   GetRes = (_NEAR_ GetSOLConfigRes_T*) pRes;
    _NEAR_ GetSOLConfigOEMRes_T*   GetOEMRes = (_NEAR_ GetSOLConfigOEMRes_T*) pRes;
    _FAR_  BMCSharedMem_T*      pSharedMem = BMC_GET_SHARED_MEM (BMCInst);
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U Size,curchannel;
    INT8U ChannelNum, EthIndex;
    INT8U oem_len;
    unsigned long oem_addr;

    ChannelNum = GetReq->ChannelNum & 0x0F;

    if(GetReq->ChannelNum & RESERVED_VALUE_70)
    {
        /* Alarm !!! Somebody is trying to set Reseved Bits */
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    if (ChannelNum > 0x0F)
    {
        *pRes =CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    if (0x0E == ChannelNum)
    {
        OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
        EthIndex = GetEthIndex (curchannel  & 0xF, BMCInst);
    }
    else
    {
        EthIndex = GetEthIndex (ChannelNum, BMCInst);
    }

    if (0xFF == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    if((GetReq->SetSel != 0x00) ||(GetReq->BlkSEl != 0x00))
    {
         *pRes =CC_INV_DATA_FIELD;
         return sizeof(*pRes);    
    }

    GetRes->CompletionCode = CC_NORMAL;
    GetRes->ParamRev       = PARAM_REVISION;

    /* Check if only parameter revision is required */
    if (0 != (GetReq->ChannelNum & GET_PARAM_REV_ONLY_MASK))
    {
        if((MAX_SOL_CONF_PARAM >= GetReq->ParamSel))
        {
            return sizeof(GetSOLConfigRes_T);
        }
        else if(( NULL != g_PDKHandle[PDK_GETSOLOEMPARAM]) && 
                ((MIN_SOL_OEM_CONF_PARAM <= GetReq->ParamSel) && (MAX_SOL_OEM_CONF_PARAM >= GetReq->ParamSel)))
        {
            oem_len = ((int(*)(INT8U, unsigned long*,int))(g_PDKHandle[PDK_GETSOLOEMPARAM]))(GetReq->ParamSel, &oem_addr ,BMCInst);
            if( oem_len == 0)
            {
                GetRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
                return sizeof(INT8U);
            }
            else
                return sizeof(GetSOLConfigRes_T);
        }
        else
        {
            *pRes = CC_PARAM_NOT_SUPPORTED;
            return sizeof (*pRes);  
        }
    }

    Size = sizeof (GetSOLConfigRes_T);
    /* Load individual configurations */
    switch (GetReq->ParamSel)
    {
        case SOL_SET_IN_PROGRESS:

            LOCK_BMC_SHARED_MEM(BMCInst);
            (*(_NEAR_ INT8U*)(GetRes + 1)) = BMC_GET_SHARED_MEM(BMCInst)->m_SOL_SetInProgress;
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            Size = Size + sizeof (INT8U);
            break;

        case SOL_ENABLE_PARAM:

            (*(_NEAR_ INT8U*)(GetRes + 1)) = pBMCInfo->SOLCfg[EthIndex].SOLEnable;
            Size = Size + sizeof (INT8U);
            break;

        case SOL_AUTHENTICATION_PARAM:

            (*(_NEAR_ INT8U*)(GetRes + 1)) = pBMCInfo->SOLCfg[EthIndex].SOLAuth;
            Size = Size + sizeof (INT8U);
            break;

        case SOL_ACCUM_THRESHOLD_PARAM:

            _fmemcpy((INT8U *)(GetRes+1),(INT8U *)&pBMCInfo->SOLCfg[EthIndex].CharAccThresh,sizeof(INT16U));
            Size = Size + sizeof (INT16U);
            break;

        case SOL_RETRY_PARAM:

            (*((_NEAR_ INT8U*)(GetRes + 1))) = pBMCInfo->SOLCfg[EthIndex].SOLRetryCount;
            (*((_NEAR_ INT8U*)(GetRes + 1) + 1)) = pBMCInfo->SOLCfg[EthIndex].SOLRetryInterval;
            Size = Size + sizeof (INT16U);
            break;

        case SOL_NVOL_BIT_RATE_PARAM:

            (*(_NEAR_ INT8U*)(GetRes + 1)) = pBMCInfo->SOLCfg[EthIndex].NVBitRate;
            Size = Size + sizeof (INT8U);
            break;

        case SOL_VOL_BIT_RATE_PARAM:

            (*(_NEAR_ INT8U*)(GetRes + 1)) = pSharedMem->SOLBitRate[EthIndex];
            Size = Size + sizeof (INT8U);
            break;

        case SOL_PAYLD_CHANNEL_PARAM:

            (*(_NEAR_ INT8U*)(GetRes + 1)) = pBMCInfo->SOLCfg[EthIndex].PayldChannel;
            Size = Size + sizeof (INT8U);
            break;

        case SOL_PAYLD_PORT_NUM_PARAM:

            _fmemcpy((INT8U *)(GetRes+1),(INT8U *)&pBMCInfo->SOLCfg[EthIndex].PayldPortNum,sizeof(INT16U));
            Size = Size + sizeof (INT16U);
            break;

        default:
            if(g_PDKHandle[PDK_GETSOLOEMPARAM] != NULL &&
                (GetReq->ParamSel >= 192 && GetReq->ParamSel <= 255))
            {
                oem_len = ((int(*)(INT8U, unsigned long*,int))(g_PDKHandle[PDK_GETSOLOEMPARAM]))(GetReq->ParamSel, &oem_addr ,BMCInst);
                if( oem_len == 0)
                {
                    GetRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
                    return sizeof(INT8U);
                }
                else
                {
                    //Acquire the OEM parameters
                    if( oem_len < MSG_PAYLOAD_SIZE - sizeof(GetSOLConfigOEMRes_T))
                    {
                        memcpy((char*)GetOEMRes + sizeof(GetSOLConfigOEMRes_T) ,\
                                    (unsigned int*)oem_addr , oem_len);
                    }
                    else
                    {
                        GetRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
                        return sizeof(INT8U);
                    }
                    return sizeof(GetSystemInfoParamOEMRes_T) + oem_len;
                }
            }
            else
            {
                GetRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
                Size = sizeof (INT8U);
            }
    }

    return  Size;
}
#endif /* GET_SOL_CONFIGURATION */


#if SET_SOL_CONFIGURATION != UNIMPLEMENTED
/*---------------------------------------
 * SetSOLConfig
 *---------------------------------------*/
int
SetSOLConfig (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SetSOLConfigReq_T*   SetReq = (_NEAR_ SetSOLConfigReq_T*) pReq;
    _NEAR_ SetSOLConfigOEMReq_T*   pSetOEMReq = (_NEAR_ SetSOLConfigOEMReq_T*) pReq;
    _FAR_  BMCSharedMem_T*      pSharedMem = BMC_GET_SHARED_MEM (BMCInst);
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int i=0, j=0;
    INT8U ChannelNum, EthIndex,curchannel;
    unsigned long oem_addr;
    int size;
    INT8U  m_SOL_SetInProgress; /**< Contains setting SOL configuration status */
    
    ChannelNum = SetReq->ChannelNum & 0x0F;

    if (ChannelNum > 0x0F)
    {
        *pRes =CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    if (0x0E == ChannelNum)
    {
        OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
        EthIndex = GetEthIndex (curchannel  & 0xF, BMCInst);
    }
    else
    {
        EthIndex = GetEthIndex (ChannelNum, BMCInst);
    }

    if (0xFF == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return  sizeof (*pRes);
    }

    if (SetReq->ParamSel >= sizeof (SOLConfigParamLengths))
    {
        *pRes = CC_PARAM_NOT_SUPPORTED;
        return sizeof (*pRes);
    }

    /* Validate Req Lengths */
    if (ReqLen != (sizeof (SetSOLConfigReq_T) +
                   SOLConfigParamLengths [SetReq->ParamSel]))
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof (*pRes);
    }

    if(SetReq->ChannelNum & RESERVED_VALUE_F0)
    {
        /* Alarm !!! Somebody is trying to set Reseved Bits */
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    /* Check for Reserved Bits */
    for (i = 0; i < sizeof (m_RsvdBitsCheck)/ sizeof (m_RsvdBitsCheck[0]); i++)
    {
        /* Check if this Parameter Selector needs Reserved bit checking !! */
        if (m_RsvdBitsCheck[i].Params == SetReq->ParamSel )	
        {
            for (j = 0; j < m_RsvdBitsCheck[i].DataLen; j++)
            {
                if ( 0 != (pReq[2+j] & m_RsvdBitsCheck[i].ReservedBits[j]))
                {
                    /* Alarm !!! Somebody is trying to set Reseved Bits */
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (*pRes);
                }
            }
        }
    }

    *pRes = CC_NORMAL;
    switch (SetReq->ParamSel)
    {
        case SOL_SET_IN_PROGRESS:
            if(((*(_NEAR_ INT8U*)(SetReq + 1)) & SOL_SET_IN_PROGRESS_MASK) == SOL_SET_IN_PROGRESS_MASK)
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (*pRes);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            m_SOL_SetInProgress = BMC_GET_SHARED_MEM(BMCInst)->m_SOL_SetInProgress;
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            /* Commit Write is optional and supported
             * only if rollback is supported */
            if ((SET_COMPLETE != (*(_NEAR_ INT8U*)(SetReq + 1))) &&
                (SET_IN_PROGRESS != (*(_NEAR_ INT8U*)(SetReq + 1))))
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                return sizeof (*pRes);
            }
            else if ((SET_IN_PROGRESS == m_SOL_SetInProgress) &&
                (SET_IN_PROGRESS == (*(_NEAR_ INT8U*)(SetReq + 1))))
            {
                /*Set In Progress already Set */
                *pRes = CC_SET_IN_PROGRESS;
                return sizeof (*pRes);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            BMC_GET_SHARED_MEM(BMCInst)->m_SOL_SetInProgress = (*(_NEAR_ INT8U*)(SetReq + 1));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case SOL_ENABLE_PARAM:
            pBMCInfo->SOLCfg[EthIndex].SOLEnable = (*(_NEAR_ INT8U*)(SetReq + 1));
            break;

        case SOL_AUTHENTICATION_PARAM:
            /*
            #34323: Check if user is setting reserved bits. This cannot be handled at main logic
            of reserve bit checking.
            */
            if (0 != IsValidPrivilege((0x3F & (*(_NEAR_ INT8U*)(SetReq + 1)))))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (*pRes);    
            }

            pBMCInfo->SOLCfg[EthIndex].SOLAuth = (*(_NEAR_ INT8U*)(SetReq + 1));
            break;

        case SOL_ACCUM_THRESHOLD_PARAM:
                if(((*(_NEAR_ INT8U*)(SetReq + 1)) == 0) || ((*((_NEAR_ INT8U*)(SetReq +1)+1)) == 0))
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (*pRes);           
                }
                    
                pBMCInfo->SOLCfg[EthIndex].CharAccThresh = (*(_NEAR_ INT16U*)(SetReq + 1));
                break;

        case SOL_RETRY_PARAM:

            pBMCInfo->SOLCfg[EthIndex].SOLRetryCount = (*((_NEAR_ INT8U*)(SetReq +1) ) );
            pBMCInfo->SOLCfg[EthIndex].SOLRetryInterval = (*((_NEAR_ INT8U*)(SetReq +1)+1));
            break;

        case SOL_NVOL_BIT_RATE_PARAM:

            if ( 0 != IsBaudRateValid (*(_NEAR_ INT8U*)(SetReq + 1)))
            {
                /*Invalid baud rate setting */
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (*pRes);
            }
            if((*(_NEAR_ INT8U*)(SetReq+1)) == 0)
            *(_NEAR_ INT8U*)(SetReq+1) = BAUD_RATE_9600;

            pBMCInfo->SOLCfg[EthIndex].NVBitRate = *(_NEAR_ INT8U*)(SetReq + 1);

            break;

        case SOL_VOL_BIT_RATE_PARAM:

            if ( 0 != IsBaudRateValid (*(_NEAR_ INT8U*)(SetReq + 1)))
            {
                /*Invalid baud rate setting */
                *pRes = CC_INV_DATA_FIELD;
                return sizeof (*pRes);
            }
            if((*(_NEAR_ INT8U*)(SetReq+1)) == 0)
            *(_NEAR_ INT8U*)(SetReq+1) = BAUD_RATE_9600;

            pSharedMem->SOLBitRate[EthIndex] = *(_NEAR_ INT8U*)(SetReq + 1);

            break;

        case SOL_PAYLD_CHANNEL_PARAM:

            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (*pRes);
            break;

        case SOL_PAYLD_PORT_NUM_PARAM:

            *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
            return sizeof (*pRes);
            break;

        default:
            if(g_PDKHandle[PDK_SETSOLOEMPARAM] != NULL &&
                (SetReq->ParamSel >= 192 && SetReq->ParamSel <= 255))
            {
                oem_addr = (unsigned long)((char*)pSetOEMReq + sizeof(SetSOLConfigOEMReq_T));
                size = ((int(*)(INT8U, unsigned long*,int))(g_PDKHandle[PDK_SETSOLOEMPARAM]))(SetReq->ParamSel, &oem_addr ,BMCInst);
                if(size <= 0)
                {
                    *pRes = CC_PARAM_NOT_SUPPORTED;
                    return sizeof(*pRes);
                }
                else
                {
                    *pRes = CC_PARAM_NOT_SUPPORTED;
                    return sizeof(*pRes);
                }
            }         
    }

    FlushIPMI((INT8U*)&pBMCInfo->SOLCfg[0],(INT8U*)&pBMCInfo->SOLCfg[EthIndex],pBMCInfo->IPMIConfLoc.SOLCfgAddr,
                      sizeof(SOLConfig_T),BMCInst);

    return sizeof (*pRes);
}
#endif /* SET_SOL_CONFIGURATION */

/**
 * IsBaudRateValid
 **/
int 
IsBaudRateValid (INT8U BaudRate)
{
	
    /* Check against supported Baud Rates */
    if ((BaudRate == 0)  			  ||
        (BaudRate == BAUD_RATE_9600)  ||
        (BaudRate == BAUD_RATE_19200) || 
        (BaudRate == BAUD_RATE_38400) || 
        (BaudRate == BAUD_RATE_57600) || 
        (BaudRate == BAUD_RATE_115200))
    {
    //Baud rate is valid 
    return 0;	
    }

    //Baud rate is invvalid 
    return -1;	
}

/*
Check if user entered Auth type is valid
*/

    int  IsValidPrivilege(INT8U  priv_lvl)
    {
        if ((priv_lvl == 02) ||
            (priv_lvl == 03) ||
            (priv_lvl == 04) ||
            (priv_lvl == 05))
        return 0; 
            else
        return -1; //// Invalid privilege
    }

#if 0
/**
 * InitSOLPort
 **/
int
InitSOLPort (INT8U BaudRate)
{
    int     status;
    int		fd;
    struct  termios tty_struct;

    if ((fd = open(SOL_IFC_PORT,O_RDONLY)) < 0)
    {
        IPMI_WARNING ("Can't open serial port..%s\n",strerror(errno));
        return -1;
    }

    status = tcgetattr(fd,&tty_struct);   			   /* get termios structure */

    switch (BaudRate) {
    case BAUD_RATE_9600:
        cfsetospeed(&tty_struct, B9600);
        cfsetispeed(&tty_struct, B9600);
        break;
    case BAUD_RATE_19200:
        cfsetospeed(&tty_struct, B19200);
        cfsetispeed(&tty_struct, B19200);
        break;
    case BAUD_RATE_38400:
        cfsetospeed(&tty_struct, B38400);
        cfsetispeed(&tty_struct, B38400);
        break;
    case BAUD_RATE_57600:
        cfsetospeed(&tty_struct, B57600);
        cfsetispeed(&tty_struct, B57600);
        break;
    case BAUD_RATE_115200:
        cfsetospeed(&tty_struct, B115200);
        cfsetispeed(&tty_struct, B115200);
        break;
    default:
        IPMI_ERROR ("SOLConfig.c : Invalid baud rate = %x\n", BaudRate);
    }

    tty_struct.c_cflag |= CS8;              /* Set 8bits/charecter          */
    tty_struct.c_cflag &= ~CSTOPB;          /* set framing to 1 stop bits   */
    tty_struct.c_cflag &= ~(PARENB);        /* set parity to NONE           */
    tty_struct.c_iflag &= ~(INPCK);

    /* Set Hardware flow control */
    tty_struct.c_cflag |= CRTSCTS;

    tty_struct.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);     /* Raw input mode */
    tty_struct.c_oflag &= ~(OCRNL | ONOCR | ONLRET);
    tty_struct.c_iflag &= ~(IXON | IXOFF);     /* no sw flow ctrl */
    tty_struct.c_iflag &= ~INLCR;
    tty_struct.c_iflag &= ~ICRNL;
    tty_struct.c_iflag &= ~IGNCR;

    PDK_InitSOLPort (&tty_struct);	    /* OEM specific SOL initialization */

    /* set the new attributes in the tty driver */
    status = tcsetattr(fd, TCSANOW, &tty_struct);

    close (fd);

    return 0;
}

#endif /* #if 0 */

