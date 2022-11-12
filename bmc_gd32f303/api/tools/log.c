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

static void parse_arguments(int argc, char **argv);

static int setLogLevelHandler(int argc, char **argv, int index);
static int getLogLevelHandler(int argc, char **argv, int index);


static int parameterChecked(int para1, int para2, int para3, int para4);

static int (*handlerList[])(int, char **, int) =  {
    setLogLevelHandler,
    getLogLevelHandler,
    NULL
};

static char *arglist[] = {
    "-set",
    "-get",
    NULL
};

static int logs(int argc, char *argv[])
{
    int retval = 0;

    /* Read and interpret the arguments */
    parse_arguments(argc, argv);
	return 0;
}

static void display_usage(void)
{
    LOG_RAW("log\r\n");
    LOG_RAW("Usage: log <arguments>\r\n");
    LOG_RAW("Arguments:\r\n");

    LOG_RAW("\n*** log Functions ***\r\n");
    LOG_RAW("\t-get: get the current log print level\r\n");
    LOG_RAW("\t-set <level>:set the current log print level \r\n");
    LOG_RAW("\t\t level %d : DBG_ERROR  \r\n", DBG_ERROR);
    LOG_RAW("\t\t level %d : DBG_WARNING  \r\n", DBG_WARNING);
    LOG_RAW("\t\t level %d : DBG_INFO  \r\n", DBG_INFO);
    LOG_RAW("\t\t level %d : DBG_LOG  \r\n", DBG_LOG);
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

static int setLogLevelHandler(int argc, char **argv, int index)
{
    int newLevel = atoi(argv[argc - 1]);
    if ((newLevel < 0 )|| (newLevel > DBG_LOG )) {
        LOG_RAW("log level set error, old level is %d\r\n", g_debugLevel);
    } else{
        g_debugLevel = newLevel;
        LOG_RAW("log level set success, new level is %d\r\n", g_debugLevel);
    }
    return 0;
}

static int getLogLevelHandler(int argc, char **argv, int index)
{
    LOG_RAW("Current log level : %d\r\n", g_debugLevel);
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, log, logs, log level);
