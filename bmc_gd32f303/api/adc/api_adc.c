#include "adc/api_adc.h"
#include "libipmi.h"
#include "OSPort.h"
#include "math.h"
#include "sensor/api_sensor.h"
#include "api_subdevices.h"

/// @brief need to sync with g_sensor_sdr
//  GetSensorReading() call . master to calc the real val by M&R.
// so, salve cant't calc the real val by self

const static Sensor_Handler *g_pADCConfig_Handler = NULL;

static float adc_sampleVal2Temp2(uint16 adcValue);
static void adc_InitADCs(const Sensor_Handler *config);

void adc_init(const  Sensor_Handler *pSensor_Handler)
{
    g_pADCConfig_Handler = pSensor_Handler;
    adc_InitADCs(g_pADCConfig_Handler);
}
static void adc_InitADCs(const Sensor_Handler *config)
{
    const ADCChannlesConfig *p_gpioCfg;
    for (UINT8 i = 0; i < config->adcCfgSize; i++)
    {
        p_gpioCfg = &config->adcCfg[i];
        if (p_gpioCfg->adcPeriph == NULL) {
            continue;
        }
        adc_init_channle(p_gpioCfg);
        config->val[i].raw = 0;
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
    if (temperature < 0) { // If the temperature is below 0 ° C, set it to above 0 ° C to avoid errors caused by symbols
        temperature = 0.05;
    }
	return temperature * 2;
}
/// @brief convert methods 2 取近似值
/// @param adcValue 
/// @return 
static float adc_sampleVal2Temp2(uint16 adcValue)
{
#define 		SENSOR_V25_VALUE		1.43f
#define 		SENSOR_TEMP25_VALUE     25
#define 		SENSOR_AVG_SLOPE        4.3f    
    float temperate = (float)adcValue * (VREFVOL / ADC_BIT);

    /* get temperate conversion value */
    temperate = (SENSOR_V25_VALUE - temperate) / SENSOR_AVG_SLOPE + SENSOR_TEMP25_VALUE;

    return temperate;
}
static void adc_test(void)
{     
    uint8_t ipmbVal;

    if (api_sensorGetIPMBValBySensorNum(0, ADC_CHANNEL_8, &ipmbVal)) {
		
    }

		   
	//printf("adc_test humanVal = %f\n", adcVal);
    //StackFlow();
}

#define ADC_SAMPLE_TIMES 3
#define ADC_SAMPLE_DEALYTIMES 10
/// @brief consume time = ADC_SAMPLE_TIMES * ADC_SAMPLE_DEALYTIMES * num
void adc_sample_all(void)
{
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
        for (UINT32 i = 0; i < ADC_SAMPLE_TIMES; i++)
        {
            sum += temp_vals[i][j];
        }
        g_pADCConfig_Handler->val[j].rawAdc = sum / ADC_SAMPLE_TIMES;
    }
    adc_test();
}      




