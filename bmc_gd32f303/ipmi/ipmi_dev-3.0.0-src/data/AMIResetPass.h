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
 ******************************************************************
 *
 * AmiResetPass.h
 * AMI Reset Password commands Macros
 *
 * Author: Gokula Kannan. S	<gokulakannans@amiindia.co.in>
 *
 ******************************************************************/
#ifndef __AMIRESETPASS_H__ 
#define __AMIRESETPASS_H__

#include "Types.h"
#include "smtpclient.h"

//Local Macros for Linux root user access
#define LINUX_USER_MIN_PASSWORD_LEN   8
#define LINUX_USER_MAX_PASSWORD_LEN   64
#define OP_ONLY_CMD_LENGTH       1
#define OP_SET_ROOT_PASSWD       2
#define OP_ENABLE_USER_ID        1
#define OP_DISABLE_USER_ID       0
#define LINUX_ROOT_USER_UID      0 

extern int AMIResetPassword(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int AMIGetEmailForUser(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int AMISetEmailForUser(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int AMIGetEmailFormatUser(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);
extern int AMISetEmailFormatUser(INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst);


#endif // __AMISMTP_H__ 
