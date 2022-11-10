/*!
    \file    api_sensor.c
    \brief   api sensor

    \version 
*/

#include "libipmi.h"
#include "OSPort.h"
#include "math.h"
#include "sensor/api_sensor.h"
#include "IPMI_SDRRecord.h"
#include "sdr.h"
#include "adc/api_adc.h"
#include "fan/api_fan.h"    
#include "sensor_helpers.h"

extern const Sensor_Handler g_sensorHandler_main;
extern const Sensor_Handler g_sensorHandler_net;
extern const Sensor_Handler g_sensorHandler_switch;
extern const Sensor_Handler g_sensorHandler_power;
extern const Sensor_Handler g_sensorHandler_storage0;


const static Sensor_Handler *g_pSensor_Handler = NULL;
static const Sensor_Handler *g_AllSensorDevices[] = {
    &g_sensorHandler_main,
    &g_sensorHandler_net,
    &g_sensorHandler_switch,
    &g_sensorHandler_power,
    &g_sensorHandler_storage0,
};

bool api_sensorGetUnitType(INT8U destMode, UINT32 sensorNum, uint8_t *unitType)
{
    return SensorGetUnitType(destMode, sensorNum, unitType);
}
// get raw 16 val. maybe ADC, ADC->temp, RPM.
// after get this val,need to convert by MR
BOOLEAN api_sensorConvertIPMBValBySensorNum(INT8U destMode, UINT16 sensorNum, UINT16 rawAdc, INT8U *ipmbVal)
{
    uint8_t unitType;
    if (api_sensorGetUnitType(destMode, sensorNum, &unitType) == false){
        return false;
    }
	uint16_t fanRpm = rawAdc;
	
	switch (unitType)
	{
        case IPMI_UNIT_RPM:
            //if (fan_get_rotate_rpm(sensorNum, &fanRpm)) {
                *ipmbVal = (INT8U)(fanRpm / 32);
                return true;
            //}
        case IPMI_UNIT_VOLTS:
            //if (adc_getRawValBySensorNum(sensorNum, &rawAdc)) {
		        *ipmbVal = (INT8U)(((INT16U)rawAdc)>>4);
                return true;
           // }
        case IPMI_UNIT_DEGREES_C:
            //if (adc_getRawValBySensorNum(sensorNum, &rawAdc)) { //获取原始值
                *ipmbVal = (float)adc_sampleVal2Temp1(rawAdc); //转换成真实值 ，并对其进行MR 编码.后续 IPMI 只需要用MR就能解码
                return true;
            //}
        case IPMI_UNIT_AMPS:
	default:
        return false;
	}       
}   
/// @brief just get
/// @param sensorNum 
/// @return 
uint8_t api_sensorGetIPMBVal(UINT16 sensorNum)
{
    uint8_t ipmbVal;
    SUB_DEVICE_MODE dev = SubDevice_GetMyMode();
    const Sensor_Handler *pSensor_Handler = api_getSensorHandler(dev);
    for (uint8_t numIdex = 0; numIdex < api_sensorGetSensorCount(); numIdex++)
    {
        if (pSensor_Handler->sensorCfg[numIdex].sensorNum == sensorNum) {
            ipmbVal = pSensor_Handler->val[numIdex].raw;
            break;
        }
	}
    return ipmbVal;
}
uint8_t api_sensorGetSensorCount(void)
{
    if (g_pSensor_Handler == NULL) {
        return 0;
    }
    return g_pSensor_Handler->sensorCfgSize;
}

/// @brief dev 中,sensor 的 adcChannl 就是他的sensorNum
/// @param idx 
/// @return 
uint8_t api_sensorGetMySensorNumByIdex(uint8_t idx)
{
    if (g_pSensor_Handler == NULL) {
        return SENSOR_CHANNEL_MAX;
    }
    if (idx >= g_pSensor_Handler->sensorCfgSize) {
        return SENSOR_CHANNEL_MAX;
    }
    return g_pSensor_Handler->sensorCfg[idx].sensorNum;
}
uint8_t api_sensorGetSensorNumByIdex(SUB_DEVICE_MODE dev, uint8_t idx)
{
    const Sensor_Handler *handler = api_getSensorHandler(dev);
    if (handler == NULL) {
        return SENSOR_CHANNEL_MAX;
    }
    if (idx >= handler->sensorCfgSize) {
        return SENSOR_CHANNEL_MAX;
    }
    return handler->sensorCfg[idx].sensorNum;
}
BOOLEAN api_sensorConvert2HumanVal(SUB_DEVICE_MODE dev, uint8_t sensorNum, uint8_t ipmiVal, float *humanVal)
{
    FullSensorRec_T *pSdr = ReadSensorRecBySensorNum(dev, sensorNum, 0);
    if (pSdr != NULL) {
        if (ipmi_convert_reading((uint8_t *)pSdr, ipmiVal, humanVal) != -1) {
            return true;
        }
    }
    return false;
}
void api_sensorSetValRaw(uint8_t sensorNum, uint8_t ipmbVal)
{
    SUB_DEVICE_MODE myMode = SubDevice_GetMyMode();
    const Sensor_Handler *handler = api_getSensorHandler(myMode);
    if (handler == NULL) {
        return;
    }
    for (size_t i = 0; i < handler->sensorCfgSize; i++)
    {
        if (handler->sensorCfg[i].sensorNum == sensorNum){
            handler->val[i].raw = ipmbVal;
            return;
        }
    }
}
const Sensor_Handler *api_getSensorHandler(SUB_DEVICE_MODE destMode)
{
    for (size_t i = 0; i < ARRARY_SIZE(g_AllSensorDevices); i++)
    {
        const Sensor_Handler **phandler = (g_AllSensorDevices + i);
        if ((*phandler)->mode == destMode)
        {
            return *phandler;
        }
    }
    return NULL;
}
void sensor_init(void)
{
    SUB_DEVICE_MODE myMode = SubDevice_GetMyMode();

    g_pSensor_Handler = api_getSensorHandler(myMode);
    if (g_pSensor_Handler == NULL)
    {
		return;
    }       
	adc_init(g_pSensor_Handler);
	SubDevice_Init();
}









