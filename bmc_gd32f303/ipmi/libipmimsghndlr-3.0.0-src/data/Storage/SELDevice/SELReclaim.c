/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2012, American Megatrends Inc.         ***
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
* File Name :- SELReclaim.c
* Description :- Delete SEL Space Reclaim using Linked List/
                 Split File  Mechanism
*
* Author: Suresh Vijayakumar (sureshv@amiindia.co.in)
*
* Revision History:
* 10/4/2012
*     Author : Suresh Vijayakumar
*     Added Support for File Merging on Delete SEL Entries.
*
* 12/5/2012
*     Author : Suresh Vijayakumar
*     Added Migration Support (INI Format for SEL Reclaim 
      Information alone).
*****************************************************************/
#define ENABLE_DEBUG_MACROS 0

#include "Debug.h"
#include "IPMIConf.h"
#include "parse-ex.h"

/**
*@fn AddSELReclaimNode
*@brief This function is invoked to Add the SEL Record to a Node of linked List
*@return Returns 0 on success
*@return Returns -1 on failure
*/
int AddSELRecalimNode(SELEventNode **HeadNode,SELEventNode **TailNode,SELRec_T *SELRecord,unsigned int FileIndex,int BMCInst)
{
    struct SELEventNode *CurrentNode = NULL;
    if(*HeadNode == NULL)
    {
         CurrentNode  = SEL_MALLOC(SELEventNode);
         if( CurrentNode  == NULL )
         {
             IPMI_ERROR("Cannot Allocate Memory for SEL Reclaim Node\n");
             return -1;
         }

         memcpy(&CurrentNode->SELRecord,SELRecord,sizeof(SELRec_T));
         CurrentNode->FileIndex = FileIndex;
         CurrentNode->NextRec = NULL;
         CurrentNode->PrevRec = NULL;

         /* Assign the Current Node as this is the Head Node Now */
         *HeadNode = CurrentNode;

         SEL_RECORD_ADDR(BMCInst,SELRecord->EvtRecord.hdr.ID).RecAddr=CurrentNode;

         g_BMCInfo[BMCInst].SELReclaimRepos.pSELReclaimInfo->MaxRecPos=0;
         SEL_RECORD_ADDR(BMCInst,SELRecord->EvtRecord.hdr.ID).RecPos=++g_BMCInfo[BMCInst].SELReclaimRepos.pSELReclaimInfo->MaxRecPos;
         TDBG("Adding first SEL %d \n",g_BMCInfo[BMCInst].SELReclaimRepos.pSELReclaimInfo->MaxRecPos);

         LOCK_BMC_SHARED_MEM(BMCInst);
         BMC_GET_SHARED_MEM(BMCInst)->m_FirstRecID = SELRecord->EvtRecord.hdr.ID;
         BMC_GET_SHARED_MEM(BMCInst)->m_LastRecID = SELRecord->EvtRecord.hdr.ID;
         UNLOCK_BMC_SHARED_MEM(BMCInst);

         CurrentNode = NULL;

         return 0;
    }
    else
    {
         if(*TailNode == NULL)
         {
             CurrentNode = *HeadNode;
         }
         else
         {
             CurrentNode = *TailNode;
         }

         CurrentNode->NextRec = SEL_MALLOC(SELEventNode);
         if(CurrentNode->NextRec == NULL )
         {
             IPMI_ERROR("Cannot Allocate Memory for SEL Reclaim Node\n");
             return -1;
         }

         memcpy(&(CurrentNode->NextRec)->SELRecord,SELRecord,sizeof(SELRec_T));
         (CurrentNode->NextRec)->FileIndex = FileIndex;
         (CurrentNode->NextRec)->NextRec = NULL;
         (CurrentNode->NextRec)->PrevRec = CurrentNode;

         SEL_RECORD_ADDR(BMCInst,SELRecord->EvtRecord.hdr.ID).RecAddr=CurrentNode->NextRec;

         SEL_RECORD_ADDR(BMCInst,SELRecord->EvtRecord.hdr.ID).RecPos=++g_BMCInfo[BMCInst].SELReclaimRepos.pSELReclaimInfo->MaxRecPos;
         TDBG("Adding first SEL %d \n",g_BMCInfo[BMCInst].SELReclaimRepos.pSELReclaimInfo->MaxRecPos);

         *TailNode = CurrentNode->NextRec;

         LOCK_BMC_SHARED_MEM(BMCInst);
         BMC_GET_SHARED_MEM(BMCInst)->m_LastRecID = SELRecord->EvtRecord.hdr.ID;
         UNLOCK_BMC_SHARED_MEM(BMCInst);

         CurrentNode = NULL;

         return 0;
    }
}

