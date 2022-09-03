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
 *
 * ipmi_bt.h
 * BT specific IPMI structures.
 *
 *  Author: maheswaria <maheswaria@amiindia.co.in>
 ******************************************************************/
#ifndef IPMI_BT_H
#define IPMI_BT_H
#include "Types.h"
#include "PDKAccess.h"

#pragma pack( 1 )

/* BT Request Structure */
typedef struct
{
    INT8U   NetFnLUN;
    INT8U   Cmd;

} PACKED  BTReq_T;
#pragma pack( )


/**
 * @def SET_SMS_ATN
 * @brief Macro to set the SMS Attention bit via BT interface.
**/
#define SET_BT_SMS_ATN(BTIFC_NUM, BMCInst)       \
    do{                                                            \
        if(g_PDKHandle[PDK_SETBTSMSATTN] != NULL)                   \
        {                                                                                           \
            ((void(*)(INT8U, int))g_PDKHandle[PDK_SETBTSMSATTN]) (BTIFC_NUM, BMCInst);   \
        }                                                                                                                     \
    }while(0);

/**
 * @def CLEAR_BT_SMS_ATN
 * @brief Macro to reset the SMS attention bit via BT interface.
**/
#define CLEAR_BT_SMS_ATN(BTIFC_NUM, BMCInst)       \
do{                                                            \
    if(g_PDKHandle[PDK_CLEARBTSMSATTN] != NULL)                   \
    {                                                                                           \
        ((void(*)(INT8U, int))g_PDKHandle[PDK_CLEARBTSMSATTN]) (BTIFC_NUM, BMCInst);   \
    }                                                                                                                     \
}while(0);

#endif	/* IPMI_BT_H */

