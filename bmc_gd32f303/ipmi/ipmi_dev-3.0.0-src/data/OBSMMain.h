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
 * @file	OBSMMain.h
 * @author	Velu	<velmuruganv@amiindia.co.in>
 * @brief
 ****************************************************************/

#ifndef OBSM_MAIN_H
#define OBSM_MAIN_H

#include "Types.h"
#include "IPMI_OBSM+.h"
#include "OBSMPort.h"
#include "IPMI_SEL.h"

/*** Definitions and Macros ***/
#define OBSM_MAJOR_VERSION                  0x01
#define OBSM_MINOR_VERSION                  0x00
#define OBSM_EXTN_VERSION                   ((OBSM_MAJOR_VERSION << 4) | (OBSM_MINOR_VERSION))
#define MAX_SUPPORTED_FRU_DEV_ID            255
#define OBSM_MOD_OPSTATE_SENSOR_TYPE        0x2C
#define MAX_SIZE_GUID                       8
#define MAX_CHASSIS_MGR_OEM_DATA_SIZE       8
#define MAX_BLADE_OEM_DATA_SIZE             8
#define MAX_SWITCH_OEM_DATA_SIZE            8

#define MAC_ADDR_LEN                        6
#define IP_ADDR_LEN                         4

#define TYPE_FRU_DEV_ID                     0x00
#define TYPE_PHY_SLOT_NUM                   0x01
#define TYPE_FRU_DEV_BMI_ADDR               0x02

#define CHASSISINFO_SHM_KEY                 0x1000

#define OBSM_TIMER_TICK_INTERVAL_IN_SEC     1

/* OBSM Task params */
#define PARAM_OBSM_TIMER_TICK               1
#define PARAM_OBSM_PLATFORM_EVT_MSG         2
#define PARAM_IPMI_PLATFORM_EVT_MSG         3
#define PARAM_BLADE_INS_REM_EVT_MSG         4
#define PARAM_SWITCH_INS_REM_EVT_MSG        5

/* OBSM return values */
#define OBSM_RET_SUCCESS            0
#define OBSM_RET_UNKNOWN_ERROR      1
#define OBSM_RET_COMM_FAILURE       2
#define OBSM_RET_SESSION_ERROR      3
#define OBSM_RET_CMD_ERROR          4
#define OBSM_RET_PARAM_ERROR        5

/* M State values for OBSM states */
#define M0_STATE	0
#define M1_STATE	1
#define M2_STATE	2
#define M3_STATE	3
#define M4_STATE	4
#define M5_STATE	5
#define M6_STATE	6
#define M7_STATE	7
#define M8_STATE	8

/* State of the devices in the slot */
#define STATUS_DEVICE_INACTIVE    0
#define STATUS_DEVICE_ACTIVE      1
#define STATUS_DEVICE_RECOVERY    2
#define STATUS_DEVICE_ABSENT      3

#define OBSM_SHARED_MEM_TIMEOUT				WAIT_INFINITE

/* Shared memory variables */
#define LOCK_CMM_SHARED_MEM(BMCInst)     OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo[BMCInst].OBSMSharedMemMutex, SHARED_MEM_TIMEOUT)

#define UNLOCK_CMM_SHARED_MEM(BMCInst)   OS_THREAD_MUTEX_RELEASE(&g_BMCInfo[BMCInst].OBSMSharedMemMutex)


