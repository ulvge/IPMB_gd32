#ifndef __API_SENSOR_H
#define	__API_SENSOR_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include "Types.h"
#include "bsp_adc.h"
#include "project_select.h"


typedef enum 
{          
    ADC_CHANNEL_MAX = ADC_CHANNEL_17 + 1,
	
    FAN_CHANNEL_1,
    FAN_CHANNEL_2,
    FAN_CHANNEL_MAX,
    SENSOR_CHANNEL_MAX	= 0xff,
}SENSOR_ENUM;

bool api_sensorGetUnitType(INT8U destMode, UINT32 sensorNum, UINT8 *unitType);
BOOLEAN api_sensorGetValBySensorNum(INT8U destMode, UINT16 sensorNum, float *val);

#ifdef __cplusplus
}
#endif

#endif /* __API_SENSOR_H */

