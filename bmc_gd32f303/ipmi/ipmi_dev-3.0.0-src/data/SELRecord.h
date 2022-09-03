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
 * SELRecord.h
 * SEL record structures.
 *
 *  Author: Rama Rao Bisa<ramab@ami.com>
 *
 ******************************************************************/
#ifndef SEL_RECORD_H
#define SEL_RECORD_H

#if defined(__linux__)
#include <dirent.h>
#endif

#include "IPMI_SEL.h"

/*** External Definitions ***/
#define VALID_RECORD        0x5A

/* This record limitation is because the JFFS2 File System
   Flushes only 20 Records and the remaining records which
   is held in RAM will be flushed by the JFFS2 garbage collector
   at anytime */
#define MAX_SEL_RECORD 20

#define MAX_RECORD_ID 65535

#define SEL_RECLAIM_HEAD_NODE(BMCInst)     g_BMCInfo[BMCInst].SELReclaimRepos.HeadeNode
#define SEL_RECLAIM_TAIL_NODE(BMCInst)     g_BMCInfo[BMCInst].SELReclaimRepos.TailNode

#define SEL_RECLAIM_RECORD_HEAD_NODE(BMCInst)     g_BMCInfo[BMCInst].SELReclaimRepos.HeadeRecNode
#define SEL_RECLAIM_RECORD_TAIL_NODE(BMCInst)     g_BMCInfo[BMCInst].SELReclaimRepos.TailRecNode

#define SEL_RECORD_ADDR(BMCInst,Var)          g_BMCInfo[BMCInst].SELReclaimRepos.pSELRecAddr[Var]
#define SEL_PREV_RECORD_ADDR(BMCInst,Var)     g_BMCInfo[BMCInst].SELReclaimRepos.pSELRecAddr[Var].RecAddr->PrevRec
#define SEL_NEXT_RECORD_ADDR(BMCInst,Var)     g_BMCInfo[BMCInst].SELReclaimRepos.pSELRecAddr[Var].RecAddr->NextRec

#define SEL_MALLOC(Var)    (struct Var *)malloc(sizeof(Var))

#define SIGN_SIZE 4

#define SEL_RECLAIM_SECTION_INFO "SELRECLAIMINFO"

#define SEL_SIGNATURE     "signature"
#define SEL_NUMRECORDS    "NumRecords"
#define SEL_ADDTIMESTAMP    "AddTimeStamp"
#define SEL_ERASETIMESTAMP    "EraseTimeStamp"
#define SEL_LASTFILEINDEX    "LastFileIndex"
#define SEL_MAXRECPOS    "MaxRecPos"

#define SEL_FREE(Var)    \
do{                      \
    free(Var);           \
    Var=NULL;            \
}while(0);


#pragma pack( 1 )

/**
 * @struct SELRec_T
 * @brief SEL Record
**/
typedef struct
{
    INT8U               Valid;
    INT8U               Len;
    SELEventRecord_T    EvtRecord;
} PACKED  SELRec_T;


/**
 * @struct SELRepository_T
 * @brief SEL Repository structure.
**/
typedef struct
{
    INT8U       Signature [SIGN_SIZE];      /* $SDR */
    INT16U      NumRecords;
    INT16U      Padding;
    INT32U      AddTimeStamp;
    INT32U      EraseTimeStamp;
    SELRec_T    *SELRecord;

} PACKED  SELRepository_T;


/* * @struct SELRecIDNode
 * @brief SELRecIDNode structure.
**/
typedef struct SELRecIDNode
{ 
    INT16U RecordID;
    struct SELRecIDNode *NextRecID;
 }SELRecIDNode;

/**
 * @struct SELEventNode
 * @brief SELEventNode structure.
**/
typedef struct SELEventNode
{
    INT32U FileIndex;
    SELRec_T SELRecord;
    struct SELEventNode *NextRec;
    struct SELEventNode *PrevRec;
 }SELEventNode;

typedef struct
{  
    INT32U RecPos;
    struct SELEventNode *RecAddr;
}SELRecAddr;

/**
 * @struct SELReclaimInfo_T
 * @brief SELReclaimInfo_T structure.
**/
typedef struct
{
    INT8U Signature[SIGN_SIZE];
    INT16U NumRecords;
    INT32U AddTimeStamp;
    INT32U EraseTimeStamp;
    INT32U LastFileIndex;
    INT32U MaxRecPos;
}PACKED SELReclaimInfo_T;

/**
 * @struct SELReclaimRepository_T
 * @brief SELReclaimRepository_T structure.
**/
typedef struct
{
/* The Node Pointer needs to be maintained
 * at the top in order to avoid structure
 * alignment issues */
    SELEventNode *HeadeNode;
    SELEventNode *TailNode;
    SELRecIDNode *HeadeRecNode;
    SELRecIDNode *TailRecNode;
    SELRecAddr *pSELRecAddr;
    SELReclaimInfo_T *pSELReclaimInfo;
}PACKED SELReclaimRepository_T;

#pragma pack( )


/* SEL Reclaim Function Prototypes */
extern int DeleteSELRecordIDHeadNode(SELRecIDNode **HeadNode,SELRecIDNode **TailNode);
extern int DeleteSELReclaimNode(SELEventNode **HeadNode,SELEventNode **TailNode,INT16U RecordID,unsigned int *FileIndex,int BMCInst);
extern int AddSELRecalimNode(SELEventNode **HeadNode,SELEventNode **TailNode,SELRec_T *SELRecord,unsigned int FileIndex,int BMCInst);
extern INT16U GetNextReclaimRecordID(int BMCInst);
extern int FlushAddReclaimSEL(SELRec_T *SELRecord,int BMCInst);
extern int ReFlushDeleteReclaimSEL(INT16U RecordID,int BMCInst);
extern int AddSELRecordIDNode(SELRecIDNode **HeadNode,SELRecIDNode **TailNode,INT16U RecordID,int BMCInst);
extern INT32U GetReclaimFileSize(char *FileName);
extern void DeleteReclaimAllSELNode(SELEventNode **HeadNode,SELEventNode **TailNode,int BMCInst);
extern int UpdateRecordIDList(INT8U *RecordID,int BMCInst);
extern int CopySELReclaimEntries(SELEventNode **HeadNode,INT8U *RecordData,INT32U NumEntriesRetrd,INT32U NumOfEntries,int BMCInst);
extern int LoadSELReclaimInfo( SELReclaimInfo_T *SELInfo,int BMCInst);
extern int SaveSELReclaimInfo( SELReclaimInfo_T *SELInfo,int BMCInst);


#endif  /* SEL_RECORD_H */
