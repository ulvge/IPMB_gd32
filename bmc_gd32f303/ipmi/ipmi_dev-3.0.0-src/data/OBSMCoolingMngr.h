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
 * @file	OBSMCoolingMngr.h
 * @author	Velu	<velmuruganv@amiindia.co.in>
 * @brief
 ****************************************************************/
#ifndef OBSMCOOLINGMNGR_H
#define OBSMCOOLINGMNGR_H
#include "Types.h"

/*** Definitions and Macros ***/
#define MOD_AGGREG_THERM_SENSOR_EVENT_TYPE      0x7F

#define THERM_STATE_OFF                         0x01
#define THERM_STATE_COLD                        0x02
#define THERM_STATE_COOL                        0x04
#define THERM_STATE_WARM                        0x08
#define THERM_STATE_HOT                         0x10
#define THERM_STATE_HOTTER                      0x20
#define THERM_STATE_WARNING                     0x40
#define THERM_STATE_CRITICAL                    0x80

#define PARAM_COOLINGMNGR_TIMER_TICK               1
#define PARAM_COOLINGMNGR_SENSOR_EVT_MSG           2
#define PARAM_COOLINGMNGR_FORCE_FAN_STATE_CHANGE   3
#define PARAM_COOLINGMNGR_FAN_STATE_CHANGE         4


/*** Typedef ***/

/**
 * @enum AggregThermSensorSevType_E
 * @brief Aggregate thermal sensor severity type
**/
typedef enum{
	NON_CRITICAL_ASSERTED_OFFSET,
	CRITICAL_ASSERTED_OFFSET,
	NON_RECOVERABLE_ASSERTED_OFFSET,
	UNKNOWN_SEVERITY = 0x0F
} AggregThermSensorSevType_E;

/*** Extern Definitions ***/

/*** Function Prototypes ***/

/**
 * @brief Get the slot information in the cooling domain
 * @param domNum is the cooling domain number
 * @param pSlotCount is the number of slots in that domain
 * @param pSlotList is the list of slot belonging to the domain
 * @return 0 if success, else -1
**/
extern int OBSM_GetCoolingDomSlotInfo (INT8U domNum, INT8U* pSlotCount, INT8U* pSlotList, int BMCInst);

/**
 * @brief Get the devices in the cooling domain
 * @param domNum is the cooling domain number
 * @return count of devices
**/
extern INT8U OBSM_GetCoolingDomDeviceCount (INT8U domNum, int BMCInst);

/**
 * @brief Invoked by cooling manager task
 * @param evtType is the type of event message received
 * @param pEvent is the SEL event message
 * @return 0 if success, -1 if error
**/
extern int OBSM_FanStateChangeEventHndlr (INT8U evtType, SELEventRecord_T *pEvent, int BMCInst );

/**
 * @brief Process the cooling domain power
 * @param domNum is the cooling domain number
 * @return 0 if success, -1 if error
**/
extern int OBSM_ProcessCoolingDomPwr (INT8U domNum, int BMCInst);

/**
 * @brief Method to intialize the cooling specific structs
 * @param None
 * @return 0 if success, -1 if error
**/
extern int OBSM_InitChassisCoolingInfo (int BMCInst);

/**
 * @brief OBSM Cooling manager task.
**/
extern void* OBSMCoolingMngrTask (void *pArg);

#endif /*OBSMCOOLINGMNGR_H*/
