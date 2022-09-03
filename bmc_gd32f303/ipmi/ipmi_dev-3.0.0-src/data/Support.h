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
 *****************************************************************
 ******************************************************************
 *
 * support.h
 * supported commands Macros
 *
 *  Author: Basavaraj Astekar <basavaraja@ami.com>
 *
 ******************************************************************/
#ifndef SUPPORT_H
#define SUPPORT_H
#include "Types.h"
#include "IPMDevice.h"

/*---------------------------------------------------------------------------*
 * DEVICES SUPPORTED
 *---------------------------------------------------------------------------*/
#define IPM_DEVICE					1
#define APP_DEVICE					1
#define CHASSIS_DEVICE				0

#define EVENT_PROCESSING_DEVICE		0
#define PEF_DEVICE					0
#define SENSOR_DEVICE				1

#define SDR_DEVICE					1
#define	SEL_DEVICE					0
#define FRU_DEVICE					0

#define BRIDGE_DEVICE				1

#define AMI_DEVICE				    1

#define OEM_DEVICE				    1

#define PICMG_DEVICE				1
/*---------------------------------------------------------------------------*
 * FEATURE SUPPORTED
 *---------------------------------------------------------------------------*/
#define FW_UPGRADE					1
#define FRB_SUPPORT					0
#define TERMINAL_MODE_SUPPORT       1
#define INTERNAL_PSGOOD_MONITORING  0
#define NO_WDT_PRETIMEOUT_INTERRUPT 0


/*---------------------------------------------------------------------------*
 * IPMI 2.0 SPECIFIC DEFINITIONS
 *---------------------------------------------------------------------------*/
#define IPMI20_SUPPORT				1
#define MAX_SOL_IN_PAYLD_SIZE       252
#define MAX_SOL_OUT_PAYLD_SIZE      252
#define MAX_IN_PAYLD_SIZE			1024
#define MAX_OUT_PAYLD_SIZE			1024
#define MAX_PYLDS_SUPPORT			2
#define MAX_PAYLD_INST				15  /* 1 to 15 only */
#define SYS_SERIAL_PORT_NUM			0

/*---------------------------------------------------------------------------*
 * OEM CONFIGURATION DATA SUPPORTED & CONFIGURATION DATA SIZE
 *---------------------------------------------------------------------------*/
#define	OEM_CONFIG_DATA_SUPPORTED	1
#define	MAX_OEM_CONFIG_DATA_SIZE	100


/*-------------------------------------------------------------------------*
 * Define the Unimplemented function based on the choice
 *-------------------------------------------------------------------------*/
#if defined UNIMPLEMENTED_AS_FUNC

#define UNIMPLEMENTED   UnImplementedFunc
extern int UnImplementedFunc (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);

#elif defined AMI_UNIMPLEMENTED_AS_FUNC

#define UNIMPLEMENTED   UnImplementedFuncAMI
extern int UnImplementedFuncAMI (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);

#else

#define UNIMPLEMENTED 	-1

#endif


#define GET_DCMI_CAPABILITY_INFO                GetDCMICapabilityInfo               /* UNIMPLEMENTED */
#define GET_POWER_READING                       GetPowerReading                     /* UNIMPLEMENTED */
#define GET_POWER_LIMIT                         GetPowerLimit                       /* UNIMPLEMENTED */
#define SET_POWER_LIMIT                         SetPowerLimit                       /* UNIMPLEMENTED */
#define ACTIVATE_POWER_LIMIT                    ActivatePowerLimit                  /* UNIMPLEMENTED */
#define GET_ASSET_TAG                           GetAssetTag                         /* UNIMPLEMENTED */
#define GET_DCMI_SENSOR_INFO                    GetDCMISensorInfo                   /* UNIMPLEMENTED */
#define SET_ASSET_TAG                           SetAssetTag                         /* UNIMPLEMENTED */
#define GET_MANAGEMENT_CONTROLLER_ID_STRING     GetManagementControllerIdString     /* UNIMPLEMENTED */
#define SET_MANAGEMENT_CONTROLLER_ID_STRING     SetManagementControllerIdString     /* UNIMPLEMENTED */

#define SET_THERMAL_LIMIT                       SetThermalLimit                          /* UNIMPLEMENTED */
#define GET_THERMAL_LIMIT                       GetThermalLimit                          /* UNIMPLEMENTED */
#define GET_TEMPERATURE_READING                   GetTemperatureReading                          /* UNIMPLEMENTED */
#define GET_DCMI_CONFIG_PARAMS                   GetDCMIConfigParameters                          /* UNIMPLEMENTED */
#define SET_DCMI_CONFIG_PARAMS                   SetDCMIConfigParameters                          /* UNIMPLEMENTED */

/*---------------------------------------------------------------------------*
 * ENABLE (OR) DISABLE INDIVIDUAL COMMANDS
 *---------------------------------------------------------------------------*/

/*----------------- IPMI Device "Global" Commands ---------------------------*/
#define GET_DEV_ID				GetDevID				/*UNIMPLEMENTED*/
#define BROADCAST_GET_DEV_ID  /*BroadCastGetDevID*/		  UNIMPLEMENTED
#define COLD_RESET				ColdReset				/*UNIMPLEMENTED*/
#define WARM_RESET			  	WarmReset				/*UNIMPLEMENTED*/
#define GET_SELF_TEST_RESULTS	GetSelfTestResults		/*UNIMPLEMENTED*/
#define MFG_TEST_ON				MfgTestOn				/*UNIMPLEMENTED*/
#define SET_ACPI_PWR_STATE		SetACPIPwrState			/*UNIMPLEMENTED*/
#define GET_ACPI_PWR_STATE		GetACPIPwrState			/*UNIMPLEMENTED*/
#define GET_DEV_GUID			GetDevGUID				/*UNIMPLEMENTED*/
#define GET_NETFN_SUP			GetNetFnSup				/*UNIMPLEMENTED*/
#define GET_CMD_SUP				GetCmdSup				/*UNIMPLEMENTED*/
#define GET_SUBFN_SUP			GetSubFnSup				/*UNIMPLEMENTED*/
#define GET_CONFIG_CMDS			GetConfigCmds			/*UNIMPLEMENTED*/
#define GET_CONFIG_SUB_FNS		GetConfigSubFns			/*UNIMPLEMENTED*/
#define SET_CMD_ENABLES			SetCmdEnables			/*UNIMPLEMENTED*/
#define GET_CMD_ENABLES			GetCmdEnables			/*UNIMPLEMENTED*/
#define SET_SUBFN_ENABLES		SetSubFnEnables			/*UNIMPLEMENTED*/
#define GET_SUBFN_ENABLES		GetSubFnEnables			/*UNIMPLEMENTED*/
#define GET_OEM_NETFN_IANA_SUPPORT	GetOEMNetFnIANASupport		/*UNIMPLEMENTED*/

/*------------------ BMC Watchdog Timer Commands ----------------------------*/
#define RESET_WDT				ResetWDT				/*UNIMPLEMENTED*/
#define SET_WDT					SetWDT					/*UNIMPLEMENTED*/
#define GET_WDT					GetWDT					/*UNIMPLEMENTED*/

