/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2014, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
*
* IPMI_AMICmds.h
* AMI specific configuration commands
*
* Author: Winston <winstont@ami.com>
*
******************************************************************/
#ifndef __IPMI_AMICMDS_H__
#define __IPMI_AMICMDS_H__

#include <Types.h>

#define DEBUG_MSG_ENABLED 0x1
#define DEBUG_MSG_DISABLED 0x0

/*
 * AMIControlDebugMsgReq_T
 */
typedef struct
{
    INT8U CtrlDebugMsg;
}PACKED AMIControlDebugMsgReq_T;

/*
 * AMIControlDebugMsgRes_T
 */
typedef struct
{
    INT8U CompletionCode;
    INT8U DebugMsgStatus;
}PACKED AMIControlDebugMsgRes_T;

/*
 * AMIGetDebugMsgStatusRes_T
 */
typedef struct
{
    INT8U CompletionCode;
    INT8U DebugMsgStatus;
}PACKED AMIGetDebugMsgStatusRes_T;

/* AMIGetSDCardPartReq_T */
typedef struct {
    INT8U   Param;
    INT8U   SlotID;
    INT8U   PartitionNum;
} PACKED  AMIGetSDCardPartReq_T;

typedef struct
{
    INT32U  Start;
    INT32U  End;
    INT32U  NumOfBlocks;
    INT8U   Type;
} PACKED PartConfig_T;

typedef union
{
    INT32U SDSize;
    PartConfig_T PartConfig;
    INT8U PartitionCount;
}GetSDInfo_T;

/* AMIGetSDCardPartRes_T */
typedef struct {
    INT8U           CompletionCode;
    GetSDInfo_T SDInfo;
} PACKED  AMIGetSDCardPartRes_T;

/* AMISetSDCardPartReq_T */
typedef struct {
    INT8U   Param;
    INT8U   SlotID;
    INT8U   PartitionNum;
    INT8U   PartitionFormat;
    INT32U  PartitionStart;
    INT32U  PartitionSize;
} PACKED  AMISetSDCardPartReq_T;

/* AMISetSDCardPartRes_T */
typedef struct {
    INT8U   CompletionCode;
} PACKED  AMISetSDCardPartRes_T;

extern int AMIControlDebugMsg(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetDebugMsgStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst);
extern int AMIGetSDCardPartition (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);
extern int AMISetSDCardPartition (_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst);

#endif //__IPMI_AMICMDS_H__