#define IS_BLADE_PRESENT(SLOTID, BMCInst)             ( BMC_GET_SHARED_MEM(BMCInst)->ChassisInfo.bladePresBitMap & (1 << SLOTID))
#define GET_BLADE_INFO(SLOTID, BMCInst)               (& BMC_GET_SHARED_MEM(BMCInst)->ChassisInfo.bladeInfo[SLOTID])
#define GET_CHASSIS_MNGR_INFO(SLOTID, BMCInst)        (& BMC_GET_SHARED_MEM(BMCInst)->ChassisInfo.chassisMgrInfo[SLOTID])
#define GET_CHASSIS_PWR_INFO(BMCInst)                 (& BMC_GET_SHARED_MEM(BMCInst)->ChassisInfo.pwrInfo)
#define GET_CHASSIS_COOLING_INFO(BMCInst)             (& BMC_GET_SHARED_MEM(BMCInst)->ChassisInfo.coolingInfo)
#define GET_CHASSIS_INV_REC_INFO(BMCInst)             (& BMC_GET_SHARED_MEM(BMCInst)->ChassisInvRecInfo)
#define GET_CHASSIS_SLOT_MAP_REC(BMCInst)             (& BMC_GET_SHARED_MEM(BMCInst)->ChassisInvRecInfo.hdrSlotMapRec)
#define GET_CHASSIS_PWR_UNIT_REDUND_REC(BMCInst)      (& BMC_GET_SHARED_MEM(BMCInst)->ChassisInvRecInfo.hdrPwrUnitRedundRec)
#define GET_CHASSIS_OUTPUT_CURR_REC(BMCInst)          (& BMC_GET_SHARED_MEM(BMCInst)->ChassisInvRecInfo.hdrOutputCurrRec)
#define GET_CHASSIS_COOLING_DOM_REC(DOMNUM, BMCInst)  (& BMC_GET_SHARED_MEM(BMCInst)->ChassisInvRecInfo.hdrChassisCoolingDomRec[DOMNUM])
#define GET_CHASSIS_SLOT_INFO(BMCInst)                (& BMC_GET_SHARED_MEM(BMCInst)->ChassisSlotInfo)


/* Shared memory variables */
#define LOCK_OBSM_SHARED_MEM()		OS_THREAD_MUTEX_ACQUIRE(&g_hOBSMChassisInfoMutex, OBSM_SHARED_MEM_TIMEOUT)
#define UNLOCK_OBSM_SHARED_MEM()	OS_THREAD_MUTEX_RELEASE(&g_hOBSMChassisInfoMutex)

/*** Typedef ***/
#pragma pack(1)

/**
 * @struct SlotInfo_T
 * @brief Slot information structure
**/
typedef struct
{
	INT8U	modClass;
	INT8U	modType;
	INT8U	slotIns;
	INT8U	slotId;
	INT8U	slotStatus;
	INT8U	busNo;
	INT8U	slaveAddr;
} PACKED SlotInfo_T;

/**
 * @struct ChassisSlotInfo_T
 * @brief Chassis slot information structure
**/
typedef struct
{
	INT8U	totalSlots;
	SlotInfo_T slotInfo [MAX_SLOT_NUM_SUPPORTED];
} PACKED ChassisSlotInfo_T;

/**
 * @struct PwrSupplyInfo_T
 * @brief Chassis power supply information structure
**/
typedef struct
{
	INT8U	pwrDomNum;
	INT8U	slotId;
	INT8U	slotNum;
	INT8U	actvnBar;
	INT8U	deactvnBar;
	INT8U	opStateSensorNum;
	INT8U	currOpState;
	INT8U	prevOpState;
	INT16U	pwrCapacity;
	INT16U	currPwrOutput;
} PACKED PwrSupplyInfo_T;

/**
 * @struct PwrDomInfo_T
 * @brief Chassis power domain information structure
**/
typedef struct
{
	INT8U	primCtlrSlotNum;
	INT8U	primCtlrBus;
	INT8U	primCtlrSlaveAddr;
	INT8U	secCtlrSlotNum;
	INT8U	secCtlrBus;
	INT8U	secCtlrSlaveAddr;

	INT8U	 redundancy;
	INT8U	hotSwap;

	INT8U	totalPwrdSlot;
	INT8U	pwrdSlotNum [MAX_SLOT_NUM_SUPPORTED];
	INT8U	totalPwrSupplySlot;
	INT8U	reqPwrSupply;
	INT8U	firstPwrSupplySlotNum;
	INT16U	maxPwrOfPwrSupply;

	INT16U	totalPwr;
	INT16U	remPwr;

	INT32U	pwrSupplyPresBitMap;
	PwrSupplyInfo_T	pwrSupplyInfo [MAX_PWR_SUPPLY_SUPPORTED_PER_DOMAIN];
} PACKED PwrDomInfo_T;

/**
 * @struct PwrInfo_T
 * @brief Chassis power information structure
**/
typedef struct
{
	INT8U	initComplete;
	INT8U	totalPwrDom;
	PwrDomInfo_T pwrDomInfo [MAX_PWR_DOMAIN_SUPPORTED];
} PACKED ChassisPwrInfo_T;

/**
 * @struct CoolingDomCtlrInfo_T
 * @brief Chassis cooling controller information structure
**/
typedef struct
{
	INT8U	slotNum;
	INT8U	bus;
	INT8U	slaveAddr;
} PACKED CoolingDomCtlrInfo_T;

