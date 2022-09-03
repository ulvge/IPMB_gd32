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
#define 		TIMES                             1
#define 		ADC_BIT                           4096    //12 bit adc
#define 		SENSOR_V25_VALUE									1.43
#define 		SENSOR_TEMP25_VALUE								25
#define 		SENSOR_AVG_SLOPE									4.3    

#define     PARTIAL_PRESSURE_CONFF1           20
#define     PARTIAL_PRESSURE_CONFF2           10
#define     TWO_PARTIAL_PRESSURE_CONFF        2
#define     ONE_PARTIAL_PRESSURE_CONFF1       1


void sample_init(void);

/*get temprate value */
float get_temprate_convers_value(uint16_t channel);

/* get voltage value*/
float get_voltage_convers_value(uint16_t channel);

/* get vref voltage value*/
float get_vref_voltage_convers_value(uint16_t channel);

/*get ADC channel raw value of conversion*/
bool get_raw_adc_data_value(uint16_t channel, uint16_t* value);

#ifdef __cplusplus
}
#endif

#endif /* __API_ADC_H */

