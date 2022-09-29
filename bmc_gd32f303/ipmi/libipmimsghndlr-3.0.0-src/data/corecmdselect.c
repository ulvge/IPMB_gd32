/******************************************************************
 ******************************************************************
 ***                                                                                                           **
 ***    (C)Copyright 2013, American Megatrends Inc.                                     **
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
 * corecmdselect.c
 * OEM can enable or disable IPMI Command accoring to requirements
 *
 *  Author: Winston <winstont@ami.com>
 ******************************************************************/

 #include "Types.h"
#include "Debug.h"
#include "IPMI_App.h"
#include "IPMI_Chassis.h"
#include "IPMI_Bridge.h"
#include "IPMI_SensorEvent.h"
#include "IPMI_Storage.h"
#include "IPMI_DeviceConfig.h"
#include "IPMI_AMI.h"
#include "IPMIDefs.h"
#include "IPMI_OPMA.h"
#include "Apml.h"
#include "Pnm.h"
#include "IPMI_GroupExtn.h"
#include "cmdselect.h"
#include "IPMIConf.h"
#include "dbgout.h"
#include "PDKCmdsAccess.h"
#include "PDKCmdselect.h"


NetFnCmds_T g_coreApp [] = 
{
    /* IPM Device Commands */
    {CMD_GET_DEV_ID                                                 , ENABLED, NONE},
    {CMD_BROADCAST_GET_DEV_ID                             ,DISABLED, NONE},
    {CMD_COLD_RESET                                                 ,ENABLED, NONE},
    {CMD_WARM_RESET                                                ,ENABLED, NONE},
    {CMD_GET_SELF_TEST_RESULTS                              ,ENABLED, NONE},
    {CMD_MFG_TEST_ON                                               ,ENABLED, NONE},
    {CMD_SET_ACPI_PWR_STATE                                  ,ENABLED, NONE},
    {CMD_GET_ACPI_PWR_STATE                                 ,ENABLED, NONE},
    {CMD_GET_DEV_GUID                                            ,ENABLED, NONE},
    {CMD_GET_NETFN_SUP                                           ,ENABLED, NONE},
    {CMD_GET_CMD_SUP                                             ,ENABLED, NONE},
    {CMD_GET_SUBFN_SUP                                          ,ENABLED, NONE},
    {CMD_GET_CONFIG_CMDS                                     ,ENABLED, NONE},
    {CMD_GET_CONFIG_SUB_FNS                                ,ENABLED, NONE},
    {CMD_SET_CMD_ENABLES                                     ,ENABLED, NONE},
    {CMD_GET_CMD_ENABLES                                     ,ENABLED, NONE},
    {CMD_SET_SUBFN_ENABLES                                  ,DISABLED, NONE},
    {CMD_GET_SUBFN_ENABLES                                  ,DISABLED, NONE},
    {CMD_GET_OEM_NETFN_IANA_SUPPORT                 ,ENABLED, NONE},
    /*    WatchDog Timer Commands  */
    {CMD_RESET_WDT                                               ,ENABLED, NONE},
    {CMD_SET_WDT                                                   ,ENABLED, NONE},
    {CMD_GET_WDT                                                   ,ENABLED, NONE},

    /*    BMC Device and Messaging Commands   */
    { CMD_SET_BMC_GBL_ENABLES                           ,ENABLED, NONE},
    { CMD_GET_BMC_GBL_ENABLES                           ,ENABLED, NONE},
    { CMD_CLR_MSG_FLAGS                                      ,ENABLED, NONE},
    { CMD_GET_MSG_FLAGS                                      ,ENABLED, NONE},
    { CMD_ENBL_MSG_CH_RCV                                  ,ENABLED, NONE},
    { CMD_GET_MSG                                                 ,ENABLED, NONE},
    { CMD_SEND_MSG                                               ,ENABLED, NONE},
    { CMD_READ_EVT_MSG_BUFFER                           ,ENABLED, NONE},
    { CMD_GET_BTIFC_CAP                                       ,ENABLED, NONE},
    { CMD_GET_SYSTEM_GUID                                  ,ENABLED, NONE},
    { CMD_GET_CH_AUTH_CAP                                  ,ENABLED, NONE},
    { CMD_GET_SESSION_CHALLENGE                       ,ENABLED, NONE},
    { CMD_ACTIVATE_SESSION                                 ,ENABLED, NONE},
    { CMD_SET_SESSION_PRIV_LEVEL                       ,ENABLED, NONE},
    { CMD_CLOSE_SESSION                                      ,ENABLED, NONE},
    { CMD_GET_SESSION_INFO                                 ,ENABLED, NONE},
    { CMD_GET_AUTH_CODE                                     ,ENABLED, NONE},
    { CMD_SET_CH_ACCESS                                     ,ENABLED, NONE},
    { CMD_GET_CH_ACCESS                                     ,ENABLED, NONE},
    { CMD_GET_CH_INFO                                          ,ENABLED, NONE},
    { CMD_SET_USER_ACCESS                                 ,ENABLED, NONE},
    { CMD_GET_USER_ACCESS                                 ,ENABLED, NONE},
    { CMD_SET_USER_NAME                                     ,ENABLED, NONE},
    { CMD_GET_USER_NAME                                     ,ENABLED, NONE},
    { CMD_SET_USER_PASSWORD                            ,ENABLED, NONE},
    { CMD_MASTER_WRITE_READ                             ,ENABLED, NONE},
    { CMD_SET_SYSTEM_INFO_PARAM                      ,ENABLED, NONE},
    { CMD_GET_SYSTEM_INFO_PARAM                      ,ENABLED, NONE},

    /*------------------------ IPMI 2.0 specific Commands ------------------*/
    { CMD_ACTIVATE_PAYLOAD                               ,ENABLED, NONE},
    { CMD_DEACTIVATE_PAYLOAD                           ,ENABLED, NONE},
    { CMD_GET_PAYLD_ACT_STATUS                       ,ENABLED, NONE},
    { CMD_GET_PAYLD_INST_INFO                          ,ENABLED, NONE},
    { CMD_SET_USR_PAYLOAD_ACCESS                  ,ENABLED, NONE},
    { CMD_GET_USR_PAYLOAD_ACCESS                  ,ENABLED, NONE},
    { CMD_GET_CH_PAYLOAD_SUPPORT                  ,ENABLED, NONE},
    { CMD_GET_CH_PAYLOAD_VER                          ,ENABLED, NONE},
    { CMD_GET_CH_OEM_PAYLOAD_INFO                 ,ENABLED, NONE},
    { CMD_GET_CH_CIPHER_SUITES                        ,ENABLED, NONE},
    { CMD_SUS_RES_PAYLOAD_ENCRYPT                 ,ENABLED, NONE},
    { CMD_SET_CH_SECURITY_KEYS                       ,ENABLED, NONE},
    { CMD_GET_SYS_IFC_CAPS                              ,ENABLED, NONE},
    {0                                                                   ,0           , NONE },
    
};

