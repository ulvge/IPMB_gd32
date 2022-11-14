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
#define MAC5023_I2C_SLAVE_ADDR 0x7C

#define MAC1625_VIN_M 0.025    // 25mV /LSB
#define MAC1625_VOUT_M 0.00125 // 1.25mV /LSB
#define MAC1625_IOUT_M 0.0625  // 62.5mA/LSB

#define MAC5023_VIN_M 0.03125 // 31.25mV /LSB
#define MAC5023_VOUT_M 03125  // 31.25mV /LSB
#define MAC5023_IOUT_M 0.0625 // 62.5mA/LSB

#define MAC5023_TEMP_M 1

static const char *MAC5023_MODE_NAME = "MAC5023";

static bool g_MAC5023_initSuccess = false;
bool MAC5023_init(void)
{
    char modeStr[10];

    g_MAC5023_initSuccess = false;
    memset(modeStr, 0, sizeof(modeStr));
    int ret = PMBus_I2CRead(MAC5023_I2C_BUS, MAC5023_I2C_SLAVE_ADDR, PMBUS_MFR_MODEL, (u8 *)modeStr);
    if (ret > 0)
    {
        if (strcmp(modeStr, MAC5023_MODE_NAME) == 0)
        {
            g_MAC5023_initSuccess = true;
        }
        else
        {
            LOG_I("MAC5023_init error, mode %s is not support\n", modeStr);
        }
    }
    else
    {
        LOG_I("MAC5023_init read bus error\n");
    }
    return g_MAC5023_initSuccess;
}
u8 MAC5023_vout_read(void)
{
    u8 ipmbVal = 0;
    u8 voutBuff[4];
    int ret = PMBus_I2CRead(MAC5023_I2C_BUS, MAC5023_I2C_SLAVE_ADDR, PMBUS_READ_VOUT, voutBuff);
    if (ret > 0)
    {
        u8 rawAdc = (voutBuff[1] << 8) | voutBuff[0];
        ipmbVal = rawAdc * 10 / MAC5023_VOUT_M;
    }
    return ipmbVal;
}

u8 MAC5023_iout_read(void)
{
    u8 ipmbVal = 0;
    u8 ioutBuff[4];
    int ret = PMBus_I2CRead(MAC5023_I2C_BUS, MAC5023_I2C_SLAVE_ADDR, PMBUS_READ_IOUT, ioutBuff);
    if (ret > 0)
    {
        u8 rawAdc = (ioutBuff[1] << 8) | ioutBuff[0];
        ipmbVal = rawAdc * 10 / MAC5023_IOUT_M;
    }
    return ipmbVal;
}

void MAC5023_Sample(void)
{
    if (g_MAC5023_initSuccess == false)
    {
        if (!MAC5023_init())
        {
            return;
        }
    }

    u8 vout = MAC5023_vout_read();
    u8 iout = MAC5023_iout_read();

    LOG_D("MAC5023_Sample, vout=%d, iout=%d\n", vout, iout);
}