/**
*@fn DeleteReclaimAllSELNode
*@brief This function is invoked to Delete the All SEL Record Node
*@return Returns none
*/

void DeleteReclaimAllSELNode(SELEventNode **HeadNode,SELEventNode **TailNode,int BMCInst)
{
     struct SELEventNode *CurrentNode = NULL;

     CurrentNode = *HeadNode;
     while(CurrentNode != NULL)
     {
          *HeadNode = CurrentNode->NextRec;
          SEL_RECORD_ADDR(BMCInst, CurrentNode->SELRecord.EvtRecord.hdr.ID).RecAddr = NULL;
          SEL_RECORD_ADDR(BMCInst, CurrentNode->SELRecord.EvtRecord.hdr.ID).RecPos = 0;
          AddSELRecordIDNode(&(SEL_RECLAIM_RECORD_HEAD_NODE(BMCInst)),&(SEL_RECLAIM_RECORD_TAIL_NODE(BMCInst)),
                                               CurrentNode->SELRecord.EvtRecord.hdr.ID,BMCInst);
          SEL_FREE(CurrentNode);
          CurrentNode = *HeadNode;
     }

     *TailNode = NULL;
}

/**
*@fn DeleteSELReclaimNode
*@brief This function is invoked to Delete the Node based on the Record ID
*@return Returns 0 on success
*@return Returns -1 on failure
*/
int DeleteSELReclaimNode(SELEventNode **HeadNode,SELEventNode **TailNode,INT16U RecordID,unsigned int *FileIndex,int BMCInst)
{
    struct SELEventNode *CurrentNode,*PrevNode = NULL;
    _FAR_  PEFRecordDetailsConfig_T*        pPEFRecordDetailsConfig;
     BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    CurrentNode = *HeadNode;

    pPEFRecordDetailsConfig  = &pBMCInfo->PEFRecordDetailsConfig;

    SEL_RECORD_ADDR(BMCInst,RecordID).RecAddr = NULL;
    SEL_RECORD_ADDR(BMCInst,RecordID).RecPos = 0;

    while( CurrentNode != NULL )
    {
          if(CurrentNode->SELRecord.EvtRecord.hdr.ID == RecordID)
          {
              if(CurrentNode == *HeadNode)
              {
                   if(NULL != CurrentNode->NextRec)
                   {
                       (CurrentNode->NextRec)->PrevRec = NULL;
                   }
                   *HeadNode = CurrentNode->NextRec;

                   LOCK_BMC_SHARED_MEM(BMCInst);
                   if(*HeadNode != NULL)
                   {
                       BMC_GET_SHARED_MEM(BMCInst)->m_FirstRecID = (*HeadNode)->SELRecord.EvtRecord.hdr.ID;

                       if(*HeadNode == *TailNode)
                       {
                           *TailNode = NULL;
                           BMC_GET_SHARED_MEM(BMCInst)->m_LastRecID = (*HeadNode)->SELRecord.EvtRecord.hdr.ID;;
                       }
                   }
                   else
                   {
                       BMC_GET_SHARED_MEM(BMCInst)->m_FirstRecID = 0;
                       BMC_GET_SHARED_MEM(BMCInst)->m_LastRecID = 0;

                       *TailNode = NULL;
                   }
                   UNLOCK_BMC_SHARED_MEM(BMCInst);

                  *FileIndex = CurrentNode->FileIndex;
                  if(pPEFRecordDetailsConfig->LastBMCProcessedEventID == RecordID)
                   { 
                      TDBG("deleting first node\n");
                      pPEFRecordDetailsConfig->LastBMCProcessedEventID=0xFFFF;
                      FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                      pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID),BMCInst);
                      TDBG("Setting Last Processed ID while deleting %d\n", pPEFRecordDetailsConfig->LastBMCProcessedEventID);
                   }

                   SEL_FREE(CurrentNode);

                   
              }
              else
              {
                   PrevNode->NextRec = CurrentNode->NextRec;
                   if(NULL != CurrentNode->NextRec)
                   {
                       (CurrentNode->NextRec)->PrevRec = PrevNode;
                   }
                   *FileIndex = CurrentNode->FileIndex;

                   if( CurrentNode ==*TailNode )
                   {
                      LOCK_BMC_SHARED_MEM(BMCInst);

                      if(PrevNode == *HeadNode)
                      {
                          BMC_GET_SHARED_MEM(BMCInst)->m_LastRecID = (*HeadNode)->SELRecord.EvtRecord.hdr.ID;;
                          *TailNode = NULL;
                      }
                      else
                      {
                           BMC_GET_SHARED_MEM(BMCInst)->m_LastRecID = PrevNode->SELRecord.EvtRecord.hdr.ID;
                           *TailNode = PrevNode;
                      }

                      UNLOCK_BMC_SHARED_MEM(BMCInst);
                   }
                   
                   if((pPEFRecordDetailsConfig->LastBMCProcessedEventID==RecordID) && (pPEFRecordDetailsConfig->LastBMCProcessedEventID!=0xFFFF))
                   { 
                      pPEFRecordDetailsConfig->LastBMCProcessedEventID=PrevNode->SELRecord.EvtRecord.hdr.ID;
                      FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                      pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID),BMCInst);
                      TDBG("Setting Last Processed ID while deleting %d\n", pPEFRecordDetailsConfig->LastBMCProcessedEventID);
                   }

                   SEL_FREE(CurrentNode);

                   
              }
              
              return 0;
          }
          else
          {
              PrevNode = CurrentNode;
              CurrentNode = CurrentNode->NextRec;
          }
    }
    return -1;
}