NetFnCmds_T g_coreChassis [] =
{
    /*--------------------- Chassis Commands ---------------------------------*/
    { CMD_GET_CHASSIS_CAPABILITIES                          ,ENABLED, NONE},
    { CMD_GET_CHASSIS_STATUS                                   ,ENABLED, NONE},
    { CMD_CHASSIS_CONTROL                                        ,ENABLED, NONE},
    { CMD_CHASSIS_RESET                                             ,DISABLED, NONE},
    { CMD_CHASSIS_IDENTIFY                                        ,ENABLED, NONE},
    { CMD_SET_CHASSIS_CAPABILITIES                          ,ENABLED, NONE},
    { CMD_SET_POWER_RESTORE_POLICY                        ,ENABLED, NONE},
    { CMD_GET_SYSTEM_RESTART_CAUSE                        ,ENABLED, NONE},
    { CMD_SET_SYSTEM_BOOT_OPTIONS                          ,ENABLED, NONE},
    { CMD_GET_SYSTEM_BOOT_OPTIONS                          ,ENABLED, NONE},
    { CMD_GET_POH_COUNTER                                         ,ENABLED, NONE},
    { CMD_SET_FP_BTN_ENABLES                                     ,ENABLED, NONE},
    { CMD_SET_POWER_CYCLE_INTERVAL                         ,ENABLED, NONE},
    {0,                                                                            0             , NONE}

};

NetFnCmds_T g_coreBridge [] = 
{
    /*----------------------- Bridge Management Commands --------------------------------*/
    { CMD_GET_BRIDGE_STATE                                        ,ENABLED, NONE},
    { CMD_SET_BRIDGE_STATE                                        ,ENABLED, NONE},
    { CMD_GET_ICMB_ADDR                                            ,ENABLED, NONE},
    { CMD_SET_ICMB_ADDR                                            ,ENABLED, NONE},
    { CMD_SET_BRIDGE_PROXY_ADDR                             ,ENABLED, NONE},
    { CMD_GET_BRIDGE_STATISTICS                               ,ENABLED, NONE},
    { CMD_GET_ICMB_CAPABILITIES                                ,ENABLED, NONE},
    { CMD_CLEAR_BRIDGE_STATISTICS                           ,ENABLED, NONE},
    { CMD_GET_BRIDGE_PROXY_ADDR                             ,ENABLED, NONE},
    { CMD_GET_ICMB_CONNECTOR_INFO                          ,ENABLED, NONE},
    { CMD_GET_ICMB_CONNECTION_ID                            ,DISABLED, NONE},
    { CMD_SEND_ICMB_CONNECTION_ID                          ,DISABLED, NONE},

    /*---------------------- Bridge Discovery Commands -----------------------------------*/
    { CMD_PREPARE_FOR_DISCOVERY                              ,ENABLED, NONE},
    { CMD_GET_ADDRESSES                                            ,ENABLED, NONE},
    { CMD_SET_DISCOVERED                                           ,ENABLED, NONE},
    { CMD_GET_CHASSIS_DEVICE_ID                               ,ENABLED, NONE},
    { CMD_SET_CHASSIS_DEVICE_ID                               ,ENABLED, NONE},
    
    /*----------------------- Bridging Commands ------------------------------------------*/
    { CMD_BRIDGE_REQUEST                                           ,ENABLED, NONE},
    { CMD_BRIDGE_MESSAGE                                          ,ENABLED, NONE},
    
    /*---------------------- Bridge Event Commands ---------------------------------------*/
    { CMD_GET_EVENT_COUNT                                        ,ENABLED, NONE},
    { CMD_SET_EVENT_DESTINATION                              ,ENABLED, NONE},
    { CMD_SET_EVENT_RECEPTION_STATE                      ,ENABLED, NONE},
    { CMD_SEND_ICMB_EVENT_MESSAGE                        ,ENABLED, NONE},
    { CMD_GET_EVENT_DESTINATION                             ,ENABLED, NONE},
    { CMD_GET_EVENT_RECEPTION_STATE                      ,ENABLED, NONE},
    { 0                                                                           ,0           , NONE}

};

NetFnCmds_T g_coreSensor [] = 
{

    { CMD_SET_EVENT_RECEIVER                                   ,ENABLED, NONE},
    { CMD_GET_EVENT_RECEIVER                                   ,ENABLED, NONE},
    { CMD_PLATFORM_EVENT                                          ,ENABLED, NONE},

    { CMD_GET_PEF_CAPABILITIES                                 ,ENABLED, NONE},
    { CMD_ARM_PEF_POSTPONE_TIMER                           ,ENABLED, NONE},
    { CMD_SET_PEF_CONFIG_PARAMS                             ,ENABLED, NONE},
    { CMD_GET_PEF_CONFIG_PARAMS                             ,ENABLED, NONE},
    { CMD_SET_LAST_PROCESSED_EVENT_ID                  ,ENABLED, NONE},
    { CMD_GET_LAST_PROCESSED_EVENT_ID                  ,ENABLED, NONE},
    { CMD_ALERT_IMMEDIATE                                         ,ENABLED, NONE},
    { CMD_PET_ACKNOWLEDGE                                      ,ENABLED, NONE},


    { CMD_GET_DEV_SDR_INFO                                     ,ENABLED, NONE},
    { CMD_GET_DEV_SDR                                              ,ENABLED, NONE},
    { CMD_RESERVE_DEV_SDR_REPOSITORY                  ,ENABLED, NONE},
    { CMD_GET_SENSOR_READING_FACTORS                 ,ENABLED, NONE},
    { CMD_SET_SENSOR_HYSTERISIS                             ,ENABLED, NONE},
    { CMD_GET_SENSOR_HYSTERISIS                             ,ENABLED, NONE},
    { CMD_SET_SENSOR_THRESHOLDS                           ,ENABLED, NONE},
    { CMD_GET_SENSOR_THRESHOLDS                           ,ENABLED, NONE},
    { CMD_SET_SENSOR_EVENT_ENABLE                         ,ENABLED, NONE},
    { CMD_GET_SENSOR_EVENT_ENABLE                         ,ENABLED, NONE},
    { CMD_REARM_SENSOR_EVENTS                               ,ENABLED, NONE},
    { CMD_GET_SENSOR_EVENT_STATUS                        ,ENABLED, NONE},
    { CMD_GET_SENSOR_READING                                 ,ENABLED, NONE},
    { CMD_SET_SENSOR_TYPE                                        ,ENABLED, NONE},
    { CMD_GET_SENSOR_TYPE                                        ,ENABLED, NONE},
    { CMD_SET_SENSOR_READING                                  ,ENABLED, NONE},
    {0,                                                                           0             , NONE}
};

