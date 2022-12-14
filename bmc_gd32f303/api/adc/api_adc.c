#include "adc/api_adc.h"
#include "libipmi.h"
#include "OSPort.h"
#include "math.h"
#include "sensor/api_sensor.h"
#include "api_subdevices.h"

/// @brief need to sync with g_sensor_sdr
//  GetSensorReading() call . master to calc the real val by M&R.
// so, salve cant't calc the real val by self

#define BAT_LOW_THRESHOLD 3.1
#define BAT_HIGH_THRESHOLD 3.45
#define BAT_K_THRESHOLD 265/3.3

const static Dev_Handler *g_pADCConfig_Handler = NULL;
static float adc_sampleVal2Temp2(uint16 adcValue);
static void adc_InitADCs(const Dev_Handler *config);
static void adc_InitWDGBattery(void);

void adc_init(const  Dev_Handler *pDev_Handler)
{
    g_pADCConfig_Handler = pDev_Handler;
    adc_InitADCs(g_pADCConfig_Handler);
    //adc_InitWDGBattery();
}

__attribute__((unused)) static void adc_InitWDGBattery(void)
{
    /* enable ADC clock */
    rcu_periph_clock_enable(RCU_ADC0);  //ADC_WDG_GROUP
    /* config ADC clock */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6);
    adc_config(ADC_WDG_GROUP);

    uint16_t low_threshold = BAT_K_THRESHOLD * BAT_LOW_THRESHOLD;
    uint16_t high_threshold = BAT_K_THRESHOLD * BAT_HIGH_THRESHOLD;
    adc_watchdog_threshold_config(ADC_WDG_GROUP, low_threshold, high_threshold);
    adc_watchdog_single_channel_enable(ADC_WDG_GROUP, ADC_CHANNEL_17);
    adc_channel_length_config(ADC_WDG_GROUP, ADC_REGULAR_CHANNEL, 1);

    /* ADC temperature and Vrefint enable */
    adc_tempsensor_vrefint_enable();
    // clear ADC int flag
    adc_interrupt_flag_clear(ADC_WDG_GROUP, ADC_INT_FLAG_WDE | ADC_INT_FLAG_EOC);
    // set ADC irq event
    adc_interrupt_enable(ADC_WDG_GROUP, ADC_INT_WDE | ADC_INT_EOC);    
    // enable ADC irq
    nvic_irq_enable(ADC0_1_IRQn, 1, 0);  
	/* ADC software trigger enable */
    adc_software_trigger_enable(ADC_WDG_GROUP, ADC_REGULAR_CHANNEL);
}
static void adc_InitADCs(const Dev_Handler *config)
{
    const ADCChannlesConfig *p_gpioCfg;
    for (UINT8 i = 0; i < config->adcCfgSize; i++)
    {
        p_gpioCfg = &config->adcCfg[i];
        if (p_gpioCfg->adcPeriph == NULL) {
            continue;
        }
        adc_init_channle(p_gpioCfg);
        config->val[i].rawIPMB = 0;
        config->val[i].errCnt = 0;
        config->val[i].human = 0;
    }
}
uint8_t adc_getChannelSize(void)
{
    if (g_pADCConfig_Handler == NULL) {
        return 0;
    }
    return g_pADCConfig_Handler->adcCfgSize;
}

BOOLEAN adc_getValByIndex(uint8_t idx, const ADCChannlesConfig **channlCfg, uint16_t *adcVal)
{
	if (idx > adc_getChannelSize())
	{
		return false;
	}
	*channlCfg = &g_pADCConfig_Handler->adcCfg[idx];
    *adcVal = g_pADCConfig_Handler->val[idx].rawAdc;
    return true;
}
BOOLEAN adc_getRawValBySensorNum(uint8_t sensorNum, uint16_t *rawAdc)
{
    for (uint8_t j = 0; j < adc_getChannelSize(); j++)
    {
        if (g_pADCConfig_Handler->adcCfg[j].adcChannl == sensorNum) {
            *rawAdc = g_pADCConfig_Handler->val[j].rawAdc;
            return true;
        }
    }
    return false;
}
/// @brief convert methods 1 利用公式
/// @param adcValue 
/// @return 
float adc_sampleVal2Temp1(uint16 adcValue)
{
    #define ZOOM_SCALE 2
    static const float resistanceInSeries = 10000.0; //ntc串联的分压电阻
    static const float ntcBvalue = 3500.0;  //B 值
    static const float ntcR25 = 10000.0; //25度时电阻ֵ
    static const float KelvinsZero = 273.15; //绝对零度
    static const float sysPowerVoltage = VREFVOL * 1000;
    static const float resolution = sysPowerVoltage / ADC_BIT;
    static const float T25 = 298.15; //25 =KelvinsZero+25

    uint16_t ntcVoltage = adcValue * resolution;
    float ntcCurrent = (sysPowerVoltage - ntcVoltage)/ resistanceInSeries; //计算NTC的电流(A)
    float ntcResistance = ntcVoltage / ntcCurrent; //计算当前电阻值
    float temperature = (ntcBvalue * T25) / (T25 * (log(ntcResistance) - log(ntcR25)) + ntcBvalue);
    temperature -= KelvinsZero; //计算最终温度  
	if (temperature > (0xff / ZOOM_SCALE)){
		return temperature = 0xff;
	}
    if (temperature < 0) { // If the temperature is below 0 ° C, set it to above 0 ° C to avoid errors caused by symbols
        temperature = 0.05;
    }
	return temperature * ZOOM_SCALE;
}
/// @brief convert methods 2 取近似值
/// @param adcValue 
/// @return 
__attribute__((unused)) static float adc_sampleVal2Temp2(uint16 adcValue)
{
#define 		SENSOR_V25_VALUE		1.43f
#define 		SENSOR_TEMP25_VALUE     25
#define 		SENSOR_AVG_SLOPE        4.3f    
    float temperate = (float)adcValue * (VREFVOL / ADC_BIT);

    /* get temperate conversion value */
    temperate = (SENSOR_V25_VALUE - temperate) / SENSOR_AVG_SLOPE + SENSOR_TEMP25_VALUE;

    return temperate;
}

