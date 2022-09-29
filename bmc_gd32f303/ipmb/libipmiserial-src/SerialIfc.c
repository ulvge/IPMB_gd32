/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2005-2006, American Megatrends Inc.        **
 ***                                                            **
 ***            All Rights Reserved.                            **
 ***                                                            **
 ***        6145-F, Northbelt Parkway, Norcross,                **
 ***                                                            **
 ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 ***                                                            **
 *****************************************************************
 *****************************************************************
 ******************************************************************
 *
 * serialif.c
 * Serial Interface.
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 *              Bakka Reddy <bakkar@ami.com>
 *
 ******************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
// #include "Debug.h"
#include "debug_print.h"
// #include "OSPort.h"
#include "Support.h"
#include "MsgHndlr.h"
#include "IPMI_Main.h"
// #include "NVRAccess.h"
// #include "SharedMem.h"
#include "Serial.h"
#include "SerialIfc.h"
#include "IPMIConf.h"
// #include "PDKAccess.h"
// #include "IfcSupport.h"
// #include "ThrdPrio.h"

/*** Local Macro Definitions ***/
#define BAUD_RATE_9600          6
#define BAUD_RATE_19200         7
#define BAUD_RATE_38400         8
#define BAUD_RATE_57600         9
#define BAUD_RATE_115200        10

#define NO_FLOWCONTROL          0
#define HARDWARE_FLOWCONTROL    1
#define XON_XOFF_FLOWCONTRO     2

#define STATE_IDLE              0x00
#define STATE_RECEIVING         0x01
#define STATE_PKT_RECEIVED      0x02
#define PREV_BYTE_NOT_ESC       0x00
#define PREV_BYTE_ESC           0x01

#define ASCII_ESC_BYTE          0x1b
#define ASCII_ESC_ENCODE_BYTE   0x3b

#define SERIAL_ALERT_DIAL_PAGE  0
#define SERIAL_ALERT_TAP_PAGE   1
#define SERIAL_ALERT_PPP_ALERT  2

#define PING_PKT_CHECKSUM       0x89

#define MODEM_MODE_CONNECT      1
#define MODEM_MODE_OK           2

#define MODEM_RES_TIME_OUT      3
#define MODEM_CONNECT_TIME_OUT  20



#define CB_STR_SIZE (MAX_MODEM_INIT_STR_BLOCKS * MAX_MODEM_INIT_STR_BLOCK_SIZE + \
                                MAX_MODEM_DIAL_CMD_SIZE + 2)

extern xQueueHandle ResponseDatMsg_Queue;

/*** Function Prototypes ***/
static int      InitSerialPort          (int BMCInst);
static void*    RecvSerialPkt           (void*);
static int      ValidateSerialCheckSum  (_FAR_ INT8U* Pkt, INT16U Len);
static int      ProcessSerPortReq       (_NEAR_ MsgPkt_T* pReq,  MsgPkt_T *pRes);
static int      ProcessBridgeReq        (_NEAR_ MsgPkt_T* pReq,  MsgPkt_T *pRes);
static INT16U   EncodeSerialPkt         (_NEAR_ INT8U* Pkt, INT16U Len, _NEAR_ INT8U* EnPkt);
static INT16U   DecodeSerialPkt         (_NEAR_ INT8U* Pkt, INT16U Len);
static void     OnBasicModeByteReceived (INT8U byte,int BMCInst);
//static void     CreatTerminalTask (int BMCInst);

/**
 * @brief Waits for Request from MsgHndlr and Sends the req for processing
 * @param Addr holds the address of structure which contains BMCInst,Argument
 * and Length of Argument.
 **/
bool ProcessSerialReq (_NEAR_ MsgPkt_T *pReq, _NEAR_ MsgPkt_T *pRes)
{
    int ret = -1; 

    pReq->Size = DecodeSerialPkt(pReq->Data, pReq->Size);
    ret = ProcessSerPortReq (pReq, pRes);

    if(ret != 0){
        IPMI_WARNING ("Get msg failed!\n");  
        return false;  
    }

    pRes->Param = SERIAL_REQUEST;    
    return true;  
}