NetFnCmds_T g_coreStorage [] = 
{
    /*--------------------- FRU Device Commands ---------------------------------*/
    { CMD_FRU_INVENTORY_AREA_INFO                           ,ENABLED, NONE},
    { CMD_READ_FRU_DATA                                            ,ENABLED, NONE},
    { CMD_WRITE_FRU_DATA                                           ,ENABLED, NONE},

    /*--------------------- SDR Device Commands ---------------------------------*/
    { CMD_GET_SDR_REPOSITORY_INFO                           ,ENABLED, NONE},
    { CMD_GET_SDR_REPOSITORY_ALLOCATION_INFO       ,ENABLED, NONE},
    { CMD_RESERVE_SDR_REPOSITORY                             ,ENABLED, NONE},
    { CMD_GET_SDR                                                         ,ENABLED, NONE},
    { CMD_ADD_SDR                                                        ,ENABLED, NONE},
    { CMD_PARTIAL_ADD_SDR                                          ,ENABLED, NONE},
    { CMD_DELETE_SDR                                                    ,DISABLED, NONE},
    { CMD_CLEAR_SDR_REPOSITORY                                 ,ENABLED, NONE},
    { CMD_GET_SDR_REPOSITORY_TIME                            ,ENABLED, NONE},
    { CMD_SET_SDR_REPOSITORY_TIME                            ,DISABLED, NONE},
    { CMD_ENTER_SDR_REPOSITORY_UPDATE_MODE         ,DISABLED, NONE},
    { CMD_EXIT_SDR_REPOSITORY_UPDATE_MODE            ,DISABLED, NONE},
    { CMD_RUN_INITIALIZATION_AGENT                            ,ENABLED, NONE},

    /*--------------------- SEL Device Commands ----------------------------------*/
    { CMD_GET_SEL_INFO                                                  ,ENABLED, NONE},
    { CMD_GET_SEL_ALLOCATION_INFO                              ,ENABLED, NONE},
    { CMD_RESERVE_SEL                                                    ,ENABLED, NONE},
    { CMD_GET_SEL_ENTRY                                                 ,ENABLED, NONE},
    { CMD_ADD_SEL_ENTRY                                                ,ENABLED, NONE},
    { CMD_PARTIAL_ADD_SEL_ENTRY                                  ,ENABLED, NONE},
    { CMD_DELETE_SEL_ENTRY                                            ,ENABLED, NONE},
    { CMD_CLEAR_SEL                                                        ,ENABLED, NONE},
    { CMD_GET_SEL_TIME                                                   ,ENABLED, NONE},
    { CMD_SET_SEL_TIME                                                   ,ENABLED, NONE},
    { CMD_GET_AUXILIARY_LOG_STATUS                            ,DISABLED, NONE},
    { CMD_SET_AUXILIARY_LOG_STATUS                            ,DISABLED, NONE},
    { CMD_GET_SEL_TIME_UTC_OFFSET                              ,ENABLED, NONE},
    { CMD_SET_SEL_TIME_UTC_OFFSET                              ,ENABLED, NONE},
    { 0,                                                                             0             , NONE}

};

NetFnCmds_T g_coreTransport [] = 
{
    /*------------------------- IPM Device Commands --------------------------------------*/
    { CMD_SET_LAN_CONFIGURATION_PARAMETERS          ,ENABLED, NONE},
    { CMD_GET_LAN_CONFIGURATION_PARAMETERS          ,ENABLED, NONE},
    { CMD_SUSPEND_BMC_ARPS                                       ,ENABLED, NONE},
    { CMD_GET_IP_UDP_RMCP_STATISTICS                       ,DISABLED, NONE},

    /*--------------------- Serial/Modem Device Commands ---------------------------------*/
    { CMD_SET_SERIAL_MODEM_CONFIG                           ,ENABLED, NONE},
    { CMD_GET_SERIAL_MODEM_CONFIG                           ,ENABLED, NONE},
    { CMD_SET_SERIAL_MODEM_MUX                                ,ENABLED, NONE},
    { CMD_SERIAL_MODEM_CONNECTION_ACTIVITY          ,DISABLED, NONE},
    { CMD_CALLBACK                                                       ,DISABLED, NONE},
    { CMD_SET_USER_CALLBACK_OPTION                         ,DISABLED, NONE},
    { CMD_GET_USER_CALLBACK_OPTION                         ,DISABLED, NONE},
    { CMD_GET_TAP_RES_CODES                                      ,DISABLED, NONE},

    /*------------------------- Serial Over LAN Commands ---------------------------------*/
    { CMD_GET_SOL_CONFIGURATION                              ,ENABLED, NONE},
    { CMD_SET_SOL_CONFIGURATION                              ,ENABLED, NONE},
    { 0                                                                            ,0            , NONE}
};

