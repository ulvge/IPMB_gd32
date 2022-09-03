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
#include "utc/api_utc.h"
//#include "stdlib.h"

static int operation_mode = -1;
__attribute__((unused)) static int bus = 0;
static uint8_t host_addr;

extern int8_t g_temperature_raw[4];

unsigned char MAC_ADDR0;
unsigned char MAC_ADDR1;
unsigned char MAC_ADDR2;
unsigned char MAC_ADDR3;
unsigned char MAC_ADDR4;
unsigned char MAC_ADDR5;

unsigned char GW_ADDR0 = 192;
unsigned char GW_ADDR1 = 168;
unsigned char GW_ADDR2 = 2;
unsigned char GW_ADDR3 = 10;

unsigned char IP_ADDR0 = 192;
unsigned char IP_ADDR1 = 168;
unsigned char IP_ADDR2 = 2;
unsigned char IP_ADDR3 = 111;

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

static int parameterChecked(int para1, int para2, int para3, int para4);

static int (*handlerList[])(int, char **, int) =
    {
        busArgHandler,
        setHostArgHandler,
        getHostArgHandler,
        resetArgHandler,
        scanArgHandler,
        NULL};

static char *arglist[] =
    {
        "-b",
        "--sethost",
        "--gethost",
        "--reset",
        "--scan",
        NULL};

int i2cTest(int argc, char *argv[])
{
    int retval = 0;

    /* Read and interpret the arguments */
    parse_arguments(argc, argv);

    switch (operation_mode)
    {
    case RESET_MODE:
        retval = do_reset(atoi(argv[argc - 1]));
        break;
    case SCAN_MODE:
        retval = do_scan(atoi(argv[argc - 1]));
        break;

    case SETH_MODE:
        retval = do_set_host(atoi(argv[argc - 1]));
        break;

    case GETH_MODE:
        retval = do_get_host(atoi(argv[argc - 1]));
        break;
    default:
        printf("Unknown operation mode: %d\r\n", operation_mode);
        return false;
    }

    if (retval < 0)
        return false;
    else
        return true;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, i2cTest, i2cTest, i2c test);

static void display_usage(void)
{
    printf("i2c-test\r\n");
    printf("Usage: i2c-test <arguments>\r\n");
    printf("Arguments:\r\n");

    printf("\n*** I2C Functions ***\r\n");
    printf("\t-b <bus number>: Set the bus number for this transaction.  Defaults to 0\r\n");
    printf("\t--sethost <addr>:\tSet the host slave address\r\n");
    printf("\t--gethost:\tGet the current host slave address for the specified bus\r\n");
    printf("\t--reset:\tReset the I2C controller\r\n");
    printf("\t--scan:\t\tScan the I2C bus and show the slave addresses\r\n");
    printf("\t\t\tcommand line.\r\n");

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
                    printf("Cannot handle argument: %s\r\n", arglist[j]);
                }
            }
            j++;
        }
    }
}

static int busArgHandler(int argc, char **argv, int index)
{
    if (index + 1 <= argc)
        bus = (uint8_t)strtol(argv[index + 1], NULL, 10);
    else
    {
        printf("Missing argument to -b\r\n");
        return -1;
    }

    return (1);
}

static int getHostArgHandler(int argc, char **argv, int index)
{
    operation_mode = GETH_MODE;

    return (0);
}

static int setHostArgHandler(int argc, char **argv, int index)
{
    if (index + 1 <= argc)
        host_addr = (uint8_t)strtol(argv[index + 1], NULL, 16);
    else
    {
        printf("Missing argument to --host\r\n");
        return -1;
    }

    if (host_addr == (uint8_t)0)
    {
        printf("Host address 0x00 is not valid.\r\n");
        return -1;
    }

    operation_mode = SETH_MODE;

    return (1);
}

