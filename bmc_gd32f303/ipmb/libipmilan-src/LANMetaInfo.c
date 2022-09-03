/*******************************************************************
********************************************************************
****                                                              **
****    (C)Copyright 2008-2009, American Megatrends Inc.          **
****                                                              **
****    All Rights Reserved.                                      **
****                                                              **
****    5555 , Oakbrook Pkwy, Norcross,                           **
****                                                              **
****    Georgia - 30093, USA. Phone-(770)-246-8600.               **
****                                                              **
********************************************************************
********************************************************************
********************************************************************
**
** LANMetaInfo.c
** LAN Meta Info file
**
** Author: 
*******************************************************************/
#include "Types.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMIConf.h"

IPMILibMetaInfo_T g_LANMetaInfo[] =
{
    {
    "SUPPORT_LAN_IFC",
    "LANIfcTask",
    "LAN",
    CREATE_TASK,                      // Ifc Type
    1,                      // Argument
    0,
    NULL
    },
};


INT8U GetLibMetaInfo (IPMILibMetaInfo_T **pMetaInfo)
{
    *pMetaInfo = &g_LANMetaInfo[0];

    // return the argument count
    return (sizeof(g_LANMetaInfo) / sizeof(g_LANMetaInfo[0]));
}

