#include <string.h>  
#include <stdlib.h>
#include "debug_print.h"
#include "api_subdevices.h"
#include "bsp_gpio.h"

static const SubDeviceName_T g_SubDeviceName[] = {
    {SUB_DEVICE_MODE_MAIN,      "main"},
    {SUB_DEVICE_MODE_POWER,     "power"},
    {SUB_DEVICE_MODE_NET,       "net"},
    {SUB_DEVICE_MODE_SWITCH,    "switch"},
    {SUB_DEVICE_MODE_STORAGE0,  "storage0"},
    {SUB_DEVICE_MODE_STORAGE1,  "storage1"},
    {SUB_DEVICE_MODE_STORAGE2,  "storage2"},
};

static SubDeviceMODE_T g_AllModes[SUB_DEVICE_MODE_MAX];

#define SUB_DEVICES_DEFAULT_ADDR 0xFF
static void SubDevice_InitAllMode(void)
{
    for (uint8_t i = SUB_DEVICE_MODE_MAIN; i < SUB_DEVICE_MODE_MAX; i++)
    {
        SubDeviceMODE_T *obj = &g_AllModes[i];
        obj->isMain = false;
        obj->isRegistered = false;
        obj->mode = SUB_DEVICE_MODE_MAX;
        obj->name = g_SubDeviceName[i].name;
        obj->slaveAddrI2cBus1 = SUB_DEVICES_DEFAULT_ADDR;
        obj->slaveAddrI2cBus2 = SUB_DEVICES_DEFAULT_ADDR;
    }
}
static void SubDevice_InsertMode(SubDeviceMODE_T *obj, SUB_DEVICE_MODE mode)
{
    if (mode == SUB_DEVICE_MODE_MAIN) {
        obj->isMain = true;
    } else {
        obj->isMain = false;
    }
    obj->isRegistered = true;
    obj->mode = mode;
    obj->name = g_SubDeviceName[mode].name;
    obj->slaveAddrI2cBus1 = 0x80 | mode << 1;
    obj->slaveAddrI2cBus2 = 0x90 | mode << 1;
}
static SubDeviceMODE_T *pSubDeviceSelf = NULL;
bool SubDevice_Init(void)
{
    SUB_DEVICE_MODE mode = (SUB_DEVICE_MODE)get_board_slave_addr();
    if (mode >= SUB_DEVICE_MODE_MAX) {
        return false;
    }
    SubDevice_InitAllMode();

    SubDevice_InsertMode(&g_AllModes[mode], mode);
    pSubDeviceSelf = &g_AllModes[mode];

	return true;
}
// master called only
static bool SubDevice_QueryModeByAddr(uint8_t addrBit8, SUB_DEVICE_MODE *mode)
{
    uint8_t addrHi = addrBit8 & 0xF0;
    uint8_t addrLow = (addrBit8 & 0x0F) >> 1;

    if ((addrHi != 0x80) && (addrHi != 0x90)) {
        return false;
    }
    
    if (addrLow > SUB_DEVICE_MODE_MAX) {
        return false;
    }
    *mode = (SUB_DEVICE_MODE)addrLow;
    return true;
}


bool SubDevice_IsMain(void)
{
    if (pSubDeviceSelf == NULL) {
        return false;
    }
    return pSubDeviceSelf->isMain;
}
bool SubDevice_IsRegistered(void)
{
    if (pSubDeviceSelf == NULL) {
        return false;
    }
    return pSubDeviceSelf->isRegistered;
}
SubDeviceMODE_T *SubDevice_GetSlef(void)
{
    return pSubDeviceSelf;
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

