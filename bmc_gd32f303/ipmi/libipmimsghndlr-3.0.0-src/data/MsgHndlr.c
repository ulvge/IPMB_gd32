/****************************************************************
****************************************************************
**                                                            **
**    (C)Copyright 2020-2020        **

**                                                            **
****************************************************************
*****************************************************************
*
* MsgHndlr.c
* Message Handler.
*
* Author: Govind Kothandapani <govindk@ami.com>
*
*****************************************************************/
#define ENABLE_DEBUG_MACROS 0
#define UNIMPLEMENTED_AS_FUNC
//#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include "Types.h"
#include "ipmi_common.h"
//#include "Debug.h"
// #include "debug_print.h"
#include "Support.h"
//#include "Message.h"
#include "MsgHndlr.h"
#include "IPMI_App.h"
//#include "IPMI_Main.h"
//#include "IPMI_AMI.h"
//#include "IPMI_HPM.h"
//#include "IPMIDefs.h"
#include "AMI.h"
#include "App.h"
#include "Bridge.h"
#include "Chassis.h"
#include "Storage.h"
#include "SensorEvent.h"
#include "Oem.h"
#include "DeviceConfig.h"
//#include "ChassisDevice.h"
//#include "ChassisCtrl.h"
//#include "SDR.h"
//#include "SEL.h"
//#include "FRU.h"
//#include "Sensor.h"
//#include "SensorMonitor.h"
//#include "FFConfig.h"
//#include "NVRAccess.h"
//#include "Platform.h"
//#include "PDKCmds.h"
//#include "AMIDevice.h"
//#include "PendTask.h"
//#include "PEF.h"
//#include "SensorAPI.h"
#include "IPMIConf.h"
//#include "IPMBIfc.h"
///*added for the execution of the "Power Restore Policy"	*/
//#include "PMConfig.h"
//#include "GroupExtn.h"
//#include "DCMDevice.h"
//#include "PDKAccess.h"
//#include "IfcSupport.h"
//#include "Badpasswd.h"
//#include "PDKCmdsAccess.h"

//#include "featuredef.h"
//#include "PDKBridgeMsg.h"
//#include "BridgeMsgAPI.h"
//#include "cmdselect.h"
#include "LANIfc.h"
#include "OSPort.h"
#include "bsp_i2c.h"
#include "bsp_gpio.h"
#include "main.h"

#define UNIMPLEMENTED UnImplementedFunc

// extern pthread_mutex_t MsgHndlrMutex;

/*--------------------------------------------------------------------
* Global Variables
*--------------------------------------------------------------------*/
BMCInfo_t g_BMCInfo;

static TaskHandle_t xHandleTaskResponseDatWrite = NULL;
xQueueHandle ResponseDatMsg_Queue = NULL;

xQueueHandle RecvDatMsg_Queue = NULL;
xQueueHandle RecvForwardI2CDatMsg_Queue = NULL;

/*-----------------------------------------------------------------------------
* Function Prototypes
*-----------------------------------------------------------------------------*/
static void device_addr_set(void);

/*--------------------------------------------------------------------
* Module Variables
*--------------------------------------------------------------------*/
const DisableMsgFilterTbl_T m_DisableMsgFilterTbl[] =
    {
        /* NET_FUN                                      Command */
        /*----------------- BMC Device and Messaging Commands ------------------*/
        {NETFN_APP, CMD_GET_CH_AUTH_CAP},
        {NETFN_APP, CMD_GET_CH_AUTH_CAP},
        {NETFN_APP, CMD_GET_SESSION_CHALLENGE},
        {NETFN_APP, CMD_ACTIVATE_SESSION},
        {NETFN_APP, CMD_SET_SESSION_PRIV_LEVEL},
        {NETFN_APP, CMD_CLOSE_SESSION},
        {NETFN_APP, CMD_GET_SESSION_INFO},
        /*------------------------ IPMI 2.0 specific Commands ------------------*/
        {NETFN_APP, CMD_ACTIVATE_PAYLOAD},
        {NETFN_APP, CMD_DEACTIVATE_PAYLOAD},
        {NETFN_APP, CMD_GET_PAYLD_ACT_STATUS},
        {NETFN_APP, CMD_GET_PAYLD_INST_INFO},
        {NETFN_APP, CMD_SET_USR_PAYLOAD_ACCESS},
        {NETFN_APP, CMD_GET_USR_PAYLOAD_ACCESS},
        {NETFN_APP, CMD_GET_CH_PAYLOAD_SUPPORT},
        {NETFN_APP, CMD_GET_CH_PAYLOAD_VER},
        {NETFN_APP, CMD_GET_CH_OEM_PAYLOAD_INFO},
        {NETFN_APP, CMD_SUS_RES_PAYLOAD_ENCRYPT},
        /*------------------------ IPMI 2.0 SOL Commands ------------------*/
        {NETFN_TRANSPORT, CMD_GET_SOL_CONFIGURATION},
        {NETFN_TRANSPORT, CMD_SET_SOL_CONFIGURATION},

        {NETFN_UNKNOWN, 0}, /* Indicate the end of array */
};

