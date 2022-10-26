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

typedef struct
{
    uint32_t  busUsed;
    uint8_t   slaveSensorNum[2];
} SubDeviceHandler_T;

static SubDeviceHandler_T g_subDeviceHandler;

static void SubDevice_HeartBeatTimerCallBack(xTimerHandle pxTimer);
static const SubDeviceName_T g_SubDeviceName[] = {
    {SUB_DEVICE_MODE_MAIN,      "main"},
    {SUB_DEVICE_MODE_POWER,     "power"},
    {SUB_DEVICE_MODE_NET,       "net"},
    {SUB_DEVICE_MODE_SWITCH,    "switch"},
    {SUB_DEVICE_MODE_STORAGE0,  "storage0"},
    {SUB_DEVICE_MODE_STORAGE1,  "storage1"},
    {SUB_DEVICE_MODE_STORAGE2,  "storage2"},
};

static char* SubDevice_GetModeName(SUB_DEVICE_MODE mode)
{
    if (mode < SUB_DEVICE_MODE_MAX) {
        for (uint8_t i = 0; i < SUB_DEVICE_MODE_MAX; i++)
        {
			if (mode == g_SubDeviceName[i].mode) {
				return g_SubDeviceName[i].name;
			}
		}
    }
    return "\n";
}
static SubDeviceMODE_T g_AllModes[SUB_DEVICE_MODE_MAX];

#define SUB_DEVICES_ADDR_DEFAULT 0xFF
#define SUB_DEVICES_ADDR_PRIFIXED 0x80
static void SubDevice_InitAllMode(void)
{
    for (uint8_t i = 0; i < SUB_DEVICE_MODE_MAX; i++)
    {
        SubDeviceMODE_T *obj = &g_AllModes[i];
        obj->isMain = false;
        obj->isOnLine = false;
        obj->mode = SUB_DEVICE_MODE_MAX;
        obj->name = g_SubDeviceName[i].name;
        obj->i2c0SlaveAddr = obj->i2c1SlaveAddr = SUB_DEVICES_ADDR_DEFAULT;
    }
}

static uint8_t SubDevice_modeConvertSlaveAddr(SUB_DEVICE_MODE mode)
{
    return SUB_DEVICES_ADDR_PRIFIXED | (mode << 1);
}
static void SubDevice_InsertMode(SubDeviceMODE_T *obj, SUB_DEVICE_MODE mode)
{
    if (mode == SUB_DEVICE_MODE_MAIN) {
        obj->isMain = true;
    }
    obj->isOnLine = true;
    obj->mode = mode;
    obj->name = g_SubDeviceName[mode].name;
    obj->i2c0SlaveAddr = obj->i2c1SlaveAddr = SubDevice_modeConvertSlaveAddr(mode);
}
static SubDeviceMODE_T *pSubDeviceSelf = NULL;
/// @brief read my slave address from GPIO switch,then init all buff if I'm a main(master)
/// @param  
/// @return 
bool SubDevice_Init(void)
{
    SUB_DEVICE_MODE mode = (SUB_DEVICE_MODE)get_board_slave_addr();
    if (mode >= SUB_DEVICE_MODE_MAX) {
        printf("This borad ID is not support, id=%d\n", mode);
        printf("\tBelow are supported\n");
        for (uint32_t i = 0; i < (sizeof(g_SubDeviceName) / sizeof(g_SubDeviceName[0])); i++)
        {
            printf("\t\tid %d----- [%s]\n", i, SubDevice_GetModeName((SUB_DEVICE_MODE)i));
        }
        return false;
    }
    printf("This borad as [%s]\n", SubDevice_GetModeName(mode));
    SubDevice_InitAllMode();

    SubDevice_InsertMode(&g_AllModes[mode], mode);
    pSubDeviceSelf = &g_AllModes[mode];

    g_subDeviceHandler.busUsed = NM_SECONDARY_IPMB_BUS;          
    SDR_GetAllRecSensorNum((INT8U)SUB_DEVICE_MODE_POWER, g_subDeviceHandler.slaveSensorNum, sizeof(g_subDeviceHandler.slaveSensorNum));
    if (SubDevice_IsSelfMaster()) {
        TimerHandle_t xTimersIpmiReset = xTimerCreate("SubDeviceHeartBeat", 500/portTICK_RATE_MS, pdTRUE, 
                                        0, SubDevice_HeartBeatTimerCallBack);
        return xTimerStart(xTimersIpmiReset, portMAX_DELAY);
    }
	return true;
}

