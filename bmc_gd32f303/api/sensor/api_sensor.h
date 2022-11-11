#ifndef __API_SENSOR_H
#define	__API_SENSOR_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include "Types.h"
#include "bsp_adc.h"
#include "project_select.h"
#include "debug_print.h"
#include "FreeRTOS.h"
#include "task.h"       

typedef enum 
{          
    ADC_CHANNEL_MAX = ADC_CHANNEL_17 + 1,
	
    FAN_CHANNEL_1,
    FAN_CHANNEL_2,
    FAN_CHANNEL_MAX,
    SENSOR_CHANNEL_MAX	= 0xff,
}SENSOR_ENUM;


typedef struct {
    uint8_t      			sensorNum;
    SUB_DEVICE_SDR_IDX      sdrIdx;
    char *      			sensorAlias;
} SensorConfig;

typedef struct {          
	SUB_DEVICE_MODE mode;
    SubDevice_Reading_T *val;

    uint8_t adcCfgSize;
    const ADCChannlesConfig *adcCfg;

    uint8_t sensorCfgSize;
    const SensorConfig *sensorCfg;
    void (*TaskHandler)(void *arg); 
} Dev_Handler;

bool api_sensorGetUnitType(INT8U destMode, UINT32 sensorNum, UINT8 *unitType);
void Dev_Task(void *pvParameters);
uint8_t api_sensorGetSensorCount(void);
uint8_t api_sensorGetMySensorNumByIdex(uint8_t idx);
uint8_t api_sensorGetSensorNumByIdex(SUB_DEVICE_MODE dev, uint8_t idx);
BOOLEAN api_sensorConvertIPMBValBySensorNum(INT8U destMode, UINT16 sensorNum, UINT16 rawAdc, INT8U *ipmbVal);
BOOLEAN api_sensorConvert2HumanVal(SUB_DEVICE_MODE dev, uint8_t sensorNum, uint8_t ipmiVal, float *humanVal);
void api_sensorSetValRaw(uint8_t sensorNum, uint8_t ipmbVal);
const Dev_Handler *api_getDevHandler(SUB_DEVICE_MODE destMode);
uint8_t api_sensorGetIPMBVal(UINT16 sensorNum);

#ifdef __cplusplus
}
#endif

#endif /* __API_SENSOR_H */

