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
 * LANIfc.h
 * LAN Interface Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#ifndef LANIFC_H
#define LANIFC_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include "Types.h"
#include "Message.h"
#include "IPMI_LANIfc.h"


/*** External Definitions ***/
#define LAN_SMB_REQUEST				1
#define LAN_SNMP_REQUEST			2
#define LAN_SM_REQUEST				3
#define LAN_ARP_REQUEST				4
#define GRATUITOUS_ARP_REQUEST		5
#define PCI_RST_INTR				6
#define LAN_SMB_ALERT				6
#define INIT_SMB					7
#define LAN_ICTS_MODE				8
#define VLAN_SMB_REQUEST			9
#define LOOP_BACK_LAN_SMB_REQUEST	10
#define LAN_IFC_READY				11

#define LAN_CONFIG_IPV4_HEADER      12
#define LAN_RMCP_PORT_CHANGE     13
#define LAN_CONFIG_IPV6_HEADER 14
#define LAN_IFC_UP 1
#define LAN_IFC_DOWN 0
#define FLAG_SET 1
#define FLAG_UNSET 0

typedef struct
{
    int  Socket;
    INT8U   Valid;
    INT8U  IsLoopBackSocket;
    INT8U  IsFixedSocket;
}PACKED SocketTbl_T;


/**
 * This variable is used to define if the packet
 * is from VLAN channel or not 
 */
//extern INT8U	g_IsPktFromVLAN;

/**
 * @defgroup lanifc LAN Interface Module
 * LAN interface functions.
 * @{
**/
extern BOOL	ValidateUDPChkSum	(UDPPseudoHdr_T* pUDPPseudoHdr, UDPHdr_T* pUDPHdr);
extern BOOL	ValidateIPChkSum	(IPHdr_T* pIPHdr);
extern INT16U CalculateUDPChkSum (UDPPseudoHdr_T* pUDPPseudoHdr, UDPHdr_T* pUDPHdr);
extern INT16U CalculateIPChkSum	(IPHdr_T* pIPHdr);


//extern int SendLANPkt (MsgPkt_T *pRes);
//extern void ProcessLANReq (const MsgPkt_T* pReq, MsgPkt_T *pRes);

//extern int SendSOLPkt  (INT8U* pSOLPkt, INT16U Len);
/** @} */


#ifdef __cplusplus
}
#endif

#endif /* LANIFC_H */
