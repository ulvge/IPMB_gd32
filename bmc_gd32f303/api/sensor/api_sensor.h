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
    ADC_CHANNEL_P1V8 = ADC_CHANNEL_0,
    ADC_CHANNEL_TEMP_X100 = ADC_CHANNEL_8,
    ADC_CHANNEL_P12V = ADC_CHANNEL_10,
    ADC_CHANNEL_PTEST1 = ADC_CHANNEL_1,
    ADC_CHANNEL_PTEST2 = ADC_CHANNEL_2,
    ADC_CHANNEL_PTEST3 = ADC_CHANNEL_3,
    ADC_CHANNEL_TEMP1 = ADC_CHANNEL_4,
    ADC_CHANNEL_TEMP2 = ADC_CHANNEL_5,
    ADC_CHANNEL_TEMP3 = ADC_CHANNEL_6,
    ADC_CHANNEL_MAX = ADC_CHANNEL_17,
	
    FAN_CHANNEL_1,    //ADC_CHANNEL[0~17]   FAN_CHANNEL[18...]
    FAN_CHANNEL_2,   
    FAN_CHANNEL_MAX,
}SENSOR_ENUM;

bool api_sensorGetUnitType(UINT32 sensorNum, UINT8 *unitType);
BOOLEAN api_sensorGetValBySensorNum(UINT16 sensorNum, UINT16 *val);

#ifdef __cplusplus
}
#endif

#endif /* __API_SENSOR_H */