/**
 * @struct CoolingDomZoneInfo_T
 * @brief Chassis cooling domain zone information structure
**/
typedef struct
{
	INT8U	domNum;
	INT8U	slotNum;
	INT8U	zoneNum;
	INT8U	readingValid;
	INT8U	sensorNum;
	INT8U	currState;
	INT8U	prevState;
} PACKED CoolingDomSlotInfo_T;

/**
 * @struct CoolingDomZoneInfo_T
 * @brief Chassis cooling domain zone information structure
**/
typedef struct
{
	INT16U				 totalCooledSlots;
	CoolingDomSlotInfo_T cooledSlotInfo [MAX_SLOT_NUM_SUPPORTED * MAX_COOLING_ZONE_SUPPORTED_PER_DOMAIN];
} PACKED CoolingDomZoneInfo_T;

/**
 * @struct CoolingDomTempInfo_T
 * @brief Chassis cooling domain temperature information structure
**/
typedef struct
{
	INT8U	currThermState;
	INT8U	prevThermState;
} PACKED CoolingDomTempInfo_T;

/**
 * @struct FanInfo_T
 * @brief Fan information structure
**/
typedef struct
{
	INT8U	domNum;
	INT8U	slotId;
	INT8U	slotNum;
	INT8U	actvnBar;
	INT8U	deactvnBar;
	INT8U	opStateSensorNum;
	INT8U	currOpState;
	INT8U	prevOpState;
	INT16U	pwrRequired;
	INT16U	pwrGranted;
	INT16U	pwrPending;
}PACKED FanInfo_T;

/**
 * @struct CoolingDomModuleInfo_T
 * @brief Chassis cooling module information structure
**/
typedef struct
{
	INT8U	totalFans;
	INT8U	redundancy;
	INT8U	hotSwap;
	INT16U	pwrAllocated;
	INT16U	maxPwrConsumption;
	INT16U	currPwrConsumption;
	INT16U	prevPwrConsumption;
	INT8U	reqFanCount;
	INT32U	fanPresBitMap;
	FanInfo_T	fanInfo [MAX_FAN_SUPPORTED_PER_DOMAIN];
} PACKED CoolingDomModuleInfo_T;

/**
 * @struct CoolingDomInfo_T
 * @brief Chassis cooling domain information structure
**/
typedef struct
{
	INT8U	totalNumOfCtlr;
	INT8U	totalNumOfZones;
	CoolingDomCtlrInfo_T	ctlrInfo [MAX_COOLING_CTLR_SUPPORTED];
	CoolingDomZoneInfo_T	zoneInfo [MAX_COOLING_ZONE_SUPPORTED_PER_DOMAIN];
	CoolingDomTempInfo_T	tempInfo;
	CoolingDomModuleInfo_T	moduleInfo;
} PACKED CoolingDomInfo_T;

/**
 * @struct CoolingInfo_T
 * @brief Chassis cooling information structure
**/
typedef struct
{
	INT8U	initComplete;
	INT8U	totalCoolingDom;
	CoolingDomInfo_T coolingDomInfo [MAX_COOLING_DOMAIN_SUPPORTED];
} PACKED ChassisCoolingInfo_T;

/**
 * @struct ChassisMgrInfo_T
 * @brief Chassis manager information structure
**/
typedef struct
{
	INT8U	initComplete;
	INT8U	guid [MAX_SIZE_GUID];
	INT8U	slotId;
	INT8U	slotNum;
	INT8U	slotCount;
	INT8U	maxFRUDevId;

	INT8U	ipmbBus;
	INT8U	ipmbSlaveAddr;
	INT8U	OEMData [MAX_CHASSIS_MGR_OEM_DATA_SIZE];
} PACKED ChassisMgrInfo_T;

/**
 * @struct BladeLANInfo_T
 * @brief Blade LAN information structure
**/
typedef struct
{
	INT8U	macAddr [MAC_ADDR_LEN];
	INT8U	ipAddr [IP_ADDR_LEN];
	INT16U	port;
} PACKED BladeLANInfo_T;

/**
 * @struct BladeSlotPwrInfo_T
 * @brief Blade slots power information structure
**/
typedef struct
{
	INT8U	numPwrLevel;
	INT8U	pwrMultiplier;
	INT8U	pwrLevel [MAX_NUM_PWR_LEVEL];
	INT16U	currPwr;
	INT16U	desPwr;
} PACKED BladeSlotPwrInfo_T;