NetFnCmds_T g_coreAMI [] = 
{
    /* ---------------YAFU Commands -----------------------------*/
    { CMD_AMI_YAFU_SWITCH_FLASH_DEVICE			    ,ENABLED, NONE},
    { CMD_AMI_YAFU_ACTIVATE_FLASH_DEVICE		    ,ENABLED, NONE},
    { CMD_AMI_YAFU_RESTORE_FLASH_DEVICE			    ,ENABLED, NONE},
    { CMD_AMI_YAFU_GET_FLASH_INFO                           ,ENABLED, NONE},
    { CMD_AMI_YAFU_GET_FIRMWARE_INFO                    ,ENABLED, NONE},
    { CMD_AMI_YAFU_GET_FMH_INFO                              ,ENABLED, NONE},
    { CMD_AMI_YAFU_GET_STATUS                                 ,ENABLED, NONE},
    { CMD_AMI_YAFU_ACTIVATE_FLASH                          ,ENABLED, NONE},
    { CMD_AMI_YAFU_ALLOCATE_MEMORY                       ,ENABLED, NONE},
    { CMD_AMI_YAFU_FREE_MEMORY                               ,ENABLED, NONE},
    { CMD_AMI_YAFU_READ_FLASH                                 ,ENABLED, NONE},
    { CMD_AMI_YAFU_WRITE_FLASH                                ,ENABLED, NONE},
    { CMD_AMI_YAFU_ERASE_FLASH                                ,ENABLED, NONE},
    { CMD_AMI_YAFU_PROTECT_FLASH                            ,ENABLED, NONE},
    { CMD_AMI_YAFU_ERASE_COPY_FLASH                      ,ENABLED, NONE},
    { CMD_AMI_YAFU_VERIFY_FLASH                               ,ENABLED, NONE},
    { CMD_AMI_YAFU_READ_MEMORY                              ,ENABLED, NONE},
    { CMD_AMI_YAFU_WRITE_MEMORY                             ,ENABLED, NONE},
    { CMD_AMI_YAFU_COPY_MEMORY                              ,ENABLED, NONE},
    { CMD_AMI_YAFU_COMPARE_MEMORY                        ,ENABLED, NONE},
    { CMD_AMI_YAFU_CLEAR_MEMORY                            ,ENABLED, NONE},
    { CMD_AMI_YAFU_GET_BOOT_CONFIG                       ,ENABLED, NONE},
    { CMD_AMI_YAFU_SET_BOOT_CONFIG                       ,ENABLED, NONE},
    { CMD_AMI_YAFU_GET_BOOT_VARS                          ,ENABLED, NONE},
    { CMD_AMI_YAFU_DEACTIVATE_FLASH_MODE            ,ENABLED, NONE},
    { CMD_AMI_YAFU_RESET_DEVICE                             ,ENABLED, NONE},
    { CMD_AMI_YAFU_GET_ECF_STATUS                         ,ENABLED, NONE},
    { CMD_AMI_YAFU_GET_VERIFY_STATUS                    ,ENABLED, NONE},
    { CMD_AMI_YAFU_DUAL_IMAGE_SUP                        ,ENABLED, NONE},
    { CMD_AMI_GET_CHANNEL_NUM                               ,ENABLED, NONE},
    { CMD_AMI_GET_ETH_INDEX                                    ,ENABLED, NONE},
    { CMD_AMI_YAFU_FIRMWARE_SELECT_FLASH            ,ENABLED, NONE},
    { CMD_AMI_YAFU_SIGNIMAGEKEY_REPLACE              ,ENABLED, NONE},
    { CMD_AMI_GET_FW_VERSION                         ,ENABLED, NONE},
    { CMD_AMI_FILE_UPLOAD                           , ENABLED, NONE},
    { CMD_AMI_FILE_DOWNLOAD                         , ENABLED, NONE},
    { CMD_AMI_YAFU_MISCELLANEOUS_INFO               ,ENABLED, NONE },
    /*-------------------- AMI Smtp Commands -----------------------*/
    { CMD_SET_SMTP_CONFIG_PARAMS                         ,ENABLED, NONE},
    { CMD_GET_SMTP_CONFIG_PARAMS                         ,ENABLED, NONE},
    { CMD_AMI_GET_EMAIL_USER                                  ,ENABLED, NONE},
    { CMD_AMI_SET_EMAIL_USER                                  ,ENABLED, NONE},
    { CMD_AMI_GET_EMAILFORMAT_USER                      ,ENABLED, NONE},
    { CMD_AMI_SET_EMAILFORMAT_USER                      ,ENABLED, NONE},
    { CMD_AMI_RESET_PASS                                         ,ENABLED, NONE},
    { CMD_AMI_RESTORE_DEF                                       ,ENABLED, NONE},
    { CMD_AMI_GET_LOG_CONF                                     ,ENABLED, NONE},
    { CMD_AMI_SET_LOG_CONF                                     ,ENABLED, NONE},

    /*-------------------- AMI Get Bios Code Commands -----------------------------------------*/
    { CMD_AMI_GET_BIOS_CODE                                   ,ENABLED, NONE},
    { CMD_AMI_SEND_TO_BIOS                                    ,ENABLED, NONE},
    { CMD_AMI_GET_BIOS_COMMAND                                ,ENABLED, NONE},
    { CMD_AMI_SET_BIOS_RESPONSE                               ,ENABLED, NONE},
    { CMD_AMI_GET_BIOS_RESPONSE                               ,ENABLED, NONE},
    { CMD_AMI_SET_BIOS_FLAG                                   ,ENABLED, NONE},
    { CMD_AMI_GET_BIOS_FLAG                                   ,ENABLED, NONE},
   
  /*----------------------AMI Firmware Update Command --------------------------------------*/
    { CMD_AMI_FIRMWAREUPDATE                                  ,ENABLED, NONE},
    { CMD_AMI_GETRELEASENOTE                                      ,ENABLED, NONE},

 
    /*-------------------- AMI SERVICE Commands -----------------------------------------*/
    { CMD_AMI_GET_SERVICE_CONF                              ,ENABLED, NONE},
    { CMD_AMI_SET_SERVICE_CONF                              ,ENABLED, NONE},
    { CMD_AMI_LINK_DOWN_RESILENT                           ,ENABLED, NONE},

    /*-------------------- AMI DNS Commands -----------------------------------------*/
    { CMD_AMI_GET_DNS_CONF                                     ,ENABLED, NONE},
    { CMD_AMI_SET_DNS_CONF                                     ,ENABLED, NONE},

    /*-------------------- AMI Iface State Commands -----------------------------------------*/
    { CMD_AMI_GET_IFACE_STATE                                 ,ENABLED, NONE},
    { CMD_AMI_SET_IFACE_STATE                                 ,ENABLED, NONE},

    { CMD_AMI_SET_FIREWALL                                      ,ENABLED, NONE},
    { CMD_AMI_GET_FIREWALL                                      ,ENABLED, NONE},
    { CMD_AMI_GET_FRU_DETAILS                                 ,ENABLED, NONE},
    { CMD_AMI_GET_ROOT_USER_ACCESS                     ,ENABLED, NONE},
    { CMD_AMI_SET_ROOT_PASSWORD                          ,ENABLED, NONE},
    { CMD_AMI_GET_USER_SHELLTYPE                           ,ENABLED, NONE},
    { CMD_AMI_SET_USER_SHELLTYPE                           ,ENABLED, NONE},
    { CMD_AMI_SET_EXTENDED_PRIV                           ,ENABLED, NONE},
    { CMD_AMI_GET_EXTENDED_PRIV                           ,ENABLED, NONE},
    { CMD_AMI_SET_TIMEZONE                                   ,ENABLED, NONE},
    { CMD_AMI_GET_TIMEZONE                                   ,ENABLED, NONE},
    { CMD_AMI_GET_NTP_CFG                                 ,ENABLED, NONE},
    { CMD_AMI_SET_NTP_CFG                                 ,ENABLED, NONE},
    { CMD_AMI_SET_TRIGGER_EVT                                ,ENABLED, NONE},
    { CMD_AMI_GET_TRIGGER_EVT                                ,ENABLED, NONE},
    { CMD_AMI_GET_SOL_CONFIG_PARAMS                          ,ENABLED, NONE},
    { CMD_AMI_SET_LOGIN_AUDIT_CFG,      ENABLED , NONE}, 
    { CMD_AMI_GET_LOGIN_AUDIT_CFG,      ENABLED , NONE},
    { CMD_AMI_GET_IPV6_ADDRESS,         ENABLED , NONE},
    { CMD_AMI_SET_SNMP_CONF, ENABLED , NONE},
    { CMD_AMI_GET_SNMP_CONF, ENABLED , NONE},
    { CMD_AMI_SET_PAM_ORDER,            ENABLED , NONE},
    { CMD_AMI_GET_PAM_ORDER,            ENABLED , NONE},
    { CMD_AMI_GET_CHANNEL_TYPE,         ENABLED , NONE},

    /*------------------------------ AMI SEL Commands ------------------------------*/
    { CMD_AMI_GET_SEL_POLICY,           ENABLED , NONE},
    { CMD_AMI_SET_SEL_POLICY,           ENABLED , NONE},
    { CMD_AMI_SET_PRESERVE_CONF,        ENABLED , NONE}, 
    { CMD_AMI_GET_PRESERVE_CONF,        ENABLED , NONE}, 
    { CMD_AMI_GET_SEL_ENTIRES,          ENABLED , NONE},
    { CMD_AMI_GET_SENSOR_INFO,          ENABLED , NONE},
    { CMD_AMI_SET_ALL_PRESERVE_CONF,    ENABLED , NONE},
    { CMD_AMI_GET_ALL_PRESERVE_CONF,    ENABLED , NONE},

    /*------------------------------- TFTP Firmware Update -------------------------*/
    { CMD_AMI_START_TFTP_FW_UPDATE,     ENABLED , NONE},
    { CMD_AMI_GET_TFTP_FW_PROGRESS_STATUS, ENABLED , NONE},
    { CMD_AMI_SET_FW_CONFIGURATION,     ENABLED , NONE},
    { CMD_AMI_GET_FW_CONFIGURATION,     ENABLED , NONE},
    { CMD_AMI_SET_FW_PROTOCOL,          ENABLED , NONE},
    { CMD_AMI_GET_FW_PROTOCOL,          ENABLED , NONE},
    { CMD_AMI_GET_IPMI_SESSION_TIMEOUT,  ENABLED, NONE},
    
    /*------------------------------- UDS Channel Info Command ---------------------*/
    { CMD_AMI_GET_UDS_CHANNEL_INFO,     ENABLED , NONE},
    { CMD_AMI_GET_UDS_SESSION_INFO,     ENABLED , NONE},
    
    /*-------------------------------AMI Dual Image Support Command-----------------*/
    { CMD_AMI_DUAL_IMG_SUPPORT,            ENABLED , NONE},

    /*---------- Password Encryption Key--------- */
    { CMD_AMI_SET_PWD_ENCRYPTION_KEY,     ENABLED , NONE},

    /*-------------------------------U-Boot Memory Test-----------------------------*/
    { CMD_AMI_SET_UBOOT_MEMTEST,            ENABLED, NONE},
    { CMD_AMI_GET_UBOOT_MEMTEST_STATUS,     ENABLED, NONE},

    /*-------------------- AMI Remote Images serviceCommands ----------------------------*/
    { CMD_AMI_GET_RIS_CONF                              ,ENABLED, NONE},
    { CMD_AMI_SET_RIS_CONF                              ,ENABLED, NONE},
    { CMD_AMI_RIS_START_STOP                              ,ENABLED, NONE},
    { CMD_AMI_MEDIA_REDIRECTION_START_STOP                ,ENABLED, NONE},
    { CMD_AMI_GET_MEDIA_INFO                              ,ENABLED, NONE},
    { CMD_AMI_SET_MEDIA_INFO                              ,ENABLED, NONE},
    /*---------------------AMI Control Debug Messages Commands-----------------------*/
    { CMD_AMI_CTL_DBG_MSG,                   ENABLED , NONE},
    { CMD_AMI_GET_DBG_MSG_STATUS,            ENABLED , NONE},
    { CMD_AMI_VIRTUAL_DEVICE_SET_STATUS,	ENABLED , NONE},
    { CMD_AMI_VIRTUAL_DEVICE_GET_STATUS,	ENABLED , NONE},
    
    { CMD_AMI_GET_LICENSE_VALIDITY,                                     ENABLED, NONE},
    { CMD_AMI_ADD_LICENSE_KEY,                                          ENABLED, NONE},
    { CMD_AMI_PECI_READ_WRITE,                                          ENABLED,NONE},
    { CMD_AMI_GET_REMOTEKVM_CONF,                                          ENABLED,NONE},
    { CMD_AMI_SET_REMOTEKVM_CONF,                                          ENABLED,NONE},
    { CMD_AMI_GET_VMEDIA_CONF,                                          ENABLED,NONE},
    { CMD_AMI_SET_VMEDIA_CONF,                                          ENABLED,NONE},
    { CMD_AMI_GET_SSL_CERT_STATUS,                                       ENABLED,NONE},
    { CMD_AMI_GET_LDAP_CONF,                                            ENABLED,NONE},
    { CMD_AMI_SET_LDAP_CONF,                                            ENABLED,NONE},
    { CMD_AMI_GET_AD_CONF,                                          ENABLED,NONE},
    { CMD_AMI_SET_AD_CONF,                                          ENABLED,NONE},
    { CMD_AMI_GET_RADIUS_CONF,                                      ENABLED,NONE},
    { CMD_AMI_SET_RADIUS_CONF,                                      ENABLED,NONE},

    { CMD_AMI_GET_SDCARD_PART,                                      ENABLED,NONE},
    { CMD_AMI_SET_SDCARD_PART,                                      ENABLED,NONE},



    /*******************AMI Host_loakc feature command*****************************/
    {CMD_AMI_GET_HOST_LOCK_FEATURE_STATUS,              ENABLED, NONE},
    {CMD_AMI_SET_HOST_LOCK_FEATURE_STATUS,              ENABLED, NONE},
    {CMD_AMI_GET_HOST_AUTO_LOCK_STATUS,                  ENABLED, NONE},
    {CMD_AMI_SET_HOST_AUTO_LOCK_STATUS,                  ENABLED, NONE},
    /*******************AMI Active Sessions Management commands*********************/
    {CMD_AMI_GET_ALL_ACTIVE_SESSIONS,                       ENABLED, NONE},
    {CMD_AMI_ACTIVE_SESSIONS_CLOSE,                          ENABLED, NONE},

    {CMD_AMI_GET_VIDEO_RCD_CONF,                              ENABLED, NONE},
    {CMD_AMI_SET_VIDEO_RCD_CONF,                              ENABLED, NONE},
    /*******************AMI Run Time Single Port Status Commands********************/
    {CMD_AMI_GET_RUN_TIME_SINGLE_PORT_STATUS,       ENABLED, NONE},
    {CMD_AMI_SET_RUN_TIME_SINGLE_PORT_STATUS,       ENABLED, NONE},
	{ CMD_AMI_ADD_EXTEND_SEL_ENTIRES,   ENABLED, NONE },
	{ CMD_AMI_GET_EXTEND_SEL_DATA   ,   ENABLED, NONE },
	{ CMD_AMI_PARTIAL_ADD_EXTEND_SEL_ENTIRES,  ENABLED, NONE },
	{ CMD_AMI_PARTIAL_GET_EXTEND_SEL_ENTIRES,  ENABLED, NONE },
    { CMD_AMI_GET_FEATURE_STATUS   ,   ENABLED, NONE },
    /*****************AMI Backup-Restore Configuration commands*********************/
    {CMD_AMI_SET_BACKUP_FLAG,										ENABLED, NONE},
    {CMD_AMI_GET_BACKUP_FLAG,										ENABLED, NONE},
    {CMD_AMI_MANAGE_BMC_CONFIG,										ENABLED, NONE},
    /****************************Restart web service*****************************/
    {CMD_AMI_RESTART_WEB_SERVICE,										ENABLED, NONE},
    {CMD_AMI_GET_PEND_STATUS,										ENABLED, NONE},
    {CMD_AMI_PLDM_BIOS_MSG,										ENABLED, NONE},
    /******************AMI SMBMC Commands ***********************************/
    {CMD_AMI_GET_BMC_INSTANCE_COUNT,							ENABLED, NONE},
    {CMD_AMI_GET_USB_SWITCH_SETTING,							ENABLED, NONE},
    {CMD_AMI_SET_USB_SWITCH_SETTING,							ENABLED, NONE},

    {CMD_AMI_MUX_SWITCHING,										ENABLED, NONE},
    {CMD_AMI_GET_RAID_INFO,                                     ENABLED, NONE},

    {0                                                                           ,0            , NONE}
};

