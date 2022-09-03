/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2006-2007, American Megatrends Inc.        **
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
 * internal_sensor.h
 * Hanlders for Internal Sensors.
 *
 *  Author: Vinothkumar S <vinothkumars@ami.com>
 *          
 ******************************************************************/
#ifndef INTERNAL_SENSOR_H
#define INTERNAL_SENSOR_H

#include "Types.h"
#include "IPMI_Sensor.h"
#include "SensorMonitor.h"



/*** External Definitions ***/

/*** Global definitions ***/

/* SEL Sensor */
#define EVENT_LOG_AREA_RESET					0x2
#define EVENT_SEL_ALMOST_FULL					0x5
#define EVENT_SEL_IS_FULL					0x4	



/* Watchdog Sensor */
#define EVENT_TIMER_EXPIRED					0x00
#define EVENT_HARD_RESET					0x01
#define EVENT_POWER_DOWN					0x02
#define EVENT_POWER_CYCLE					0x03
#define EVENT_TIMER_INT						0x08

/*RAID Internal Sensor*/
#define RAID_ROC_TEMP1          0x01
#define RAID_ROC_TEMP2          0x02
#define RAID_EXPANDER_TEMP1     0x01
#define RAID_EXPANDER_TEMP2     0x02
#define RAID_BBU_TEMP1          0x01
#define RAID_BBU_TEMP2          0x02


typedef struct
{
    INT8U                       SensorTypeCode;     /* Sensor Type code      */
    INT16U                      SensorReading;      /* Sensor Reading        */
    INT8U                       SensorNum;          /* Sensor Number         */	
    INT16U                      u16AssertionHistory;/* Assertion tracking    */
    INT8U                       u8TmrUse;           /* WDT use               */
    INT8U                       u8TmrActions;       /* WDT actions           */
    INT8U                       u8Restarted;        /* WDT restarted         */
    pPDK_SensorInitHook_T       pInitSensor;        /* Init Sensor hook      */
    pPDK_MonitorHook_T          pPreMonitor;        /* Pre monitor hook      */
    pPDK_MonitorHook_T          pPostMonitor;       /* Post monitor hook     */
    pPDK_MonitorExtHook_T       pPostMonitorExt;    /* Post monitor Ext hook */
    INT8U                       EntityID;           /* Entity ID             */
    INT8U                       EntityInst;         /* Entity Instance       */
    INT32U                       FeatureSupport;     /* Feature Supported or not*/
	INT8U                       SensorOwnerLun;     /* OwnerLUN            */
} InternalSensorTbl_T;


/*** Prototype Declaration ***/

/* Functions for SEL Sensor Implementation */
int SELSensorInit (void*  pSensorInfo,int BMCInst);
int SELMonitor (void* pSenInfo, INT8U* pReadFlags,int BMCInst);
int SELEventLog (void* pSenInfo, INT8U* pReadFlags,int BMCInst);
int SELEventLogExt (void* pSenInfo, INT8U* pReadFlags,int BMCInst);

/* Functions for Watchdog Sensor Implementation */
int WD2SensorInit (void*  pSensorInfo, int BMCInst);
int WD2Monitor (void* pSenInfo, INT8U* pReadFlags,int BMCInst);
int WD2DummyEventLog (void* pSenInfo, INT8U* pReadFlags,int BMCInst);
int WD2EventLog (void* pSenInfo, INT8U* pReadFlags,int BMCInst);
int WD2EventLogExt (void* pSenInfo, INT8U* pReadFlags,int BMCInst);

/* Functions for SSI Compute Blade Operational State Sensor Implementation */
int OpStateSensorInit (void* pSenInfo, int BMCInst);
int OpStateMonitor (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int OpStateEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int OpStateEventLogExt (void* pSenInfo, INT8U* pReadFlags, int BMCInst);

/* Functions for SSI Compute Blade Aggregated Thermal Sensor Implementation */
int AggregateThermalSensorInit (void* pSenInfo, int BMCInst);
int AggregateThermalMonitor (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int AggregateThermalEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int AggregateThermalEventLogExt (void* pSenInfo, INT8U* pReadFlags, int BMCInst);

/* Functions for SSI Compute Blade Aggregated Fault Sensor Implementation */
int AggregateFaultSensorInit (void* pSenInfo, int BMCInst);
int AggregateFaultMonitor (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int AggregateFaultEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int AggregateFaultEventLogExt (void* pSenInfo, INT8U* pReadFlags, int BMCInst);

/* Functions for SSI Compute Blade Service State Sensor Implementation */
int ServiceStateSensorInit (void* pSenInfo, int BMCInst);
int ServiceStateMonitor (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int ServiceStateEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int ServiceStateEventLogExt (void* pSenInfo, INT8U* pReadFlags, int BMCInst);

int RAIDSensorInit (void* pSenInfo, int BMCInst);
int RAIDMonitor (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int RAIDEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst);
int RAIDEventLogExt (void* pSenInfo, INT8U* pReadFlags, int BMCInst);


/*-----------------------------------------
 * GetSELFullPercentage
 *-----------------------------------------*/
extern INT8U  GetSELFullPercentage(int BMCInst);
	
/**
 * @brief GetSensorNumFromSensorType.
 * @param  SensorType       - Sensor Type of the sensor.
 * @return SensorInfo if success, 0xFF if error.
**/
extern void* GetSensorInfoFromSensorType(INT8U SensorType, int BMCInst);


/**
 * @brief SetSELSensorReading.
 * @param Res       - Sensor reading to be set.
 * @return 0 if success, -1 if error.
**/
extern int  SetSELSensorReading (INT16U Reading,int BMCinst);

/**
 * @brief SetWD2SensorReading.
 * @param Res       - Sensor reading to be set.
 * @return 0 if success, -1 if error.
**/
extern int  SetWD2SensorReading (INT16U Reading, INT8U u8TmrUse, INT8U u8TmrActions,int BMCInst); 

extern int  RestartWD2Sensor(int BMCInst);

extern int  WD2SELLog (INT8U Action);


#endif  /* INTERNAL_SENSOR_H */


