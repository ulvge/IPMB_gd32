/****************************************************************
 **                                                            **
 **    (C)Copyright 2007-2008, American Megatrends Inc.        **
 **                                                            **
 **                   All Rights Reserved.                     **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200, Norcross,        **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************/

 /****************************************************************
 * @file	OBSMBladeStateMngr.h
 * @author	Velu	<velmuruganv@amiindia.co.in>
 * @brief
 ****************************************************************/
#ifndef OBSMBLADESTATEMNGR_H
#define OBSMBLADESTATEMNGR_H

#include "Types.h"

/*** Definitions and Macros ***/

/*** Typedef ***/

/*** Extern Definitions ***/

/*** Function Prototypes ***/
/**
 * @brief Process the OBSM blade state routines on every tick
 * @param pBlade is the Blade object
 * @return None
**/
extern void OBSM_ProcessBladeStateOnTimerTick (BladeInfo_T *pBlade, int BMCInst);

/**
 * @brief Process all OBSM blade state transitions
 * @param Blade is the Blade object
 * @param pEvent is the SEL Event containing all related information
 * @return 0 if success, -1 if error
**/
extern int OBSM_ProcessBladeStateChange (BladeInfo_T *pBlade, SELEventRecord_T *pEvent, int BMCInst);

#endif /*OBSMBLADESTATEMNGR_H*/