//INT8U ExtNetFnMap[MAX_NUM_BMC][MAX_NETFN];

static const MsgHndlrTbl_T m_MsgHndlrTbl[15] =
    {
        {NETFN_CHASSIS, g_Chassis_CmdHndlr},//0
        {NETFN_BRIDGE, g_Bridge_CmdHndlr},//0
        {NETFN_SENSOR, g_SensorEvent_CmdHndlr},
        {NETFN_APP, g_App_CmdHndlr},
        {NETFN_STORAGE, g_Storage_CmdHndlr},
        {NETFN_OEM, g_Oem_CmdHndlr},
        //    { NETFN_TRANSPORT,              g_Config_CmdHndlr               },
        //    { NETFN_AMI,                    (CmdHndlrMap_T*)g_AMI_CmdHndlr  },
};

//GroupExtnMsgHndlrTbl_T m_GroupExtnMsgHndlrTbl [10] =
//{

//};

const FlashModeFilterTbl_T m_FlashModeFilterTbl[] =
    {
        {NETFN_APP, CMD_GET_CH_AUTH_CAP},
        {NETFN_APP, CMD_GET_SESSION_CHALLENGE},
        {NETFN_APP, CMD_ACTIVATE_SESSION},
        {NETFN_APP, CMD_SET_SESSION_PRIV_LEVEL},
        {NETFN_APP, CMD_CLOSE_SESSION},
};

void MsgHndlrInit()
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    memcpy(pBMCInfo->MsgHndlrTbl, m_MsgHndlrTbl, sizeof(m_MsgHndlrTbl));
}

static void vTaskResponseDatWrite(void *pvParameters)
{
    char buff[sizeof(MsgPkt_T)];
    MsgPkt_T *ResMsg = (MsgPkt_T *)buff;
    ResponseDatMsg_Queue = xQueueCreate(10, sizeof(MsgPkt_T));
    RecvForwardI2CDatMsg_Queue = xQueueCreate(10, sizeof(MsgPkt_T));

    while (1)
    {
        xQueueReceive(ResponseDatMsg_Queue, buff, portMAX_DELAY);

        switch(ResMsg->Param)
        {
        case IPMI_REQUEST:
        case NORMAL_RESPONSE:
            ipmb_write(ResMsg->Data, ResMsg->Size);
            // LOG_RAW("I2C write->:");
            break;
        case SERIAL_REQUEST:
            serial_write(ResMsg->Data, ResMsg->Size);
            break;
        case LAN_REQUEST:
            // LOG_RAW("LAN write->:");
            //SendLANPkt(ResMsg);
            break;
        default:
            break;
        }

        for(int i=0; i<ResMsg->Size; i++)
        {
             LOG_RAW("%02x ", ResMsg->Data[i]);
        }
        LOG_RAW("\r\n");
    }
}

/**
*@fn MsgCoreHndlr
*@brief Message Handler Task
*       Starting Main function of MsgHndlr task
*/
void *MsgCoreHndlr(void *pArg)
{
    MsgPkt_T Res;
    char buff[sizeof(MsgPkt_T)];
    MsgPkt_T* Req = (MsgPkt_T*)buff;
    BaseType_t err = pdFALSE;
    __attribute__((unused)) int i;

    MsgHndlrInit();
    device_addr_set();

    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ==                                             
		xTaskCreate(vTaskResponseDatWrite, "Task ResponseDatWrite", 256, NULL, 24, &xHandleTaskResponseDatWrite)) {
        LOG_E("vTaskResponseDatWrite create task ERR!");
    }
    RecvDatMsg_Queue = xQueueCreate(1, sizeof(MsgPkt_T));  //10
    if (RecvDatMsg_Queue ==  NULL) {
        LOG_E("RecvDatMsg_Queue create ERR!");
    }

    while (1)
    {
        if (RecvDatMsg_Queue == NULL)
        {
            vTaskDelay(1000);
            continue;
        }
        err = xQueueReceive(RecvDatMsg_Queue, buff, portMAX_DELAY);
        if (err == pdFALSE)
        {
            LOG_E("xQueueReceive get msg ERR!");
            continue;
        }
        switch(Req->Param)
        {
        case IPMI_REQUEST:
            ProcessIPMIReq(Req, &Res);
            break;
        case SERIAL_REQUEST:
         // ProcessSerPortReq ProcessSerialMessage ProcessIPMIReq
            if (ProcessSerialReq(Req, &Res) == false){
				continue;
			}
            break;
        case LAN_REQUEST:
            //ProcessLANReq(Req, &Res);
            break;
        default :
            break;
        }

        err = xQueueSend(ResponseDatMsg_Queue, (char*)&Res, 10);
        if (err == pdFALSE)
        {
            LOG_E("xQueueSend send msg ERR!");
        }

        // LOG_RAW("len:%d\n",Req.Size);
        // for (i = 0; i < Req.Size; i++)
        // {
        //     LOG_RAW("%02x ", Req.Data[i]);
        // }
        // LOG_RAW("\n");
    }
}