bool SubDevice_IsSelfMaster(void)
{
    if (pSubDeviceSelf == NULL) {
        return false;
    }
    return pSubDeviceSelf->isMain;
}
SUB_DEVICE_MODE SubDevice_GetMyMode(void)
{
    if (pSubDeviceSelf == NULL) {
        return SUB_DEVICE_MODE_MAX;
    }
    return pSubDeviceSelf->mode;
}
bool SubDevice_IsOnLine(void)
{
    if (pSubDeviceSelf == NULL) {
        return false;
    }
    return pSubDeviceSelf->isOnLine;
}
/// @brief master and slave i2c init will be called
/// @param bus 
/// @return 
uint8_t SubDevice_GetMySlaveAddress(uint32_t bus)
{
    if (pSubDeviceSelf == NULL) {
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
SubDeviceMODE_T *SubDevice_GetSelf(void)
{
    return pSubDeviceSelf;
}
// master called only
static void SubDevice_SetOnLine(SUB_DEVICE_MODE mode, bool isOnline)
{
    for (uint8_t i = 0; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModes[i].mode == mode) {
            pSubDeviceSelf->isOnLine = isOnline;
            return;
        }
    }
}
    
static bool SubDevice_QueryModeByAddr(uint8_t addrBit8, SUB_DEVICE_MODE *mode)
{
    uint8_t addrHi = addrBit8 & 0xF0;
    uint8_t addrLow = (addrBit8 & 0x0F) >> 1;

    if (addrHi != SUB_DEVICES_ADDR_PRIFIXED) {
        return false;
    }

    if (addrLow > SUB_DEVICE_MODE_MAX) {
        return false;
    }
    *mode = (SUB_DEVICE_MODE)addrLow;
    return true;
}
bool SubDevice_Management(uint8_t addr)
{
    SUB_DEVICE_MODE newMode;
    if (SubDevice_QueryModeByAddr(addr, &newMode) == false) {
        return false;
    }
    uint8_t i = 0;
    for (i = 0; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModes[i].mode == newMode) {
            return false; //already exist
        }
        if (g_AllModes[i].mode == SUB_DEVICE_MODE_MAX) {// get a new buff
            SubDevice_InsertMode(&g_AllModes[i], newMode);
            return true;
        }
    }
    return false; // full
}
uint32_t SubDevice_GetBus(void)
{
    return g_subDeviceHandler.busUsed;
}
extern xQueueHandle ResponseDatMsg_Queue;
static void SubDevice_HeartBeatTimerCallBack(xTimerHandle pxTimer)
{
	//uint32_t cmd = (CHASSIS_CMD_CTRL)((uint32_t)pvTimerGetTimerID(pxTimer));

    MsgPkt_T requestPkt, recvReq;
    IPMIMsgHdr_T *hdr = (IPMIMsgHdr_T *)&(requestPkt.Data);
    requestPkt.Param = IPMB_SUB_DEVICE_HEARTBEAT_REQUEST;
    requestPkt.Channel = SubDevice_GetBus();
    int len;

    for (SUB_DEVICE_MODE i = SUB_DEVICE_MODE_NET; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModes[i].mode == pSubDeviceSelf->mode) { // master self
            continue;
        }
        
        hdr->ResAddr = SubDevice_modeConvertSlaveAddr(i);
        hdr->NetFnLUN = NETFN_SENSOR << 2; // RAW
        hdr->ChkSum = CalculateCheckSum((INT8U*)hdr, 2);
        
        hdr->ReqAddr = SubDevice_GetMySlaveAddress(requestPkt.Channel);
        hdr->RqSeqLUN = 0x01;
        hdr->Cmd = CMD_GET_SENSOR_READING;

        for (uint8_t numIdex = 0; numIdex < sizeof(g_subDeviceHandler.slaveSensorNum); numIdex++)
        {
            len = sizeof(IPMIMsgHdr_T);
            requestPkt.Data[len++] = g_subDeviceHandler.slaveSensorNum[numIdex];
            requestPkt.Data[len++] = i; // OEMFiled

            requestPkt.Data[len++] = CalculateCheckSum(requestPkt.Data, len);
            requestPkt.Size = len;

            if (SendMsgAndWait(&requestPkt, &recvReq, 500) == pdFALSE)
            {
                continue;
            }
            if (recvReq.Size - sizeof(IPMIMsgHdr_T) == sizeof(GetSensorReadingRes_T)) {
                GetSensorReadingRes_T *pSensorReading = (GetSensorReadingRes_T *)&recvReq.Data[sizeof(IPMIMsgHdr_T)];
                if (pSensorReading->CompletionCode == CC_NORMAL) {
                    g_AllModes[i].deviceCache[0] = pSensorReading->SensorReading;
                    // float humanReading;
                    // ipmi_convert_reading( sdr_buffer, g_AllModes[i].deviceCache[0], &humanReading );
                }
            }
        }
    }
}