static int resetArgHandler(int argc, char **argv, int index)
{
    operation_mode = RESET_MODE;
    return (0);
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
    case 0:
        i2c_deinit(I2C0);
        i2c_channel_init(I2C0);
        LOG_RAW("I2C0 reset");
        break;
    case 1:
        i2c_deinit(I2C1);
        i2c_channel_init(I2C1);
        LOG_RAW("I2C1 reset");
        break;
	#ifdef I2C2
    case 2:
        i2c_deinit(I2C2);
        i2c_channel_init(I2C2);
        LOG_RAW("I2C2 reset");
        break;
	#endif
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
    printf("Scanning the I2C Bus...this may take a while...\n\r");

    // Valid Address 7-bit Range
    for (k = 0x00; k <= 0x7F; k += 1)
    {
        retval = (int)i2c0_bytes_write(write_buffer, 0);

        if (retval > 0)
        {
            printf("X");
            (void)fflush(stdout);

            if (j < Max_SALVES)
            {
                valid_slaves[j] = (uint8_t)k;
                j += (uint8_t)1;
            }
        }
        else if (0 == retval)
        {
            printf("\nHealth monitoring is suspended\n\r");
            return true;
        }
        else
        {
            printf(".");
            (void)fflush(stdout);
        }

        if ((k % 86) == 0)
            printf("\n");
    }

    printf("\nDone!  Found %i valid slave address(es)\n\r", (int)j);
    printf("Slave list:\n\r");

    /*@-usedef@*/
    for (k = 0; k < (int)j; k += 1)
        printf("0x%02x\n\r", (unsigned int)valid_slaves[k] << 1);

    return true;
}

static int do_set_host(uint8_t bus)
{
    switch (bus)
    {
    case 0:
        i2c0_set_as_slave_device_addr(host_addr);
        LOG_RAW("I2C%d:         SLAVE_ADDRESS: %02x\n\r", bus, host_addr);
        break;
    case 1:
        i2c1_set_as_slave_device_addr(host_addr);
        LOG_RAW("I2C%d:       SLAVE_ADDRESS: %02x\n\r", bus, I2C1_SLAVE_ADDRESS7);
        break;
#ifdef  I2C2
    case 2:
        i2c2_set_as_slave_device_addr(host_addr);
        LOG_RAW("I2C%d:        SLAVE_ADDRESS: %02x\n\r", bus, host_addr);
        break;
#endif
    default:
        break;
    }
    return 0;
}

static int do_get_host(uint8_t bus)
{
    printf("do_get_host\r\n");
    switch (bus)
    {
    case 0:
        LOG_I("I2C%d:         SLAVE_ADDRESS: %02x", bus, get_device_addr(bus));
        break;
    case 1:
        LOG_I("I2C%d:        SLAVE_ADDRESS: %02x", bus, get_device_addr(bus));
        break;
    case 2:
        LOG_I("I2C%d:        SLAVE_ADDRESS: %02x", bus, get_device_addr(bus));
        break;
    default:
        break;
    }

    return 0;
}

// tool 2******************************************************
int reboot(int argc, char *argv[])
{
    NVIC_SystemReset();
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, reboot, reboot, reboot the mcu);

// tool 3******************************************************
int version(int argc, char *argv[])
{
    LOG_RAW("" BMC_VERSION "\r\n");

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, version, version, get firmware version);

// tool 4********list all sensors info **********************************************
int sensor(int argc, char *argv[])
{
    char buf[16 * 3] = {0};
    int SensorNum = 4;
    for (int i = 0; i < SensorNum; i++)
    {
        printf("channel %d\n\r", i);
        sprintf(buf, "sensor :%d", g_temperature_raw[i]);
        LOG_I("%d", buf);
    }

    /*
		char reading = 8;
		char unit[] = "��";
	
		sprintf(buf, "%s", name);
		sprintf(buf+16, "%d", reading);
		sprintf(buf+32, "%s", unit);

        */
    //LOG_RAW("%s", buf);

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, sensor, sensor, get sensors info);

