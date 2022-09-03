/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        Suite 200, 5555 oakbrook pkwy, Norcross            **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 *****************************************************************
 *
 * DCMDevice.h
 * DCMDevice Commands Handler
 *
 * Author: Rama Bisa <ramab@ami.com>
 * 
 *****************************************************************/
#ifndef DCMDEVICE_H
#define DCMDEVICE_H
#include "Types.h"

#define FRU_SUB_AREA_NO_DATA   0xC1
#define FRU_PRODUCT_INFO_LEN   128
#define DCMI_FRU_TYPE_CODE     6
#define CC_FRU_ENCODING_TYPE   0x80
#define PRODUCT_INFO_LANG_CODE 2
#define ENCODING_ASCII_LATIN1  0x03

#define DCMI_MAJOR_REVISION					1
#define DCMI_MINOR_REVISION_v1_5			5
#define DCMI_MINOR_REVISION_v1_0			0

#define POWER_LIMIT_OUT_OF_RANGE    -1
#define CORRECTION_TIME_OUT_OF_RANGE    -2
#define THERMAL_LIMIT_OUT_OF_RANGE      -1
#define THERMAL_EXCEPTION_TIME_OUT_OF_RANGE     -2


/******************************************************/
/* INTERNAL MACRO DEFINITIONS FOR DCMI COMMANDS        */
/******************************************************/
#define	SUPPORTED_DCMI_CAPABILITIES			1
#define MANDATORY_PLATFORM_ATTRIBUTES		2
#define OPTIONAL_PLATFORM_ATTRIBUTES		3
#define MANAGEABILITY_ACCESS_ATTRIBUTES		4
#define ENHANCED_SYSTEM_POWER_ATTRIBUTES	5  /* For DCMI version 1.1 */

#define IDENTIFICATION_SUPPORT				(1 << 0)
#define SEL_LOG_SUPPORT						(1 << 1)
#define CHASSIS_POWER_SUPPORT				(1 << 2)
#define TEMP_MONITOR_SUPPORT				(1 << 3)
#define PWR_MANAGEMENT_SUPPORT				(1 << 0)
#define KCS_SUPPORT							(1 << 0)
#define TMODE_SUPPORT						(1 << 1)
#define SECONDARY_LAN_SUPPORT				(1 << 2)
#define PRIMARY_LAN_SUPPORT					(1 << 3)
#define SOL_SUPPORT							(1 << 4)
#define VLAN_SUPPORT						(1 << 5)
#define SEL_AUTO_ROLLOVER_SUPPORT_CHECK			 15
#define SEL_ROLLOVER_FLUSH_ENTIRE                   (1<<14)
#define SEL_ROLLOVER_FLUSH_RECORD_LEVEL               (1<<13)
#define ASSET_TAG_SUPPORT					(1 << 2)
#define DHCP_HOST_NAME_SUPPORT				(1 << 1)
#define GUID_SUPPORT						(1 << 0)
#define BASEBOARD_TEMP_MON_SUPPORT			(1 << 2)
#define PROCESSORS_TEMP_MON_SUPPORT			(1 << 1)
#define INLET_TEMP_MON_SUPPORT				(1 << 0)
#define SAMPLING_FREQUENCY_TEMP_MON			(1 << 0)
#define DCMI_PARAMETER_REVISION				2
#define PRIMARY_LAN_CHANNEL_NUMBER			LAN_RMCP_CHANNEL
#define SECONDARY_LAN_CHANNEL_NUMBER		0xFF /* 0xFF for not supported */
#define TMODE_OOB_CHANNEL_NUMBER			0xFF /* 0xFF for not supported */
#define MAX_ASSET_TAG_LEN					63
#define MAX_OFFSET_VALUE					62
#define MAX_IDENTIFIERSTRING_LENGTH			64
#define MAX_IDENTIFIERSTRING_OFFSET			63
#define FRU_PRODUCT_INFO_AREA_OFFSET		4
#define DCMI_TEMP_SENSOR_TYPE				1
#define ACTIVATE_POWER_LIMIT_CODE			0x01
#define DEACTIVATE_POWER_LIMIT_CODE			0x00
#define MAX_EXCEPTION_ACTION_CODE			0x11
#define MODE_POWER_STATS					0x01
#define MODE_ENHANCED_POWER_STATS			0x02
#define DEFAULT_TIMEOUT 					100
#define	SUPPORTED_DCMI_CAPABILITIES			1
#define MANDATORY_PLATFORM_ATTRIBUTES		2
#define OPTIONAL_PLATFORM_ATTRIBUTES		3
#define GET_POWER_LIMIT_RESERVED_BYTE        0xFF
#define SET_THERMAL_LIMIT_RESERVED           0x8F


extern int GetDCMICapabilityInfo (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int GetPowerReading (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int GetPowerLimit (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int SetPowerLimit (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int ActivatePowerLimit (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int GetDCMISensorInfo (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int GetAssetTag (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern void DCMIPowerSamplingTask (int BMCInst);
extern int SetAssetTag (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int GetManagementControllerIdString (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int SetManagementControllerIdString (INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern void DCMIThermalSamplingTask(int BMCInst);
extern int SetThermalLimit(INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int GetThermalLimit(INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);
extern int GetTemperatureReading(INT8U* pReq, INT8U ReqLen, INT8U* pRes,_NEAR_ int BMCInst);

#endif  /* IPMDEVICE_H */
