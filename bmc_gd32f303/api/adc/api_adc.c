#include "adc/api_adc.h"
#include "libipmi.h"
#include "OSPort.h"
#include "math.h"
#include "sensor/api_sensor.h"
#include "api_subdevices.h"

/// @brief need to sync with g_sensor_sdr
//  GetSensorReading() call adc_getVal. master to calc the real val by M&R.
// so, salve cant't calc the real val by self

const static ADCChannlesConfig_Handler *g_pADCConfig_Handler = NULL;
const static ADCChannlesConfig_Handler *g_ADCAllDevices[] = {
    &g_adcChannlHandler_main,
    &g_adcChannlHandler_net,
    &g_adcChannlHandler_switch,
    &g_adcChannlHandler_power,
    &g_adcChannlHandler_storage0,
};

static void adc_test(void);
static float adc_sampleVal2Temp2(uint16 adcValue);
static void adc_InitADCs(const ADCChannlesConfig_Handler *config);

void adc_init(void)
{
    SUB_DEVICE_MODE myMode = SubDevice_GetMyMode();

    for (size_t i = 0; i < ARRARY_SIZE(g_ADCAllDevices); i++)
    {
        const ADCChannlesConfig_Handler **phandler = (g_ADCAllDevices + i);
        if ((*phandler)->mode == myMode)
        {
            g_pADCConfig_Handler = *phandler;
            adc_InitADCs(g_pADCConfig_Handler);
            return;
        }
    }
}
static void adc_InitADCs(const ADCChannlesConfig_Handler *config)
{
    const ADCChannlesConfig *p_gpioCfg;
    for (UINT8 i = 0; i < config->cfgSize; i++)
    {
        p_gpioCfg = &config->cfg[i];
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
    return g_pADCConfig_Handler->cfgSize;
}

/// @brief 
/// @param channel sensorNum
/// @return 
static const ADCChannlesConfig *adc_getADCConfigBySensorNum(uint16_t channel)
{
    uint8_t channleSize = adc_getChannelSize();
    for (UINT32 i = 0; i < channleSize; i++)
    {
        if (g_pADCConfig_Handler->cfg[i].adcChannl == channel)
        {
            return &g_pADCConfig_Handler->cfg[i];
        }
    }
    return NULL;
}

/*get temprate value */
float get_temprate_convers_value(uint16_t channel)
{
    float adcVal;
    if (adc_getVal(channel, &adcVal) == false){
        return 0;
    }

    return adc_sampleVal2Temp2(adcVal);
}

/* get vref voltage value*/
float get_vref_voltage_convers_value(uint16_t channel)
{
    float adcVal;
    if (adc_getVal(channel, &adcVal) == false){
        return 0;
    }

    float convers_value = (float)adcVal * (VREFVOL / ADC_BIT);
    return convers_value;
}

/* get  voltage value*/
float get_voltage_convers_value(uint16_t channel)
{
    float voltage, dc_voltage;

    float adcVal;
    if (adc_getVal(channel, &adcVal) == false){
        return 0;
    }
    voltage = adcVal * (VREFVOL / ADC_BIT);

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

#define ADC_SAMPLE_TIMES 3
#define ADC_SAMPLE_DEALYTIMES 10
/// @brief consume time = ADC_SAMPLE_TIMES * ADC_SAMPLE_DEALYTIMES * num
void adc_sample_all(void)
{
    const ADCChannlesConfig *chanCfg;
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
            temp_vals[i][j] = adc_get_value(&g_pADCConfig_Handler->cfg[j]);
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

BOOLEAN adc_getValByIndex(uint8_t idx, const ADCChannlesConfig **channlCfg, uint16_t *adcVal)
{
	if (idx > adc_getChannelSize())
	{
		return false;
	}
    *adcVal = g_pADCConfig_Handler->val[idx].rawAdc;
    return true;
}
/// @brief convert methods 1 利用公式
/// @param adcValue 
/// @return 
static float adc_sampleVal2Temp1(uint16 adcValue)
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
    float temperate = (float)adcValue * (VREFVOL / ADC_BIT);

    /* get temperate conversion value */
    temperate = (SENSOR_V25_VALUE - temperate) / SENSOR_AVG_SLOPE + SENSOR_TEMP25_VALUE;

    return temperate;
}
static float adc_convertVal(uint8_t unitType, uint16 rawAdc)
{
    float res;
    switch (unitType)
    {
        case IPMI_UNIT_DEGREES_C:
            res = adc_sampleVal2Temp1(rawAdc);
            break;
        case IPMI_UNIT_VOLTS:
        default:
            res = (float)rawAdc;
            break;
    }
	return res;
}
BOOLEAN adc_getVal(uint8_t channel, float *humanVal)
{
    uint8_t unitType;
    if (api_sensorGetUnitType(SubDevice_GetMyMode(), channel, &unitType) == false)
    {
        return false;
    }
    uint8_t channleSize = adc_getChannelSize();
    for (uint8_t i = 0; i < channleSize; i++)
    {
        if (g_pADCConfig_Handler->cfg[i].adcChannl == channel)
        {
            *humanVal = adc_convertVal(unitType, g_pADCConfig_Handler->val[i].rawAdc);
            return true;
        }
    }

    return false;
}
static void adc_test(void)
{
    get_temprate_convers_value(ADC_CHANNEL_8);
    //StackFlow();
}
