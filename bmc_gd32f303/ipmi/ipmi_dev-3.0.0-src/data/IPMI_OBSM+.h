/****************************************************************
 **                                                            **
 **    (C)Copyright 2007-2008, American Megatrends Inc.        **
 **                                                            **
 **                   All Rights Reserved.                     **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200, Norcross,        **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************/

 /****************************************************************
 * @file	IPMI_OBSM+.h
 * @author	Velu	<velmuruganv@amiindia.co.in>
 * @brief
 ****************************************************************/

#ifndef IPMI_OBSMCMM_H
#define IPMI_OBSMCMM_H

#include "Types.h"
#include "OBSMPort.h"

/*** Definitions and Macros ***/
#define OPEN_BLADE_ID                   0x02
#define DEFAULT_MODULE_FRU_DEV_ID       0x00
#define CHASSIS_INVENTORY_FRU_ID        1

#define MAX_PWR_SLOT_OFFSET             8
#define MAX_NUM_PWR_LEVEL               21
#define MAX_NUM_SLOTS_PER_MODULE        8

#define MAX_SERVICE_NAME_LEN            32
#define MAX_STRING_LEN                  32
#define MAX_TICKET_LEN                  128
#define MAX_URI_LEN                     128

#define PWR_DRAW_CURRENT_LEVEL          0x00
#define PWR_DRAW_DESIRED_LEVEL          0x01

#define PWR_DRAW_DISCRETE_LEVEL         0x00
#define PWR_DRAW_RANGE_BASED            0x01

#define SUPPORT_DYNAMIC_PWR_CONFIG      0x80
#define SUPPORT_PWR_DRAW_FORMAT         0x40

#define EVT_MSG_GEN_TYPE_MASK           0x01
#define EVT_MSG_LUN_MASK                0x03

#define EVT_MSG_GEN_TYPE_MNG_CONTRLR    0x00
#define EVT_MSG_GEN_TYPE_SYS_SOFTWARE   0x01

#define GET_SLOT_MODULECLASS(VAL)       ((VAL & 0xF0) >> 4)
#define GET_SLOT_MODULETYPE(VAL)        (VAL & 0x0F)
#define GET_SLOT_INSTANCE(VAL)          ((VAL & 0xC0) >> 6)
#define GET_SLOT_ID(VAL)                (VAL & 0x3F)

#define MOD_ACTIVATION                  0x01
#define MOD_DEACTIVATION                0x00

#define PWR_PROPS_REQ_PWR_LEVEL(x)	((x) & 0x1F)
#define SET_PWR_TYPE_SLOT_BYTE(T,S)	(((T & 0x03)<<3) | (S & 0x07))

#define ADDR_KEY_TYPE_PHY_SLOT_NUM          0x00
#define ADDR_KEY_TYPE_BMI_ADDR              0x01

#define OPEN_BLADE_SLOT_MAP_REC_ID          0x10
#define OPEN_BLADE_INTER_CONN_REC_ID        0x11
#define OPEN_BLADE_PWR_DOM_REC_ID           0x12
#define OPEN_BLADE_PWR_UNIT_RED_REC_ID      0x13
#define OPEN_BLADE_OUTPUT_CURR_REC_ID       0x14
#define OPEN_BLADE_CHASSIS_COOL_REC_ID      0x15

#define IFDOWN_ALL                          "/sbin/ifdown -a"
#define IFUP_ALL                            "/sbin/ifup -a"


/*** Typedef ***/

/**
 * @enum OBSMModuleClass_E
 * @brief Module Class Definitions
**/
typedef enum{
	MODCLASS_BLADE,
	MODCLASS_SWITCH,
	MODCLASS_CHASSISMNGR,
	MODCLASS_POWER,
	MODCLASS_COOLING,
	MODCLASS_DISK,
	MODCLASS_OEM,
	MODCLASS_RESERVED	/* 7h - Fh : Reserved */
} OBSMModuleClass_E;

