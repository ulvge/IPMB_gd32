/*******************************************************************
********************************************************************
****                                                              **
****    (C)Copyright 2013, American Megatrends Inc.               **
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
 ** MBMCInfo.h
 ** Multiple BMC related functions.
 **
 **  Author: Muthuchamy Kumar <muthusamyk@amiindia.co.in>
 *******************************************************************/
 #ifndef _MBMCINFO_H_
 #define _MBMCINFO_H_
#include "Types.h"
#include <stdio.h>

#define MAX_FILE_NAME       128
#define MAX_FILE_PATH_SIZE  256
#define MAX_FILE_TRANSFER   15
#define MAX_PKT_LEN         (50 * 1024)
#define MAX_DATA_CMD_LEN         1024


#define FILE_TRANSFER_INPROGRESS    1
#define FILE_TRANSFER_COMPLETED     2

#define FILE_UP_OPCODE_OFFSET       4

typedef struct
{
    INT8U TransID;
    INT8U Inprogess;
    FILE  *fp;
    INT8S FilePath[MAX_FILE_PATH_SIZE];
}FileTrans_T;

typedef struct
{
    INT32U UpTimeout[MAX_FILE_TRANSFER];
    INT32U DownTimeout[MAX_FILE_TRANSFER];
}FileTransMonitor_T;


 extern FileTrans_T* GetFileUpTrackInfo(INT8U TransID);
 extern FileTrans_T* GetFileDownTrackInfo(INT8U TransID);
 extern int GetFilePath(INT8U OpCode,char *FilePath);
 extern INT8U GetTransID(void);
 extern void* FileTimeoutTask(void* pArg);

 #endif

