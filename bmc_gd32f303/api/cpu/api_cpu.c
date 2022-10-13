#include "cpu/api_cpu.h"
#include "OSPort.h"
#include "Message.h"
#include "ipmi_common.h"
#include "utc/api_utc.h"

xQueueHandle FTUartWrite_Queue = NULL;
xQueueHandle FTUartRead_Queue = NULL;
xQueueHandle FTPowerStatus_Queue = NULL;

static TimerHandle_t xTimerCPUTick = NULL;

VariableCPUParam g_CPUVariableParam;
FixedCPUParam g_CPUFixedParam;
__IO bool s_is_CPUInfo_has_received = false;
__IO bool g_CPUStatus = false;

static void vTaskFTUartWrite(void *pvParameters);
static void vTaskFTUartRead(void *pvParameters);

static INT16U EncodeSerialPkt2(INT8U *Pkt, INT16U Len, INT8U *EnPkt, INT8U enPktLength);
static INT16U DecodeSerialPkt(INT8U *Pkt, INT16U Len);

static void FTMsgProcess(INT8U *pReq, INT8U len);

static void vTimerCallback(xTimerHandle pxTimer);

void cpuGetInfoTask(void *arg)
{
	BaseType_t err = pdFALSE;
	MsgPkt_T Req;

	FTUartWrite_Queue = xQueueCreate(5, sizeof(MsgPkt_T));
	FTUartRead_Queue = xQueueCreate(5, sizeof(MsgPkt_T));

	xTaskCreate(vTaskFTUartWrite, "FTuartwrite", 256, NULL, 16, NULL);
	xTaskCreate(vTaskFTUartRead, "FTuartRead", 256, NULL, 15, NULL);
	xTimerCPUTick = xTimerCreate("Timer", 5000 / portTICK_RATE_MS, pdTRUE, 0, vTimerCallback); // 5S
	xTimerStart(xTimerCPUTick, portMAX_DELAY);

	while (1)
	{
		if (!s_is_CPUInfo_has_received)
		{
			Req.Size = GetEncodeCmd(CPUFixedInfoCmd, Req.Data);
			Req.Cmd  = CPUFixedInfoCmd;
		}
		else
		{
			Req.Size = GetEncodeCmd(CPUVariInfoCmd, Req.Data);
			Req.Cmd  = CPUVariInfoCmd;
		}
		err = xQueueSend(FTUartWrite_Queue, (char *)&Req, 10);
		if (err == pdFALSE)
		{
			LOG_E("FTUartWrite_Queue send msg ERR!");
		}
		msleep(1000);
	}
}

static void vTaskFTUartWrite(void *pvParameters)
{
	BaseType_t err = pdFALSE;
	char buff[sizeof(MsgPkt_T)];
	MsgPkt_T *Msg = (MsgPkt_T *)buff;
	int  req_len = 0;

	UNUSED(req_len);
	while (1)
	{
		err = xQueueReceive(FTUartWrite_Queue, buff, portMAX_DELAY);
		if (err == pdFALSE)
		{
			LOG_E("vTaskFTUartWrite get msg of write to FT ERR!");
			continue;
		}

		// ft_uart_write(Msg->Data, Msg->Size); // A0 00 00 00 00 00 00 00 00 04 00 FC A5

		switch(Msg->Cmd)
		{
		case CPUFixedInfoCmd:
			req_len = sizeof(VariableCPUParam) + sizeof(CPUMsgHdr) +  3 + 20;
			break;
		case CPUVariInfoCmd:
			req_len = sizeof(FixedCPUParam) + sizeof(CPUMsgHdr) +  3 + 20;
			break;
		case CPUPowerOffCmd:
		case CPUPowerUpCmd:
		case CPUResetCmd:
			req_len = sizeof(CPUMsgHdr) +  4 + 20;
			break;
		default:
			break;
		}
		//uart1_dma_enable(req_len);
		
	}
}

static void vTaskFTUartRead(void *pvParameters)
{
	BaseType_t err = pdFALSE;
	char buff[sizeof(MsgPkt_T)];
	MsgPkt_T *Msg = (MsgPkt_T *)buff;

	FTPowerStatus_Queue = xQueueCreate(5, sizeof(MsgPkt_T));
	LOG_I("vTaskFTUartRead START");
	while (1)
	{
		err = xQueueReceive(FTUartRead_Queue, buff, portMAX_DELAY);
		if (err == pdFALSE)
		{
			LOG_E("vTaskFTUartRead get msg of read ERR!");
			continue;
		}

		Msg->Size = DecodeSerialPkt(&Msg->Data[1], Msg->Size-2);

		if (CalculateCheckSum(&Msg->Data[1], Msg->Size) != 0)
		{
			LOG_E("FT Uart read msg check sum ERROR!");
			continue;
		}
		FTMsgProcess(&Msg->Data[1], Msg->Size);
	}
}

static int GetRawCmd(INT8U cmd, INT8U *pReq)
{
	CPUMsgHdr *hdr = (CPUMsgHdr *)pReq;

	hdr->ID = 0;
	hdr->Cmd = cmd;
	hdr->Len = 0;
	pReq[sizeof(CPUMsgHdr)] = CalculateCheckSum(pReq, sizeof(CPUMsgHdr));

	return sizeof(CPUMsgHdr) + 1;
}

int GetEncodeCmd(INT8U cmd, INT8U *pReq)
{
	INT8U cmd_buf[20];
	int len;

	len = GetRawCmd(cmd, cmd_buf);
	return EncodeSerialPkt2(cmd_buf, len, pReq, MSG_PAYLOAD_SIZE);   
}