/**
 * @enum OBSMBladeClassModuleType_E
 * @brief Blade Class Module Type Definitions
**/
typedef enum
{
	BLADECLASS_TYPE_COMPUTEBLADE,
	BLADECLASS_TYPE_STORAGE,
	BLADECLASS_TYPEOEM,
	BLADECLASS_TYPE_RESERVED, /* 3h - Fh : Reserved */
} OBSMBladeClassModuleType_E;

/**
 * @enum OBSMSwitchClassModuleType_E
 * @brief Switch Class Module Type Definitions
**/
typedef enum
{
	SWITCHCLASS_TYPE_PRIMARY,
	SWITCHCLASS_TYPE_SECONDARY,
	SWITCHCLASS_TYPE_PATCHBOARD,
	SWITCHCLASS_TYPE_OEM,
	SWITCHCLASS_TYPE_RESERVED, /* 4h - Fh : Reserved */
} OBSMSwitchClassModuleType_E;

/**
 * @enum OBSMChassisMngrClassModuleTye_E
 * @brief Chassis Manager Class Module Type Definitions
**/
typedef enum
{
	CHASSISMNGRCLASS_TYPE_CMM,
	CHASSISMNGRCLASS_TYPE_RESERVED, /* 1h - Fh : Reserved */
} OBSMChassisMngrClassModuleType_E;

/**
 * @enum OBSMPwrClassModuleTye_E
 * @brief Power Class Module Type Definitions
**/
typedef enum
{
	PWRCLASS_TYPE_PWRUNITCTLR,
	PWRCLASS_TYPE_PWRSUPPLY,
} OBSMPwrClassModuleType_E;

/**
 * @enum OBSMCoolingClassModuleType_E
 * @brief Cooling Class Module Type Definitions
**/
typedef enum
{
	COOLINGCLASS_TYPE_COOLINGCTLR,
	COOLINGCLASS_TYPE_COOLINGMODULE,
	COOLINGCLASS_TYPE_RESERVED   /* 2h - Fh : Reserved */
} OBSMCoolingClassModuleType_E;

/**
 * @enum OBSMModuleSiteType_E
 * @brief Module Site Type definitions
**/
typedef enum
{
    SITETYPE_RESERVED0,
	SITETYPE_OPEN_BLADE_COMP_BLADE = 0x01,
	SITETYPE_ETHERNET_SWITCH,
	SITETYPE_FIBRE_CHANNEL_SWITCH,
	SITETYPE_PCI_EXP_SWITCH,
	SITETYPE_INFINIBAND_SWITCH,
	SITETYPE_DED_CHASSIS_MANAGEMENT_MODULE,
	SITETYPE_CHASSIS_CONFIG_INFORMATION,
	SITETYPE_PWR_UNIT_MODULE,
	SITETYPE_PWR_SUPPLY_MODULE,
	SITETYPE_FAN_COOLING_MODULE,
	SITETYPE_ALARM_BOARD,
	SITETYPE_MEZZ_BOARD,
	SITETYPE_MEMORY_MODULE,
	SITETYPE_STORAGE_CTRL_MODULE,
	SITETYPE_GENERIC_MODULE,
} OBSMModuleSiteType_E;

#pragma pack(1)

/**
 * @struct GetOpenBladePropsReq_T
 * @brief Get Open Blade Properties command request structure
**/
typedef struct
{
    INT8U  openBladeId;
} PACKED GetOpenBladePropsReq_T;

/**
 * @struct GetOpenBladePropsRes_T
 * @brief Get Open Blade Properties command response structure
**/
typedef struct
{
    INT8U  completionCode;
    INT8U  openBladeId;
    INT8U  openBladeExtVer;
    INT8U  slotCount;
    INT8U  maxFRUDevId;
} PACKED GetOpenBladePropsRes_T;

/**
 * @struct GetAddrInfoReq_T
 * @brief Get Address Info request structure
**/
typedef struct
{
    INT8U   openBladeId;
    INT8U   fruDevId;
    INT8U   addrKeyType;
    INT8U   addrKey;
} PACKED OBSMGetAddrInfoReq_T;