NetFnCmds_T g_coreopma1 [] = 
{
    {  CMD_OPMA_SET_SENSOR_RD_OFFSET                  ,ENABLED, NONE},
    {  CMD_OPMA_GET_SENSOR_RD_OFFSET                  ,ENABLED, NONE},
    {  0                                                                         ,0             , NONE}

};

NetFnCmds_T g_coreopma2 [] =
{
    {  CMD_OPMA_SET_SYS_TYPE_ID                             ,ENABLED, NONE},
    {  CMD_OPMA_GET_SYS_TYPE_ID                             ,ENABLED, NONE},
    {  CMD_OPMA_GET_MCARD_CAP                              ,ENABLED, NONE},
    {  CMD_OPMA_CLR_CMOS                                        ,ENABLED, NONE},
    {  CMD_OPMA_SET_LOCAL_LOCKOUT                        ,ENABLED, NONE},
    {  CMD_OPMA_GET_LOCAL_LOCKOUT                        ,ENABLED, NONE},
    {  CMD_OPMA_GET_SUPPORTED_HOST_IDS               ,ENABLED, NONE},
    {  0                                                                          ,0             , NONE}
};

NetFnCmds_T g_coreapml [] = 
{
    {CMD_APML_GET_INTERFACE_VERSION                     ,ENABLED , NONE},
    {CMD_APML_READ_RMI_REG                                     ,ENABLED, NONE},
    {CMD_APML_WRITE_RMI_REG                                   ,ENABLED, NONE},
    {CMD_APML_READ_CPUID                                        ,ENABLED, NONE},
    {CMD_APML_READ_HTC_REG                                    ,ENABLED, NONE},
    {CMD_APML_WRITE_HTC_REG                                   ,ENABLED, NONE},
    {CMD_APML_READ_PSTATE                                       ,ENABLED, NONE},
    {CMD_APML_READ_MAX_PSTATE                               ,ENABLED, NONE},
    {CMD_APML_READ_PSTATE_LIMIT                             ,ENABLED, NONE},
    {CMD_APML_WRITE_PSTATE_LIMIT                            ,ENABLED, NONE},
    {CMD_APML_READ_MCR                                            ,ENABLED, NONE},
    {CMD_APML_WRITE_MCR                                          ,ENABLED, NONE},
    {CMD_APML_READ_TSI_REG                                     ,ENABLED, NONE},
    {CMD_APML_WRITE_TSI_REG                                    ,ENABLED, NONE},
    {CMD_APML_READ_TDP_LIMIT_REG                               ,ENABLED, NONE}, 
    {CMD_APML_WRITE_TDP_LIMIT_REG                              ,ENABLED, NONE},
    {CMD_APML_READ_PROCESSOR_POWER_REG                         ,ENABLED, NONE},
    {CMD_APML_READ_POWER_AVERAGING_REG                         ,ENABLED, NONE},
    {CMD_APML_READ_DRAM_THROTTLE_REG                           ,ENABLED, NONE},
    {CMD_APML_WRITE_DRAM_THROTTLE_REG                          ,ENABLED, NONE},
    {0                                                                            ,0           , NONE}

};

