/****************************************************************
 ****************************************************************

 ****************************************************************/
/*****************************************************************
 *
 * SensorEvent.c
 * Sensor Event Command Handler
 *
 * Author: 
 * 
 *****************************************************************/
#define UNIMPLEMENTED_AS_FUNC
#include "MsgHndlr.h"
#include "Support.h"
#include "SensorEvent.h"
#include "Events.h"
#include "PEFDevice.h"
#include "Sensor.h"
#include "IPMI_Events.h"
#include "IPMI_PEF.h"
#include "IPMI_Sensor.h"

/*** Global Variables ***/
const CmdHndlrMap_T	g_SensorEvent_CmdHndlr [] =
{
#if EVENT_PROCESSING_DEVICE == 1
    { CMD_SET_EVENT_RECEIVER,			PRIV_ADMIN,		SET_EVENT_RECEIVER,			sizeof (SetEvtRcvReq_T),	0xAAAA ,0xFFFF},
    { CMD_GET_EVENT_RECEIVER,			PRIV_USER,		GET_EVENT_RECEIVER,			0x00,	0xAAAA	,0xFFFF},
    { CMD_PLATFORM_EVENT,				PRIV_OPERATOR, 	PLATFORM_EVENT,				0xFF,	0xAAAA	,0xFFFF},
#endif	/* EVENT_PROCESSING_DEVICE */

#if PEF_DEVICE == 1
    { CMD_GET_PEF_CAPABILITIES,			PRIV_USER,		GET_PEF_CAPABILITIES,		0x00,	0xAAAA	,0xFFFF},											
    { CMD_ARM_PEF_POSTPONE_TIMER,		PRIV_ADMIN,		ARM_PEF_POSTPONE_TIMER,		0x01,	0xAAAA	,0xFFFF},
    { CMD_SET_PEF_CONFIG_PARAMS,		PRIV_ADMIN,		SET_PEF_CONFIG_PARAMS,		0xFF,	0xAAAA	,0xFFFF},											
    { CMD_GET_PEF_CONFIG_PARAMS,		PRIV_OPERATOR,	GET_PEF_CONFIG_PARAMS,		sizeof (GetPEFConfigReq_T),	0xAAAA ,0xFFFF},											
    { CMD_SET_LAST_PROCESSED_EVENT_ID,	PRIV_ADMIN,		SET_LAST_PROCESSED_EVENT_ID,sizeof (SetLastEvtIDReq_T),	0xAAAA	,0xFFFF},									
    { CMD_GET_LAST_PROCESSED_EVENT_ID,	PRIV_ADMIN,		GET_LAST_PROCESSED_EVENT_ID,0x00,	0xAAAA	,0xFFFF},									
    { CMD_ALERT_IMMEDIATE,				PRIV_ADMIN,		ALERT_IMMEDIATE,			0xFF,	0xAAAA  ,0xFFFF},														
    { CMD_PET_ACKNOWLEDGE,				PRIV_NONE,		PET_ACKNOWLEDGE,			sizeof (PETAckReq_T),	0xAAAA	,0xFFFF},														
#endif /* PEF_DEVICE */

#if SENSOR_DEVICE == 1
    { CMD_GET_DEV_SDR_INFO,				PRIV_LOCAL,		GET_DEV_SDR_INFO,			0xFF,	0xAAAA	,0xFFFF},
    { CMD_GET_DEV_SDR,					PRIV_LOCAL,		GET_DEV_SDR,				sizeof(GetDevSDRReq_T),	0xAAAA	,0xFFFF},
    { CMD_RESERVE_DEV_SDR_REPOSITORY,	PRIV_LOCAL,		RESERVE_DEV_SDR_REPOSITORY, 0x00,	0xAAAA	,0xFFFF},
    { CMD_GET_SENSOR_READING_FACTORS,	PRIV_USER,	 	GET_SENSOR_READING_FACTORS, sizeof (GetSensorReadingFactorReq_T),	0xAAAA ,0xFFFF},
    { CMD_SET_SENSOR_HYSTERISIS,		PRIV_OPERATOR, 	SET_SENSOR_HYSTERISIS,		sizeof (SetSensorHysterisisReq_T),	0xAAAA	,0xFFFF},
    { CMD_GET_SENSOR_HYSTERISIS,		PRIV_USER,	 	GET_SENSOR_HYSTERISIS,		sizeof (GetSensorHysterisisReq_T),	0xAAAA ,0xFFFF},
    { CMD_SET_SENSOR_THRESHOLDS,		PRIV_OPERATOR, 	SET_SENSOR_THRESHOLDS,		sizeof (SetSensorThresholdReq_T),	0xAAAA ,0xFFFF},
    { CMD_GET_SENSOR_THRESHOLDS,		PRIV_USER,	 	GET_SENSOR_THRESHOLDS,		sizeof (GetSensorThresholdReq_T),	0xAAAA ,0xFFFF},
    { CMD_SET_SENSOR_EVENT_ENABLE,		PRIV_OPERATOR, 	SET_SENSOR_EVENT_ENABLE,	0xFF,	0xAAAA ,0xFFFF},
    { CMD_GET_SENSOR_EVENT_ENABLE,		PRIV_USER,	 	GET_SENSOR_EVENT_ENABLE,	sizeof (GetSensorEventEnableReq_T),	0xAAAA ,0xFFFF},
    { CMD_REARM_SENSOR_EVENTS,			PRIV_OPERATOR,	REARM_SENSOR_EVENTS,		0xFF,	0xAAAA ,0xFFFF},
    { CMD_GET_SENSOR_EVENT_STATUS,		PRIV_USER,		GET_SENSOR_EVENT_STATUS,	sizeof (GetSensorEventStatusReq_T),	0xAAAA ,0xFFFF},
    { CMD_GET_SENSOR_READING,			PRIV_USER,	 	GET_SENSOR_READING,			sizeof (GetSensorReadingReq_T),	0xAAAA ,0xFFFF},
    { CMD_SET_SENSOR_TYPE,				PRIV_OPERATOR,	SET_SENSOR_TYPE,			sizeof (SetSensorTypeReq_T),	0xAAA8 ,0xFFFF},
    { CMD_GET_SENSOR_TYPE,				PRIV_USER,	 	GET_SENSOR_TYPE,			sizeof (GetSensorTypeReq_T),	0xAAA8 ,0xFFFF},
    { CMD_SET_SENSOR_READING, 			PRIV_USER,		SET_SENSOR_READING,         0xFF,   0xAAAA ,0xFFFF},

#endif /* SENSOR_DEVICE */

    { 0x00, 					0x00, 			0x00,				  0x00,	0x0000  , 0x0000}
};

