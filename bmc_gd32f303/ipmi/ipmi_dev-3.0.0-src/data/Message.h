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
 * message.h
 * Inter task messaging functions.
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 ******************************************************************/
#ifndef MESSAGE_H
#define MESSAGE_H
#include "Types.h"
#include "libipmi.h"


#pragma pack( 1 )

#define IP_ADDR_LEN 4
#define IP6_ADDR_LEN 16
#define COMMON_QUEUE 0

/*-----------------------------------
 * Definitions
 *-----------------------------------*/
#define MSG_PAYLOAD_SIZE 	  100
#define PIPE_NAME_LEN        10

/*-----------------------------------
 * Type definitions
 *-----------------------------------*/
typedef struct
{
    INT32U      Param;                    /* Parameter */
    INT8U       Channel;                  /* Originator's channel number */
    // INT8U       SrcQ [PIPE_NAME_LEN];     /* Originator Queue */
    INT8U       Cmd;                      /* Command that needs to be processed*/
    INT8U       NetFnLUN;                 /* Net function and LUN of command   */
    // INT8U       Privilege;                /* Current privilege level */
    // INT32U      SessionID;                /* Session ID if any */
    // time_t      ReqTime;                  /* Request Timestamp */
  	// INT32U      ReqTime;
    // INT16U       ResTimeOut;              /* response timeout in secs */
    // INT16U      SessionType;               /* Session Type */
    INT8U       IPAddr [4];    /* IPv6 Address */
    INT16U      UDPPort;                  /* UDP Port Number  */
    INT16S      Socket;                   /* socket handle    */
    INT32U      Size;                     /* Size of the data */
    INT8U       Data [MSG_PAYLOAD_SIZE];  /* Data */
} PACKED MsgPkt_T;

typedef struct
{
    INT32U      Param;                    /* Parameter */
    INT8U       Channel;                  /* Originator's channel number */
    CHASSIS_CMD_CTRL       Cmd;           /* Command that needs to be processed*/
    INT8U       NetFnLUN;                 /* Net function and LUN of command   */
    INT32U      Size;                     /* Size of the data */
    INT8U       Data [20];  /* Data */
} PACKED SamllMsgPkt_T;
#pragma pack( )


#endif	/* MESSAGE_H */
