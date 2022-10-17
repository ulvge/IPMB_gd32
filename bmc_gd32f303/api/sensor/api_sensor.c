/*!
    \file    api_sensor.c
    \brief   api sensor

    \version 
*/

#include "libipmi.h"
#include "OSPort.h"
#include "math.h"
#include "sensor/api_sensor.h"
#include "IPMI_SDRRecord.h"
#include "sdr.h"
#include "adc/api_adc.h"
#include "fan/api_fan.h"

bool api_sensorGetUnitType(UINT32 sensorNum, uint8_t *unitType)
{
	int BMCInst = 0;
    FullSensorRec_T *pSdr = ReadSensorRecBySensorNum(sensorNum, BMCInst);   
	if (pSdr == NULL){
		return false;
	}
    *unitType = pSdr->Units2;
    return true;
}

BOOLEAN api_sensorGetValBySensorNum(UINT16 sensorNum, UINT16 *val)
{
    uint8_t *unitType;
    if (api_sensorGetUnitType(sensorNum, unitType) == false){
        return false;
    }                   
	uint16_t *fanRpm;
	
	switch (*unitType)
	{
        case IPMI_UNIT_RPM:     
            if (fan_get_rotate_rpm(sensorNum, val)) {
                return true;
            }
            break;
        case IPMI_UNIT_VOLTS:
        case IPMI_UNIT_AMPS: 
        case IPMI_UNIT_DEGREES_C:
            if (adc_getVal(sensorNum, val)) {
                return true;
            }
            break;
	default:    
        return true;
	}       
	return false;
}   











