#include <string.h>
#include <stdlib.h>
#include "debug_print.h"
#include "api_subdevices.h"
#include "bsp_gpio.h"
#include "bsp_i2c.h"
#include "IPMIConf.h"
#include "IPMDevice.h"
#include "OSPort.h"
#include "IPMI_SensorEvent.h"
#include "ipmi_common.h"
#include "sdr.h"
#include "IPMI_Sensor.h"
#include "sensor_helpers.h"
#include "sensor.h"
#include "api_adc.h"
#include "api_sensor.h"
#include "cJSON.h"
#include "eeprom.h"

#define SUB_DEVICES_ADDR_DEFAULT 0xFF
#define SUB_DEVICES_ADDR_PRIFIXED 0x80

#define SUB_DEVICES_FAILED_MAX_COUNT 3
#define SUB_DEVICES_SENDMSG_WAIT_TIMEOUT_XMS 20

#define SUB_DEVICES_TASK_DELAY_MS 500
#define SUB_DEVICES_TIMER_SAMPLE_PERIOD_XMS 1000
#define SUB_DEVICES_TIMER_UPLOAD_PERIOD_XMS 1100
#define SUB_DEVICES_TIMER_SWITCH_IPMB_BUS_XMS 2000

typedef enum
{
    SUB_DEVICES_TIMER_SAMPLE = 0,
    SUB_DEVICES_TIMER_UPLOAD = 1,
} SUB_DEVICES_TIMER_WORK;

typedef struct
{
    uint8_t sensorUnitTypeCode : 7;
    uint8_t isExist : 1;
    char *typeName;
} SENSOR_UNITTYPECODE_EXIST;
static  SENSOR_UNITTYPECODE_EXIST g_sensorUnitTypeExist[] = {
    {IPMI_UNIT_DEGREES_C, 0, "tempture"},
    {IPMI_UNIT_VOLTS, 0, "voltage"},
    {IPMI_UNIT_AMPS, 0, "current"},
    {IPMI_UNIT_WATTS, 0, "power"},
    {IPMI_UNIT_RPM, 0, "fan"},
};
typedef struct
{
    uint8_t sensorUnitTypeCount;
    SENSOR_UNITTYPECODE_EXIST *sensorUnitTypeExist;
} SubDeviceTpyeExist_T;

static SubDeviceTpyeExist_T g_subDeviceTypeExist = {
    .sensorUnitTypeCount = ARRARY_SIZE(g_sensorUnitTypeExist),
    .sensorUnitTypeExist = g_sensorUnitTypeExist,
};

static SubDeviceModeStatus_T *pSubDeviceSelf = NULL;

static const SubDeviceName_T g_SubDeviceConfigName[] = {
    {SUB_DEVICE_MODE_MAIN, "main"},
    {SUB_DEVICE_MODE_POWER, "power"},
    {SUB_DEVICE_MODE_SWITCH, "switch"},
    {SUB_DEVICE_MODE_STORAGE0, "storage0"},
    {SUB_DEVICE_MODE_STORAGE1, "storage1"},
    {SUB_DEVICE_MODE_STORAGE2, "storage2"},
};

char *SubDevice_GetModeName(SUB_DEVICE_MODE mode)
{
    if (mode < SUB_DEVICE_MODE_MAX)
    {
        for (uint8_t i = SUB_DEVICE_MODE_MIN; i < SUB_DEVICE_MODE_MAX; i++)
        {
            if (mode == g_SubDeviceConfigName[i].mode)
            {
                return g_SubDeviceConfigName[i].name;
            }
        }
    }
    return "\n";
}
static SubDeviceModeStatus_T g_AllModesStatus[SUB_DEVICE_MODE_MAX];

static void SubDevice_InitAllMode(void)
{
    for (uint8_t i = SUB_DEVICE_MODE_MIN; i < SUB_DEVICE_MODE_MAX; i++)
    {
        SubDeviceModeStatus_T *obj = &g_AllModesStatus[i];
        obj->isMain = false;
        obj->isOnLine = false;
        obj->mode = (SUB_DEVICE_MODE)i;
        obj->name = g_SubDeviceConfigName[i].name;
        obj->i2c0SlaveAddr = obj->i2c1SlaveAddr = SUB_DEVICES_ADDR_DEFAULT;
    }
}