/**
*@fn ProcessIPMIReq
*@brief Processes the requested IPMI command
*@param pReq Request of the command
*@param pRes Response for the requested command
*@return none
*/
void ProcessIPMIReq(_NEAR_ MsgPkt_T *pReq, _NEAR_ MsgPkt_T *pRes)
{
    CmdHndlrMap_T *pCmdHndlrMap;
    INT32U HdrOffset = sizeof(IPMIMsgHdr_T);
    IPMIMsgHdr_T *pIPMIResHdr = (IPMIMsgHdr_T *)pRes->Data;
    const IPMIMsgHdr_T *pIPMIReqHdr = (const IPMIMsgHdr_T *)pReq->Data;
    INT8U ResDatSize = 0;
    pCmdHndlr_T CmdHndlr;

    if (!CheckMsgValidation(pReq->Data, pReq->Size))
    {
        IPMI_DBG_PRINT("IPMI Msg Check ERR");
        pRes->Size = 0;
        return;
    }

    // LOG_I("Process");
    // /* Set the Cmd and Net function in response packet */
    // pRes->Cmd		= pReq->Cmd;
    // pRes->NetFnLUN	= pReq->NetFnLUN | 0x04;

    /* Normal IPMI Command response */
    pRes->Param = NORMAL_RESPONSE;
    pRes->Size = HdrOffset + sizeof(INT8U);

    //    IPMI_DBG_PRINT("Processing IPMI Packet.\n");

    SwapIPMIMsgHdr(pIPMIReqHdr, pIPMIResHdr);

    if (0 != GetMsgHndlrMap(NET_FN(pIPMIReqHdr->NetFnLUN), &pCmdHndlrMap)) // get netfn handlr
    {
        pRes->Data[HdrOffset] = CC_INV_CMD;
        pRes->Size = HdrOffset + 1 + 1; // IPMI Header + completion code + Second Checksum
        // IPMI_WARNING("Invalid NetFn 0x%x cmd 0x%x\n", NET_FN(pIPMIReqHdr->NetFnLUN), pIPMIReqHdr->Cmd);
    }
    else
    {
        CmdHndlr = GetCmdHndlr(pCmdHndlrMap, pIPMIReqHdr->Cmd);
        if (CmdHndlr == NULL)
        {
            pRes->Data[HdrOffset] = CC_INV_CMD;
            pRes->Size = HdrOffset + 1 + 1; // IPMI Header + completion code + Second Checksum
            IPMI_WARNING("Invalid Cmd 0x%x\n", pIPMIReqHdr->Cmd);
        }
        else
        {
            ResDatSize = CmdHndlr(&pReq->Data[HdrOffset], pReq->Size - HdrOffset - 1, &pRes->Data[HdrOffset], 0);
            pRes->Size = ResDatSize + HdrOffset + 1; // IPMI Header + Response data field + Second Checksum
        }
    }

    /* Calculate the Second CheckSum */
    pRes->Data[pRes->Size - 1] = CalculateCheckSum2(pRes->Data, pRes->Size - 1);

    return;
}

