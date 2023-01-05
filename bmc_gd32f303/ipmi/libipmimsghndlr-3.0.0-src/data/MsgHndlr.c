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

// extern pthread_mutex_t MsgHndlrMutex;

/*--------------------------------------------------------------------
* Global Variables
*--------------------------------------------------------------------*/
BMCInfo_t g_BMCInfo;

TaskHandle_t xHandleTaskResponseDatWrite = NULL;
xQueueHandle ResponseDatMsg_Queue = NULL;

xQueueHandle RecvDatMsg_Queue = NULL;
xQueueHandle RecvForwardI2CDatMsg_Queue = NULL;

//INT8U ExtNetFnMap[MAX_NUM_BMC][MAX_NETFN];

static const MsgHndlrTbl_T m_MsgHndlrTbl[7] =
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

void ipmb_set_dualaddr(INT32U i2c_periph, INT32U dualaddr)
{
    i2c_dualaddr_set(i2c_periph, dualaddr);
}
static uint8_t ipmb_get_dualaddr(INT32U i2c_periph)
{
    return i2c_dualaddr_get(i2c_periph);
}
static bool ipmb_write(INT8U channel, const uint8_t *p_buffer, uint16_t len)
{
    return i2c_write(channel, p_buffer, len);
}
static void vTaskResponseDatWrite(void *pvParameters)
{
    char buff[sizeof(MsgPkt_T)];
    MsgPkt_T *ResMsg = (MsgPkt_T *)buff;
    ResponseDatMsg_Queue = xQueueCreate(2, sizeof(MsgPkt_T));
    RecvForwardI2CDatMsg_Queue = xQueueCreate(1, sizeof(MsgPkt_T));

    while (1)
    {
        xQueueReceive(ResponseDatMsg_Queue, buff, portMAX_DELAY);

        switch(ResMsg->Param)
        {
        case IPMB_SUB_DEVICE_HEARTBEAT_REQUEST:
        case IPMI_REQUEST:
        case FORWARD_IPMB_REQUEST:
        case NORMAL_RESPONSE:
            ipmb_write(ResMsg->Channel, ResMsg->Data, ResMsg->Size);
            break;
        case SERIAL_REQUEST:
            LOG_D("send ack msg of original\r\n");
            serial_write(ResMsg->Data, ResMsg->Size);
            break;
        case LAN_REQUEST:
            // LOG_D("LAN write->:");
            //SendLANPkt(ResMsg);
            break;
        default:
            continue;
        }
        if (g_debugLevel >= DBG_LOG){
            LOG_D("\r\nsend ack msg of hex for view para:%#x\r\n", ResMsg->Param);
            for (int i = 0; i < ResMsg->Size; i++)
            {
                LOG_RAW("%02x ", ResMsg->Data[i]);
            }
            LOG_RAW("\r\n");
        }
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
    MsgPkt_T recvPkt;
    BaseType_t err = pdFALSE;

    MsgHndlrInit();

    if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ==                                             
		xTaskCreate(vTaskResponseDatWrite, "Task ResponseDatWrite", configMINIMAL_STACK_SIZE * 2, NULL, TASK_PRIO_MSG_RESPONSE_HANDLE, &xHandleTaskResponseDatWrite)) {
        LOG_E("vTaskResponseDatWrite create task ERR!");
    }
    RecvDatMsg_Queue = xQueueCreate(2, sizeof(MsgPkt_T));  //10
    if (RecvDatMsg_Queue ==  NULL) {
        LOG_E("RecvDatMsg_Queue create ERR!");
        vTaskDelete(NULL);
    }

    while (1)
    {
        memset(&recvPkt, 0, sizeof(recvPkt));
        err = xQueueReceive(RecvDatMsg_Queue, &recvPkt, portMAX_DELAY);
        if (err == pdFALSE)
        {
            LOG_E("xQueueReceive get msg ERR!");
            continue;
        }
		
        switch(recvPkt.Param)
        {
        case IPMI_REQUEST:
        case IPMB_SUB_DEVICE_HEARTBEAT_RESPONSE:
			if (recvPkt.Size == 0){
				continue;
			}             
			Res.Param = NORMAL_RESPONSE;
            if (ProcessIPMIReq(&recvPkt, &Res) == 0){
				continue;
			}
            break;
        case SERIAL_REQUEST:
            if (ProcessSerialReq(&recvPkt, &Res) == false){
				continue;
			}
            break;
        case LAN_REQUEST:
            //ProcessLANReq(&recvPkt, &Res);
            break;
        default :
            break;
        }

        err = xQueueSend(ResponseDatMsg_Queue, (char*)&Res, 10);
        if (err == pdFALSE)
        {
            LOG_E("xQueueSend send msg ERR!");
        }

        // LOG_D("len:%d\n",recvPkt.Size);
        // for (int i = 0; i < recvPkt.Size; i++)
        // {
        //     LOG_RAW("%02x ", recvPkt.Data[i]);
        // }
        // LOG_D("\r\n");
    }
}
BaseType_t SendMsgAndWait(MsgPkt_T* pReq, MsgPkt_T* pRes, INT32U timeout)
{
    BaseType_t err = xQueueSend(ResponseDatMsg_Queue, (char*)pReq, 50);
    if(err == pdFALSE) {
        return pdFAIL;
    }

    err = xQueueReceive(RecvForwardI2CDatMsg_Queue, pRes, timeout);
    if(err == pdFALSE){
        return pdFAIL;
    }
	
    return pdPASS;
}
bool ProcessIPMBForardResponse(MsgPkt_T *pReq, MsgPkt_T *pRes)
{           
    INT8U EnRes [MAX_SERIAL_PKT_SIZE];
    // check ori msg is valid
    pRes->Size = 0;
    if (pReq == NULL || pReq->Size == 0) {
        return false;
    }
	
    IPMIMsgHdr_T *pIPMIReqHdr = (IPMIMsgHdr_T *)pReq->Data;

	pIPMIReqHdr->ResAddr = ipmb_get_dualaddr(pReq->Channel); //restore the request address
	pIPMIReqHdr->ChkSum = CalculateCheckSum(pReq->Data, 2); // recalc the chksum
    if (!CheckMsgValidation(pReq->Data, pReq->Size))
    {
        IPMI_DBG_PRINT("IPMB Forard Msg Check ERR\n");
        return false;
    }
    //
    /* Normal IPMI Command response */
    pRes->Param = SERIAL_REQUEST;
    /* 4 encode and ransmit the response */ 
    pRes->Size = EncodeSerialPkt (pReq->Data, pReq->Size, EnRes, sizeof(EnRes));
    _fmemcpy(pRes->Data, EnRes, pRes->Size);
    return true;
}
/**
*@fn ProcessIPMIReq
*@brief Processes the requested IPMI command
*@param pReq Request of the command
*@param pRes Response for the requested command
*@return none
*/
INT32U ProcessIPMIReq(MsgPkt_T *pReq, MsgPkt_T *pRes)
{
    CmdHndlrMap_T *pCmdHndlrMap;
    INT32U HdrOffset = sizeof(IPMIMsgHdr_T);
    IPMIMsgHdr_T *pIPMIResHdr = (IPMIMsgHdr_T *)pRes->Data;
    const IPMIMsgHdr_T *pIPMIReqHdr = (const IPMIMsgHdr_T *)pReq->Data;
    INT8U ResDatSize = 0;
    pCmdHndlr_T CmdHndlr;

    pRes->Size = 0;
    if (pReq == NULL || pReq->Size == 0) {
        return 0;
    }
    if (!CheckMsgValidation(pReq->Data, pReq->Size))
    {
        IPMI_DBG_PRINT("IPMI Msg Check ERR");
        return 0;
    }

    // LOG_I("Process");
    // /* Set the Cmd and Net function in response packet */
    pRes->Channel		= pReq->Channel;
    // pRes->Cmd		= pReq->Cmd;
    // pRes->NetFnLUN	= pReq->NetFnLUN | 0x04;

    /* Normal IPMI Command response */
    pRes->Size = HdrOffset + sizeof(INT8U);

    //    IPMI_DBG_PRINT("Processing IPMI Packet.\n");

    SwapIPMIMsgHdr(pIPMIReqHdr, pIPMIResHdr);
	
	if (pReq->Size > 8){
		//LOG_E("ProcessIPMIReq debug!");
	}
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
			LOG_D("do work Cmd %#x\n", (UINT32 )CmdHndlr);
            ResDatSize = CmdHndlr(&pReq->Data[HdrOffset], pReq->Size - HdrOffset - 1, &pRes->Data[HdrOffset], 0);
            pRes->Size = ResDatSize + HdrOffset + 1; // IPMI Header + Response data field + Second Checksum
        }
    }

    /* Calculate the Second CheckSum */
    pRes->Data[pRes->Size - 1] = CalculateCheckSum2(pRes->Data, pRes->Size - 1); 
	//printf("check sum = %x", pRes->Data[pRes->Size - 1]);
    return pRes->Size;
}

/**
*@fn SwapIPMIMsgHdr
*@brief Swaps the header and copies into response
*@param pIPMIMsgReq Header of the Request
*@param pIPMIMsgRes Header of the response
*@return none
*/
void SwapIPMIMsgHdr(const IPMIMsgHdr_T *pIPMIMsgReq, IPMIMsgHdr_T *pIPMIMsgRes)
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
int UnImplementedFunc(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
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
int GetMsgHndlrMap(INT8U NetFn, CmdHndlrMap_T **pCmdHndlrMap)
{
    int i;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

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
    //    BMCInfo_t* pBMCInfo = &g_BMCInfok;

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
    //		BMCInfo_t* pBMCInfo = &g_BMCInfo;

    //		MsgPkt_T   ResPkt;
    //		IPMIMsgHdr_T*  pIPMIResHdr = (IPMIMsgHdr_T*)ResPkt.Data;
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
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
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
