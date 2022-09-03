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
 * DeviceConfig.c
 * Device Configuration Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#define UNIMPLEMENTED_AS_FUNC
#include "MsgHndlr.h"
#include "DeviceConfig.h"
#include "IPMI_SerialModem.h"
#include "IPMI_LANConfig.h"
#include "IPMI_SOLConfig.h"
#include "SerialModem.h"
#include "LANConfig.h"
#include "SOLConfig.h"
#include "Support.h"

/**
 * @var g_Config_CmdHndlr
 * @brief Transport Configuration Commands Map
**/
CmdHndlrMap_T g_Config_CmdHndlr [] =
{
    /*------------------------- IPM Device Commands --------------------------------------*/
    { CMD_SET_LAN_CONFIGURATION_PARAMETERS,     PRIV_ADMIN,     SET_LAN_CONFIGURATION_PARAMETERS,   0xFF,   0xAAAA  ,LAN_IFC_SUP},
    { CMD_GET_LAN_CONFIGURATION_PARAMETERS,     PRIV_OPERATOR,  GET_LAN_CONFIGURATION_PARAMETERS,   sizeof (GetLanConfigReq_T), 0xAAAA  ,LAN_IFC_SUP},
    { CMD_SUSPEND_BMC_ARPS,                    	PRIV_ADMIN,     SUSPEND_BMC_ARPS,                   sizeof (SuspendBMCArpsReq_T),   0xAAAA ,LAN_IFC_SUP},
    { CMD_GET_IP_UDP_RMCP_STATISTICS,    		PRIV_ADMIN,     GET_IP_UDP_RMCP_STATISTICS,   0xFF,   0xAAAA  ,LAN_IFC_SUP},

    /*--------------------- Serial/Modem Device Commands ---------------------------------*/
    { CMD_SET_SERIAL_MODEM_CONFIG,  PRIV_ADMIN,     SET_SERIAL_MODEM_CONFIGURATION,  0xFF,  0xAAAA  ,SERIAL_IFC_SUP},
    { CMD_GET_SERIAL_MODEM_CONFIG,  PRIV_OPERATOR,  GET_SERIAL_MODEM_CONFIGURATION,  sizeof (GetSerialModemConfigReq_T),    0xAAAA ,SERIAL_IFC_SUP},
    { CMD_SET_SERIAL_MODEM_MUX,     PRIV_OPERATOR,  SET_SERIAL_MODEM_MUX,            sizeof (SetMuxReq_T),  0xAAAA  ,SERIAL_IFC_SUP},
    { CMD_SERIAL_MODEM_CONNECTION_ACTIVITY,     PRIV_OPERATOR,  SERIAL_MODEM_CONNECTION_ACTIVITY,sizeof (SerialModemActivePingReq_T),   0xAAAA ,SERIAL_IFC_SUP},
    { CMD_CALLBACK,                 PRIV_ADMIN,     CALLBACK,                        sizeof (CallbackReq_T) ,   0xAAAA  ,SERIAL_IFC_SUP},
    { CMD_SET_USER_CALLBACK_OPTION, PRIV_ADMIN,     SET_USER_CALLBACK_OPTIONS,       sizeof (SetUserCallbackReq_T), 0xAAAA  ,SERIAL_IFC_SUP},
    { CMD_GET_USER_CALLBACK_OPTION, PRIV_USER,      GET_USER_CALLBACK_OPTIONS,       sizeof (GetUserCallbackReq_T), 0xAAAA  ,SERIAL_IFC_SUP},
    { CMD_GET_TAP_RES_CODES,        PRIV_USER,      GET_TAP_RESPONSE,                sizeof (GetTAPResCodeReq_T),   0xAAAA  ,SERIAL_IFC_SUP},

    /*------------------------- Serial Over LAN Commands ---------------------------------*/
    { CMD_GET_SOL_CONFIGURATION,    PRIV_USER,      GET_SOL_CONFIGURATION,          sizeof (GetSOLConfigReq_T), 0xAAAA  ,SOL_IFC_SUP},
    { CMD_SET_SOL_CONFIGURATION,    PRIV_ADMIN,     SET_SOL_CONFIGURATION,          0xFF,       0xAAAA  ,SOL_IFC_SUP},

    { 0x00,                         0x00,           0x00,                            0x00,  0x0000    ,0x0000}
};