/**
 * InitSerialPort
 * @brief Initialize the Serial port
 * @Param BMCInst holds the instance value of BMC
 * @Return 0 if Initialization is success else -1
 **/
__attribute__((unused)) int
InitSerialPort (int BMCInst)
{
    return 0;
}

/**
 * RecvSerialPkt
 * @brief Receives data from serial port
 * .@Param pArg pointer to the BMCInst value
 **/
__attribute__((unused)) static void*  RecvSerialPkt (void* pArg)
{
 
    return 0;
}


/**
 * @brief 1 check sum, 2 send handshake,3 get&hand map, 4 encode and send ack
 * @param pReq Pointer to Request Message packet
 * @param BMCInst holds the Instance value of BMC
 **/
static int
ProcessSerPortReq (_NEAR_ MsgPkt_T* pReq,  MsgPkt_T *pRes)  // get ipmitool msg and send I2C msg to slave
{
    INT8U       ResLen;
    INT8U       HandShake;
    INT8U       EnRes [MAX_SERIAL_PKT_SIZE];

    /* 1 Validate the checksum */
    if (0 != ValidateSerialCheckSum (pReq->Data, pReq->Size))
    {
        IPMI_DBG_PRINT ("**** Checksum Failed ****\n");
        return -1;
    }
    /* 2 Send Basic Mode Handshake to the remote Console*/
    HandShake = HANDSHAKE_BYTE;
    serial_write(&HandShake, sizeof(HandShake));

    /* 3 Post to Message Handler and get the response */
  //  pReq->Channel = 0;
    pReq->Param = SERIAL_BASIC_MODE;
    // pReq->Size = 0;

    IPMI_DBG_PRINT ("\nRequest Message :\n");

    ResLen = ProcessSerialMessage (pReq, pRes,0);

    if (0 == ResLen)
    {
        IPMI_DBG_PRINT ("SerialIfc: Packet Ignored\n");
        return -1;
    }

    /* 4 encode and ransmit the response */
    pRes->Size = EncodeSerialPkt (pRes->Data, ResLen, EnRes);

    _fmemcpy(pRes->Data, EnRes, pRes->Size);

    return 0;
}


/**
 * @brief Processes IPMI bridge requests
 * @param pReq Pointer to Request Message packet
 * @param BMCInst holds the Instance value of BMC
 **/
__attribute__((unused)) static int
ProcessBridgeReq (_NEAR_ MsgPkt_T* pReq,  MsgPkt_T *pRes)  // recv I2C msg and send to ipmitool
{
    pRes->Size = EncodeSerialPkt (pReq->Data, pReq->Size, pRes->Data);

    /* Transmit the packet*/
    // SendSerialPkt (1, EnPkt, EnPktLen,0);

    return 0;
}



/**
 * @brief Handles received byte when serial inetrface is in Basic mode state
 * @param byte a byte received through serial interface
 * @param BMCInst holds the Instance value of BMC
 **/
__attribute__((unused)) static void
OnBasicModeByteReceived (INT8U byte,int BMCInst)
{
 
}

/**
 * @brief Encode the serial packet in IPMI format
 * @param Pkt Pointer to packet
 * @param Len Size of the packet
 * @param EnPkt Pointer to encoded packet
 * @return Size of the Encoded packet
 **/