/**
 * @struct GetAddrInfoRes_T
 * @brief Get Address Info response structure
**/
typedef struct
{
    INT8U   completionCode;
    INT8U   openBladeId;
    INT8U	fruDevId;
    INT8U   slotsOccupied;
    INT8U   bmiAddr;
    INT8U   siteType;
} PACKED OBSMGetAddrInfoRes_T;

/**
 * @struct OBSMPlatEvntMsgReq_T
 * @brief OBSM Platform Event Message request structure
**/
typedef struct
{
	INT8U openBladeId;
	INT8U genId[2];
	INT8U evMRev;
	INT8U sensorType;
	INT8U sensorNum;
	INT8U evtDirType;
	INT8U evtData1;
	INT8U evtData2;
	INT8U evtData3;
} PACKED OBSMPlatEvntMsgReq_T;

/**
 * @struct OBSMPlatEvntMsgRes_T
 * @brief OBSM Platform Event Message response structure
**/
typedef struct
{
	INT8U completionCode;
	INT8U openBladeId;
} PACKED OBSMPlatEvntMsgRes_T;

/**
 * @struct MgdModBMICtrlReq_T
 * @brief Managed Module BMI Control request structure
**/
typedef struct
{
	INT8U	openBladeId;
	INT8U	ipmbCtrl;
} PACKED MgdModBMICtrlReq_T;

/**
 * @struct MgdModBMICtrlRes_T
 * @brief Managed Module BMI Control response structure
**/
typedef struct
{
	INT8U	completionCode;
	INT8U	openBladeId;
	INT8U	ipmbState;
} PACKED MgdModBMICtrlRes_T;

/**
 * @struct MgdModPayldCtrlReq_T
 * @brief Managed Module Payload Control request structure
**/
typedef struct
{
    INT8U   openBladeId;
    INT8U   fruDevId;
    INT8U   payloadId;
    INT8U   payldCtrlOpt;
} PACKED MgdModPayldCtrlReq_T;

/**
 * @struct MgdModPayldCtrlRes_T
 * @brief Managed Module Payload Control response structure
**/
typedef struct
{
    INT8U   completionCode;
    INT8U   openBladeId;
} PACKED MgdModPayldCtrlRes_T;

/**
 * @struct SetSysEvntLogPolicyReq_T
 * @brief Set System Event Log Policy request structure
**/
typedef struct
{
	INT8U	openBladeId;
	INT8U	policyCtrl;
} PACKED SetSysEvntLogPolicyReq_T;

/**
 * @struct SetSysEvntLogPolicyRes_T
 * @brief Set System Event Log Policy response structure
**/
typedef struct
{
	INT8U	completionCode;
	INT8U	openBladeId;
	INT8U	currentPolicy;
} PACKED SetSysEvntLogPolicyRes_T;

/**
 * @struct SetModActvnPolicyReq_T
 * @brief Set Module Activation Policy request structure
**/
typedef struct
{
    INT8U   openBladeId;
    INT8U   fruDevId;
    INT8U   activationPolicy;
} PACKED SetModActvnPolicyReq_T;

/**
 * @struct SetModActvnPolicyRes_T
 * @brief Set Module Activation Policy response structure
**/
typedef struct
{
    INT8U   completionCode;
    INT8U   openBladeId;
} PACKED SetModActvnPolicyRes_T;

/**
 * @struct GetModActvnPolicyReq_T
 * @brief Get Module Activation Policy request structure
**/
typedef struct
{
    INT8U   openBladeId;
    INT8U   fruDevId;
} PACKED GetModActvnPolicyReq_T;

/**
 * @struct GetModActvnPolicyRes_T
 * @brief Get Module Activation Policy response structure
**/
typedef struct
{
    INT8U   completionCode;
    INT8U   openBladeId;
    INT8U   activationPolicy;
} PACKED GetModActvnPolicyRes_T;

/**
 * @struct SetModActvnReq_T
 * @brief Set Module Activation request structure
**/
typedef struct
{
    INT8U   openBladeId;
    INT8U   fruDevId;
    INT8U   modActvnDeactvn;
} PACKED SetModActvnReq_T;

