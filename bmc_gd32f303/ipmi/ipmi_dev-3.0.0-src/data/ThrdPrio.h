/******************************************************************
 ******************************************************************
 ***                                                                                                           **
 ***    (C)Copyright 2012, American Megatrends Inc.                                     **
 ***                                                                                                           **
 ***    All Rights Reserved.                                                                          **
 ***                                                                                                           **
 ***    5555 , Oakbrook Pkwy, Norcross,                                                       **
 ***                                                                                                           **
 ***    Georgia - 30093, USA. Phone-(770)-246-8600.                                  **
 ***                                                                                                           **
 ******************************************************************
 ******************************************************************
 ******************************************************************
 *
 * ThrdPrio.h
 * Initialize IPMI Interface's Thread Priority
 *
 ******************************************************************/

#ifndef _THRDPRIO_H_
#define _THRDPRIO_H_

#include <pthread.h>

extern void SetLANIfcPriority(pthread_t thread_id);

extern void SetUSBIfcPriority(pthread_t thread_id);

extern void SetIPMBIfcPriority(pthread_t thread_id);

extern void SetSSIFIfcPriority(pthread_t thread_id);

extern void SetKCSIfcPriority(pthread_t thread_id);

extern void SetBTIfcPriority(pthread_t thread_id);

extern void SetSerialIfcPriority(pthread_t thread_id);

extern void SetUDSIfcPriority(pthread_t thread_id);

#endif

