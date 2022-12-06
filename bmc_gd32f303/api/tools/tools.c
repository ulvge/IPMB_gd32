/**
 * @file shell.c
 * @author Letter (NevermindZZT@gmail.com)
 * @version 3.0.0
 * @date 2019-12-30
 * 
 * @copyright (c) 2020 Letter
 * 
 */

#include "tools.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "shell_ext.h"
#include "shell_port.h"
#include "shell_port.h"
#include "project_select.h"
#include "debug_print.h"
#include "main.h"
#include "bsp_i2c.h"  
#include "adc/api_adc.h"
#include "utc/api_utc.h"
#include "stdlib.h"  
#include "OSPort.h"
#include "jump.h"

static int8_t operation_mode = -1;
static uint8_t g_bus = 0;
static uint8_t host_addr;

//truct netif g_mynetif;

static void parse_arguments(int argc, char **argv);

static int busArgHandler(int argc, char **argv, int index);
static int scanArgHandler(int argc, char **argv, int index);
static int setHostArgHandler(int argc, char **argv, int index);
static int getHostArgHandler(int argc, char **argv, int index);
static int resetArgHandler(int argc, char **argv, int index);

static int do_reset(uint8_t bus);
static int do_scan(uint8_t bus);
static int do_set_host(uint8_t bus);
static int do_get_host(uint8_t bus);

static int (*const handlerList[])(int, char **, int) =  {
        busArgHandler,
        setHostArgHandler,
        getHostArgHandler,
        resetArgHandler,
        scanArgHandler,
        NULL
};

static char *const arglist[] ={
        "-b",
        "--sethost",
        "--gethost",
        "--reset",
        "--scan",
        NULL
};

int i2cTest(int argc, char *argv[])
{
    int retval = 0;

    /* Read and interpret the arguments */
    parse_arguments(argc, argv);

    if (operation_mode == SCAN_MODE)
    {
        retval = do_scan(g_bus);
    }

    if (retval < 0)
        return false;
    else
        return true;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, i2cTest, i2cTest, i2c test);

static void display_usage(void)
{
    LOG_RAW("i2c-test\r\n");
    LOG_RAW("Usage: i2c-test <arguments>\r\n");
    LOG_RAW("Arguments:\r\n");

    LOG_RAW("\n*** I2C Functions ***\r\n");
    LOG_RAW("\t-b <bus number>: Set the bus number for this transaction.  Defaults to 0\r\n");
    LOG_RAW("\t--sethost <addr>:\tSet the host slave address\r\n");
    LOG_RAW("\t--gethost:\tGet the current host slave address for the specified bus\r\n");
    LOG_RAW("\t--reset:\tReset the I2C controller\r\n");
    LOG_RAW("\t--scan:\t\tScan the I2C bus and show the slave addresses\r\n");
    LOG_RAW("\t\t\tcommand line.\r\n");

    return;
}

static void parse_arguments(int argc, char **argv)
{
    int i, j;

    if (argc <= 1)
    {
        display_usage();
        return;
    }

    for (i = 1; i < argc; i++)
    {
        j = 0;
        while (arglist[j] != NULL)
        {
            if (strcmp(argv[i], arglist[j]) == 0)
            {
                int retval;

                /* Match!  Handle this argument (and skip the specified
                   number of arguments that were just handled) */
                retval = handlerList[j](argc, argv, i);
                if (retval >= 0)
                    i += retval;
                else
                {
                    LOG_E("Cannot handle argument: %s\r\n", arglist[j]);
                }
            }
            j++;
        }
    }
}

static int busArgHandler(int argc, char **argv, int index)
{
    if (index + 1 <= argc)
        g_bus = (uint8_t)strtol(argv[index + 1], NULL, 10);
    else
    {
        LOG_E("Missing argument to -b\r\n");
        return -1;
    }

    return (1);
}

static int getHostArgHandler(int argc, char **argv, int index)
{
    return do_get_host(atoi(argv[argc - 1]));
}

static int setHostArgHandler(int argc, char **argv, int index)
{
    if (index + 1 <= argc)
        host_addr = (uint8_t)strtol(argv[index + 1], NULL, 16);
    else
    {
        LOG_E("Missing argument to --host\r\n");
        return -1;
    }

    if (host_addr == (uint8_t)0)
    {
        LOG_E("Host address 0x00 is not valid.\r\n");
        return -1;
    }

    return do_set_host(atoi(argv[argc - 1]));
}

static int resetArgHandler(int argc, char **argv, int index)
{
    return do_reset(atoi(argv[argc - 1]));
}

static int scanArgHandler(int argc, char **argv, int index)
{
    operation_mode = SCAN_MODE;
    return (0);
}

