/**
 * @file   libipmi_struct.h
 * @author Rajasekhar (rajasekharj@amiindia.co.in)
 * @date   02-Sep-2004
 *
 * @brief  Contains data structures	required for establishing a session
 *
 */

#ifndef __LIBIPMI_STRUCT_H__
#define __LIBIPMI_STRUCT_H__

#include "platform.h"
#include "Types.h"
//#include "coreTypes.h"

#define PACK 

#pragma pack(1)


#define	MAX_REQUEST_SIZE		100
#define	MAX_RESPONSE_SIZE		100


#define HANDLE			int
#define SOCKET			int
#define SOCKET_ERROR	-1



#define IP4_VERSION     4
#define IP6_VERSION     6
#define IP4_ADDR_LEN    4
#define IP6_ADDR_LEN   16

/**
 @def LAN_CHANNEL_T
 @brief holds data required for network medium
*/
typedef struct LAN_CHANNEL_T_tag {
	char			szIPAddress[46];
	INT16U			wPort;
	SOCKET			hSocket;
	INT8U			byIsConnected;
	INT8U			bProtocol;
        INT8U                   bFamily;
} LAN_CHANNEL_T;

/**
 @def UDS_CHANNEL_T
 @brief holds data required for network medium
*/
typedef struct UDS_CHANNEL_T_tag {
	INT16U			hSocketPath;
	SOCKET			hSocket;
	INT8U			byIsConnected;
} UDS_CHANNEL_T;
/**
 @def SERIAL_CHANNEL_T
 @brief holds data required for Serial medium
*/
typedef struct SERIAL_CHANNEL_T_tag {
	char			szdevice[32];
} PACK SERIAL_CHANNEL_T;

/* Authentication Types */
#define	AUTHTYPE_NONE									0x00
#define	AUTHTYPE_MD2									0x01
#define	AUTHTYPE_MD5									0x02
#define	AUTHTYPE_RESERVED								0x03
#define	AUTHTYPE_STRAIGHT_PASSWORD						0x04
#define	AUTHTYPE_OEM_PROPRIETARY						0x05
#define	AUTHTYPE_RMCP_PLUS_FORMAT						0x06

/* Privilege levels */
#define PRIV_LEVEL_NO_ACCESS                            0x0F
#define PRIV_LEVEL_PROPRIETARY							0x05
#define PRIV_LEVEL_ADMIN								0x04
#define PRIV_LEVEL_OPERATOR								0x03
#define PRIV_LEVEL_USER									0x02
#define PRIV_LEVEL_CALLBACK								0x01
#define PRIV_LEVEL_RESERVED								0x00


/* Authentication Algorithms */
#define AUTH_ALG_RAKP_NONE								0x00
#define AUTH_ALG_RAKP_HMAC_SHA1							0x01
#define AUTH_ALG_RAKP_HMAC_MD5							0x02

/* Integrity Algorithms */
#define INTEGRITY_ALG_NONE								0x00
#define INTEGRITY_ALG_HMAC_SHA1_96						0x01
#define INTEGRITY_ALG_HMAC_SHA1_128						0x02
#define INTEGRITY_ALG_MD5_128							0x03

/* Confidentiality Algorithms */
#define CONFIDENTIALITY_ALG_NONE						0x00
#define CONFIDENTIALITY_ALG_AES_CBC_128					0x01
#define CONFIDENTIALITY_ALG_XRC4_128					0x02
#define CONFIDENTIALITY_ALG_XRC4_40						0x03

/* Payload Types */
#define PAYLOAD_TYPE_IPMI								0
#define PAYLOAD_TYPE_SOL								1
#define PAYLOAD_TYPE_RSSP_OPEN_SES_REQ					0x10
#define PAYLOAD_TYPE_RSSP_OPEN_SES_RES					0x11
#define PAYLOAD_TYPE_RAKP_MSG_1							0x12
#define PAYLOAD_TYPE_RAKP_MSG_2							0x13
#define PAYLOAD_TYPE_RAKP_MSG_3							0x14
#define PAYLOAD_TYPE_RAKP_MSG_4							0x15

