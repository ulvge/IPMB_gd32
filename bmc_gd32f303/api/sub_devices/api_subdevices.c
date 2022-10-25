#include <string.h>  
#include <stdlib.h>
#include "debug_print.h"
#include "api_subdevices.h"
#include "bsp_gpio.h"
#include "bsp_i2c.h"
#include "IPMIConf.h"
#include "IPMDevice.h"
#include "OSPort.h"

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
static void SubDevice_InsertMode(SubDeviceMODE_T *obj, SUB_DEVICE_MODE mode)
{
    if (mode == SUB_DEVICE_MODE_MAIN) {
        obj->isMain = true;
    } else {
        obj->isMain = false;
    }
    obj->isOnLine = true;
    obj->mode = mode;
    obj->name = g_SubDeviceName[mode].name;
    obj->i2c0SlaveAddr = obj->i2c1SlaveAddr = SUB_DEVICES_ADDR_PRIFIXED | mode << 1;
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
static uint32_t g_busUsed = NM_SECONDARY_IPMB_BUS;
uint32_t SubDevice_GetBus(void)
{
    return g_busUsed;
}
extern xQueueHandle ResponseDatMsg_Queue;
static void SubDevice_HeartBeatTimerCallBack(xTimerHandle pxTimer)
{
	//uint32_t cmd = (CHASSIS_CMD_CTRL)((uint32_t)pvTimerGetTimerID(pxTimer));

    SamllMsgPkt_T msgPkt;
    //IPMIMsgHdr_T *hdr = &msgPkt.Data;
    msgPkt.Param = FORWARD_IPMB_RESPONSE;
    msgPkt.Channel = NM_SECONDARY_IPMB_BUS;
    msgPkt.Size = sizeof(IPMIMsgHdr_T) + sizeof(INT8U);

    for (uint8_t i = 0; i < SUB_DEVICE_MODE_MAX; i++)
    {
        if (g_AllModes[i].mode == pSubDeviceSelf->mode) { // master self
            continue;
        }
//        if (g_AllModes[i].mode == SUB_DEVICE_MODE_MAX) {// get a new buff
//            SubDevice_InsertMode(&g_AllModes[i], newMode);
//            return true;
//        }
//        SendMessage
    }
}
