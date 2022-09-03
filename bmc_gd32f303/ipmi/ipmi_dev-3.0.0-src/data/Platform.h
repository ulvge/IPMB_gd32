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
 * PlatformPort.h
 * Platform porting functions and defines.
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 ******************************************************************/
#ifndef PLATFORM_PORT_H
#define PLATFORM_PORT_H

#include "Types.h"


/*** Extern Declarations ***/

extern int InitMultiplePlatform(void);
/*** Functions Prototypes ***/
/**
 * @brief Initialises the Platform.
 * @return 0 if success else -1
 **/
extern int Platform_Init (void);

/**
 * @brief Platform setup performed after shared memory initialized.
 * @return 0 if success else -1
 **/
extern int Platform_Init_SharedMem (void);

/**
 * @brief Platform setup performed after IPMI stack running.
 * @return 0 if success else -1
 **/
extern int Platform_Init_Done (void);

/**
 * @brief Warm resets the BMC.
 **/
extern void Platform_WarmReset (int BMCInst);

/**
 * @brief Cold resets the BMC.
 **/
extern void Platform_ColdReset (int BMCInst);

/**
 * @brief Powers up the Host.
 **/
extern void Platform_HostPowerUp (int BMCInst);

/**
 * @brief Powers Off the Host.
 **/
extern void Platform_HostPowerOff (int BMCInst);

/**
 * @brief Powers on the Host.
 * @return 0 if success else -1.
 **/
extern BOOL Platform_HostPowerOn (int BMCInst);

/**
 * @brief Power cycles the Host.
 **/
extern void Platform_HostPowerCycle (int BMCInst);

/**
 * @brief Soft Shutdowns the Host.
 **/
extern void Platform_HostSoftShutDown (int BMCInst);

/**
 * @brief Resets the Host.
 **/
extern void Platform_HostColdReset (int BMCInst);

/**
 * @brief Generates the Diagnodtic (NMI) interrupt.
 **/
extern void Platform_HostDiagInt (int BMCInst);

/**
 * @brief Identifies the platform
 **/
extern void Platform_Identify (INT32U Timeout, int Force, int BMCInst);

extern void ResetSetInProgressParam (int BMCInst);


#endif  /* PLATFORM_PORT_H */
