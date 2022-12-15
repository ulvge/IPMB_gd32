/*!
    \file    mac5023.c
    \brief   api sensor

    \version
*/
#include "string.h"

#include "mac5023.h"
#include "bsp_i2c_gpio.h"
#include "bsp_i2c.h"
#include "FreeRTOS.h"
#include "task.h"

#define MAC5023_I2C_BUS I2C_BUS_S0
#define MAC5023_I2C_SLAVE_ADDR  (0x40 << 1)

#define MAC5023_PIN_M   1 // 1W /LSB
#define MAC5023_IOUT_M  (62.5/1000) // 62.5mA/LSB
#if 0
#define MAC5023_VIN_M   (25.0/1000)    // 25mV /LSB
#define MAC5023_VOUT_M  (1.25/1000) // 1.25mV /LSB
#else
#define MAC5023_VIN_M   (31.25/1000) // 31.25mV /LSB
#define MAC5023_VOUT_M  (31.25/1000)  // 31.25mV /LSB
#endif
typedef struct
{
    UINT8 dev;
    char *name;
} MAC5023_VOUT_CONFIG;

static const MAC5023_VOUT_CONFIG g_MAC5023DevsConfig[] = {
    {MAC5023_I2C_SLAVE_ADDR,    "P12V"},
};

typedef struct
{
    UINT8 cmd;
    UINT16 mask;
    float scaleZoom;
    float lsb;
    char *name;
    char *units;
} MAC5023_CMDS_CONFIG;

#define  MAC5023_MASK_DEFAULT 0x3FFF
static const MAC5023_CMDS_CONFIG g_MAC5023CmdsConfig[] = {
    {PMBUS_VOUT_COMMAND,    MAC5023_MASK_DEFAULT, 1,    2,             "VoutCmd", "V"},
    {PMBUS_VOUT_SCALE_LOOP, MAC5023_MASK_DEFAULT, 1,    0.001f,        "ScaleLoop",  " "},
    {PMBUS_READ_PIN,        MAC5023_MASK_DEFAULT, 0.25, MAC5023_PIN_M,  "P-IN", "watt"},
    {PMBUS_READ_VIN,        MAC5023_MASK_DEFAULT, 10,   MAC5023_VIN_M,  "V-IN", "V"},

    {PMBUS_READ_VOUT,       MAC5023_MASK_DEFAULT, 10,   MAC5023_VOUT_M, "V-OUT","V"}, //(R2+R1)/R2,R2=1
    {PMBUS_READ_IOUT,       MAC5023_MASK_DEFAULT, 5, 	MAC5023_IOUT_M, "I-OUT","A"},
};


#define MAC5023_V_I_MASK 0x3FF
#define MAC5023_TEMP_M 1

static const char *MAC5023_MODE_NAME = "3205PM";

static bool g_MAC5023_initSuccess[ARRARY_SIZE(g_MAC5023DevsConfig)] = {false};

static char * MAC5023_getDevName(UINT8 dev)
{
	for (UINT8 i = 0; i < ARRARY_SIZE(g_MAC5023DevsConfig); i++)
    {
        if(g_MAC5023DevsConfig[i].dev == dev) {
            return g_MAC5023DevsConfig[i].name;
        }
    }
    return "NULL";
}
static char * MAC5023_getCmdsUints(const MAC5023_CMDS_CONFIG *p_DevConfig, UINT8 configSize, UINT8 cmd)
{
	for (UINT8 i = 0; i < configSize; i++)
    {
        if(p_DevConfig[i].cmd == cmd) {
            return p_DevConfig[i].units;
        }
    }
    return "NULL";
}
static bool MAC5023_init(UINT8 bus, UINT8 slaveAddr)
{
    char modeStr[10];

    memset(modeStr, 0, sizeof(modeStr));
	modeStr[0] = strlen(MAC5023_MODE_NAME) + 1;
    int ret = PMBus_I2CRead(bus, slaveAddr, PMBUS_MFR_MODEL, (u8 *)modeStr);
    if (ret > 0)
    {
        if (strcmp(modeStr + 1, MAC5023_MODE_NAME) == 0)
        {
            return true;
        }
        else
        {
            LOG_I("MAC5023_init error, mode %s is not support\r\n", MAC5023_getDevName(slaveAddr));
        }
    }
    else
    {
        LOG_I("MAC5023_init read bus error\r\n");
    }
    return false;
}

static const MAC5023_CMDS_CONFIG *MAC5023_getCmdConfig(UINT8 cmd)
{
	for (UINT8 i = 0; i < ARRARY_SIZE(g_MAC5023CmdsConfig); i++)
    {
        if(g_MAC5023CmdsConfig[i].cmd == cmd) {
            return &g_MAC5023CmdsConfig[i];
        }
    }
    return NULL;
}
static char *MAC5023_getCmdName(UINT8 cmd)
{
    const MAC5023_CMDS_CONFIG *cmdCfg = MAC5023_getCmdConfig(cmd);
    if (cmdCfg == NULL) {
        return "";
    }
    return cmdCfg->name;
}
static bool MAC5023_ReadItem(UINT8 bus, UINT8 slaveAddr, UINT8 cmd, float *humVal, UINT8 *ipmbVal)
{
    u8 buff[4];
    memset(buff, 0, sizeof(buff));
    const MAC5023_CMDS_CONFIG *cmdCfg = MAC5023_getCmdConfig(cmd);
    if (cmdCfg == NULL) {
        return false;
    }
    int ret = PMBus_I2CRead(bus, slaveAddr, cmdCfg->cmd, buff);
    if (ret > 0)
    {
        u16 rawAdc = ((buff[1] << 8) | buff[0]) & (cmdCfg->mask);
        *humVal = rawAdc * (cmdCfg->lsb);
        *ipmbVal = (UINT8)(*humVal * (cmdCfg->scaleZoom));
        return true;
    }else{
        return false;
    }
}

bool MAC5023_Sample(UINT8 devIndex, UINT8 cmd, float *humanVal, UINT8 *ipmbVal)
{
    if (devIndex >= ARRARY_SIZE(g_MAC5023DevsConfig)) {
        return false;
    }
    UINT8 slaveAddr = g_MAC5023DevsConfig[devIndex].dev;
    if (g_MAC5023_initSuccess[devIndex] == false){
        if (!MAC5023_init(MAC5023_I2C_BUS, slaveAddr))
        {
            return false;
        }else{
            g_MAC5023_initSuccess[devIndex] = true;
        }
    }

    bool res = MAC5023_ReadItem(MAC5023_I2C_BUS, slaveAddr, cmd, humanVal, ipmbVal);
	
    LOG_D("MAC5023_Sample dev name=%-8s, cmd=%s, addr7=%#x, humanVal=%-5.2f %s\r\n",
           g_MAC5023DevsConfig[devIndex].name, MAC5023_getCmdName(cmd), slaveAddr >> 1, *humanVal,
           MAC5023_getCmdsUints(g_MAC5023CmdsConfig, ARRARY_SIZE(g_MAC5023CmdsConfig), cmd));
	return res;
}