/*------------------ BMC Device and Messaging Commands ----------------------*/
#define SET_BMC_GBL_ENABLES		SetBMCGlobalEnables		/*UNIMPLEMENTED*/
#define GET_BMC_GBL_ENABLES		GetBMCGlobalEnables		/*UNIMPLEMENTED*/
#define CLR_MSG_FLAGS			ClrMsgFlags				/*UNIMPLEMENTED*/
#define GET_MSG_FLAGS			GetMsgFlags				/*UNIMPLEMENTED*/
#define ENBL_MSG_CH_RCV			EnblMsgChannelRcv		/*UNIMPLEMENTED*/
#define GET_MSG					GetMessage				/*UNIMPLEMENTED*/
#define SEND_MSG				SendMessage				/*UNIMPLEMENTED*/
#define READ_EVT_MSG_BUFFER		ReadEvtMsgBuffer		/*UNIMPLEMENTED*/
#define GET_BTIFC_CAP		    GetBTIfcCap			    /*UNIMPLEMENTED*/
#define GET_SYSTEM_GUID			GetSystemGUID			/*UNIMPLEMENTED*/
#define GET_CH_AUTH_CAP			GetChAuthCap			/*UNIMPLEMENTED*/
#define GET_SESSION_CHALLENGE	GetSessionChallenge		/*UNIMPLEMENTED*/
#define ACTIVATE_SESSION		ActivateSession			/*UNIMPLEMENTED*/
#define SET_SESSION_PRIV_LEVEL	SetSessionPrivLevel		/*UNIMPLEMENTED*/
#define CLOSE_SESSION			CloseSession			/*UNIMPLEMENTED*/
#define GET_SESSION_INFO		GetSessionInfo			/*UNIMPLEMENTED*/
#define GET_AUTH_CODE			GetAuthCode				/*UNIMPLEMENTED*/
#define SET_CH_ACCESS			SetChAccess				/*UNIMPLEMENTED*/
#define GET_CH_ACCESS			GetChAccess				/*UNIMPLEMENTED*/
#define GET_CH_INFO				GetChInfo				/*UNIMPLEMENTED*/
#define SET_USER_ACCESS 		SetUserAccess			/*UNIMPLEMENTED*/
#define GET_USER_ACCESS 		GetUserAccess			/*UNIMPLEMENTED*/
#define SET_USER_NAME			SetUserName				/*UNIMPLEMENTED*/
#define GET_USER_NAME			GetUserName				/*UNIMPLEMENTED*/
#define SET_USER_PASSWORD		SetUserPassword			/*UNIMPLEMENTED*/
#define GET_SYSTEM_INFO_PARAM	GetSystemInfoParam /*UNIMPLEMENTED*/
#define SET_SYSTEM_INFO_PARAM	SetSystemInfoParam /*UNIMPLEMENTED*/

#define ACTIVATE_PAYLOAD		ActivatePayload      	/*UNIMPLEMENTED*/
#define DEACTIVATE_PAYLOAD		DeactivatePayload		/*UNIMPLEMENTED*/
#define GET_PAYLD_ACT_STATUS	GetPayldActStatus		/*UNIMPLEMENTED*/
#define GET_PAYLD_INST_INFO		GetPayldInstInfo		/*UNIMPLEMENTED*/
#define SET_USR_PAYLOAD_ACCESS	SetUsrPayloadAccess		/*UNIMPLEMENTED*/
#define GET_USR_PAYLOAD_ACCESS	GetUsrPayloadAccess		/*UNIMPLEMENTED*/
#define GET_CH_PAYLOAD_SUPPORT	GetChPayloadSupport		/*UNIMPLEMENTED*/
#define GET_CH_PAYLOAD_VER		GetChPayloadVersion		/*UNIMPLEMENTED*/
#define GET_CH_OEM_PAYLOAD_INFO	GetChOemPayloadInfo		/*UNIMPLEMENTED*/

#define MASTER_WRITE_READ		MasterWriteRead			/*UNIMPLEMENTED*/

#define GET_CH_CIPHER_SUITES	GetChCipherSuites		/*UNIMPLEMENTED*/
#define SUS_RES_PAYLOAD_ENCRYPT SusResPayldEncrypt		/*UNIMPLEMENTED*/
#define SET_CH_SECURITY_KEYS	SetChSecurityKeys		/*UNIMPLEMENTED*/
#define GET_SYS_IFC_CAPS		GetSysIfcCaps   		/*UNIMPLEMENTED*/

/*--------------------- Chassis Device Commands ----------------------------*/
#define GET_CHASSIS_CAPABILITIES	GetChassisCaps			/*UNIMPLEMENTED*/
#define GET_CHASSIS_STATUS			GetChassisStatus		/*UNIMPLEMENTED*/
#define CHASSIS_CONTROL				ChassisControl			/*UNIMPLEMENTED*/
#define CHASSIS_RESET_CMD			/*ChassisReset*/		  UNIMPLEMENTED
#define CHASSIS_IDENTIFY_CMD   		GetChassisIdentify		/*UNIMPLEMENTED*/
#define SET_CHASSIS_CAPABILITIES	SetChassisCaps			/*UNIMPLEMENTED*/
#define SET_POWER_RESTORE_POLICY	SetPowerRestorePolicy	/*UNIMPLEMENTED*/
#define GET_SYSTEM_RESTART_CAUSE	GetSysRestartCause		/*UNIMPLEMENTED*/
#define SET_SYSTEM_BOOT_OPTIONS		SetSysBOOTOptions		/*UNIMPLEMENTED*/
#define GET_SYSTEM_BOOT_OPTIONS		GetSysBOOTOptions		/*UNIMPLEMENTED*/
#define GET_POH_COUNTER				GetPOHCounter			/*UNIMPLEMENTED*/
#define SET_FP_BTN_ENABLES			SetFPButtonEnables		/*UNIMPLEMENTED*/
#define SET_POWER_CYCLE_INTERVAL	SetPowerCycleInterval	/*UNIMPLEMENTED*/

/*----------------------- Event Commands -----------------------------------*/
#define SET_EVENT_RECEIVER			SetEventReceiver		/*UNIMPLEMENTED*/
#define GET_EVENT_RECEIVER			GetEventReceiver		/*UNIMPLEMENTED*/
#define PLATFORM_EVENT				PlatformEventMessage	/*UNIMPLEMENTED*/

/*--------------------- PEF and Alerting Commands --------------------------*/
//#define GET_PEF_CAPABILITIES		GetPEFCapabilities		/*UNIMPLEMENTED*/
#define ARM_PEF_POSTPONE_TIMER		ArmPEFPostponeTimer		/*UNIMPLEMENTED*/
#define SET_PEF_CONFIG_PARAMS		SetPEFConfigParams		/*UNIMPLEMENTED*/
#define GET_PEF_CONFIG_PARAMS		GetPEFConfigParams		/*UNIMPLEMENTED*/
#define SET_LAST_PROCESSED_EVENT_ID	SetLastProcessedEventId	/*UNIMPLEMENTED*/
#define GET_LAST_PROCESSED_EVENT_ID	GetLastProcessedEventId	/*UNIMPLEMENTED*/
#define ALERT_IMMEDIATE				AlertImmediate			/*UNIMPLEMENTED*/
#define PET_ACKNOWLEDGE				PETAcknowledge			/*UNIMPLEMENTED*/

/*----------------------- Sensor Device Commands -------------------------*/
#define GET_DEV_SDR_INFO		   GetDevSDRInfo		  /*UNIMPLEMENTED*/
#define GET_DEV_SDR				   GetDevSDR		  	  /*UNIMPLEMENTED*/
#define RESERVE_DEV_SDR_REPOSITORY ReserveDevSDRRepository /*UNIMPLEMENTED*/
#define GET_SENSOR_READING_FACTORS  GetSensorReadingFactors	/*UNIMPLEMENTED*/
#define SET_SENSOR_HYSTERISIS		SetSensorHysterisis		/*UNIMPLEMENTED*/
#define GET_SENSOR_HYSTERISIS		GetSensorHysterisis		/*UNIMPLEMENTED*/
#define SET_SENSOR_THRESHOLDS		SetSensorThresholds		/*UNIMPLEMENTED*/
#define GET_SENSOR_THRESHOLDS		GetSensorThresholds		/*UNIMPLEMENTED*/
#define SET_SENSOR_EVENT_ENABLE		SetSensorEventEnable	/*UNIMPLEMENTED*/
#define GET_SENSOR_EVENT_ENABLE		GetSensorEventEnable	/*UNIMPLEMENTED*/
#define REARM_SENSOR_EVENTS		    ReArmSensor			    /*UNIMPLEMENTED*/
#define GET_SENSOR_EVENT_STATUS	    GetSensorEventStatus	/*  UNIMPLEMENTED*/
#define GET_SENSOR_READING		    GetSensorReading		/*UNIMPLEMENTED*/
#define SET_SENSOR_READING		    SetSensorReading		/*UNIMPLEMENTED*/
#define SET_SENSOR_TYPE			    SetSensorType			/*UNIMPLEMENTED*/
#define GET_SENSOR_TYPE			    GetSensorType			/*UNIMPLEMENTED*/