#define	MAX_KEY1_SIZE									20
#define	MAX_KEY2_SIZE									20
#define	MAX_GUID_SIZE									16
#define	MAX_USER_NAME_LEN								(16+1) //1 for stroing the null character
#define	MAX_USER_PWD_LEN								(20+1) //1 for storing the null character
#define MAX_RAND_NO_LEN									16


/* (0x6 << 2) == 0x18 */
#define DEFAULT_NET_FN_LUN					0x18
#define NETFNLUN_IPMI_APP					0x18
#define NETFNLUN_IPMI_SENSOR					0x10
#define NETFNLUN_IPMI_STORAGE					0x28
#define NETFNLUN_IPMI_CHASSIS                                   0x00

/**
 @def IPMI20_NETWORK_SESSION_T
 @brief holds data required for maintaining session with network medium
*/
typedef struct IPMI20_NETWORK_SESSION_tag
{
	LAN_CHANNEL_T	hLANInfo;
	char			szUserName [MAX_USER_NAME_LEN];
	char			szPwd [MAX_USER_PWD_LEN];
	INT32U			dwSessionID;
	INT8U			byIPMBSeqNum;
	INT32U			dwSessionSeqNum;
	INT8U			byAuthType;
	INT8U			byChannelNum;
	INT8U			byPerMsgAuthentication;
	INT8U			byRole;
	INT8U			byAuthAlgthm;
	INT8U			byIntegrityAlgthm;
	INT8U			byEncryptAlgthm;
	INT8U			byPreSession;
	INT8U			byResAddr;
	INT8U			byReqAddr;
	INT8U			byGUID [MAX_GUID_SIZE];
	INT8U			byIsEncryptionReq;
	INT8U			byKey1 [MAX_KEY1_SIZE];
	INT8U			byKey2 [MAX_KEY2_SIZE];
	INT8U			byMsgTag;
	INT32U			dwRemConSessID;
	INT8U			byRemConRandomNo [16];
	INT8U			byMgdSysRandomNo [16];
	INT8U			byDefTimeout;
} IPMI20_NETWORK_SESSION_T;


/**
 @def IPMI20_SERIAL_SESSION_T
 @brief holds data required for maintaining session with serial medium
*/
typedef struct IPMI20_SERIAL_SESSION_T_tag {
	SERIAL_CHANNEL_T	hSerialInfo;
	char			szUserName [MAX_USER_NAME_LEN];
	char			szPwd [MAX_USER_PWD_LEN];
	HANDLE			hSerialPort;
	INT8U			byMaxRetries;
	INT32U			dwSessionID;
	INT8U			byAuthType;
	INT32U			dwInboundSeqNum;
	INT32U			dwOutboundSeqNum;
	INT8U			byIPMBSeqNum;
	INT8U			byPrivLevel;
	INT8U			byResAddr;
	INT8U			byReqAddr;
	INT8U			byDefTimeout;
} IPMI20_SERIAL_SESSION_T;

/**
 @def IPMI20_UDS_SESSION_T
 @brief holds data required for maintaining session with unix domain socket medium
*/
typedef struct IPMI20_UDS_SESSION_T_tag {
  UDS_CHANNEL_T hUDSInfo;
  char szUserName[MAX_USER_NAME_LEN];
  char szPwd[MAX_USER_PWD_LEN];
  char szUName[MAX_USER_NAME_LEN];
  char abyIPAddr[IP6_ADDR_LEN];
  INT32U dwSessionID;
  INT8U byPreSession;
  INT8U byAuthType;
  INT8U byRole;
  INT8U byChannelNum;
  INT8U byDefTimeout;
  INT8U byMaxRetries;
  INT8U byIPMBSeqNum;
} IPMI20_UDS_SESSION_T;

