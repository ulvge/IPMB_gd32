#include "adc/api_adc.h"
#include "OSPort.h"

const static ADCChannlesConfig g_adcChannlConfig[] = {
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
    {ADC_CHANNEL_0, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_0, "P1V8 VCC"},
    {ADC_CHANNEL_8, ADC0, RCU_ADC0, GPIOB, RCU_GPIOB, GPIO_PIN_0, "X100 temp"},
    {ADC_CHANNEL_10, ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, "P12V standby"},
#endif
};      

#define ADC_CHANNLE_CONFIG_NUM (sizeof(g_adcChannlConfig) / sizeof(g_adcChannlConfig[0]))

static uint16_t g_adcVals[ADC_CHANNLE_CONFIG_NUM] = {0};

static void adc_test(void);
	
void sample_init(void)
{
    adc_init(g_adcChannlConfig, sizeof(g_adcChannlConfig) / sizeof(g_adcChannlConfig[0]));
}
uint8_t adc_getChannelNum(void)
{
    return ADC_CHANNLE_CONFIG_NUM;
}    

uint16_t adc_getValByChannel(uint8_t channel)
{
    for (UINT32 i = 0; i < ADC_CHANNLE_CONFIG_NUM; i++)
    {
        if (g_adcChannlConfig[i].adcChannl == channel)
        {
            return g_adcVals[i];
        }
    }
    return 0;
}    
BOOLEAN adc_getValByIndex(uint8_t idx, const ADCChannlesConfig **channlCfg, uint16_t *adcVal)
{
	if (idx > ADC_CHANNLE_CONFIG_NUM)
	{
		return false;
	}
	*channlCfg = &g_adcChannlConfig[idx];
	//const ADCChannlesConfig *channlCfg2 = &g_adcChannlConfig[idx];
	*adcVal = g_adcVals[idx];	
	return true;
}

/*get temprate value */
float get_temprate_convers_value(uint16_t channel)
{
    uint16_t adcx;
    float temperate;
    adcx = adc_getValByChannel(channel);

    temperate = (float)adcx * (VREFVOL / ADC_BIT);

    /* get temperate conversion value */
    temperate = (SENSOR_V25_VALUE - temperate) / SENSOR_AVG_SLOPE + SENSOR_TEMP25_VALUE;

    return temperate;
}

/* get vref voltage value*/
float get_vref_voltage_convers_value(uint16_t channel)
{
    uint16_t adcx;
    float convers_value;
    adcx = adc_getValByChannel(channel);

    convers_value = (float)adcx * (VREFVOL / ADC_BIT);
    return convers_value;
}

/* get  voltage value*/
float get_voltage_convers_value(uint16_t channel)
{
    uint16_t adcx;
    float voltage, dc_voltage;

    adcx = adc_getValByChannel(channel);
    voltage = (float)adcx * (VREFVOL / ADC_BIT);

    if (channel == VOL_3_3V)
    {
        dc_voltage = ((PARTIAL_PRESSURE_CONFF1 + TWO_PARTIAL_PRESSURE_CONFF) * voltage) / TWO_PARTIAL_PRESSURE_CONFF;
    }
    else if (channel == VOL_5V)
    {
        dc_voltage = (PARTIAL_PRESSURE_CONFF2 + ONE_PARTIAL_PRESSURE_CONFF1) * voltage;
    }
    else if (channel == VOL_12V)
    {
        dc_voltage = (PARTIAL_PRESSURE_CONFF2 + ONE_PARTIAL_PRESSURE_CONFF1) * voltage;
    }
    else
        dc_voltage = voltage;

    return dc_voltage;
}

bool get_raw_adc_data_value(uint16_t channel, uint16_t *value)
{
    *value = adc_getValByChannel(channel);
    return true;
}
#define ADC_SAMPLE_TIMES 3
#define ADC_SAMPLE_DEALYTIMES 10
/// @brief consume time = ADC_SAMPLE_TIMES * ADC_SAMPLE_DEALYTIMES * num
void adc_sample_all(void)
{
    const ADCChannlesConfig *chanCfg;
    uint16_t temp_vals[ADC_SAMPLE_TIMES][ADC_CHANNLE_CONFIG_NUM] = {0};

    // sample
    for (UINT32 i = 0; i < ADC_SAMPLE_TIMES; i++)
    {
        for (UINT32 j = 0; j < ADC_CHANNLE_CONFIG_NUM; j++)
        {
            chanCfg = &g_adcChannlConfig[j];
            temp_vals[i][j] = adc_get_value(chanCfg);
        }
        delay_ms(ADC_SAMPLE_DEALYTIMES);
    }
    // average
    for (UINT32 j = 0; j < ADC_CHANNLE_CONFIG_NUM; j++)
    {
        uint16_t sum = 0;
        for (UINT32 i = 0; i < ADC_SAMPLE_TIMES; i++)
        {
            sum += temp_vals[i][j];
        }
        g_adcVals[j] = sum / ADC_SAMPLE_TIMES;
    }
    adc_test();
}
static void adc_test(void)
{
    get_temprate_convers_value(ADC_CHANNEL_8);
    //StackFlow();
}
