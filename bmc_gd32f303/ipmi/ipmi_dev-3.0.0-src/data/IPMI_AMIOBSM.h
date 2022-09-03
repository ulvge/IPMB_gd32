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
 * @file	IPMI_AMIOBSM.h
 * @author	Velu	<velmuruganv@amiindia.co.in>
 * @brief
 ****************************************************************/

#ifndef LIBIPMI_AMIOBSM_H
#define LIBIPMI_AMIOBSM_H


#include "OBSMPort.h"

/*** Definitions and Macros ***/
#define CMD_AMI_GET_SLOT_MAP_INFO			0xA0
#define CMD_AMI_GET_SLOT_INFO				0xA1
#define CMD_AMI_GET_PWR_INFO				0xA2
#define CMD_AMI_GET_PWR_DOM_INFO			0xA3
#define CMD_AMI_GET_PWR_SUPPLY_INFO			0xA4
#define CMD_AMI_GET_COOLING_INFO			0xA5
#define CMD_AMI_GET_COOLING_DOM_INFO		0xA6
#define CMD_AMI_GET_FAN_INFO				0xA7
#define CMD_AMI_GET_BLADE_STATUS			0xA8
#define CMD_AMI_ETH_RESTART_ALL				0xA9

/* Debug Commands */
#define CMD_DBG_GET_CHASSIS_PWR_INFO		0xD0
#define CMD_DBG_GET_CHASSIS_COOLING_INFO	0xD1
#define CMD_DBG_GET_BLADE_INFO				0xD2
#define CMD_DBG_BLADE_INS_REM_EVT			0xD3
#define CMD_DBG_PS_STATE_CHANGE_EVT			0xD4
#define CMD_DBG_FAN_STATE_CHANGE_EVT		0xD5
#define CMD_DBG_THERMAL_STATE_CHANGE_EVT	0xD6

#pragma pack(1)

/*
 * Slot Data Info structure
 */
typedef struct
{
	INT8U	modClass;
	INT8U	modType;
	INT8U	slotIns;
	INT8U	slotId;
	INT8U   slotStatus;
	INT8U   devId[16];
}PACKED SlotData_T;

/*
 * Power Supply Info Structure
 */
typedef struct
{
	INT8U  presence;
	INT8U  status;
	INT16U maxPwr;
}PACKED PwrSupplyStatusInfo_T;

/*
 * Power Domain Info Structure
 */
typedef struct
{
	INT8U  domNum;
	INT8U  redundancy;
	INT8U  redundancyState;
	INT8U  hotSwap;
	INT16U totalPwr;
	INT16U remPwr;
	INT8U  devCount;
	INT8U  pwrSupplyCount;
	INT8U  totalPwrSupplySlot;
	INT8U  reqPwrSupply;
	INT8U  slotCount;
	INT8U  slotList[MAX_SLOT_NUM_SUPPORTED];
}PACKED ChassisPwrDomInfo_T;

/*
 * Fan Info Structure
 */
typedef struct
{
	INT8U tachSensorNum;
	INT8U rdg;
	INT8U presence;
	INT8U status;
}PACKED FanModuleInfo_T;

/*
 * Cooling Domain Info Structure
 */
typedef struct
{
	INT8U  domNum;
	INT8U  redundancy;
	INT8U  redundancyState;
	INT8U  hotSwap;
	INT8U  thermalState;
	INT16U allocPwr;
	INT8U  devCount;
	INT8U  fanCount;
	INT8U  totalFanSlot;
	INT8U  reqFanCount;
	INT8U  slotCount;
	INT8U  slotList[MAX_SLOT_NUM_SUPPORTED];
}PACKED ChassisCoolingDomInfo_T;

/*
 * Get Power Supply Info structures
 */
typedef struct
{
	INT8U   openBladeId;
	INT8U   domNum;
	INT8U   pwrSupplyId;
} PACKED GetPwrSupplyInfoReq_T;

typedef struct
{
	INT8U   completionCode;
	INT8U   openBladeId;
	PwrSupplyStatusInfo_T pwrSupplyStatusInfo;
} PACKED GetPwrSupplyInfoRes_T;

/*
 * Get Power Domain Info structures
 */
typedef struct
{
	INT8U   openBladeId;
	INT8U   domNum;
} PACKED GetPwrDomInfoReq_T;

typedef struct
{
	INT8U   completionCode;
	INT8U   openBladeId;
	ChassisPwrDomInfo_T chassisPwrDomInfo;
} PACKED GetPwrDomInfoRes_T;

/*
 * Get Power Info structures
 */
