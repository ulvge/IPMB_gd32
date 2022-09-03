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
 ****************************************************************
 ****************************************************************
 *
 * IPMI_AMI.h
 * AMI specific IPMI Commands
 *
 * Author: Basavaraj Astekar <basavaraja@ami.com>
 *
 *****************************************************************/
#ifndef IPMI_AMI_H_
#define IPMI_AMI_H_

/*Zone Name Length*/
#define TIME_ZONE_LEN   64
#define TIMEZONE_OFFSET_PVE  "Etc/GMT+"
#define TIMEZONE_OFFSET_NVE  "Etc/GMT-"

#define TIMEZONE_GMT_PVE  "GMT+"
#define TIMEZONE_GMT_NVE  "GMT-"


#define PACKED// __attribute__ ((packed))
#pragma pack( 1 )

/**
*@sturct AMIGetChNumRes_T
*@brief Structure to get respective channel num
*           for non BMC related Sensor Owner IDs
*/
typedef struct
{
    INT8U CompletionCode;
    INT8U ChannelNum;

}PACKED AMIGetChNumRes_T;

/**
*@struct AMIGetEthIndexReq_T
*@brief Structure to get respective Channel Num
*/
typedef struct
{

    INT8U ChannelNum;

}PACKED AMIGetEthIndexReq_T;

/**
*@struct AMIGetEthIndexRes_T
*@brief Structure to hold the respective EthIndex
*            for the requested Channel Num
*/

typedef struct
{

    INT8U CompletionCode;
    INT8U EthIndex;

}PACKED AMIGetEthIndexRes_T;


#pragma pack( )

/* AMI Net function group command numbers */

/* define your command numbers here */

#define CMD_AMI_YAFU_COMMON_NAK (0X00FF)
#define CMD_AMI_YAFU_GET_FLASH_INFO  (0x0001)
#define CMD_AMI_YAFU_GET_FIRMWARE_INFO (0x0002)
#define CMD_AMI_YAFU_GET_FMH_INFO (0x0003)
#define CMD_AMI_YAFU_GET_STATUS (0x0004)
#define CMD_AMI_YAFU_ACTIVATE_FLASH (0x0010)
#define CMD_AMI_YAFU_ALLOCATE_MEMORY (0x0020)
#define CMD_AMI_YAFU_FREE_MEMORY (0x0021)
#define CMD_AMI_YAFU_READ_FLASH (0x0022)
#define CMD_AMI_YAFU_WRITE_FLASH (0x0023)
#define CMD_AMI_YAFU_ERASE_FLASH (0x0024)
#define CMD_AMI_YAFU_PROTECT_FLASH (0x0025)
#define CMD_AMI_YAFU_ERASE_COPY_FLASH (0x0026)
#define CMD_AMI_YAFU_VERIFY_FLASH (0x0027)
#define CMD_AMI_YAFU_GET_ECF_STATUS (0x0028)
#define CMD_AMI_YAFU_GET_VERIFY_STATUS (0x0029)
#define CMD_AMI_YAFU_READ_MEMORY (0x0030)
#define CMD_AMI_YAFU_WRITE_MEMORY (0x0031)
#define CMD_AMI_YAFU_COPY_MEMORY (0x0032)
#define CMD_AMI_YAFU_COMPARE_MEMORY (0x0033)
#define CMD_AMI_YAFU_CLEAR_MEMORY (0x0034)
#define CMD_AMI_YAFU_GET_BOOT_CONFIG (0x0040)
#define CMD_AMI_YAFU_SET_BOOT_CONFIG (0x0041)
#define CMD_AMI_YAFU_GET_BOOT_VARS (0x0042)
#define CMD_AMI_YAFU_DEACTIVATE_FLASH_MODE (0x0050)
#define CMD_AMI_YAFU_RESET_DEVICE (0x0051)
#define CMD_AMI_YAFU_SWITCH_FLASH_DEVICE	(0x0052)
#define CMD_AMI_YAFU_RESTORE_FLASH_DEVICE	(0x0053)
#define CMD_AMI_YAFU_DUAL_IMAGE_SUP (0x0054)
#define CMD_AMI_YAFU_FIRMWARE_SELECT_FLASH  (0x0055)
#define CMD_AMI_YAFU_ACTIVATE_FLASH_DEVICE     (0x0056)
#define CMD_AMI_FILE_UPLOAD	(0x0057)
#define CMD_AMI_FILE_DOWNLOAD	(0x0058)
#define CMD_AMI_YAFU_MISCELLANEOUS_INFO     (0x0059)
#define CMD_AMI_GET_CHANNEL_NUM (0x60)
#define CMD_AMI_GET_ETH_INDEX (0x62)

