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
 *
 * IPMDevice.h
 * IPMDevice Commands Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com> 
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 * 
 *****************************************************************/
#ifndef IPMDEVICE_H
#define IPMDEVICE_H
#include "Types.h"

/*** Extern Declaration ***/




/**
 * @var INT8U g_ACPISysPwrState
 * @brief Contains System ACPI state.
 * @warning Should not be accessed from task other than Message Handler
 **/

 /*Commented it inorder to access the data across the processes and moved it to the
 shared memory structure BMCSharedMem_T in SharedMem.h file*/

//extern INT8U      g_ACPISysPwrState;

/**
 * @var INT8U g_ACPIDevPwrState
 * @brief Contains Device ACPI state.
 * @warning Should not be accessed from task other than Message Handler
 **/

 /*Commented it inorder to access the data across the processes and moved it to the
 shared memory structure BMCSharedMem_T in SharedMem.h file*/

//	extern INT8U      g_ACPIDevPwrState;

/*** Function Prototypes ***/
/**
 * @defgroup apcf1 IPM Device Commands
 * @ingroup apcf
 * IPMI IPM Device Command Handlers. Invoked by the message handler
 * @{
 **/
extern int GetDevID             (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int ColdReset            (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int WarmReset            (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSelfTestResults   (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int MfgTestOn            (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetACPIPwrState      (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetACPIPwrState      (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetDevGUID           (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

INT8U GetDevAddr(void);
void SetDevAddr(INT8U device_addr);

#endif  /* IPMDEVICE_H */