/**
*@fn SwapIPMIMsgHdr
*@brief Swaps the header and copies into response
*@param pIPMIMsgReq Header of the Request
*@param pIPMIMsgRes Header of the response
*@return none
*/
void SwapIPMIMsgHdr(const IPMIMsgHdr_T *pIPMIMsgReq, _NEAR_ IPMIMsgHdr_T *pIPMIMsgRes)
{
    pIPMIMsgRes->ResAddr = pIPMIMsgReq->ReqAddr;
    pIPMIMsgRes->NetFnLUN = (pIPMIMsgReq->NetFnLUN & 0xFC) + 0x04;
    pIPMIMsgRes->NetFnLUN |= pIPMIMsgReq->RqSeqLUN & 0x03;

    /* Calculate the Checksum for above two bytes */
    pIPMIMsgRes->ChkSum = CalculateCheckSum((unsigned char *)pIPMIMsgRes, 2);

    pIPMIMsgRes->ReqAddr = pIPMIMsgReq->ResAddr;

    pIPMIMsgRes->RqSeqLUN = (pIPMIMsgReq->RqSeqLUN & 0xFC);
    pIPMIMsgRes->RqSeqLUN |= (pIPMIMsgReq->NetFnLUN & 0x03);

    pIPMIMsgRes->Cmd = pIPMIMsgReq->Cmd;

    return;
}

/**
*@fn UnImplementedFunc
*@brief Executes if the requested command in unimplemented
*@param pReq Request for the command
*@param ReqLen Request Length of the command
*@param pRes Response for the command
*@return Returns the size of the response
*/
int UnImplementedFunc(_NEAR_ INT8U *pReq, INT8U ReqLen, _NEAR_ INT8U *pRes, int BMCInst)
{
    *pRes = CC_INV_CMD;
    return sizeof(*pRes);
}

/**
*@fn GetMsgHndlrMap
*@brief Gets the exact command Handler by comparing NetFn
*@param Netfn -NetFunction of the Cmd to execute
*@param pCmdHndlrMap Pointer to the Command Handler
*@return Returns 0 on success
*            Returns -1 on failure
*/
int GetMsgHndlrMap(INT8U NetFn, _FAR_ CmdHndlrMap_T **pCmdHndlrMap)
{
    int i;
    _FAR_ BMCInfo_t *pBMCInfo = &g_BMCInfo;

    /* Get the command handler corresponding to the net function */
    for (i = 0; i < sizeof(pBMCInfo->MsgHndlrTbl) / sizeof(pBMCInfo->MsgHndlrTbl[0]); i++)
    {
        if (pBMCInfo->MsgHndlrTbl[i].NetFn == NetFn)
        {
            break;
        }
    }

    /* Check if we have not found our net function */
    if (i == sizeof(pBMCInfo->MsgHndlrTbl) / sizeof(pBMCInfo->MsgHndlrTbl[0]))
    {
        return -1;
    }

    /* Get the handler corresponding to the command */
    *pCmdHndlrMap = (CmdHndlrMap_T *)pBMCInfo->MsgHndlrTbl[i].CmdHndlrMap;
    return 0;
}

/**
*@fn GetCmdHndlr
*@brief Picks up the exact command to execute by comparing Cmd no.
*@param pReq Request buffer for the command
*@param pRes Response buffer for the command
*@param pCmdHndlrMap    m_MsgHndlrTbl
*@param HdrOffset
*@param CmdHndlr
*@return Returns TRUE on success
*            Returns FALSE on failure
*/
pCmdHndlr_T GetCmdHndlr(CmdHndlrMap_T *pCmdHndlrMap, INT8U Cmd)
{
    while (1)
    {
        if (pCmdHndlrMap->Cmd == Cmd)
        {
            /* Check if command has been implemented */
            return pCmdHndlrMap->CmdHndlr;
        }
        /**
        * If we reached the end of the Command Handler map - invalid command
        **/
        if (0 == pCmdHndlrMap->CmdHndlr)
        {
            return NULL;
        }
        pCmdHndlrMap++;
    }
}

/**
*@fn GroupExtnGetMsgHndlrMap
*@brief Gets the exact command Handler by comparing NetFn
*@param Netfn -NetFunction of the Cmd to execute
*@GroupExtnCode - Group Extension code
*@param pCmdHndlrMap Pointer to the Command Handler
*@return Returns 0 on success
*            Returns -1 on failure
*/
int GroupExtnGetMsgHndlrMap(INT8U NetFn, INT8U GroupExtnCode, CmdHndlrMap_T **pCmdHndlrMap, int BMCInst)
{
    //    int i;
    //    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfok;

    //    /* Get the command handler corresponding to the net function */
    //    for (i = 0; i < sizeof (pBMCInfo->GroupExtnMsgHndlrTbl) / sizeof (pBMCInfo->GroupExtnMsgHndlrTbl [0]); i++)
    //    {
    //        if ((pBMCInfo->GroupExtnMsgHndlrTbl [i].NetFn == NetFn) && (pBMCInfo->GroupExtnMsgHndlrTbl [i].GroupExtnCode == GroupExtnCode)) { break; }
    //    }

    //    /* Check if we have not found our net function */
    //    if (i == sizeof (pBMCInfo->GroupExtnMsgHndlrTbl) / sizeof (pBMCInfo->GroupExtnMsgHndlrTbl [0]))
    //    {
    //        return -1;
    //    }

    //    /* Get the handler corresponding to the command */
    //    *pCmdHndlrMap = (CmdHndlrMap_T*)pBMCInfo->GroupExtnMsgHndlrTbl [i].CmdHndlrMap;
    return 0;
}