/*------------------------ FRU Device Commands ----------------------------*/
#define GET_FRU_INVENTORY_AREA_INFO GetFRUAreaInfo			/*UNIMPLEMENTED*/
#define READ_FRU_DATA				ReadFRUData				/*UNIMPLEMENTED*/
#define WRITE_FRU_DATA				WriteFRUData			/*UNIMPLEMENTED*/

/*----------------------- SDR Device Commands -----------------------------*/
#define GET_SDR_REPOSITORY_INFO		GetSDRRepositoryInfo	/*UNIMPLEMENTED*/
#define GET_SDR_REPOSITORY_ALLOCATION_INFO GetSDRRepositoryAllocInfo	/*UNIMPLEMENTED*/
#define RESERVE_SDR_REPOSITORY		ReserveSDRRepository	/*UNIMPLEMENTED*/
#define GET_SDR				GetSDR			/*UNIMPLEMENTED*/
#define ADD_SDR						AddSDR					/*UNIMPLEMENTED*/
#define PARTIAL_ADD_SDR				PartialAddSDR			/*UNIMPLEMENTED*/
#define DELETE_SDR			DeleteSDR                   	/* UNIMPLEMENTED*/
#define CLEAR_SDR_REPOSITORY		ClearSDRRepository		/*UNIMPLEMENTED*/
#define GET_SDR_REPOSITORY_TIME		 GetSDRRepositoryTime	 /*UNIMPLEMENTED*/
#define SET_SDR_REPOSITORY_TIME		 /*SetSDRRepositoryTime*/  UNIMPLEMENTED
#define ENTER_SDR_REPOSITORY_UPDATE_MODE /*EnterSDRUpdateMode*/	UNIMPLEMENTED
#define EXIT_SDR_REPOSITORY_UPDATE_MODE	 /*ExitSDRUpdateMode*/    UNIMPLEMENTED
#define RUN_INITIALIZATION_AGENT	 RunInitializationAgent /*UNIMPLEMENTED*/

/*------------------------- SEL Device Commands ---------------------------*/
#define GET_SEL_INFO				GetSELInfo				/*UNIMPLEMENTED*/
#define GET_SEL_ALLOCATION_INFO		GetSELAllocationInfo	/*UNIMPLEMENTED*/
#define RESERVE_SEL					ReserveSEL				/*UNIMPLEMENTED*/
#define GET_SEL_ENTRY				GetSELEntry				/*UNIMPLEMENTED*/
#define ADD_SEL_ENTRY				AddSELEntry				/*UNIMPLEMENTED*/
#define PARTIAL_ADD_SEL_ENTRY	    PartialAddSELEntry      /*UNIMPLEMENTED*/
#define DELETE_SEL_ENTRY			DeleteSELEntry		    /*UNIMPLEMENTED*/
#define CLEAR_SEL				    ClearSEL				/*UNIMPLEMENTED*/
#define GET_SEL_TIME				GetSELTime				/*UNIMPLEMENTED*/
#define SET_SEL_TIME				SetSELTime				/*UNIMPLEMENTED*/
#define GET_AUXILIARY_LOG_STATUS    /*GetAuxiliaryLogStatus*/UNIMPLEMENTED
#define SET_AUXILIARY_LOG_STATUS    /*SetAuxiliaryLogStatus*/UNIMPLEMENTED
#define GET_SEL_TIME_UTC_OFFSET		GetSELTimeUTC_Offset	/*UNIMPLEMENTED*/
#define SET_SEL_TIME_UTC_OFFSET		SetSELTimeUTC_Offset	/*UNIMPLEMENTED*/

/*------------------------- LAN Device Commands ---------------------------*/
#define SET_LAN_CONFIGURATION_PARAMETERS SetLanConfigParam	/*UNIMPLEMENTED*/
#define GET_LAN_CONFIGURATION_PARAMETERS GetLanConfigParam	/*UNIMPLEMENTED*/
#define SUSPEND_BMC_ARPS			     SuspendBMCArps      /*UNIMPLEMENTED*/
#define GET_IP_UDP_RMCP_STATISTICS	   /*GetIPUDPRMCPStats*/  UNIMPLEMENTED

/*----------------------- Serial/Modem Device Commands --------------------*/
#define SET_SERIAL_MODEM_CONFIGURATION SetSerialModemConfig  	  		/*UNIMPLEMENTED*/
#define GET_SERIAL_MODEM_CONFIGURATION GetSerialModemConfig				/*UNIMPLEMENTED*/
#define SET_SERIAL_MODEM_MUX		   SetSerialModemMUX			  	/*UNIMPLEMENTED*/
#define GET_TAP_RESPONSE			   GetTAPResponseCodes				/*UNIMPLEMENTED*/
#define SET_PPP_UDP_PROXY_TRANSMIT_DATA/*SetPPPUDPProxyTransmitData*/	  UNIMPLEMENTED
#define GET_PPP_UDP_PROXY_TRANSMIT_DATA/*GetPPPUDPProxyTransmitData*/	  UNIMPLEMENTED
#define SEND_PPP_UDP_PROXY_PACKET	   /*SendPPPUDPProxyPacket*/		  UNIMPLEMENTED
#define GET_PPP_UDP_PROXY_RECEIVE_DATA /*GetPPPUDPProxyReceiveData*/	  UNIMPLEMENTED
#define SERIAL_MODEM_CONNECTION_ACTIVITY /*SerialModemConnectActive*/ 	  UNIMPLEMENTED
#define CALLBACK					   CallBack						  	/*UNIMPLEMENTED*/
#define SET_USER_CALLBACK_OPTIONS	   SetUserCallBackOptions		    /*UNIMPLEMENTED*/
#define GET_USER_CALLBACK_OPTIONS	   GetUserCallBackOptions		  	/*UNIMPLEMENTED*/
#define SOL_ACTIVATING_COMMAND		   SOLActivating		  			/*UNIMPLEMENTED*/
#define GET_SOL_CONFIGURATION		   GetSOLConfig		  				/*UNIMPLEMENTED*/
#define SET_SOL_CONFIGURATION		   SetSOLConfig		  				/*UNIMPLEMENTED*/

/*--------------------- Bridge Management Commands (ICMB) ------------------*/
#define GET_BRIDGE_STATE              GetBridgeState            /*UNIMPLEMENTED*/
#define SET_BRIDGE_STATE              SetBridgeState            /*UNIMPLEMENTED*/
#define GET_ICMB_ADDR                 GetICMBAddr				/*UNIMPLEMENTED*/
#define SET_ICMB_ADDR                 SetICMBAddr				/*UNIMPLEMENTED*/
#define SET_BRIDGE_PROXY_ADDR	      SetBridgeProxyAddr        /*UNIMPLEMENTED*/
#define GET_BRIDGE_STATISTICS         GetBridgeStatistics       /*UNIMPLEMENTED*/
#define GET_ICMB_CAPABILITIES         GetICMBCaps               /*UNIMPLEMENTED*/
#define CLEAR_BRIDGE_STATISTICS       ClearBridgeStatistics     /*UNIMPLEMENTED*/
#define GET_BRIDGE_PROXY_ADDR	      GetBridgeProxyAddr        /*UNIMPLEMENTED*/
#define GET_ICMB_CONNECTOR_INFO       GetICMBConnectorInfo      /*UNIMPLEMENTED*/
#define GET_ICMB_CONNECTION_ID        /*GetICMBConnectionID*/   UNIMPLEMENTED
#define SEND_ICMB_CONNECTION_ID       /*SendICMBConnectionID*/  UNIMPLEMENTED

