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
 * sensor.h
 * Sensor functions.
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 *          
 ******************************************************************/
#ifndef SENSOR_H
#define SENSOR_H

#include "Types.h"
#include "IPMI_Sensor.h"
#include "SDRRecord.h"
#include "SensorMonitor.h"


#define SENSOR_FAN_NUM 4  // no use
#define SENSOR_TEMP_NUM 4
#define SENSOR_VOLTAGE_NUM 4
#define SENSOR_NUM (SENSOR_TEMP_NUM + SENSOR_VOLTAGE_NUM)

/**
 * @brief Sensor Types
 */
#define PROCESSOR				0x07
#define POWER_SUPPLY				0x08
#define MEMORY					0x0c
#define ENTITY_PRESENCE				0x25
#define BATTERY					0x29


/**
 * @brief Sensor presence bit
 */
#define PROCESSOR_PRESENCE_DETECTED				0x07
#define	POWER_SUPPLY_PRESENCE_DETECTED				0x00
#define POWER_SUPPLY_OUT_OF_RANGE_PRESENT			0x05
#define MEMORY_PRESENCE_DETECTED				0x06
#define	ENTITY_PRESENCE_ENTITY_PRESENT				0x00
#define	BATTERY_PRESENCE_DETECTED				0x02

/**
 * @brief Event type codes
 */
#define GENERIC_EVENT_TYPE_DEV_PRESENCE		0x08
#define GENERIC_EVENT_TYPE_DEV_AVAILABLE	0x09
#define EVENT_TYPE_SENSOR_SPECIFIC		0x6f



#define EVENT_AND_SCANNING_ENABLE (BIT7|BIT6)
#define READING_UNAVAILABLE (BIT5)

/**
 * @brief Sensor Reading for Device
 */
#define DEVICE_PRESENT		0x01
#define DEVICE_ENABLED		0x01

/*** External Definitions ***/
#define THRESHOLD_SENSOR_CLASS      0x01

#define GET_SETTABLE_SENSOR_BIT(FLAGS)  (FLAGS & 0x80)
#define GET_EVENT_DATA_OP(OP)      ( ((OP) >> 6))
#define GET_ASSERT_EVT_OP(OP)      ( ((OP) >> 4) & 0x03)
#define GET_DEASSERT_EVT_OP(OP)    ( ((OP) >> 2) & 0x03)
#define GET_SETSENSOR_OP(OP)       ( (OP) & 0x03)


#define CLEAR_ASSERT_BITS            0x3
#define SET_ASSERT_BITS              0x2
#define WRITE_ASSERT_BITS            0x1

#define CLEAR_DEASSERT_BITS            0x3
#define SET_DEASSERT_BITS              0x2
#define WRITE_DEASSERT_BITS            0x1


#define WRITE_NO_EVTDATA1             0x02
#define WRITE_EVTDATA1                0x01
#define USE_SM_EVTDATA               0x00


#define DCMI_INLET_TEMP_ENTITY_ID           0x40
#define DCMI_CPU_TEMP_ENTITY_ID             0x41
#define DCMI_BASEBOARD_TEMP_ENTITY_ID       0x42

#define IPMI_INLET_TEMP_ENTITY_ID           0x37
#define IPMI_CPU_TEMP_ENTITY_ID             0x03
#define IPMI_BASEBOARD_TEMP_ENTITY_ID       0x07

// Assertion bits for discrete sensor
#define STATE_0_ASSERTED   0x01
#define STATE_1_ASSERTED   0x02
#define STATE_2_ASSERTED   0x04
#define STATE_3_ASSERTED   0x08
#define STATE_4_ASSERTED   0x10
#define STATE_5_ASSERTED   0x20
#define STATE_6_ASSERTED   0x40
#define STATE_7_ASSERTED   0x80
#define STATE_8_ASSERTED   0x01
#define STATE_9_ASSERTED   0x02
#define STATE_10_ASSERTED  0x04
#define STATE_11_ASSERTED  0x08
#define STATE_12_ASSERTED  0x10
#define STATE_13_ASSERTED  0x20
#define STATE_14_ASSERTED  0x40

// FRU State events
#define FRU_NOT_INSTALLED               0x00
#define FRU_INACTIVE                    0x01
#define FRU_ACTIVATION_REQUESTED        0x02
#define FRU_ACTIVATION_IN_PROGRESS      0x03
#define FRU_ACTIVE                      0x04
#define FRU_DEACTIVATION_REQUESTED      0x05
#define FRU_DEACTIVATION_IN_PROGRESS    0x06
#define FRU_COMMUNICATION_LOST          0x07

