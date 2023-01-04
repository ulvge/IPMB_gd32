
#include <stdbool.h>
#include <stdio.h>
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
#include "ChassisCtrl.h"

/* import bin describle     */
extern uint32_t Image$$ER_IROM1$$Base;
extern uint32_t Region$$Table$$Base;

#define IMAGE_BIN_START ((uint32_t)&Image$$ER_IROM1$$Base)
#define IMAGE_DESC ((uint32_t *)&Region$$Table$$Base)
#define IMAGE_BIN_LENGTH ((*(IMAGE_DESC + 4)) - IMAGE_BIN_START)

/* import task handler     */
extern TaskHandle_t xHandleUploadTask;
extern TaskHandle_t ComTask_Handler;

/* import Queue handler     */
extern xQueueHandle RecvDatMsg_Queue;
extern xQueueHandle RecvForwardI2CDatMsg_Queue;
extern xQueueHandle ResponseDatMsg_Queue;

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
static bool updateDev_notifyDevBoot(SUB_DEVICE_MODE mode)
{
    uint32_t reSend;
    MsgPkt_T requestDevBoot;
    MsgPkt_T respDevBoot;
    IPMIMsgHdr_T *hdr = (IPMIMsgHdr_T *)&(requestDevBoot.Data);
    ChassisControlRes_T *pBMCInfo;
    requestDevBoot.Channel = BOOT_I2C_BUS; // SubDevice_GetBus(dev); // 
    requestDevBoot.Param = IPMI_REQUEST;
    hdr->ResAddr = SubDevice_modeConvertSlaveAddr(mode);
    hdr->NetFnLUN = NETFN_OEM << 2;
    hdr->ChkSum = CalculateCheckSum((INT8U *)hdr, 2);

    hdr->ReqAddr = SubDevice_GetMySlaveAddress(requestDevBoot.Channel);
    hdr->RqSeqLUN = 0x01;
    hdr->Cmd = CMD_UPDATE_FIRMWARE; // UpdateFirmware

    int len = sizeof(IPMIMsgHdr_T);
    requestDevBoot.Data[len++] = SubDevice_GetMySlaveAddress(requestDevBoot.Channel);
    requestDevBoot.Data[len++] = CalculateCheckSum(requestDevBoot.Data, len);
    requestDevBoot.Size = len;

    do {
        if (SendMsgAndWait(&requestDevBoot, &respDevBoot, 50)) {
            pBMCInfo = (ChassisControlRes_T *)(&respDevBoot.Data[sizeof(IPMIMsgHdr_T)]);
            if (pBMCInfo->CompletionCode == CC_NORMAL) {
                return true;
            }
        }
        vTaskDelay(100);
    } while (reSend++ < 3);

    return false;
}
static bool updateDev_decodeMsg(UINT32 timetOut)
{
    MsgPkt_T i2cReadMsg;
    UINT32 startTime = GetTickMs();
    while ((GetTickMs() - startTime) < timetOut)
    {
        if (!updateDev_readMsg(timetOut, &i2cReadMsg)) { // timetOut
            LOG_I("updateDev i2c write ok, but read timeout\r\n");
            return false;
        } else { // decode boot_sendAckMsg()
            xmodeClientAckProtocol_T *pAck = (xmodeClientAckProtocol_T *)&i2cReadMsg.Data[0];
            if ((pAck->identification == BOOT_I2C_SPECIAL_IDENTIFICATION) && (i2cReadMsg.Size == sizeof(xmodeClientAckProtocol_T))) {
                switch (pAck->msg) {
                    case XMODEM_NAK:
                    case XMODEM_HANDSHAKECRC:
                        LOG_I("updateDev NAK\r\n");
                        vTaskDelay(20);
                        continue;
                    case XMODEM_ACK: // client update pakge success
                        return true;
                    case XMODEM_CANCEL:
                        return false;
                    default:
                        vTaskDelay(20);
                        continue;
                }
            } else {
                LOG_I("updateDev SPECIAL IDENTIFICATION error\r\n");
                vTaskDelay(20);
                continue;
            }
        }
    }
    return false;
}
static bool updateDev_xmode(SUB_DEVICE_MODE mode)
{
    uint8_t reSend;
    bool isWriteSuccess;
    uint32_t codeAddr;
    uint32_t dlyMs;
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
            if (i2c_write(BOOT_I2C_BUS, (uint8_t *)&sendXmodeMsg, sizeof(XMODEM_Msg))) {
                if (packNum == 0) {
                    dlyMs = 2000;
                } else {
                    dlyMs = 200;
                }
                if(updateDev_decodeMsg(dlyMs)){
                    break;
                }
            } else {
                if (reSend++ > 100) {
                    LOG_I("updateDev i2c_write failed\r\n");
                    return false;
                }
                vTaskDelay(20);
                continue;
            }
        } while (true);
    }
    p_sendXmodeMsg->head = XMODEM_EOT;
    reSend = 0;
    do {
        isWriteSuccess = i2c_write(BOOT_I2C_BUS, (uint8_t *)&sendXmodeMsg, 2); // 2: SlaveAddr + XMODEM_EOT
        if (isWriteSuccess) {
            return true;
        }
        vTaskDelay(20);
    } while (reSend++ < 3);
    return false;
}
static bool updateDev_isModeVersionLower(SUB_DEVICE_MODE mode, INT16U destVer)
{
    MsgPkt_T requestVersion;
    MsgPkt_T recvVer;
    IPMIMsgHdr_T *hdr = (IPMIMsgHdr_T *)&(requestVersion.Data);
    requestVersion.Channel = BOOT_I2C_BUS; // SubDevice_GetBus(dev); 
    requestVersion.Param = IPMI_REQUEST;
    hdr->ResAddr = SubDevice_modeConvertSlaveAddr(mode);
    hdr->NetFnLUN = NETFN_OEM << 2; // RAW
    hdr->ChkSum = CalculateCheckSum((INT8U *)hdr, 2);

    hdr->ReqAddr = SubDevice_GetMySlaveAddress(requestVersion.Channel);
    hdr->RqSeqLUN = 0x01;
    hdr->Cmd = CMD_BMC_INFO; // GetBMCInfo CMD_UPDATE_FIRMWARE;

    int len = sizeof(IPMIMsgHdr_T);
    requestVersion.Data[len++] = CalculateCheckSum(requestVersion.Data, len);
    requestVersion.Size = len;

    if (SendMsgAndWait(&requestVersion, &recvVer, 50) == pdFALSE) {
        LOG_I("updateDev check ver. mode[%d], name[%s], modeVer[failed]\r\n", mode, SubDevice_GetModeName(mode));
        return false;
    }
    GetBMCInfoRes_T *pBMCInfo = (GetBMCInfoRes_T *)(&recvVer.Data[sizeof(IPMIMsgHdr_T)]);

    INT16U modeVer = pBMCInfo->BMCFirmwareVersion;
    LOG_I("updateDev check ver. mode[%d], name[%s], modeVer[%d.%d]\r\n",
          mode, SubDevice_GetModeName(mode), (modeVer >> 8) & 0xff, modeVer & 0xff);
    return modeVer < destVer;
}