static void FTMsgProcess(INT8U *pReq, INT8U len)
{
	BaseType_t err = pdFALSE;
	CPUMsgHdr *hdr; 
	MsgPkt_T Msg;

	if(len < sizeof(CPUMsgHdr))
	{
		LOG_I("FT uart recv dat less than sizeof(CPUMsgHdr)");
		return;
	}

	hdr = (CPUMsgHdr *)pReq;

	switch (hdr->Cmd)
	{
	case CPUVariInfoCmd:
		if(len < sizeof(VariableCPUParam) + sizeof(CPUMsgHdr) + 1)
		{
			LOG_I("FT uart recv dat less than sizeof(VariableCPUParam)+sizeof(CPUMsgHdr)+1");
			return;
		}
		memcpy((char *)&g_CPUVariableParam, (char *)&pReq[sizeof(CPUMsgHdr)], len - sizeof(CPUMsgHdr) - 1);
		g_CPUStatus = true;
		xTimerStart(xTimerCPUTick, portMAX_DELAY);
		break;
	case CPUFixedInfoCmd:
		if(len < sizeof(FixedCPUParam) + sizeof(CPUMsgHdr) + 1)
		{
			LOG_I("FT uart recv dat less than sizeof(FixedCPUParam)+sizeof(CPUMsgHdr)+1");
			return;
		}
		s_is_CPUInfo_has_received = true;
		memcpy((char *)&g_CPUFixedParam, (char *)&pReq[sizeof(CPUMsgHdr)], len - sizeof(CPUMsgHdr) - 1);
		break;
	case CPUPowerOffCmd:
	case CPUPowerUpCmd:
		if(len < 1 + sizeof(CPUMsgHdr) + 1)
		{
			LOG_I("FT uart recv dat less than 1+sizeof(CPUMsgHdr)+1");
			return;
		}
		Msg.Data[0] = pReq[sizeof(CPUMsgHdr)];
		Msg.Size = 1;
		err = xQueueSend(FTPowerStatus_Queue, (char *)&Msg, 10);
		if (err == pdFALSE)
		{
			LOG_E("FTPowerStatus_Queue send msg ERR!");
		}
		break;
	default:
		break;
	}
}

static void vTimerCallback(xTimerHandle pxTimer)
{
	uint32_t ulTimerID;
	ulTimerID = (uint32_t)pvTimerGetTimerID(pxTimer);

	switch (ulTimerID)
	{
	case 0: // software timer0
		g_CPUStatus = false;
		//LOG_I("CPU NOT RESPONSE.");
		xTimerStop(xTimerCPUTick, portMAX_DELAY);
		break;
	default:
		break;
	}
}

/**
 * @brief Encode the serial packet in IPMI format
 * @param Pkt Pointer to packet
 * @param Len Size of the packet
 * @param EnPkt Pointer to encoded packet
 * @return Size of the Encoded packet
 **/
static INT16U EncodeSerialPkt2(INT8U *Pkt, INT16U Len, INT8U *EnPkt, INT8U enPktLength)
{
	INT16U index = 0;
	INT16U i;

	/* Put the START Byte */
	EnPkt[index++] = START_BYTE;

	/* Copy the packet */
	for (i = 0; i < Len; i++)
	{
		switch (*(Pkt + i))
		{
		case START_BYTE:
		case STOP_BYTE:
		case HANDSHAKE_BYTE:
		case DATA_ESCAPE_BYTE:
			EnPkt[index++] = DATA_ESCAPE_BYTE;
			EnPkt[index++] = (*(Pkt + i) + 0x10);
			break;

		case ASCII_ESC_BYTE:
			EnPkt[index++] = DATA_ESCAPE_BYTE;
			EnPkt[index++] = (*(Pkt + i) + 0x20);
			break;

		default:
			EnPkt[index++] = (*(Pkt + i));
			break;
		}
		if (index >= (enPktLength - 1)) {
			LOG_E("EncodeSerialPkt ERR! Exceeding the maximum length");
			return 0;
		}
	}

	/* Put the STOP Byte */
	EnPkt[index++] = STOP_BYTE;

	return index;
}

static INT16U
DecodeSerialPkt(INT8U *Pkt, INT16U Len)
{
	uint32 i;
	uint8 byESCByteRecvd = 0;

	INT16U index = 0;
	for (i = 0; i < Len; i++)
	{
		/* ESC byte already received so decode the next byte */
		if (byESCByteRecvd)
		{
			switch (Pkt[i])
			{
			case ENCODED_START_BYTE:
				Pkt[index++] = START_BYTE;
				break;
			case ENCODED_STOP_BYTE:
				Pkt[index++] = STOP_BYTE;
				break;
			case ENCODED_HAND_SHAKE_BYTE:
				Pkt[index++] = HANDSHAKE_BYTE;
				break;
			case ENCODED_DATA_ESCAPE:
				Pkt[index++] = DATA_ESCAPE_BYTE;
				break;
			case ENCODED_BYTE_ESCAPE:
				Pkt[index++] = BYTE_ESC;
				break;
			default:
				return 0;
			}

			byESCByteRecvd = 0;
		}
		else if (Pkt[i] == DATA_ESCAPE_BYTE) /* ESC received just now */
			byESCByteRecvd = 1;
		else /* Normal data */
			Pkt[index++] = Pkt[i];
	}

	return index;
}

uint16_t GetBmcFirmwareVersion(char* str)
{
	char buff[20] = {0};
	uint16_t version = 0xffff;

	int str_len = strlen(str);

	if (str_len > sizeof(buff))
	{
		return version;
	}
	
	memcpy(buff, str, str_len);
	version = atoi(buff);

	return version;	
}
extern uint32_t g_time_run;
uint32_t  GetBmcRunTime(void)
{
  	return g_time_run;	
}

