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
*****************************************************************
 *
 * Bridge.c
 * Bridge Command Handler
 *
 * Author: Gowtham.M.S <gowthamms@amiindia.co.in>
 *
 *****************************************************************/
#define UNIMPLEMENTED_AS_FUNC
#include "MsgHndlr.h"
#include "Bridge.h"
#include "BridgeMgmt.h"
#include "Bridging.h"
#include "BridgeDiscovery.h"
#include "BridgeEvt.h"

/**
 * @brief Bridge Command Handler Map.
**/
const CmdHndlrMap_T g_Bridge_CmdHndlr [] =
{
#if BRIDGE_DEVICE == 1
/*----------------------- Bridge Management Commands --------------------------------*/
    { CMD_GET_BRIDGE_STATE,     PRIV_USER,      GET_BRIDGE_STATE,       0x00,   0xAAAA  ,0xFFFF},
    { CMD_SET_BRIDGE_STATE,     PRIV_OPERATOR,  SET_BRIDGE_STATE,       sizeof(SetBridgeStateReq_T),    0xAAAA  ,0xFFFF},
    { CMD_GET_ICMB_ADDR,        PRIV_USER,      GET_ICMB_ADDR,          0x00,   0xAAAA  ,0xFFFF},
    { CMD_SET_ICMB_ADDR,        PRIV_OPERATOR,  SET_ICMB_ADDR,          sizeof(SetICMBAddrReq_T),   0xAAAA  ,0xFFFF},
    { CMD_SET_BRIDGE_PROXY_ADDR,PRIV_OPERATOR,  SET_BRIDGE_PROXY_ADDR,      0xff,   0xAAAA  ,0xFFFF}, //sizeof(SetBrProxyAddrReq_T)
    { CMD_GET_BRIDGE_STATISTICS,PRIV_USER,      GET_BRIDGE_STATISTICS,      sizeof(GetBrStatisticsReq_T),   0xAAAA  ,0xFFFF},
    { CMD_GET_ICMB_CAPABILITIES,PRIV_USER,      GET_ICMB_CAPABILITIES,      0x00,   0xAAAA  ,0xFFFF},
    { CMD_CLEAR_BRIDGE_STATISTICS,PRIV_OPERATOR,CLEAR_BRIDGE_STATISTICS,    0x00,   0xAAAA  ,0xFFFF},
    { CMD_GET_BRIDGE_PROXY_ADDR,PRIV_USER,      GET_BRIDGE_PROXY_ADDR,      0x00,   0xAAAA  ,0xFFFF},
    { CMD_GET_ICMB_CONNECTOR_INFO,PRIV_USER,    GET_ICMB_CONNECTOR_INFO,    sizeof(GetICMBConnInfoReq_T),   0xAAAA  ,0xFFFF},
    { CMD_GET_ICMB_CONNECTION_ID,PRIV_USER,     GET_ICMB_CONNECTION_ID,     0xff,   0xAAAA  ,0xFFFF},
    { CMD_SEND_ICMB_CONNECTION_ID,PRIV_USER,    SEND_ICMB_CONNECTION_ID,    0x00,   0xAAAA  ,0xFFFF},

/*---------------------- Bridge Discovery Commands -----------------------------------*/
    { CMD_PREPARE_FOR_DISCOVERY,  PRIV_OPERATOR, PREPARE_FOR_DISCOVERY, 0x00,   0xAAAA  ,0xFFFF},
    { CMD_GET_ADDRESSES,          PRIV_USER,     GET_ADDRESSES,         0x00,   0xAAAA  ,0xFFFF},
    { CMD_SET_DISCOVERED,         PRIV_OPERATOR, SET_DISCOVERED,        0x00,   0xAAAA  ,0xFFFF},
    { CMD_GET_CHASSIS_DEVICE_ID,  PRIV_USER,     GET_CHASSIS_DEVICE_ID, 0x00,   0xAAAA  ,0xFFFF},
    { CMD_SET_CHASSIS_DEVICE_ID,  PRIV_OPERATOR, SET_CHASSIS_DEVICE_ID, sizeof(INT8U),  0xAAAA  ,0xFFFF},

/*----------------------- Bridging Commands ------------------------------------------*/
    { CMD_BRIDGE_REQUEST,       PRIV_OPERATOR,  BRIDGE_REQUEST,         0xff,   0xAAAA  ,0xFFFF},
    { CMD_BRIDGE_MESSAGE,       PRIV_OPERATOR,  BRIDGE_MESSAGE,         0xff,   0xAAAA  ,0xFFFF},

/*---------------------- Bridge Event Commands ---------------------------------------*/
    { CMD_GET_EVENT_COUNT,          PRIV_USER,      GET_EVENT_COUNT,            0x00,   0xAAAA  ,0xFFFF},
    { CMD_SET_EVENT_DESTINATION,    PRIV_OPERATOR,  SET_EVENT_DESTINATION,      sizeof(SetEvtDestReq_T),    0xAAAA  ,0xFFFF},
    { CMD_SET_EVENT_RECEPTION_STATE,PRIV_OPERATOR,  SET_EVENT_RECEPTION_STATE,  sizeof(SetEvtRecpStateReq_T),   0xAAAA  ,0xFFFF},
    { CMD_SEND_ICMB_EVENT_MESSAGE,  PRIV_OPERATOR,  SEND_ICMB_EVENT_MESSAGE,    0xff,   0xAAAA  ,0xFFFF}, //sizeof(SendICMBEvtMsgReq_T)
    { CMD_GET_EVENT_DESTINATION,    PRIV_USER,      GET_EVENT_DESTINATION,      0x00,   0xAAAA  ,0xFFFF},
    { CMD_GET_EVENT_RECEPTION_STATE,PRIV_USER,      GET_EVENT_RECEPTION_STATE,  0x00,   0xAAAA  ,0xFFFF},
#endif  /*BRIDGE_DEVICE*/

    { 0x00,                     0x00,           0x00,                 0x00, 0x0000           ,0x0000}   
};