// ACPI state events
#define ACPI_S0_WORKING                 0x00
#define ACPI_S1_SLEEPING                0x01
#define ACPI_S2_SLEEPING                0x02
#define ACPI_S3_SLEEPING                0x03
#define ACPI_S4_SUSPTODISK              0x04
#define ACPI_S5_SOFTOFF                 0x05
#define ACPI_G3_MECHANICAL              0x07
#define ACPI_G1_SLEEPING                0x09
#define ACPI_S5_OVERRIDE                0x0A
#define ACPI_LEGACY_ON                  0x0B
#define ACPI_LEGACY_OFF                 0x0C

/**
 * @brief Initialize Sensor information.
 * @return 0 if success, -1 if error
**/
extern int InitSensor (int BMCInst);

/**
 * @brief Initialize the scanning bit of each sensor
 * @return 0 if success, -1 if error
 */
extern int InitSensorScanningBit (int BMCInst);

/**
 * @brief Get sensor's SDR record.
 * @param SensorNum - Sensor number.
 * @return the sensor's SDR record.
**/
extern SDRRecHdr_T* SR_GetSensorSDR (INT8U SensorNum,int BMCInst);


extern int GetRecordIdsforDCMISensor (INT8U EntityID,INT8U SensorType,INT8U EntityInstance, 
		INT8U StartingEntityInstance, INT16U* pBuf, INT8U* pTotalValidInstances,int BMCInst);

extern int GetDCMITempReading(INT8U EntityID, INT8U SensorType,INT8U EntityInstance,
                            INT8U StartingEntityInstance, INT8U* pBuf, INT8U* pTotalValidInstances,int BMCInst);

/**
 * @defgroup sdc Sensor Device Commands
 * @ingroup senevt
 * IPMI Sensor Device interface command handlers. These commands are 
 * used for configuring hysterisis and thresholds and reading
 * sensor values.
 * @{
**/
extern int GetDevSDRInfo (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetDevSDR (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int ReserveDevSDRRepository (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetSensorType (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSensorType (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int ReArmSensor (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSensorEventStatus (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetSensorHysterisis (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSensorHysterisis (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetSensorThresholds (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSensorThresholds (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSensorReadingFactors (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetSensorEventEnable (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSensorEventEnable (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSensorReading (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetSensorReading (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
int CompareValues(BOOL isSigned, INT8U val1, INT8U val2);
/** @} */

/**
 * @brief Get Sensor Hysterisis.
 * @param SensorNum - Sensor number.
 * @param Res       - Response data.
 * @return 0 if success, -1 if error.
**/
extern int SR_GetSensorHysterisis  (INT8U SensorNum, GetSensorHysterisisRes_T* Res);

/**
 * @brief Get Sensor Threshold.
 * @param SensorNum - Sensor number.
 * @param Res       - Response data.
 * @return 0 if success, -1 if error.
**/
extern int SR_GetSensorThreshold   (INT8U SensorNum, GetSensorThresholdRes_T* Res);

/**
 * @brief Get Sensor Event Enables.
 * @param SensorNum - Sensor number.
 * @param Res       - Response data.
 * @return 0 if success, -1 if error.
**/
extern int SR_GetSensorEventEnable (INT8U  SensorNum, GetSensorEventEnableRes_T* Res);

/**
 * @brief Get Sensor Reading.
 * @param SensorNum - Sensor number.
 * @return the Sensor reading.
**/
extern INT16U SM_GetSensorReading (INT8U SensorNum, INT16U *pSensorReading);

/**
 * @brief SR_LoadSDRDefaults.
 * @param sr - SDR Record , pSensorInfo - Sensor information.
 * @return none.
**/

extern void SR_LoadSDRDefaults (SDRRecHdr_T* sr, SensorInfo_T* pSensorInfo,int BMCInst);


/**
 * @brief SR_FindSDR.
 * @param SensorNum - Finds SDR for this sensor number.
 * @return none.
**/

extern SDRRecHdr_T* SR_FindSDR (INT8U SensorNum, INT8U SensorOwnerLUN, INT8U SensorOwnerID, int BMCInst);	/* Multi-LUN support to find the SDR based on OwnerLUN, OwnerID, and Sensor Number */


#endif  /* SENSOR_H */