/**
 * @struct BladePwrInfo_T
 * @brief Blade power information structure
**/
typedef struct
{
	INT8U	pwrOnBar;
	INT8U	pwrOffBar;
	INT8U	dynamicCfg;
	INT8U	pwrDrawFormat;
	INT16U	totalCurrPwr;
	INT16U	totalDesPwr;
	INT16U	pwrGranted;
	INT16U	pwrPending;
	BladeSlotPwrInfo_T slotPwrInfo [MAX_NUM_SLOTS_PER_MODULE];
} PACKED BladePwrInfo_T;

/**
 * @struct BladeInfo_T
 * @brief Blade information structure
**/
typedef struct
{
	INT8U	initComplete;
	INT8U	guid [MAX_SIZE_GUID];
	INT8U	slotId;
	INT8U	slotNum;
	INT8U	slotCount;
	INT8U	slotlist [MAX_NUM_SLOTS_PER_MODULE];
	INT8U	maxFRUDevId;
	INT8U	obsmSupport;

	INT8U	ipmbBus;
	INT8U	ipmbSlaveAddr;

	INT8U	currOpState;
	INT8U	prevOpState;

	BladeLANInfo_T	lanInfo;

	BladePwrInfo_T	pwrInfo;

	INT8U	OEMData [MAX_BLADE_OEM_DATA_SIZE];
} PACKED BladeInfo_T;

/**
 * @struct SwitchInfo_T
 * @brief Switch information structure
**/
typedef struct
{
	INT8U	initComplete;
	INT8U	guid [MAX_SIZE_GUID];
	INT8U	slotId;
	INT8U	slotNum;
	INT8U	slotCount;

	INT8U	ipmbBus;
	INT8U	ipmbSlaveAddr;

	INT8U	OEMData [MAX_SWITCH_OEM_DATA_SIZE];
} PACKED SwitchInfo_T;

/**
 * @struct ChassisInfo_T
 * @brief Chassis information structure
**/
typedef struct
{
	INT32U					chassisMgrPresBitMap;
	ChassisMgrInfo_T		chassisMgrInfo [MAX_CHASSIS_MNGR_SUPPORTED];
	INT32U					bladePresBitMap;
	BladeInfo_T				bladeInfo [MAX_BLADE_SUPPORTED];
	INT32U					switchPresBitMap;
	SwitchInfo_T			switchInfo [MAX_SWITCH_SUPPORTED];
	ChassisPwrInfo_T		pwrInfo;
	ChassisCoolingInfo_T	coolingInfo;
} PACKED ChassisInfo_T;

#pragma pack()

/*** Extern Definitions ***/
extern ChassisInfo_T *pChassisInfo;
extern ChassisInvRecInfo_T g_ChassisInvRecInfo;
extern ChassisSlotInfo_T g_ChassisSlotInfo;

/*** Function Prototypes ***/

/**
 * @brief Verify the validity of FRUDevId
 * @param fruDevId is the FRU device Id to validate
 * @return TRUE on success, else FALSE.
**/
extern BOOL SSICMM_IsValidFRUDevId(INT8U fruDevId);

/**
 * @brief Verify the validity of physical slot number
 * @param slotNum is the slot number to validate
 * @return TRUE on success, else FALSE.
**/
extern BOOL SSICMM_IsValidPhySlotNum(INT8U slotNum, int BMCInst);

/**
 * @brief Verify the validity of IPMB address for that devId
 * @param fruDevId is the FRU device Id
 * @param bmiAddr is the IPMB slave address
 * @return TRUE on success, else FALSE.
**/
extern BOOL SSICMM_IsValidFRUDevBMIAddr(INT8U fruDevId, INT8U bmiAddr, int BMCInst);

/**
 * @brief Verify the validity of IPMB address
 * @param bmiAddr is the IPMB slave address
 * @return TRUE on success, else FALSE.
**/
extern BOOL SSICMM_IsValidBladeBMIAddr(INT8U bmiAddr, int BMCInst);

/**
 * @brief Get the total slots occupied by the module
 * @param reqType is the type of request which shows the validity of each parameter
 * @param fruDevId is the FRU device Id
 * @param bmiAddr is the IPMB slave address
 * @param slotNum is the slot number of the occupied slot
 * @param pSlotCount is the slot count occuppied by the device
 * @return 0 if success -1 if error.
**/
extern int SSICMM_GetSlotCount (INT8U reqType, INT8U fruDevId, INT8U bmiAddr, INT8U slotNum, INT8U* pSlotCount, int BMCInst);