/*--------------------- Discovery Commands (ICMB) --------------------------*/
#define PREPARE_FOR_DISCOVERY		  PrepareForDiscovery		  /*UNIMPLEMENTED*/
#define GET_ADDRESSES				  GetAddresses				  /*UNIMPLEMENTED*/
#define SET_DISCOVERED				  SetDiscovered				  /*UNIMPLEMENTED*/
#define GET_CHASSIS_DEVICE_ID		  GetChassisDeviceID		  /*UNIMPLEMENTED*/
#define SET_CHASSIS_DEVICE_ID		  SetChassisDeviceID		  /*UNIMPLEMENTED*/

/*--------------------- Bridging Commands (ICMB) ---------------------------*/
#define BRIDGE_REQUEST				  BridgeReq					/*UNIMPLEMENTED*/
#define BRIDGE_MESSAGE				  BridgeMsg					/*UNIMPLEMENTED*/

/*-------------------- Event Commands (ICMB) -------------------------------*/
#define GET_EVENT_COUNT               GetEventCount             /*UNIMPLEMENTED*/
#define SET_EVENT_DESTINATION         SetEventDest				/*UNIMPLEMENTED*/
#define SET_EVENT_RECEPTION_STATE     SetEventReceptionState    /*UNIMPLEMENTED*/
#define SEND_ICMB_EVENT_MESSAGE       SendICMBEventMsg		    /*UNIMPLEMENTED*/
#define GET_EVENT_DESTINATION         GetEventDest              /*UNIMPLEMENTED*/
#define GET_EVENT_RECEPTION_STATE     GetEventReceptionState    /*UNIMPLEMENTED*/

/*---------------- OEM Commands for Bridge NetFn ---------------------------*/
//#define OEM										/* OemCmdHandler */						UNIMPLEMENTED
#define SET_FAN                      SetFan             /*UNIMPLEMENTED*/
#define UPDATE_FIRMWARE              UpdateFirmware     /*UNIMPLEMENTED*/
#define CPU_INFO                     GetCPUInfo
#define BMC_INFO                     GetBMCInfo

/*----------------- Other Bridge Commands ----------------------------------*/
#define ERROR_REPORT				  /*ErrorReport*/				UNIMPLEMENTED

/*-------------------- AMI Specific Commands -------------------------------*/
#define SET_SMTP_CONFIG_PARAMS     SetSMTPConfigParams /*UNIMPLEMENTED*/
#define GET_SMTP_CONFIG_PARAMS     GetSMTPConfigParams /*UNIMPLEMENTED*/

#define AMI_GET_NM_CHANNEL_NUM               AMIGetNMChNum /*UNIMPLEMENTED*/
#define AMI_GET_ETH_INDEX               AMIGetEthIndex /*UNIMPLEMENTED*/


/* AMI RESET PASSWORD AND USER EMAIL COMMANDS */
#define AMI_SET_EMAIL_USER			AMISetEmailForUser /*UNIMPLEMENTED*/
#define AMI_GET_EMAIL_USER			AMIGetEmailForUser /*UNIMPLEMENTED*/
#define AMI_RESET_PASS				AMIResetPassword /*UNIMPLEMENTED*/
#define AMI_SET_EMAILFORMAT_USER			AMISetEmailFormatUser /*UNIMPLEMENTED*/
#define AMI_GET_EMAILFORMAT_USER			AMIGetEmailFormatUser /*UNIMPLEMENTED*/

//Linux Root User Access Commands
#define AMI_GET_ROOT_USER_ACCESS		AMIGetRootUserAccess   	/*UNIMPLEMENTED*/
#define AMI_SET_ROOT_PASSWORD			AMISetRootPassword    	/*UNIMPLEMENTED*/


/* AMI Restore Default commands */
#define AMI_RESTORE_DEF			AMIRestoreDefaults /*UNIMPLEMENTED*/
#define AMI_SET_PRESERVE_CONF			AMISetPreserveConfStatus /*UNIMPLEMENTED*/
#define AMI_GET_PRESERVE_CONF			AMIGetPreserveConfStatus /*UNIMPLEMENTED*/
#define AMI_SET_ALL_PRESERVE_CONF               AMISetAllPreserveConfStatus /*UNIMPLEMENTED*/
#define AMI_GET_ALL_PRESERVE_CONF               AMIGetAllPreserveConfStatus /*UNIMPLEMENTED*/

/* AMI log configuration commands */
#define AMI_GET_LOG_CONF			AMIGetLogConf /*UNIMPLEMENTED*/
#define AMI_SET_LOG_CONF			AMISetLogConf /*UNIMPLEMENTED*/
/* AMI Get Bios code */
#define AMI_GET_BIOS_CODE           AMIGetBiosCode /*UNIMPLEMENTED*/
#define AMI_SEND_TO_BIOS                   AMISendToBios /*UNIMPLEMENTED*/
#define AMI_GET_BIOS_COMMAND               AMIGetBiosCommand /*UNIMPLEMENTED*/
#define AMI_SET_BIOS_RESPONSE           AMISetBiosResponse /*UNIMPLEMENTED*/
#define AMI_GET_BIOS_RESPONSE              AMIGetBiosResponse /*UNIMPLEMENTED*/
#define AMI_SET_BIOS_FLAG                  AMISetBiosFlag /*UNIMPLEMENTED*/
#define AMI_GET_BIOS_FLAG                  AMIGetBiosFlag /*UNIMPLEMENTED*/

// AMI Firmware Update Comand
#define AMI_FIRMWAREUPDATE                  AMIFirmwareCommand
#define AMI_GETRELEASENOTE                  AMIGetReleaseNote

/*Setting and Getting Time Zone commands*/
#define AMI_SET_TIMEZONE              AMISetTimeZone /*UNIMPLEMENTED*/
#define AMI_GET_TIMEZONE              AMIGetTimeZone /*UNIMPLEMENTED*/

#define AMI_GET_NTP_CFG             AMIGetNTPCfg
#define AMI_SET_NTP_CFG             AMISetNTPCfg

