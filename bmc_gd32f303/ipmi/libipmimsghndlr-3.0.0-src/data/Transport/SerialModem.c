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
 ***************************************************************** ****************************************************************
 *
 * SerialModem.c
 * Serial/Modem configuration , Callback &  MUX
 *
 *  Author: Govindarajan <govindarajann@amiindia.co.in>
 *          Vinoth Kumar <vinothkumars@amiindia.co.in>
 ****************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "Debug.h"
#include "IPMI_Main.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "IPMI_SerialModem.h"
#include "SerialModem.h"
#include "SharedMem.h"
#include "AppDevice.h"
#include "Message.h"
#include "PMConfig.h"
#include "Util.h"
#include "SerialIfc.h"
#include "NVRAccess.h"
#include "IPMIConf.h"
#include "PDKDefs.h"
#include "PDKAccess.h"


/* Reserved bit macro definitions */
#define RESERVED_BITS_SETSERIALMODEMCONFIG_CH           0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_SETSERIALMODEMCONFIG_SERINPRO     0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)

#define RESERVED_BITS_GETSERIALMODEMCONFIG_REVCH        0X70 //(BIT6 | BIT5 | BIT4)

#define RESERVED_BITS_CALLBACK_CH                       0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_CALLBACK_DESTSEL                  0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)

#define RESERVED_BITS_SETUSERCALLBACKOPT_USRID          0xC0 //(BIT7 | BIT6)
#define RESERVED_USER_ID   0x01
#define RESERVED_BITS_SETUSERCALLBACKOPT_CH             0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_SETUSERCALLBACKOPT_USRCALLBAKCAP  0xFC //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2)
#define RESERVED_BITS_SETUSERCALLBACKOPT_CBCNEGOPT      0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)

#define RESERVED_BITS_GETUSERCALLBACKOPTIONS_USRID      0xC0 //(BIT7 | BIT6)

#define RESERVED_BITS_SETSERIALMODEMMUX_CH              0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_SETSERIALMODEMMUX_MUXSETREQ       0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)

#define RESERVED_BITS_SERIALMODEMCONNECTACTIVE          0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)

#define RESERVED_BITS_GETTAPRESPONSECODES_CH            0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)

/*** Local Typedefs ***/
/**
 * @struct SMConfigTable_T
 * @brief Serial Modem Configuration Table.
**/

#pragma pack( )
typedef struct
{
    INT8U   ParamFlags;
    INT8U   MaxSetSelector;
    INT8U   MaxBlockSelector;
    unsigned long  Offset;
    INT8U   Size;

} PACKED SMConfigTable_T;

#pragma pack( )

/**
 * @struct RsvdBitsTable_T
 * @brief Reserved Bits Table.
**/

#pragma pack (1)
typedef struct
{
    INT8U   Param;
    INT8U   Length;

} PACKED RsvdBitsTable_T;


#pragma pack( )


/*** Local Definitions ***/
#define PARAM_VOLATILE                  0x01
#define PARAM_NON_VOLATILE              0x02
#define PARAM_VOLATILE_NON_VOLATILE     0x04
#define PARAM_READ_ONLY                 0x08
#define PARAM_WRITE_ONLY                0x10
#define CHECK_RSVD_BITS                 0x20
#define BLOCK_SELECTOR_REQ              0x80

#define GET_PARAM_REV_ONLY              0x80
#define PARAM_REVISION                  0x11

#define SET_IN_PROGRESS                 1
#define SET_COMPLETE                    0
#define SET_AUTH_SUPPORT                1
#define SET_AUTH_ENABLE                 2
#define SET_CONNECTION_MODE             3
#define SET_SESSION_INACTIVE_TIMEOUT    4
#define SET_CHANNEL_CALLBACK_CTRL       5
#define SET_SESSION_TERMINATION         6
#define SET_BAUD_RATE                   7
#define SET_MUX_SWITCH_CTRL             8
#define SET_TMODE_CFG                 29
#define SET_BAD_PWD_THRESHOLD           54
#define SET_IN_PROGRESS_MASK 0x03

#define INIT_STR_PARAM                  10

/* Set Serial Port Mux */
#define REQ_CUR_MUX_SETTING             0
#define REQ_MUX_TO_SYS                  1
#define REQ_MUX_TO_BMC                  2
#define REQ_MUX_FORCE_TO_SYS            3
#define REQ_MUX_FORCE_TO_BMC            4
#define REQ_BLOCK_MUX_TO_SYS            5
#define REQ_ALLOW_MUX_TO_SYS            6
#define REQ_BLOCK_MUX_TO_BMC            7
#define REQ_ALLOW_MUX_TO_BMC            8

#define REQ_MUX_TO_SYS_MASK             0x80
#define REQ_MUX_TO_BMC_MASK             0x40

#define SET_MUX_TO_SYS_MASK             0xFE
#define SET_MUX_TO_BMC_MASK             0x1

#define ALLOW_MUX_TO_SYS_MASK           0x7F
#define BLOCK_MUX_TO_SYS_MASK           0x80
#define ALLOW_MUX_TO_BMC_MASK           0xBF
#define BLOCK_MUX_TO_BMC_MASK           0x40

#define MUX_REQ_ACCEPTED_MASK           0x02
#define MUX_REQ_REJECTED_MASK           0xFD
#define MUX_SESSION_ACTIVE_MASK         0x04
#define MUX_SESSION_ALERT_INACTIVE_MASK 0xF3
#define MUX_ALERT_IN_PROGRESS_MASK      0x08

#define ALLOW_MUX_SWITCH_ON_DCD_LOSS    0x01
#define SERIAL_PORT_SHARING_ENABLED     0x08

#define IPMI_CALLBACK_MASK              1
#define CBCP_CALLBACK_MASK              2

#define SESSION_STATE_ACTIVE            1
#define SESSION_STATE_INACTIVE          0
#define DIRECT_PING_ENABLE_MASK         2
#define MUX_SWITCHED_TO_SYS             2

#define PING_MSG_ACTIVE_BIT             0x01

#define BAUD_RATE_9600          		6
#define BAUD_RATE_19200         		7
#define BAUD_RATE_38400         		8
#define BAUD_RATE_57600         		9
#define BAUD_RATE_115200        		10

#define NO_FLOWCONTROL          		0
#define HARDWARE_FLOWCONTROL    		1
#define XON_XOFF_FLOWCONTRO     		2

#define MODEM_DIRECT_CONNECT_MODE_MASK          0x80
#define ENABLE_PPP_MODE_MASK                    0x02

#define SWITCH_TO_BMC_ON_PPP                    0x08
#define CONNECT_ACTIVE_MSG_CALLBACK_MASK        0x04
#define CONNECT_ACTIVE_MSG_DIRECT_MASK          0x02
#define CONNECT_ACTIVE_MSG_RETRY_MASK           0x01

#define SERIAL_BAUD_RATE            0x0F

#define SM_OFFSET(MEMBER)               (unsigned long)&(((SMConfig_T*) 0)->MEMBER)


