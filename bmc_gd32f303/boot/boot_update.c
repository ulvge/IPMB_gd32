
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
#include "IPMIConf.h"

typedef void (*pFunction)(void);
static UPDATE_SM boot_ProcessUpdateReq(const BootPkt_T *pReq);
static UINT32 g_updateChannleBak = SERIAL_CHANNEL_TYPE;
bool g_xmodemIsCheckTpyeCrc = false;

xQueueHandle updateDatMsg_Queue = NULL;
UPDATE_SM g_UpdatingSM = UPDATE_SM_INIT;
volatile UINT32 g_resendCount = 0;

void boot_updateTask(void *arg)
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
    UINT32 startPageNum = (ADDRESS_APP_START - FLASH_BASE + 1) / FMC_PAGE_SIZE;
    UINT32 erasePageNum = (ADDRESS_APP_END - ADDRESS_APP_START + 1) / FMC_PAGE_SIZE;

    erase_page(startPageNum, erasePageNum);
    return true;
}

static UPDATE_SM boot_ProcessUpdateReq(const BootPkt_T *pReq)
{
    XMODEM_Msg *msg = (XMODEM_Msg *)(&pReq->Data);
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
                if (!isCrcOK) { // Currently, the client does not know which transport protocol the master supports
                    break;
                } else {
                    g_xmodemIsCheckTpyeCrc = !g_xmodemIsCheckTpyeCrc;
                }
            }
            if (!isCrcOK) {
                boot_sendMsg2Dev(XMODEM_NAK);
                return UPDATE_SM_ERROR_TRYAGAIN;
            }
            if (g_UpdatingSM == UPDATE_SM_START) {
                g_updateChannleBak = pReq->Channel; // It will only be recorded once and will not be changed in the future
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
            startAddr = ADDRESS_APP_START + (pnPage + msg->pn - 1) * XMODEM_PAKGE_LENGTH;

            LOG_D("update addr = %#X, page Num = %d \r\n", startAddr, pnPage + msg->pn);
            if ((startAddr + XMODEM_PAKGE_LENGTH) > ADDRESS_APP_END) {
                boot_sendMsg2Dev(XMODEM_NAK);
                return UPDATE_SM_ERROR_TRYAGAIN;
            }
            boot_sendMsg2Dev(XMODEM_ACK);
            FLASH_Program(startAddr, (uint32_t *)(&msg->data), sizeof(msg->data));
            return UPDATE_SM_PROGRAMING;
        case XMODEM_EOT:
            boot_sendMsg2Dev(XMODEM_ACK);
            pnPage = 0;
            return UPDATE_SM_FINISHED;
        case XMODEM_CANCEL:
        case 'q': // quit
        case 'Q':
            boot_sendMsg2Dev(XMODEM_ACK);
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
void boot_sendMsg2Dev(UINT8 msg)
{
    if(g_updateChannleBak == SERIAL_CHANNEL_TYPE){
        usart_data_transmit(USART0, msg); while (RESET == usart_flag_get(USART0, USART_FLAG_TBE)) ;
    } else {// NM_PRIMARY_IPMB_BUS || NM_SECONDARY_IPMB_BUS
        uint8_t sendbuff[10];
        uint8_t idx = 0;
        sendbuff[idx++] = SubDevice_GetMySlaveAddress(BOOT_I2C_BUS);
        sendbuff[idx++] = BOOT_I2C_SPECIAL_IDENTIFICATION;
        sendbuff[idx++] = msg;
        i2c_write(BOOT_I2C_BUS, sendbuff, idx);
    }
}

