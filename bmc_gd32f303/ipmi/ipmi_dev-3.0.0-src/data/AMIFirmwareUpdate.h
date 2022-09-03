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
 *****************************************************************
 *AMIFirmware.h
 *
 * AMI Firmware Update IPMI Commands
 *
 * Author: Abhitesh <abhiteshk@amiindia.co.in>
 *
 *****************************************************************/
#ifndef AMI_FIRMWARE_DEVICE_H
#define AMI_FIRMWARE_DEVICE_H

#include "Types.h"

extern int AMIFirmwareCommand (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);

extern int AMIGetReleaseNote (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);


#endif
