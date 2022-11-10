
#ifndef __BSP_ADC_H
#define	__BSP_ADC_H

#include "project_select.h"
#include "systick.h"
#include "stdint.h"
#include "main.h" 
#include "Types.h"
#include "api_subdevices.h"
#include "IPMI_SDRRecord.h"

typedef struct {
    uint8_t      			adcChannl;
    INT32U      			adcPeriph;
    rcu_periph_enum			adcPeriphClk;
    INT32U      			gpioPort;
    rcu_periph_enum      	gpioClk;
    INT32U      			Pin;
} ADCChannlesConfig;

void adc_init_channle(const ADCChannlesConfig  *chanCfg);
/*get ADC channel value of average conversion*/
uint16_t adc_get_value(const ADCChannlesConfig *chanCfg);

#endif /* __BSP_ADC_H */




