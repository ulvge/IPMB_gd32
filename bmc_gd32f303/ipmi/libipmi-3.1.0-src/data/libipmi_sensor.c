/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2008, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
* 
* Filename: libipmi_sensor.c
*
******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dbgout.h"

#include "libipmi_session.h"
#include "libipmi_errorcodes.h"
#include "libipmi_sensor.h"
#include "libipmi_AppDevice.h"
#include "libipmi_AMIOEM.h"

#include "IPMI_SensorEvent.h"

#include "sensor_helpers.h"

/* 35.2 Get Device SDR Info Command */
uint16	IPMICMD_GetSDRInfo( IPMI20_SESSION_T *pSession,
							uint8	*pReqGetSDRInfo,
							GetSDRInfoRes_T *pResGetSDRInfo,
							int timeout)
{
	uint16	wRet;
	uint32	dwResLen;

	// dwResLen = MAX_RESPONSE_SIZE;
	// wRet = LIBIPMI_Send_RAW_IPMI2_0_Command(pSession, PAYLOAD_TYPE_IPMI,
	// 										NETFNLUN_IPMI_SENSOR, CMD_GET_DEV_SDR_INFO,
	// 										(uint8*)pReqGetSDRInfo, sizeof(uint8),
	// 										(uint8 *)pResGetSDRInfo, &dwResLen,
	// 										timeout);

	return wRet;
}

/* 35.3 Get Device SDR Command */
LIBIPMI_API uint16	IPMICMD_GetDevSDR( IPMI20_SESSION_T *pSession,
										GetDevSDRReq_T	*pReqDevSDR,
										GetDevSDRRes_T	*pResDevSDR,
										uint32			*pOutBuffLen,
										int timeout)
{


	return wRet;
}

/* 35.4 Reserve Device SDR Repository Command */
LIBIPMI_API uint16	IPMICMD_ReserveDevSDR( IPMI20_SESSION_T *pSession,
										ReserveDevSDRRes_T *pResReserveDevSDR,
										int timeout)
{


	return wRet;
}

/* 35.5 Get Sensor Reading Factors Command */
LIBIPMI_API uint16	IPMICMD_GetSensorReadingFactor( IPMI20_SESSION_T *pSession,
										GetSensorReadingFactorReq_T	*pReqGetSensorReadingFactor,
										GetSensorReadingFactorRes_T *pResGetSensorReadingFactor,
										int timeout)
{


	return wRet;
}

/* 35.6 Set Sensor Hysteresis Command */
LIBIPMI_API uint16	IPMICMD_SetSensorHysterisis( IPMI20_SESSION_T *pSession,
										SetSensorHysterisisReq_T *pReqSetSensorHysterisis,
										SetSensorHysterisisRes_T *pResSetSensorHysterisis,
										int timeout)
{


	return wRet;
}

/* 35.7 Get Sensor Hysteresis Command */
LIBIPMI_API uint16	IPMICMD_GetSensorHysterisis( IPMI20_SESSION_T *pSession,
										GetSensorHysterisisReq_T *pReqGetSensorHysterisis,
										GetSensorHysterisisRes_T *pResGetSensorHysterisis,
										int timeout)
{


	return wRet;
}

/* 35.8 Set Sensor Thresholds Command */
LIBIPMI_API uint16	IPMICMD_SetSensorThreshold( IPMI20_SESSION_T *pSession,
										SetSensorThresholdReq_T *pReqSetSensorThreshold,
										SetSensorThresholdRes_T *pResSetSensorThreshold,int OwnerLUN,
										int timeout)
{


	return wRet;
}

/* 35.9 Get Sensor Thresholds Command */
LIBIPMI_API uint16	IPMICMD_GetSensorThreshold( IPMI20_SESSION_T *pSession,
										GetSensorThresholdReq_T *pReqGetSensorThreshold,
										GetSensorThresholdRes_T *pResGetSensorThreshold,
										int timeout)
{


	return wRet;
}