#ifndef CONFIG_SPX_FEATURE_IPMI_NO_YAFU_SUPPORT
#define AMI_YAFU_SWITCH_FLASH_DEVICE              AMIYAFUSwitchFlashDevice
#define AMI_YAFU_ACTIVATE_FLASH_DEVICE          AMIYAFUActivateFlashDevice
#define AMI_YAFU_RESTORE_FLASH_DEVICE               AMIYAFURestoreFlashDevice
#define AMI_YAFU_GET_FLASH_INFO        		AMIYAFUGetFlashInfo              /*UNIMPLEMENTED*/
#define AMI_YAFU_GET_FIRMWARE_INFO 		AMIYAFUGetFirmwareInfo       /*UNIMPLEMENTED*/
#define AMI_YAFU_GET_FMH_INFO            		AMIYAFUGetFMHInfo               /*UNIMPLEMENTED*/
#define AMI_YAFU_GET_STATUS                		AMIYAFUGetStatus                  /*UNIMPLEMENTED*/
#define AMI_YAFU_ACTIVATE_FLASH         		AMIYAFUActivateFlashMode   /*UNIMPLEMENTED*/
#define AMI_YAFU_ALLOCATE_MEMORY      		AMIYAFUAllocateMemory       /*UNIMPLEMENTED*/
#define AMI_YAFU_FREE_MEMORY              		AMIYAFUFreeMemory             /*UNIMPLEMENTED*/
#define AMI_YAFU_READ_FLASH                 		AMIYAFUReadFlash                 /*UNIMPLEMENTED*/
#define AMI_YAFU_WRITE_FLASH		      		AMIYAFUWriteFlash	             /*UNIMPLEMENTED*/
#define AMI_YAFU_ERASE_FLASH               		AMIYAFUEraseFlash              /*UNIMPLEMENTED*/
#define AMI_YAFU_PROTECT_FLASH           		AMIYAFUProtectFlash		/*UNIMPLEMENTED*/
#define AMI_YAFU_ERASE_COPY_FLASH     		AMIYAFUEraseCopyFlash      /*UNIMPLEMENTED*/
#define AMI_YAFU_GET_ECF_STATUS			AMIYAFUGetECFStatus
#define AMI_YAFU_GET_VERIFY_STATUS		AMIYAFUGetVerifyStatus
#define AMI_YAFU_VERIFY_FLASH              		AMIYAFUVerifyFlash              /*UNIMPLEMENTED*/
#define AMI_YAFU_READ_MEMORY              		AMIYAFUReadMemory           /*UNIMPLEMENTED*/
#define AMI_YAFU_WRITE_MEMORY            		AMIYAFUWriteMemory          /*UNIMPLEMENTED*/
#define AMI_YAFU_COPY_MEMORY              		AMIYAFUCopyMemory           /*UNIMPLEMENTED*/
#define AMI_YAFU_COMPARE_MEMORY       		AMIYAFUCompareMemory    /*UNIMPLEMENTED*/
#define AMI_YAFU_CLEAR_MEMORY            		AMIYAFUClearMemory          /*UNIMPLEMENTED*/
#define AMI_YAFU_GET_BOOT_CONFIG      		AMIYAFUGetBootConfig        /*UNIMPLEMENTED*/
#define AMI_YAFU_SET_BOOT_CONFIG      		AMIYAFUSetBootConfig        /*UNIMPLEMENTED*/
#define AMI_YAFU_GET_BOOT_VARS          		AMIYAFUGetBootVars          /*UNIMPLEMENTED*/
#define AMI_YAFU_DEACTIVATE_FLASH_MODE   AMIYAFUDeactivateFlash   /*UNIMPLEMENTED*/
#define AMI_YAFU_RESET_DEVICE   			AMIYAFUResetDevice         /*UNIMPLEMENTED*/
#define AMI_YAFU_DUAL_IMG_SUP                     AMIYAFUDualImgSup
#define AMI_YAFU_FIRMWARE_SELECT_FLASH  AMIYAFUFWSelectFlash
#define AMI_YAFU_SIGNIMAGEKEY_REPLACE  AMIYAFUReplaceSignedImageKey /* UNIMPLEMENTED */
#define AMI_YAFU_MISCELLANEOUS_INFO  AMIYAFUMiscellaneousInfo
#endif

/*--------------------------AMI TFTP Update Commands--------------------------*/
#define AMI_START_TFTP_FW_UPDATE                AMIStartTFTPFwUpdate
#define AMI_GET_TFTP_FW_PROGRESS_STATUS         AMIGetTftpProgressStatus
#define AMI_SET_FW_CONFIGURATION                AMISetFWCfg
#define AMI_GET_FW_CONFIGURATION                AMIGetFWCfg
#define AMI_SET_FW_PROTOCOL                     AMISetFWProtocol
#define AMI_GET_FW_PROTOCOL                     AMIGetFWProtocol

#define AMI_GET_IPMI_SESSION_TIMEOUT            AMIGetIPMISessionTimeOut

/*-------------------------AMI File Upload/Download Commands--------------------*/
#define AMI_FILE_UPLOAD				AMIFileUpload
#define AMI_FILE_DOWNLOAD			AMIFileDownload


/*------------------------Get UDS related Commands-------------------------------------------------*/
#define AMI_GET_UDS_CHANNEL_INFO     AMIGetUDSInfo                /* UNIMPLEMENTED */
#define AMI_GET_UDS_SESSION_INFO     AMIGetUDSSessionInfo         /* UNIMPLEMENTED */

/*-------------------------AMI Dual Image Support command------------------------*/
#define AMI_DUAL_IMG_SUPPORT AMIDualImageSupport

/*------------------------ AMI Control Debug Messages Commands--------------------*/
#define AMI_CTL_DBG_MSG     AMIControlDebugMsg
#define AMI_GET_DBG_MSG_STATUS      AMIGetDebugMsgStatus

#define AMI_SET_PWD_ENCRYPTION_KEY              AMISetPwdEncryptionKey
/*--------------------------AMI Service Commands ------------------------------------*/
#define AMI_GET_SERVICE_CONF                AMIGetServiceConf             /*UNIMPLEMENTED*/
#define AMI_SET_SERVICE_CONF                AMISetServiceConf             /*UNIMPLEMENTED*/
#define AMI_LINK_DOWN_RESILENT           AMILinkDownResilent         /*UNIMPLEMENTED*/

/*--------------------------AMI DNS Commands ------------------------------------*/
#define AMI_GET_DNS_CONF                AMIGetDNSConf             /*UNIMPLEMENTED*/
#define AMI_SET_DNS_CONF                AMISetDNSConf             /*UNIMPLEMENTED*/

/*--------------------------AMI Network Interface State Commands ------------------------------------*/
#define AMI_SET_IFACE_STATE         AMISetIfaceState             /*UNIMPLEMENTED*/
#define AMI_GET_IFACE_STATE         AMIGetIfaceState            /*UNIMPLEMENTED*/

/*------------------------ AMI FIREWALL - Iptables Commands -----------------------------------------*/
#define AMI_SET_FIREWALL			AMISetFirewall				/*UNIMPLEMENTED*/
#define AMI_GET_FIREWALL			AMIGetFirewall				/*UNIMPLEMENTED*/

/*------------------------ AMI FRU Details Commands -----------------------------------------*/
#define AMI_GET_FRU_DETAILS			AMIGetFruDetails			/*UNIMPLEMENTED*/

/*------------------------ AMI PAM Reordering Command---------------------------------------*/
#define AMI_SET_PAM_ORDER			AMISetPamOrder			/*UNIMPLEMENTED*/
#define AMI_GET_PAM_ORDER			AMIGetPamOrder			/*UNIMPLEMENTED*/

/*                                           AMI SNMP Commands----------------------------------------------*/
#define AMI_GET_SNMP_CONF         AMIGetSNMPConf                          /* UNIMPLEMENTED */
#define AMI_SET_SNMP_CONF         AMISetSNMPConf                          /* UNIMPLEMENTED */

/*----------------------------- AMI SEL Commands -----------------------------*/
#define AMI_GET_SEL_POLICY          AMIGetSELPolicy         /* UNIMPLEMENTED */
#define AMI_SET_SEL_POLICY          AMISetSELPolicy         /* UNIMPLEMENTED */
#define AMI_GET_SEL_ENTRIES           AMIGetSELEntires       /* UNIMPLEMENTED */
#define AMI_ADD_EXTEND_SEL_ENTIRES      AMIAddExtendSelEntries        /*UNIMPLEMENTED*/
#define AMI_PARTIAL_ADD_EXTEND_SEL_ENTIRES      AMIPartialAddExtendSelEntries    /*UNIMPLEMENTED*/
#define AMI_PARTIAL_GET_EXTEND_SEL_ENTIRES      AMIPartialGetExtendSelEntries    /*UNIMPLEMENTED*/
#define AMI_GET_EXTEND_SEL_DATA         AMIGETExtendSelData        /*UNIMPLEMENTED*/