/**
 @def IPMI20_KCS_SESSION_T
 @brief holds data required for maintaining session with KCS medium
*/
typedef struct IPMI20_KCS_SESSION_T_tag {
	HANDLE			hKCSDevice;
	INT8U			byResAddr;
} IPMI20_KCS_SESSION_T;

/**
 @def IPMI20_IPMB_SESSION_T
 @brief holds data required for maintaining session with IPMB medium
*/
typedef struct IPMI20_IPMB_SESSION_T_tag {
	HANDLE			hIPMBDevice;
	INT8U			busnumber;
	INT8U			bySlaveAddr;
} IPMI20_IPMB_SESSION_T;

/**
 @def LIBIPMI_SERIAL_SETTINGS_T
 @brief holds settings for a session
*/
typedef struct {
/* settings state */
	INT8U		bySettings;
#define	SETTINGS_NOTSET			0x00
#define	SETTINGS_DEFAULT		0x01
#define	SETTINGS_USER			0x02

/* BaudRate */
	INT32U		dwBaudRate;

/* Parity */
	INT8U		byParity;
#define			EVEN_PARITY					0x00
#define			ODD_PARITY					0x01
#define			NO_PARITY					0x02
#define			MARK_PARITY					0x03
#define			SPACE_PARITY				0x04

/* Stop Bits */
	INT8U		byStopBit;
#define			STOPBIT_ONE					0x00
#define			STOPBIT_ONE5				0x01
#define			STOPBIT_TWO					0x02

	INT8U		byByteSize;

/* Byte Size */
} LIBIPMI_SERIAL_SETTINGS_T;

#define DEFAULT_BAUD_RATE					115200
#define DEFAULT_PARITY						NO_PARITY
#define	DEFAULT_STOPBITS					STOPBIT_ONE
#define DEFAULT_BYTESIZE					8

/**
 @def IPMI20_USB_SESSION_T
 @brief holds data required for maintaining session with USB medium
*/
#pragma anon_unions
typedef struct IPMI20_USB_SESSION_T_tag {
	union {
		HANDLE			hUSBDevice;
		int				hUSBDesc;
	};
	INT8U			byResAddr;
} IPMI20_USB_SESSION_T;


/**
 @def IPMI20_SESSION_T
 @brief holds info for maintaining a session
*/
typedef struct IPMI20_SESSION_T_tag {
/* Medium type (Network, Serial, KCS, IPMB, USB) */
	INT8U	byMediumType;

#define NETWORK_MEDIUM_TCP      0x01
#define NETWORK_MEDIUM_UDP      0x04

#define	NETWORK_MEDIUM	        NETWORK_MEDIUM_TCP
#define	SERIAL_MEDIUM	        0x02
#define KCS_MEDIUM		        0x03
#define IPMB_MEDIUM		        0x05
#define USB_MEDIUM		        0x06
#define	UDS_MEDIUM	          0x07

/* tells whether session has started or not. */
	INT8U	bySessionStarted;
#define SESSION_NOT_STARTED 0x00
#define SESSION_STARTED	0x01

/* if this value is > 0, session reestablishment will be tried for byMaxRetries times. */
	INT8U	byMaxRetries;
/* LAN Eth Index for hold Eth number if multi NIc supported */
	INT8U         EthIndex;

	IPMI20_NETWORK_SESSION_T	*hNetworkSession;
	IPMI20_SERIAL_SESSION_T		*hSerialSession;
	IPMI20_KCS_SESSION_T		*hKCSSession;
	IPMI20_IPMB_SESSION_T		*hIPMBSession;
	IPMI20_USB_SESSION_T		*hUSBSession;
	IPMI20_UDS_SESSION_T		*hUDSSession;
	IPMI20_UDS_SESSION_T		*hSMBMCUDSSession;
	INT8U SMBMCActiveInstance;

	LIBIPMI_SERIAL_SETTINGS_T	Settings;

} IPMI20_SESSION_T;
typedef enum
{
    AUTH_FLAG = 1,
    AUTH_BYPASS_FLAG,
}USER_Auth;

#pragma pack()

#endif /* __LIBIPMI_STRUCT_H__ */