#define CMD_AMI_GET_EMAIL_USER	(0x63)
#define CMD_AMI_SET_EMAIL_USER	(0x64)
#define CMD_AMI_RESET_PASS	(0x65)
#define CMD_AMI_RESTORE_DEF	(0x66)
#define CMD_AMI_GET_LOG_CONF	(0x67)
#define CMD_AMI_SET_LOG_CONF	(0x68)

/* AMI Specific Extend Commands */
#define CMD_AMI_GET_SERVICE_CONF    (0x69)
#define CMD_AMI_SET_SERVICE_CONF    (0x6a)
#define CMD_AMI_GET_DNS_CONF        (0x6b)
#define CMD_AMI_SET_DNS_CONF        (0x6c)

#define CMD_AMI_LINK_DOWN_RESILENT (0x70)

#define CMD_AMI_SET_IFACE_STATE (0x71)
#define CMD_AMI_GET_IFACE_STATE (0x72)

#define CMD_AMI_GET_BIOS_CODE       (0x73)

// DNS v6 commands */
#define CMD_AMI_GET_V6DNS_CONF        (0x74)
#define CMD_AMI_SET_V6DNS_CONF        (0x75)

#define CMD_AMI_SET_FIREWALL		(0x76)
#define CMD_AMI_GET_FIREWALL		(0x77)

#define CMD_SET_SMTP_CONFIG_PARAMS              ( 0x78 )
#define CMD_GET_SMTP_CONFIG_PARAMS              ( 0x79 )

//FRU details
#define CMD_AMI_GET_FRU_DETAILS		(0x80)
#define CMD_AMI_GET_EMAILFORMAT_USER (0x81)
#define CMD_AMI_SET_EMAILFORMAT_USER	(0x82)

//Linux Root User Access Commands
#define CMD_AMI_GET_ROOT_USER_ACCESS	(0x90)
#define CMD_AMI_SET_ROOT_PASSWORD	    (0x91)

//Set User Shelltype
#define CMD_AMI_GET_USER_SHELLTYPE	(0x92)
#define CMD_AMI_SET_USER_SHELLTYPE	(0x93)

/* Trigger Event Configuration Command Numbers */
#define CMD_AMI_SET_TRIGGER_EVT    (0x94)
#define CMD_AMI_GET_TRIGGER_EVT    (0x95)

/* SOL Configuration Command Numbers */
#define CMD_AMI_GET_SOL_CONFIG_PARAMS    (0x96)

/* Login Audit Config Command Numbers */
#define CMD_AMI_SET_LOGIN_AUDIT_CFG    (0x97)
#define CMD_AMI_GET_LOGIN_AUDIT_CFG    (0x98)
#define CMD_AMI_GET_IPV6_ADDRESS       (0x99)

/* IPMI PAM Reordering Command */
#define CMD_AMI_SET_PAM_ORDER        0x7a
#define CMD_AMI_GET_PAM_ORDER        0x7b

/* AMI-SNMP related Commands*/
#define CMD_AMI_GET_SNMP_CONF 0x7c
#define CMD_AMI_SET_SNMP_CONF 0x7d

/* AMI SEL Commands */
#define CMD_AMI_GET_SEL_POLICY  0x7e
#define CMD_AMI_SET_SEL_POLICY  0x7f