/*------------------------------------------------------------------
 *@fn RespondSendMessage
 *@brief Frames the Response packet when a IPMB destination is
 *       unavialable
 *
 *@param pReq:    Request Message Packet address
 *@param Status   Status of SendIPMBPkt method
 *@param BMCInst: BMC Instance Number
 *
 *@return none
 *-----------------------------------------------------------------*/
void RespondSendMessage(MsgPkt_T *pReq, INT8U Status, int BMCInst)
{
    //		_FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo;

    //		MsgPkt_T   ResPkt;
    //		_NEAR_ IPMIMsgHdr_T*  pIPMIResHdr = (_NEAR_ IPMIMsgHdr_T*)ResPkt.Data;
    //		const IPMIMsgHdr_T*  pIPMIReqHdr = (const IPMIMsgHdr_T*)pReq->Data;

    //		INT8U SeqNum = NET_FN(pIPMIReqHdr->RqSeqLUN);

    //		if( ((pIPMIReqHdr->RqSeqLUN & 0x03) == BMC_LUN_01) ||
    //				((pIPMIReqHdr->RqSeqLUN & 0x03) == BMC_LUN_10) ||
    //				((pIPMIReqHdr->RqSeqLUN & 0x03) == BMC_LUN_11) )
    //		{
    //				switch(Status)
    //				{
    //						case STATUS_OK:
    //								ResPkt.Data[0] = CC_NORMAL;
    //								break;
    //						case STATUS_ARBLOST:
    //								ResPkt.Data[0] = CC_ARBITRATION_LOST;
    //								break;
    //						case STATUS_BUSERR:
    //								ResPkt.Data[0] = CC_BUS_ERROR;
    //								break;
    //						case STATUS_NACK:
    //								ResPkt.Data[0] = CC_NO_ACK_FROM_SLAVE;
    //								break;
    //						default:
    //								ResPkt.Data[0] = CC_UNSPECIFIED_ERR;
    //								break;
    //				}

    //		}

    /* Fill the response packet */

    /* Post the data to Destination Interface queue */
    //       PostMsg (&ResPkt, QueueName, BMCInst);
}

/**
* @fn GetIfcSupport
* @brief This function checks the support of Interface before
*            Interface specific commands are executed
* @param IfcSupt - Interface support variable to be verified
* @param IfcSupport - Gives the Interface presence support
* @return Returns ZERO
*/
int GetIfcSupport(INT16U IfcSupt, INT8U *IfcSupport, int BMCInst)
{
    _FAR_ BMCInfo_t *pBMCInfo = &g_BMCInfo;
    switch (IfcSupt)
    {
    case LAN_IFC_SUP:
        if (pBMCInfo->IpmiConfig.LANIfcSupport == IFCENABLED)
        {
            *IfcSupport = 1;
        }
        else
        {
            *IfcSupport = 0xFF;
        }
        break;
    case SOL_IFC_SUP:
        if (pBMCInfo->IpmiConfig.SOLIfcSupport == IFCENABLED && pBMCInfo->IpmiConfig.LANIfcSupport == IFCENABLED)
        {
            *IfcSupport = 1;
        }
        else
        {
            *IfcSupport = 0xFF;
        }
        break;
    case SERIAL_IFC_SUP:
        if (pBMCInfo->IpmiConfig.SerialIfcSupport == IFCENABLED)
        {
            *IfcSupport = 1;
        }
        else
        {
            *IfcSupport = 0xFF;
        }
        break;
    default:
        *IfcSupport = 0x1;
    }
    return 0;
}


static void device_addr_set(void)
{
    uint8_t device_addr = 0;

    device_addr = I2C_SLAVE_ADDRESS7;

    ipmb_set_addr(device_addr);
    SetDevAddr(device_addr);
}

