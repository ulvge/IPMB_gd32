
#include "IPMIConf.h"
#include "IPMIDefs.h"
#include "IPMI_Oem.h"
#include "Message.h"
#include "OSPort.h"
#include "Types.h"
#include "api_cpu.h"
#include "api_subdevices.h"
#include "bsp_i2c.h"
#include "bsp_uartcomm.h"
#include "ipmi_common.h"
#include "jump.h"
#include "project_select.h"
#include "shell.h"
#include <stdbool.h>
#include <stdio.h>

/* import bin describle     */
extern uint32_t Image$$ER_IROM1$$Base;
extern uint32_t Region$$Table$$Base;

#define IMAGE_BIN_START ((uint32_t)&Image$$ER_IROM1$$Base)
#define IMAGE_DESC ((uint32_t *)&Region$$Table$$Base)
#define IMAGE_BIN_LENGTH (*(IMAGE_DESC + 4))

/* import task handler     */
extern TaskHandle_t xHandleTaskResponseDatWrite;
extern TaskHandle_t ComTask_Handler;

/* import Queue handler     */
extern xQueueHandle RecvDatMsg_Queue;
extern xQueueHandle RecvForwardI2CDatMsg_Queue;

static UINT8 xmodeCheckSum(const UINT8 *buf, UINT16 len)
{
    UINT8 cks = 0;
    for (UINT8 i = 0; i < len; ++i) {
        cks += buf[i];
    }
    return cks;
}
MsgPkt_T i2cMsg;
static bool updateDev_readMsg(UINT32 timetOut, MsgPkt_T *i2cMsg)
{
#define PERIOD_OF_SCAN 2
    UINT32 count = 0;
    do {
        vTaskDelay(PERIOD_OF_SCAN);
        if (xQueueReceive(RecvDatMsg_Queue, i2cMsg, 0) == pdPASS) {
            return true;
        }
        if (xQueueReceive(RecvForwardI2CDatMsg_Queue, i2cMsg, 0) == pdPASS) {
            return true;
        }
        count++;
    } while ((count * PERIOD_OF_SCAN) < timetOut);
    return false;
}
static bool updateDev_xmode(SUB_DEVICE_MODE mode)
{
    uint8_t reSend;
    bool isWriteSuccess;
    uint32_t codeAddr;
    MsgPkt_T i2cReadMsg;
    uint8_t sendXmodeMsg[sizeof(XMODEM_Msg) + 1];
    XMODEM_Msg *p_sendXmodeMsg = (XMODEM_Msg *)&sendXmodeMsg[1];
    sendXmodeMsg[0] = SubDevice_modeConvertSlaveAddr(mode);
    for (uint32_t packNum = 0; packNum < ((IMAGE_BIN_LENGTH + 1) / XMODEM_PAKGE_LENGTH); packNum++) {
        codeAddr = IMAGE_BIN_START + packNum * XMODEM_PAKGE_LENGTH;
        p_sendXmodeMsg->head = XMODEM_SOH;
        p_sendXmodeMsg->pn = (packNum + 1) & 0xff;
        p_sendXmodeMsg->xpn = ~p_sendXmodeMsg->pn;
        memcpy(p_sendXmodeMsg->data, (uint8_t *)codeAddr, XMODEM_PAKGE_LENGTH);
        p_sendXmodeMsg->crc = xmodeCheckSum(p_sendXmodeMsg->data, XMODEM_PAKGE_LENGTH);
        reSend = 0;
        do {
            isWriteSuccess = i2c_write(BOOT_I2C_BUS, (uint8_t *)&sendXmodeMsg, sizeof(XMODEM_Msg));
            if (!isWriteSuccess) {
                if (reSend++ > 100) {
                    LOG_I("updateDev i2c_write failed\r\n");
                    return false;
                }
                vTaskDelay(20);
                continue;
            } else {
                if (!updateDev_readMsg(5000, &i2cReadMsg)) {
                    LOG_I("updateDev i2c_read timeout\r\n");
                    return false;
                } else { // decode boot_sendAckMsg()
                    if (i2cReadMsg.Data[0] != BOOT_I2C_SPECIAL_IDENTIFICATION) {
                        LOG_I("updateDev SPECIAL IDENTIFICATION error\r\n");
                        vTaskDelay(20);
                        continue;
                    }
                    uint8_t devAckMsg = i2cReadMsg.Data[1];
                    switch (devAckMsg) {
                    case XMODEM_NAK:
                        LOG_I("updateDev NAK\r\n");
                        vTaskDelay(20);
                        continue;
                    case XMODEM_ACK: // client update pakge success
                        break;
                    case XMODEM_CANCEL:
                        return false;
                    default:
                        vTaskDelay(20);
                        continue;
                    }
                }
            }
        } while (true);
    }
    return true;
}
static bool updateDev_isModeVersionLower(SUB_DEVICE_MODE mode, INT16U destVer)
{
    MsgPkt_T requestVersion;
    MsgPkt_T recvVer;
    IPMIMsgHdr_T *hdr = (IPMIMsgHdr_T *)&(requestVersion.Data);
    requestVersion.Channel = NM_SECONDARY_IPMB_BUS; // SubDevice_GetBus(dev); // BOOT_I2C_BUS
    hdr->ResAddr = SubDevice_modeConvertSlaveAddr(mode);
    hdr->NetFnLUN = NETFN_OEM << 2; // RAW
    hdr->ChkSum = CalculateCheckSum((INT8U *)hdr, 2);

    hdr->ReqAddr = SubDevice_GetMySlaveAddress(requestVersion.Channel);
    hdr->RqSeqLUN = 0x01;
    hdr->Cmd = CMD_BMC_INFO; // GetBMCInfo CMD_UPDATE_FIRMWARE;

    int len = sizeof(IPMIMsgHdr_T);
    requestVersion.Data[len++] = CalculateCheckSum(requestVersion.Data, len);
    requestVersion.Size = sizeof(IPMIMsgHdr_T);

    if (SendMsgAndWait(&requestVersion, &recvVer, 50) == pdFALSE) {
        return false;
    }
    GetBMCInfoRes_T *pBMCInfo = (GetBMCInfoRes_T *)(&recvVer.Data[sizeof(GetBMCInfoRes_T)]);

    INT16U modeVer = pBMCInfo->BMCFirmwareVersion;
    LOG_I("updateDev check ver. mode[%d], name[%s], modeVer[%d.%d]\r\n",
          mode, SubDevice_GetModeName(mode), (modeVer >> 8) & 0xff, modeVer & 0xff);
    return modeVer < destVer;
}