static int do_reset(uint8_t bus)
{
    switch (bus)
    {
    case I2C_BUS_0:
        i2c_deinit(I2C0);
        i2c_channel_init(I2C0);
        LOG_I("I2C0 reset");
        break;
    case I2C_BUS_1:
        i2c_deinit(I2C1);
        i2c_channel_init(I2C1);
        LOG_I("I2C1 reset");
        break;
	#ifdef I2C2
    case I2C_BUS_2:
        i2c_deinit(I2C2);
        i2c_channel_init(I2C2);
        LOG_I("I2C2 reset");
        break;
	#endif
    case I2C_BUS_S0:
        i2c_channel_init(I2C_BUS_S0);
        LOG_I("I2C_BUS_S0 reset");
        break;
    default:
        break;
    }
    return 0;
}

static int do_scan(uint8_t bus)
{
   // char i2c_dev[1] = {0};
  //  char i2c_saveAddr[4] = {0};
    uint8_t write_buffer[2] = {0};

    int retval, k;
    int j = (uint8_t)0;
    uint8_t valid_slaves[Max_SALVES];
    LOG_RAW("Scanning the I2C Bus...\n\r");

    // Valid Address 7-bit Range
    for (k = 0x00; k <= 0x7F; k += 1)
    {
        write_buffer[0] = k<<1;
        retval = (int)i2c_write(bus, write_buffer, 1);

        if ((k % 16) == 0) {
            LOG_RAW("\n 0x%d0\t", k / 16);
            vTaskDelay(20);
		}
        if (retval > 0)
        {
            LOG_RAW("% 2s", "X");

            if (j < Max_SALVES)
            {
                valid_slaves[j] = (uint8_t)k;
                j += (uint8_t)1;
            }
        }
        else
        {
            LOG_RAW("% 2s", ".");
        }
    }

    LOG_RAW("\nDone!  Found %i valid slave address(es)\n\r", (int)j);
    if (j != 0) {
		LOG_RAW("Slave list 7bit, \t 8bit:\n\r");
	}
    /*@-usedef@*/
    for (k = 0; k < (int)j; k += 1)
        LOG_RAW("\t0x%02x\t\t 0x%02x\n\r", valid_slaves[k], valid_slaves[k] << 1);

    return true;
}

static int do_set_host(uint8_t bus)
{
    i2c_set_slave_addr(bus, host_addr);
    LOG_I("I2C%d:         SLAVE_ADDRESS: %02x\n\r", bus, host_addr);
    return 0;
}

static int do_get_host(uint8_t bus)
{
    LOG_I("do_get_host\r\n");
    switch (bus)
    {
    case 0:
        LOG_I("I2C%d:         SLAVE_ADDRESS: %02x", bus, I2C_SADDR0(I2C0));
        break;
    case 1:
        LOG_I("I2C%d:        SLAVE_ADDRESS: %02x", bus, I2C_SADDR0(I2C1));
        break; 
#ifdef  I2C2
    case 2:
        LOG_I("I2C%d:        SLAVE_ADDRESS: %02x", bus, I2C_SADDR0(I2C2));
        break;   
#endif
    default:
        break;
    }
									
    return 0;
}

__attribute__((unused)) static UINT32 g_AppWantToUpdateKeys __attribute__((at(APP_WANTTO_UPDATE_KEYS_ADDR)));
// tool 2******************************************************
int reboot(int argc, char *argv[])
{
    if (argc == 2){
        if (strcmp(argv[1], UPDATING_CMD_SYS_BOOT) == 0)
        {
            g_AppWantToUpdateKeys = APP_WANTTO_UPDATE_KEYS;
            JumpToBootloader();
        }
    }
    NVIC_SystemReset();
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, reboot, reboot, reboot the mcu);

// tool 3******************************************************
int version(int argc, char *argv[])
{
    LOG_I("" BMC_VERSION "\r\n");

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, version, version, get firmware version);

// only support ADC ,not others ,eg : I2C 
// tool 4********list all sensors info **********************************************
int sensor(int argc, char *argv[])
{
    uint8_t sensorNum;
    float humanVal;
	int sensorSize = api_sensorGetSensorCount();
	SUB_DEVICE_MODE dev = SubDevice_GetMyMode();
    const Dev_Handler *pDev_Handler = api_getDevHandler(dev);

    for (uint8_t numIdex = 0; numIdex < sensorSize; numIdex++)
    {
        sensorNum = api_sensorGetMySensorNumByIdex(numIdex);
        char *name = pDev_Handler->sensorCfg[numIdex].sensorAlias;
        //humanVal = pDev_Handler->val[numIdex].human;
        humanVal = api_sensorGetValHuman(sensorNum);
        LOG_I("sensor :idx = %d, name = %s, channel = %d, val = %f\r\n", 
                    numIdex, name, sensorNum, humanVal);
        if ((numIdex + 1) % 5) {
            vTaskDelay(5);
        }
	}

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, sensor, sensor, get sensors info);

void Delay_NoSchedue(uint32_t clk)
{
    for (uint32_t i = 0; i < clk; i++) {
        ;
    }
}