/* AMI-Preserve Conf related Commands*/
#define CMD_AMI_SET_PRESERVE_CONF       0x83
#define CMD_AMI_GET_PRESERVE_CONF       0x84

/* Retrive SEL Entries Command */
#define CMD_AMI_GET_SEL_ENTIRES 0x85

/* Retrive Sensor Info Command*/
#define CMD_AMI_GET_SENSOR_INFO 0x86

/* TFTP FW Update Commands */
#define CMD_AMI_START_TFTP_FW_UPDATE		0x87
#define CMD_AMI_GET_TFTP_FW_PROGRESS_STATUS	0x88
#define CMD_AMI_SET_FW_CONFIGURATION		0x89
#define CMD_AMI_GET_FW_CONFIGURATION		0x8A
#define CMD_AMI_SET_FW_PROTOCOL			0x8B
#define CMD_AMI_GET_FW_PROTOCOL			0x8C

#define CMD_AMI_GET_IPMI_SESSION_TIMEOUT	0x8D
#define CMD_AMI_GET_UDS_CHANNEL_INFO	    0x8E
#define CMD_AMI_DUAL_IMG_SUPPORT            0x8F
#define CMD_AMI_GET_UDS_SESSION_INFO	    0x9A
#define CMD_AMI_SET_PWD_ENCRYPTION_KEY		0x9B

/* Set the U-boot Memtest Variable and Get Memtest Status*/
#define CMD_AMI_SET_UBOOT_MEMTEST       0x9C
#define CMD_AMI_GET_UBOOT_MEMTEST_STATUS        0x9D

/* Remote Images service Commands */
#define CMD_AMI_GET_RIS_CONF    (0x9E)
#define CMD_AMI_SET_RIS_CONF    (0x9F)

/* Media Redirection Commands*/
#define CMD_AMI_MEDIA_REDIRECTION_START_STOP 0xD7
#define CMD_AMI_GET_MEDIA_INFO		    0xD8
#define CMD_AMI_SET_MEDIA_INFO		    0xD9

//SDcard Commands
#define CMD_AMI_GET_SDCARD_PART         0xDA
#define CMD_AMI_SET_SDCARD_PART         0xDB

#define CMD_AMI_RIS_START_STOP 			        0xA0
#define CMD_AMI_CTL_DBG_MSG     		        0xA1
#define CMD_AMI_GET_DBG_MSG_STATUS 		        0xA2
#define CMD_AMI_SET_EXTENDED_PRIV       	        0xA3
#define CMD_AMI_GET_EXTENDED_PRIV 	                0xA4

/*Macro's are used to set and get the TimeZone*/
#define CMD_AMI_SET_TIMEZONE         			0xA5
#define CMD_AMI_GET_TIMEZONE         			0xA6

#define CMD_AMI_GET_NTP_CFG          			0xA7
#define CMD_AMI_SET_NTP_CFG          			0xA8

#define CMD_AMI_YAFU_SIGNIMAGEKEY_REPLACE         	0xA9

/**Enable/DISable the Virual Dvices*/
#define CMD_AMI_VIRTUAL_DEVICE_SET_STATUS		0xAA
#define CMD_AMI_VIRTUAL_DEVICE_GET_STATUS		0xAB

#define CMD_AMI_ADD_LICENSE_KEY                         0XAC
#define CMD_AMI_GET_LICENSE_VALIDITY                    0XAD

/* Host Lock Monitor Status Get and Set*/
#define CMD_AMI_GET_HOST_LOCK_FEATURE_STATUS		0xAE
#define CMD_AMI_SET_HOST_LOCK_FEATURE_STATUS		0xAF
#define CMD_AMI_GET_HOST_AUTO_LOCK_STATUS		0xBC
#define CMD_AMI_SET_HOST_AUTO_LOCK_STATUS		0xBD

//Get all the active sessions
#define CMD_AMI_GET_ALL_ACTIVE_SESSIONS 		0xB0
#define CMD_AMI_ACTIVE_SESSIONS_CLOSE 			0xB1