uint8_t SubDevice_modeConvertSlaveAddr(SUB_DEVICE_MODE mode)
{
    return SUB_DEVICES_ADDR_PRIFIXED | (mode << 1);
}
static void SubDevice_InsertMode(SubDeviceModeStatus_T *obj, SUB_DEVICE_MODE mode)
{
    if (mode == SUB_DEVICE_MODE_MAIN)
    {
        obj->isMain = true;
    }
    obj->isOnLine = true;
    obj->mode = mode;
    obj->name = g_SubDeviceConfigName[mode].name;
    obj->i2c0SlaveAddr = obj->i2c1SlaveAddr = SubDevice_modeConvertSlaveAddr(mode);
    obj->busUsed = NM_SECONDARY_IPMB_BUS;
    SetDevAddr(obj->i2c0SlaveAddr);
}
void SubDevice_PrintModeName(void)
{
    for (uint32_t i = 0; i < ARRARY_SIZE(g_SubDeviceConfigName); i++)
    {
        LOG_E("\t\tid %d----- [%s]\r\n", i, SubDevice_GetModeName(g_SubDeviceConfigName[i].mode));
    }
}
bool SubDevice_CheckAndPrintMode(void)
{
    char buff[100];
    memset(buff, 0, sizeof(buff));
    SUB_DEVICE_MODE mode = (SUB_DEVICE_MODE)get_board_addr();
    if (mode >= SUB_DEVICE_MODE_MAX)
    {
        LOG_E("This borad ID is not support, id=%d\r\n", mode);
        LOG_E("\tBelow are supported\r\n");
        sprintf(buff, "This borad ID is not support, id=%d\r\n", mode);
        EEP_WriteDataCheckFirst(EEP_ADDR_SAVE_MODE, (uint8_t *)buff, strlen(buff));
        SubDevice_PrintModeName();
        return false;
    }
    LOG_I("\r\n\r\nThis borad as [%s]\r\n", SubDevice_GetModeName(mode));
    sprintf(buff, "This borad as [%s]\r\n", SubDevice_GetModeName(mode));
    EEP_WriteDataCheckFirst(EEP_ADDR_SAVE_MODE, (uint8_t *)buff, strlen(buff));
    SubDevice_InitAllMode();

    SubDevice_InsertMode(&g_AllModesStatus[mode], mode);
    pSubDeviceSelf = &g_AllModesStatus[mode];
    return true;
}

static bool SubDevice_isExistSensorUnit(uint8_t queryUnitType)
{
    uint8_t sensorNum;
    uint8_t unitType;
    for (SUB_DEVICE_MODE dev = SUB_DEVICE_MODE_MIN; dev < SUB_DEVICE_MODE_MAX; dev++)
    {
        const Dev_Handler *pHandler = api_getDevHandler(dev);
        if (pHandler == NULL)
        {
            continue;
        }
        for (uint8_t numIdex = 0; numIdex < pHandler->sensorCfgSize; numIdex++)
        {
            sensorNum = api_sensorGetSensorNumByIdex(dev, numIdex);

            if (api_sensorGetUnitType(dev, sensorNum, &unitType) == true)
            {
                if (unitType == queryUnitType)
                {
                    return true;
                }
            }
        }
    }
    return false;
}
static void SubDevice_readAllSensorUnit(void)
{
    for (uint8_t idx = 0; idx < g_subDeviceTypeExist.sensorUnitTypeCount; idx++)
    {
        SENSOR_UNITTYPECODE_EXIST *pTypeCode = &g_subDeviceTypeExist.sensorUnitTypeExist[idx];
        if (SubDevice_isExistSensorUnit(pTypeCode->sensorUnitTypeCode))
        {
            pTypeCode->isExist = true;
        }
    }
}

/// @brief read my slave address from GPIO switch,then init all buff if I'm a main(master)
/// @param
/// @return
static void SubDevice_Init(void)
{
    cJSON_Hooks hooks;
    hooks.malloc_fn = pvPortMalloc;
    hooks.free_fn = vPortFree;
    cJSON_InitHooks(&hooks);

    SubDevice_readAllSensorUnit();
}

