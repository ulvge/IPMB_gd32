/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * MsgHndlr.h
 * Message Handler.
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 * 		 : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#ifndef MSG_HNDLR_H
#define MSG_HNDLR_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "Types.h"
#include "Message.h"
#include "IPMIDefs.h"
#include "OSPort.h"

#define BMC_LUN_00                      0
#define BMC_LUN_01                      1
#define BMC_LUN_10                      2
#define BMC_LUN_11                      3

#define LOOP_BACK_REQ                     0x10

#define LAN_IFC_SUP 0x0001
#define SOL_IFC_SUP 0x0002
#define SERIAL_IFC_SUP 0x0004
#define AMI_DEVICE_SUP 0x0008
#define USER_LOCKED                    1
#define USER_UNLOCKED                0
#define MAX_NETFN                        64

#define MAX_CMD_LIMIT 20

/*FLASH TYPE*/
#define YAFU_FLASH   1
#define TFTP_FLASH   2
#define HPM_FLASH    3


/*Get the current cmd channel for OBSM*/
#define GetCurCmdChannel(BMCInst)	g_BMCInfo[BMCInst].Msghndlr.CurChannel

/*-----------------------
 * Command Handler Type
 *----------------------*/
typedef int (*pCmdHndlr_T) (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);

/*----------------------
 * Command Handler map
 *----------------------*/
typedef struct
{
    INT8U		Cmd;
    INT8U		Privilege;
    pCmdHndlr_T	CmdHndlr;
    INT8U		ReqLen;		/* 0xFF - Any Length */
    INT16U		FFConfig;
    INT16U            IfcSupport;

}  CmdHndlrMap_T;

/*-------------------------------
 * Extended Command Handler Type
 *-------------------------------*/
typedef int (*pExCmdHndlr_T) (INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);

/*------------------------------
 * Extended Command Handler map
 *------------------------------*/
/* FFConfig :
 * odd bits were earlier used for checking command's Fire wall configuration, 
 * but since it is now handled in libipmipar direclty, 
 * this odd bits are now reserved*/

typedef struct
{
    INT8U		Cmd;
    INT8U		Privilege;
    pExCmdHndlr_T	CmdHndlr;
    INT8U		ReqLen;		/* 0xFF - Any Length */
    INT16U		FFConfig;
    INT16U            IfcSupport;

} ExCmdHndlrMap_T;



/*-------------------------
 * Message Handler Table
 *-------------------------*/
typedef struct
{
    INT8U						NetFn;
    const CmdHndlrMap_T*	CmdHndlrMap;

}  MsgHndlrTbl_T;

typedef struct
{
    INT8U			NetFn;
    INT8U			GroupExtnCode;
    const CmdHndlrMap_T*	CmdHndlrMap;

} GroupExtnMsgHndlrTbl_T;


#pragma pack( 1 )

/*-------------------------------------------------------------
 * Pending Bridged Response Tbl Size
 * 
 * Do not increase beyond 64 as maximum sequence number is 64
 *------------------------------------------------------------*/
#define MAX_PENDING_BRIDGE_RES    64
#define MAX_PENDING_BRIDGE_TBL    3
#define PRIMARY_PB_TBL    0
#define SECONDARY_PB_TBL    1
#define THIRD_PB_TBL    2


typedef union
{
    IPMIMsgHdr_T        IPMIMsgHdr;
    IPMIUDSMsg_T        UDSMsgHdr;

}PACKED ResMsgHdr_T;

typedef struct
{
    INT8U               RetryInterval;
    INT8U               RetryCount;
    MsgPkt_T            MsgPkt;
}PACKED BridgeMsgRetryInfo_T;

typedef struct
{
    INT8U                 Used;
    INT8U                 TimeOut;
    INT8U                 SeqNum;
    INT8U                 ChannelNum;
    INT8U                 DstSessionHandle;
    INT8U                 SrcSessionHandle;
    INT32U                SrcSessionID;
    INT8U                 DestQ[PIPE_NAME_LEN];
    INT8U                 OriginSrc;
    IPMIMsgHdr_T          ReqMsgHdr;
    ResMsgHdr_T           ResMsgHdr;
    BridgeMsgRetryInfo_T  BridgeMsgRetryInfo;
} PACKED  PendingBridgedResTbl_T;