NetFnCmds_T g_coredcmi [] = 
{
    /* DCMI Commands */
    { CMD_GET_DCMI_CAPABILITY_INFO                          ,ENABLED, NONE},
    { CMD_GET_POWER_READING                                     ,ENABLED, NONE},
    { CMD_GET_POWER_LIMIT                                           ,ENABLED, NONE},
    { CMD_SET_POWER_LIMIT                                           ,ENABLED, NONE},
    { CMD_ACTIVATE_POWER_LIMIT                                  ,ENABLED, NONE},
    { CMD_GET_ASSET_TAG                                              ,ENABLED, NONE},
    { CMD_GET_DCMI_SENSOR_INFO                                 ,ENABLED, NONE},
    { CMD_SET_ASSET_TAG                                              ,ENABLED, NONE},
    { CMD_GET_MANAGEMENT_CONTROLLER_ID_STRING   ,ENABLED, NONE},
    { CMD_SET_MANAGEMENT_CONTROLLER_ID_STRING   ,ENABLED, NONE},
    {CMD_SET_THERMAL_LIMIT                                        ,ENABLED, NONE},
    {CMD_GET_THERMAL_LIMIT                                        ,ENABLED, NONE},
    {CMD_GET_TEMPERATURE_READING                           ,ENABLED, NONE},
    {CMD_SET_DCMI_CONF_PARAMS                                ,ENABLED, NONE},
    {CMD_GET_DCMI_CONF_PARAMS                                ,ENABLED, NONE},
    { 0                                                                             ,0          , NONE}

};