/*------------------------------AMI SensorInfo Commands -----------------------*/
#define AMI_GET_SENSOR_INFO         AMIGetSenforInfo        /*UNIMPLEMENTED*/
/*--------------------------APML Specific Commands ------------------------------------*/
//SB-RMI
#define APML_GET_INTERFACE_VERSION	ApmlGetInterfaceVersion   /*UNIMPLEMENTED*/ //APML
#define APML_READ_RMI_REG			ApmlReadRMIReg	/*UNIMPLEMENTED*/
#define APML_WRITE_RMI_REG			ApmlWriteRMIReg	/*UNIMPLEMENTED*/
#define APML_READ_CPUID				ApmlReadCPUId	/*UNIMPLEMENTED*/
#define APML_READ_HTC_REG			ApmlReadHTCReg	/*UNIMPLEMENTED*/
#define APML_WRITE_HTC_REG			ApmlWriteHTCReg	/*UNIMPLEMENTED*/
#define APML_READ_PSTATE			ApmlReadPState	/*UNIMPLEMENTED*/
#define APML_READ_MAX_PSTATE		ApmlReadMaxPState	/*UNIMPLEMENTED*/
#define APML_READ_PSTATE_LIMIT		ApmlReadPStateLimit	/*UNIMPLEMENTED*/
#define APML_WRITE_PSTATE_LIMIT		ApmlWritePStateLimit	/*UNIMPLEMENTED*/
#define APML_READ_MCR				ApmlReadMCR	/*UNIMPLEMENTED*/
#define APML_WRITE_MCR				ApmlWriteMCR	/*UNIMPLEMENTED*/
// SB-TSI
#define APML_READ_TSI_REG			ApmlReadTSIReg	/*UNIMPLEMENTED*/
#define APML_WRITE_TSI_REG			ApmlWriteTSIReg	/*UNIMPLEMENTED*/
#define APML_READ_TDP_LIMIT_REG                 ApmlReadTDPLimitReg       /*UNIMPLEMENTED*/ 
#define APML_WRITE_TDP_LIMIT_REG                ApmlWriteTDPLimitReg      /*UNIMPLEMENTED*/ 
#define APML_READ_PROCESSOR_POWER_REG           ApmlReadProcessorPowerReg /*UNIMPLEMENTED*/ 
#define APML_READ_POWER_AVERAGING_REG           ApmlReadPowerAveragingReg /*UNIMPLEMENTED*/ 
#define APML_READ_DRAM_THROTTLE_REG             ApmlReadDramThrottleReg   /*UNIMPLEMENTED*/ 
#define APML_WRITE_DRAM_THROTTLE_REG            ApmlWriteDramThrottleReg  /*UNIMPLEMENTED*/

/*------------------------APML Commands ends here--------------------------------------*/

/*------------------------OPMA Specific Commands ---------------------------------------*/

#define SET_SENSOR_READING_OFFSET               SetSensorReadingOffset      /*UNIMPLEMENTED*/
#define GET_SENSOR_READING_OFFSET               GetSensorReadingOffset      /*UNIMPLEMENTED*/

#define SET_SYSTEM_TYPE_IDENTIFIER              SetSystemTypeIdentifier     /*UNIMPLEMENTED*/
#define GET_SYSTEM_TYPE_IDENTIFIER              GetSystemTypeIdentifier     /*UNIMPLEMENTED*/
#define GET_MCARD_CAPABLITITES                      GetmCardCapabilities            /*UNIMPLEMENTED*/
#define CLEAR_CMOS                                              ClearCMOS                               /*UNIMPLEMENTED*/
#define SET_LOCAL_ACCESS_LOCKOUT_STATE      SetLocalAccessLockOutState      /*UNIMPLEMENTED*/
#define GET_LOCAL_ACCESS_LOCKOUT_STATE      GetLocalAccessLockOutState      /*UNIMPLEMENTED*/
#define GET_SUPPORTED_HOST_IDS                      GetSupportedHostIDs                 /*UNIMPLEMENTED*/

/*------------------------OPMA Commands ends here--------------------------------------*/

/*--------------------------PNM Specific Commands ------------------------------------*/
#define PNM_OEM_GET_READING	        PnmOemGetReading   /*UNIMPLEMENTED*/
#define PNM_OEM_ME_POWER_STATE_CHANGE  PnmOemMePowerStateChange	/*UNIMPLEMENTED*/
#define PNM_OEM_PLATFORM_POWER_CHARACTERIZATION_NOTIFICATION PnmOemPlatformPowerCharacterizationNotification /*UNIMPLEMENTED*/

/*------------------------PNM Commands ends here--------------------------------------*/


/*------------------------User Shell related commands --------------------------------------*/

#define AMI_SET_USER_SHELLTYPE  AMISetUserShelltype	/*UNIMPLEMENTED*/
#define AMI_GET_USER_SHELLTYPE  AMIGetUserShelltype	/*UNIMPLEMENTED*/

/*------------------------User Shell related commands --------------------------------------*/

/*------------------------UserFlag  related commands --------------------------------------*/

#define AMI_SET_EXTENDED_PRIV  AMISetExtendedPrivilege     /*UNIMPLEMENTED*/
#define AMI_GET_EXTENDED_PRIV  AMIGetExtendedPrivilege    /*UNIMPLEMENTED*/

/*------------------------UserFlag related commands ends here-------------------------------------*/

/*------------------------Set Trigger Event Configuration-------------------------------------------------*/
#define AMI_SET_TRIGGER_EVT    AMISetTriggerEvent              /*UNIMPLEMENTED*/
/*------------------------Set Trigger Event Configuration-------------------------------------------------*/

/*------------------------Get Trigger Event Configuration-------------------------------------------------*/
#define AMI_GET_TRIGGER_EVT    AMIGetTriggerEvent              /*UNIMPLEMENTED*/
/*------------------------Get Trigger Event Configuration-------------------------------------------------*/

/*------------------------Get SOL Configuration-------------------------------------------------*/
#define AMI_GET_SOL_CONF    AMIGetSolConf              /*UNIMPLEMENTED*/
/*------------------------Get SOL Configuration-------------------------------------------------*/

/*------------------------Set Login Audit Configuration-------------------------------------------------*/
#define AMI_SET_LOGIN_AUDIT_CFG    AMISetLoginAuditConfig              /*UNIMPLEMENTED*/
/*------------------------Set Login Audit Configuration-------------------------------------------------*/

/*------------------------Get Login Audit Configuration-------------------------------------------------*/
#define AMI_GET_LOGIN_AUDIT_CFG    AMIGetLoginAuditConfig              /*UNIMPLEMENTED*/
/*------------------------Get Login Audit Configuration-------------------------------------------------*/

/*------------------------Get All IPv6 address-------------------------------------------------*/
#define AMI_GET_IPV6_ADDRESS          AMIGetAllIPv6Address               /*UNIMPLEMENTED*/
/*------------------------Get All IPv6 address-------------------------------------------------*/
/*------------------------------Set Virtual Device-----------------------------------------*/
#define AMI_VIRTUAL_DEVICE_GET_STATUS AMIVirtualDeviceGetStatus			/*UNIMPLEMENTED*/
#define AMI_VIRTUAL_DEVICE_SET_STATUS AMIVirtualDeviceSetStatus			/*UNIMPLEMENTED*/
/*------------------------------------------------------------------------------------------*/
/*------------------------U-boot Memory Test Command----------------------------------------*/
#define AMI_SET_UBOOT_MEMTEST       AMISetUBootMemtest               /*UNIMPLEMENTED*/
#define AMI_GET_UBOOT_MEMTEST_STATUS        AMIGetUBootMemtestStatus  /*UNIMPLEMENTED*/
/*------------------------U-boot Memory Test Command----------------------------------------*/

/*-----------------------------------------License Support Commands---------------------------*/
#define AMI_GET_LICENSE_VALIDITY AMIGetLicenseValidity
#define AMI_ADD_LICENSE_KEY AMIAddLicenseKey
/*-----------------------------------------License Support Commands----------------------------*/

#define AMI_GET_CHANNEL_TYPE  AMIGetChannelType

#define AMI_PECI_READ_WRITE           AMIPECIWriteRead /* UNIMPLEMENTED */

/*-----------------------------------------Radius Commands----------------------------*/
#define AMI_GET_RADIUS_CONF           AMIGetRadiusConf /* UNIMPLEMENTED */
#define AMI_SET_RADIUS_CONF           AMISetRadiusConf /* UNIMPLEMENTED */
/*-----------------------------------------Radius Commands----------------------------*/
#define AMI_GET_ALL_ACTIVE_SESSIONS		AMIGetAllActiveSessions

