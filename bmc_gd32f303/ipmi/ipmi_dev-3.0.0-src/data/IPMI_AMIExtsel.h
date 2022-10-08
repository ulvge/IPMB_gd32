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
* IPMI_Extsel.h
* AMI specific Extended SEL configurations
*
* Author: Winston <winstont@ami.com>
*
******************************************************************/
#ifndef __IPMI_AMIEXTSEL_H__
#define __IPMI_AMIEXTSEL_H__

#include <Types.h>

#define MAXEXTSELINFILE 100
#define MAXRECORDID 0xFFFE
#define  MAXRECORDFILE     (MAXRECORDID/MAXEXTSELINFILE)

#ifdef CONFIG_SPX_FEATURE_EXTENDED_SEL_BUFFER_SIZE
#define  EXTSELBUFFER       CONFIG_SPX_FEATURE_EXTENDED_SEL_BUFFER_SIZE
#else 
#define  EXTSELBUFFER       128
#endif 

#define EXTENDEDSELFOLDER "/ExtendSELData"
#define SPIPATH      "/conf"
#define EMMCPATH      "/mnt/sdmmc0p"
#define STATUS_DELETE_SEL           0xA5
#define SEL_INDEX_SIZE              16
#define SEL_FILE_PATH_LEN        70
#define SELCMD_SIZE        40
#define EXTENDED_SEL_PATH_LENGTH        512
#define EXTENDED_SEL_FILE_PATH_LENGTH   512

#define EXTENDED_SEL_PARTIAL_ADD_CMD_MAX_LEN				256
#define EXTENDED_SEL_RECORD_SIZE							249
#define EXTENDED_SEL_MAX_RECORD_SIZE			0x0800

typedef struct 
{
    INT16U  ID;
    INT8U   Type;
    INT32U  TimeStamp;
    INT8U   GenID [2];
    INT8U   EvMRev;
    INT8U   SensorType;
    INT8U   SensorNum;
    INT8U   DirType;
    INT8U	Signature;
    INT16U	offset;
} PACKED ExtendSELHead_T;

typedef struct
{
	INT16U  RecID;
    
}PACKED AMIGetExtendSELDataReq_T;

typedef struct
{
    INT8U   CompletionCode;
    INT8U	DataSize;
    INT8U   ExtendSELData[EXTSELBUFFER];
}PACKED AMIGetExtendSELDataRes_T;

//Add Extend SEL entries
typedef struct
{
    INT8U   CompletionCode;
    INT16U  RecID;
}PACKED AddExtendSELRes_T;

/* OEMExtendSELReq_T */
typedef struct 
{
	ExtendSELHead_T ExtendSELHead;
    /* RECORD BODY BYTES */
	INT8U  ExtendSELData[EXTSELBUFFER];
} PACKED AddExtendSELReq_T;

/* PartialExtendedAddSELReq_T */
typedef struct
{
    INT8U      LSBReservationID;
    INT8U      MSBReservationID;
    INT8U      LSBRecordID;
    INT8U      MSBRecordID;
    INT16U     Offset;
    INT8U      Progress;
    INT8U      ESELRecordData[EXTENDED_SEL_RECORD_SIZE];

} PACKED  AMIPartialAddExtendSELReq_T;

/* PartialAddSELRes_T */
typedef struct
{
    INT8U   CompletionCode;
    INT16U  RecID;
} PACKED  AMIPartialAddExtendSELRes_T;


typedef struct
{
    INT16U    RecID;
    INT16U    ExtendSELOffset;
}PACKED AMIPartialGetExtendSELDataReq_T;

typedef struct
{
    INT8U    CompletionCode;
    INT16U   DataSize;
    INT8U    Progress;
    INT16U   RemainingBytes;
    INT8U    ExtendSELData[250];		// 250+ 6 Bytes
}PACKED AMIPartialGetExtendSELDataRes_T;

extern int AMIAddExtendSelEntries(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIPartialAddExtendSelEntries(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIGETExtendSelData(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);
extern int AMIPartialGetExtendSelEntries(INT8U *pReq, INT32U ReqLen, INT8U *pRes, int BMCInst);


#endif//__IPMI_AMIEXTSEL_H__