/**
*@fn AddSELRecordIDNode
*@brief This function is invoked to Add Record ID to the free
        pool of Record ID Linked List
*@return Returns 0 on success
*@return Returns -1 on failure
*/

int AddSELRecordIDNode(SELRecIDNode **HeadNode,SELRecIDNode **TailNode,INT16U RecordID,int BMCInst)
{
    struct  SELRecIDNode *CurrentNode = NULL;

    if(*HeadNode == NULL)
    {
        CurrentNode = SEL_MALLOC(SELRecIDNode);
        if(CurrentNode == NULL)
        {
            IPMI_ERROR("Cannot Allocate Memory for Record ID Node\n");
            return -1;
        }

        CurrentNode->RecordID = RecordID;
        CurrentNode->NextRecID = NULL;

        *HeadNode = CurrentNode;

        CurrentNode = NULL;

        return 0;
    }
    else
    {
      if(*TailNode == NULL)
      {
          CurrentNode = *HeadNode;
      }
       else
      {
          CurrentNode = *TailNode;
      }

      CurrentNode->NextRecID = SEL_MALLOC(SELRecIDNode);
      if(CurrentNode->NextRecID == NULL )
      {
          IPMI_ERROR("Cannot Allocate Memory for SEL Record ID Node\n");
          return -1;
      }

       (CurrentNode->NextRecID)->RecordID = RecordID;
       (CurrentNode->NextRecID)->NextRecID = NULL;

       *TailNode = CurrentNode->NextRecID;

       CurrentNode = NULL;

       return 0;
    }
}

/**
*@fn AddSELRecordIDNode
*@brief This function is invoked to Delete Record ID
        from free pool of Record ID Linked List
*@return Returns 0 on success
*@return Returns -1 on failure
*/
int DeleteSELRecordIDHeadNode(SELRecIDNode **HeadNode , SELRecIDNode **TailNode)
{
     struct SELRecIDNode *CurrentNode = NULL;

     CurrentNode = *HeadNode;
     if(CurrentNode != NULL)
     {
         *HeadNode = CurrentNode->NextRecID;
         if(*HeadNode == *TailNode)
         {
             *TailNode = NULL;
         }
         SEL_FREE(CurrentNode);
         return 0;
     }

     return -1;
}

/**
*@fn UpdateRecordIDList
*@brief This function is invoked to free pool
        list of Record ID
*@return Returns 0 on success
*@return Returns -1 on failure
*/
int UpdateRecordIDList(INT8U *RecordID,int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int RetVal,RecordCount = 0;
    INT16U MaxRecordCnt = ((pBMCInfo->IpmiConfig.SELAllocationSize *1024)/sizeof(SELRec_T));

    for(RecordCount = 1;RecordCount <= MaxRecordCnt ;RecordCount++) 
    {
       if(RecordID[RecordCount] == FALSE)
      {    
            SEL_RECORD_ADDR(BMCInst,RecordCount).RecAddr=NULL;
            SEL_RECORD_ADDR(BMCInst,RecordCount).RecPos=0;
            RetVal = AddSELRecordIDNode(&(SEL_RECLAIM_RECORD_HEAD_NODE(BMCInst)),&(SEL_RECLAIM_RECORD_TAIL_NODE(BMCInst))
                                                              ,RecordCount,BMCInst);
            if(RetVal == -1)
            {
                IPMI_ERROR("Cannot Add Record ID to Node\n");
                return -1;
            }
      }
    }

    return 0;
}