/*------------------------------
 * Pending Sequence Number
 *-----------------------------*/
//#define		NO_IPMB_PROTOCOL_CHANNEL	3
#define		MAX_PENDING_SEQ_NO			50
#define		SEQ_NO_EXPIRATION_TIME		5

typedef struct
{
    INT8U			Used;
    INT8U			TimeOut;
    INT8U			SeqNum;
    INT8U			NetFn;
    INT8U			Cmd;
} PACKED  PendingSeqNoTbl_T;

#define MAX_IPMI_CMD_BLOCKS         3
typedef struct
{
    INT8U			Channel;		/**< Originator's channel number 		*/
//    HQueue_T		hSrcQ;			/**< Originator Queue 					*/
    INT8U			Param;			/**< Parameter							*/
    INT8U			Cmd;			/**< Command that needs to be processed */
    INT8U			NetFnLUN;		/**< Net function and LUN of command    */
    INT8U			Privilege;		/**< Current privilege level			*/
    INT32U			SessionID;		/**< Session ID if any					*/
    INT8U			RequestSize;						/**< Size of the Request	*/
    INT8U			Request	 [MSG_PAYLOAD_SIZE];		/**< Request Data	buffer	*/
    INT8U			ResponseSize;						/**< Size of the response	*/
    INT8U			Response [MSG_PAYLOAD_SIZE];		/**< Response Data buffer	*/
    INT8U*    pRequest;                   /**< Pointer to Request data    */
    INT8U*    pResponse;                  /**< Pointer to Response data   */

} PACKED  IPMICmdMsg_T;

typedef struct
{
    INT8U    NetFnLUN;
    INT32U   SessionID;
    INT8U    SrcQ [PIPE_NAME_LEN];
} PACKED KCSBridgeResInfo_T;

#pragma pack( )

/*------------------ Timer Task Table --------------------------*/
typedef void (*pTimerFn_T) (int BMCInst);
typedef struct
{
    INT32U          NumSecs; //Change Datatype to INT32 for hourly or day base updation
    pTimerFn_T      TimerFn;

} TimerTaskTbl_T;

/*-----------------------------------------------------------
 * Bridge Status
 *---------------------------------------------------------*/
typedef enum
{
    STATUS_OK = 0,
    STATUS_ARBLOST,
    STATUS_BUSERR,
    STATUS_NACK,
    STATUS_FAIL,
}BridgeStatus;


typedef enum
{
    MSG_I2C = 0,
    MSG_UART,
}MsgType;
			 
/* Extern declaration */                                    
extern pCmdHndlr_T GetCmdHndlr(CmdHndlrMap_T *pCmdHndlrMap, INT8U Cmd);
extern int	  GetMsgHndlrMap (INT8U NetFn, CmdHndlrMap_T ** pCmdHndlrMap);

/**
*@fn GroupExtnGetMsgHndlrMap
*@brief Gets the exact command Handler by comparing NetFn
*@param Netfn -NetFunction of the Cmd to execute
*@GroupExtnCode - Group Extension code
*@param pCmdHndlrMap Pointer to the Command Handler
*/
extern int 	  GroupExtnGetMsgHndlrMap (INT8U NetFn, INT8U GroupExtnCode, CmdHndlrMap_T ** pCmdHndlrMap,int BMCInst);

extern  PendingBridgedResTbl_T  m_PendingBridgedResTbl[MAX_PENDING_BRIDGE_TBL][MAX_PENDING_BRIDGE_RES];
extern INT32U CalculateCheckSum2 (INT8U* Pkt, INT32U Len);
extern void  SwapIPMIMsgHdr (const IPMIMsgHdr_T* pIPMIMsgReq, IPMIMsgHdr_T* pIPMIMsgRes);
extern void  SwapUDSIPMIMsg (MsgPkt_T* pIPMIMsgReq, MsgPkt_T* pIPMIMsgRes);