__attribute__((unused)) static void StackFlow(void)
{
	int a[3],i;
	
	//while(1);
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk; /* DIV_0_TRP enable */

	for(i=0; i<3; i++)
	{
		a[i]=100/i;
	}       
	LOG_E("adc_test StackFlow = %d\r\n", a[0]);
}
__attribute__((unused)) static void adc_test(void)
{     
    float valHuman = api_sensorGetValHuman(ADC_CHANNEL_8);
    
	LOG_D("adc_test humanVal = %.3f\r\n", valHuman);
    StackFlow();
}
#define ADC_SAMPLE_TIMES 3
#define ADC_SAMPLE_DEALYTIMES 10
/// @brief consume time = ADC_SAMPLE_TIMES * ADC_SAMPLE_DEALYTIMES * num
void adc_sample_all(void)
{
    uint8_t ipmbVal;
    int channleSize = adc_getChannelSize();
    if (channleSize == 0) {
        return;
    }
    uint16_t temp_vals[ADC_SAMPLE_TIMES][10] = {0};

    // sample
    for (UINT32 i = 0; i < ADC_SAMPLE_TIMES; i++)
    {
        for (UINT32 j = 0; j < channleSize; j++)
        {
            if (g_pADCConfig_Handler->adcCfg[j].adcChannl < ADC_CHANNEL_MAX) {
                temp_vals[i][j] = adc_get_value(&g_pADCConfig_Handler->adcCfg[j]);
            }
        }
        delay_ms(ADC_SAMPLE_DEALYTIMES);
    }
    // average
    for (UINT32 j = 0; j < channleSize; j++)
    {
        uint16_t sum = 0;
        uint8_t sensorNum;
        float humanVal;
        SUB_DEVICE_MODE dev = SubDevice_GetMyMode();
        for (UINT32 i = 0; i < ADC_SAMPLE_TIMES; i++)
        {
            sum += temp_vals[i][j];
        }
        g_pADCConfig_Handler->val[j].rawAdc = sum / ADC_SAMPLE_TIMES;
        sensorNum = g_pADCConfig_Handler->adcCfg[j].adcChannl;

        if (api_sensorConvertIPMBValBySensorNum(dev, sensorNum, g_pADCConfig_Handler->val[j].rawAdc, &ipmbVal))
        {
            api_sensorSetValRaw(sensorNum, ipmbVal);
            if (api_sensorConvert2HumanVal(dev, sensorNum, ipmbVal, &humanVal)) {
                api_sensorSetValHuman(sensorNum, humanVal);
            }
        }
    }
    //adc_test();
}      

/*!
    \brief      this function handles ADC0 and ADC1 interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ADC0_1_IRQHandler(void)
{
    if (adc_interrupt_flag_get(ADC_WDG_GROUP, ADC_INT_FLAG_WDE)) {
        adc_interrupt_flag_clear(ADC_WDG_GROUP, ADC_INT_FLAG_WDE);
        //adc_watchdog_disable(ADC_WDG_GROUP);                        
		uint16_t adcValue = adc_regular_data_read(ADC_WDG_GROUP);
		LOG_E("battery is lower, prepare to power off!, adcValue = %#x\r\n", adcValue);
    }

    if (adc_interrupt_flag_get(ADC_WDG_GROUP, ADC_INT_FLAG_EOC)) {
        adc_interrupt_flag_clear(ADC_WDG_GROUP, ADC_INT_FLAG_EOC); 
    }
}