static bool updateDev_update(SUB_DEVICE_MODE mode)
{
    if (!updateDev_notifyDevBoot(mode)) {
        LOG_I("updateDev notifyDevBoot mode[%d], name[%s] failed!!\r\n", mode, SubDevice_GetModeName(mode));
        return false;
    }
    vTaskDelay(GPIO_ACTIVE_PULSE_TIME_MS + 200); // Wait for the mode enter to bootloader
    return updateDev_xmode(mode);
}
static int modever(int argc, char *argv[])
{
    INT16U destVer = GetBmcFirmwareVersion(BMC_VERSION);
    for (SUB_DEVICE_MODE mode = SUB_DEVICE_MODE_MIN; mode < SUB_DEVICE_MODE_MAX; mode++) {
        if (mode == SubDevice_GetMyMode()) {
            LOG_I("updateDev check ver. mode[%d], name[%s], modeVer[%d.%d]\r\n",
                  mode, SubDevice_GetModeName(mode), (destVer >> 8) & 0xff, destVer & 0xff);
			continue;
        }
        updateDev_isModeVersionLower(mode, destVer);
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

    if (SubDevice_IsSelfMaster()) {
        vTaskSuspend(xHandleUploadTask);
    }
    vTaskSuspend(ComTask_Handler);

    if (cmdUpdateModes != SUB_DEVICE_MODE_MAX) { // update Specified devs
        if (isForceUpdate || updateDev_isModeVersionLower(cmdUpdateModes, destVer)) {
            if (!updateDev_update(cmdUpdateModes)) {
                LOG_W("updateDev mode[%d], name[%s] failed!!\r\n", cmdUpdateModes, SubDevice_GetModeName(cmdUpdateModes));
            }
        }
    } else { // update all devs, Except for myself
        for (SUB_DEVICE_MODE mode = SUB_DEVICE_MODE_MIN; mode < SUB_DEVICE_MODE_MAX; mode++) {
            if (mode == SubDevice_GetMyMode()) {
                continue;
            }
            if (isForceUpdate || updateDev_isModeVersionLower(mode, destVer)) {
                LOG_W("updateDev mode[%d], name[%s] start...\r\n", mode, SubDevice_GetModeName(mode));
                if (!updateDev_update(mode)) {
                    LOG_W("updateDev mode[%d], name[%s] failed!!!\r\n", mode, SubDevice_GetModeName(mode));
                } else {
                    LOG_W("updateDev mode[%d], name[%s] success!!!\r\n", mode, SubDevice_GetModeName(mode));
                }
            }
        }
    }
    vTaskDelay(100);
    taskENTER_CRITICAL();
    if (SubDevice_IsSelfMaster()) {
        vTaskResume(xHandleUploadTask);
    }
    vTaskResume(ComTask_Handler);
    taskEXIT_CRITICAL();
    vTaskDelete(NULL);
}
