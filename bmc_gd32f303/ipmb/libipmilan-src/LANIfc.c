/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * LANIfc.c
 * LAN Interface Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS   0
#include "Types.h"
#include "OSPort.h"
#include "IPMI_Main.h"
#include "MsgHndlr.h"
#include "Support.h"
// #include "SharedMem.h"
#include "IPMI_LANIfc.h"
// #include "IPMI_RMCP.h"
#include "Debug.h"
// #include "PMConfig.h"
#include "RMCP.h"
#include "LANIfc.h"
#include "IPMIDefs.h"
// #include "NVRAccess.h"
// #include "Util.h"
#include "MD.h"
// #include "nwcfg.h"
#include "IPMIConf.h"
// #include "PDKAccess.h"
// #include "ncml.h"
// #include <netdb.h>        /* getaddrinfo(3) et al.                       */
// #include <netinet/in.h>   /* sockaddr_in & sockaddr_in6 definition.      */
// #include <net/if.h>
// #include <sys/prctl.h>
// #include <unistd.h>
// #include <fcntl.h>

// #include "featuredef.h"
// #include "LANConfig.h"
// #include "ThrdPrio.h"
#include "lwip/sockets.h"


#define NO_OF_RETRY                     3
#define MAX_POSSIBLE_IPMI_DATA_SIZE     1024
#define MAX_LAN_BUFFER_SIZE             1024
#define LAN_TIMER_INTERVAL              10

#define RMCP_CLASS_MSG_OFFSET           3
#define IPMI_MSG_AUTH_TYPE_OFFSET       4
#define RMCP_ASF_PING_MESSAGE_LENGTH    12
#define IPMI_MSG_LEN_OFFSET             13
#define IPMI20_MSG_LEN_OFFSET           14
#define RMCP_CLASS_MSG_ASF              0x06
#define RMCP_CLASS_MSG_IPMI             0x07


extern BMCInfo_t g_BMCInfo;
 
void ProcessBridgeMsg (const MsgPkt_T* pReq);
void UDPSocketRecv(void *arg);
int  ReadData (MsgPkt_T *pMsgPkt, int Socket);

/**
 * @brief LAN Interface Task.
**/
void LANIfcTask (void *arg)
{
    UDPSocketRecv(arg);
}

extern xQueueHandle RecvDatMsg_Queue;
/**
*@fn InitUDPSocket
*@brief This function is invoked to initialize LAN udp sockets
*@return Returns 0 on success
*/
void UDPSocketRecv(void *arg)
{
    int sockfd = -1;
    int udp_port = 623;
    struct sockaddr_in server_addr;        
    int tick = 100;
    struct timeval timeout;
    fd_set readset;
    MsgPkt_T    Req;
  
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(udp_port);
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    
    while (1){
        /* create the socket */
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd < 0){
            // LOG_W("create socket failed, try again...\n");
            sleep(1);
            continue;
        }

        LOG_I("create socket success.");
        /* 绑定socket到服务端地址 */
        if (bind(sockfd, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr)) == -1)
        {
            tick--;
            if(tick ==0)
            {
                LOG_E("Unable to bind.");
                goto __exit;
            }
            sleep(2);
            continue;
        }
        else
        {
            LOG_I("bind success.");
            break;
        }  
    }

    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    while (1)
    {
        BaseType_t err = pdFALSE;
        
        FD_ZERO(&readset);
        FD_SET(sockfd, &readset);

        /* Wait for read or write */
        if (select(sockfd + 1, &readset, NULL, NULL, &timeout) == 0){
            continue;
        }

        ReadData(&Req, sockfd);
        // LOG_I("recv data %04d\r\n", recv_tick++);
        // msleep(10);
        err = xQueueSend(RecvDatMsg_Queue, (char*)&Req, 10);
        if (err == pdFALSE)
        {
            LOG_E("LAN send queue msg ERR!");
            continue;
        }
        // for(i=0; i<Req.Size; i++)
        // {
        //     LOG_RAW(" %02x", Req.Data[i]);
        // }
        // LOG_RAW("\r\n");
    }		

__exit:
    if (sockfd >= 0)
    {
        close(sockfd);
        sockfd = -1;
    }
}


