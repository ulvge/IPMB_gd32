/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2014, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 *****************************************************************
 *
 * SSIFIFc.H
 * 
 *
 * Author: YiShan Chen <yishanchen@ami.com.tw>
 *
 * 
 *****************************************************************/
#ifndef SSIFIFC_H
#define SSIFIFC_H

#include "Types.h"

#define SSIF_RECV_Q                   MSG_PIPES_PATH "SSIFRecvQ"

/*** External Definitions ***/
#define BMC_SLAVE_ADDRESS   0x20
#define BMC_LUN             0x00
#define MAX_SSIF_REQ_PKT_SIZE   255    
#define MAX_SSIF_RES_PKT_SIZE   128    //It's equal to MAX_IPMB_MSG_SIZE in i2c-core.c

#define SSIF_REQUEST			1

#endif /* SSIFIFC_H	*/
