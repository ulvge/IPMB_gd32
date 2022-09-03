/* ***************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2012, American Megatrends Inc.             **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200,  Norcross,       **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************/
/****************************************************************
 *
 * HPM.h
 * HPM Command Handler
 *
 * Author: Joey Chen <JoeyChen@ami.com.tw>
 *
 *****************************************************************/
#ifndef HPM_H
#define HPM_H

#include "MsgHndlr.h"


/*** Extern Definitions ***/
extern const CmdHndlrMap_T g_HPM_CmdHndlr []; /**< HPM specific command handler table. */
/**
 * @defgroup acf HPM Specific Module
  * IPMI HPM Specific Command Handlers. Invoked by the message handler
 * @{
 **/
/** @} */

#endif  /* HPM_H */