/**
 * @fn  ReadData
 * @brief This function receives the IPMI LAN request packets
 *
 * @param pMsgPkt - pointer to message packet
 * @param Socket  - Socket handle
**/
int  ReadData (MsgPkt_T *pMsgPkt, int Socket)
{
    INT8U   *pData      = pMsgPkt->Data;
    INT16S  Len         = 0;
    INT16U  RecvdLen    = 0;
    INT16U  IPMIMsgLen  = 0;
    INT16U  IPMIMsgOffset   = 0;

   struct  sockaddr_in Sourcev4;
      
   unsigned int  Address_Len = sizeof (Sourcev4);

    /* Read minimum bytes to find class of message */
    while (RecvdLen < RMCP_CLASS_MSG_OFFSET)
    {
        Len = recvfrom (Socket, &pData[RecvdLen], MAX_LAN_BUFFER_SIZE, 0, (struct sockaddr *)&Sourcev4, &Address_Len);
        if ((Len >= -1) && (Len <= 0))
        {
            return -1;
        }
        RecvdLen += (INT16U)Len;
    }

    /*  if RMCP Presence Ping Requested */
    if (RMCP_CLASS_MSG_ASF == pData[RMCP_CLASS_MSG_OFFSET])
    {
        /* Read remaining RMCP ASF ping message */
        while (RecvdLen < RMCP_ASF_PING_MESSAGE_LENGTH)
        {
              Len = recvfrom (Socket, &pData[RecvdLen], MAX_LAN_BUFFER_SIZE, 0, (struct sockaddr *)&Sourcev4, &Address_Len);
            if ((Len >= -1) && (Len <= 0))
            {
                return -1;
            }
            RecvdLen += (INT16U)Len;
        }
    }
            /*else if IPMI RMCP request */
    else if (RMCP_CLASS_MSG_IPMI == pData[RMCP_CLASS_MSG_OFFSET])
    {
        /* Read minimum no of bytes for IPMI Auth type offset*/
        while (RecvdLen < IPMI_MSG_AUTH_TYPE_OFFSET)
        {
            Len = recvfrom (Socket, &pData[RecvdLen], MAX_LAN_BUFFER_SIZE, 0,(struct sockaddr *)&Sourcev4, &Address_Len);
            if ((Len >= -1) && (Len <= 0))
            {
                return -1;
            }
            RecvdLen += (INT16U)Len;
        }
        /* Get the IPMI message length offset based on format/authentication type */
        if (pData [IPMI_MSG_AUTH_TYPE_OFFSET] == RMCP_PLUS_FORMAT)
        {
            IPMIMsgOffset = IPMI20_MSG_LEN_OFFSET + 1;
        }
        else if (pData [IPMI_MSG_AUTH_TYPE_OFFSET] == AUTH_TYPE_NONE)
        {
            IPMIMsgOffset = IPMI_MSG_LEN_OFFSET;
        }
        else
        {
            IPMIMsgOffset = IPMI_MSG_LEN_OFFSET + AUTH_CODE_LEN;
        }

        /* Read minimum no of bytes for IPMI message length offset*/
        while (RecvdLen < IPMIMsgOffset)
        {
            Len = recvfrom (Socket, &pData[RecvdLen], MAX_LAN_BUFFER_SIZE, 0,(struct sockaddr *)&Sourcev4, &Address_Len);
            if ((Len >= -1) && (Len <= 0))
            {
                return -1;
            }
            RecvdLen += (INT16U)Len;
        }

        /* Get the IPMI message length based on RMCP format type */
        if (pData [IPMI_MSG_AUTH_TYPE_OFFSET] == RMCP_PLUS_FORMAT)
        {
            // IPMIMsgLen = ipmitoh_u16 (*((INT16U*)&pData [IPMI20_MSG_LEN_OFFSET]));
            IPMIMsgLen = *((INT16U*)&pData [IPMI20_MSG_LEN_OFFSET]);
        }
        else
        {
            IPMIMsgLen = pData [IPMIMsgOffset];
        }
        /* We are assuming that we cannot get more than 17 K data in IPMI Msg */
        /* This work around for fix the malformed IPMI Msg length */

        if(IPMIMsgOffset > MAX_POSSIBLE_IPMI_DATA_SIZE )
        {
            return -1;
        }
        /* Read the remaining IPMI message packets */
        while (RecvdLen < IPMIMsgLen)
        {
            Len = recvfrom (Socket, &pData[RecvdLen], MAX_LAN_BUFFER_SIZE, 0,(struct sockaddr *)&Sourcev4, &Address_Len);
            if ((Len >= -1) && (Len <= 0))
            {
                if(Len == -1)
                {
                    return -1;
                }
                else
                    return -1;
            }

            RecvdLen += (INT16U)Len;
        }
    }/* else other RMCP class are not supported. */
    else
    {
        IPMI_DBG_PRINT ("Unknown RMCP class");
    }
    pMsgPkt->Size     = RecvdLen;
    pMsgPkt->UDPPort  = ((struct sockaddr_in *)&Sourcev4)->sin_port;
    pMsgPkt->Socket   = Socket;
    pMsgPkt->Param = LAN_REQUEST; 
    memcpy (pMsgPkt->IPAddr, &((struct sockaddr_in *)&Sourcev4)->sin_addr.s_addr, sizeof (struct in_addr));
    // LOG_I("socket recv data len: %d", pMsgPkt->Size);

    // LOG_I("\n I received in Socket  and Channel for verification : %d %x %x",Address_Len,Socket, pMsgPkt->Channel);

    return 0;
}

