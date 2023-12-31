
#include "boot_update.h"
#include "Message.h"
#include "OSPort.h"
#include "Types.h"
#include "bsp_uartcomm.h"
#include "flash.h"
#include "jump.h"
#include "project_select.h"
#include <stdbool.h>
#include <stdio.h>
#include "bsp_i2c.h"

xQueueHandle RecvDatMsg_Queue  = NULL;
xQueueHandle RecvForwardI2CDatMsg_Queue = NULL;


uint8_t SubDevice_GetMySlaveAddress(uint32_t bus)
{
    return update_BkpDateRead(I2C_UPDATE_MODE_ADDR) & 0xff;
}
uint8_t SubDevice_GetDestAddress(void)
{
    return (update_BkpDateRead(I2C_UPDATE_MODE_ADDR) >> 8) & 0xff;
}

static void boot_I2cForwardTask1(void *arg)
{
    RecvDatMsg_Queue = xQueueCreate(3, sizeof(MsgPkt_T));
    MsgPkt_T i2cMsg;
    BootPkt_T forwardMsg;
    BaseType_t err;
    while (true)
    {
        xQueueReceive(RecvDatMsg_Queue, &i2cMsg, portMAX_DELAY);
        forwardMsg.Channel = i2cMsg.Channel;
        forwardMsg.Size = i2cMsg.Size;
        memcpy(forwardMsg.Data, i2cMsg.Data + 1, forwardMsg.Size - 1);
        do {
            if (xQueueSend(updateDatMsg_Queue, &forwardMsg, 50) == pdFALSE) {
                LOG_E("boot_I2cForwardTask1 Forward msg failed : %d \r\n", err);
            }
        } while (err == pdFALSE);
    }
}
static void boot_I2cForwardTask2(void *arg)
{
    RecvForwardI2CDatMsg_Queue  = xQueueCreate(3, sizeof(MsgPkt_T));
    MsgPkt_T i2cMsg;
    BootPkt_T forwardMsg;
    BaseType_t err;
    while (true)
    {
        xQueueReceive(RecvForwardI2CDatMsg_Queue, &i2cMsg, portMAX_DELAY);
        forwardMsg.Channel = i2cMsg.Channel;
        forwardMsg.Size = i2cMsg.Size;
        memcpy(forwardMsg.Data, i2cMsg.Data + 1, forwardMsg.Size - 1);
        do {
            if (xQueueSend(updateDatMsg_Queue, &forwardMsg, 50) == pdFALSE) {
                LOG_E("boot_I2cForwardTask2 Forward msg failed : %d \r\n", err);
            }
        } while (err == pdFALSE);
    }
}

void boot_i2c_init(void)
{
    i2c_int();
    xTaskCreate(boot_I2cForwardTask1, "ForwardTask1", configMINIMAL_STACK_SIZE * 2, NULL, 15, NULL);
    xTaskCreate(boot_I2cForwardTask2, "ForwardTask2", configMINIMAL_STACK_SIZE * 2, NULL, 16, NULL);
}





