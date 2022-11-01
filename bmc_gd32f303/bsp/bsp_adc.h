
#ifndef __BSP_ADC_H
#define	__BSP_ADC_H

#include "project_select.h"
#include "systick.h"
#include "stdint.h"
#include "main.h" 
#include "Types.h"
#include "api_subdevices.h"
#include "IPMI_SDRRecord.h"


#ifdef DEBUG

    #define   ADC_MODULE       ADC1
    #define   ADC_MODULE_CLK   RCU_ADC1
    #define   ADC_GPIO_CLK     RCU_GPIOB
    #define   ADC_GPIO_PORT    GPIOB
    #define   ADC_GPIO_PIN     GPIO_PIN_0 | GPIO_PIN_1
	d
    #define   ADC_GPIO2_CLK     RCU_GPIOA
    #define   ADC_GPIO2_PORT    GPIOA
    #define   ADC_GPIO2_PIN     GPIO_PIN_4

    #define   ADC_CHANNEL_NUM   2 
#else
		 
    #define   ADC_MODULE       ADC2
    #define   ADC_MODULE_CLK   RCU_ADC2
    #define   ADC_GPIO_PORT    GPIOF
    #define   ADC_GPIO_CLK     RCU_GPIOF
    #define   ADC_GPIO_PIN     GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10

    #define   ADC_CHANNEL_NUM   8
#endif

typedef struct {
    uint8_t      			adcChannl;
    INT32U      			adcPeriph;
    rcu_periph_enum			adcPeriphClk;
    INT32U      			gpioPort;
    rcu_periph_enum      	gpioClk;
    INT32U      			Pin;
    SUB_DEVICE_SDR_IDX      sdrIdx;
    char *      			adcAlias;
} ADCChannlesConfig;

typedef struct {          
	SUB_DEVICE_MODE mode;
    SubDevice_Reading_T *val;

    uint8_t cfgSize;
    const ADCChannlesConfig *cfg;
    
} ADCChannlesConfig_Handler;


extern const ADCChannlesConfig_Handler g_adcChannlHandler_main;
extern const ADCChannlesConfig_Handler g_adcChannlHandler_net;
extern const ADCChannlesConfig_Handler g_adcChannlHandler_switch;
extern const ADCChannlesConfig_Handler g_adcChannlHandler_power;
extern const ADCChannlesConfig_Handler g_adcChannlHandler_storage0;

void adc_init_channle(const ADCChannlesConfig  *chanCfg);
/*get ADC channel value of average conversion*/
uint16_t adc_get_value(const ADCChannlesConfig *chanCfg);

#endif /* __BSP_ADC_H */