/**
*@fn GetNextReclaimRecordID
*@brief This function is invoked to Get 
        Valid Record ID from free pool
*@return Returns 0 on success
*@return Returns -1 on failure
*/
INT16U GetNextReclaimRecordID(int BMCInst)
{
    BMCInfo_t *pBMCInfo =&g_BMCInfo[BMCInst];
    int CircularSEL=0;
    INT16U RecordID = 0;

    if(g_corefeatures.circular_sel == ENABLED)
    {
        CircularSEL = pBMCInfo->AMIConfig.CircularSEL;
    }

    if(SEL_RECLAIM_RECORD_HEAD_NODE(BMCInst) != NULL)
    {
        RecordID = SEL_RECLAIM_RECORD_HEAD_NODE(BMCInst)->RecordID;

        DeleteSELRecordIDHeadNode(&SEL_RECLAIM_RECORD_HEAD_NODE(BMCInst),&SEL_RECLAIM_RECORD_TAIL_NODE(BMCInst));

        return RecordID;
    }
    else if((CircularSEL)  && (pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords >= pBMCInfo->SELConfig.MaxSELRecord ))
    {
         RecordID = BMC_GET_SHARED_MEM(BMCInst)->m_FirstRecID;

         return RecordID;
    }

    return 0;
}

/**
*@fn FlushAddReclaimSEL
*@brief This function is invoked to Flush 
        Valid Record to File based on the
        Last File Index
*@return Returns 0 on success
*@return Returns -1 on failure
*/
int FlushAddReclaimSEL(SELRec_T *SELRecord,int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    int nRet=0;
    unsigned int FileSize = 0;
    INT16U Length,MaxRecords = 0;
    char TempFileName[MAXFILESIZE],OrigFileName[MAXFILESIZE]={0};
    char DirName[MAXFILESIZE]={0};
    SELRec_T FileSELRecord[MAX_SEL_RECORD];

    if((pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords >= pBMCInfo->SELConfig.MaxSELRecord) &&
       (pBMCInfo->SELConfig.MaxSELRecord != 0))
    {
        nRet = ReFlushDeleteReclaimSEL(SELRecord->EvtRecord.hdr.ID, BMCInst);
        if(nRet < 0)
        {
            IPMI_ERROR("Cannot Delete the First Node in SEL List\n");
            return -1;
        }
    }

      SEL_RECLAIM_DIR(BMCInst,(char *)DirName);

      sprintf(OrigFileName,"%s/%.10u.dat",DirName,pBMCInfo->SELReclaimRepos.pSELReclaimInfo->LastFileIndex);

      FileSize = GetReclaimFileSize(OrigFileName);
      if(FileSize != 0)
      {
          MaxRecords = FileSize/sizeof(SELRec_T);
          if(MaxRecords == MAX_SEL_RECORD)
          {              
              pBMCInfo->SELReclaimRepos.pSELReclaimInfo->LastFileIndex++;
              sprintf(OrigFileName,"%s/%.10u.dat",DirName,pBMCInfo->SELReclaimRepos.pSELReclaimInfo->LastFileIndex);
              FileSize = 0;
              MaxRecords = 0;
          }
      }
     
     nRet = AddSELRecalimNode(&(SEL_RECLAIM_HEAD_NODE(BMCInst)),&(SEL_RECLAIM_TAIL_NODE(BMCInst)),SELRecord
                                                  ,pBMCInfo->SELReclaimRepos.pSELReclaimInfo->LastFileIndex,BMCInst);
     if(nRet < 0)
     {
         IPMI_ERROR("Cannot Add the Requested Record in SEL List\n");
         return -1;
     }

      if(MaxRecords == (MAX_SEL_RECORD-1))
      {
            Length = ReadWriteNVR(OrigFileName, (INT8U *)&FileSELRecord[0], 0,FileSize , READ_NVR);
            if(Length < FileSize)
            {
                IPMI_ERROR("Minimal Reading of SEL Record has taken place\n");
                return -1;
            }

           memcpy(&FileSELRecord[MAX_SEL_RECORD-1],SELRecord,sizeof(SELRec_T));
           sprintf(TempFileName,"%s/%.10u_t.dat",DirName,pBMCInfo->SELReclaimRepos.pSELReclaimInfo->LastFileIndex);

            Length = ReadWriteNVR(TempFileName,(INT8U *)&FileSELRecord[0],0,FileSize+sizeof(SELRec_T),WRITE_NVR);
            if(Length < FileSize+sizeof(SELRec_T))
            {
                IPMI_ERROR("Minimal Writing of SEL Record has taken place\n");
                return -1;
            }

           unlink(OrigFileName);
           rename(TempFileName,OrigFileName);

      }
      else
      {
            Length = ReadWriteNVR(OrigFileName,(INT8U *)SELRecord,FileSize,sizeof(SELRec_T),WRITE_NVR);
            if(Length < sizeof(SELRec_T))
            {
                IPMI_ERROR("Minimal Writing of SEL Record has taken place\n");
                return -1;
            }
      }

      return 0;
}