NetFnCmds_T g_corehpm [] = 
{
    { CMD_GET_TARGET_UPLD_CAPABLITIES,              ENABLED, NONE},
    { CMD_GET_COMPONENT_PROPERTIES,                 ENABLED, NONE},
    { CMD_INITIATE_UPG_ACTION,                      ENABLED, NONE},
    { CMD_QUERY_SELF_TEST_RESULTS,                  ENABLED, NONE},
    { CMD_ABORT_FIRMWARE_UPGRADE,                   ENABLED, NONE},
    { CMD_UPLOAD_FIRMWARE_BLOCK,                    ENABLED, NONE},
    { CMD_FINISH_FIRMWARE_UPLOAD,                   ENABLED, NONE},
    { CMD_GET_UPGRADE_STATUS,                       ENABLED, NONE},
    { CMD_ACTIVATE_FIRMWARE,                        ENABLED, NONE},
    { CMD_QUERY_ROLLBACK_STATUS,                    ENABLED, NONE},
    { CMD_INITIATE_MANUAL_ROLLBACK,                 ENABLED, NONE},
    { 0,                                            0      , NONE}

};

NetFnCmds_T g_corepnm [] =
{
    {CMD_PNM_OEM_GET_READING                                 ,ENABLED, NONE},
    {CMD_PNM_OEM_ME_POWER_STATE_CHANGE             ,ENABLED, NONE},
    {0                                                                             ,0           , NONE}
};

NetFnCmds_T g_coressi [] =
{
    { CMD_SSICB_GET_COMPUTE_BLADE_PROPERTIES,   ENABLED, NONE},
    { CMD_SSICB_GET_ADDR_INFO,                  ENABLED, NONE},
    { CMD_SSICB_PLATFORM_EVENT_MESSAGE,         ENABLED, NONE},
    { CMD_SSICB_MODULE_BMI_CONTROL,             ENABLED, NONE},
    { CMD_SSICB_MODULE_PAYLOAD_CONTROL,         ENABLED, NONE},
    { CMD_SSICB_SET_SYSTEM_EVENT_LOG_POLICY,    ENABLED, NONE},
    { CMD_SSICB_SET_MODULE_ACTIVATION_POLICY,   ENABLED, NONE},
    { CMD_SSICB_GET_MODULE_ACTIVATION_POLICY,   ENABLED, NONE},
    { CMD_SSICB_SET_MODULE_ACTIVATION,          ENABLED, NONE},
    { CMD_SSICB_SET_POWER_LEVEL,                ENABLED, NONE},
    { CMD_SSICB_GET_POWER_LEVEL,                ENABLED, NONE},
    { CMD_SSICB_RENEGOTIATE_POWER,              ENABLED, NONE},
    { CMD_SSICB_GET_SERVICE_INFO,               ENABLED, NONE},
    { CMD_SSICB_GET_APPLET_PACKAGE_URI,         ENABLED, NONE},
    { CMD_SSICB_GET_SERVICE_ENABLE_STATE,       ENABLED, NONE},
    { CMD_SSICB_SET_SERVICE_ENABLE_STATE,       ENABLED, NONE},
    { CMD_SSICB_SET_SERVICE_TICKET,             ENABLED, NONE},
    { CMD_SSICB_STOP_SERVICE_SESSION,           ENABLED, NONE},
    { 0,                                        0, NONE}
};



// config "is enable"
NETFNTable_T CoreNetfntbl [] = 
{

    { NETFN_APP,        GRPEXT_NA,      g_coreApp       },
    { NETFN_CHASSIS,    GRPEXT_NA,      g_coreChassis   },
    { NETFN_BRIDGE,     GRPEXT_NA,      g_coreBridge    },
    { NETFN_SENSOR,     GRPEXT_NA,      g_coreSensor    },
    { NETFN_STORAGE,    GRPEXT_NA,      g_coreStorage   },
    { NETFN_TRANSPORT,  GRPEXT_NA,      g_coreTransport },
    { NETFN_AMI,        GRPEXT_NA,      g_coreAMI       },
    { NETFN_OPMA1,      GRPEXT_NA,      g_coreopma1     },
    { NETFN_OPMA2,      GRPEXT_NA,      g_coreopma2     },
    { NETFN_APML,       GRPEXT_NA,      g_coreapml      },
    { NETFN_HPM,        GRPEXT_HPM,     g_corehpm       },
    { NETFN_DCMI,       GRPEXT_DCMI,    g_coredcmi      },
    { NETFN_PNM,        GRPEXT_NA,      g_corepnm       },
    { NETFN_SSI,        GRPEXT_SSI,     g_coressi       },

};

/*
*@fn GetCommanEnabledStatus
*@brief This function finds whether the command is enabled by the OEM
*@param NetFunction - Pointer to the structure of NetFnCmds_T
*@param Cmd - Command Number
*@return Returns 0 on Enabled commands
*            Returns -1 on non-Enabled commands
*/
int GetCommanEnabledStatus(NETFNTable_T *NetFuntbl,INT8U Cmd)
{
    int i=0;

    while(NetFuntbl->NetFunction[i].Status != 0)
    {
        if(NetFuntbl->NetFunction[i].CmdNum ==Cmd)
        {
            if(NetFuntbl->NetFunction[i].Status ==ENABLED)
            {
                return 0;
            }
            else
            {
                return 0xFF;
            }
        }
        i++;
    }
    return -1;
}