#define AMI_ACTIVE_SESSIONS_CLOSE		AMIActiveSessionClose

/*-----------------------------------------LDAP Commands----------------------------*/
#define AMI_GET_LDAP_CONF           AMIGetLDAPConf /* UNIMPLEMENTED */
#define AMI_SET_LDAP_CONF           AMISetLDAPConf /* UNIMPLEMENTED */
/*-----------------------------------------LDAP Commands----------------------------*/
/*-----------------------------------------Active Directory Commands---------------------------*/
#define AMI_GET_AD_CONF AMIGetADConf /* UNIMPLEMENTED */
#define AMI_SET_AD_CONF AMISetADConf /* UNIMPLEMENTED */
/*-----------------------------------------Active Directory Commands----------------------------*/
/*-----------------------------------------Video Record Commands---------------------------*/
#define AMI_GET_VIDEO_RCD_CONF AMIGetVideoRcdConf /* UNIMPLEMENTED */
#define AMI_SET_VIDEO_RCD_CONF AMISetVideoRcdConf /* UNIMPLEMENTED */
/*-----------------------------------------Video Record Commands----------------------------*/
/*_______________________________________________________________________________________________*/
/*------------------------------------- AMI CMM  Commands ---------------------------------------*/
         /*---------------------------- CMM Commands -------------------------------*/
#define AMI_GET_SLOT_MAP_INFO           AMIGetSlotMapInfo               /*UNIMPLEMENTED*/
#define AMI_GET_SLOT_INFO               AMIGetSlotInfo                  /*UNIMPLEMENTED*/
#define AMI_GET_PWR_INFO                AMIGetPwrInfo                   /*UNIMPLEMENTED*/
#define AMI_GET_PWR_DOM_INFO            AMIGetPwrDomInfo                /*UNIMPLEMENTED*/
#define AMI_GET_PWR_SUPPLY_INFO         AMIGetPwrSupplyInfo             /*UNIMPLEMENTED*/
#define AMI_GET_COOLING_INFO            AMIGetCoolingInfo               /*UNIMPLEMENTED*/
#define AMI_GET_COOLING_DOM_INFO        AMIGetCoolingDomInfo            /*UNIMPLEMENTED*/
#define AMI_GET_FAN_INFO                AMIGetFanInfo                   /*UNIMPLEMENTED*/
#define AMI_GET_BLADE_STATUS            AMIGetBladeStatus               /*UNIMPLEMENTED*/
#define AMI_ETH_RESTART_ALL             AMIEthRestartAll                /*UNIMPLEMENTED*/

/*--------------------------------------OBSM Specific Commands -----------------------------------*/
         /*---------------------------- OBSM Commands -------------------------------*/
#define OBSM_GET_OPEN_BLADE_PROPS       OBSMGetOpenBladeProps           /*UNIMPLEMENTED*/
#define OBSM_GET_ADDR_INFO              OBSMGetAddrInfo                 /*UNIMPLEMENTED*/
#define OBSM_PLATFORM_EVT_MSG           OBSMPlatformEvtMsg              /*UNIMPLEMENTED*/
#define OBSM_MGD_MOD_BMI_CTRL           /*OBSMManagedModuleBMICtrl*/    UNIMPLEMENTED
#define OBSM_MGD_MOD_PAYLD_CTRL         /*OBSMManagedModulePayldCtrl*/  UNIMPLEMENTED
#define OBSM_SET_SYS_EVNT_LOG_POLICY    /*OBSMSetSysEvntLogPolicy*/     UNIMPLEMENTED
#define OBSM_SET_MOD_ACTVN_POLICY       /*OBSMSetModuleActvnPolicy*/    UNIMPLEMENTED
#define OBSM_GET_MOD_ACTVN_POLICY       /*OBSMGetModuleActvnPolicy*/    UNIMPLEMENTED
#define OBSM_SET_MOD_ACTVN              /*OBSMSetModuleActivation*/     UNIMPLEMENTED
#define OBSM_SET_POWER_LEVEL            /*OBSMSetPowerLevel*/           UNIMPLEMENTED
#define OBSM_GET_POWER_LEVEL            /*OBSMGetPowerLevel*/           UNIMPLEMENTED
#define OBSM_RENOG_POWER                /*OBSMRenegotiatePower*/        UNIMPLEMENTED
#define OBSM_GET_SERVICE_INFO           /*OBSMGetServiceInfo*/          UNIMPLEMENTED
#define OBSM_GET_APPLET_PACKAGE_URI     /*OBSMGetAppletPackageURI*/     UNIMPLEMENTED
#define OBSM_GET_SERVICE_ENABLE_STATE   /*OBSMGetServiceEnableState*/   UNIMPLEMENTED
#define OBSM_SET_SERVICE_ENABLE_STATE   /*OBSMSetServiceEnableState*/   UNIMPLEMENTED
#define OBSM_SET_SERVICE_TICKET         /*OBSMSetServiceTiecket*/       UNIMPLEMENTED
#define OBSM_STOP_SERVICE_SESSION       /*OBSMStopServiceSession*/      UNIMPLEMENTED

             /*------------------------ Debug OBSM Commands -------------------------*/
#define DBG_GET_CHASSIS_PWR_INFO        DbgGetChassisPwrInfo            /*UNIMPLEMENTED*/
#define DBG_GET_CHASSIS_COOLING_INFO    DbgGetChassisCoolingInfo        /*UNIMPLEMENTED*/
#define DBG_GET_BLADE_INFO              DbgGetBladeInfo                 /*UNIMPLEMENTED*/
#define DBG_BLADE_INS_REM_EVT           DbgBladeInsRemEvent             /*UNIMPLEMENTED*/
#define DBG_PS_STATE_CHANGE_EVT         DbgPSStateChangeEvent           /*UNIMPLEMENTED */
#define DBG_FAN_STATE_CHANGE_EVT        DbgFanStateChangeEvent          /*UNIMPLEMENTED */
#define DBG_THERMAL_STATE_CHANGE_EVT    DbgThermalStateChangeEvent      /*UNIMPLEMENTED */

/*________________________________AMI CMM Commands ends here ____________________________________*/
/*-----------------------------------------------------------------------------------------------*/

/*-------------------------- SSI Compute Blade Specific Commands ---------------------------------------*/
#define SSICB_GET_COMPUTE_BLADE_PROPERTIES  SSICB_GetComputeBladeProperties /* UNIMPLEMENTED */
#define SSICB_GET_ADDR_INFO                 SSICB_GetAddrInfo               /* UNIMPLEMENTED */
#define SSICB_PLATFORM_EVENT_MESSAGE        SSICB_PlatformEventMessage      /* UNIMPLEMENTED */
#define SSICB_MODULE_BMI_CONTROL            SSICB_ModuleBMIControl          /* UNIMPLEMENTED */
#define SSICB_MODULE_PAYLOAD_CONTROL        SSICB_ModulePayloadControl      /* UNIMPLEMENTED */
#define SSICB_SET_SYSTEM_EVENT_LOG_POLICY   SSICB_SetSystemEventLogPolicy   /* UNIMPLEMENTED */
#define SSICB_SET_MODULE_ACTIVATION_POLICY  SSICB_SetModuleActivationPolicy /* UNIMPLEMENTED */
#define SSICB_GET_MODULE_ACTIVATION_POLICY  SSICB_GetModuleActivationPolicy /* UNIMPLEMENTED */
#define SSICB_SET_MODULE_ACTIVATION         SSICB_SetModuleActivation       /* UNIMPLEMENTED */
#define SSICB_SET_POWER_LEVEL               SSICB_SetPowerLevel             /* UNIMPLEMENTED */
#define SSICB_GET_POWER_LEVEL               SSICB_GetPowerLevel             /* UNIMPLEMENTED */
#define SSICB_RENEGOTIATE_POWER             SSICB_RenegotiatePower          /* UNIMPLEMENTED */
#define SSICB_GET_SERVICE_INFO              SSICB_GetServiceInfo            /* UNIMPLEMENTED */
#define SSICB_GET_APPLET_PACKAGE_URI        SSICB_GetAppletPackageURI       /* UNIMPLEMENTED */
#define SSICB_GET_SERVICE_ENABLE_STATE      SSICB_GetServiceEnableState     /* UNIMPLEMENTED */
#define SSICB_SET_SERVICE_ENABLE_STATE      SSICB_SetServiceEnableState     /* UNIMPLEMENTED */
#define SSICB_SET_SERVICE_TICKET            SSICB_SetServiceTicket          /* UNIMPLEMENTED */
#define SSICB_STOP_SERVICE_SESSION          SSICB_StopServiceSession        /* UNIMPLEMENTED */
/*-------------------------- SSI Compute Blade Commands ends here --------------------------------------*/