/**
*@fn GetReclaimFileSize
*@brief This function is invoked to Get 
        Size of File
*@return Returns File Size on success
*@return Returns 0 on failure
**/
INT32U GetReclaimFileSize(char *FileName)
{
    struct stat buf;

    if((stat(FileName,&buf) !=0 ) && (errno == ENOENT))
    {
        //No File available
        return 0;
    }

    //Return the exact number of bytes to calculate number of SEL records
    return buf.st_size;
}

/**
*@fn FilterSELReclaimFiles
*@brief This function is passed as 
        an argument to scandir function
        to remove the unwanted files from
        the list
*@return Returns File Size on success
*@return Returns 0 on failure
**/
int FilterSELReclaimFiles(const struct dirent *entry)
{
    if((strcmp(entry->d_name,".") == 0) || (strcmp(entry->d_name,"..") == 0))
    {
      //Got the file named in the comparison
      return 0;
    }
    return -1;
}

/**
*@fn CheckPreviousReclaimFile
*@brief This function is invoked to
           merge the contents of SEL
           Files if the size doesn't exceeds
           more than MAX_SEL_RECORD
*@return Returns 0 on success
*@return Returns -1 on failure
**/
int CheckPreviousReclaimFile(INT32U FileIndex,INT32U FileSize,SELRec_T *pSELReclaimRec,INT8U *SELCount,char *PrevFileName ,int BMCInst)
{
    struct SELEventNode *FirstNode,*LastNode = NULL;
    char DirName[MAXFILESIZE] = {0};
    SELRec_T PrevSELRecord[MAX_SEL_RECORD];
    INT8U RecCount,PrevRecCount = 0;
    INT32U FileCount,PrevFileSize = 0;

    FirstNode = SEL_RECORD_ADDR(BMCInst,BMC_GET_SHARED_MEM(BMCInst)->m_FirstRecID).RecAddr;
    LastNode = SEL_RECORD_ADDR(BMCInst,BMC_GET_SHARED_MEM(BMCInst)->m_LastRecID).RecAddr;

    /*If first node is empty then there is no file available at this time
      If last node is empty, then there is no SEL initialization happened yet
      If File index is equal to  first node index then there is no previous file
      If File index is greater than last node index then out of boundary exception*/
    if(FirstNode == NULL)
    {
        IPMI_ERROR("First node is empty.\n");
        return -1;
    }
    else if(FileIndex == FirstNode->FileIndex)
    {
        TDBG("File Index points to first file in the node\n");
        return -1;
    }
    else if((LastNode == NULL) || (FileIndex > LastNode->FileIndex))
    {
        IPMI_ERROR("Last Node is empty as the SEL Initialization isn't happened yet or Linked List Corrupted\n");
        return -1;
    }

    SEL_RECLAIM_DIR(BMCInst,DirName);

    for(FileCount=FileIndex;FileCount>FirstNode->FileIndex;FileCount--)
    {
        sprintf(PrevFileName,"%s/%.10u.dat",DirName,FileCount-1);

        PrevFileSize = GetReclaimFileSize(PrevFileName);
        if(PrevFileSize == 0)
        {
            continue;
        }
        else if(((FileSize+PrevFileSize)/sizeof(SELRec_T)) <= MAX_SEL_RECORD)
        {
            memset(&PrevSELRecord[0],0,sizeof(PrevSELRecord));

            ReadWriteNVR(PrevFileName,(INT8U *)&PrevSELRecord[0] ,0, PrevFileSize, READ_NVR);

            PrevRecCount = PrevFileSize/sizeof(SELRec_T);

            memcpy(&PrevSELRecord[PrevRecCount],&pSELReclaimRec[0],FileSize);

            //Changing the File Index to make the entry point to exact file location
            for(RecCount = 0;RecCount<PrevRecCount;RecCount++)
            {
                if(SEL_RECORD_ADDR(BMCInst,PrevSELRecord[RecCount].EvtRecord.hdr.ID).RecAddr != NULL)
                {
                    SEL_RECORD_ADDR(BMCInst,PrevSELRecord[RecCount].EvtRecord.hdr.ID).RecAddr->FileIndex = FileIndex;
                }
            }

            memset(pSELReclaimRec,0,sizeof(SELRec_T) * MAX_SEL_RECORD);

            memcpy(&pSELReclaimRec[0],&PrevSELRecord[0],FileSize+PrevFileSize);

            *SELCount += PrevRecCount;

            return 0;
        }
        else
        {
            return -1;
        }
    }
    return -1;
}

