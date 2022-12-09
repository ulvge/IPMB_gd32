
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

#define XMODEM_PAKGE_LENGTH 128

typedef void (*pFunction)(void);
static UPDATE_SM boot_ProcessUpdateReq(const BootPkt_T *pReq);

xQueueHandle updateDatMsg_Queue = NULL;
UPDATE_SM g_UpdatingSM = UPDATE_SM_INIT;
volatile UINT32 g_resendCount = 0;
bool g_xmodemIsCheckTpyeCrc = false;

void updateTask(void *arg)
{
    BootPkt_T reqMsg;

    updateDatMsg_Queue = xQueueCreate(1, sizeof(BootPkt_T));
    if (updateDatMsg_Queue == NULL) {
        LOG_E("updateDatMsg_Queue create ERR!");
    }

    while (1) {
        xQueueReceive(updateDatMsg_Queue, &reqMsg, portMAX_DELAY);

        vTaskSuspend(updateMonitorHandle);
        g_UpdatingSM = boot_ProcessUpdateReq(&reqMsg);
        vTaskResume(updateMonitorHandle);
    }
}
static bool boot_xmodeCheck(bool type, const UINT8 *buf, UINT8 len, UINT16 recCrc)
{
    UINT8 i = 0;

    if (type == XMODEM_CHECK_CRC16) {
        UINT16 crc = 0;
        UINT16 tcrc;
        while (len--) {
            crc ^= *buf++ << 8;

            for (i = 0; i < 8; ++i) {
                if (crc & 0x8000) {
                    crc = (crc << 1) ^ 0x1021;
                } else {
                    crc = crc << 1;
                }
            }
        }
        tcrc = (recCrc << 8) | (recCrc >> 8);
        if (crc != tcrc)
        {
            return false;
        }
         return true;
    } else  if (type == XMODEM_CHECK_SUM) {
        UINT8 cks = 0;
        for (i = 0; i < len; ++i) 
        {
            cks += buf[i];
        }
        if (cks != (recCrc & 0xff))
        {
            return false;
        }
         return true;
    }
    return false;
}
static bool boot_eraseAllPage()
{
    UINT32 startPageNum = (ADDRESS_START_APP - FLASH_BASE + 1) / FMC_PAGE_SIZE;
    UINT32 erasePageNum = (ADDRESS_END_APP - ADDRESS_START_APP + 1) / FMC_PAGE_SIZE;

    erase_page(startPageNum, erasePageNum);
    return true;
}

static UPDATE_SM boot_ProcessUpdateReq(const BootPkt_T *pReq)
{
    xmodemMsg *msg = (xmodemMsg *)(&pReq->Data);
    static UINT32 pnPage = 0;
    static UINT8 lastPn = 0;
    UINT32 startAddr;
    bool isCrcOK;

    g_resendCount = 0;
    switch (msg->head) // 128*256 =
    {
        case XMODEM_SOH:
            isCrcOK = boot_xmodeCheck(g_xmodemIsCheckTpyeCrc, msg->data, sizeof(msg->data), msg->crc);
            if (!isCrcOK && (g_UpdatingSM == UPDATE_SM_START)) {
                isCrcOK = boot_xmodeCheck(!g_xmodemIsCheckTpyeCrc, msg->data, sizeof(msg->data), msg->crc);
                if (!isCrcOK) {
                    break;
                } else {
                    g_xmodemIsCheckTpyeCrc = !g_xmodemIsCheckTpyeCrc;
                }
            }
            if (!isCrcOK) {
                boot_UartSendByte(XMODEM_NAK);
                return UPDATE_SM_ERROR_TRYAGAIN;
            }
            if (g_UpdatingSM == UPDATE_SM_START) {
                boot_setPrintUartPeriph(USART1);
                boot_eraseAllPage();
            }
            if (((UINT8)(lastPn + 1)) != msg->pn){
                return UPDATE_SM_ERROR_TRYAGAIN;
            }
            lastPn = msg->pn;
            if (msg->pn == 0) {
                pnPage += 256;
            }
            startAddr = ADDRESS_START_APP + (pnPage + msg->pn - 1) * XMODEM_PAKGE_LENGTH;

            LOG_D("update addr = %#X, page Num = %d \r\n", startAddr, pnPage + msg->pn);
            if ((startAddr + XMODEM_PAKGE_LENGTH) > ADDRESS_END_APP) {
                boot_UartSendByte(XMODEM_NAK);
                return UPDATE_SM_ERROR_TRYAGAIN;
            }
            FLASH_Program(startAddr, (uint32_t *)(&msg->data), sizeof(msg->data));
            vTaskDelay(1);
            boot_UartSendByte(XMODEM_ACK);
            return UPDATE_SM_PROGRAMING;
        case XMODEM_EOT:
            boot_UartSendByte(XMODEM_ACK);
            pnPage = 0;
            return UPDATE_SM_FINISHED;
        case XMODEM_CANCEL:
        case 'q': // quit
        case 'Q':
            boot_UartSendByte(XMODEM_ACK);
            pnPage = 0;
            return UPDATE_SM_CANCEL;
        case XMODEM_CTRLC: /* abandon startup ,and prepare to upload */
        case 'u':
        case 'U':
            if (g_UpdatingSM == UPDATE_SM_INIT) {
                return UPDATE_SM_START;
            }
            if (g_UpdatingSM == UPDATE_SM_PROGRAMING) {
                return UPDATE_SM_CANCEL;
            }
            break;
        case 'a': // go to app /* give up upload,then startup */
        case 'A':
            if ((g_UpdatingSM == UPDATE_SM_INIT) || (g_UpdatingSM == UPDATE_SM_START)) {
                return UPDATE_SM_FINISHED;
            }
            break;
        default:
            break;
    }
    return g_UpdatingSM;
}