typedef struct
{
	INT8U   openBladeId;
} PACKED GetPwrInfoReq_T;

typedef struct
{
	INT8U   completionCode;
	INT8U   openBladeId;
	INT8U   totPwrDomain;
} PACKED GetPwrInfoRes_T;


/*
 * Get Fan Info structures
 */
typedef struct
{
	INT8U   openBladeId;
	INT8U   domNum;
	INT8U   fanId;
} PACKED GetFanInfoReq_T;

typedef struct
{
	INT8U   completionCode;
	INT8U   openBladeId;
	FanModuleInfo_T fanModuleInfo;
} PACKED GetFanInfoRes_T;

/*
 * Get Cooling Domain Info structures
 */
typedef struct
{
	INT8U   openBladeId;
	INT8U   domNum;
} PACKED GetCoolingDomInfoReq_T;

typedef struct
{
	INT8U   completionCode;
	INT8U   openBladeId;
	ChassisCoolingDomInfo_T chassisCoolingDomInfo;
} PACKED GetCoolingDomInfoRes_T;

/*
 * Get Cooling Info structures
 */
typedef struct
{
	INT8U   openBladeId;
} PACKED GetCoolingInfoReq_T;

typedef struct
{
	INT8U   completionCode;
	INT8U   openBladeId;
	INT8U   totCoolingDomain;
} PACKED GetCoolingInfoRes_T;

/*
 * Get Slot Map Info Structures
 */
typedef struct
{
	INT8U	openBladeId;
} PACKED GetSlotMapInfoReq_T;

typedef struct
{
	INT8U	completionCode;
	INT8U	openBladeId;
	INT8U	slotCount;
} PACKED GetSlotMapInfoRes_T;

/*
 * Get Slot Info Structures
 */
typedef struct
{
	INT8U	openBladeId;
	INT8U	slotNum;
} PACKED GetSlotInfoReq_T;

typedef struct
{
	INT8U	completionCode;
	INT8U	openBladeId;
	SlotData_T slotData;
} PACKED GetSlotInfoRes_T;

/*
 * Get Blade Status Structure
 */
typedef struct
{
	INT8U   openBladeId;
	INT8U   slotId;
} PACKED GetBladeStatusReq_T;

typedef struct
{
	INT8U   completionCode;
	INT8U   openBladeId;
	INT8U   bladePresentFlag;
} PACKED GetBladeStatusRes_T;

/*-------------------
 * Debug Commands
 *-------------------*/
/*
 * Get Power Info Structure
 */
typedef struct
{
	INT8U  param;
	INT8U  size;
	INT16U offset;
} PACKED GetChassisPwrInfoReq_T;

typedef struct
{
	INT8U	completionCode;
	INT8U	pwrData [512];
} PACKED GetChassisPwrInfoRes_T;

/*
 * Get Cooling Info Structure
 */
typedef struct
{
	INT8U  param;
	INT8U  size;
	INT16U offset;
} PACKED GetChassisCoolingInfoReq_T;

typedef struct
{
	INT8U	completionCode;
	INT8U	coolingData [512];
} PACKED GetChassisCoolingInfoRes_T;

/*
 * Get Blade Info Structure 
 */
typedef struct
{
	INT8U	slotId;
} PACKED GetBladeInfoReq_T;

typedef struct
{
	INT8U	completionCode;
	INT8U	bladePresentFlag;
	INT8U	bladeInfo[127];
} PACKED GetBladeInfoRes_T;

/*
 * Blade Insertion/Removal Structure
 */
typedef struct
{
	INT8U	slotId;
	INT8U	insertFlag;
} PACKED BladeInsRemEvtReq_T;

typedef struct
{
	INT8U	completionCode;
} PACKED BladeInsRemEvtRes_T;

/*
 * Power supply state change event
 */
typedef struct
{
	INT8U	param;
	INT8U	sensorNum;
} PACKED PSEvtReq_T;

typedef struct
{
	INT8U	completionCode;
} PACKED PSEvtRes_T;

/*
 * Fan state change event
 */
typedef struct
{
	INT8U	param;
	INT8U	sensorNum;
} PACKED FanEvtReq_T;

typedef struct
{
	INT8U	completionCode;
} PACKED FanEvtRes_T;

/*
 * Thermal state change event
 */
typedef struct
{
	INT8U	param;
	INT8U	slotId;
	INT8U	sensorNum;
	INT8U   domId;
	INT8U   thermalState;
} PACKED ThermalEvtReq_T;

typedef struct
{
	INT8U	completionCode;
} PACKED ThermalEvtRes_T;


#pragma pack()
#endif