/**
* @fn GetIfcSupport
* @brief This function checks the support of Interface before 
*            Interface specific commands are executed
* @param IfcSupt - Interface support variable to be verified
* @param IfcSupport - Gives the Interface presence support
*/
extern int GetIfcSupport(INT16U IfcSupt,INT8U *IfcSupport,int BMCInst);

/*------------------------
 * Privilege Levels
 *------------------------*/
#define PRIV_NONE		0x00
#define PRIV_CALLBACK	0x01
#define PRIV_USER		0x02
#define PRIV_OPERATOR	0x03
#define PRIV_ADMIN		0x04
#define PRIV_OEM		0x05
#define PRIV_LOCAL		0x81
#define PRIV_SYS_IFC	0x82

/*------------------------------
* Channel type used for BadPassword
*------------------------------*/
#define LAN_CHANNEL_BADP       0x1
#define SERIAL_CHANNEL_BADP  0x2

/*----------------------------
 * Command Handler Example (app.c)
 *----------------------------*/
/*
#ifdef	APP_DEVICE

 .... Code ....

const CmdHndlrMap_T	g_APP_CmdHndlr [] =
{
	{ CMD_GET_DEV_ID,			PRIV_USER,		GET_DEV_ID,			  0x00 },
	{ CMD_BROADCAST_GET_DEV_ID, PRIV_LOCAL,		BROADCAST_GET_DEV_ID, 0x00 },
	...
	...
	{ 0x00, 					0x00, 			0x00,				  0x00 }
};

#else
const CmdHndlrMap_T	g_APP_CmdHndlr [] = { 0x00, 0x00, 0x00, 0x00 };
#endif	// APP_DEVICE
*/

/*---------------------------
 * app.h
 *---------------------------*/
/*
#ifndef APP_H
#define APP_H
#include "MsgHndlr.h"

extern const CmdHndlrMap_T g_APP_CmdHndlr [];

#endif
*/

/*---------------------------------------------------
 * Parameters passed by through the message handler Q.
 *---------------------------------------------------*/
#define PARAM_IFC			        0x01
#define PARAM_TIMER			        0x02
#define PARAM_EXECUTE_SP	        0x03
#define PARAM_SENSOR_MONITOR        0x04
#define PARAM_SENSOR_MONITOR_DONE   0x05
#define PARAM_IFC_READY				0x06
#define RCVMSGQ_LENGTH              100


#define NO_RESPONSE			        0x10
#define NORMAL_RESPONSE		        0x11
#define BRIDGING_REQUEST	        0x12
#define IPMI_REQUEST	            0x13
#define SERIAL_REQUEST	            0x14
#define LAN_REQUEST	                0x15
#define FTCPU_RESPONSE	            0x16

#define FORWARD_IPMB_REQUEST		0x18

#define IPMB_SUB_DEVICE_HEARTBEAT_REQUEST	0x1a
#define IPMB_SUB_DEVICE_HEARTBEAT_RESPONSE	0x1b

/*----------------------------------------------------
 * Receive Message Queue Names
 *----------------------------------------------------*/
extern char g_RcvMsgQ [4][RCVMSGQ_LENGTH];

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
extern void RespondSendMessage ( MsgPkt_T* pReq, INT8U Status, int BMCInst);

/**
*@fn ValidateMsgHdr
*@brief Validates the Message header and keeps track of the messages that has been bridged
*@param pReq Request packet of the command to be executed
*@return Returns -1 in case of the response to the bridged message
*            Returns 0 otherwise
*/
//extern int  ValidateMsgHdr (MsgPkt_T* pReq, int BMCInst);

/**
*@fn GetUTC_Offset
*@brief returns UTC offset from local time
*/
extern int GetUTC_Offset(void);

extern void *MsgCoreHndlr(void *pArg);
extern void ipmb_set_dualaddr(INT32U i2c_periph, INT32U dualaddr);

extern INT32U ProcessIPMIReq(MsgPkt_T *pReq, MsgPkt_T *pRes);
extern bool ProcessIPMBForardResponse(MsgPkt_T *pReq, MsgPkt_T *pRes);
extern BaseType_t SendMsgAndWait(MsgPkt_T *pReq, MsgPkt_T *pRes, INT32U timeout);

#ifdef __cplusplus
}
#endif

#endif  /* MSG_HNDLR_H */