static int modever(int argc, char *argv[])
{
    INT16U destVer = GetBmcFirmwareVersion(BMC_VERSION);
    for (SUB_DEVICE_MODE mode = SUB_DEVICE_MODE_MIN; mode < SUB_DEVICE_MODE_MAX; mode++) {
        if (mode == SubDevice_GetMyMode()) {
            LOG_I("updateDev check ver. mode[%d], name[%s], modeVer[%d.%d]\r\n",
                  mode, SubDevice_GetModeName(mode), (destVer >> 8) & 0xff, destVer & 0xff);
        }
        if(!updateDev_isModeVersionLower(mode, destVer)) {
            LOG_I("updateDev check ver. mode[%d], name[%s], modeVer[failed]\r\n",
                  mode, SubDevice_GetModeName(mode));
        }
    }
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, modever, modever, print all modes version);
void updateDev_task(void *arg)
{
    uint32_t cmd = *(uint32_t *)arg;
    SUB_DEVICE_MODE cmdUpdateModes = (SUB_DEVICE_MODE)(cmd & 0x0f);
    bool isForceUpdate = (bool)((cmd >> 8) & 0x0f);
    INT16U destVer = GetBmcFirmwareVersion(BMC_VERSION);

    if ((cmdUpdateModes > SUB_DEVICE_MODE_MAX) || (cmdUpdateModes == SubDevice_GetMyMode())) {
        vTaskDelete(NULL);
    }

    vTaskSuspend(xHandleTaskResponseDatWrite);
    vTaskSuspend(ComTask_Handler);

    if (cmdUpdateModes != SUB_DEVICE_MODE_MAX) { // update one devs
        if (isForceUpdate || updateDev_isModeVersionLower(cmdUpdateModes, destVer)) {
            if (!updateDev_xmode(cmdUpdateModes)) {
                LOG_W("updateDev mode[%d], name[%s] failed!!\r\n", cmdUpdateModes, SubDevice_GetModeName(cmdUpdateModes));
            }
        }
    } else { // update all devs, Except for myself
        for (SUB_DEVICE_MODE mode = SUB_DEVICE_MODE_MIN; mode < SUB_DEVICE_MODE_MAX; mode++) {
            if (mode == SubDevice_GetMyMode()) {
                continue;
            }
            if (isForceUpdate || updateDev_isModeVersionLower(cmdUpdateModes, destVer)) {
                LOG_W("updateDev mode[%d], name[%s] start...\r\n", mode, SubDevice_GetModeName(mode));
                if (!updateDev_xmode(mode)) {
                    LOG_W("updateDev mode[%d], name[%s] failed!!!\r\n", mode, SubDevice_GetModeName(mode));
                }
            }
        }
    }
    vTaskDelay(500);
    vTaskResume(xHandleTaskResponseDatWrite);
    vTaskResume(ComTask_Handler);
    vTaskDelete(NULL);
}