/**
 * @brief SerialModem Validation Info Table
**/
const SMConfigTable_T SMConfigTable [] =
{
    { PARAM_VOLATILE,               0,                          0,                              SM_OFFSET (SetInProgress),          1                           },  // Set In Progress
    { PARAM_READ_ONLY,              0,                          0,                              SM_OFFSET (AuthTypeSupport),        1                           },  // Authentication Type Support
    { PARAM_NON_VOLATILE | \
                CHECK_RSVD_BITS,    0,                          0,                              SM_OFFSET (AuthTypeEnable),         sizeof (AuthTypeEnable_T)   },  // Authetication Type enable
    { PARAM_NON_VOLATILE | \
                CHECK_RSVD_BITS,    0,                          0,                              SM_OFFSET (ConnectionMode),         1,                          },  // Connection Mode
    { PARAM_NON_VOLATILE | \
                CHECK_RSVD_BITS,    0,                          0,                              SM_OFFSET (SessionInactivity),1                         },
    { PARAM_NON_VOLATILE | \
                CHECK_RSVD_BITS,    0,                          0,                              SM_OFFSET (ChannelCallBackCtrl),    sizeof (ChannelCallbackCtrl_T)  },
    { PARAM_NON_VOLATILE | \
                CHECK_RSVD_BITS,    0,                          0,                              SM_OFFSET (SessionTermination),     1                               },
    { PARAM_NON_VOLATILE | \
                CHECK_RSVD_BITS,    0,                          0,                              SM_OFFSET (IpmiMsgCommSet),         sizeof (IpmiMsgCommSettings_T)  },
    { PARAM_NON_VOLATILE | \
                CHECK_RSVD_BITS,    0,                          0,                              SM_OFFSET (MUXSwitchCtrl),          sizeof (MuxSwitchCtrl_T)        },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (RingTime),               sizeof (ModemRingTime_T)        },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_INIT_STR_BLOCKS,  0,                              SM_OFFSET (ModemInitString [0] [0]),MAX_MODEM_INIT_STR_BLOCK_SIZE + 0x80    },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (ModemEscapeSeq [0]),     MAX_MODEM_ESC_SEQ_SIZE      },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (ModemHangup [0]),        MAX_MODEM_HANG_UP_SEQ_SIZE  },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (ModemDialCmd [0]),       MAX_MODEM_DIAL_CMD_SIZE     },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (PageBlockOut),           1                           },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (CommunityString [0]),    MAX_COMM_STRING_SIZE        },
    { PARAM_READ_ONLY,              0,                          0,                              SM_OFFSET (TotalAlertDest),         1                           },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_SERIAL_ALERT_DESTINATIONS,0,                            SM_OFFSET (DestinationInfo [0]),        sizeof (DestInfo_T)         },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (CallRetryInterval),      1                           },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_SERIAL_ALERT_DESTINATIONS,0,                            SM_OFFSET (DestComSet [0]),             sizeof (ModemDestCommSettings_T)    },
    { PARAM_READ_ONLY,              0,                          0,                              SM_OFFSET (TotalDialStr),           1                           },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_DIAL_STRS + 0x80, MAX_MODEM_DIAL_STR_BLOCKS,      SM_OFFSET (DestDialStrings [0] [0] [0]),MAX_MODEM_DIAL_STR_BLOCK_SIZE + 0x80},
    { PARAM_READ_ONLY,              0,                          0,                              SM_OFFSET (TotalDestIP),            1                           },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_ALERT_DEST_IP_ADDRS,0,                            SM_OFFSET (DestAddr [0]),           sizeof (DestIPAddr_T)       },
    { PARAM_READ_ONLY,              0,                          0,                              SM_OFFSET (TotalTAPAcc),            1                           },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_TAP_ACCOUNTS,     0,                              SM_OFFSET (TAPAccountSelector[0]),  1                           },
    { PARAM_VOLATILE_NON_VOLATILE | \
                PARAM_WRITE_ONLY,   MAX_MODEM_TAP_ACCOUNTS,     0,                              SM_OFFSET (TAPPasswd [0] [0]),      TAP_PASSWORD_SIZE           },
    { PARAM_NON_VOLATILE,           MAX_MODEM_TAP_ACCOUNTS,     0,                              SM_OFFSET (TAPPagerIDStrings [0] [0]),TAP_PAGER_ID_STRING_SIZE  },
    { PARAM_NON_VOLATILE,           MAX_MODEM_TAP_ACCOUNTS,     0,                              SM_OFFSET (TAPServiceSettings [0]), sizeof (TAPServiceSettings_T)},
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (Termconfig),             sizeof (TermConfig_T)       },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (PPPProtocolOptions),     sizeof (PPPProtocolOptions_T)},
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (PPPPrimaryRMCPPort),     sizeof (INT16U)             },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (PPPSecondaryRMCPPort),   sizeof (INT16U)             },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (PPPLinkAuth),            1                           },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (CHAPName [0]),           MAX_MODEM_CHAP_NAME_SIZE + 0x80},
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (PPPACCM),                sizeof (PPPAccm_T)          },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (PPPSnoopACCM),           sizeof (PPPSnoopAccm_T)     },
    { PARAM_READ_ONLY,              0,                          0,                              SM_OFFSET (TotalPPPAcc),            1                           },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_PPP_ACCOUNTS,     0,                              SM_OFFSET (PPPAccDialStrSel [0]),   1                           },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_PPP_ACCOUNTS,     0,                              SM_OFFSET (PPPAccIPAddress [0] [0]),IP_ADDR_LEN         },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_PPP_ACCOUNTS,     0,                              SM_OFFSET (PPPAccUserNames [0] [0]),PPP_ACC_USER_NAME_DOMAIN_PASSWD_SIZE + 0x80 },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_PPP_ACCOUNTS,     0,                              SM_OFFSET (PPPAccUserDomain [0] [0]),PPP_ACC_USER_NAME_DOMAIN_PASSWD_SIZE + 0x80},
    { PARAM_VOLATILE_NON_VOLATILE | \
      PARAM_WRITE_ONLY,             MAX_MODEM_PPP_ACCOUNTS,     0,                              SM_OFFSET (PPPAccUserPasswd [0] [0]),PPP_ACC_USER_NAME_DOMAIN_PASSWD_SIZE + 0x80},
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_PPP_ACCOUNTS,     0,                              SM_OFFSET (PPPAccAuthSettings [0]), 1                           },
    { PARAM_VOLATILE_NON_VOLATILE,  MAX_MODEM_PPP_ACCOUNTS,     0,                              SM_OFFSET (PPPAccConnHoldTimes [0]),1                           },
    { PARAM_NON_VOLATILE,           0,                          0,                              SM_OFFSET (PPPUDPProxyIPHeadData),sizeof (PPPUDPProxyIPHeaderData_T)    },
    { PARAM_READ_ONLY,              0,                          0,                              SM_OFFSET (PPPUDPProxyTransmitBuffSize),2                       },
    { PARAM_READ_ONLY,              0,                          0,                              SM_OFFSET (PPPUDPProxyReceiveBuffSize),2                        },
    { PARAM_READ_ONLY,              0,                          0,                              SM_OFFSET (PPPRemoteConsoleIPAdd [0]),IP_ADDR_LEN           },
    {0,                             0,                          0,                                                                  0,                     0},
    {0,                             0,                          0,                                                                  0,                     0},
    {0,                             0,                          0,                                                                  0,                     0},
    {0,                             0,                          0,                                                                  0,                     0},
    {0,                             0,                          0,                                                                  0,                     0},
    { PARAM_NON_VOLATILE | CHECK_RSVD_BITS,             0,                          0,                              SM_OFFSET (BadPasswd),sizeof(BadPassword_T)          },

};


