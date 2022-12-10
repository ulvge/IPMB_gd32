
#include "Message.h"
#include "OSPort.h"
#include "Types.h"
#include "bsp_uartcomm.h"
#include "jump.h"
#include "project_select.h"
#include <stdbool.h>
#include <stdio.h>
#include "bsp_i2c.h"
#include "api_subdevices.h"

void updateDev_task(void *arg)
{
    uint32_t cmd = *(uint32_t *)arg;
    SUB_DEVICE_MODE cmdUpdateModes = (SUB_DEVICE_MODE)cmd;

    if (cmdUpdateModes > SUB_DEVICE_MODE_MAX) {
        vTaskDelete(NULL);
    }
    while (true)
    {
        vTaskDelay(500);
        if (cmdUpdateModes == SUB_DEVICE_MODE_MAX) { // update all devs, Except for myself
            LOG_W("updateDev_task is running....\r\n");
        }
    }
}