/**
 * @struct SetModActvnRes_T
 * @brief Set Module Activation response structures
**/
typedef struct
{
    INT8U   completionCode;
    INT8U   openBladeId;
} PACKED SetModActvnRes_T;

/**
 * @struct SetPwrLevelReq_T
 * @brief Set Power Level request structure
**/
typedef struct
{
    INT8U   openBladeId;
    INT8U   fruDevId;
    INT8U   slotPwrLevel[MAX_PWR_SLOT_OFFSET];
} PACKED SetPwrLevelReq_T;

/**
 * @struct SetPwrLevelRes_T
 * @brief Set Power Level request structure
**/
typedef struct
{
    INT8U   completionCode;
    INT8U   openBladeId;
} PACKED SetPwrLevelRes_T;

/**
 * @struct GetPwrLevelReq_T
 * @brief Get Power Level request structures
**/
typedef struct
{
    INT8U   openBladeId;
    INT8U   fruDevId;
    INT8U   pwrTypeSlot;
} PACKED GetPwrLevelReq_T;

/**
 * @struct GetPwrLevelRes_T
 * @brief Get Power Level response structures
**/
typedef struct
{
    INT8U   completionCode;
    INT8U   openBladeId;
    INT8U   pwrProperties;
    INT8U   pwrMultiplier;
    INT8U   pwrLevel [MAX_NUM_PWR_LEVEL];
} PACKED GetPwrLevelRes_T;

/**
 * @struct RenegotiatePwrReq_T
 * @brief Renegotiate Power request structure
**/
typedef struct
{
    INT8U   openBladeId;
    INT8U   fruDevId;
} PACKED RenegotiatePwrReq_T;

/**
 * @struct RenegotiatePwrRes_T
 * @brief Renegotiate Power response structure
**/
typedef struct
{
    INT8U   completionCode;
    INT8U   openBladeId;
} PACKED RenegotiatePwrRes_T;

/**
 * @struct GetServiceInfoReq_T
 * @brief Get Service Information from the serice index
**/
typedef struct
{
    INT8U    openBladeId;
    INT8U    serviceIndex;
} PACKED OBSMGetServiceInfoReq_T;

/**
 * @struct GetServiceInfoRes_T
 * @brief Get Service Information from the serice index
**/
typedef struct
{
    INT8U   completionCode;
    INT8U   openBladeId;
    INT8U   serviceClass;
    INT8U   transport;
    INT8U   enterpriseNumber[3];
    INT16U  protocolNumber;
    INT8U   protocolCapabilities;
    INT8U   maxSessionSupport;
    INT8U   serviceName[MAX_SERVICE_NAME_LEN];
    INT8U   portNumberCount;
    INT8U   portNumbers[MAX_STRING_LEN]; /* 2 bytes port number, support max upto 64 port number */
} PACKED OBSMGetServiceInfoRes_T;

/**
 * @struct GetAppletPackageURIReq_T
 * @brief Get Applet Package URI from the blade
**/
typedef struct
{
    INT8U openBladeId;
} PACKED OBSMGetAppletPackageURIReq_T;

/**
 * @struct GetAppletPackageURIRes_T
 * @brief Get Applet Package URI from the blade
**/
typedef struct
{
    INT8U completionCode;
    INT8U openBladeId;
    INT8U versionString[MAX_STRING_LEN];
    INT8U appletPackageURI[MAX_URI_LEN];
} PACKED OBSMGetAppletPackageURIRes_T;

/**
 * @struct GetServiceEnableStateReq_T
 * @brief Get service enable state
**/
typedef struct
{
    INT8U openBladeId;
} PACKED OBSMGetServiceEnableStateReq_T;

/**
 * @struct GetServiceEnableStateRes_T
 * @brief Get service enable state
**/
typedef struct
{
    INT8U completionCode;
    INT8U openBladeId;
    INT32U serviceIndexBitmap;
} PACKED OBSMGetServiceEnableStateRes_T;

/**
 * @struct SetServiceEnableStateReq_T
 * @brief Set service enable state
**/
typedef struct
{
    INT8U openBladeId;
    INT8U stateOperation;
    INT32U serviceIndexBitmap;
} PACKED OBSMSetServiceEnableStateReq_T;