/**
 * @brief SerialModem Reserved Bits Validation Info Table
**/
const INT8U ReservedBitsTable [] =
{
    /*Param,    Length, ReservedBits Masks*/
     2,         5,      0xc8,0xc8,0xc8,0xc8,0xc8,
     3,         1,      0x78,
     4,         1,      0xf0,
     5,         5,      0xfc, 0xf0, 0x00, 0x00, 0x00,
     6,         1,      0xfc,
     7,         2,      0x0f,0xf0,
     8,         2,      0x80, 0xf0,
     54,       1,      0xFE,
};

const  INT8U SerialSupParam [] = 
{
    SET_IN_PROGRESS_PARAM,
    SET_AUTH_SUPPORT,
    SET_AUTH_ENABLE,
    SET_CONNECTION_MODE,
    SET_SESSION_INACTIVE_TIMEOUT,
    SET_CHANNEL_CALLBACK_CTRL,
    SET_SESSION_TERMINATION,
    SET_BAUD_RATE,
	SET_MUX_SWITCH_CTRL,
    SET_BAD_PWD_THRESHOLD
};

/*** Prototype Declarations ***/
static _FAR_ INT8U* getSMConfigAddr (_FAR_ const SMConfigTable_T* SMEntry,
                                                 INT8U            SetSelector,
                                                 INT8U            BlockSelector,
                                                 int			  BMCInst);
static _FAR_ ChannelUserInfo_T* GetCBChUserInfo (INT8U UserID, INT8U Channel,int BMCInst);
static  int     SetSerialPort (int BMCInst);


/*---------------------------------------
 * SetSerialModemConfig
 *---------------------------------------*/
int
SetSerialModemConfig (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SetSerialModemConfigReq_T* SetReq =
                              (_NEAR_ SetSerialModemConfigReq_T*) pReq;
    const  SMConfigTable_T*           SMEntry;
    _NEAR_ RsvdBitsTable_T*           RsvdEntry;
    INT8U                      SetSelector;
    INT8U                      BlockSelector;
    INT8U                      ReqStartOffset,i,tempdata;
    INT8U                      retValue = 0;
    _FAR_  INT8U*                     SetStart;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U  m_Serial_SetInProgress; /**< Contains setting Serial configuration status */

    IPMI_DBG_PRINT ("Set SerialModem Configuration Cmd\n");

	if(SetReq->ChannelNo & RESERVED_BITS_SETSERIALMODEMCONFIG_CH)
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

    /* Verify if the Channel number & param selector is valid */
    if (SetReq->ChannelNo != pBMCInfo->SERIALch)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    //Go for OEM defined set parameters
    if (g_PDKHandle[PDK_BEFORESETSERIALMODEMCONFIG] != NULL )
    {
        retValue = ((int(*)(INT8U *, INT8U, INT8U *,int))(g_PDKHandle[PDK_BEFORESETSERIALMODEMCONFIG]))(pReq, ReqLen, pRes, BMCInst);
        // if return value or response length < 1 then continue in the command
        // Otherwise return the response and return from this command
        if(retValue != 0)
        {
              return retValue;
        }
    }

    for(i=0; i < sizeof(SerialSupParam); i++)
    {
        if(SetReq->ParamSel == SerialSupParam[i])
        {
            break;
        }
    }

    if(i == sizeof(SerialSupParam))
    {
        *pRes = CC_PARAM_NOT_SUPPORTED;
        return sizeof (*pRes);
    }

    if (SetReq->ParamSel >= sizeof (SMConfigTable) / sizeof (SMConfigTable [0]))
    {
        *pRes = CC_PARAM_NOT_SUPPORTED;
        return sizeof (*pRes);
    }

    SMEntry = &(SMConfigTable [SetReq->ParamSel]);

    /*Adjust length if Set Selector Needed */
    if (0 != (SMEntry->MaxSetSelector & 0x7f))
    {
        ReqLen--;
    }

    /*Adjust length if Block Selector Needed */
    if (0 != (SMEntry->MaxSetSelector & BLOCK_SELECTOR_REQ))
    {
        ReqLen--;
    }

    /* Verify length */
    if (0 == (SMEntry->Size & 0x80))
    {
        if ((ReqLen - sizeof (SetSerialModemConfigReq_T)) != SMEntry->Size)
        {
            *pRes = CC_REQ_INV_LEN;
            return sizeof (*pRes);
        }
    }
    else
    {
        IPMI_DBG_PRINT_1 ("ReqLen = %d\n",ReqLen);
        if ((ReqLen - sizeof (SetSerialModemConfigReq_T)) > (SMEntry->Size & 0x7f))
        {
            *pRes = CC_REQ_INV_LEN;
            return sizeof (*pRes);
        }
    }

    LOCK_BMC_SHARED_MEM(BMCInst);
    m_Serial_SetInProgress = BMC_GET_SHARED_MEM(BMCInst)->m_Serial_SetInProgress;
    UNLOCK_BMC_SHARED_MEM(BMCInst);
    /*Process Set In Progress parameter */
    if (SetReq->ParamSel == SET_IN_PROGRESS_PARAM)
    {
        if((pReq[2] & RESERVED_BITS_SETSERIALMODEMCONFIG_SERINPRO) ||
            ((pReq[2] & SET_IN_PROGRESS_MASK) == SET_IN_PROGRESS_MASK))
        {
            *pRes = CC_INV_DATA_FIELD;
            return sizeof(*pRes);
        }

        /* Commit Write is optional and supported
         * only if rollback is supported */
        if ((SET_COMPLETE != pReq [2]) && (SET_IN_PROGRESS != pReq [2]))
        {
            *pRes = CC_PARAM_NOT_SUPPORTED;
            return sizeof (*pRes);
        }
        else if ((SET_IN_PROGRESS == m_Serial_SetInProgress) &&
                (SET_IN_PROGRESS == pReq [2]))
        {
            /*Set In Progress already Set */
            *pRes = CC_SET_IN_PROGRESS;
            return sizeof (*pRes);
        }

        LOCK_BMC_SHARED_MEM(BMCInst);
        BMC_GET_SHARED_MEM(BMCInst)->m_Serial_SetInProgress = pReq [2];
        UNLOCK_BMC_SHARED_MEM(BMCInst);
        *pRes           = CC_NORMAL;
        return sizeof (*pRes);
    }


    /* Get the param selector & Block Selector */
    if (0 != (SMEntry->MaxSetSelector & BLOCK_SELECTOR_REQ))
    {
        SetSelector     = pReq [2];
        BlockSelector   = pReq [3];
        ReqStartOffset  = sizeof (SetSerialModemConfigReq_T) +
                          sizeof (SetSelector) + sizeof (BlockSelector);
    }
    else if (0 == (SMEntry->MaxSetSelector & 0x7f))
    {
        SetSelector     = 0;
        BlockSelector   = 0;
        ReqStartOffset  = sizeof (SetSerialModemConfigReq_T);
    }
    else
    {
        SetSelector     = pReq [2];
        BlockSelector   = 0;
        ReqStartOffset  = sizeof (SetSerialModemConfigReq_T) +
                          sizeof (SetSelector);
    }

    /*Check Reserved Bits */

    i = 0;
    while ((0 != (SMEntry->ParamFlags & CHECK_RSVD_BITS)) &&
                 (i < sizeof (ReservedBitsTable)))
    {
        RsvdEntry   = (_NEAR_ RsvdBitsTable_T*)&ReservedBitsTable [i];
        i = i + sizeof (RsvdBitsTable_T);
        if (RsvdEntry->Param == SetReq->ParamSel)
        {
            INT8U j;

            for (j=0; j < RsvdEntry->Length; j++)
            {
                if (0 != (pReq [ReqStartOffset + j] & ReservedBitsTable [i + j]))
                {
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (*pRes);
                }
            }
            break;
        }

        i =  i + RsvdEntry->Length;
    }

    /* Verify the set selector & block selector */
    if ((0 != (SMEntry->MaxSetSelector & 0x7f)) &&
        ((SMEntry->MaxSetSelector & 0x7f) < SetSelector))
    {
        *pRes = CC_PARAM_OUT_OF_RANGE;
        return sizeof (*pRes);
    }

    if ((0 != SMEntry->MaxBlockSelector) &&
        (SMEntry->MaxBlockSelector <= BlockSelector))
    {
        *pRes = CC_PARAM_OUT_OF_RANGE;
        return sizeof (*pRes);
    }

    /* Check read only access */
    if (0 != (SMEntry->ParamFlags & PARAM_READ_ONLY))
    {
        *pRes = CC_ATTEMPT_TO_SET_RO_PARAM;
        return sizeof (*pRes);
    }

   /* Get the address where we want to set */
    SetStart = getSMConfigAddr (SMEntry , SetSelector, BlockSelector,BMCInst);

    switch(SetReq->ParamSel)
    {
        case SET_AUTH_SUPPORT:
            *pRes = CC_PEF_ATTEMPT_TO_SET_READ_ONLY_PARAM;
            return sizeof(*pRes);

        case SET_AUTH_ENABLE:
            /*Check for Valid bits */
            if ((pBMCInfo->SMConfig.AuthTypeSupport & pReq[2]) != pReq[2] ||
            	  (pBMCInfo->SMConfig.AuthTypeSupport & pReq[3]) != pReq[3] ||
            	  (pBMCInfo->SMConfig.AuthTypeSupport & pReq[4]) != pReq[4] ||
            	  (pBMCInfo->SMConfig.AuthTypeSupport & pReq[5]) != pReq[5] ||
            	  (pBMCInfo->SMConfig.AuthTypeSupport & pReq[6]) != pReq[6]
            	 )
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(*pRes);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (SetStart, (_FAR_ INT8U*) (pReq + ReqStartOffset), (SMEntry->Size & 0x7f));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case SET_CONNECTION_MODE:
            /*Check for Valid bits */
            if (GetBits (pReq[2], ENABLE_PPP_MODE_MASK) ||
                !GetBits (pReq[2], MODEM_DIRECT_CONNECT_MODE_MASK))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(*pRes);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (SetStart, (_FAR_ INT8U*) (pReq + ReqStartOffset), (SMEntry->Size & 0x7f));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case SET_SESSION_INACTIVE_TIMEOUT:
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (SetStart, (_FAR_ INT8U*) (pReq + ReqStartOffset), (SMEntry->Size & 0x7f));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case SET_CHANNEL_CALLBACK_CTRL:
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (SetStart, (_FAR_ INT8U*) (pReq + ReqStartOffset), (SMEntry->Size & 0x7f));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;
        
        case SET_SESSION_TERMINATION:
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (SetStart, (_FAR_ INT8U*) (pReq + ReqStartOffset), (SMEntry->Size & 0x7f));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;
    	
        case SET_BAUD_RATE:
            /*Check for Valid Baud Rate*/
            tempdata = GetBits (pReq[3], SERIAL_BAUD_RATE);
            if((tempdata > BAUD_RATE_115200)||(tempdata < BAUD_RATE_9600))
            {
            *pRes = CC_PARAM_OUT_OF_RANGE;
            return sizeof(*pRes);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (SetStart, (_FAR_ INT8U*) (pReq + ReqStartOffset), (SMEntry->Size & 0x7f));
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            /* if Set serial port setting, initlize the serial port */
            SetSerialPort (BMCInst);
            break;

        case SET_MUX_SWITCH_CTRL:
        	  /*Check for Valid Baud Rate*/
            if (GetBits (pReq[2], SWITCH_TO_BMC_ON_PPP) ||
                GetBits (pReq[3], CONNECT_ACTIVE_MSG_CALLBACK_MASK) ||
                GetBits (pReq[3], CONNECT_ACTIVE_MSG_DIRECT_MASK) ||
                GetBits (pReq[3], CONNECT_ACTIVE_MSG_RETRY_MASK))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(*pRes);
            }
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (SetStart, (_FAR_ INT8U*) (pReq + ReqStartOffset), (SMEntry->Size & 0x7f));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        case SET_BAD_PWD_THRESHOLD:
            LOCK_BMC_SHARED_MEM(BMCInst);
            _fmemcpy (SetStart, (_FAR_ INT8U*) (pReq + ReqStartOffset), (SMEntry->Size & 0x7f));
            UNLOCK_BMC_SHARED_MEM(BMCInst);
            break;

        default:
            *pRes=CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
   }

    FlushIPMI((INT8U*)&pBMCInfo->SMConfig,(INT8U*)&pBMCInfo->SMConfig,pBMCInfo->IPMIConfLoc.SMConfigAddr,
                      sizeof(SMConfig_T),BMCInst);

    *pRes = CC_NORMAL;
    return sizeof (*pRes);
}