/**
*@fn ReFlushDeleteReclaimSEL
*@brief This function is invoked to Delete
        requested Record from File and
        Linked List
*@return Returns 0 on success
*@return Returns -1 on failure
**/
int ReFlushDeleteReclaimSEL(INT16U RecordID,int BMCInst)
{
      int nRet = 0;
      INT8U SELCount,NewSELCount = 0;
      INT32U FileSize,FileIndex = 0;
      char DirName[MAXFILESIZE]= {0};
      char RepFileName[MAXFILESIZE] = {0},PrevFileName[MAXFILESIZE] = {0},FileName[MAXFILESIZE]={0};
      SELRec_T DelSELRecord[MAX_SEL_RECORD],NewSELRecord[MAX_SEL_RECORD];

      memset(&DelSELRecord[0],0,sizeof(DelSELRecord));
      memset(&NewSELRecord[0],0,sizeof(NewSELRecord));

      nRet = DeleteSELReclaimNode(&(SEL_RECLAIM_HEAD_NODE(BMCInst)), &(SEL_RECLAIM_TAIL_NODE(BMCInst)),RecordID, &FileIndex,BMCInst);
       if(nRet != 0)
       {
           IPMI_ERROR("Cannot Delete the Requested Record from SEL List\n");
           return -1;
       }

       SEL_RECLAIM_DIR(BMCInst, DirName);

       sprintf(FileName,"%s/%.10u.dat",DirName,FileIndex);
       sprintf(RepFileName,"%s/%.10u_t.dat",DirName,FileIndex);

       FileSize = GetReclaimFileSize(FileName);
       if(FileSize > 0)
       {
           ReadWriteNVR(FileName, (INT8U * )&DelSELRecord[0], 0, FileSize, READ_NVR);

           for(SELCount = 0;SELCount<(FileSize/sizeof(SELRec_T));SELCount++)
           {
              if((DelSELRecord[SELCount].Valid == VALID_RECORD) && ( DelSELRecord[SELCount].EvtRecord.hdr.ID != RecordID))
              {
                 memcpy(&NewSELRecord[NewSELCount],&DelSELRecord[SELCount],sizeof(SELRec_T));
                 NewSELCount++;
              }
           }
       

          if(NewSELCount > 0)
          {
              nRet = CheckPreviousReclaimFile(FileIndex,NewSELCount*sizeof(SELRec_T),&NewSELRecord[0],&NewSELCount,&PrevFileName[0],BMCInst);

              ReadWriteNVR(RepFileName, (INT8U *)&NewSELRecord[0],0, (NewSELCount*sizeof(SELRec_T)), WRITE_NVR);

              unlink(FileName);

              if(0 == nRet) // This condition is required to remove the previous file from the File System
              {
                  unlink(PrevFileName);
              }

              rename(RepFileName,FileName);
          }
          else
          {
              unlink(FileName);
          }
       }
       
  return 0;
}