bool SubDevice_IsSelfMaster(void)
{
    if (pSubDeviceSelf == NULL)
    {
        return false;
    }
    return pSubDeviceSelf->isMain;
}
SUB_DEVICE_MODE SubDevice_GetMyMode(void)
{
    if (pSubDeviceSelf == NULL)
    {
        return SUB_DEVICE_MODE_MAX;
    }
    return pSubDeviceSelf->mode;
}
/// @brief master and slave i2c init will be called
/// @param bus
/// @return
uint8_t SubDevice_GetMySlaveAddress(uint32_t bus)
{
    if (pSubDeviceSelf == NULL)
    {
        return 0;
    }
    switch (bus)
    {
    case I2C0:
    case NM_PRIMARY_IPMB_BUS:
        return pSubDeviceSelf->i2c0SlaveAddr;
    case I2C1:
    case NM_SECONDARY_IPMB_BUS:
        return pSubDeviceSelf->i2c1SlaveAddr;
    default:
        return 0;
    }
}
SubDeviceModeStatus_T *SubDevice_GetSelf(void)
{
    return pSubDeviceSelf;
}
// master called only
static bool SubDevice_IsOnLine(SUB_DEVICE_MODE mode)
{
    for (uint8_t i = SUB_DEVICE_MODE_MIN; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModesStatus[i].mode == mode)
        {
            return g_AllModesStatus[i].isOnLine;
        }
    }
    return false;
}
static void SubDevice_SetOnLine(SUB_DEVICE_MODE mode, bool isOnline)
{
    for (uint8_t i = SUB_DEVICE_MODE_MIN; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModesStatus[i].mode == mode)
        {
            g_AllModesStatus[i].isOnLine = isOnline;
            return;
        }
    }
}

__attribute__((unused)) static bool SubDevice_QueryModeByAddr(uint8_t addrBit8, SUB_DEVICE_MODE *mode)
{
    uint8_t addrHi = addrBit8 & 0xF0;
    uint8_t addrLow = (addrBit8 & 0x0F) >> 1;

    if (addrHi != SUB_DEVICES_ADDR_PRIFIXED)
    {
        return false;
    }

    if (addrLow > SUB_DEVICE_MODE_MAX)
    {
        return false;
    }
    *mode = (SUB_DEVICE_MODE)addrLow;
    return true;
}
uint32_t SubDevice_GetBus(SUB_DEVICE_MODE mode)
{
    for (uint8_t i = SUB_DEVICE_MODE_MIN; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModesStatus[i].mode == mode)
        {
            return g_AllModesStatus[i].busUsed;
        }
    }
    return NM_SECONDARY_IPMB_BUS;
}

