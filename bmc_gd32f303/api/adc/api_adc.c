#include "adc/api_adc.h"
#include "libipmi.h"
#include "OSPort.h"
#include "math.h"
#include "sensor/api_sensor.h"

/// @brief need to sync with g_sensor_sdr
//  GetSensorReading() call adc_getVal. master to calc the real val by M&R.
// so, salve cant't calc the real val by self
const static ADCChannlesConfig g_adcChannlConfig[] = {
#ifdef GD32F3x
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_0, "P0V9 VCC"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_1, "P2V5"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_2, "VBat"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_5, "workTemp"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_6, "P12V"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_7, "P3V3"},
	
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOB, RCU_GPIOB, GPIO_PIN_1,  "P1V8"},  
	
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, "P0V75 Vcore"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_1, "VTT"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_3, "P1V2 VDDQ"},
    {ADC_CHANNEL_P1V8, ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_4, "CPUTemp"}
#else
    {ADC_CHANNEL_TEMP_X100, ADC0, RCU_ADC0, GPIOB, RCU_GPIOB, GPIO_PIN_0, "X100 temp"},
    {ADC_CHANNEL_P1V8,      ADC0, RCU_ADC0, GPIOA, RCU_GPIOA, GPIO_PIN_0, "P1V8 VCC"},
    {ADC_CHANNEL_P12V,      ADC0, RCU_ADC0, GPIOC, RCU_GPIOC, GPIO_PIN_0, "P12V standby"},
#endif
};      

#define ADC_CHANNLE_CONFIG_NUM (sizeof(g_adcChannlConfig) / sizeof(g_adcChannlConfig[0]))

typedef struct 
{
    uint8_t adcIdx;
    uint16_t adcVal;
    const ADCChannlesConfig *config;
} ADCStruct;
static ADCStruct g_ADC[ADC_CHANNLE_CONFIG_NUM] = {0};

static void adc_test(void);
static float adc_sampleVal2Temp2(uint16 adcValue);

void sample_init(void)
{
    for(int32_t i = 0; i < ADC_CHANNLE_CONFIG_NUM; i++)
    {
        ADCStruct *pADC = &g_ADC[i];
        pADC->adcIdx = i;
        pADC->adcVal = 0;
        pADC->config = &g_adcChannlConfig[i];
        adc_init(pADC->config);
    }
}
uint8_t adc_getChannelNum(void)
{
    return ADC_CHANNLE_CONFIG_NUM;
}

/// @brief 
/// @param channel sensorNum
/// @return 
ADCStruct *adc_getADCStructBySensorNum(uint16_t channel)
{
    for (UINT32 i = 0; i < ADC_CHANNLE_CONFIG_NUM; i++)
    {
        if (g_ADC[i].config->adcChannl == channel)
        {
            return &g_ADC[i];
        }
    }
    return NULL;
}
ADCStruct *adc_getADCStructByIdx(uint16_t idx)
{
    if (idx < ADC_CHANNLE_CONFIG_NUM)
    {
        return &g_ADC[idx];
    }
    return NULL;
}
/*get temprate value */
float get_temprate_convers_value(uint16_t channel)
{
    uint16_t adcVal;
    if (adc_getVal(channel, &adcVal) == false){
        return 0;
    }

    return adc_sampleVal2Temp2(adcVal);
}

/* get vref voltage value*/
float get_vref_voltage_convers_value(uint16_t channel)
{
    float convers_value;
    uint16_t adcVal;
    if (adc_getVal(channel, &adcVal) == false){
        return 0;
    }

    convers_value = (float)adcVal * (VREFVOL / ADC_BIT);
    return convers_value;
}

/* get  voltage value*/
float get_voltage_convers_value(uint16_t channel)
{
    float voltage, dc_voltage;

    uint16_t adcVal;
    if (adc_getVal(channel, &adcVal) == false){
        return 0;
    }
    voltage = (float)adcVal * (VREFVOL / ADC_BIT);

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
            ADCStruct * adc = adc_getADCStructBySensorNum(g_ADC[j].config->adcChannl);
            if (adc == NULL) {
                continue;
            }
            chanCfg = adc->config;
            temp_vals[i][j] = adc_get_value(chanCfg);
        }
        delay_ms(ADC_SAMPLE_DEALYTIMES);
    }
    // average
    for (UINT32 j = 0; j < ADC_CHANNLE_CONFIG_NUM; j++)
    {
        ADCStruct * adc = adc_getADCStructBySensorNum(g_ADC[j].config->adcChannl);
        if (adc == NULL) {
            continue;
        }
        uint16_t sum = 0;
        for (UINT32 i = 0; i < ADC_SAMPLE_TIMES; i++)
        {
            sum += temp_vals[i][j];
        }
        adc->adcVal = sum / ADC_SAMPLE_TIMES;
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
    ADCStruct * adc = adc_getADCStructByIdx(idx);
    if (adc == NULL) {
        return false;
    }
	*adcVal = adc->adcVal;
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
static uint16_t adc_convertVal(uint8_t unitType, uint16 raw)
{
    uint16_t res;
    switch (unitType)
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
BOOLEAN adc_getVal(uint8_t channel, uint16_t *humanVal)
{
    ADCStruct * adc = adc_getADCStructBySensorNum(channel);
    if (adc == NULL) {
        return false;
    }
    uint8_t unitType;
    if (api_sensorGetUnitType(channel, &unitType) == false){
        return false;
    }                 
    *humanVal = adc_convertVal(unitType, adc->adcVal);
    return true;
}
static void adc_test(void)
{
    get_temprate_convers_value(ADC_CHANNEL_8);
    //StackFlow();
}
