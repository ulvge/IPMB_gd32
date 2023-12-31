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

extern const Dev_Handler g_devHandler_main;
extern const Dev_Handler g_devHandler_switch;
extern const Dev_Handler g_devHandler_power;
extern const Dev_Handler g_devHandler_storage0;
extern const Dev_Handler g_devHandler_storage1;
extern const Dev_Handler g_devHandler_storage2;

const static Dev_Handler *g_pDev_Handler = NULL;
static const Dev_Handler *g_AllDevices[] = {
    &g_devHandler_main,
    &g_devHandler_switch,
    &g_devHandler_power,
    &g_devHandler_storage0,
    &g_devHandler_storage1,
    &g_devHandler_storage2,
};

bool api_sensorGetUnitType(INT8U destMode, UINT32 sensorNum, uint8_t *unitType)
{
    return SensorGetUnitType(destMode, sensorNum, unitType);
}
// get raw 16 val. maybe ADC, ADC->temp, RPM.
// after get this ipmbVal, need convert by MR if want to get human val
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
        // if (fan_get_rotate_rpm(sensorNum, &fanRpm)) {
        *ipmbVal = (INT8U)(fanRpm / 32);
        return true;
        //}
    case IPMI_UNIT_VOLTS:
        *ipmbVal = (INT8U)(((INT16U)rawAdc) >> 4);
        return true;
        // }
    case IPMI_UNIT_DEGREES_C:
        *ipmbVal = (float)adc_sampleVal2Temp1(rawAdc); //转换成真实值 ，并对其进行MR 编码.后续 IPMI 只需要用MR就能解码
        return true;
        //}
    case IPMI_UNIT_AMPS:   //current is read from IC, not from ADC,so this statement is unreachable
    default:
        return false;
    }
}
/// @brief just get
/// @param sensorNum 
/// @return 
uint8_t api_sensorGetValIPMB(UINT16 sensorNum)
{
    uint8_t ipmbVal;
    SUB_DEVICE_MODE dev = SubDevice_GetMyMode();
    const Dev_Handler *pDev_Handler = api_getDevHandler(dev);
    for (uint8_t numIdex = 0; numIdex < api_sensorGetSensorCount(); numIdex++)
    {
        if (pDev_Handler->sensorCfg[numIdex].sensorNum == sensorNum) {
            ipmbVal = pDev_Handler->val[numIdex].rawIPMB;
            break;
        }
    }
    return ipmbVal;
}
/// @brief only SUB_DEVICE_MODE_MAIN can call others
/// @param dev 
/// @param sensorNum 
/// @return 
float api_sensorGetModeValHuman(SUB_DEVICE_MODE dev, UINT16 sensorNum)
{
    SUB_DEVICE_MODE myDev = SubDevice_GetMyMode();
    if ((myDev == dev) || (myDev == SUB_DEVICE_MODE_MAIN)) {
        const Dev_Handler *pDev_Handler = api_getDevHandler(dev);
        if (pDev_Handler == NULL) {
            return 0;
        }
        for (uint8_t numIdex = 0; numIdex < pDev_Handler->sensorCfgSize; numIdex++)
        {
            if (pDev_Handler->sensorCfg[numIdex].sensorNum == sensorNum) {
                return pDev_Handler->val[numIdex].human;
            }
        }
    }
    return 0;
}
float api_sensorGetValHuman(UINT16 sensorNum)
{
    SUB_DEVICE_MODE dev = SubDevice_GetMyMode();
    return api_sensorGetModeValHuman(dev, sensorNum);
}
uint8_t api_sensorGetSensorCount(void)
{
    if (g_pDev_Handler == NULL) {
        return 0;
    }
    return g_pDev_Handler->sensorCfgSize;
}

/// @brief dev 中,sensor 的 adcChannl 就是他的sensorNum
/// @param idx 
/// @return 
uint8_t api_sensorGetMySensorNumByIdex(uint8_t idx)
{
    if (g_pDev_Handler == NULL) {
        return SENSOR_CHANNEL_MAX;
    }
    if (idx >= g_pDev_Handler->sensorCfgSize) {
        return SENSOR_CHANNEL_MAX;
    }
    return g_pDev_Handler->sensorCfg[idx].sensorNum;
}
uint8_t api_sensorGetSensorNumByIdex(SUB_DEVICE_MODE dev, uint8_t idx)
{
    const Dev_Handler *handler = api_getDevHandler(dev);
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
void api_sensorSetIpmbVal(uint8_t sensorNum, uint8_t ipmbVal)
{
    SUB_DEVICE_MODE myMode = SubDevice_GetMyMode();
    const Dev_Handler *handler = api_getDevHandler(myMode);
    if (handler == NULL) {
        return;
    }
    for (size_t i = 0; i < handler->sensorCfgSize; i++)
    {
        if (handler->sensorCfg[i].sensorNum == sensorNum){
            handler->val[i].rawIPMB = ipmbVal;
            return;
        }
    }
}

void api_sensorSetValHuman(uint8_t sensorNum, float humanVal)
{
    SUB_DEVICE_MODE myMode = SubDevice_GetMyMode();
    const Dev_Handler *handler = api_getDevHandler(myMode);
    if (handler == NULL) {
        return;
    }
    for (size_t i = 0; i < handler->sensorCfgSize; i++)
    {
        if (handler->sensorCfg[i].sensorNum == sensorNum){
            handler->val[i].human = humanVal;
            return;
        }
    }
}
const Dev_Handler *api_getDevHandler(SUB_DEVICE_MODE destMode)
{
    for (size_t i = 0; i < ARRARY_SIZE(g_AllDevices); i++)
    {
        const Dev_Handler **phandler = (g_AllDevices + i);
        if ((*phandler)->mode == destMode)
        {
            return *phandler;
        }
    }
    return NULL;
}
void Dev_Task(void *pvParameters)
{
    SUB_DEVICE_MODE myMode = SubDevice_GetMyMode();

    g_pDev_Handler = api_getDevHandler(myMode);
    if (g_pDev_Handler == NULL) {
        vTaskDelete(NULL);
    } else {
        adc_init(g_pDev_Handler);
        if (g_pDev_Handler->TaskHandler == NULL) {
            vTaskDelete(NULL);
        } else {
            g_pDev_Handler->TaskHandler(pvParameters);
        }
    }
}









