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
 * SerialIfc.h
 * Serial Interface Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com> 
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 * 
 *****************************************************************/
#ifndef SERIALIFC_H
#define SERIALIFC_H

#include "Types.h"
#include "bsp_usart0.h"
#include "bsp_usart1.h"
#include "bsp_uart3.h"
#include "bsp_uart7.h"
#include "main.h"

/*** Macro Definitions ***/

#define	SERIAL_COMM_REQUEST				1
#define	SERIAL_SNMP_REQUEST				2
#define	SERIAL_CONNECT_TO_MODEM			4
#define	SERIAL_ALERT_REQUEST			5
#define	SERIAL_CLOSE_SESS_REQUEST		6
#define	SERIAL_INIT_CALLBACK_REQUEST	7
#define	SERIAL_PING_REQUEST				8
#define	SERIAL_TERM_MODE_REQUEST		9

#define BASIC_MODE_CALLBACK				3
#define	PPP_MODE_CALLBACK				4

#define serial_write(dat, len)          UART_sendData(IPMI_UART_PERIPH, dat, len)
/**
 * @def MAX_SERIAL_PKT_SIZE			    
 * @brief Maximum serial buffer size 
 **/
#define MAX_SERIAL_PKT_SIZE			(100) 


/*** Extern Definitions ***/

/*** Global Variables ***/

/**
 * @var INT8U	g_SerialPkt [MAX_SERIAL_PKT_SIZE]
 * @brief Serial buffer. Interrupt puts bytes in this buffer
 * @warning Must be used by the serial interface task and serial inetrrupt
 **/
//extern INT8U g_SerialPkt [MAX_SERIAL_PKT_SIZE];

/**
 * @var INT8U	g_SerialPktIx
 * @brief Serial buffer index. 
 * @warning Must be used by the serial interface task and serial inetrrupt
 **/
//extern INT16U g_SerialPktIx;

/*** Function Prototypes ***/

/**
 * @brief Called by interrupt after receiving a byte through serial port
 * @param Port Serial Port number
 * @param byte a byte data received throught port
 **/
extern void OnSerialByteReceived (INT8U byte,int BMCInst);

/**
 * @brief Hangs up the modem
 **/
extern void HangUpModem (void);
extern bool ProcessSerialReq (MsgPkt_T *pReq, MsgPkt_T *pRes);
extern INT16U   EncodeSerialPkt         (INT8U* Pkt, INT16U Len, INT8U* EnPkt, INT8U enPktLength);
#endif /* SERIALIFC_H */