/**
 * @fn SendLANPkt
 * @brief This function sends the IPMI LAN Response to the requester
 * @param pRes - Response message.
**/
int SendLANPkt (MsgPkt_T *pRes)
{
    struct  sockaddr_in Dest;
    //    struct stat Stat;
    int ret = 0;

    /* Set the destination UDP port and IP Address */
    Dest.sin_family     =   AF_INET;
    Dest.sin_port       =   pRes->UDPPort;
    memcpy (&Dest.sin_addr.s_addr, pRes->IPAddr, sizeof (struct in_addr));

    // LOG_RAW("lan send:");
    // for(i=0; i<pRes->Size; i++)
    // {
    //     LOG_RAW(" %02x", pRes->Data[i]);
    // }
    // LOG_RAW("\r\n");
    //Check the socket before send a message on a socket
//    if (fstat(pRes->Socket, &Stat) != -1) {
    ret = sendto (pRes->Socket, pRes->Data, pRes->Size, 0, (struct sockaddr*)&Dest, sizeof (Dest));
    if (ret == -1)
    {
        IPMI_WARNING ("LANIfc.c : Error sending response packets to LAN");
    }
    else
    {
        // LOG_I("lan send %04d",tick++);
    }
//    }

    return 1;
}

extern xQueueHandle ResponseDatMsg_Queue;
/**
 * @brief Process SMB Request.
 * @param pReq - Request message.
**/
void
ProcessLANReq (const MsgPkt_T* pReq, _NEAR_ MsgPkt_T *pRes)
{
    // SessionInfo_T*  pSessionInfo;
    // SessionHdr_T*   pSessionHdr;
    // SessionHdr2_T*  pSessionHdr2;
    // INT32U          SessionID =0;
    // BMCInfo_t *pBMCInfo= &g_BMCInfo;
    MiscParams_T pParams;

	
    /* Copy the request to response */
    *pRes = *pReq;

    /* Save the LAN header inofmation */
    // pSessionHdr  = (SessionHdr_T*) (((RMCPHdr_T*)pReq->Data) + 1);
    // pSessionHdr2 = (SessionHdr2_T*)(((RMCPHdr_T*)pReq->Data) + 1);

    /* Process the RMCP Request */
    pRes->Size = ProcessRMCPReq ((RMCPHdr_T*)pReq->Data, (RMCPHdr_T*)pRes->Data, &pParams, pReq->Channel, 0);

    /* ResLen is 0, don't send the packet */
    if (0 == pRes->Size )
    {
        IPMI_DBG_PRINT ("LANIfc.c : LAN request packet dropped, not processed\r\n");
        return;
    }

    return;
}