// Run Time Single Port Feature Status Commands
#define CMD_AMI_GET_RUN_TIME_SINGLE_PORT_STATUS 	0xB7
#define CMD_AMI_SET_RUN_TIME_SINGLE_PORT_STATUS 	0xB8
#define CMD_AMI_GET_FW_VERSION                  	0xB4

//Video Record Commands
#define CMD_AMI_GET_VIDEO_RCD_CONF 			0xB5
#define CMD_AMI_SET_VIDEO_RCD_CONF 			0xB6

/* AMI-Preserve All Conf related Commands*/
#define CMD_AMI_SET_ALL_PRESERVE_CONF   		0xBA
#define CMD_AMI_GET_ALL_PRESERVE_CONF   		0xBB

#define CMD_AMI_GET_CHANNEL_TYPE      			0xBE
#define CMD_AMI_PECI_READ_WRITE 			0xBF

//Remote KVM Configuration Commands
#define CMD_AMI_GET_REMOTEKVM_CONF 			0xC0
#define CMD_AMI_SET_REMOTEKVM_CONF 			0xC1

//Get Feature Status
#define CMD_AMI_GET_FEATURE_STATUS			0xC2

// Get SSL Cert Status
#define CMD_AMI_GET_SSL_CERT_STATUS 			0xC3
#define CMD_AMI_GET_AD_CONF                     	0XC4
#define CMD_AMI_SET_AD_CONF                     	0XC5
#define CMD_AMI_GET_LDAP_CONF 				0xC8
#define CMD_AMI_SET_LDAP_CONF 				0xC9

#define CMD_AMI_GET_RADIUS_CONF 			0xC6
#define CMD_AMI_SET_RADIUS_CONF 			0xC7

//Vmedia Commands
#define CMD_AMI_GET_VMEDIA_CONF 			0xCA
#define CMD_AMI_SET_VMEDIA_CONF 			0xCB

#define CMD_AMI_ADD_EXTEND_SEL_ENTIRES 			0xCC
#define CMD_AMI_PARTIAL_ADD_EXTEND_SEL_ENTIRES  0xF0
#define CMD_AMI_PARTIAL_GET_EXTEND_SEL_ENTIRES  0xF1
#define CMD_AMI_GET_EXTEND_SEL_DATA         		0xCD
#define CMD_AMI_SEND_TO_BIOS                0xCE
#define CMD_AMI_GET_BIOS_COMMAND            0xCF
#define CMD_AMI_SET_BIOS_RESPONSE           0xD1
#define CMD_AMI_GET_BIOS_RESPONSE           0xD2
#define CMD_AMI_SET_BIOS_FLAG               0xD3
#define CMD_AMI_GET_BIOS_FLAG               0xD4
#define CMD_AMI_PLDM_BIOS_MSG               0xD5

// Extended Logging Config Commands
#define CMD_AMI_SET_EXTLOG_CONF        0xE1
#define CMD_AMI_GET_EXTLOG_CONF        0xE2

// Backup-Restore Configuration Commands
#define CMD_AMI_SET_BACKUP_FLAG        0xE3
#define CMD_AMI_GET_BACKUP_FLAG        0xE4
#define CMD_AMI_MANAGE_BMC_CONFIG      0xE5

//Restart web service
#define CMD_AMI_RESTART_WEB_SERVICE      0xE6

//Get Pend Status
#define CMD_AMI_GET_PEND_STATUS        0xE7

// Firmware Update Commands
#define CMD_AMI_FIRMWAREUPDATE          0xE8
#define CMD_AMI_GETRELEASENOTE          0xE9

#define CMD_AMI_GET_LIFECYCLE_EVTLOG    0xEA

// AMI SMBMC Commands
#define CMD_AMI_GET_BMC_INSTANCE_COUNT                    0xEB
#define CMD_AMI_GET_USB_SWITCH_SETTING                    0xEC
#define CMD_AMI_SET_USB_SWITCH_SETTING                    0xED

#define CMD_AMI_MUX_SWITCHING           0xEE
#define CMD_AMI_GET_RAID_INFO           0xEF

#endif /* IPMI_AMI_H */