/*---------------------------------------
 * GetSerialModemConfig
 *---------------------------------------*/
int
GetSerialModemConfig (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetSerialModemConfigReq_T* GetReq =
                              (_NEAR_ GetSerialModemConfigReq_T*) pReq;
    _NEAR_ GetSerialModemConfigRes_T* GetRes =
                              (_NEAR_ GetSerialModemConfigRes_T*) pRes;
    const  SMConfigTable_T*           SMEntry;
    _FAR_  INT8U*                     SetStart;
    INT8U                      ResStartOffset;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U  m_Serial_SetInProgress; /**< Contains setting Serial configuration status */
    INT8U                      retValue = 0;

	if(GetReq->ParamRevChannelNo & RESERVED_BITS_GETSERIALMODEMCONFIG_REVCH)
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

    /* Verify if the Channel number & param selector is valid */
    if ((GetReq->ParamRevChannelNo & 0x7f) != pBMCInfo->SERIALch)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }
    if (GetReq->ParamSel >= sizeof (SMConfigTable) / sizeof (SMConfigTable [0]))
    {
        *pRes = CC_PARAM_NOT_SUPPORTED;
        return sizeof (*pRes);
    }


    SMEntry = &(SMConfigTable [GetReq->ParamSel]);

    /* Verify length */
    if (ReqLen != sizeof (GetSerialModemConfigReq_T))
    {
        *pRes = CC_REQ_INV_LEN;
        return (*pRes);
    }

    GetRes->CompCode = CC_NORMAL;
    GetRes->ParamRev = PARAM_REVISION;

    /* Check if only parameter revision is required */
    if (0 != (GetReq->ParamRevChannelNo & GET_PARAM_REV_ONLY))
    {
        return sizeof (GetSerialModemConfigRes_T);
    }

    /* Get the param selector & Block Selector */
    if (0 != (SMEntry->MaxSetSelector & BLOCK_SELECTOR_REQ))
    {
        IPMI_DBG_PRINT ("Param With Set & BLock Selector \n");
        *(pRes + 2)    = GetReq->SetSel;
        *(pRes + 3)    = GetReq->BlockSel;
        ResStartOffset = sizeof (GetSerialModemConfigRes_T) +
                         sizeof (GetReq->SetSel) + sizeof (GetReq->BlockSel);
    }
    else if (0 == (SMEntry->MaxSetSelector & 0x7F))
    {
        IPMI_DBG_PRINT ("Param With No Set Selector \n");
        ResStartOffset  = sizeof (GetSerialModemConfigRes_T);
    }
    else
    {
        IPMI_DBG_PRINT ("Param With Set Selector \n");
        *(pRes + 2)    = GetReq->SetSel;
        ResStartOffset = sizeof (GetSerialModemConfigRes_T) +
                         sizeof (GetReq->SetSel);
    }

    /* Verify the set selector & block selector */
    *pRes = CC_PARAM_OUT_OF_RANGE;

    if (GetReq->ParamSel == INIT_STR_PARAM)
    {
        if (0 != GetReq->SetSel)
        {
            return sizeof (*pRes);
        }
        GetReq->SetSel   = GetReq->BlockSel;
        GetReq->BlockSel = 0;
        *(pRes + 2)      = GetReq->SetSel;
        ResStartOffset   = sizeof (GetSerialModemConfigRes_T) +
                           sizeof (GetReq->SetSel);
    }


    if ((0 != (SMEntry->MaxSetSelector & 0x7f)) &&
        ((SMEntry->MaxSetSelector & 0x7f) < GetReq->SetSel))
    {
        return sizeof (*pRes);
    }

    if ((0 != SMEntry->MaxBlockSelector) &&
        ((SMEntry->MaxBlockSelector <= GetReq->BlockSel) ||
         (0 == GetReq->BlockSel)))
    {
        return sizeof (*pRes);
    }

    if ((0 == (SMEntry->MaxSetSelector & 0x7f)) &&
        (0 != GetReq->SetSel))
    {
        return sizeof (*pRes);
    }

    if ((0 == SMEntry->MaxBlockSelector) &&
        (0 != GetReq->BlockSel))
    {
        return sizeof (*pRes);
    }

    if ((0 == GetReq->SetSel) && (GetReq->ParamSel == INIT_STR_PARAM))
    {
        return sizeof (*pRes);
    }

    /* Load Response */
    GetRes->CompCode = CC_NORMAL;

    /* Process Set In Progress parameter */
    if (GetReq->ParamSel == SET_IN_PROGRESS_PARAM)
    {
        LOCK_BMC_SHARED_MEM(BMCInst);
        m_Serial_SetInProgress = BMC_GET_SHARED_MEM(BMCInst)->m_Serial_SetInProgress;
        UNLOCK_BMC_SHARED_MEM(BMCInst);
        *(pRes + 2) = m_Serial_SetInProgress;
        return (sizeof (GetSerialModemConfigRes_T) + sizeof (m_Serial_SetInProgress));
    }

    /* Check Write only access */
    if (0 != (SMEntry->ParamFlags & PARAM_WRITE_ONLY))
    {
        *pRes = CC_WRITE_ONLY_PARAM;
        return sizeof (*pRes);
    }

    //Go for OEM defined get parameters
    if (g_PDKHandle[PDK_BEFOREGETSERIALMODEMCONFIG] != NULL )
    {
        retValue = ((int(*)(INT8U *, INT8U, INT8U *,int))(g_PDKHandle[PDK_BEFOREGETSERIALMODEMCONFIG]))(pReq, ReqLen, pRes, BMCInst);
        // if return value or response length < 1 then continue in the command
        // Otherwise return the response and return from this command
        if(retValue != 0)
        {
              return retValue;
        }
    }

    switch(GetReq->ParamSel)
    {
        case SET_AUTH_SUPPORT:
            break;
        case SET_AUTH_ENABLE:
            break;
        case SET_CONNECTION_MODE:
            break;
        case SET_SESSION_INACTIVE_TIMEOUT:
            break;
        case SET_CHANNEL_CALLBACK_CTRL:
            break;
        case SET_SESSION_TERMINATION:
            break;
        case SET_BAUD_RATE:
            break;
        case SET_MUX_SWITCH_CTRL:
            break;
        case SET_BAD_PWD_THRESHOLD:
            break;
        case SET_TMODE_CFG:
            break;
        default:
            *pRes=CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
    }

    /* Get the address where we want to set */
    SetStart = getSMConfigAddr (SMEntry, GetReq->SetSel, GetReq->BlockSel, BMCInst);

    LOCK_BMC_SHARED_MEM(BMCInst);
    _fmemcpy ( (pRes + ResStartOffset), SetStart,  (SMEntry->Size & 0x7f));
    UNLOCK_BMC_SHARED_MEM(BMCInst);

    return ( (SMEntry->Size & 0x7f) + ResStartOffset);
}