/*--------------------------------------------
 * ProcessBridgeMsg
 *--------------------------------------------*/
void
ProcessBridgeMsg (const MsgPkt_T* pReq)
{
    MsgPkt_T        ResPkt;
    INT16U          PayLoadLen   = 0;
    __attribute__((unused)) INT8U           PayLoadType  = 0;
    // BMCInfo_t *pBMCInfo = &g_BMCInfo;

    IPMI_DBG_PRINT ("LANIfc: Bridge Request\n");

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SessionTblMutex, WAIT_INFINITE);
    /* Copy Lan RMCP headers from Session Record */
    // ResPkt.UDPPort  = pSessionInfo->LANRMCPPkt.UDPHdr.SrcPort;
    // ResPkt.Socket   = pSessionInfo->hSocket;

    // _fmemcpy (ResPkt.IPAddr, pSessionInfo->LANRMCPPkt.IPHdr.Srcv4Addr, sizeof (struct in_addr));
    // _fmemcpy (ResPkt.Data, &pSessionInfo->LANRMCPPkt.RMCPHdr, sizeof (RMCPHdr_T));

#if IPMI20_SUPPORT == 1
    // if (RMCP_PLUS_FORMAT == pSessionInfo->AuthType)
    if(1)
    {
        /* Fill Session Header */
        // pSessionInfo->OutboundSeq++;
        PayLoadLen      = pReq->Size - 1;
        PayLoadType     = pReq->Cmd;
        // PayLoadType    |= (pSessionInfo->SessPyldInfo[PayLoadType].AuxConfig[0] & 0xC0);
        // PayLoadLen      = Frame20Payload (PayLoadType, (_NEAR_ RMCPHdr_T*)&ResPkt.Data [0],
        //                              &pReq->Data[1], PayLoadLen, pSessionInfo, BMCInst);
    }
    else
#endif /*IPMI20_SUPPORT == 1*/
    {
        /* Fill Session Header */
        _NEAR_ SessionHdr_T* pSessionHdr = (_NEAR_ SessionHdr_T*)(&ResPkt.Data [sizeof(RMCPHdr_T)]);
        _NEAR_ INT8U*        pPayLoad    = (_NEAR_ INT8U*)(pSessionHdr + 1);

        // pSessionHdr->AuthType       = pSessionInfo->AuthType;
        // pSessionHdr->SessionSeqNum  = pSessionInfo->OutboundSeq++;
        // pSessionHdr->SessionID      = pSessionInfo->SessionID;

        /* If AuthType is not 0 - Compute AuthCode */
        // if (0 != pSessionInfo->AuthType)
        if(1)
        {
            IPMI_DBG_PRINT ("Compute Authcode\n");
            PayLoadLen = AUTH_CODE_LEN;
            pPayLoad [PayLoadLen++] = pReq->Size - 1;
            _fmemcpy (&pPayLoad [PayLoadLen], (pReq->Data + 1), (pReq->Size - 1));
            PayLoadLen += pReq->Size;
            PayLoadLen--;
            PayLoadLen += sizeof (SessionHdr_T) + sizeof (RMCPHdr_T);
            // ComputeAuthCode (pSessionInfo->Password, pSessionHdr,
            //                  (_NEAR_ IPMIMsgHdr_T*) &pPayLoad [AUTH_CODE_LEN+1],
            //                  pPayLoad, MULTI_SESSION_CHANNEL);
        }
        else
        {
           pPayLoad [PayLoadLen++] = pReq->Size - 1;
            /*  Fill the ipmi message */
           _fmemcpy (&pPayLoad [PayLoadLen], (pReq->Data + 1), (pReq->Size - 1));
           PayLoadLen += pReq->Size;
           PayLoadLen--;
           PayLoadLen += sizeof (SessionHdr_T) + sizeof (RMCPHdr_T);
        }

    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SessionTblMutex);

    // ResPkt.Size = PayLoadLen;
    // ResPkt.Channel = pSessionInfo->Channel;

    // if(pSessionInfo->Activated)
    // {
    //     /* Sent the response packet */
    //     SendLANPkt (&ResPkt);
    // }

    return;
}