/* 35.10 Set Sensor Event Enable Command */
LIBIPMI_API uint16	IPMICMD_SetSensorEventEnable( IPMI20_SESSION_T *pSession,
										SetSensorEventEnableReq_T *pReqSetSensorEventEnable,
										SetSensorEventEnableRes_T *pResSetSensorEventEnable,
										int timeout)
{
	return wRet;
}

/* 35.11 Get Sensor Event Enable Command */
LIBIPMI_API uint16	IPMICMD_GetSensorEventEnable( IPMI20_SESSION_T *pSession,
										GetSensorEventEnableReq_T *pReqGetSensorEventEnable,
										GetSensorEventEnableRes_T *pResGetSensorEventEnable,
										int timeout)
{

	return wRet;
}


/* 35.12 ReArm Sensor Events Command */
LIBIPMI_API uint16	IPMICMD_ReArmSensorEvents( IPMI20_SESSION_T *pSession,
										ReArmSensorReq_T *pReArmSensorReq,
										ReArmSensorRes_T *pReArmSensorRes,
										int timeout)
{

	return wRet;
}


/* 35.13 Get Sensor Event Status Command */
LIBIPMI_API uint16	IPMICMD_GetSensorEventStatus( IPMI20_SESSION_T *pSession,
										GetSensorEventStatusReq_T *pReqGetSensorEventStatus,
										GetSensorEventStatusRes_T *pResGetSensorEventStatus,
										int timeout)
{

	return wRet;
}

/* 35.14 Get Sensor Reading Command */
LIBIPMI_API uint16	IPMICMD_GetSensorReading( IPMI20_SESSION_T *pSession,
										GetSensorReadingReq_T *pReqGetSensorReading,
										GetSensorReadingRes_T *pResGetSensorReading,
										int timeout)
{

	return wRet;
}

/* 35.15 Set Sensor Type Command */
LIBIPMI_API uint16	IPMICMD_SetSensorType( IPMI20_SESSION_T *pSession,
										SetSensorTypeReq_T *pReqSetSensorType,
										SetSensorTypeRes_T *pResSetSensorType,
										int timeout)
{

	return wRet;
}


/* 35.16 Get Sensor Type Command */
LIBIPMI_API uint16	IPMICMD_GetSensorType( IPMI20_SESSION_T *pSession,
										GetSensorTypeReq_T *pReqGetSensorType,
										GetSensorTypeRes_T *pResGetSensorType,
										int timeout)
{

	return wRet;
}
LIBIPMI_API uint16 IPMICMD_SetSensorReading( IPMI20_SESSION_T *pSession,
                                            SetSensorReadingReq_T *pReqSetSensorReading,
                                            SetSensorReadingRes_T *pResSetSensorReading,
                                            int timeout)
{

    return wRet;
}



/* ------------------ High level functions ------------------ */
#define SDR_FILE_PATH   "/tmp/sdr_data"

LIBIPMI_API uint16
LIBIPMI_HL_ReadSensorFromSDR( IPMI20_SESSION_T *pSession, uint8 *sdr_buffer,
							  uint8 *raw_reading, float *reading,
							  uint8 *discrete, int timeout )
{
    return( LIBIPMI_E_SUCCESS );
}


LIBIPMI_API uint16
LIBIPMI_HL_LoadSensorSDRs( IPMI20_SESSION_T *pSession, uint8 **sdr_buffer, 
                           int *count, int timeout )
{
 
    return( LIBIPMI_E_SUCCESS );   
}


LIBIPMI_API uint16
LIBIPMI_HL_GetSensorCount( IPMI20_SESSION_T *pSession, int *sdr_count, int timeout )
{

    return wRet;
}

/**
 * @fn LIBIPMI_HL_GetAllSensorReadings
 * @brief High Level API to get all the sensor Info
 * @params Session, sensor_list, nNumSensor, timout
 * @return proper completion code on success
 */

LIBIPMI_API uint16
LIBIPMI_HL_GetAllSensorReadings( IPMI20_SESSION_T *pSession,
                                 struct sensor_data *sensor_list, uint32* nNumSensor,int timeout )
{

    return wRet ;
}
