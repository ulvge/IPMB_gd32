
#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/sys.h"
#include "update/update.h"
#include <string.h>
#include <stdio.h>
#include "gd32f20x.h"
#include "lwip/memp.h"
#include "main.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"
#include "ipmi_common.h"
#include "update/flash.h"
#include "event_groups.h"

#define MAX_BUF_SIZE 1024

static int ReadData(MsgPkt_T *pMsgPkt, int Socket);
static void TCPSocketRecv(void *arg);
static int SendTcpPkt(MsgPkt_T *pRes);
static void ProcessUpdateReq(const MsgPkt_T *pReq, MsgPkt_T *pRes);
static bool ProcessSlaveUpdateReq(const MsgPkt_T *pReq, MsgPkt_T *pRes);
static void Jump_To_Update_Main(void);
unsigned char checksum(const char *buff, int len);

xQueueHandle TcpRecvMsg_Queue = NULL;

void updateTask(void *arg)
{
    MsgPkt_T MsgReq;
    int sock, connected;
    struct sockaddr_in server_addr, client_addr;
    int listen_port = 1234;
    struct timeval timeout;
    fd_set readset, readset_c;
    socklen_t sin_size = sizeof(struct sockaddr_in);

		TcpRecvMsg_Queue = xQueueCreate(10, sizeof(MsgPkt_T));
    xTaskCreate(TCPSocketRecv, "TCPSocketRecv", configMINIMAL_STACK_SIZE * 3, NULL, 10, NULL);

    while (1)
    {
        /* 一个socket在使用前，需要预先创建出来，指定SOCK_STREAM为TCP的socket */
        if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
            sleep(1);
            continue;
            // LOG_E("Create socket error\r\n");
            // goto __exit;
        }

        /* 初始化服务端地址 */
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(listen_port); /* 服务端工作的端口 */
        server_addr.sin_addr.s_addr = INADDR_ANY;
        memset(&(server_addr.sin_zero), 0x0, sizeof(server_addr.sin_zero));

        /* 绑定socket到服务端地址 */
        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
        {
            LOG_E("Unable to bind");
            continue;
            // goto __exit;
        }

        /* 在socket上进行监听 */
        if (listen(sock, 10) == -1)
        {
            LOG_E("Listen error");
            goto __exit;
        }

        LOG_I("TCPServer Waiting for client on port %d...", listen_port);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        while (1)
        {
            FD_ZERO(&readset);
            FD_SET(sock, &readset);

            // LOG_I("Waiting for a new connection...\r\n");

            /* Wait for read or write */
            if (select(sock + 1, &readset, NULL, NULL, &timeout) == 0)
                continue;

            /* 接受一个客户端连接socket的请求，这个函数调用是阻塞式的 */
            connected = accept(sock, (struct sockaddr *)&client_addr, &sin_size);
            /* 返回的是连接成功的socket */
            if (connected < 0)
            {
                LOG_E("update socket accept connection failed!");
                continue;
            }

            /* 接受返回的client_addr指向了客户端的地址信息 */
            LOG_I("update socket got a connection from (%s , %d)",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            /* 客户端连接的处理 */
            while (1)
            {
                FD_ZERO(&readset_c);
                FD_SET(connected, &readset_c);

                /* Wait for read or write */
                if (select(connected + 1, &readset_c, NULL, NULL, &timeout) == 0)
                    continue;

                ReadData(&MsgReq, connected);
                //                 for(i=0; i<MsgReq.Size; i++)
                //                 {
                //                     LOG_RAW(" %02x", MsgReq.Data[i]);
                //                 }
                //                 LOG_RAW("\r\n");

                xQueueSend(TcpRecvMsg_Queue, (char *)&MsgReq, 5);
            }
        }
    }

__exit:
    if (connected >= 0)
    {
        closesocket(connected);
        connected = -1;
    }
    if (sock >= 0)
    {
        closesocket(sock);
        sock = -1;
    }
    vTaskDelete(NULL);
    return;
}

/**
 * @fn  ReadData
 * @brief This function receives the IPMI LAN request packets
 *
 * @param pMsgPkt - pointer to message packet
 * @param Socket  - Socket handle
**/
static int ReadData(MsgPkt_T *pMsgPkt, int Socket)
{
    INT8U *pData = pMsgPkt->Data;
    INT16S Len = 0;
    INT16U RecvdLen = 0;

    /* Read minimum bytes to find class of message */
    while (RecvdLen < sizeof(UpdateMsgReq))
    {
        Len = recv(Socket, &pData[RecvdLen], MAX_BUF_SIZE, 0);
        if ((Len >= -1) && (Len <= 0))
        {
            return -1;
        }
        RecvdLen += (INT16U)Len;
    }

    pMsgPkt->Size = RecvdLen;
    pMsgPkt->Socket = Socket;

    return 0;
}

static void TCPSocketRecv(void *arg)
{
    char buff[sizeof(MsgPkt_T)];
    MsgPkt_T *tcp_recv_msg = (MsgPkt_T *)buff;
    MsgPkt_T tcp_send_msg;

    while (1)
    {
        xQueueReceive(TcpRecvMsg_Queue, buff, portMAX_DELAY);
        ProcessUpdateReq(tcp_recv_msg, &tcp_send_msg);
        SendTcpPkt(&tcp_send_msg);
    }
}

