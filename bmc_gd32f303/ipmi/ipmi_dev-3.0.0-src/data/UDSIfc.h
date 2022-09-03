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
 * UDSIfc.h
 *
 * Author: Suresh V <sureshv@amiindia.com>
 *
 *****************************************************************/

#ifndef UDSIFC_H
#define UDSIFC_H

#define MAX_UDS_BUFFER_SIZE 1024*60
#define UDS_SMB_PARAM 1
#define UDS_TIMER_INTERVAL 10

/* This Channel Number is decided based on the Reserved Channel Number List as per IPMI Specification 
The Reserved Channel Numbers are 0x0C-0x0D*/
#define UDS_SMBMC_CHANNEL 0x0C
#define UDS_CHANNEL 0x0D

#define SOCKET_PATH_LEN 108

#define UDS_SUCCESS 0
#define UDS_CMD_FAILURE 1
#define UDS_FAILURE -1
#define UDS_PARAM_FAILURE -2

#endif /* UDSIFC_H */