/*---------------------------------------
 * getSMConfigAddr
 *--------------------------------------*/
static _FAR_ INT8U*
getSMConfigAddr (_FAR_ const SMConfigTable_T* SMEntry,
                                INT8U            SetSelector,
                                INT8U            BlockSelector,
                                int			  BMCInst)
{
    _FAR_ INT8U*        SetStart;
    INT8U         Size;
    INT8U         MaxBlockSel;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    /* Get the NVRAM PM config address */
    SetStart = (_FAR_ INT8U*) &pBMCInfo->SMConfig;
    Size     = SMEntry->Size & 0x7f;

    if (0 == SMEntry->MaxBlockSelector)
    {
        MaxBlockSel = 1;
    }
    else
    {
        MaxBlockSel = SMEntry->MaxBlockSelector;
    }

    if (0 != (SMEntry->ParamFlags & PARAM_VOLATILE_NON_VOLATILE))
    {
        if (0 == SetSelector)
        {
            /*Get Shared Memory Info */
            SetStart = (_FAR_ INT8U*)&BMC_GET_SHARED_MEM (BMCInst)->SMConfig;
            SetStart = SetStart + SMEntry->Offset +
                       (Size * ((SetSelector * MaxBlockSel) + BlockSelector));
            return SetStart;
        }
        else
        {
            SetSelector--;
            if (0 != BlockSelector)
            {
                BlockSelector--;
            }
        }
    }

    SetStart = SetStart + SMEntry->Offset +
               (Size * ((SetSelector * MaxBlockSel) + BlockSelector));

    return SetStart;
}


/*---------------------------------------
 * CallBack
 *---------------------------------------*/
