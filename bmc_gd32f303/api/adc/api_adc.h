#ifndef __API_ADC_H
#define	__API_ADC_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include "bsp_adc.h"

/* para definitions */

#define 		VOL_3_3V                          ADC_CHANNEL_1
#define 		VOL_5V                            ADC_CHANNEL_8
#define 		VOL_12V                           ADC_CHANNEL_9

#define     VREFVOL                           3.3
#define 		ADC_BIT                           4096    //12 bit adc


void adc_init(void);
ADCChannlesConfig *adc_getConfig(void);   
uint8_t adc_getChannelSize(void); 
BOOLEAN adc_getValByIndex(uint8_t idx, const ADCChannlesConfig **channlCfg, uint16_t *adcVal);  
const ADCChannlesConfig_Handler *adc_getADCConfigHandler(SUB_DEVICE_MODE destMode);
uint8_t adc_getSensorNumByIdex(uint8_t idx);

/* get voltage value*/
float get_voltage_convers_value(uint16_t channel);

/* get vref voltage value*/
BOOLEAN adc_getVal(uint8_t channel, float *humanVal);
void adc_sample_all(void);
#ifdef __cplusplus
}
#endif

#endif /* __API_ADC_H */

