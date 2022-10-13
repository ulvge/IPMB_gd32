#include "adc/api_adc.h"
#include "libipmi.h"
#include "OSPort.h"
#include "math.h"

/// @brief need to sync with g_sensor_sdr
//  GetSensorReading() call adc_getValByChannel. master to calc the real val by M&R.
// so, salve cant't calc the real val by self
const static ADCChannlesConfig g_adcChannlConfig[] = {
#ifdef GD32F3x
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_0, IPMI_UNIT_VOLTS, "P0V9 VCC"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_1, IPMI_UNIT_VOLTS, "P2V5"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_2, IPMI_UNIT_VOLTS, "VBat"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_5, IPMI_UNIT_DEGREES_C, "workTemp"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_6, IPMI_UNIT_VOLTS, "P12V"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_7, IPMI_UNIT_VOLTS, "P3V3"},
	
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOB, RCU_GPIOB, GPIO_PIN_1, IPMI_UNIT_VOLTS, "P1V8"},  
	
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, IPMI_UNIT_VOLTS, "P0V75 Vcore"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_1, IPMI_UNIT_VOLTS, "VTT"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_3, IPMI_UNIT_VOLTS, "P1V2 VDDQ"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_4, IPMI_UNIT_DEGREES_C, "CPUTemp"}
#else
    {ADC_CHANNEL_TEMP_X100, ADC0, RCU_ADC0, GPIOB, RCU_GPIOB, GPIO_PIN_0, IPMI_UNIT_DEGREES_C, "X100 temp"},
    {ADC_CHANNEL_P1V8,      ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_0, IPMI_UNIT_VOLTS, "P1V8 VCC"},
    {ADC_CHANNEL_P12V,      ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, IPMI_UNIT_VOLTS, "P12V standby"},
#endif
};      

#define ADC_CHANNLE_CONFIG_NUM (sizeof(g_adcChannlConfig) / sizeof(g_adcChannlConfig[0]))

static uint16_t g_adcVals[ADC_CHANNLE_CONFIG_NUM] = {0};

static void adc_test(void);
static float adc_sampleVal2Temp2(uint16 adcValue);

void sample_init(void)
{
    adc_init(g_adcChannlConfig, sizeof(g_adcChannlConfig) / sizeof(g_adcChannlConfig[0]));
}
uint8_t adc_getChannelNum(void)
{
    return ADC_CHANNLE_CONFIG_NUM;
}


/*get temprate value */
float get_temprate_convers_value(uint16_t channel)
{
    ADCChannlesRes res;
    if (adc_getValByChannel(channel, &res) == false){
        return 0;
    }

    return adc_sampleVal2Temp2(res.adcVal);
}

/* get vref voltage value*/
float get_vref_voltage_convers_value(uint16_t channel)
{
    ADCChannlesRes res;
    uint16_t adcx;
    float convers_value;
    if (adc_getValByChannel(channel, &res) == false){
        return 0;
    }

    adcx = res.adcVal;
    convers_value = (float)adcx * (VREFVOL / ADC_BIT);
    return convers_value;
}

/* get  voltage value*/
float get_voltage_convers_value(uint16_t channel)
{
    ADCChannlesRes res;
    uint16_t adcx;
    float voltage, dc_voltage;

    if (adc_getValByChannel(channel, &res) == false){
        return 0;
    }
    adcx = res.adcVal;
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
/// @brief convert methods 1 利用公式
/// @param adcValue 
/// @return 
static uint16_t adc_sampleVal2Temp1(uint16 adcValue)
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
static uint16_t adc_convertVal(uint8_t sensorUnitType, uint16 raw)
{
    uint16_t res;
    switch (sensorUnitType)
    {
        case IPMI_UNIT_DEGREES_C:
            res = adc_sampleVal2Temp1(raw);
            break;
        case IPMI_UNIT_VOLTS:
        default:
            res = raw;
            break;
    }
	return res;
}
BOOLEAN adc_getValByChannel(uint8_t channel, ADCChannlesRes *val)
{
    for (UINT32 i = 0; i < ADC_CHANNLE_CONFIG_NUM; i++)
    {
        if (g_adcChannlConfig[i].adcChannl == channel)
        {
            val->sensorUnitType = g_adcChannlConfig[i].sensorUnitType;
            val->adcVal = adc_convertVal(val->sensorUnitType, g_adcVals[i]);
            val->alias = g_adcChannlConfig[i].alias;
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