/**
 * @struct SetServiceEnableStateRes_T
 * @brief Set service enable state
**/
typedef struct
{
    INT8U completionCode;
    INT8U openBladeId;
    INT32U serviceIndexBitmap;
} PACKED OBSMSetServiceEnableStateRes_T;

/**
 * @struct SetServiceTicketReq_T
 * @brief Set service ticket
**/
typedef struct
{
    INT8U openBladeId;
    INT8U serviceIndex;
    INT8U expirySeconds;
    INT8U serviceTicket[MAX_TICKET_LEN];
} PACKED OBSMSetServiceTicketReq_T;

/**
 * @struct SetServiceTicketRes_T
 * @brief Set service ticket
**/
typedef struct
{
    INT8U completionCode;
    INT8U openBladeId;
    INT8U serviceSessionId;
} PACKED OBSMSetServiceTicketRes_T;

/**
 * @struct StopServiceSessionReq_T
 * @brief Stop service session
**/
typedef struct
{
    INT8U openBladeId;
    INT8U serviceIndex;
    INT8U serviceSessionId;
    INT8U terminationString[MAX_STRING_LEN];
} PACKED OBSMStopServiceSessionReq_T;

/**
 * @struct StopServiceSessionRes_T
 * @brief Stop service session
**/
typedef struct
{
    INT8U completionCode;
    INT8U openBladeId;
    INT8U serviceSessionId;
} PACKED OBSMStopServiceSessionRes_T;

/**
 * Chassis Inventory Records Structures
**/

/**
 * @struct FRUMultiRecHdr_T
 * @brief FRU MultiRecord Header
**/
typedef struct
{
	INT8U	recTypeId;
    INT8U	recFormatVersionEOL;
    INT8U	recLength;
    INT8U	recChecksum;	/* zero checksum */
    INT8U	hdrChecksum;	/* zero checksum */
} PACKED FRUMultiRecHdr_T;

/**
 * @struct ChassisInvInfoHdr_T
 * @brief Chassis Inventory FRU Header
**/
typedef struct
{
	FRUMultiRecHdr_T	fruMultiRecHdr;
	INT8U				mfgId[3];
	INT8U				openBladeRecId;
	INT8U				recFormVer;
} PACKED ChassisInvInfoHdr_T;

/**
 * @struct SlotMapSubRec_T
 * @brief Open Blade Slot Map sub Record
**/
typedef struct
{
	INT8U	modClassType;
	INT8U	slotIdIns;
} PACKED SlotMapSubRec_T;

/**
 * @struct SlotMapRec_T
 * @brief Open Blade Slot Map Record
**/
typedef struct
{
	ChassisInvInfoHdr_T	chassisInvInfoHdr;
	INT8U				slotCount;
	SlotMapSubRec_T		slotMapSubRec[MAX_SLOT_NUM_SUPPORTED];
} PACKED SlotMapRec_T;

/**
 * @struct InterConnTopSubRec_T
 * @brief Open Blade Interconnection topology sub record
**/
typedef struct
{
	INT8U	bladeSlotNum;
	INT8U	bladeChanNum;
} PACKED InterConnTopSubRec_T;

/**
 * @struct InterConnTopRec_T
 * @brief Open Blade Interconnection topology record
**/
typedef struct
{
	ChassisInvInfoHdr_T		chassisInvInfoHdr;
	INT8U					switchSlotNum;
	INT8U					channelCount;
	InterConnTopSubRec_T	interConnTopSubRec[MAX_CHAN_NUM_SUPPORTED];
} PACKED InterConnTopRec_T;

/**
 * @struct ChassisPwrDomRec_T
 * @brief Open Blade Chassis Power Domain record
**/
typedef struct
{
	ChassisInvInfoHdr_T		chassisInvInfoHdr;
	INT8U					pwrUnitSlotNum;
	INT8U					secPwrUnitSlotNum;
	INT8U					firstPwrSupplySlotNum;
	INT8U					slotCount;
	INT8U					pwrdSlotNum[MAX_SLOT_NUM_SUPPORTED];
} PACKED ChassisPwrDomRec_T;