/*
*@fn IsCommandEnabled
*@brief This function gives the status of Command
*@param NetFn         - Net Function 
*@param GroupExtnCode - Group Extension Code
*@param Cmd           - Command Number
*@return Returns 0 if the command is Enabled
*            Returns -1 if the command is Disabled
*            Returns 0xFF when 
*/
int IsCommandEnabled(INT8U NetFn,INT8U* GroupExtnCode,INT8U Cmd,int BMCInst)
{
    int i=0,OPMAEnabled = 0,ret =0;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    NETFNTable_T *Netfntbl= NULL;
    int found = 0;

    if(g_PDKCmdsHandle[PDKCMDS_PDKGETNETFNHNDLR] != NULL)
    {
        Netfntbl = ((NETFNTable_T*(*)())g_PDKCmdsHandle[PDKCMDS_PDKGETNETFNHNDLR])() ;
    }
    else
    {
        return -1;
    }

    while((i<MAX_NET_FN) && (Netfntbl[i].NetFunction != NULL ))
    {
        if(NetFn == Netfntbl[i].NetFn)
        {
            if((pBMCInfo->IpmiConfig.OPMASupport != 1) && (OPMAEnabled == 0) && (NetFn == NETFN_PNM))
            {
                OPMAEnabled = 1;
            }
            else
            {
                if (Netfntbl[i].NetFn == NETFN_DCMI)
                {
                    if(GroupExtnCode != NULL)
                    {
                        if (Netfntbl[i].GroupExtCode == *GroupExtnCode)
                        {
                        	found = 1;
                            break;
                        }
                    }
                }
                else
                {
                	found = 1;
                    break;
                }
            }
        }
        i++;
    }

    if(found)
    {
        ret = GetCommanEnabledStatus((NETFNTable_T*)&Netfntbl[i],Cmd);
        if(ret == 0xFF)
        {
            return 0xFF;
        }
        else
        {
            return ( 0 == ret) ? 0 : -1;
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

/*
*@fn GetCommandEnabledStatus
*@brief This function gives the status of Command
*@param NetFn         - Net Function 
*@param GroupExtnCode - Group Extension Code
*@param Cmd           - Command Number
*@return Returns 0 if the command is Enabled
*            Returns -1 if the command is Disabled
*/
int GetCommandEnabledStatus(INT8U NetFn,INT8U* GroupExtnCode,INT8U Cmd,int BMCInst)
{
    int i=0,OPMAEnabled = 0,ret =0,retstatus=0;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    ret = IsCommandEnabled(NetFn,GroupExtnCode,Cmd,BMCInst);
    if(ret < 0)
    {
        while(i<(sizeof(CoreNetfntbl)/sizeof(NETFNTable_T)))
        {
            if(NetFn == CoreNetfntbl[i].NetFn)
            {
                if((pBMCInfo->IpmiConfig.OPMASupport != 1) && (OPMAEnabled == 0) && (NetFn == NETFN_PNM))
                {
                    OPMAEnabled = 1;
                }
                else
                {
                    if (CoreNetfntbl[i].NetFn == NETFN_DCMI)
                    {
                        if(GroupExtnCode != NULL)
                        {
                            if (CoreNetfntbl[i].GroupExtCode == *GroupExtnCode)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            i++;
        }

        if(i != sizeof(CoreNetfntbl)/sizeof(NETFNTable_T))
        {
            retstatus = GetCommanEnabledStatus((NETFNTable_T*)&CoreNetfntbl[i],Cmd);
            if(retstatus == 0xFF || retstatus < 0)
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if(ret == 0xFF)
    {
        return -1;
    }

    return 0;
}


/*
*@fn GetMaskValue
*@brief Helps in retrieving the mask value for a particular command
*@param NetFunc - Net Function
*@param Cmd  Command number
*@return Returns the mask value
*/
int GetMaskValue(INT8U NetFunc, INT8U Cmd)
{
        int i=0, mask;
        NETFNTable_T *NetFnTbl = NULL;

        if(g_PDKCmdsHandle[PDKCMDS_PDKGETNETFNHNDLR] != NULL)
        {
            NetFnTbl = ((NETFNTable_T*(*)())g_PDKCmdsHandle[PDKCMDS_PDKGETNETFNHNDLR])() ;
        }
        else
        {
            return 0;
        }

        for(;i < MAX_NET_FN;i++)
        {
                if(NetFnTbl[i].NetFn == NetFunc)
                {
                        NetFnTbl = (NETFNTable_T*)&NetFnTbl[i];
                        break;
                }
        }

        for(i=0; NetFnTbl->NetFunction[i].Status != 0; i++)
        {
                if(NetFnTbl->NetFunction[i].CmdNum == Cmd)
                {
                        mask = NetFnTbl->NetFunction[i].CmdMask;
                        return mask;
                }
        }
        return 0;
}

/*
*@fn LogIPMICmd
*@brief This function logs request and response of IPMI command to volatile and extended log
*@param priority - 
*@param ReqRes - Denotes Request or Response of IPMI command
*@param NetFn - Net Function
*@param Cmd - Command Number
*@param ChannelNum - Channel Number
*@param ReqResLen - Request or Response Length
*@param pReqRes -Request/Response of IPMI command 
*/
int LogIPMICmd(int priority, char *ReqRes, INT8U NetFn, INT8U Cmd, INT8U ChannelNum, INT8U ReqResLen, unsigned char *pReqRes ) 
{
    int i,buffLen = 0,mask;
    char logMsgBuff[MAX_SYSLOG_MSG] = {0}; 

    mask = GetMaskValue(NetFn,Cmd); 
    if(strcmp(ReqRes, "Response:") == 0 && g_corefeatures.log_all_response_commands == DISABLED ) 
    {
        if((mask & 0x04) != 4)
        {
            return 1;		
        }
    }

    if(ReqResLen > 0)
        buffLen += snprintf(logMsgBuff, MAX_SYSLOG_MSG, "%s Channel:%x; Netfn:%x; Cmd:%x; Data:", ReqRes, ChannelNum, NetFn, Cmd);
    else
        buffLen += snprintf(logMsgBuff, MAX_SYSLOG_MSG, "%s Channel:%x; Netfn:%x; Cmd:%x;", ReqRes, ChannelNum, NetFn, Cmd);
                 
    for (i =0;i < ReqResLen;i++)
    {
        if ( buffLen >= MAX_SYSLOG_MSG)
        {
            break;
        }
        buffLen += snprintf(logMsgBuff + buffLen, MAX_SYSLOG_MSG-buffLen, "%x ", *(pReqRes+ i ));
    }
    if(buffLen < MAX_SYSLOG_MSG)
    {
        logMsgBuff[buffLen] = '\0';
    }
    else
    {
        logMsgBuff[MAX_SYSLOG_MSG - 1] = '\0';
    }
    //Check to see if "Log all command" feature is enabled
    if(g_corefeatures.log_all_command == ENABLED)
    {
        if((g_corefeatures.log_all_request_commands == ENABLED) && (strcmp(ReqRes,"Request:") == 0))
        {
            TEXTLOG(priority, "%s", logMsgBuff);
        }
        else if((g_corefeatures.log_all_response_commands == ENABLED) && (strcmp(ReqRes,"Response:") == 0)) //check to see if "Log all Response data" feature is enabled
        {
            TEXTLOG(priority, "%s", logMsgBuff);
        }
        fflush(stdout);
        return 1;
    }
    else
    {
        //Check which Mask bits are set and log the command accordingly
        if((mask & EXT_REQ_LOG) && (strcmp(ReqRes,"Request:") == 0))
        {
            TEXTLOG(priority, "%s", logMsgBuff);	//Log to Extended Log file
        }
        else if((mask & EXT_RES_LOG) && (strcmp(ReqRes,"Response:") == 0))
        {
            TEXTLOG(priority, "%s", logMsgBuff);
        }

        if(mask & VOL_REQ_RES_LOG)
        {
            TINFO("%s", logMsgBuff);                        //Log to /var/log
        }

        return 1;
    }

    return 0;
}