int SendTcpPkt(MsgPkt_T *pRes)
{
    int ret;

    ret = send(pRes->Socket, (char *)pRes->Data, pRes->Size, 0);
    if (ret < 0)
    {
        LOG_E("socket send error.");
        closesocket(pRes->Socket);
        pRes->Socket = -1;
    }
    else if (ret == 0)
    {
        LOG_W("Send warning, send function return 0.");
    }

    return ret;
}

extern xQueueHandle RecvForwardI2CDatMsg_Queue;
extern xQueueHandle ResponseDatMsg_Queue;

static bool ProcessSlaveUpdateReq(const MsgPkt_T *pReq, MsgPkt_T *pRes)
{
    BaseType_t err = pdFALSE;
    IPMIMsgHdr_T hdr;
    MsgPkt_T Msg;
    int len;

    hdr.ReqAddr = pReq->Data[4];
    hdr.NetFnLUN = NETFN_OEM << 2; // RAW
    hdr.ChkSum = CalculateCheckSum((INT8U*)&hdr, 2);
    hdr.RqSeqLUN = 0x01;
    hdr.Cmd = 0x02; // update firmware

    len = sizeof(IPMIMsgHdr_T);
    memcpy(Msg.Data, &hdr, len);
    memcpy(&Msg.Data[len], pReq->Data, pReq->Size);
    len = len + pReq->Size;

    Msg.Data[len] = CalculateCheckSum(Msg.Data, len);
    Msg.Size = len + 1;

    pRes->Param = IPMI_REQUEST;
    err = xQueueSend(ResponseDatMsg_Queue, (char*)&Msg, 50);
    if (err == pdFALSE)
    {
        LOG_E("update app xQueueSend ERR!");
        return false;
    }

    err = xQueueReceive(RecvForwardI2CDatMsg_Queue, (char*)&Msg, 100);
    if (err == pdFALSE)
    {
        LOG_E("update app xQueueReceive ERR!");
        return false;
    }
		
    pRes->Size = Msg.Size - sizeof(IPMIMsgHdr_T);
    memcpy(pRes->Data, &Msg.Data[sizeof(IPMIMsgHdr_T)], pRes->Size);
     
    return true;
}

static void ProcessUpdateReq(const MsgPkt_T *pReq, MsgPkt_T *pRes)
{
    bool ret;
    char *username;
    char *password;
    const UpdateMsgReq *pMsgReq = (UpdateMsgReq *)pReq->Data;
    UpdateMsgRes *pMsgRes = (UpdateMsgRes *)pRes->Data;

    pRes->Size = sizeof(UpdateMsgRes);
    pRes->Socket = pReq->Socket;
    pMsgRes->cmd1 = pMsgReq->cmd1;
    pMsgRes->cmd2 = pMsgReq->cmd2;

    if (checksum((char *)pReq->Data, pReq->Size - 1) != pReq->Data[pReq->Size - 1])
    {
        LOG_E("update app recv dat checksum failded!");
        pMsgRes->err_code = 0xFF; // 0xFF: ERR CODE
        return;
    }

    if(pMsgReq->addr != 0x20){  // slave
        ret = ProcessSlaveUpdateReq(pReq, pRes);
        if(!ret)
        {
            LOG_E("update app recv no slave dat");
            pMsgRes->err_code = 0xFF; // 0xFF: ERR CODE
        }
        return;
    }

    switch (pMsgReq->cmd2)
    {
    case ENTRY_UPDATE_MODE_CMD:
        username = (char *)&pMsgReq->dat[0];
        password = (char *)&pMsgReq->dat[16];
        if (strcmp(username, "zkcc") == 0 && strcmp(password, "zkcc") == 0)
        {
             pMsgRes->err_code = 0x0B; // app
            SendTcpPkt(pRes);
            closesocket(pRes->Socket);
            LOG_I("restart.");
            //	Jump_To_Update_Main();
            NVIC_SystemReset();
        }
        else
        {
            pMsgRes->err_code = 0x0C; // ERR CODE  can't operate
        }

        break;
    default:
        pMsgRes->err_code = 0xFF; // 0xFF: ERR CODE
        break;
    }
}

typedef void (*pFunction)(void);
#define ApplicationAddress 0x08000000 // boot main addr
uint32_t JumpAddress = 0;
pFunction Jump_To_Application;

__attribute__((unused)) void Jump_To_Update_Main()
{
    if (((*(__IO uint32_t *)ApplicationAddress) & 0x2FFE0000) == 0x20000000)
    {
        CPU_IntDisable();
        LOG_I("jump to boot main.");
        JumpAddress = *(__IO uint32_t *)(ApplicationAddress + 4);
        Jump_To_Application = (pFunction)JumpAddress;
        __set_MSP(*(__IO uint32_t *)ApplicationAddress);
        Jump_To_Application();
    }
}

unsigned char checksum(const char *buff, int len)
{
    unsigned char sum = 0;
    int i;

    for (i = 0; i < len; i++)
    {
        sum = sum + buff[i];
    }

    return sum & 0xFF;
}