int
CallBack (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_  CallbackReq_T*  CallBackReq = (_NEAR_ CallbackReq_T*) pReq;
    _FAR_   SMConfig_T*     pVSMConfig  =
                 &(((_FAR_ BMCSharedMem_T*) BMC_GET_SHARED_MEM (BMCInst))->SMConfig);
    _FAR_   ChannelInfo_T*  pChannelInfo;
    _FAR_   DestInfo_T*     pDestInfo;
    INT8U           DestType,curchannel;
    MsgPkt_T        MsgPkt;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];			
    _FAR_   SMConfig_T*     pNVSMConfig = &pBMCInfo->SMConfig;
    INT32U CurSesID;

	if(CallBackReq->ChannelNum & RESERVED_BITS_CALLBACK_CH)
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

	if(CallBackReq->DestSel & RESERVED_BITS_CALLBACK_DESTSEL)
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    /* Check if Session Already exist */
    if ((pBMCInfo->SERIALch!= (curchannel  & 0xF)) &&
        (TRUE == BMC_GET_SHARED_MEM (BMCInst)->SerialSessionActive))
    {
        *pRes = CC_CALLBACK_REJ_SESSION_ACTIVE;
        return sizeof (*pRes);
    }

    CallBackReq->DestSel &= 0x0f;

    /* Check the validity of Destination Selector */
    if (CallBackReq->DestSel > pNVSMConfig->TotalAlertDest)
    {
        *pRes = CC_DEST_UNAVAILABLE;
        return sizeof (*pRes);
    }

    if (0 == CallBackReq->DestSel)
    {
        /* Destination Info is volatile */
        pDestInfo = (_FAR_ DestInfo_T*) &pVSMConfig->DestinationInfo [0];
    }
    else
    {
        /* Destination Info is non-volatile */
        pDestInfo = (_FAR_ DestInfo_T*) &pNVSMConfig->DestinationInfo [CallBackReq->DestSel - 1];
    }

    /*Check if Destination if Configured & Enabled for CALLBACK */
    *pRes     = CC_INV_DATA_FIELD;
    DestType  = pDestInfo->DesType & MODEM_CFG_DEST_INFO_DEST_TYPE_MASK;


    if ((BASIC_MODE_CALLBACK != DestType) && (PPP_MODE_CALLBACK != DestType))
    {
        return sizeof (*pRes);
    }

    if (0 == (pNVSMConfig->ChannelCallBackCtrl.CallBackEnable & IPMI_CALLBACK_MASK))
    {
        return sizeof (*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    /* If Current Channel is Session Based, Check is User is allowed for CallBack */
    pChannelInfo = getChannelInfo (curchannel  & 0xF, BMCInst);
    if(NULL == pChannelInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        *pRes = CC_INV_DATA_FIELD;
        return	sizeof (*pRes);
    }

    /* Disable Ping during CallBack */
    pNVSMConfig->MUXSwitchCtrl.Data2  &=  ~DIRECT_PING_ENABLE_MASK;


    if (pChannelInfo->SessionSupport != SESSIONLESS_CHANNEL)
    {
        _FAR_ SessionInfo_T*     pSessionInfo;
        _FAR_ SessionTblInfo_T*  pSessionTblInfo =
                         &pBMCInfo->SessionTblInfo;
        _FAR_ ChannelUserInfo_T* pChUserInfo;
        INT8U              Index;

        IPMI_DBG_PRINT_1 ("Session Less ch  - %d\n",curchannel  & 0xF);

        OS_THREAD_TLS_GET(g_tls.CurSessionID,CurSesID);
        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex,WAIT_INFINITE);
        pSessionInfo = getSessionInfo (SESSION_ID_INFO,&CurSesID, BMCInst);
        if(pSessionInfo == NULL)
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            return sizeof (*pRes);
        }
        pChUserInfo = getChUserIdInfo (pSessionInfo->UserId,&Index,
                                       pChannelInfo->ChannelUserInfo, BMCInst);
        if(pChUserInfo == NULL)
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            return sizeof (*pRes);
        }

        if (0 == (pChUserInfo->UserCallbackCapabilities & 0x01))
        {
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
            return sizeof (*pRes);
        }

        /* For Serial Channel Existing Session should be deleted */
        if ((pBMCInfo->SERIALch!= (curchannel  & 0xF)) &&
            (TRUE == BMC_GET_SHARED_MEM (BMCInst)->SerialSessionActive))
        {
            for (Index=0; Index < pBMCInfo->IpmiConfig.MaxSession; Index++)
            {
                if (pBMCInfo->SERIALch == pSessionTblInfo->SessionTbl [Index].Channel)
                {
                    pSessionTblInfo->SessionTbl [Index].TimeOutValue = 0;
                }
            }
        }
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
    /* Request to Initiate CallBack */
    MsgPkt.Channel  = pBMCInfo->SERIALch;
    MsgPkt.Param    = SERIAL_INIT_CALLBACK_REQUEST;
    MsgPkt.Size     = sizeof (CallBackReq->DestSel);
    MsgPkt.Data [0] = CallBackReq->DestSel;

    IPMI_DBG_PRINT ("Posting CallBack Request\n");
    if( 0 != PostMsg (&MsgPkt,SERIAL_IFC_Q,BMCInst ))
    {
           IPMI_WARNING ("SerialModem.c: Unable to post messages to hSerialIfc_Q\n");
    }    

    *pRes = CC_NORMAL;

    return sizeof (*pRes);
}


/*---------------------------------------
 * SetUserCallBackOptions
 *---------------------------------------*/
int
SetUserCallBackOptions (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SetUserCallbackReq_T *Req = (_NEAR_ SetUserCallbackReq_T*) pReq;
    _FAR_  ChannelUserInfo_T    *pChUserInfo;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if((Req->UserID & RESERVED_BITS_SETUSERCALLBACKOPT_USRID) ||
        ((Req->UserID & 0x3F) == RESERVED_USER_ID))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    if(Req->ChannelNum & RESERVED_BITS_SETUSERCALLBACKOPT_CH)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    if(Req->UserCallbackCapabilities & RESERVED_BITS_SETUSERCALLBACKOPT_USRCALLBAKCAP)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    if(Req->CBCPNegOptions & RESERVED_BITS_SETUSERCALLBACKOPT_CBCNEGOPT)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pChUserInfo = GetCBChUserInfo (Req->UserID, Req->ChannelNum, BMCInst);

    *pRes = CC_INV_DATA_FIELD;

    /*Check if the user info exits */
    if (NULL == pChUserInfo)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        return sizeof (*pRes);
    }

    /*Set CallBack Options */
    pChUserInfo->UserAccessCallback       = (Req->UserCallbackCapabilities &
                                             IPMI_CALLBACK_MASK);
    pChUserInfo->UserAccessCBCPCallback   = (Req->UserCallbackCapabilities &
                                             CBCP_CALLBACK_MASK);
    pChUserInfo->UserCallbackCapabilities = Req->UserCallbackCapabilities;
    pChUserInfo->CBCPNegOptions           = Req->CBCPNegOptions;
    pChUserInfo->CallBack1                = Req->CallBack1;
    pChUserInfo->CallBack2                = Req->CallBack2;
    pChUserInfo->CallBack3                = Req->CallBack3;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    *pRes = CC_NORMAL;
    return sizeof (*pRes);
}


/*---------------------------------------
 * SetUserCallBackOptions
 *---------------------------------------*/
