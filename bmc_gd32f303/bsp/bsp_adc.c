#include "bsp_adc.h"
#include "OSPort.h"

static void rcu_config(const ADCChannlesConfig *cfg);
static void adc_config(uint32_t adc_periph);

void adc_init(const ADCChannlesConfig  *adcChannlConfig, UINT8 num)
{
	const ADCChannlesConfig  *chanCfg;
	for (UINT8 i=0; i< num;i++){
		chanCfg = &adcChannlConfig[i];
		/*configure system clocks*/	
		rcu_config(chanCfg);
        /* config the GPIO as analog mode */
        gpio_init(chanCfg->gpioPort, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, chanCfg->Pin); 
        /* ADC configuration */
        adc_config(chanCfg->adcPeriph);
	}
}


/*!
    \brief      configure the different system clocks
    \param[in]  rcu_gpio_periph
		\param[in]  rcu_adc_periph
    \param[out] none
    \retval     none
*/
static void rcu_config(const ADCChannlesConfig *chanCfg)
{
    /* enable ADC GPIO clock */
    rcu_periph_clock_enable(chanCfg->gpioClk);
    /* enable ADC clock */
    rcu_periph_clock_enable(chanCfg->adcPeriphClk);
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
    // adc_tempsensor_vrefint_enable();
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

uint16_t adc_get_value(const ADCChannlesConfig *chanCfg)
{
    if (chanCfg->adcChannl > ADC_CHANNEL_17) {
        return 0;
    }

    adc_regular_channel_config(chanCfg->adcPeriph, 0, chanCfg->adcChannl, ADC_SAMPLETIME_239POINT5);

	/* ADC software trigger enable */
    adc_software_trigger_enable(chanCfg->adcPeriph, ADC_REGULAR_CHANNEL);
    adc_flag_clear(chanCfg->adcPeriph, ADC_FLAG_EOC);
    while(SET != adc_flag_get(chanCfg->adcPeriph, ADC_FLAG_EOC)){
    }

    /*read ADC regular group data register */
    return adc_regular_data_read(chanCfg->adcPeriph);
}
