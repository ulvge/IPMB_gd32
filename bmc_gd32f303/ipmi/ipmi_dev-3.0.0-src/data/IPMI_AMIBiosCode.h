/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy, Building 200, Norcross,         **
 **                                                            **
 **        Georgia 30093, USA. Phone-(770)-246-8600.           **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * IPMI_AMIBiosCode.h
 * 
 *
 * Author: Gokulakannan. S <gokulakannans@amiindia.co.in>
 *****************************************************************/


#ifndef IPMI_AMIBIOSCODE_H_
#define IPMI_AMIBIOSCODE_H_

#define MAX_SIZE 256

#define CURRENT     0x00
#define PREVIOUS    0x01
#define MAX_BUFFER_SIZE  200
#define MAX_FILE_SIZE    50
#define BIOS             "BIOS"
#define BIOS_FLAG        "BIOS_FLAG"
#pragma pack (1)

typedef struct
{
    INT8U Command;
} PACKED AMIGetBiosCodeReq_T;

typedef struct
{
    INT8U CompletionCode;
    INT8U BiosCode[MAX_SIZE];
} PACKED AMIGetBiosCodeRes_T;

typedef struct
{
    INT8U Sequence_ID;
    INT8U Action;
    INT8U Request_Data_Length;
    INT8U Variable_data[MAX_BUFFER_SIZE];
} PACKED AMISendToBiosReq_T;

typedef struct
{
    INT8U CompletionCode;
    INT8U Sequence_ID;
    
} PACKED AMISendToBiosRes_T;


typedef struct
{
    INT8U CompletionCode;
    INT8U Sequence_ID;
    INT8U Action;
    INT8U Request_Data_Length;
    INT8U Variable_data[MAX_BUFFER_SIZE];
} PACKED AMIGetBiosCommandRes_T;



typedef struct
{
    INT8U Sequence_ID;
    INT8U Response_Data_Length;
    INT8U Response_data[MAX_BUFFER_SIZE];

} PACKED AMISetBiosReponseReq_T;
typedef struct
{
    INT8U CompletionCode;
} PACKED AMISetBiosReponseRes_T;


typedef struct
{
    INT8U Sequence_ID;
} PACKED AMIGetBiosReponseSReq_T;

typedef struct
{
    INT8U CompletionCode;
    INT8U Response_Data_Length;
    INT8U Response_data[MAX_BUFFER_SIZE];
} PACKED AMIGetBiosReponseSRes_T;


typedef struct
{
	INT32U MASK;
    INT32U BIOS_Flag;
} PACKED AMISetBiosFlagReq_T;

typedef struct
{
    INT8U CompletionCode;
} PACKED AMISetBiosFlagRes_T;


typedef struct
{
    INT8U CompletionCode;
    INT32U BIOS_Flag;
} PACKED AMIGetBiosFlagRes_T;

typedef struct
{
    INT8U Sequence_ID;
    INT8U Action;
    INT8U Data_Length;
    INT8U Variable_data[MAX_BUFFER_SIZE];
} PACKED BIOS_Control_buffer_T;

/****** PLDM Related Structures *******/

typedef enum
{
 START = 0x1,                             /*Start, which is the first part of the data transfer*/
 MIDDLE,                                 /*Middle, which is neither the first nor the last part of the data transfer*/
 END =0x4,                                    /*End, which is the last part of the data transfer*/
 STARTEND=0x5,                             /*StartAndEnd, which is the first and the last part of the data transfer*/
 STARTREAD=0x6,
 MIDDLEREAD
}TransferFlag;

typedef enum
{
 GETNEXTPART=0,	
 GETFIRSTPART
}TransferOperationFlag;


typedef struct 
{
  INT8U InstanceID;  
  INT8U PLDMType;
  INT8U PLDM_CommandCode;
}PACKED PLDMMsgCmdFields_T;

typedef struct
{
  uint32  DataTransferHandle;
  INT8U TransferFlag;
  INT8U TableType;
}PACKED PLDMTransferInfo_T;

typedef struct
{ 
  PLDMMsgCmdFields_T PLDMMsgFields;
  PLDMTransferInfo_T PLDMTransferInfo;
}PACKED SetGetPLDMBIOSTableReq_T;                  

typedef  struct
{
   INT8U CompletionCode;
   PLDMMsgCmdFields_T PLDMMsgFields;
   INT8U PLDM_CompletionCode;
   uint32 NextDataTransferHandle;
}PACKED PLDMBIOSTableResponse_T;

typedef union
{
  INT8U TransferFlag;
}PACKED CommonPLDM_T;

typedef struct
{
  PLDMBIOSTableResponse_T PLDMBIOSRes;
  CommonPLDM_T CommonPLDM;
}PACKED SetGetPLDMBIOSTableRes_T;  

typedef struct
{
        INT8U TableType;                        //BIOSStringTable=0x0, BIOSAttributeTable=0x1, BIOSAttributeValueTable=0x2
        INT32U TableTag;
}PACKED SetRequest_T;

#pragma pack ()

#endif /* IPMI_AMIBIOSCODE_H_ */