int
GetUserCallBackOptions (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_   GetUserCallbackReq_T* Req = ( _NEAR_ GetUserCallbackReq_T*) pReq;
    _NEAR_   GetUserCallbackRes_T* Res = ( _NEAR_ GetUserCallbackRes_T*) pRes;
    _FAR_    ChannelUserInfo_T*    pChUserInfo;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

	if(Req->UserID & RESERVED_BITS_GETUSERCALLBACKOPTIONS_USRID)
	{
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->ChUserMutex,WAIT_INFINITE);
    pChUserInfo  = GetCBChUserInfo (Req->UserID , Req->ChannelNum, BMCInst);

    *pRes = CC_INV_DATA_FIELD;

    /*Check if the user info exits */
    if (NULL == pChUserInfo  )
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);
        return sizeof (*pRes);
    }

    /*Set CallBack Options */
    Res->CompletionCode           = CC_NORMAL;
    Res->UserCallbackCapabilities = pChUserInfo->UserCallbackCapabilities ;
    Res->CBCPNegOptions           = pChUserInfo->CBCPNegOptions;
    Res->CallBack1                = pChUserInfo->CallBack1;
    Res->CallBack2                = pChUserInfo->CallBack2;
    Res->CallBack3                = pChUserInfo->CallBack3;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->ChUserMutex);

    return sizeof (GetUserCallbackRes_T);
}


/*---------------------------------------
 * GetCBChUserInfo
 *---------------------------------------*/
static _FAR_ ChannelUserInfo_T*
GetCBChUserInfo (INT8U UserID, INT8U Channel,int BMCInst)
{
    _FAR_   ChannelInfo_T* pChannelInfo;
    INT8U          Index;

    /*Get Channel Info */
    pChannelInfo = getChannelInfo (Channel, BMCInst);
    if (NULL == pChannelInfo )
    {
        return NULL;
    }

    /*Get the particular user entitled for callback */
    return getChUserIdInfo (UserID,&Index,pChannelInfo->ChannelUserInfo, BMCInst);
}


/*---------------------------------------
 * SetSerialModemMUX
 *---------------------------------------*/
int
SetSerialModemMUX (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SetMuxReq_T* Req         = (_NEAR_   SetMuxReq_T*) pReq;
    _NEAR_ SetMuxRes_T* Res         = (_NEAR_   SetMuxRes_T*) pRes;
     INT8U        TSettings;
    ChannelInfo_T* pChannelInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_  SMConfig_T*  pNVSMConfig = &(pBMCInfo->SMConfig);
    int SerialStatus = 0xFF;

    Res->CompCode = CC_INV_DATA_FIELD;

	if(Req->ChannelNo & RESERVED_BITS_SETSERIALMODEMMUX_CH) return sizeof(*pRes);
	if(Req->MuxSettingReq & RESERVED_BITS_SETSERIALMODEMMUX_MUXSETREQ) return sizeof(*pRes);
	if(Req->MuxSettingReq > REQ_ALLOW_MUX_TO_BMC) return sizeof(*pRes);

    if (Req->ChannelNo != pBMCInfo->SERIALch)
    {
        return sizeof(*pRes);
    }

    TSettings = BMC_GET_SHARED_MEM (BMCInst)->SerialMuxSetting;
    /* No Session or Alert  In progress */
    TSettings = TSettings & MUX_SESSION_ALERT_INACTIVE_MASK;
    if (TRUE == BMC_GET_SHARED_MEM (BMCInst)->SerialSessionActive)
    {
        TSettings |= MUX_SESSION_ACTIVE_MASK;
    }

    /* Accept Request */
    TSettings = TSettings | MUX_REQ_ACCEPTED_MASK;

    Res->CompCode = CC_NORMAL;

    pChannelInfo = getChannelInfo(pBMCInfo->SERIALch, BMCInst);
    if(NULL == pChannelInfo)
    {
        Res->CompCode = CC_INV_DATA_FIELD;
        return	sizeof (*pRes);
    }

    /*Check if Serial Port Sharing Enabled */
    if ((0 == (pNVSMConfig->MUXSwitchCtrl.Data2 & SERIAL_PORT_SHARING_ENABLED)) ||
        (CHANNEL_ALWAYS_AVAILABLE == pChannelInfo->AccessMode))
    {

        if(Req->MuxSettingReq==0x0)
        {
            Res->MuxSetting = TSettings& MUX_REQ_REJECTED_MASK;
            return sizeof (SetMuxRes_T);
        }
        else
        {
            Res->CompCode = CC_COULD_NOT_PROVIDE_RESP;
            return sizeof(*pRes);
        }
    }

    switch (Req->MuxSettingReq)
    {
        case REQ_CUR_MUX_SETTING:
            break;

        case REQ_MUX_TO_SYS:

            if (0 == (REQ_MUX_TO_SYS_MASK & TSettings))
            {
                IPMI_DBG_PRINT ("SYS HOLDS EMP\n");
                if(g_PDKHandle[PDK_SWITCHEMPMUX] != NULL)
                {
                    ((void(*)(INT8U,int))g_PDKHandle[PDK_SWITCHEMPMUX]) (MUX_2_SYS,BMCInst);
                }
                TSettings = (TSettings & SET_MUX_TO_SYS_MASK);
            }
            else
            {
                TSettings &= MUX_REQ_REJECTED_MASK;
            }
            break;

        case REQ_MUX_TO_BMC:
            /*Check if Disable mux switch to BMC on DCD loss */
             if ((pNVSMConfig->MUXSwitchCtrl.Data1 & ALLOW_MUX_SWITCH_ON_DCD_LOSS) == 0)
            {
                if (ioctl (pBMCInfo->SerialConfig.serial_fd, TIOCMGET, &SerialStatus) < 0)
                {
                     IPMI_WARNING("Error in getting TIOCMGET serial status\n");
                }
                else if ((SerialStatus & TIOCM_CAR) == 0)
                {
                    Res->CompCode = CC_COULD_NOT_PROVIDE_RESP;
                    return sizeof(*pRes);
                }
            }
            if (0 == (REQ_MUX_TO_BMC_MASK & TSettings))
            {
                IPMI_DBG_PRINT ("BMC HOLDS EMP\n");
                if(g_PDKHandle[PDK_SWITCHEMPMUX] != NULL)
                {
                    ((void(*)(INT8U,int))g_PDKHandle[PDK_SWITCHEMPMUX]) (MUX_2_BMC,BMCInst);
                }
                TSettings = (TSettings | SET_MUX_TO_BMC_MASK);
            }
            else
            {
                TSettings &= MUX_REQ_REJECTED_MASK;
            }
            break;

        case REQ_MUX_FORCE_TO_SYS:

            IPMI_DBG_PRINT ("SYS HOLDS EMP\n");
            if(g_PDKHandle[PDK_SWITCHEMPMUX] != NULL)
            {
                ((void(*)(INT8U,int))g_PDKHandle[PDK_SWITCHEMPMUX]) (MUX_2_SYS,BMCInst);
            }
            TSettings = (TSettings & SET_MUX_TO_SYS_MASK);
            break;

        case REQ_MUX_FORCE_TO_BMC:
            /*Check if Disable mux switch to BMC on DCD loss */
            if ((pNVSMConfig->MUXSwitchCtrl.Data1 & ALLOW_MUX_SWITCH_ON_DCD_LOSS) == 0)
            {
                if (ioctl (pBMCInfo->SerialConfig.serial_fd, TIOCMGET, &SerialStatus) < 0)
                {
                     IPMI_WARNING("Error in getting TIOCMGET serial status\n");
                }
                else if ((SerialStatus & TIOCM_CAR) == 0)
                {
                    Res->CompCode = CC_COULD_NOT_PROVIDE_RESP;
                    return sizeof(*pRes);
                }
            }
            IPMI_DBG_PRINT ("BMC HOLDS EMP\n");
            if(g_PDKHandle[PDK_SWITCHEMPMUX] != NULL)
            {
                ((void(*)(INT8U,int))g_PDKHandle[PDK_SWITCHEMPMUX]) (MUX_2_BMC,BMCInst);
            }
            TSettings = (TSettings | SET_MUX_TO_BMC_MASK);
            break;

        case REQ_BLOCK_MUX_TO_SYS:

            TSettings = (TSettings | BLOCK_MUX_TO_SYS_MASK);
            break;

        case REQ_ALLOW_MUX_TO_SYS:

            TSettings = (TSettings & ALLOW_MUX_TO_SYS_MASK);
            break;

        case REQ_BLOCK_MUX_TO_BMC:

            TSettings = (TSettings | BLOCK_MUX_TO_BMC_MASK);
            break;

        case REQ_ALLOW_MUX_TO_BMC:

            TSettings = (TSettings & ALLOW_MUX_TO_BMC_MASK);
            break;

        default:

        return sizeof (*pRes);
    }

    Res->MuxSetting = TSettings;
    BMC_GET_SHARED_MEM (BMCInst)->SerialMuxSetting = TSettings;
    IPMI_DBG_PRINT_1 ("SerialMUXSetting = %X\n",TSettings);

    return sizeof (SetMuxRes_T);
}