/**
*@fn CopySELReclaimEntries
*@brief This function is invoked to Copy
  SEL Entries based on the record count
*@return Returns 0 on success
*@return Returns -1 on failure
**/
int CopySELReclaimEntries(SELEventNode **HeadNode,INT8U *RecordData,INT32U NumEntriesRetrd,INT32U NumOfEntries,int BMCInst)
{
    INT32U EntriesRetrieved,NumEntries;
    struct SELEventNode *CurrentNode = NULL;

    for(CurrentNode = *HeadNode,EntriesRetrieved = NumEntries = 0; CurrentNode != NULL; CurrentNode = CurrentNode->NextRec)
    {
        if((EntriesRetrieved >= NumEntriesRetrd) && (NumEntries < NumOfEntries))
        {
            memcpy(RecordData+(NumEntries*sizeof(SELRec_T)),&CurrentNode->SELRecord,sizeof(SELRec_T));
            NumEntries++;
        }
        else if(NumEntries >= NumOfEntries)
        {
            /* Break For loop */
            CurrentNode = NULL;
            EntriesRetrieved = 0;
            break;
        }

        EntriesRetrieved++;
    }

    if(NumEntries == 0)
    {
        return -1;
    }

    return 0;
}

/**
*@fn SaveSELReclaimInfo
*@brief This function is invoked to read
   SEL Informations from SELReclaimInfo.ini
*@return Returns 0 on success
*@return Returns -1 on failure
**/
int LoadSELReclaimInfo(SELReclaimInfo_T *SELInfo,int BMCInst)
{
    INI_HANDLE ini;
    char FileName[MAXFILESIZE]={0};

    SEL_RECLAIM_INFO_INI(BMCInst, FileName);

    ini=IniLoadFile(FileName);
    if(ini == NULL)
    {
        return -1;
    }

    strcpy((char*)SELInfo->Signature, IniGetStr(ini, SEL_RECLAIM_SECTION_INFO, SEL_SIGNATURE, "AMI"));
    SELInfo->NumRecords = IniGetUInt(ini, SEL_RECLAIM_SECTION_INFO, SEL_NUMRECORDS, 0);
    SELInfo->AddTimeStamp = IniGetUInt(ini, SEL_RECLAIM_SECTION_INFO, SEL_ADDTIMESTAMP, 0);
    SELInfo->EraseTimeStamp = IniGetUInt(ini, SEL_RECLAIM_SECTION_INFO, SEL_ERASETIMESTAMP, 0);
    SELInfo->LastFileIndex = IniGetUInt(ini, SEL_RECLAIM_SECTION_INFO,SEL_LASTFILEINDEX, 0);
    SELInfo->MaxRecPos = IniGetUInt(ini, SEL_RECLAIM_SECTION_INFO,SEL_MAXRECPOS, 0);

    IniCloseFile(ini);

    return 0;
}

/**
*@fn LoadSELReclaimInfo
*@brief This function is invoked to Flush
   SEL Informations in SELReclaimInfo.ini
*@return Returns 0 on success
*@return Returns -1 on failure
**/
int SaveSELReclaimInfo(SELReclaimInfo_T *SELInfo,int BMCInst)
{
    INI_HANDLE ini;
    char FileName[MAXFILESIZE]={0};

    SEL_RECLAIM_INFO_INI(BMCInst, FileName);

    ini=IniLoadFile(FileName);
    if(ini == NULL)
    {
        return -1;
    }

    if(strlen((char *)SELInfo->Signature) == 0)
    {
        memcpy(&SELInfo->Signature[0],"AMI",SIGN_SIZE);
    }

    IniSetStr(ini, SEL_RECLAIM_SECTION_INFO,SEL_SIGNATURE, (char *)&SELInfo->Signature[0]);
    IniSetUInt(ini, SEL_RECLAIM_SECTION_INFO,SEL_NUMRECORDS, SELInfo->NumRecords);
    IniSetUInt(ini, SEL_RECLAIM_SECTION_INFO, SEL_ADDTIMESTAMP, SELInfo->AddTimeStamp);
    IniSetUInt(ini, SEL_RECLAIM_SECTION_INFO, SEL_ERASETIMESTAMP, SELInfo->EraseTimeStamp);
    IniSetUInt(ini,SEL_RECLAIM_SECTION_INFO, SEL_LASTFILEINDEX, SELInfo->LastFileIndex);
    IniSetUInt(ini, SEL_RECLAIM_SECTION_INFO, SEL_MAXRECPOS, SELInfo->MaxRecPos);

    IniSaveFile(ini,FileName);
    IniCloseFile(ini);

    return 0;
}




