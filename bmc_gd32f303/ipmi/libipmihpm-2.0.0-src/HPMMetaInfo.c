/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2002-2012, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200, Norcross,        **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * HPMMetaInfo.c
 * HPM Meta Info file
 *
 * Author: Joey Chen <joeychen@ami.com.tw>
 ******************************************************************/
 
#include "Types.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMIConf.h"

IPMILibMetaInfo_T g_HPMMetaInfo[] =
{
    {
        "SUPPORT_HPM_IFC",
        "g_HPM_CmdHndlr",
        "",
        ADDTO_GRPEXTN_TBL,						// Ifc Type
        0x2C, 					   // NetFn for HPM  
        0x00,
        NULL
    },
};


INT8U GetLibMetaInfo (IPMILibMetaInfo_T **pMetaInfo)
{
    *pMetaInfo = &g_HPMMetaInfo[0];	

    // return the argument count
    return (sizeof(g_HPMMetaInfo) / sizeof(g_HPMMetaInfo[0]));
}




