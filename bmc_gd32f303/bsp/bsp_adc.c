#include "bsp_adc.h"
#include "OSPort.h"

static void rcu_config(void);
static void gpio_config(void);
static void adc_config(uint32_t adc_periph);


void adc_init(void)
{
	/*configure system clocks*/	
	rcu_config();
    /* GPIO configuration */
    gpio_config();
	/* ADC configuration */
    adc_config(ADC_MODULE);
}


/*!
    \brief      configure the different system clocks
    \param[in]  rcu_gpio_periph
		\param[in]  rcu_adc_periph
    \param[out] none
    \retval     none
*/
static void rcu_config()
{
    /* enable ADC GPIO clock */
    rcu_periph_clock_enable(ADC_GPIO_CLK);
#ifdef DEBUG
    rcu_periph_clock_enable(ADC_GPIO2_CLK);
#endif
    /* enable ADC clock */
    rcu_periph_clock_enable(ADC_MODULE_CLK);
	/* enable AF clock */
    rcu_periph_clock_enable(RCU_AF);
    /* config ADC clock */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6);
}

/*!
    \brief      configure the GPIO peripheral
    \param[in]  gpio_periph:
    \param[out] none
    \retval     none
*/
static void gpio_config()
{
    /* config the GPIO as analog mode */
    gpio_init(ADC_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, ADC_GPIO_PIN); 
#ifdef DEBUG
    gpio_init(ADC_GPIO2_PORT, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, ADC_GPIO2_PIN); 
#endif
	
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