/**
 * @struct PwrUnitRedundRec_T
 * @brief Open Blade Power Unit Redundancy record
**/
typedef struct
{
	ChassisInvInfoHdr_T		chassisInvInfoHdr;
	INT8U					pwrSupplySlotCount;
	INT8U					reqPwrSupplyCount;
} PACKED PwrUnitRedundRec_T;

/**
 * @struct OutputCurrRec_T
 * @brief Open Blade Output current record
**/
typedef struct
{
	ChassisInvInfoHdr_T		chassisInvInfoHdr;
	INT8U					currScaleFactor;
	INT16U					max12vCurr;
} PACKED OutputCurrRec_T;

/**
 * @struct CoolingCtlrInfo_T
 * @brief Open Blade Chassis Cooling controller information
**/
typedef struct
{
	INT8U	slotNum;
	INT8U	coolDomId;
} PACKED CoolingCtlrInfo_T;

/**
 * @struct ChassisCoolingDomRec_T
 * @brief Open Blade Chassis Cooling Domain record
**/
typedef struct
{
	ChassisInvInfoHdr_T		chassisInvInfoHdr;
	INT8U					ctlrCount;
	CoolingCtlrInfo_T		ctlrInfo[MAX_COOLING_CTLR_SUPPORTED];
	INT8U					chassisSlotCount;
	INT8U					chassisSlotZoneNum[MAX_SLOT_NUM_SUPPORTED];
} PACKED ChassisCoolingDomRec_T;

/**
 * @struct HdrSlotMapRecord_T
 * @brief Record Tables With Valid Field Header
**/
typedef struct
{
	INT8U			valid;
	SlotMapRec_T	slotMapRec;
} PACKED  HdrSlotMapRec_T;

/**
 * @struct HdrInterConnTopRec_T
 * @brief Record Tables With Valid Field Header
**/
typedef struct
{
	INT8U				valid;
	InterConnTopRec_T	interConnTopRec;
} PACKED  HdrInterConnTopRec_T;

/**
 * @struct HdrChassisPwrDomRec_T
 * @brief Record Tables With Valid Field Header
**/
typedef struct
{
	INT8U					valid;
	ChassisPwrDomRec_T		chassisPwrDomRec;
} PACKED  HdrChassisPwrDomRec_T;

/**
 * @struct HdrPwrUnitRedundRec_T
 * @brief Record Tables With Valid Field Header
**/
typedef struct
{
	INT8U					valid;
	PwrUnitRedundRec_T		pwrUnitRedundRec;
} PACKED  HdrPwrUnitRedundRec_T;

/**
 * @struct HdrOutputCurrRec_T
 * @brief Record Tables With Valid Field Header
**/
typedef struct
{
	INT8U					valid;
	OutputCurrRec_T			outputCurrRec;
} PACKED  HdrOutputCurrRec_T;

/**
 * @struct HdrChassisCoolingDomRec_T
 * @brief Record Tables With Valid Field Header
**/
typedef struct
{
	INT8U					valid;
	ChassisCoolingDomRec_T	chassisCoolingDomRec;
} PACKED  HdrChassisCoolingDomRec_T;

/**
 * @struct ChassisInvRecInfo_T
 * @brief Chassis Inventory Info
**/
typedef struct
{
	INT8U					totNumChassisPwrDomRec;
	INT8U					totNumChassisCoolingDomRec;
	HdrSlotMapRec_T			hdrSlotMapRec;
	HdrInterConnTopRec_T	hdrInterConnTopRec;
	HdrChassisPwrDomRec_T	hdrChassisPwrDomRec[MAX_PWR_DOMAIN_SUPPORTED];
	HdrPwrUnitRedundRec_T	hdrPwrUnitRedundRec;
	HdrOutputCurrRec_T		hdrOutputCurrRec;
	HdrChassisCoolingDomRec_T	hdrChassisCoolingDomRec[MAX_COOLING_DOMAIN_SUPPORTED];
} PACKED ChassisInvRecInfo_T;

#pragma pack()
#endif