static void SubDevice_SwitchBus(SUB_DEVICE_MODE mode)
{
    for (uint8_t i = SUB_DEVICE_MODE_MIN; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModesStatus[i].mode == mode)
        {
            if (g_AllModesStatus[i].busUsed == NM_SECONDARY_IPMB_BUS) {
                g_AllModesStatus[i].busUsed = NM_PRIMARY_IPMB_BUS;
            } else {
                g_AllModesStatus[i].busUsed = NM_SECONDARY_IPMB_BUS;
            }
            return;
        }
    }
}
static void SubDevice_SendDataSpilt(uint32_t usart_periph, char *pstr)
{
#define SEND_LENGTH_PRE     (UART1_BUFF_SIZE / 2)
    uint32_t len = strlen(pstr);
    uint32_t offset;

    for (uint32_t i = 0; (i < (len + SEND_LENGTH_PRE) / SEND_LENGTH_PRE);)
    {
        offset = i * SEND_LENGTH_PRE;
        if ((offset + SEND_LENGTH_PRE) <= len)
        {
            if (UART_sendDataBlock(usart_periph, (uint8_t *)pstr + offset, SEND_LENGTH_PRE))
            {
                i++;
                vTaskDelay(2);// 0.1ms/byte
            }
            else
            {
                vTaskDelay((SEND_LENGTH_PRE / 10) + 5);// 0.1ms/byte
            }
        }
        else
        { // last
            if (UART_sendDataBlock(usart_periph, (uint8_t *)pstr + offset, len - offset))
            {
                return;
            }
        }
    }
}
static void SubDevice_Upload()
{
    char nameBuf[30];
    char humanVal[10];
    uint8_t sensorNum;
    uint8_t unitType;
    uint8_t prefixLen;
    cJSON *pCJType = cJSON_CreateObject();
    cJSON *pCJData = NULL;
    char *pstr;

    LOG_D("\t\tSubDevice_Upload, free byte = %d\n", xPortGetFreeHeapSize());
    for (uint8_t idx = 0; idx < g_subDeviceTypeExist.sensorUnitTypeCount; idx++)
    {
        SENSOR_UNITTYPECODE_EXIST *pTypeCode = &g_subDeviceTypeExist.sensorUnitTypeExist[idx];
        if (pTypeCode->isExist == false)
        {
            continue;
        }

        cJSON_AddItemToObject(pCJType, pTypeCode->typeName, pCJData = cJSON_CreateObject());
        for (SUB_DEVICE_MODE dev = SUB_DEVICE_MODE_MIN; dev < SUB_DEVICE_MODE_MAX; dev++)
        {
            char *modeName = SubDevice_GetModeName(dev);
            memset(nameBuf, 0, sizeof(nameBuf));
            memcpy(nameBuf, modeName, strlen(modeName));
            strcat(nameBuf, "_");
            prefixLen = strlen(nameBuf);
            const Dev_Handler *pHandler = api_getDevHandler(dev);
            if (pHandler == NULL)
            {
                continue;
            }
            for (uint8_t numIdex = 0; numIdex < pHandler->sensorCfgSize; numIdex++)
            {
                sensorNum = api_sensorGetSensorNumByIdex(dev, numIdex);
                if (api_sensorGetUnitType(dev, sensorNum, &unitType) == true)
                {
                    if (unitType == pTypeCode->sensorUnitTypeCode)
                    {
                        char *sensorName = pHandler->sensorCfg[numIdex].sensorAlias;
                        memset(nameBuf + prefixLen, 0, sizeof(nameBuf) - prefixLen);
                        strcat(nameBuf, sensorName);

                        SubDevice_Reading_T *pDeviceReading = &pHandler->val[numIdex];
                        memset(humanVal, 0, sizeof(humanVal));
                        if (pDeviceReading->human == 0)
                        {
                            sprintf(humanVal, "%s", "0");
                        }
                        else
                        {
                            sprintf(humanVal, "%.3f", pDeviceReading->human);
                        }
                        cJSON_AddStringToObject(pCJData, nameBuf, humanVal);
                    }
                }
            }
        }
    }
    pstr = cJSON_PrintUnformatted(pCJType);
    if (pstr == NULL)
    {
        cJSON_Delete(pCJType);
        LOG_E("\t\tupload failed, no memory to malloc, free byte = %d\r\n", xPortGetFreeHeapSize());
    }
    else
    {
        char *newLine = "\r\n";
        SubDevice_SendDataSpilt(CPU_UART_PERIPH, pstr);
        while (!UART_sendDataBlock(CPU_UART_PERIPH, (uint8_t *)newLine, strlen(newLine)));
        
        if (g_debugLevel >= DBG_LOG) {
            SubDevice_SendDataSpilt(DEBUG_UART_PERIPH, pstr);
            while (!UART_sendDataBlock(DEBUG_UART_PERIPH, (uint8_t *)newLine, strlen(newLine)));
        }
        cJSON_Delete(pCJType);
        vPortFree(pstr);

        LOG_D("\t\tupload success, free byte = %d\n", xPortGetFreeHeapSize());
    }
}
static bool SubDevice_readingSensorForeach(SUB_DEVICE_MODE dev, uint8_t sensorNum, MsgPkt_T *requestPkt, SubDevice_Reading_T *pDeviceReading)
{
    MsgPkt_T recvReq;
    IPMIMsgHdr_T *hdr = (IPMIMsgHdr_T *)&(requestPkt->Data);

    int len = sizeof(IPMIMsgHdr_T);
    requestPkt->Data[len++] = sensorNum;
    requestPkt->Data[len++] = dev; // OEMFiled

    requestPkt->Data[len++] = CalculateCheckSum(requestPkt->Data, len);
    requestPkt->Size = len;

    GetSensorReadingRes_T *pSensorReading = (GetSensorReadingRes_T *)(&recvReq.Data[sizeof(IPMIMsgHdr_T)]);
    if (dev == SUB_DEVICE_MODE_MAIN)
    { // skip master self
        requestPkt->Data[0] = sensorNum;
        requestPkt->Data[1] = dev;
        GetSensorReading(requestPkt->Data, 2, (INT8U *)&recvReq.Data[sizeof(IPMIMsgHdr_T)], 0);
    }
    else
    {
        if (SendMsgAndWait(requestPkt, &recvReq, SUB_DEVICES_SENDMSG_WAIT_TIMEOUT_XMS) == pdFALSE)
        {
            return false;
        }
        IPMIMsgHdr_T *recHdr = (IPMIMsgHdr_T *)&recvReq.Data;
        // pSensorReadRes->OptionalStatus = pSensorReadReq->SensorNum;
        if ((recHdr->ReqAddr != hdr->ResAddr) || (pSensorReading->OptionalStatus != sensorNum))
        {
            return false;
        }
    }

    if (pSensorReading->CompletionCode == CC_NORMAL)
    {
        pDeviceReading->rawIPMB = pSensorReading->SensorReading;
        //pDeviceReading->ComparisonStatus = pSensorReading->ComparisonStatus;
        return true;
    }
    return false;
}
/// @brief Sample All ipmb val, and convert to human
static void SubDevice_SampleAll()
{
    MsgPkt_T requestPkt;
    IPMIMsgHdr_T *hdr = (IPMIMsgHdr_T *)&(requestPkt.Data);
    requestPkt.Param = IPMB_SUB_DEVICE_HEARTBEAT_REQUEST;
    uint8_t sensorNum;
    bool isDevOnline;

    for (SUB_DEVICE_MODE dev = SUB_DEVICE_MODE_MIN; dev < SUB_DEVICE_MODE_MAX; dev++)
    {
        const Dev_Handler *pHandler = api_getDevHandler(dev);
        if (pHandler == NULL)
        {
            continue;
        }
        requestPkt.Channel = SubDevice_GetBus(dev);
        // hdr->ResAddr = 0x20;      //  main ipmb
        hdr->ResAddr = SubDevice_modeConvertSlaveAddr(dev);
        hdr->NetFnLUN = NETFN_SENSOR << 2; // RAW
        hdr->ChkSum = CalculateCheckSum((INT8U *)hdr, 2);

        hdr->ReqAddr = SubDevice_GetMySlaveAddress(requestPkt.Channel);
        hdr->RqSeqLUN = 0x01;
        hdr->Cmd = CMD_GET_SENSOR_READING;
        isDevOnline = false;
        for (uint8_t numIdex = 0; numIdex < pHandler->sensorCfgSize; numIdex++)
        {
            SubDevice_Reading_T *pDeviceReading = &pHandler->val[numIdex]; 
            // sensorNum = 0x23;     // P1V8 standby
            sensorNum = api_sensorGetSensorNumByIdex(dev, numIdex);
            if (SubDevice_readingSensorForeach(dev, sensorNum, &requestPkt, pDeviceReading))
            {
                isDevOnline = true;
                if (api_sensorConvert2HumanVal(dev, sensorNum, pDeviceReading->rawIPMB, &pDeviceReading->human) == true)
                {
                    pDeviceReading->errCnt = 0;
                    continue; // success
                }
            }
            if (pDeviceReading->errCnt++ >= SUB_DEVICES_FAILED_MAX_COUNT)
            {
                pDeviceReading->errCnt = SUB_DEVICES_FAILED_MAX_COUNT;
                pDeviceReading->rawIPMB = 0;
                pDeviceReading->human = 0;
                isDevOnline = false;
            }else{
                isDevOnline = true;
            }
        }
        SubDevice_SetOnLine(dev , isDevOnline);
    }
}
static void SubDevice_statisticsOnlineSwitchBus(void)
{
    for (SUB_DEVICE_MODE dev = SUB_DEVICE_MODE_MIN; dev < SUB_DEVICE_MODE_MAX; dev++)
    {
        if (!SubDevice_IsOnLine(dev)) {
            SubDevice_SwitchBus(dev);
            LOG_W("[ %d : %-10s ] is offline, switch ipmb bus to %d\r\n", dev, SubDevice_GetModeName(dev), SubDevice_GetBus(dev));
        } else {
            LOG_D("[ %d : %-10s ] is online\r\n", dev, SubDevice_GetModeName(dev));
        }
    }
}
/// @brief sample & upload
/// @param pvParameters 
void SubDevice_uploadTask(void *pvParameters)
{
    uint32_t sampleTimeStamp = 0;
    uint32_t uploadTimeStamp = 0;
    uint32_t switchBusTimeStamp = 0;
    cJSON_Hooks hooks;
    hooks.malloc_fn = pvPortMalloc;
    hooks.free_fn = vPortFree;

    SubDevice_Init();
    cJSON_InitHooks(&hooks);
    while (1)
    {
        vTaskDelay(SUB_DEVICES_TASK_DELAY_MS);
        if ((GetTickMs() - sampleTimeStamp) >= SUB_DEVICES_TIMER_SAMPLE_PERIOD_XMS){
            sampleTimeStamp = GetTickMs();
            SubDevice_SampleAll();
        }

        if ((GetTickMs() - uploadTimeStamp) >= SUB_DEVICES_TIMER_UPLOAD_PERIOD_XMS){
            uploadTimeStamp = GetTickMs();
            SubDevice_Upload();
        }

       if ((GetTickMs() - switchBusTimeStamp) >= SUB_DEVICES_TIMER_SWITCH_IPMB_BUS_XMS){
            switchBusTimeStamp = GetTickMs();
            SubDevice_statisticsOnlineSwitchBus();
        }
    }
}