// tool5***1.ifconfig  list all eth ip info *************************************
//      ***2.configure ip and gateway address: ifconfig ************************
int ifconfig(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
//    int index = argc;
//    switch (index)
//    {
//    case 1:

//        /******if ip configure success that it echo  list all ip info,otherwise, no echo****************************
//         * *******example: ifconfig   *******************************
//        */
//        if (g_net_init_flag == true)
//        {
//            char buf[ETH_MAX_BYTES] = {0};
//            // MAC address
//            sprintf(buf, "HwADDR:             	     %02x:%02x:%02x:%2x:%02x:%02x", MAC_ADDR0, MAC_ADDR1, MAC_ADDR2, MAC_ADDR3, MAC_ADDR4, MAC_ADDR5);
//            LOG_RAW("%s\n\r", buf);
//            //static IP address
//            sprintf(buf, "Inet addr：                   %d.%d.%d.%d", IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
//            LOG_RAW("%s\n\r", buf);
//            //remote IP address
//            //sprintf(buf, "Remote inet addr：            %d.%d.%d.%d", IP_S_ADDR0, IP_S_ADDR1, IP_S_ADDR2, IP_S_ADDR3);
//           // LOG_RAW("%s\n\r", buf);
//            //net mask
//            sprintf(buf, "Mask：                        %d.%d.%d.%d", NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
//            LOG_RAW("%s\n\r", buf);
//            //gateway address
//            sprintf(buf, "Gateway address：             %d.%d.%d.%d", GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
//            LOG_RAW("%s", buf);
//        }
//        else
//        {
//            LOG_RAW("Link no detected \n\r");
//        }
//        break;
//    case 2:
//        /*******configure PHY ip addr:*************************************
//         * ****example:  ifconfig  192.168.2.100 **********************/

//        if (("-G" != argv[argc - 1]) || ("-g" != argv[argc - 1]))
//        {
//            char *ipstr = argv[argc - 1];
//            int num = 0;
//            char *p[4] = {0};
//            num = split(ipstr, ".", p);
//            if (num == 4)
//            {
//                if (-1 == parameterChecked(atoi(p[0]), atoi(p[1]), atoi(p[2]), atoi(p[3])))
//                {
//                    LOG_RAW("input IP error!");
//                }
//                else
//                {
//                    IP_ADDR0 = atoi(p[0]);
//                    IP_ADDR1 = atoi(p[1]);
//                    IP_ADDR2 = atoi(p[2]);
//                    IP_ADDR3 = atoi(p[3]);

//                    /* initilaize the LwIP stack */
//                    if (enet_software_init())
//                    {
//                        g_net_init_flag = true;
//                        /* initilaize the LwIP IP */
//                        lwip_ip_init();
//                    }
//                    LOG_RAW("rebuild ip addr: %d.%d.%d.%d\r\n", IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
//                }
//            }
//            else
//            {
//                LOG_E("input  ip  parameter error!");
//            }
//        }
//        else
//        {
//            LOG_RAW("configure ip addr parameter input error!");
//        }

//        break;

//    case 3:
//        /*******configure gateway addr:*************************************
//         * ****example:  ifconfig  -g (-G)  192.168.2.100 **********************/

//        if (("-G" != argv[argc - 2]) || ("-g" != argv[argc - 2]))
//        {
//            char *gatewaystr = argv[argc - 1];
//            int num = 0;
//            char *p[4] = {0};
//            num = split(gatewaystr, ".", p);
//            if (num == 4)
//            {
//							
//                if (-1 == parameterChecked(atoi(p[0]), atoi(p[1]), atoi(p[2]), atoi(p[3])))
//                {
//                    LOG_RAW("input IP error!");
//                }
//                else
//                {
//                    GW_ADDR0 = atoi(p[0]);
//                    GW_ADDR1 = atoi(p[1]);
//                    GW_ADDR2 = atoi(p[2]);
//                    GW_ADDR3 = atoi(p[3]);

//                    /* initilaize the LwIP stack */
//                    if (enet_software_init())
//                    {
//                        g_net_init_flag = true;
//                        /* initilaize the LwIP IP */
//                        lwip_ip_init();
//                    }
//                    printf("rebuild gateway addr: %d.%d.%d.%d\r\n", GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
//                }
//            }
//            else
//            {
//                LOG_E("gateway  error!");
//            }
//        }
//        else
//        {
//            LOG_RAW("configure gateway parameter input error!");
//        }
//        break;

//    default:
//        break;
//    }
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, ifconfig, ifconfig, get all eth ip info);

int shell_exit(int argc, char *argv[])
{

    NVIC_SystemReset();
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, exit, shell_exit, exit shell);

__attribute__((unused)) static int parameterChecked(int para1, int para2, int para3, int para4)
{
	if(((para1>=0) && (para1<=255)) && ((para2>=0) && (para2<=255)) && ((para3>=0) && (para3<=255)) && ((para4>=0) && (para4<=255)))
  {
     return 0;
  }
  else
  {
      return -1;
  }
}


