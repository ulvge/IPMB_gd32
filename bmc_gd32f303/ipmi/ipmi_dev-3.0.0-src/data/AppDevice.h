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
 ****************************************************************
 *
 * AppDevice.h
 * AppDevice Commands Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#ifndef APPDEVICE_H
#define APPDEVICE_H

#include "Types.h"
//#include "OSPort.h"

#define EVT_MSG_BUF_SIZE    1  /**<Event Message maximum Buffer size */
#define USER_ID             (((INT32U)'U'<< 24) | ((INT32U)'S'<< 16) | ('E'<<8) | 'R')
#define IPMI_ROOT_USER      ( 2 )
#define TWENTY_BYTE_PWD  0x80
#define CURRENT_CHANNEL_NUM		0x0E
#define MASTER_RW_ERRCODE 0xFF
#define IGNORE_ADD_OR_REMOVE 0
#define IGNORE_ADD_OR_REMOVE_SHELL -1
#define IPMI_15_PASSWORD_LEN (16 + 2)
#define IPMI_20_PASSWORD_LEN (20 + 2)
#define SYS_SEND_MSG_LUN 0x02

/*** Extern Declaration ***/


/*** Function Prototypes ***/
/**
 * @defgroup apcf2 BMC Watchdog Timer Commands
 * @ingroup apcf
 * IPMI BMC Watchdog Timer Command Handlers. Invoked by the message handler
 * @{
 **/
extern int      ResetWDT            (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      SetWDT              (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetWDT              (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

/**
 * @defgroup apcf3 BMC Device and Messaging Commands
 * @ingroup apcf
 * IPMI BMC Device and Messaging Command Handlers. Invoked by the message handler
 * @{
 **/
extern int      SetBMCGlobalEnables (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetBMCGlobalEnables (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      ClrMsgFlags         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetMsgFlags         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      EnblMsgChannelRcv   (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetMessage          (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      SendMessage         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      ReadEvtMsgBuffer    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetBTIfcCap         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetSystemGUID       (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetChAuthCap        (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetSessionChallenge (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      ActivateSession     (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      SetSessionPrivLevel (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      CloseSession        (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetSessionInfo      (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetAuthCode         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      SetChAccess         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetChAccess         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetChInfo           (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      SetUserAccess       (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetUserAccess       (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      SetUserName         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      GetUserName         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      SetUserPassword     (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int      MasterWriteRead     (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int		SetSystemInfoParam  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int 		GetSystemInfoParam  (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern INT8U  IsChannelSuppGroups(INT8U ChannelNum, int BMCInst);
extern int ModifyUsrGrp(char * UserName,INT8U ChannelNum,INT8U OldAccessLimit, INT8U NewAccessLimit );

/** @} */

#endif  /* APPDEVICE_H */

