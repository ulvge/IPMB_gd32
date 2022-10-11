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
 * Serial.h
 * Serial Packet Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com> 
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 * 
 *****************************************************************/
#ifndef SERIAL_H
#define SERIAL_H

#include "Types.h"
#include "Message.h"

/*** Macro Definitions **/
#define SERIAL_MODE_IDLE        0
#define SERIAL_BASIC_MODE       1
#define SERIAL_PPP_MODE         2
#define SERIAL_TERMINAL_MODE    3
#define MODEM_CONNECT_MODE      4
#define MODEM_HANGUP_MODE       5
#define SERIAL_DIAL_PAGE_MODE   6
#define SERIAL_TAP_PAGE_MODE    7

#define START_BYTE              0xA0
#define STOP_BYTE               0xA5
#define HANDSHAKE_BYTE          0xA6
#define DATA_ESCAPE_BYTE        0xAA
#define	BYTE_ESC				0x1B

#define		ENCODED_START_BYTE			0xB0
#define		ENCODED_STOP_BYTE			0xB5
#define		ENCODED_HAND_SHAKE_BYTE		0xB6
#define		ENCODED_DATA_ESCAPE			0xBA
#define		ENCODED_BYTE_ESCAPE			0x3B


#define BMC_SLAVE_ADDR          0x20
#define SECOND_CHECK_BYTE       BMC_SLAVE_ADDR

#define INVOKE_SERVICE_PROC_SEQ     '('
#define EXIT_SERVICE_PROC_SEQ       'Q'
#define INVOKE_POWERUP_PROC_SEQ '^'
#define INVOKE_POWERRST_PROC_SEQ1 'R'
#define INVOKE_POWERRST_PROC_SEQ2 'r'
#define INVOKE_POWERRST_PROC_SEQ3 'R'
#define POWERUP_SWITCH_ENABLE   BIT6
#define POWERRST_SWITCH_ENABLE  BIT5
#define BMC2SYS_SWITCH_ENABLE   BIT2
#define SYS2BMC_SWITCH_ENABLE   BIT1

#define TERMINAL_UNUSED         0xFF

/*** Extern Declarations ***/

/*** Global Variables ***/
/**
 * @var INT32U g_SerialSessionID
 * @brief Contains the Serial interface session ID
 * @warning Must be used by only serial interface task
 **/
//extern INT32U g_SerialSessionID;

/**
 * @var INT8U g_SerialSessionActive
 * @brief Contains the Serial interface session ID
 * @warning Must be used by only serial interface task
 **/
//extern INT8U g_SerialSessionActive;

/**
 * @brief Processes the IPMI requests received from Serial interface
 * @param pReq Pointer to request message packet
 * @param pRes Pointer to response message packet
 * @return Size of the response data
 **/
extern INT16U ProcessSerialMessage (MsgPkt_T* pReq, MsgPkt_T* pRes,int BMCInst);

#endif /* SERIAL_H */
