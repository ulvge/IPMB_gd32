#include "bsp_adc.h"
#include "OSPort.h"

static void rcu_config(const ADCChannles *chan);
static void adc_config(uint32_t adc_periph);

const static ADCChannles  g_adcChannl[] = {
	#if 0
    {ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_0, "P0V9 VCC"},
    {ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_1, "P2V5"},
    {ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_2, "VBat"}, 
    {ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_5, "workTemp"},
    {ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_6, "P12V"},
    {ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_7, "P3V3"},
	
    {ADC0, RCU_ADC0, GPIOB, RCU_GPIOB, GPIO_PIN_1, "P1V8"},  
	
    {ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, "P0V75 Vcore"},
    {ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_1, "VTT"},
    {ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_3, "P1V2 VDDQ"},
    {ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_4, "CPUTemp"}
#else
    {ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_0, "P1V8 VCC"},
    {ADC0, RCU_ADC0, GPIOB, RCU_GPIOB, GPIO_PIN_0, "X100 temp"}, 
    {ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, "P12V standby"},
#endif
};
void adc_init(void)
{
	const ADCChannles  *chan;
	for (UINT8 i=0; i< sizeof(g_adcChannl)/sizeof(g_adcChannl[0]);i++){
		chan = &g_adcChannl[i];
		/*configure system clocks*/	
		rcu_config(chan);
        /* config the GPIO as analog mode */
        gpio_init(chan->gpioPort, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, chan->Pin); 
        /* ADC configuration */
        adc_config(chan->adcPeriph);
	}
}


/*!
    \brief      configure the different system clocks
    \param[in]  rcu_gpio_periph
		\param[in]  rcu_adc_periph
    \param[out] none
    \retval     none
*/
static void rcu_config(const ADCChannles *chan)
{
    /* enable ADC GPIO clock */
    rcu_periph_clock_enable(chan->gpioClk);
    /* enable ADC clock */
    rcu_periph_clock_enable(chan->adcPeriphClk);
	/* enable AF clock */
    rcu_periph_clock_enable(RCU_AF);
    /* config ADC clock */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6);
}

/*!
    \brief      configure the ADC peripheral
    \param[in]  adc_periph: ADCx, x=0,1,2
    \param[out] none
    \retval     none
*/
static void adc_config(uint32_t adc_periph)
{
    /* reset ADC */
    adc_deinit(adc_periph);
    /* ADC mode config */
    adc_mode_config(ADC_MODE_FREE);
    /* ADC data alignment config */
    adc_data_alignment_config(adc_periph, ADC_DATAALIGN_RIGHT);
    /* ADC continous function enable */
    // adc_special_function_config(adc_periph, ADC_CONTINUOUS_MODE, DISABLE);  
    // adc_special_function_config(adc_periph, ADC_SCAN_MODE, ENABLE); 
    /* ADC temperature and Vrefint enable */
    //adc_tempsensor_vrefint_enable();
    /* ADC channel length config */
    adc_channel_length_config(adc_periph, ADC_REGULAR_CHANNEL, 1);

    /* ADC trigger config */
    adc_external_trigger_source_config(adc_periph, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(adc_periph, ADC_REGULAR_CHANNEL, ENABLE);

    /* ADC discontinuous mode */
    adc_discontinuous_mode_config(adc_periph, ADC_REGULAR_CHANNEL, 1);
    
    /* enable ADC interface */
    adc_enable(adc_periph);
    delay_ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(adc_periph);
}

/*!
    \brief      get ADC channel value of conversion
    \param[in]  adc_channel: the selected ADC channel  0-16
    \param[out] none
    \retval     the conversion value
*/
uint16_t  get_adc_convers_value(uint8_t channel)
{
    switch(channel)
    {
    case 0:
        adc_regular_channel_config(ADC_MODULE, 0, ADC_CHANNEL_8, ADC_SAMPLETIME_239POINT5);
        break;
    case 1:
        adc_regular_channel_config(ADC_MODULE, 0, ADC_CHANNEL_9, ADC_SAMPLETIME_239POINT5);
        break;
    case 2:
        adc_regular_channel_config(ADC_MODULE, 0, ADC_CHANNEL_9, ADC_SAMPLETIME_239POINT5);
        break;
    case 3:
        adc_regular_channel_config(ADC_MODULE, 0, ADC_CHANNEL_14, ADC_SAMPLETIME_239POINT5);
        break;
    case 4:
        adc_regular_channel_config(ADC_MODULE, 0, ADC_CHANNEL_15, ADC_SAMPLETIME_239POINT5);
        break;
    case 5:
        adc_regular_channel_config(ADC_MODULE, 0, ADC_CHANNEL_8, ADC_SAMPLETIME_239POINT5);
        break;
    case 6:
        adc_regular_channel_config(ADC_MODULE, 0, ADC_CHANNEL_6, ADC_SAMPLETIME_239POINT5);
        break;
    default:
        return 0;
    }

	/* ADC software trigger enable */
    adc_software_trigger_enable(ADC_MODULE, ADC_REGULAR_CHANNEL);
    adc_flag_clear(ADC_MODULE, ADC_FLAG_EOC);
    while(SET != adc_flag_get(ADC_MODULE, ADC_FLAG_EOC)){
    }
        

    /*read ADC regular group data register */
    return adc_regular_data_read(ADC_MODULE);
}

/*!
    \brief      get ADC channel value of average conversion
    \param[in]  adc_channel: the selected ADC channel  0-16
    \param[out] none
    \retval     the  average conversion value
*/
uint16_t  get_adc_average_convers_value(uint8_t channel,uint8_t times)
{
	  uint16_t temp_val=0;
	  uint8_t iter;
	   
	if(!times)
		return 0;
	
    for(iter=0;iter<times;iter++)
    {
        temp_val += get_adc_convers_value(channel);
        delay_ms(5);
    }
  return temp_val/times;

}