/**
 * @brief Get the first slot occupied by the module
 * @param reqType is the type of request which shows the validity of each parameter
 * @param fruDevId is the FRU device Id
 * @param bmiAddr is the IPMB slave address
 * @param slotNum is the slot number of the occupied slot
 * @param pFirstSlotNum is the first occupied slot number
 * @return 0 if success -1 if error
**/
extern int SSICMM_GetFirstSlotNum (INT8U reqType, INT8U fruDevId, INT8U bmiAddr, INT8U slotNum, INT8U* pFirstSlotNum,int BMCInst);

/**
 * @brief Get the IPMB address of the module
 * @param reqType is the type of request which shows the validity of each parameter
 * @param fruDevId is the FRU device Id
 * @param bmiAddr is the IPMB slave address
 * @param slotNum is the slot number of the occupied slot
 * @param pBMIAddr is the slave Address
 * @return 0 if success -1 if error
**/
extern int SSICMM_GetModuleBMIAddr(INT8U reqType, INT8U fruDevId, INT8U bmiAddr, INT8U slotNum, INT8U* pBMIAddr,int BMCInst);

/**
 * @brief Get the site type of the module
 * @param reqType is the type of request which shows the validity of each parameter
 * @param fruDevId is the FRU device Id
 * @param bmiAddr is the IPMB slave address
 * @param slotNum is the slot number of the occupied slot
 * @param pSiteType is the site type
 * @return 0 if success -1 if error
**/
extern int SSICMM_GetModuleSiteType (INT8U reqType, INT8U fruDevId, INT8U bmiAddr, INT8U slotNum, INT8U* pSiteType,int BMCInst);

/**
 * @brief Retrieve the chassis manager information from Slot ID
 * @param SlotId is the Slot Id of Chassis manager
 * @return Chassis manager information
**/
extern ChassisMgrInfo_T* SSICMM_GetChassisMngrInfoFromSlotId (INT8U slotId, int BMCInst);

/**
 * @brief Retrieve the blade information from Slot ID
 * @param SlotId is the Slot Id of Blade
 * @return Blade information
**/
extern BladeInfo_T* SSICMM_GetBladeInfoFromSlotId(INT8U slotId, int BMCInst);

/**
 * @brief Retrieve the blade information from blade number
 * @param bladeNo is the blade number (1 based)
 * @return Blade information
**/
extern BladeInfo_T* SSICMM_GetBladeInfoFromBladeNo (INT8U bladeNo, int BMCInst);

/**
 * @brief Get the slot information
 * @param slotNum is the slot number
 * @return slot info on success, else NULL.
**/
//extern SlotInfo_T* AMIGetSlotInfo (INT8U  slotNum, int BMCInst);

/**
 * @brief Get the BMI (IPMB) information of device in the slot
 * @param slotNum is the slotId of the device
 * @param pIPMBBus is the Bus number of the blade
 * @param pSlaveAddr is the slave address of the blade
 * @return 0 on success, else -1.
**/
extern int SSICMM_GetBMIInfoFromSlotNum (INT8U  slotNum,INT8U* pIPMBBus, INT8U* pSlaveAddr, int BMCInst);

/**
 * @brief Retrieve the blade slotId information from slot number
 * @param slotNum is the slot number in the chassis
 * @return slotId if found, else 0xFF for invalid slotid
**/
extern INT8U SSICMM_GetBladeSlotIdFromSlotNum (INT8U slotNum, int BMCInst);

/**
 * @brief Retrieve the blade slot number from slave address
 * @param slotId is the slot identifier of the blade
 * @return slot number if found, else 0xFF for invalid slotid
**/
extern INT8U SSICMM_GetBladeSlotNumFromSlaveAddr (INT8U slaveAddr, int BMCInst);

/**
 * @brief Invoked by OBSM task on arrival of event
 * @param evtType is the type of event message received
 * @param pEvent is the SEL event message
 * @return 0 if success, -1 if error
**/
extern int SSICMM_BladeStateChangeEventHndlr (INT8U evtType, SELEventRecord_T *pEvent, int BMCInst);

/**
 * @brief This routine is being called from MsgHndlr every n seconds.
 * @param None
 * @return 0 if success, -1 if error
**/
void OBSMTimer(int BMCInst);

/**
 * @brief  OBSM Task
**/
extern void* OBSMTask (void *pArg);

#endif