static INT16U
EncodeSerialPkt (_NEAR_ INT8U* Pkt, INT16U Len, _NEAR_ INT8U* EnPkt)
{
    INT16U index = 0;
    INT16U i;

    /* Put the START Byte */
    EnPkt [index++] = START_BYTE;

    /* Copy the packet */
    for (i = 0; i < Len; i++)
    {
        switch (*(Pkt + i))
        {
            case START_BYTE:
            case STOP_BYTE:
            case HANDSHAKE_BYTE:
            case DATA_ESCAPE_BYTE:
                EnPkt [index++] = DATA_ESCAPE_BYTE;
                EnPkt [index++] = (*(Pkt + i) + 0x10);
                break;

            case ASCII_ESC_BYTE:
                EnPkt [index++] = DATA_ESCAPE_BYTE;
                EnPkt [index++] = (*(Pkt + i) + 0x20);
                break;

            default:
                EnPkt [index++] = (*(Pkt + i));
                break;
        }
    }

    /* Put the STOP Byte */
    EnPkt [index++] = STOP_BYTE;

    return index;
}

static INT16U
DecodeSerialPkt (_NEAR_ INT8U* Pkt, INT16U Len)
{
	uint32	i;
	uint8	byESCByteRecvd=0;

	INT16U index = 0;
	for(i = 0; i < Len; i++)
	{
		/* ESC byte already received so decode the next byte */
		if( byESCByteRecvd )
		{
			switch(Pkt[i])
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
		else if( Pkt[i] == DATA_ESCAPE_BYTE ) /* ESC received just now */
			byESCByteRecvd = 1;
		else /* Normal data */
			Pkt[index++] = Pkt[i];
	}

	return index;
}

// static INT16U
// DecodeSerialPkt (_NEAR_ INT8U* Pkt, INT16U Len, _NEAR_ INT8U* EnPkt)
// {
// 	uint32	i;
// 	uint8	byESCByteRecvd=0;

// 	INT16U index = 0;
// 	for(i = 0; i < Len; i++)
// 	{
// 		/* ESC byte already received so decode the next byte */
// 		if( byESCByteRecvd )
// 		{
// 			switch(Pkt[i])
// 			{
// 				case ENCODED_START_BYTE:
// 					EnPkt[index++] = START_BYTE;
// 					break;
// 				case ENCODED_STOP_BYTE:
// 					EnPkt[index++] = STOP_BYTE;
// 					break;
// 				case ENCODED_HAND_SHAKE_BYTE:
// 					EnPkt[index++] = HANDSHAKE_BYTE;
// 					break;
// 				case ENCODED_DATA_ESCAPE:
// 					EnPkt[index++] = DATA_ESCAPE_BYTE;
// 					break;
// 				case ENCODED_BYTE_ESCAPE:
// 					EnPkt[index++] = BYTE_ESC;
// 					break;
// 				default:
// 					return 0;					
// 			}
			
// 			byESCByteRecvd = 0;
// 		}
// 		else if( Pkt[i] == DATA_ESCAPE_BYTE ) /* ESC received just now */
// 			byESCByteRecvd = 1;
// 		else /* Normal data */
// 			EnPkt[index++] = Pkt[i];
// 	}

// 	return index;
// }


/**
 * @brief Validates the IPMI serial packet checksum.
 * @param Pkt Pointer to packet.
 * @param Len Size of the packet.
 * @return 0 if checksum correct else -1.
 **/
static int
ValidateSerialCheckSum (_FAR_ INT8U* Pkt, INT16U Len)
{
    INT16U i;
    INT8U Sum;

    /* Validate checksum 1 */
    Sum = Pkt [0] + Pkt [1] + Pkt [2];

    if (0 != Sum)
    {
		//printf("checksum tips :seg1 = %d\n", Sum);
        return -1;
    }

    /* Validate checksum 2 */
    Sum = 0;

    for (i = 3; i < Len; i++)
    {
        Sum += Pkt [i];
    }

    if (0 == Sum)
    {
        return 0;
    }
    else
    {             
		//printf("checksum tips :seg2 = %d\n", Sum);
        return -1;
    }
}


/**
 * @brief Receives byte by byte and calls the corresponding Mode.
 * @param Pkt Pointer to packet.
 * @param Len Size of the packet.
 * @return 0 if checksum correct else -1.
 **/
void
OnSerialByteReceived (INT8U byte,int BMCInst)
{

}
