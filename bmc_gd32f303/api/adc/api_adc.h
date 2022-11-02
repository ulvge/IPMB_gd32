#ifndef __API_ADC_H
#define	__API_ADC_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include "bsp_adc.h"
#include "api_sensor.h"

/* para definitions */

#define 		VOL_3_3V                          ADC_CHANNEL_1
#define 		VOL_5V                            ADC_CHANNEL_8
#define 		VOL_12V                           ADC_CHANNEL_9

#define     VREFVOL                           3.3
#define 		ADC_BIT                           4096    //12 bit adc

void adc_init(const Sensor_Handler *pSensor_Handler);
void adc_sample_all(void);
float adc_sampleVal2Temp1(uint16 adcValue);
float adc_sampleVal2Temp2(uint16 adcValue);

uint8_t adc_getChannelSize(void); 
BOOLEAN adc_getValByIndex(uint8_t idx, const ADCChannlesConfig **channlCfg, uint16_t *adcVal);
BOOLEAN adc_getRawValBySensorNum(uint8_t sensorNum, uint16_t *rawAdc);
#ifdef __cplusplus
}
#endif

#endif /* __API_ADC_H */