/*-------------------- PICMG HPM Specific Commands -------------------------------*/
#define GET_TARGET_UPLD_CAPABLITIES   GetTargetUpgradeCapablities  /*UNIMPLEMENTED*/
#define GET_COMPONENT_PROPERTIES      GetComponentProperties       /*UNIMPLEMENTED*/
#define INITIATE_UPGRADE_ACTION		  InitiateUpgradeAction		   /*UNIMPLEMENTED*/	
#define QUERY_SELF_TEST_RESULTS       QuerySelfTestResults         /*UNIMPLEMENTED*/
#define ABORT_FIRMWARE_UPGRADE        AbortFirmwareUpgrade         /*UNIMPLEMENTED*/
#define UPLOAD_FIRMWARE_BLOCK         UploadFirmwareBlock          /*UNIMPLEMENTED*/
#define FINISH_FIRMWARE_UPLOAD        FinishFirmwareUpload         /*UNIMPLEMENTED*/
#define GET_UPGRADE_STATUS            GetUpgradeStatus             /*UNIMPLEMENTED*/
#define ACTIVATE_FIRMWARE             ActivateFirmware             /*UNIMPLEMENTED*/
#define QUERY_ROLLBACK_STATUS         QueryRollbackStatus          /*UNIMPLEMENTED*/
#define INITIATE_MANUAL_ROLLBACK      InitiateManualRollback       /*UNIMPLEMENTED*/

/*--------------------------AMI Service Commands ------------------------------------*/
#define AMI_GET_RIS_CONF            AMIGetRISConf           /*UNIMPLEMENTED*/
#define AMI_SET_RIS_CONF            AMISetRISConf           /*UNIMPLEMENTED*/
#define AMI_RIS_START_STOP          AMIRISStartStop         /*UNIMPLEMENTED*/
/*--------------------------AMI Media Redirection Commands---------------------------*/
#define AMI_MEDIA_REDIRECTION_START_STOP AMIMediaRedirectionStartStop
#define AMI_GET_MEDIA_INFO		AMIGetMediaInfo
#define AMI_SET_MEDIA_INFO		AMISetMediaInfo

/*--------------------------AMI Host Monitor Lock Commands ------------------------------------*/
#define AMI_GET_HOST_LOCK_FEATURE_STATUS            	AMIGetHostLockFeatureStatus           /*UNIMPLEMENTED*/
#define AMI_SET_HOST_LOCK_FEATURE_STATUS            	AMISetHostLockFeatureStatus           /*UNIMPLEMENTED*/
#define AMI_GET_HOST_AUTO_LOCK_STATUS            	AMIGetHostAutoLockStatus           /*UNIMPLEMENTED*/
#define AMI_SET_HOST_AUTO_LOCK_STATUS            	AMISetHostAutoLockStatus           /*UNIMPLEMENTED*/

/*--------------------------AMI RemoteKVM Configuration Commands-------------------------------------------------------*/
#define AMI_GET_REMOTEKVM_CONF				AMIGetRemoteKVMCfg				/*UNIMPLEMENTED*/
#define AMI_SET_REMOTEKVM_CONF				AMISetRemoteKVMCfg				/*UNIMPLEMENTED*/
#define AMI_GET_SSL_CERT_STATUS				AMIGetSSLCertStatus				/*UNIMPLEMENTED*/

// Vmedia Configuration Commands
#define AMI_GET_VMEDIA_CONF				AMIGetVmediaCfg
#define AMI_SET_VMEDIA_CONF				AMISetVmediaCfg

#define AMI_GET_RUN_TIME_SINGLE_PORT_STATUS		AMIGetRunTimeSinglePortStatus
#define AMI_SET_RUN_TIME_SINGLE_PORT_STATUS		AMISetRunTimeSinglePortStatus

//Get Firmware Version
#define AMI_GET_FW_VERSION                      	AMIGetFwVersion

//AMI OEM Command To Get Core Feature Status
#define AMI_GET_FEATURE_STATUS            		AMIGetFeatureStatus

//AMI Get/Set Debug Logging Flag Command
#define AMI_GET_EXTLOG_CONF        AMIGetExtendedLogConf /*UNIMPLEMENTED*/
#define AMI_SET_EXTLOG_CONF        AMISetExtendedLogConf /*UNIMPLEMENTED*/

//AMI BMCConfig Backup-Restore commands
#define AMI_SET_BACKUP_FLAG        AMISetBackupFlag   /*UNIMPLEMENTED*/
#define AMI_GET_BACKUP_FLAG        AMIGetBackupFlag   /*UNIMPLEMENTED*/
#define AMI_MANAGE_BMC_CONFIG      AMIManageBMCConfig /*UNIMPLEMENTED*/

//AMI OEM Command to Restart Web Service
#define AMI_RESTART_WEB_SERVICE	AMIRestartWebService

//AMI OEM Command to Get PEND Status
#define AMI_GET_PEND_STATUS        AMIGetPendStatus /*UNIMPLEMENTED*/

//AMI OEM PLDM Command to Set/Get BIOS table.
#define AMI_PLDM_BIOS_MSG        AMIPLDMBIOSMsg  /*UNIMPLEMENTED*/

#define AMI_GET_LIFECYCLE_EVTLOG   AMIGetLifeCycleEvtLog   /*UNIMPLEMENTED*/

// SD configuration Commands
#define AMI_GET_SDCARD_PART        AMIGetSDCardPartition   /*UNIMPLEMENTED*/
#define AMI_SET_SDCARD_PART        AMISetSDCardPartition   /*UNIMPLEMENTED*/

/*--------------------------AMI SMBMC Related Commands-------------------------------------------------------*/
// AMI SMBMC Commands
#define SYSTEM_NODE		1
#define AMI_GET_BMC_INSTANCE_COUNT      AMIGetBMCInstanceCount /*UNIMPLEMENTED*/
#define AMI_GET_USB_SWITCH_SETTING      AMIGetUSBSwitchSetting /*UNIMPLEMENTED*/
#define AMI_SET_USB_SWITCH_SETTING      AMISetUSBSwitchSetting /*UNIMPLEMENTED*/

#define AMI_MUX_SWITCHING          AMISwitchMUX 

#define AMI_GET_RAID_INFO           AMIGetRAIDInfo      /*UNIMPLEMENTED*/

/*---------------------------------------------------------------------------*
 * ENABLE (OR) DISABLE INDIVIDUAL DEVICES BASED ON INTERFACE SELECTED
 *---------------------------------------------------------------------------*/
#if	SUPPORT_ICMB_IFC == 0
#undef	BRIDGE_DEVICE
#define BRIDGE_DEVICE				0
#endif


#if EVENT_PROCESSING_DEVICE == 1
#define EVENT_GENERATOR         1
#define EVENT_RECEIVER          1
#else
#define EVENT_GENERATOR         0
#define EVENT_RECEIVER          0
#endif


#if	DELETE_SDR 	!= UNIMPLEMENTED
#define MARK_FOR_DELETION_SUPPORT  0x00	/* 0x01 to mark records for deletion */
#endif



/***************************  Oem -Specific Commands **********************************/


#endif /* SUPPORT_H */
