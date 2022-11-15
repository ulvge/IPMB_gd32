/*!
    \file    api_sensor.c
    \brief   api sensor

    \version
*/
#include "string.h"

#include "mac5023.h"
#include "bsp_i2c_gpio.h"
#include "bsp_i2c.h"

#define MAC5023_I2C_BUS I2C_BUS_S0

#if 1       
#define MAC5023_I2C_SLAVE_ADDR_P0V9_1 (0x36 << 1) // 1      P0V9_VCORE
#define MAC5023_I2C_SLAVE_ADDR_P0V9_2 (0x38 << 1) // 1.475  P0V9_VCORE
#define MAC5023_I2C_SLAVE_ADDR_P0V885 (0x3E << 1) // 1.475  0.885 VDD_GPU 0x7C
#define MAC5023_I2C_SLAVE_ADDR_P1V2 (0x31 << 1) // 2      P1V2_VDDQ
#define MAC5023_I2C_SLAVE_ADDR_P5V  (0x30 << 1)    // 8.5    P5V

#define MAC1625_VIN_M   25    // 25mV /LSB
#define MAC1625_VOUT_M  1.25 // 1.25mV /LSB
#define MAC1625_IOUT_M  62.5  // 62.5mA/LSB

#define MAC5023_VIN_M 	MAC1625_VIN_M
#define MAC5023_VOUT_M 	MAC1625_VOUT_M
#define MAC5023_IOUT_M 	MAC1625_IOUT_M
typedef struct
{
    UINT8 dev;
    float resDiv;
    char *name;
} MAC5023_VOUT_CONFIG;

static const MAC5023_VOUT_CONFIG g_MAC5023VoutConfig[] = {
    {MAC5023_I2C_SLAVE_ADDR_P0V9_1,  1.475f, "P0V9_1"},
    {MAC5023_I2C_SLAVE_ADDR_P0V9_2,  1.475f, "P0.9_2"},
    {MAC5023_I2C_SLAVE_ADDR_P0V885,  1.475f, "P0V885"},
    {MAC5023_I2C_SLAVE_ADDR_P1V2,    2.0f,   "P1V2"},
    {MAC5023_I2C_SLAVE_ADDR_P5V,     8.5f,   "P5V"},
};

#define  MAC5023_I2C_SLAVE_ADDR  MAC5023_I2C_SLAVE_ADDR_P0V885
#else

#define MAC5023_VIN_M   31.25 // 31.25mV /LSB
#define MAC5023_VOUT_M  31.25  // 31.25mV /LSB
#define MAC5023_IOUT_M  62.5 // 62.5mA/LSB    

#define  MAC5023_I2C_SLAVE_ADDR  0x30

#endif

#define MAC5023_V_I_MASK 0x3FF
#define MAC5023_TEMP_M 1

static const char *MAC1625_MODE_NAME = "P5468Q";
static const char *MAC5023_MODE_NAME = "XXXXXX";

static bool g_MAC5023_initSuccess[ARRARY_SIZE(g_MAC5023VoutConfig)] = {false};

static float MAC5023_getResDiv(UINT8 dev)
{
	for (UINT8 i = 0; i < ARRARY_SIZE(g_MAC5023VoutConfig); i++)
    {
        if(g_MAC5023VoutConfig[i].dev == dev) {
            return g_MAC5023VoutConfig[i].resDiv;
        }
    }
    return 1;
}
static char * MAC5023_getName(UINT8 dev)
{
	for (UINT8 i = 0; i < ARRARY_SIZE(g_MAC5023VoutConfig); i++)
    {
        if(g_MAC5023VoutConfig[i].dev == dev) {
            return g_MAC5023VoutConfig[i].name;
        }
    }
    return "NULL";
}
bool MAC5023_init(UINT8 bus, UINT8 slaveAddr)
{
    char modeStr[10];

    memset(modeStr, 0, sizeof(modeStr));
	modeStr[0] = strlen(MAC1625_MODE_NAME) + 1;
    int ret = PMBus_I2CRead(bus, slaveAddr, PMBUS_MFR_MODEL, (u8 *)modeStr);
    if (ret > 0)
    {
        if (strcmp(modeStr + 1, MAC1625_MODE_NAME) == 0)
        {
            return true;
        }
        else
        {
            LOG_I("MAC5023_init error, mode %s is not support\n", MAC5023_getName(slaveAddr));
        }
    }
    else
    {
        LOG_I("MAC5023_init read bus error\n");
    }
    return false;
}

float MAC5023_vin_read(UINT8 bus, UINT8 slaveAddr)
{
    u8 vinBuff[4];
    float mV = 0;
    memset(vinBuff, 0, sizeof(vinBuff));
    int ret = PMBus_I2CRead(bus, slaveAddr, PMBUS_READ_VIN, vinBuff);
    if (ret > 0)
    {
        u16 rawAdc = ((vinBuff[1] << 8) | vinBuff[0]) & MAC5023_V_I_MASK;
        mV = rawAdc * MAC5023_VIN_M;
    }
    return mV;
}

float MAC5023_vout_read(UINT8 bus, UINT8 slaveAddr)
{ 
	u16 rawAdc;
	u16 divedAdc;   
    float mV = 0; 
    u8 voutBuff[4];
    memset(voutBuff, 0, sizeof(voutBuff));
    int ret = PMBus_I2CRead(bus, slaveAddr, PMBUS_READ_VOUT, voutBuff);
    if (ret > 0)
    {
        rawAdc = ((voutBuff[1] << 8) | voutBuff[0]) & MAC5023_V_I_MASK;
        divedAdc = rawAdc * MAC5023_VOUT_M;
        mV = MAC5023_getResDiv(slaveAddr) * divedAdc;
    }
    return mV;
}

float MAC5023_iout_read(UINT8 bus, UINT8 slaveAddr)
{
    u8 ioutBuff[4];
    float mA = 0;
    memset(ioutBuff, 0, sizeof(ioutBuff));
    int ret = PMBus_I2CRead(bus, slaveAddr, PMBUS_READ_IOUT, ioutBuff);
    if (ret > 0)
    {
        u16 rawAdc = ((ioutBuff[1] << 8) | ioutBuff[0]) & MAC5023_V_I_MASK;
        mA = rawAdc * MAC5023_IOUT_M;
    }
    return mA;
}

void MAC5023_Sample(void)
{
    for (UINT8 i = 0; i < ARRARY_SIZE(g_MAC5023VoutConfig); i++)
    {
        UINT8 slaveAddr = g_MAC5023VoutConfig[i].dev;
        if (g_MAC5023_initSuccess[i] == false){
            if (!MAC5023_init(MAC5023_I2C_BUS, slaveAddr))
            {
                g_MAC5023_initSuccess[i] = false;
                continue;
            }else{
                g_MAC5023_initSuccess[i] = true;
            }
        }

        float vin = (MAC5023_vin_read(MAC5023_I2C_BUS, slaveAddr)) / 1000;
        float vout = (MAC5023_vout_read(MAC5023_I2C_BUS, slaveAddr)) / 1000;
        float iout = (MAC5023_iout_read(MAC5023_I2C_BUS, slaveAddr)) / 1000;

        LOG_D("MAC5023_Sample name=%8s, addr7=%#x, vin=%.2f, vout=%.2f, iout=%.2f\n",
            g_MAC5023VoutConfig[i].name, slaveAddr>>1, vin, vout, iout);
    }
}