/*---------------------------------------
 * SerialModemConnectActive
 *---------------------------------------*/
int
SerialModemConnectActive (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SerialModemActivePingReq_T* Req = (_NEAR_ SerialModemActivePingReq_T*) pReq;

    if(Req->SessionState & RESERVED_BITS_SERIALMODEMCONNECTACTIVE)
    {
    	*pRes = CC_INV_DATA_FIELD;
    	return sizeof(*pRes);
    }

    *pRes = CC_NORMAL;

    if ((Req->SessionState != 0)    && /* No Session State      */
        (Req->SessionState != 1)    && /* Session State Active  */
        (Req->SessionState != 2)    && /* MUX Switch to SYS     */
        (Req->IPMIVersion  != 0x51))   /* IPMIVersion 1.5       */
    {
        *pRes = CC_INV_DATA_FIELD;
    }

    return sizeof (*pRes);
}


/*---------------------------------------
 * SerialModemPingTask
 *---------------------------------------*/
void
SerialModemPingTask ( int BMCInst )
{
      MsgPkt_T     MsgPkt;
    _FAR_ static INT8U MuxSwitchToSys;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_ SMConfig_T*  pNVSMConfig = &pBMCInfo->SMConfig;

    /* Check if Serial Port Sharing Enabled & Mux is switched to BMC */
    if (0 == (pNVSMConfig->MUXSwitchCtrl.Data2 & SERIAL_PORT_SHARING_ENABLED))
    {
        return;
    }

    /* Check if Serial Ping Message Enabled */
    if (0 == (pNVSMConfig->MUXSwitchCtrl.Data2 & DIRECT_PING_ENABLE_MASK))
    {
        return;
    }

    /* Update Session State */
    if (0 == (BMC_GET_SHARED_MEM (BMCInst)->SerialMuxSetting & SET_MUX_TO_BMC_MASK))
    {
        /* Ping should be generated Only once before Mux switch to System */
        if (1 == MuxSwitchToSys )
        {
            return;
        }

        MsgPkt.Data [0] = MUX_SWITCHED_TO_SYS;
        MuxSwitchToSys  = 1;
    }
    else
    {
        /* Check for periodical ping message active flag */
        if (0 == (pNVSMConfig->MUXSwitchCtrl.Data2 & PING_MSG_ACTIVE_BIT))
        {
            return;
        }

        MsgPkt.Data [0] = SESSION_STATE_INACTIVE;
        MuxSwitchToSys  = 0;

        if (TRUE == BMC_GET_SHARED_MEM (BMCInst)->SerialSessionActive)
        {
            MsgPkt.Data [0] = SESSION_STATE_ACTIVE;
        }
    }

    /* Request to give Ping Message */
    MsgPkt.Channel  = pBMCInfo->SERIALch;
    MsgPkt.Param    = SERIAL_PING_REQUEST;
    MsgPkt.Size     = 1;


    /* Post Ping Request */
    if( 0 != PostMsg (&MsgPkt,SERIAL_IFC_Q ,BMCInst))
    {
           IPMI_WARNING ("SerialModem.c: Unable to post messages to hSerialIfc_Q\n");
    }    
    
}


/*---------------------------------------
 * GetTAPResponseCodes
 *---------------------------------------*/
int
GetTAPResponseCodes (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetTAPResCodeReq_T* Req      = (_NEAR_ GetTAPResCodeReq_T*) pReq;
    _NEAR_ GetTAPResCodeRes_T* Res      = (_NEAR_ GetTAPResCodeRes_T*) pRes;
    _FAR_  TAPResCode_T*       TAPCodes = &(((_FAR_ BMCSharedMem_T*) BMC_GET_SHARED_MEM (BMCInst))->TAPRes);
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    IPMI_DBG_PRINT ("Get TAP Respone Code CMD\n");

    if(Req->ChannelNo & RESERVED_BITS_GETTAPRESPONSECODES_CH)
    {
    	*pRes = CC_INV_DATA_FIELD;
    	return sizeof(*pRes);
    }

    /* Check Channel No */
    if (Req->ChannelNo != pBMCInfo->SERIALch)
    {
        Res->CompCode = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    /*Load Response Codes*/
    Res->CompCode = CC_NORMAL;
    LOCK_BMC_SHARED_MEM(BMCInst);
    _fmemcpy (&pRes [1],TAPCodes,sizeof (TAPResCode_T));
    UNLOCK_BMC_SHARED_MEM(BMCInst);

    return sizeof (GetTAPResCodeRes_T);
}


/**
 * SetSerialPort
 * @brief Initilize the Serial port
 **/

int
SetSerialPort (int BMCInst)
{
    INT8U   BaudRate;
    //INT8U   FlowCtrl;
    //int     status;
    int		fd;
    struct  termios tty_struct;
    _FAR_  BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    if ((fd = open(pBMCInfo->IpmiConfig.pSerialPort, O_RDONLY)) < 0)
    {
        IPMI_WARNING ("Can't open serial port..%s\n",strerror(errno));
        return -1;
    }

    /* Get the default Baudrate & FlowControl from Serial Modem configureation */
    BaudRate = pBMCInfo->SMConfig.IpmiMsgCommSet.BitRate;
    //FlowCtrl = pBMCInfo->SMConfig.IpmiMsgCommSet.FlowCtrl;

    tcgetattr(fd,&tty_struct);   			   /* get termios structure */

    switch (BaudRate) 
    {
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
            IPMI_ERROR ("SerialIfc.c : Invalid baud rate\n");
    }

    tty_struct.c_cflag |= CS8;              /* Set 8bits/charecter          */
    tty_struct.c_cflag &= ~CSTOPB;          /* set framing to 1 stop bits   */
    tty_struct.c_cflag &= ~(PARENB);        /* set parity to NONE           */
    tty_struct.c_iflag &= ~(INPCK);

    /* set the new attributes in the tty driver */
    tcsetattr(fd, TCSANOW, &tty_struct);

    close (fd);

    return 0;
}


