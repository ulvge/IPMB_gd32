/****************************************************************
 ****************************************************************
 **                                                            **

 ****************************************************************
 *****************************************************************
 *
 * App.c
 * Application Command Handler
 *
 * Author:  
 * 
 *****************************************************************/
#define UNIMPLEMENTED_AS_FUNC

#include "Types.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "App.h"
#include "IPMDevice.h"
#include "AppDevice.h"
#include "AppDevice+.h"
#include "Firewall.h"
#include "IPMI_IPM.h"
#include "IPMI_AppDevice.h"
#include "IPMI_AppDevice+.h"
#include "IPMI_Firewall.h"

/*** Global Variables ***/
const CmdHndlrMap_T g_App_CmdHndlr [] =
{
#if 1//IPM_DEVICE == 1
    /*------------------------- IPM Device Commands -------------------------*/
    { CMD_APP_RESERVED,               PRIV_USER,      APP_RESERVED,             0x00,                           0xAAAA , 0xFFFF},
    { CMD_GET_DEV_ID,               PRIV_USER,      GET_DEV_ID,             0x00,                           0xAAAA , 0xFFFF},
//    { CMD_BROADCAST_GET_DEV_ID,     PRIV_LOCAL,     BROADCAST_GET_DEV_ID,   0x00,                           0xAAAA ,0xFFFF},
    { CMD_COLD_RESET,               PRIV_ADMIN,     COLD_RESET,             0x00,                           0xAAAA ,0xFFFF},
//    { CMD_WARM_RESET,               PRIV_ADMIN,     WARM_RESET,             0x00,                           0xAAAA , 0xFFFF},
//    { CMD_GET_SELF_TEST_RESULTS,    PRIV_USER,      GET_SELF_TEST_RESULTS,  0x00,                           0xAAAA , 0xFFFF},
//    { CMD_MFG_TEST_ON,              PRIV_ADMIN,     MFG_TEST_ON,            0xFF,                           0xAAAA ,0xFFFF},
//    { CMD_SET_ACPI_PWR_STATE,       PRIV_ADMIN,     SET_ACPI_PWR_STATE,     sizeof (SetACPIPwrStateReq_T),  0xAAAA ,0xFFFF},
//    { CMD_GET_ACPI_PWR_STATE,       PRIV_USER,      GET_ACPI_PWR_STATE,     0x00,                           0xAAAA ,0xFFFF},
//    { CMD_GET_DEV_GUID,             PRIV_USER,      GET_DEV_GUID,           0x00,                           0xAAAA ,0xFFFF},
//    { CMD_GET_NETFN_SUP,            PRIV_USER,      GET_NETFN_SUP,          0x01,                           0xAAAA ,0xFFFF},
//    { CMD_GET_CMD_SUP,              PRIV_USER,      GET_CMD_SUP,            0xFF,                           0xAAAA ,0xFFFF},
//    { CMD_GET_SUBFN_SUP,            PRIV_USER,      GET_SUBFN_SUP,          0xFF,                           0xAAAA ,0xFFFF},
//    { CMD_GET_CONFIG_CMDS,          PRIV_USER,      GET_CONFIG_CMDS,        0xFF,                           0xAAAA ,0xFFFF},
//    { CMD_GET_CONFIG_SUB_FNS,       PRIV_USER,      GET_CONFIG_SUB_FNS,     0xFF,                           0xAAAA ,0xFFFF},
//    { CMD_SET_CMD_ENABLES,          PRIV_USER,      SET_CMD_ENABLES,        0xFF,                           0xAAAA ,0xFFFF},
//    { CMD_GET_CMD_ENABLES,          PRIV_USER,      GET_CMD_ENABLES,        0xFF,                           0xAAAA ,0xFFFF},
//    { CMD_SET_SUBFN_ENABLES,        PRIV_USER,      SET_SUBFN_ENABLES,      sizeof (SetSubFnEnablesReq_T),  0xAAAA ,0xFFFF},
//    { CMD_GET_SUBFN_ENABLES,        PRIV_USER,      GET_SUBFN_ENABLES,      sizeof (GetSubFnEnablesReq_T),  0xAAAA ,0xFFFF},
//    { CMD_GET_OEM_NETFN_IANA_SUPPORT,    PRIV_USER,      GET_OEM_NETFN_IANA_SUPPORT,      sizeof (GetOEMNetFnIANASupportReq_T),  0xAAAA ,0xFFFF},
#endif /* IPM_DEVICE */

#if APP_DEVICE == 1
    /*--------------------- Watch Dog Timer Commands -----------------------*/
    { CMD_RESET_WDT,                PRIV_OPERATOR,  RESET_WDT,              0x00,                           0xAAAA ,0xFFFF},
    { CMD_SET_WDT,                  PRIV_OPERATOR,  SET_WDT,                sizeof (SetWDTReq_T),           0xAAAA ,0xFFFF},
    { CMD_GET_WDT,                  PRIV_USER,      GET_WDT,                0x00,                           0xAAAA ,0xFFFF},

    /*----------------- BMC Device and Messaging Commands ------------------*/
    { CMD_SET_BMC_GBL_ENABLES,      PRIV_LOCAL,   SET_BMC_GBL_ENABLES,    sizeof (INT8U),                 0xAAAA ,0xFFFF},
    { CMD_GET_BMC_GBL_ENABLES,      PRIV_USER,      GET_BMC_GBL_ENABLES,    0x00,                           0xAAAA ,0xFFFF},
    { CMD_CLR_MSG_FLAGS,            PRIV_LOCAL,   CLR_MSG_FLAGS,          sizeof (INT8U),                 0xAAAA ,0xFFFF},
    { CMD_GET_MSG_FLAGS,            PRIV_LOCAL,   GET_MSG_FLAGS,          0x00,                           0xAAAA ,0xFFFF},
    { CMD_ENBL_MSG_CH_RCV,          PRIV_LOCAL,   ENBL_MSG_CH_RCV,        sizeof (EnblMsgChRcvReq_T),     0xAAAA ,0xFFFF},
    { CMD_GET_MSG,                  PRIV_LOCAL,      GET_MSG,                0x00,                           0xAAAA ,0xFFFF},
    { CMD_SEND_MSG,                 PRIV_USER,      SEND_MSG,               0xFF,                           0xAAAA ,0xFFFF},
    { CMD_READ_EVT_MSG_BUFFER,      PRIV_LOCAL,     READ_EVT_MSG_BUFFER,    0x00,                           0xAAAA ,0xFFFF},
    { CMD_GET_BTIFC_CAP,            PRIV_USER,      GET_BTIFC_CAP,          0x00,                           0xAAAA ,0xFFFF},
    { CMD_GET_SYSTEM_GUID,          PRIV_NONE,      GET_SYSTEM_GUID,        0x00,                           0xAAAA ,0xFFFF},
    { CMD_GET_CH_AUTH_CAP,          PRIV_NONE,      GET_CH_AUTH_CAP,        sizeof (GetChAuthCapReq_T),     0xAAAA ,0xFFFF},
    { CMD_GET_SESSION_CHALLENGE,    PRIV_NONE,      GET_SESSION_CHALLENGE,  sizeof (GetSesChallengeReq_T),  0xAAAA ,0xFFFF},
    { CMD_ACTIVATE_SESSION,         PRIV_NONE,      ACTIVATE_SESSION,       sizeof (ActivateSesReq_T),      0xAAAA ,0xFFFF},
    { CMD_SET_SESSION_PRIV_LEVEL,   PRIV_USER,      SET_SESSION_PRIV_LEVEL, sizeof (INT8U),                 0xAAAA ,0xFFFF},
    { CMD_CLOSE_SESSION,            PRIV_CALLBACK,  CLOSE_SESSION,          0xFF,                           0xAAAA ,0xFFFF},
    { CMD_GET_SESSION_INFO,         PRIV_USER,      GET_SESSION_INFO,       0xFF,                           0xAAAA ,0xFFFF},
    { CMD_GET_AUTH_CODE,            PRIV_OPERATOR,  GET_AUTH_CODE,          sizeof (GetAuthCodeReq_T),      0xAAAA ,0xFFFF},
    { CMD_SET_CH_ACCESS,            PRIV_ADMIN,     SET_CH_ACCESS,          sizeof (SetChAccessReq_T),      0xAAAA ,0xFFFF},
    { CMD_GET_CH_ACCESS,            PRIV_USER,      GET_CH_ACCESS,          sizeof (GetChAccessReq_T),      0xAAAA ,0xFFFF},
    { CMD_GET_CH_INFO,              PRIV_USER,      GET_CH_INFO,            sizeof (INT8U),                 0xAAAA ,0xFFFF},
    { CMD_SET_USER_ACCESS,          PRIV_ADMIN,     SET_USER_ACCESS,        0xFF,    0xAAAA ,0xFFFF},
    { CMD_GET_USER_ACCESS,          PRIV_OPERATOR,  GET_USER_ACCESS,        sizeof (GetUserAccessReq_T),    0xAAAA ,0xFFFF},
    { CMD_SET_USER_NAME,            PRIV_ADMIN,     SET_USER_NAME,          sizeof (SetUserNameReq_T),      0xAAAA ,0xFFFF},
    { CMD_GET_USER_NAME,            PRIV_OPERATOR,  GET_USER_NAME,          sizeof (INT8U),                 0xAAAA ,0xFFFF},
    { CMD_SET_USER_PASSWORD,        PRIV_ADMIN,     SET_USER_PASSWORD,      0xFF,      0xAAAA ,0xFFFF},
    { CMD_MASTER_WRITE_READ,        PRIV_OPERATOR,  MASTER_WRITE_READ,      0xFF,                           0xAAAA ,0xFFFF},
    { CMD_SET_SYSTEM_INFO_PARAM,    PRIV_ADMIN,     SET_SYSTEM_INFO_PARAM,  0xFF,                           0xAAAA ,0xFFFF},
    { CMD_GET_SYSTEM_INFO_PARAM,    PRIV_USER ,  GET_SYSTEM_INFO_PARAM,  sizeof (GetSystemInfoParamReq_T),      0xAAAA ,0xFFFF},

    /*------------------------ IPMI 2.0 specific Commands ------------------*/
//    { CMD_ACTIVATE_PAYLOAD,         PRIV_CALLBACK,  ACTIVATE_PAYLOAD,       sizeof (ActivatePayloadReq_T),  0xAAAA ,0xFFFF},
//    { CMD_DEACTIVATE_PAYLOAD,       PRIV_CALLBACK,  DEACTIVATE_PAYLOAD,     sizeof (DeactivatePayloadReq_T),0xAAAA ,0xFFFF},
//    { CMD_GET_PAYLD_ACT_STATUS,     PRIV_USER,      GET_PAYLD_ACT_STATUS,   1,                              0xAAAA ,0xFFFF},
//    { CMD_GET_PAYLD_INST_INFO,      PRIV_USER,      GET_PAYLD_INST_INFO,    2,                              0xAAAA ,0xFFFF},
//    { CMD_SET_USR_PAYLOAD_ACCESS,   PRIV_ADMIN,     SET_USR_PAYLOAD_ACCESS, sizeof (SetUsrPayldAccReq_T),   0xAAAA ,0xFFFF},
//    { CMD_GET_USR_PAYLOAD_ACCESS,   PRIV_OPERATOR,  GET_USR_PAYLOAD_ACCESS, sizeof (GetUsrPayldAccReq_T),   0xAAAA ,0xFFFF},
//    { CMD_GET_CH_PAYLOAD_SUPPORT,   PRIV_USER,      GET_CH_PAYLOAD_SUPPORT, 1,                              0xAAAA ,0xFFFF},
//    { CMD_GET_CH_PAYLOAD_VER,       PRIV_USER,      GET_CH_PAYLOAD_VER,     sizeof (GetChPayldVerReq_T),    0xAAAA ,0xFFFF},
//    { CMD_GET_CH_OEM_PAYLOAD_INFO,  PRIV_USER,      GET_CH_OEM_PAYLOAD_INFO,sizeof (GetChOemPayldInfoReq_T),0xAAAA ,0xFFFF},
//    { CMD_GET_CH_CIPHER_SUITES,     PRIV_NONE,      GET_CH_CIPHER_SUITES,   sizeof (GetChCipherSuitesReq_T),0xAAAA ,0x0001},
//    { CMD_SUS_RES_PAYLOAD_ENCRYPT,  PRIV_USER,      SUS_RES_PAYLOAD_ENCRYPT,sizeof (SusResPayldEncryptReq_T),0xAAAA ,0xFFFF},
//    { CMD_SET_CH_SECURITY_KEYS,     PRIV_ADMIN,     SET_CH_SECURITY_KEYS,   0xFF,                           0xAAAA ,0xFFFF},
//    { CMD_GET_SYS_IFC_CAPS,         PRIV_USER,      GET_SYS_IFC_CAPS,       1,                              0xAAAA ,0xFFFF},

#endif  /* APP_DEVICE */
    { 0x00,                         0x00,           0x00,                   0x00,                           0x0000 ,         0x0000}
};

