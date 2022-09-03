/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends .           **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 *****************************************************************
 *
 * Sel.c
 * Sel Command Handler
 *
 * Author: Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0

#include <dlfcn.h>
#include "Types.h"
#include "Message.h"
#include "MsgHndlr.h"
#include "SharedMem.h"
#include "PMConfig.h"
#include "Debug.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "IPMI_SEL.h"
#include "SELRecord.h"
#include "NVRAccess.h"
#include "SEL.h"
#include "PEF.h"
#include "Util.h"
#include "IPMI_KCS.h"
#include "IPMI_BT.h"
#include "IPMIConf.h"
#include "PDKAccess.h"
#include "PDKDefs.h"
#include "IPMI_IPM.h"
#include "IPMDevice.h"
#include "IPMI_Storage.h"
#include "PDKCmdsAccess.h"
#include "IPMI_Sensor.h"
#include "featuredef.h"
#include <dirent.h>
#include "video_misc.h"
#include "SSIAccess.h"
#include "aes.h"
#include "safesystem.h"
#include "cmdselect.h"
#include "IPMI_AMIExtsel.h"

/* Reserved bit macro definitions */
#define RESERVED_BITS_PARTIALADDSELENTRY                0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)

#if SEL_DEVICE == 1


/*** Local Definitions ***/
#define SEL_RECORD_SIZE             16
#define CLR_SEL_PASSWORD_STR_LEN    3
#define CLEAR_SEL_GET_STATUS        0x00
#define CLEAR_SEL_ACTION            0xaa

#define SEL_ERASE_IN_PROGRESS       0x00
#define SEL_ERASE_COMPLETED         0x01
#define SEL_ALLOC_UNIT_SIZE         0x10
#define SEL_MAX_RECORD_SIZE         0x10
#define INVALID_RECORD_ID           0x00

#define OVERFLOW_FLAG               0x80
#define DELETE_SEL_SUPPORT          0x08
#define PARTIAL_ADD_SEL_SUPPORT     0x04
#define RESERVE_SEL_SUPPORT         0x02
#define GET_SEL_ALLOC_SUPPORT       0x01
#define NO_SUPPORT                  0x00


#define STATUS_DELETE_SEL           0xA5

#define SEL_ALMOST_FULL_PERCENTAGE  75
#define SEL_PARTIAL_ADD_REQUEST_MISC_BYTES		6
#define SEL_PARTIAL_ADD_CMD_MAX_LEN				22
#define SEL_INITIALIZED 0x01
#define SEL_UNINITIALIZED 0x00

#if SET_SEL_TIME != UNIMPLEMENTED || SET_SEL_TIME_UTC_OFFSET != UNIMPLEMENTED

#define CMD_LEN              128
#define LINK_CMD             "ln -sf"
#define ZONEINFO_DIR         "/usr/share/zoneinfo/Etc"
#define LOCALTIME            "/conf/localtime"
#define UTC_MAX_RANGE        720
#define UTC_MIN_RANGE        -720

#endif // SET_SEL_TIME_UTC_OFFSET

#define PRE_CLK_SET     0x00
#define POST_CLK_SET    0x80

//Capture BSOD
#define CRASH_SCREEN_DIRECTORY "/var/bsod"
#define CRASH_SCREEN_FILE "/var/bsod/crashscreen.cap"
#define CRASH_SCREEN_FILE_JPEG "/var/bsod/crashscreen.jpeg"
#define VIDEO_LIB "/usr/local/lib/libvideo.so"
#define CAP_CRASHSCREEN_FUNC "Capture_CrashScreen"

#define BSOD_CAPTURE 0x2
//For informing Continuous recording about BSOD Occurrence
#define BSOD_SIGNAL 0x3

#define CMD_COMM_PIPE   "/var/cmdpipe"
/*** Prototype Declaration ***/
static _FAR_ SELRec_T*    GetNextSELEntry (_FAR_ SELRec_T* rec, int BMCInst);
static _FAR_ SELRec_T* GetSELRec(INT16U RecID,int BMCInst);
static _FAR_ void FindRecOrder(int BMCInst);
static INT8U SELTimeClockSync(INT8U Action, int BMCInst);


/*** Global variables ***/

/*** Module variables ***/
_FAR_ SELRepository_T* _FAR_    m_sel;
//Added for Partial Adding
//SELEventRecord_T  SelPartialAddRecord;
#define SENSOR_TYPE_EVT_LOGGING                 0x10

/* Clear and Full Event Log Message */
static INT8U m_SELEventMsg [] = {

        0x00, 0x00,					//Record ID	
        0x02,						//Record Type is System
        0x00, 0x00, 0x00, 0x00,		//TimeStamp
        0x20, 0x00,					//Generator ID is BMC
        IPMI_EVM_REVISION,
        SENSOR_TYPE_EVT_LOGGING,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF
};

/* System Event Message */
static INT8U m_SysEventMsg [] = {

        0x00, 0x00,                 //Record ID
        0x02,                       //Record Type is System
        0x00, 0x00, 0x00, 0x00,     //TimeStamp
        0x20, 0x00,                 //Generator ID is BMC
        IPMI_EVM_REVISION,
        SENSOR_TYPE_SYSTEM_EVENT,
        0x6F,
        0xFF,
        0xFF,
        0xFF,
        0xFF
};

/*---------------------------------------
 * LogClearSELEvent
 *---------------------------------------*/
void 
LogClearSELEvent (int BMCInst)
{
    AddSELRes_T     AddSelRes; 	
    SensorInfo_T*   pSensorInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    pSensorInfo = (SensorInfo_T*)GetSensorInfoFromSensorType(SENSOR_TYPE_EVT_LOGGING, BMCInst);
        
    if (pSensorInfo != NULL)
    {
        // make sure the assertion event log is enabled
        if ((pSensorInfo->AssertionEventEnablesByte1 & (1 << EVENT_LOG_AREA_RESET)))
        {
            pBMCInfo->SELConfig.SELEventMsg [11] = pSensorInfo->SensorNumber;
            pBMCInfo->SELConfig.SELEventMsg [12] = 0x6F;	// Sensor Specific Event Type;
            pBMCInfo->SELConfig.SELEventMsg [13] = EVENT_LOG_AREA_RESET;
            pBMCInfo->SELConfig.SELEventMsg [15] = pBMCInfo->SELConfig.SELEventMsg [14] = 0xFF;

            /* Add clear SEL Event */
            AddSELEntry (&pBMCInfo->SELConfig.SELEventMsg[0], sizeof (pBMCInfo->SELConfig.SELEventMsg), (INT8U*)&AddSelRes,BMCInst);
        }
    }
}

/*---------------------------------------
 * LogSELFullEvent
 *---------------------------------------*/
void 
LogSELFullEvent (int BMCInst)
{
    AddSELRes_T     AddSelRes; 	
    SensorInfo_T*   pSensorInfo;
//    INT8U *curchannel;

    pSensorInfo = (SensorInfo_T*)GetSensorInfoFromSensorType(SENSOR_TYPE_EVT_LOGGING, BMCInst);
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    if (pSensorInfo != NULL)
    {
        // make sure the assertion event log is enabled
        if ((pSensorInfo->AssertionEventEnablesByte1 & (1 << EVENT_SEL_IS_FULL)))
        {
            pBMCInfo->SELConfig.SELEventMsg [11] = pSensorInfo->SensorNumber;
            pBMCInfo->SELConfig.SELEventMsg [12] = 0x6F;	// Sensor Specific Event Type;
            pBMCInfo->SELConfig.SELEventMsg [13] = EVENT_SEL_IS_FULL;
            pBMCInfo->SELConfig.SELEventMsg [15] = pBMCInfo->SELConfig.SELEventMsg [14] = 0xFF;

  //          OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
            /* Add SEL Full Event */
            LockedAddSELEntry (&pBMCInfo->SELConfig.SELEventMsg[0], sizeof (pBMCInfo->SELConfig.SELEventMsg), (INT8U*)&AddSelRes,
                             FALSE,POST_SEL_AND_PEF,BMCInst);	
        }
    }
}

/*---------------------------------------
 * PostSELToPEF 
 *---------------------------------------*/
int
PostSELToPEF (SELEventRecord_T *pSELRec, int BMCInst)
{
    MsgPkt_T            MsgToPEF;

    /* PEF Action related Event messages are discarded */
    if((pSELRec->SensorType == SENSOR_TYPE_SYSTEM_EVENT) &&
        (pSELRec->EvtData1 == PEF_ACTION_SEN_SPECIFIC_OFFSET) && 
        ((pSELRec->EvtDirType & 0x7F) == SENSOR_SPECIFIC_READ_TYPE))
    {
        return 0;
    }

    MsgToPEF.Param = PARAM_SENSOR_EVT_MSG;
    MsgToPEF.Size  = sizeof (SELEventRecord_T);
    _fmemcpy ((_FAR_ INT8U*)MsgToPEF.Data, (_FAR_ INT8U*)pSELRec,
              sizeof(SELEventRecord_T));
    IPMI_DBG_PRINT ("Posting Message to PEF\n");
    PostMsgNonBlock (&MsgToPEF, PEF_TASK_Q,BMCInst);

    return 0;
}


/*----------------------------------------------------------------- 
 * @fn CheckLastSELRecordID 
 * 
 * @brief This function is called after PEF Postpone timer expires.  
 *  When LastSELRecordID and LastSWProcessedEventID are mismatch,  
 *  BMC will automatically perform PEF against any existing,  
 *  unprocessed events in SEL. Please refer IPMI spec 17.4.1 for details.
 *  
 * @param BMCInst 
 *-----------------------------------------------------------------*/ 
void 
CheckLastSELRecordID (int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_ PEFRecordDetailsConfig_T*  pPEFRecordDetailsConfig; 
    INT16U        i; 
    _FAR_ SELRepository_T* _FAR_    m_sel = NULL;
    INT32U MaxAllowRec = 0;
    int Recpos,BMCRecpos=0,SWRecpos=0;
    struct SELEventNode *LastProcessedNode = NULL;

    if(g_corefeatures.del_sel_reclaim_support != ENABLED)
    {
        m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
        m_sel->SELRecord = (SELRec_T *)GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024) + sizeof(SELRepository_T), BMCInst);
            pBMCInfo->SELConfig.LastEvtTS = (m_sel->AddTimeStamp > m_sel->EraseTimeStamp) ?  m_sel->AddTimeStamp : m_sel->EraseTimeStamp;
    }
    else
    {
    	pBMCInfo->SELConfig.LastEvtTS = (pBMCInfo->SELReclaimRepos.pSELReclaimInfo->AddTimeStamp > pBMCInfo->SELReclaimRepos.pSELReclaimInfo->EraseTimeStamp) ?
                 pBMCInfo->SELReclaimRepos.pSELReclaimInfo->AddTimeStamp : pBMCInfo->SELReclaimRepos.pSELReclaimInfo->EraseTimeStamp;
    }
    
    pPEFRecordDetailsConfig = &pBMCInfo->PEFRecordDetailsConfig;

    /* Check if there any records in SEL */
    if (0xFFFF != pPEFRecordDetailsConfig->LastSELRecordID) 
    {
        INT16U  LastProcessedID = 0;
        if(g_corefeatures.del_sel_reclaim_support != ENABLED)
        {
            if((pPEFRecordDetailsConfig->LastSELRecordID > pPEFRecordDetailsConfig->LastBMCProcessedEventID)
                && (0 != pPEFRecordDetailsConfig->LastBMCProcessedEventID)
                && (pPEFRecordDetailsConfig->LastSELRecordID > pPEFRecordDetailsConfig->LastSWProcessedEventID))
            {
                /* Process all events that were not either processed by BMC or SW */
                LastProcessedID = (pPEFRecordDetailsConfig->LastBMCProcessedEventID > pPEFRecordDetailsConfig->LastSWProcessedEventID) ? 
                            pPEFRecordDetailsConfig->LastBMCProcessedEventID : pPEFRecordDetailsConfig->LastSWProcessedEventID;
            }
            else if (0xFFFF == pPEFRecordDetailsConfig->LastBMCProcessedEventID)
            {
                if (0xFFFF == pPEFRecordDetailsConfig->LastSWProcessedEventID)
                {
                    /* Neither BMC nor SW processed any event in the SEL; So process all now */
                    LastProcessedID = 0xFFFF;
                }
                else
                {
                    /* If BMC didn't process any event, but SW did, then process the remaining events */
                    LastProcessedID = pPEFRecordDetailsConfig->LastSWProcessedEventID;
                }
            }
            else if (0xFFFF == pPEFRecordDetailsConfig->LastSWProcessedEventID)
            {
                /* If SW didn't process any event, but BMC did, then process the remaining events */
                LastProcessedID = pPEFRecordDetailsConfig->LastBMCProcessedEventID;
            }

            /* If there any event to be processed, notify PEF */
            if (LastProcessedID)
            {
                _FAR_  SELRec_T*    pSelRec;

                if (0xFFFF == LastProcessedID)
                    LastProcessedID = 1;
                else
                    LastProcessedID++;

                for (i = LastProcessedID; i <= (pPEFRecordDetailsConfig->LastSELRecordID); i++)
                {
                    pSelRec = GetSELRec(i,BMCInst);
                    if (pSelRec != 0)
                    {
                        PostSELToPEF (&(pSelRec->EvtRecord), BMCInst);
                    }
                }
            }
           }
           else
           {
                if(pPEFRecordDetailsConfig->LastBMCProcessedEventID==0)
                 {
                    return ;
                 }

               MaxAllowRec = ((pBMCInfo->IpmiConfig.SELAllocationSize) * 1024)/sizeof(SELRec_T);

              /* Checking the Record id for processing */
               if(((pPEFRecordDetailsConfig->LastSELRecordID != 0xFFFF) && (pPEFRecordDetailsConfig->LastSELRecordID > MaxAllowRec))||
                ((pPEFRecordDetailsConfig->LastBMCProcessedEventID != 0xFFFF) && (pPEFRecordDetailsConfig->LastBMCProcessedEventID > MaxAllowRec))
                 ||((pPEFRecordDetailsConfig->LastSWProcessedEventID != 0xFFFF) && (pPEFRecordDetailsConfig->LastSWProcessedEventID > MaxAllowRec)))
               {
                    pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID         = 0xffff;
                    pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID  = 0xffff;
                    pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID = 0xffff;

                    /* Flush the values to NVRAM */
                    FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
                        pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                    FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID,
                        pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                    FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                        pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                    return;
               }

                if(SEL_RECORD_ADDR(BMCInst,pPEFRecordDetailsConfig->LastSELRecordID).RecAddr != NULL)
                {
                    Recpos = SEL_RECORD_ADDR(BMCInst,pPEFRecordDetailsConfig->LastSELRecordID).RecPos;
                }
                else
                {
                    pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID         = 0xffff;
                    pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID  = 0xffff;
                    pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID = 0xffff;

                    /* Flush the values to NVRAM */
                    FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
                        pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                    FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID,
                        pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                    FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                        pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                    return;
                }
                if((pPEFRecordDetailsConfig->LastBMCProcessedEventID)!=0xFFFF)
                {
                   if(SEL_RECORD_ADDR(BMCInst, pPEFRecordDetailsConfig->LastBMCProcessedEventID).RecAddr!= NULL)
                   {
                       BMCRecpos=SEL_RECORD_ADDR(BMCInst, pPEFRecordDetailsConfig->LastBMCProcessedEventID).RecPos;
                   }
                   else
                   {
                        pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID         = 0xffff;
                        pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID  = 0xffff;
                        pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID = 0xffff;

                        /* Flush the values to NVRAM */
                        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
                            pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID,
                            pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                            pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                        return;
                   }
                }
                if((pPEFRecordDetailsConfig->LastSWProcessedEventID)!=0xFFFF)
                {
                     if(SEL_RECORD_ADDR(BMCInst, pPEFRecordDetailsConfig->LastSWProcessedEventID).RecAddr != NULL)
                     {
                       SWRecpos=SEL_RECORD_ADDR(BMCInst, pPEFRecordDetailsConfig->LastSWProcessedEventID).RecPos;
                     }
                     else
                     {
                        pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID         = 0xffff;
                        pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID  = 0xffff;
                        pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID = 0xffff;

                        /* Flush the values to NVRAM */
                        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
                            pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID,
                            pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                            pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                        return;
                     }
                }
                
                if (0xFFFF == pPEFRecordDetailsConfig->LastBMCProcessedEventID)
                {
                    if (0xFFFF == pPEFRecordDetailsConfig->LastSWProcessedEventID)
                    {
                        /* Neither BMC nor SW processed any event in the SEL; So process all now */
                        if(SEL_RECLAIM_HEAD_NODE(BMCInst) != NULL)
                        {
                            LastProcessedNode = SEL_RECLAIM_HEAD_NODE(BMCInst);
                        }
                        else
                        {
                            pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID         = 0xffff;
                            pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID  = 0xffff;
                            pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID = 0xffff;

                            /* Flush the values to NVRAM */
                            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                            return;
                        }
                    }
                    else
                    {
                        /* If BMC didn't process any event, but SW did, then process the remaining events */
                        if(SEL_RECORD_ADDR(BMCInst,pPEFRecordDetailsConfig->LastSWProcessedEventID).RecAddr != NULL)
                        {
                            LastProcessedNode = SEL_RECORD_ADDR(BMCInst,pPEFRecordDetailsConfig->LastSWProcessedEventID).RecAddr->NextRec;
                        }
                        else
                        {
                            pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID         = 0xffff;
                            pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID  = 0xffff;
                            pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID = 0xffff;

                            /* Flush the values to NVRAM */
                            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                            return;
                        }
                    }
                }
                else if (0xFFFF == pPEFRecordDetailsConfig->LastSWProcessedEventID)
                {
                    /* If SW didn't process any event, but BMC did, then process the remaining events */
                        if(SEL_RECORD_ADDR(BMCInst,pPEFRecordDetailsConfig->LastBMCProcessedEventID).RecAddr != NULL)
                        {
                            LastProcessedNode = SEL_RECORD_ADDR(BMCInst,pPEFRecordDetailsConfig->LastBMCProcessedEventID).RecAddr->NextRec;
                        }
                        else
                        {
                            pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID         = 0xffff;
                            pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID  = 0xffff;
                            pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID = 0xffff;

                            /* Flush the values to NVRAM */
                            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                            return;
                        }

                }
                else if(( Recpos > BMCRecpos )&& (0 != pPEFRecordDetailsConfig->LastBMCProcessedEventID)&& (Recpos > SWRecpos))
                {
                    /* Process all events that were not either processed by BMC or SW */
                    LastProcessedID= (BMCRecpos > SWRecpos) ? pPEFRecordDetailsConfig->LastBMCProcessedEventID : pPEFRecordDetailsConfig->LastSWProcessedEventID;
                    if(SEL_RECORD_ADDR(BMCInst,LastProcessedID).RecAddr != NULL)
                    {
                        LastProcessedNode = SEL_RECORD_ADDR(BMCInst,LastProcessedID).RecAddr->NextRec;
                    }
                    else
                    {
                        pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID         = 0xffff;
                        pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID  = 0xffff;
                        pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID = 0xffff;

                        /* Flush the values to NVRAM */
                        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
                            pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID,
                            pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                            pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
                        return;
                    }
                }

                IPMI_DBG_PRINT_2("in Init SEL %d %d\n",LastProcessedID,pPEFRecordDetailsConfig->LastSELRecordID);

                /* If there any event to be processed, notify PEF */
                _FAR_  SELRec_T*    pSelRec;
                while (LastProcessedNode)
                {
                    pSelRec = &(LastProcessedNode->SELRecord);
                    if (pSelRec != 0)
                    {
                        PostSELToPEF (&(pSelRec->EvtRecord), BMCInst);
                    }
                   LastProcessedNode=LastProcessedNode->NextRec;
                }
        }
    }
}

/*---------------------------------------
 * InitSEL
 *---------------------------------------*/
int
InitSEL (int BMCInst)
{
    _FAR_ SELRec_T*     pSELRecord;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_ SELRepository_T* _FAR_    m_sel = NULL;
    int ReclaimSELSpace = 0;

    if(g_corefeatures.del_sel_reclaim_support == ENABLED)
    {
         ReclaimSELSpace = ENABLED;
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    memcpy(&pBMCInfo->SELConfig.SELEventMsg[0],m_SELEventMsg, sizeof(pBMCInfo->SELConfig.SELEventMsg));

    if(!ReclaimSELSpace)
    {
        m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
        m_sel->SELRecord = (SELRec_T *)GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024) + sizeof(SELRepository_T), BMCInst);

        pBMCInfo->SELConfig.LastEvtTS = (m_sel->AddTimeStamp > m_sel->EraseTimeStamp) ?
                                                                  m_sel->AddTimeStamp : m_sel->EraseTimeStamp;
        pBMCInfo->SELConfig.MaxSELRecord = ((pBMCInfo->IpmiConfig.SELAllocationSize *1024) - sizeof(SELRepository_T)) / sizeof(SELRec_T);

    }
    else
    {
        pBMCInfo->SELConfig.LastEvtTS = (pBMCInfo->SELReclaimRepos.pSELReclaimInfo->AddTimeStamp > pBMCInfo->SELReclaimRepos.pSELReclaimInfo->EraseTimeStamp) ?
                                                                  pBMCInfo->SELReclaimRepos.pSELReclaimInfo->AddTimeStamp : pBMCInfo->SELReclaimRepos.pSELReclaimInfo->EraseTimeStamp;
        pBMCInfo->SELConfig.MaxSELRecord = (pBMCInfo->IpmiConfig.SELAllocationSize * 1024)/sizeof(SELRec_T);
    }
    pBMCInfo->SELConfig.RsrvIDCancelled = TRUE;
    pBMCInfo->SELConfig.SELOverFlow = FALSE;

    if(BMC_GET_SHARED_MEM (BMCInst)->InitSELDone == SEL_UNINITIALIZED)
    {
        //To get the data across the processes added in Shared memory structure in SharedMem.h

        BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID = 0;
        BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID = 0;
        BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex = 0;
        BMC_GET_SHARED_MEM (BMCInst)->InitSELDone = SEL_INITIALIZED;

        //To get the data across the processes added in Shared memory structure in SharedMem.
        if(!ReclaimSELSpace)
        {
            m_sel->NumRecords = 0;
            pBMCInfo->SELConfig.SELCnt = 0; /*It will be used to get the correct SEL count when the SEL reclaim feature is not enabled*/
            pSELRecord = m_sel->SELRecord;

        while (1)
        {
            if (VALID_RECORD == pSELRecord->Valid)
            {
                m_sel->NumRecords++;
                BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID = pSELRecord->EvtRecord.hdr.ID;
                BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex +=1;
                if(BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID == 0)
                {
                    BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID = pSELRecord->EvtRecord.hdr.ID;
                }
                pBMCInfo->SELConfig.SELCnt++;
                pSELRecord++;
            }
            else if (STATUS_DELETE_SEL == pSELRecord->Valid)
            {
                pSELRecord++;
                m_sel->NumRecords++;
                BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex += 1;
            }
            else
            {
                break;
            }
        }

            /*Update the First and Last Record ID for CircularSEL*/
            if(m_sel->NumRecords == pBMCInfo->SELConfig.MaxSELRecord)
            {
                 (BMCInst);
            }
            IPMI_DBG_PRINT_1("Num SEL Records = %x\n", m_sel->NumRecords);
         }
        else
        {
                 if(SEL_RECLAIM_HEAD_NODE(BMCInst) != NULL)
                 {
                    BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID= SEL_RECLAIM_HEAD_NODE(BMCInst)->SELRecord.EvtRecord.hdr.ID;
                    BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID = SEL_RECLAIM_HEAD_NODE(BMCInst)->SELRecord.EvtRecord.hdr.ID;
                 }

                 if(SEL_RECLAIM_TAIL_NODE(BMCInst) != NULL)
                 {
                    BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID = SEL_RECLAIM_TAIL_NODE(BMCInst)->SELRecord.EvtRecord.hdr.ID;
                 }
        }

        if(g_PDKHandle[PDK_GETSELLIMIT] != NULL)
        {
            pBMCInfo->SELConfig.SELLimit =((INT8U(*)(int)) g_PDKHandle[PDK_GETSELLIMIT])(BMCInst);
        }
       OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
    }
    else
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
    }
    if(g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrArmed==FALSE)
    {
        pBMCInfo->PefConfig.PEFTmrMgr.TakePEFAction = TRUE;
    }

    CheckLastSELRecordID (BMCInst);

    return 0;
}


/*---------------------------------------
 * GetSELInfo
 *---------------------------------------*/
int
GetSELInfo (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SELInfo_T*   pSelInfo = (_NEAR_ SELInfo_T*) pRes;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_ SELRepository_T* _FAR_	m_sel;

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    if(g_corefeatures.del_sel_reclaim_support != ENABLED)
    {
        m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
        pSelInfo->RecCt          = htoipmi_u16 (pBMCInfo->SELConfig.SELCnt);
        pSelInfo->FreeSpace      = htoipmi_u16 ((g_BMCInfo[BMCInst].SELConfig.MaxSELRecord - m_sel->NumRecords) * sizeof(SELRec_T));
        pSelInfo->AddTimeStamp   = htoipmi_u16(m_sel->AddTimeStamp);
        pSelInfo->EraseTimeStamp =htoipmi_u16( m_sel->EraseTimeStamp);
    }
    else
    {
        pSelInfo->RecCt          = htoipmi_u16 (pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords);
        pSelInfo->FreeSpace      = htoipmi_u16 ((g_BMCInfo[BMCInst].SELConfig.MaxSELRecord - pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords) * sizeof(SELRec_T));
        pSelInfo->AddTimeStamp   = htoipmi_u16(pBMCInfo->SELReclaimRepos.pSELReclaimInfo->AddTimeStamp);
        pSelInfo->EraseTimeStamp =htoipmi_u16( pBMCInfo->SELReclaimRepos.pSELReclaimInfo->EraseTimeStamp);
    }

    pSelInfo->CompletionCode = CC_NORMAL;
    pSelInfo->Version        = SEL_VERSION;
    pSelInfo->OpSupport=NO_SUPPORT;

    if(g_BMCInfo[BMCInst].SELConfig.SELOverFlow == TRUE)
    {
        pSelInfo->OpSupport |= OVERFLOW_FLAG;
    }

    if(GetCommandEnabledStatus(NETFN_STORAGE,NULL,CMD_RESERVE_SEL,BMCInst) == 0)
        {
            pSelInfo->OpSupport|= RESERVE_SEL_SUPPORT;
        }
    if(GetCommandEnabledStatus(NETFN_STORAGE,NULL,CMD_GET_SEL_ALLOCATION_INFO,BMCInst) == 0)
        {
            pSelInfo->OpSupport|= GET_SEL_ALLOC_SUPPORT;
        }
    if(GetCommandEnabledStatus(NETFN_STORAGE,NULL,CMD_PARTIAL_ADD_SEL_ENTRY ,BMCInst) == 0)
        {      
            pSelInfo->OpSupport|=PARTIAL_ADD_SEL_SUPPORT;
        }
    if(GetCommandEnabledStatus(NETFN_STORAGE,NULL,CMD_DELETE_SEL_ENTRY,BMCInst) == 0)
        {
            pSelInfo->OpSupport|= DELETE_SEL_SUPPORT;
        }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    return sizeof (SELInfo_T);
}


/*---------------------------------------
 * GetSELAllocationInfo
 *---------------------------------------*/
int
GetSELAllocationInfo(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SELAllocInfo_T* pAllocInfo = (_NEAR_ SELAllocInfo_T*) pRes;
    _FAR_ SELRepository_T* _FAR_	m_sel;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    pAllocInfo->CompletionCode    = CC_NORMAL;
    pAllocInfo->NumAllocUnits     = htoipmi_u16 (g_BMCInfo[BMCInst].SELConfig.MaxSELRecord);
    pAllocInfo->AllocUnitSize     = htoipmi_u16 (sizeof (SELRec_T));

    if(g_corefeatures.del_sel_reclaim_support != ENABLED)
    {
        m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
        pAllocInfo->NumFreeAllocUnits = htoipmi_u16 ((g_BMCInfo[BMCInst].SELConfig.MaxSELRecord - m_sel->NumRecords));
        pAllocInfo->LargestFreeBlock  = htoipmi_u16 ((g_BMCInfo[BMCInst].SELConfig.MaxSELRecord - m_sel->NumRecords));
    }
    else
    {
        pAllocInfo->NumFreeAllocUnits = htoipmi_u16 ((g_BMCInfo[BMCInst].SELConfig.MaxSELRecord -pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords));
        pAllocInfo->LargestFreeBlock  = htoipmi_u16 ((g_BMCInfo[BMCInst].SELConfig.MaxSELRecord - pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords));
    }

    pAllocInfo->MaxRecSize        = sizeof (SELRec_T)/pAllocInfo->AllocUnitSize;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    return sizeof (SELAllocInfo_T);
}


/*---------------------------------------
 * ReserveSEL
 *---------------------------------------*/
int
ReserveSEL (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ ReserveSELRes_T* pRsvSelRes = (_NEAR_ ReserveSELRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    g_BMCInfo[BMCInst].SELConfig.SelReservationID=rand();
    if (0 == g_BMCInfo[BMCInst].SELConfig.SelReservationID) { g_BMCInfo[BMCInst].SELConfig.SelReservationID = 1; }
    pRsvSelRes->CompletionCode = CC_NORMAL;
    pRsvSelRes->ReservationID  = g_BMCInfo[BMCInst].SELConfig.SelReservationID;
    g_BMCInfo[BMCInst].SELConfig.RsrvIDCancelled = FALSE;
    g_BMCInfo[BMCInst].SELConfig.PartialAdd=0 ;//Addded for DR#30287 
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
    return sizeof (ReserveSELRes_T);
}


/*---------------------------------------
 * GetSELEntry
 *---------------------------------------*/
int
GetSELEntry (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)      
{
    _FAR_  SELRec_T*    pSelRec;
    _FAR_  SELRec_T*    pNextSelRec;
    _NEAR_ GetSELReq_T* pGetSelReq = (_NEAR_ GetSELReq_T*) pReq;
    _NEAR_ GetSELRes_T* pGetSelRes = (_NEAR_ GetSELRes_T*) pRes;
    INT16U         LastRecID = 0,FirstRecID = 0;
    _FAR_ SELRepository_T* _FAR_	m_sel = NULL;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT16U NumRecords = 0;
    BOOL SELReclaim,CircularSEL = FALSE;

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    if(g_corefeatures.circular_sel == ENABLED)
    {
        CircularSEL = pBMCInfo->AMIConfig.CircularSEL;
    }

    SELReclaim = g_corefeatures.del_sel_reclaim_support;

   if(!SELReclaim)
   {
        m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
        NumRecords = m_sel->NumRecords;
   }
   else
   {
       NumRecords = pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords;
   }

    pGetSelRes->CompletionCode = CC_NORMAL;
    if (pGetSelReq->ReservationID && g_BMCInfo[BMCInst].SELConfig.RsrvIDCancelled)
    {
        pGetSelRes->CompletionCode = CC_INV_RESERVATION_ID;
         OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (INT8U);
    }

    /* Check if the reservation IDs match */
    if ((g_BMCInfo[BMCInst].SELConfig.SelReservationID != pGetSelReq->ReservationID) &&
        ((0 != pGetSelReq->Offset) || (pGetSelReq->ReservationID != 0)) )
    {
        pGetSelRes->CompletionCode = CC_INV_RESERVATION_ID;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (INT8U);
    }

    if ((!SELReclaim) && (0 == m_sel->NumRecords))
    { 
        pGetSelRes->CompletionCode = CC_SEL_REC_NOT_PRESENT;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (INT8U);
    }
    else if((SELReclaim) && (0 == pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords))
    {
        pGetSelRes->CompletionCode = CC_SEL_REC_NOT_PRESENT;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (INT8U);
    }

    FirstRecID = BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID;
    LastRecID = BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID;

    /* If ID == 0x0000  return first record */
    if (0 == pGetSelReq->RecID)
    {
        pSelRec = GetSELRec(FirstRecID, BMCInst);
        if (0 == pSelRec) 
        {
            pGetSelRes->CompletionCode = CC_SEL_REC_NOT_PRESENT;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return sizeof(INT8U);
        }
        else 
        { 
            pNextSelRec = GetNextSELEntry (pSelRec, BMCInst); 
        }
        if (0 == pNextSelRec) 
        { 
            pGetSelRes->NextRecID = 0xFFFF;
        }
        else
        {
            pGetSelRes->NextRecID = pNextSelRec->EvtRecord.hdr.ID;
        }
    }
    else if (0xFFFF == pGetSelReq->RecID)
    {
        pSelRec = GetSELRec(LastRecID, BMCInst);
        if(0== pSelRec)
        {
            pGetSelRes->CompletionCode = CC_SEL_REC_NOT_PRESENT;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return sizeof(INT8U);
        }
        pGetSelRes->NextRecID = 0xFFFF;
    }
/*    else if (!CircularSEL && pGetSelReq->RecID > LastRecID)
    {*/
        /* When Circular SEL is enabled and SEL buffer is full,
           LastRecID will reset from zero, the RecID may greater than LastRecID.
           Thus don't check this in Circular SEL mode. */
/*        pGetSelRes->CompletionCode = CC_SEL_REC_NOT_PRESENT;
        return sizeof(INT8U);
    }*/
    else 
    {
        pSelRec = GetSELRec(pGetSelReq->RecID,BMCInst);
        if (pSelRec == 0)
        {
            pGetSelRes->CompletionCode = CC_SEL_REC_NOT_PRESENT;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return sizeof (INT8U);
        }

        if(LastRecID == pGetSelReq->RecID)
        {
            pGetSelRes->NextRecID = 0xFFFF;
        }
        else
        {
            pNextSelRec = GetNextSELEntry (pSelRec,BMCInst);
            if (0 == pNextSelRec) 
            {
                /*If the given SEL ID is the maximum SEL record ,then the next SEL ID will be the First SEL ID 
                    in case of Circular SEL Implementation */
                if( CircularSEL && (pBMCInfo->SELConfig.MaxSELRecord == NumRecords))
                {
                   if(!SELReclaim)
                   {
                        pGetSelRes->NextRecID = pSelRec->EvtRecord.hdr.ID+1;
                   }
                   else
                   {
                        pGetSelRes->NextRecID = GetNextReclaimRecordID(BMCInst);
                   }
                }
                else
                {
                    pGetSelRes->NextRecID = 0xFFFF;
                    IPMI_DBG_PRINT ("SEL: Last Record returned 0xFFFF\n");
                }
            }
            else 
            {
                pGetSelRes->NextRecID = pNextSelRec->EvtRecord.hdr.ID;
            }
        }

    }

    if ((0xff == pGetSelReq->Size) && (sizeof (SELEventRecord_T) >= pGetSelReq->Offset)) 
    { 
        pGetSelReq->Size = sizeof (SELEventRecord_T) - pGetSelReq->Offset;        
    }
    // Check for the request bytes and offset not to exceed the actual size of the record
    else if ((pGetSelReq->Size > (sizeof(SELEventRecord_T))) ||
             (pGetSelReq->Offset > (sizeof(SELEventRecord_T))) ||
             (pGetSelReq->Size > ((sizeof(SELEventRecord_T))- pGetSelReq->Offset)))
    {
        pGetSelRes->CompletionCode = CC_PARAM_OUT_OF_RANGE;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (INT8U);
    }
     _fmemcpy(pGetSelRes + 1, ((_FAR_ INT8U*)&pSelRec->EvtRecord) +
             pGetSelReq->Offset, pGetSelReq->Size);

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
    return sizeof(GetSELRes_T) + pGetSelReq->Size;
}

/*---------------------------------------
 * BSODCaptureScreen 
 *---------------------------------------*/
void BSODCaptureScreen(_NEAR_ SELEventRecord_T*  EvtRecord)
{
    void *phandle;
    int (*capturecrashscreen) (char *);
    char command[256] = {0};
    capturecrashscreen = NULL;
    int retval = -1;
     
    /*No need to check complete byte, only LSB is enough to check that the sel event is of OS Runtime error or not*/

/*	Sensor Specific Offsets and their Events for Sensor Type 20h(OS Stop/Shutdown)
	00h - Critical Stop during OS load/initialization
	01h - Runtime Critical Stop / BSOD / Core Dump
	02h - OS Graceful Stop
	03h - OS Graceful Shutdown
	04h - Soft shutdown initiated by PEF
*/
    if((EvtRecord->SensorType == SENSOR_TYPE_OS_CRITICAL_STOP) && ((EvtRecord->EvtData1 & SENSOR_SPECIFIC_OFFSET_MASK) == OS_RUNTIME_CRITICAL_STOP))
    {
        phandle = NULL;
        
        if(g_corefeatures.capture_bsod_raw == ENABLED)
        {
            phandle = dlopen((char *)VIDEO_LIB,RTLD_LAZY);
            if(!phandle)
            {
                TCRIT("Error in loading video library %s\n",dlerror());
                return;
            }

            capturecrashscreen = dlsym(phandle,CAP_CRASHSCREEN_FUNC);
            if(capturecrashscreen  == NULL)
            {
                TCRIT("Error in Calling %s function\n","Capture_CrashScreen");
                dlclose(phandle);
                return;
            }
        }

        sprintf(command, "mkdir %s 2&>/dev/null", CRASH_SCREEN_DIRECTORY);
        safe_system(command);
        if( g_corefeatures.capture_bsod_jpeg == ENABLED)
        {
            sprintf(command, "rm %s 2&>/dev/null",CRASH_SCREEN_FILE_JPEG);
        }
        else if(g_corefeatures.capture_bsod_raw == ENABLED)
        {
            sprintf(command, "rm %s 2&>/dev/null",CRASH_SCREEN_FILE);
        }
        safe_system(command);

        if( g_corefeatures.capture_bsod_jpeg == ENABLED)
        {
        	if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE] != NULL)
        	{
        		retval = ((int(*)(int , int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE])(BSOD_CAPTURE,0);
        		if(retval <0)
        			IPMI_ERROR("Error in Writing data to Adviser\n");
        	}
        }
        else if(g_corefeatures.capture_bsod_raw== ENABLED)
        {
            if(capturecrashscreen(CRASH_SCREEN_FILE) < 0)
                TCRIT("Capture Failed\n");

            if(phandle != NULL)
                dlclose(phandle);
        }
    }
    return;
}
/*---------------------------------------
 * LockedAddSELEntry with SEL locked 
 *---------------------------------------*/
int
LockedAddSELEntry (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes, INT8U SysIfcFlag, INT8U SelectTbl, int BMCInst)
{
    _NEAR_ AddSELRes_T*     pAddSelRes = (_NEAR_ AddSELRes_T*) pRes;
    _NEAR_ SELRecHdr_T*     pSelRec    = (_NEAR_ SELRecHdr_T*) pReq;
    _FAR_  PEFRecordDetailsConfig_T*     nvrPefRecordDetails;
    _FAR_   AMIConfig_T*    pAMIcfg;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_  INT8U            SelExceededAction=0;
    static _FAR_  INT8U     NumSelAccum=0;
    INT16U         LastRecID = 0;
    int    nRet=0;
    SELRec_T NodeSELRecord;
    _FAR_ SELRepository_T* _FAR_	m_sel = NULL;
    INT16U NumRecords=0;
    BOOL CircularSEL = FALSE;
    BOOL SELReclaim = FALSE;

    if(BMC_GET_SHARED_MEM(BMCInst)->InitSELDone == SEL_UNINITIALIZED)
    {
        pAddSelRes->CompletionCode = CC_INIT_AGENT_IN_PROGRESS;
        return sizeof(INT8U);
    }

    if(g_corefeatures.circular_sel == ENABLED)
    {
        CircularSEL = pBMCInfo->AMIConfig.CircularSEL;
    }

   SELReclaim = g_corefeatures.del_sel_reclaim_support;

    // Send event to Automation engine
    AES_SendEvent((SELEventRecord_T *)pReq);

    nvrPefRecordDetails = &pBMCInfo->PEFRecordDetailsConfig;
    pAMIcfg = &pBMCInfo->AMIConfig;

    if((g_corefeatures.ssi_event_forward == ENABLED) && (g_corefeatures.ssi_support == ENABLED))
    {
        if (g_SSIHandle[SSICB_EVENTFWD] != NULL)
        {
            ((void(*)(SELEventRecord_T *, int))g_SSIHandle[SSICB_EVENTFWD]) ((SELEventRecord_T*)pSelRec, BMCInst);
        }
    }

    /* PDK Module Pre Add SEL Control function*/
    if(g_PDKHandle[PDK_PREADDSEL] != NULL)
    {
        SelectTbl = ((INT8U(*)(INT8U *, INT8U, int)) g_PDKHandle[PDK_PREADDSEL])((_FAR_ INT8U*)pSelRec, SelectTbl, BMCInst);
    }
    //if event type is critical,need to intimate adviser for stope and merge pre crash video
    if((((SELEventRecord_T*)pSelRec)->SensorType == SENSOR_TYPE_OS_CRITICAL_STOP) && ((((SELEventRecord_T*)pSelRec)->EvtData1 & SENSOR_SPECIFIC_OFFSET_MASK) == OS_RUNTIME_CRITICAL_STOP))
    {
        if( ( g_corefeatures.record_pre_boot_or_crash_video == ENABLED ) && ( pBMCInfo->TriggerEvent.PreBSODRcdFlag ) )
        {
            if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE] != NULL)
            {
                if( 0 > ((int(*)(int , int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE])(BSOD_SIGNAL,0) )
                    IPMI_ERROR("Error in Writing data to Adviser\n");
            }
        }
    }

    if(g_corefeatures.capture_bsod == ENABLED)
    {
        BSODCaptureScreen((SELEventRecord_T*)pSelRec);
    }

    if(!SysIfcFlag) // Event logging via the System Interface is always enabled 
    {
        // If SEL logging is disabled, Post the event to PEF and return Invalid ID
        if(0 == (BMC_GET_SHARED_MEM(BMCInst)->GlobalEnables & SYS_EVENT_LOGGING_MASK))
        {
            IPMI_DBG_PRINT_1("BMC SEL Logging is disabled; BMC Global Enable Byte is %x", BMC_GET_SHARED_MEM()->GlobalEnables);
            pSelRec->ID = 0xFFFF;
            if(SelectTbl & ENABLE_PEF_MASK)
            {
                PostSELToPEF((SELEventRecord_T*)pSelRec,BMCInst);
                nvrPefRecordDetails->LastBMCProcessedEventID = 0;
            }
            else
            {
                nvrPefRecordDetails->LastBMCProcessedEventID = ((SELEventRecord_T*)pSelRec)->hdr.ID;
            }

            if(g_corefeatures.sel_write_background == ENABLED)
            {
                PostSELToFlush(FLUSH_BMC_PROC_EVT_ID,NULL,BMCInst);
            }
            else
            {
                FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                                pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID),BMCInst);
            }

            pAddSelRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
            return sizeof(INT8U);
        }
    }

    if(!SELReclaim)
    {
        m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
        NumRecords = m_sel->NumRecords;
    }
    else
    {
        NumRecords = pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords;
    }

    /* If we dont have enough space return invalid ID */
    if (NumRecords >= pBMCInfo->SELConfig.MaxSELRecord)
    {
        /* If record buffer is full, reset the m_LastRecID index starting from 0 */
        if (CircularSEL)
        {
            if ((BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex == pBMCInfo->SELConfig.MaxSELRecord))
            {
			  if(!SELReclaim)
			  {
                BMC_GET_SHARED_MEM(BMCInst)->m_SELIndex = 0;
			  }
            }
            /*Reset the SEL OverFlow Flag in Circular SEL mode*/
            pBMCInfo->SELConfig.SELOverFlow = FALSE;
        }
        else
        {
            /*Even though SEL is full, PEF action has to be taken. 
            So Posting SEL to PEF Task */ 
            pSelRec->ID = 0xFFFF;     
            if(SelectTbl & ENABLE_PEF_MASK)
            {
                PostSELToPEF ((SELEventRecord_T*)pSelRec,BMCInst);
                nvrPefRecordDetails->LastBMCProcessedEventID = 0;
            }
            else
            {
                nvrPefRecordDetails->LastBMCProcessedEventID = ((SELEventRecord_T*)pSelRec)->hdr.ID;
            }
            pBMCInfo->SELConfig.SELOverFlow = TRUE;
            if(g_corefeatures.sel_write_background == ENABLED)
            {
                PostSELToFlush(FLUSH_BMC_PROC_EVT_ID,NULL,BMCInst);
            }
            else
            {
                FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                      pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID),BMCInst);
            }
            pAddSelRes->CompletionCode = CC_OUT_OF_SPACE;
            return sizeof (INT8U);
        }
    }

    if(!SELReclaim)
    {
        if( BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID == 0xFFFE)
        {
            BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID = 0;
            /*Enable Circular overflow Flag*/
            pAMIcfg->CircularSELFlag = 1;

            if(g_corefeatures.sel_write_background == ENABLED)
            {
                PostSELToFlush(FLUSH_CIRCULAR_SEL_FLAG,NULL,BMCInst);
            }
            else
            {
                FlushIPMI((INT8U*)&pBMCInfo->AMIConfig,(INT8U*)&pBMCInfo->AMIConfig.CircularSELFlag,pBMCInfo->IPMIConfLoc.AMIConfigAddr,sizeof(INT8U),BMCInst);
            }
        }

        if( BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID == 0xFFFE)
        {
            BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID = 0;
            /*Disable Circular overflow Flag*/
            pAMIcfg->CircularSELFlag = 0;
            if(g_corefeatures.sel_write_background == ENABLED)
            {
                PostSELToFlush(FLUSH_CIRCULAR_SEL_FLAG,NULL,BMCInst);
            }
            else
            {
                FlushIPMI((INT8U*)&pBMCInfo->AMIConfig,(INT8U*)&pBMCInfo->AMIConfig.CircularSELFlag,pBMCInfo->IPMIConfLoc.AMIConfigAddr,sizeof(INT8U),BMCInst);
            }
        }

        if(CircularSEL && NumRecords == pBMCInfo->SELConfig.MaxSELRecord)
        {
            BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID +=1;
        }
    }

    if(SelectTbl & ENABLE_SEL_MASK)
    { 
        /* time stamp for record type less than 0xE0*/
        if (pSelRec->Type < 0xE0)
        {
            if((NULL == g_PDKHandle[PDK_SELTIMESTAMP]) || (-1 == ((int(*)(SELEventRecord_T*))(g_PDKHandle[PDK_SELTIMESTAMP]))((SELEventRecord_T*)pSelRec)))
            {
                // The timestamp gets set if the hook is not present or if the hook returns -1 
                pSelRec->TimeStamp = GetSelTimeStamp (BMCInst);
            }
        }
        if(!SELReclaim)
       {
             pSelRec->ID = BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID +1;
             LastRecID = BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex ;

             _fmemcpy (&m_sel->SELRecord [LastRecID].EvtRecord , pSelRec, 
                              sizeof (SELEventRecord_T));

            m_sel->SELRecord [LastRecID].Valid  = VALID_RECORD;
            m_sel->SELRecord [LastRecID].Len  = sizeof (SELEventRecord_T) + 2;
       }
        else
       {
           pSelRec->ID = GetNextReclaimRecordID(BMCInst);
           if(pSelRec->ID == 0)
           {
               IPMI_WARNING("SEL Initialization Agent Running....\n");
               pAddSelRes->CompletionCode = CC_INIT_AGENT_IN_PROGRESS;
               return sizeof (INT8U);
           }

           NodeSELRecord.Valid = VALID_RECORD;
           NodeSELRecord.Len = sizeof(SELEventRecord_T)+2;

           memcpy(&NodeSELRecord.EvtRecord,pSelRec,sizeof(SELEventRecord_T));
       }
       
        if ((0 == pBMCInfo->SELConfig.SenMonSELFlag) || (NumSelAccum > 2)
            || (g_corefeatures.sel_write_background == ENABLED))
        {
            if(!SELReclaim)
            {
                if(g_corefeatures.sel_write_background == ENABLED)
                {
                    PostSELToFlush(FLUSH_SEL_REC,&LastRecID,BMCInst);
                }
                else
                {
                    FlushSEL (&m_sel->SELRecord [LastRecID - NumSelAccum],
                                  sizeof(SELRec_T) * (NumSelAccum + 1), nRet,BMCInst);
                }
             }
             else
             {
                 nRet = FlushAddReclaimSEL(&NodeSELRecord,BMCInst);
             }

            if(g_corefeatures.sel_write_background != ENABLED)
            {
#if IPM_DEVICE == 1
                if (-1 == nRet)
                {
                        g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
                }
                else
                {
                        g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
                }
#endif
            }
            NumSelAccum = 0;
        }
        else
        {
            if(g_corefeatures.sel_write_background != ENABLED)
            {
                NumSelAccum += 1;
            }
        }
        
        if (!CircularSEL || (CircularSEL && NumRecords < pBMCInfo->SELConfig.MaxSELRecord))
        {
            /* Update the fields in SEL Repository */
            if(!SELReclaim)
            {
                ++pBMCInfo->SELConfig.SELCnt;
                NumRecords = ++m_sel->NumRecords;
            }
            else
            {
                NumRecords = ++pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords;
            }
        }

        /*Update the First SEL entry after clear SEL command*/
        if(!SELReclaim)
        {
            if( BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID == 0)
            {
                BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID = pSelRec->ID;
            }

          BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID += 1;
          BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex += 1;

           pBMCInfo->SELConfig.LastEvtTS = m_sel->AddTimeStamp = GetSelTimeStamp (BMCInst);

            if(g_corefeatures.sel_write_background == ENABLED)
            {
                PostSELToFlush(FLUSH_SEL_TIMESTAMP,NULL,BMCInst);
            }
            else
            {
                FlushSEL (&m_sel->AddTimeStamp, sizeof(INT32U), nRet,BMCInst);
            }

            if(g_corefeatures.sel_write_background != ENABLED)
            {
#if IPM_DEVICE == 1
                if (-1 == nRet)
                {
                        g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
                }
                else
                {
                        g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
                }
#endif
            }
        }
        else
        {
            pBMCInfo->SELConfig.LastEvtTS = pBMCInfo->SELReclaimRepos.pSELReclaimInfo->AddTimeStamp = GetSelTimeStamp(BMCInst);

           nRet = SaveSELReclaimInfo(pBMCInfo->SELReclaimRepos.pSELReclaimInfo,BMCInst);
#if IPM_DEVICE == 1
            if (-1 == nRet)
            {
                    g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
            }
            else
            {
                    g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
            }
#endif
        }

        nvrPefRecordDetails->LastSELRecordID = pSelRec->ID;

        if(g_corefeatures.sel_write_background == ENABLED)
        {
            PostSELToFlush(FLUSH_LAST_RECID,NULL,BMCInst);
        }
        else
        {
            FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
                          pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID),BMCInst);
        }

        /* Ivalidate Reservation ID if any */
        pBMCInfo->SELConfig.RsrvIDCancelled = TRUE;

        /* check for SEL limit as specfied by OEM */
        if (0 != pBMCInfo->SELConfig.SELLimit)
        {
            if (NumRecords > (pBMCInfo->SELConfig.MaxSELRecord * pBMCInfo->SELConfig.SELLimit / 100))
            {
                if(g_PDKHandle[PDK_SELLIMITEXCEEDED] != NULL)
                {
                    SelExceededAction =((INT8U(*)(int)) g_PDKHandle[PDK_SELLIMITEXCEEDED])(BMCInst);
                }

                switch (SelExceededAction)
                {
                    case SET_MSG_FLAG_OEM_0 :   /* set OEM 0 bit */
                    case SET_MSG_FLAG_OEM_1 :   /* set OEM 1 bit */
                    case SET_MSG_FLAG_OEM_2 :   /* set OEM 2 bit */
                        BMC_GET_SHARED_MEM (BMCInst)->MsgFlags |= SelExceededAction;
                        /* set SYS_ATT bit */
                        //SET_SMS_ATN ();
                        if (pBMCInfo->IpmiConfig.KCS1IfcSupport == 1)
                        {
                            SET_SMS_ATN (0, BMCInst);
                        }
    
                        if (pBMCInfo->IpmiConfig.KCS2IfcSupport == 1)
                        {
                            SET_SMS_ATN (1, BMCInst);
                        }
    
                        if (pBMCInfo->IpmiConfig.KCS3IfcSuppport == 1)
                        {
                            SET_SMS_ATN (2, BMCInst);
                        }
                        if(pBMCInfo->IpmiConfig.BTIfcSupport == 1 )
                        {
                            SET_BT_SMS_ATN (0, BMCInst);
                        }
                        break;
                    default :
                        break;
                }
            }
        }

        /* PDK Module Post Add SEL Control function*/
        if(g_PDKHandle[PDK_POSTADDSEL] != NULL)
        {
            ((INT8U(*)(INT8U *,int)) g_PDKHandle[PDK_POSTADDSEL])((_FAR_ INT8U*)pSelRec,BMCInst);
        }

        pAddSelRes->CompletionCode = CC_NORMAL;
        pAddSelRes->RecID          = pSelRec->ID;
	
	// Sending notification to CIM after successful SEL addition
        if(g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM] != NULL)
        {
            uint8 CMD;
            
	    // Set bits for SEL Event & Add operation
            CMD = 0x21;

            ((int(*)(uint8, uint16))g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM])(CMD, pAddSelRes->RecID);

        }

        /* Posting SEL to PEF Task */
        if(SelectTbl & ENABLE_PEF_MASK)
        {
            PostSELToPEF ((SELEventRecord_T*)pSelRec,BMCInst);
        }
        else
        {
            pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID = ((SELEventRecord_T*)pSelRec)->hdr.ID;

            if(g_corefeatures.sel_write_background == ENABLED)
            {
                PostSELToFlush(FLUSH_BMC_PROC_EVT_ID,NULL,BMCInst);
            }
            else
            {
                FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig, (INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                    pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr, sizeof(pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID), BMCInst);
            }
        }
        
        /* Check if SEL Full Pecentage is more than 75% */
        if(g_corefeatures.internal_sensor == ENABLED )
        {
            if ((GetSELFullPercentage(BMCInst) >= SEL_ALMOST_FULL_PERCENTAGE) && (g_BMCInfo[BMCInst].SELConfig.selalmostfull != 1))
            {
                /* Set SEL reading to be almost full */
                SetSELSensorReading (EVENT_SEL_ALMOST_FULL,BMCInst);
                g_BMCInfo[BMCInst].SELConfig.selalmostfull = 1;
            }

        
            /* Check if SEL is full or not  */
            if (!CircularSEL && NumRecords == (pBMCInfo->SELConfig.MaxSELRecord - 1))
            {
                SetSELSensorReading ( EVENT_SEL_IS_FULL,BMCInst);
                LogSELFullEvent(BMCInst);
            }
        }

        return sizeof (AddSELRes_T);
    }
    else
    {
        IPMI_DBG_PRINT("BMC SEL Logging and PEF action is disabled by PreAddSEL");
        pSelRec->ID = 0xFFFF;
        pAddSelRes->CompletionCode = CC_NORMAL;
        pAddSelRes->RecID = 0x0000;
        return sizeof(AddSELRes_T);
    }
}

/*---------------------------------------
 * AddSELEntry
 *---------------------------------------*/
int
AddSELEntry (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    int reslen = 0;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U curchannel;

    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE)

    if(g_corefeatures.disable_pef_for_sel_entry == ENABLED)
        reslen = LockedAddSELEntry(pReq, ReqLen, pRes, (((SYS_IFC_CHANNEL == (curchannel  & 0xF)) || (pBMCInfo->SMBUSCh == (curchannel  & 0xF))) ? TRUE : FALSE),POST_ONLY_SEL,BMCInst);
    else
        reslen = LockedAddSELEntry(pReq, ReqLen, pRes, (((SYS_IFC_CHANNEL == (curchannel  & 0xF)) || (pBMCInfo->SMBUSCh == (curchannel  & 0xF)))  ? TRUE : FALSE),POST_SEL_AND_PEF,BMCInst);

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    return reslen; 

}


/*---------------------------------------
 * PartialAddSELEntry
 *---------------------------------------*/

int
PartialAddSELEntry (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ PartialAddSELReq_T*  pParAddSelReq = (_NEAR_ PartialAddSELReq_T*) pReq;
    _NEAR_ PartialAddSELRes_T*  pParAddSelRes = (_NEAR_ PartialAddSELRes_T*) pRes;
    SELInfo_T SelInfo;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    INT16U      RecordID=0, ReservationID=0;
    INT8U       RecordOffset=0,Len=0,curchannel;


    ReservationID = (UINT16)(pParAddSelReq->LSBReservationID | 
                                            ((UINT16)pParAddSelReq->MSBReservationID << 8));

    RecordID = (UINT16)(((UINT16)pParAddSelReq->MSBRecordID << 8) |
                                    pParAddSelReq->LSBRecordID);


    RecordOffset = (UINT32)pParAddSelReq->Offset;
    Len = ReqLen;
    
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE)
    /*  Checks for the Valid Reservation ID */
    while(g_BMCInfo[BMCInst].SELConfig.RsrvIDCancelled)
    {
        if((ReservationID == 0) && (pParAddSelReq->Progress == 1) && (ReqLen == SEL_PARTIAL_ADD_CMD_MAX_LEN))
            break;
        else
        {
            if(ReqLen < SEL_PARTIAL_ADD_REQUEST_MISC_BYTES)
            {
                pParAddSelRes->CompletionCode = CC_REQ_INV_LEN;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
                return(sizeof(PartialAddSELRes_T));	
            }
            else
            {
                pParAddSelRes->CompletionCode = CC_INV_RESERVATION_ID;
                pParAddSelRes->RecID          = 0;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
                return(sizeof(PartialAddSELRes_T));
            }
        }
    }

    /* Check for the reserved bytes should b zero */

    if  ( 0 !=  (pParAddSelReq->Progress & RESERVED_BITS_PARTIALADDSELENTRY ) )
    {
        pParAddSelRes->CompletionCode = CC_INV_DATA_FIELD;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof(INT8U);
    }

    if((pParAddSelReq->Progress>1)|| (pParAddSelReq->Offset>SEL_RECORD_SIZE))
    {
        pParAddSelRes->CompletionCode = CC_PARAM_OUT_OF_RANGE;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return(sizeof(PartialAddSELRes_T));
    }
    else if(g_BMCInfo[BMCInst].SELConfig.PartialAdd==0)
    {
        if(((RecordOffset+(Len-SEL_PARTIAL_ADD_REQUEST_MISC_BYTES)==SEL_RECORD_SIZE)&&(pParAddSelReq->Progress!=1))||
        (( RecordOffset+(Len-SEL_PARTIAL_ADD_REQUEST_MISC_BYTES)<SEL_RECORD_SIZE)&&(pParAddSelReq->Progress==1))||
        (RecordOffset+(Len-SEL_PARTIAL_ADD_REQUEST_MISC_BYTES)>SEL_RECORD_SIZE))

        {
            pParAddSelRes->CompletionCode = CC_REQ_INV_LEN;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return(sizeof(PartialAddSELRes_T));
        }
    }
    else if((ReservationID==0)&&(Len==SEL_PARTIAL_ADD_CMD_MAX_LEN)&&(pParAddSelReq->Progress!=1))
    {
        pParAddSelRes->CompletionCode =CC_INV_DATA_FIELD;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return(sizeof(PartialAddSELRes_T));
    }

    /*Checking for reservation ID <<Added few more condition checking */
    if(((ReservationID == g_BMCInfo[BMCInst].SELConfig.SelReservationID)&&(g_BMCInfo[BMCInst].SELConfig.PartialAdd==0)) ||  
    ((ReservationID == g_BMCInfo[BMCInst].SELConfig.SelReservationID) && (ReqLen <= SEL_PARTIAL_ADD_CMD_MAX_LEN))
    ||((ReservationID == 0) && (pParAddSelReq->Progress == 1) && (ReqLen == SEL_PARTIAL_ADD_CMD_MAX_LEN)))  
    {
        if(g_BMCInfo[BMCInst].SELConfig.PartialAdd==0)
        {
            // Requirement says partial adds must start at 0, with no
            // gaps or overlaps. First request must be to RecordID = 0.
            if ((RecordID != 0) || (RecordOffset != 0))
            {
                pParAddSelRes->CompletionCode = CC_INV_DATA_FIELD;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
                return(sizeof(PartialAddSELRes_T));// Modified from sizeof (INT8U);
            }

            _fmemset((void *)&g_BMCInfo[BMCInst].SELConfig.SelPartialAddRecord,
            0xFF, 
            sizeof(SELEventRecord_T));

            // Fill in the record ID into the partial add record. The
            // record ID is always the offset in memory of where the
            // SEL record starts (which includes a delete time first).
            RecordID = g_BMCInfo[BMCInst].SELConfig.SelPartialAddRecord.hdr.ID = BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID +1;

            g_BMCInfo[BMCInst].SELConfig.PartialAddRecordID=RecordID;
            SelInfo.OpSupport |= PARTIAL_ADD_SEL_SUPPORT;
            g_BMCInfo[BMCInst].SELConfig.PartialAddRecOffset = 0;
            g_BMCInfo[BMCInst].SELConfig.PartialAdd=1;
        }
        else
        {
            if (RecordID != g_BMCInfo[BMCInst].SELConfig.PartialAddRecordID)
            {
                pParAddSelRes->CompletionCode = CC_INV_DATA_FIELD;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
                return sizeof (PartialAddSELRes_T); 
            }
        }

        if (pParAddSelReq->Offset != g_BMCInfo[BMCInst].SELConfig.PartialAddRecOffset)
        {
            pParAddSelRes->CompletionCode = CC_INV_DATA_FIELD;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return(sizeof(PartialAddSELRes_T));
        }
                       
        // Checking for Exceeding data values //
        if((g_BMCInfo[BMCInst].SELConfig.PartialAddRecOffset + Len - SEL_PARTIAL_ADD_REQUEST_MISC_BYTES) > SEL_MAX_RECORD_SIZE)
            {
            pParAddSelRes->CompletionCode = CC_REQ_INV_LEN;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return(sizeof(PartialAddSELRes_T));
        }

        if((( g_BMCInfo[BMCInst].SELConfig.PartialAddRecOffset + Len - SEL_PARTIAL_ADD_REQUEST_MISC_BYTES) < SEL_MAX_RECORD_SIZE) && 
        (pParAddSelReq->Progress == 1))	
        {
            pParAddSelRes->CompletionCode = CC_REQ_INV_LEN;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return(sizeof(PartialAddSELRes_T));
        }

        if(((g_BMCInfo[BMCInst].SELConfig.PartialAddRecOffset + Len - SEL_PARTIAL_ADD_REQUEST_MISC_BYTES) == SEL_MAX_RECORD_SIZE) &&
            (pParAddSelReq->Progress != 1))
        {
            pParAddSelRes->CompletionCode = CC_REQ_INV_LEN;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return(sizeof(PartialAddSELRes_T));
        }
        
        if((Len - SEL_PARTIAL_ADD_REQUEST_MISC_BYTES)==0)
		{
			pParAddSelRes->CompletionCode = CC_REQ_INV_LEN;
			return(sizeof(pParAddSelRes->CompletionCode));         
		}

        _fmemcpy((void *)((UINT8 *)&g_BMCInfo[BMCInst].SELConfig.SelPartialAddRecord + RecordOffset),
        (void *)pParAddSelReq->RecordData, 
        ((UINT8)(Len - SEL_PARTIAL_ADD_REQUEST_MISC_BYTES)));
        g_BMCInfo[BMCInst].SELConfig.PartialAddRecordID=RecordID;
        pParAddSelRes->RecID = RecordID;
        g_BMCInfo[BMCInst].SELConfig.PartialAddRecOffset +=	(Len - SEL_PARTIAL_ADD_REQUEST_MISC_BYTES);
        g_BMCInfo[BMCInst].SELConfig.PartialAdd=1;
        pParAddSelRes->CompletionCode = CC_NORMAL;
        //if the progress bit is 1 indicates that this is the last data of the record so put the entire record in sel repository

        if(pParAddSelReq->Progress==1)
        {
            // Checking for complete filling of gap
            if((pParAddSelReq->Offset + Len) == SEL_PARTIAL_ADD_CMD_MAX_LEN)
            {
                g_BMCInfo[BMCInst].SELConfig.PartialAdd=0;
                OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
                if(g_corefeatures.disable_pef_for_sel_entry == ENABLED)
                    LockedAddSELEntry ( (INT8U*)&g_BMCInfo[BMCInst].SELConfig.SelPartialAddRecord, sizeof(SELEventRecord_T),  pRes,(((SYS_IFC_CHANNEL == (curchannel  & 0xF)) || (pBMCInfo->SMBUSCh == (curchannel  & 0xF))) ? TRUE : FALSE),POST_ONLY_SEL,BMCInst);
                else
                    LockedAddSELEntry ( (INT8U*)&g_BMCInfo[BMCInst].SELConfig.SelPartialAddRecord, sizeof(SELEventRecord_T),  pRes,(((SYS_IFC_CHANNEL == (curchannel  & 0xF)) || (pBMCInfo->SMBUSCh == (curchannel  & 0xF))) ? TRUE : FALSE),POST_SEL_AND_PEF,BMCInst);
            }
            else
            {
                pParAddSelRes->CompletionCode = CC_REQ_INV_LEN;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
                return(sizeof(PartialAddSELRes_T));
            }
        }
    }
    else
    {
        if(ReservationID != g_BMCInfo[BMCInst].SELConfig.SelReservationID)
        {
            pParAddSelRes->CompletionCode = CC_INV_RESERVATION_ID;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return(sizeof(PartialAddSELRes_T));
        }
        else
        {
            pParAddSelRes->CompletionCode = CC_INV_DATA_FIELD;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return(sizeof(PartialAddSELRes_T));
        }
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
    return sizeof (PartialAddSELRes_T);

}


/*---------------------------------------
 * DeleteSELEntry
 *---------------------------------------*/
int
DeleteSELEntry (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ DeleteSELReq_T*  pDelSelReq = (_NEAR_ DeleteSELReq_T*) pReq;
    _NEAR_ DeleteSELRes_T*  pDelSelRes = (_NEAR_ DeleteSELRes_T*) pRes;
//    _FAR_  SELRec_T*        pPrevSel;
    INT16U LastRecID = 0,FirstRecID = 0;
    _FAR_ SELRepository_T* _FAR_	m_sel = NULL;
    _FAR_ SELRec_T* pRec = NULL;
    _FAR_   AMIConfig_T*    pAMIcfg;
    int nRet =0;
    int SelReclaim = 0;
    int ReservCmdStat=0;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);

    SelReclaim = g_corefeatures.del_sel_reclaim_support;

    if(!SelReclaim)
    {
        m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
    }
    
    if(GetCommandEnabledStatus(NETFN_STORAGE,NULL,CMD_RESERVE_SEL,BMCInst) == 0)
    {
        ReservCmdStat=ENABLED;
    }
    
    if (TRUE == g_BMCInfo[BMCInst].SELConfig.RsrvIDCancelled)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        pDelSelRes->CompletionCode = CC_INV_RESERVATION_ID;
        return sizeof (INT8U);
    }

    /* Check if the reservation IDs match */
    if (((g_BMCInfo[BMCInst].SELConfig.SelReservationID != pDelSelReq->ReservationID) &&
                                   (pDelSelReq->ReservationID != 0)) || ((pDelSelReq->ReservationID == 0) && ReservCmdStat == ENABLED ))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        pDelSelRes->CompletionCode = CC_INV_RESERVATION_ID;
        return sizeof (INT8U);
    }


    LastRecID = BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID;
    FirstRecID = BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID;

    if ( pDelSelReq->RecID == 0x0000 )
    {
        pDelSelReq->RecID = FirstRecID ;
    }
    else if ( pDelSelReq->RecID == 0xFFFF )
    {
        pDelSelReq->RecID = LastRecID;
    }

    pRec= GetSELRec(pDelSelReq->RecID, BMCInst);
    if ( pRec == 0 ||(pRec->Valid != VALID_RECORD) )
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        pDelSelRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
        return sizeof (INT8U);
    }

    g_BMCInfo[BMCInst].SELConfig.RsrvIDCancelled = TRUE;

    if(!SelReclaim)
    {
        pBMCInfo->SELConfig.SELCnt--;
    //    m_sel->NumRecords--; Do not decrease the sel count, as sel space can't be reclaimed in linear sel 
        pRec->Valid = STATUS_DELETE_SEL;
        FlushSEL(pRec, sizeof(SELRec_T), nRet,BMCInst);
        #if IPM_DEVICE == 1
        if (-1 == nRet)
        {
            g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
         }
        else
        {
           g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
        }
        #endif

        /*Update the Last Deleted SEL time*/
        m_sel->EraseTimeStamp = GetSelTimeStamp (BMCInst);
        FlushSEL (&m_sel->EraseTimeStamp, sizeof(INT32U), nRet,BMCInst);
        #if IPM_DEVICE == 1
        if (-1 == nRet)
        {
             g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
        }
        else
        {
             g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
        }
        #endif
    }
    else
    {
        pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords--;

        /* When there is no record in repository, reset the record postion */
        if(pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords == 0)
        {
             pBMCInfo->SELReclaimRepos.pSELReclaimInfo->MaxRecPos = 0;
        }

        nRet = ReFlushDeleteReclaimSEL( pDelSelReq->RecID, BMCInst);
         if( 0 == nRet )
         {
            AddSELRecordIDNode(&(SEL_RECLAIM_RECORD_HEAD_NODE(BMCInst)),
                                             &(SEL_RECLAIM_RECORD_TAIL_NODE(BMCInst)),pDelSelReq->RecID,BMCInst);
            pBMCInfo->SELReclaimRepos.pSELReclaimInfo->EraseTimeStamp = GetSelTimeStamp(BMCInst);

            nRet = SaveSELReclaimInfo(pBMCInfo->SELReclaimRepos.pSELReclaimInfo,BMCInst);
#if IPM_DEVICE == 1
            if (-1 == nRet)
            {
                    g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
            }
            else
            {
                    g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
            }
#endif
         }
         else
         {
            #if IPM_DEVICE == 1
            if (-1 == nRet)
            {
                g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
            }
            else
            {
                 g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
            }
            #endif
         }
    }

    /*Disable the Overflow Flag if the Last deleted record id is 1*/
if((LastRecID==1) &&(pDelSelReq->RecID == LastRecID)&& (pBMCInfo->AMIConfig.CircularSELFlag) && (!SelReclaim))
{
    pAMIcfg             = &pBMCInfo->AMIConfig;
    pAMIcfg->CircularSELFlag = 0;
    FlushIPMI((INT8U*)&pBMCInfo->AMIConfig,(INT8U*)&pBMCInfo->AMIConfig.CircularSELFlag,
              pBMCInfo->IPMIConfLoc.AMIConfigAddr,sizeof(INT8U),BMCInst);
}
    /*Update the First and last Record ID if the requested RecID is either First or Last*/
    if(!SelReclaim)
    {
        FindRecOrder(BMCInst);
    }


    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    pDelSelRes->CompletionCode  = CC_NORMAL;
    pDelSelRes->RecID           = pDelSelReq->RecID;

    if(g_PDKHandle[PDK_POSTDELETESEL] != NULL)   
    {
        ((INT8U(*)(INT16U, int)) g_PDKHandle[PDK_POSTDELETESEL])(pDelSelReq->RecID, BMCInst);
    }

	// To send notification to CIM
    if(g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM] != NULL)
    {
    	uint8 CMD;
	// Set bits for SEL Event & Delete operation
	CMD = 0x23;
	((int(*)(uint8, uint16))g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM])(CMD, pDelSelReq->RecID);

    }

    return sizeof (DeleteSELRes_T);
}


/*---------------------------------------
 * ClearSEL
 *---------------------------------------*/
int
ClearSEL (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)     
{
    _NEAR_ ClearSELReq_T* pClrSelReq = (_NEAR_ ClearSELReq_T*) pReq;
    _NEAR_ ClearSELRes_T* pClrSelRes = (_NEAR_ ClearSELRes_T*) pRes;
    _FAR_  PEFRecordDetailsConfig_T*   nvrPefRecordDetails;
    _FAR_   AMIConfig_T*    pAMIcfg;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_ SELRepository_T* _FAR_	m_sel = NULL;
    char RemDirName[MAXFILESIZE],DirName[MAXFILESIZE]={0};
	char ExtendedSELCmd[EXTENDED_SEL_PATH_LENGTH];
    int ReclaimSEL,nRet = 0;

   ReclaimSEL = g_corefeatures.del_sel_reclaim_support;

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);

    if(!ReclaimSEL)
    {
        m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
    }

    if (_fmemcmp(pClrSelReq->CLR, "CLR", CLR_SEL_PASSWORD_STR_LEN))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        pClrSelRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    if ( (TRUE == pBMCInfo->SELConfig.RsrvIDCancelled) || (pBMCInfo->SELConfig.SelReservationID != pClrSelReq->ReservationID) )
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        pClrSelRes->CompletionCode = CC_INV_RESERVATION_ID;
        return sizeof (INT8U);
    }


    if (pClrSelReq->InitOrStatus == CLEAR_SEL_GET_STATUS)
    {
        pClrSelRes->CompletionCode = CC_NORMAL;
        pClrSelRes->EraseProgress  = SEL_ERASE_COMPLETED;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (ClearSELRes_T);
    }

    if (pClrSelReq->InitOrStatus != CLEAR_SEL_ACTION)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        pClrSelRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }


    /* Check if the reservation IDs match */
  /*  if (m_SelReservationID != pClrSelReq->ReservationID)
    {
        pClrSelRes->CompletionCode = CC_INV_RESERVATION_ID;
        return sizeof (INT8U);
    }*/


    /* PDK Module Pre Clear SEL Controle function*/
    if(g_PDKHandle[PDK_PRECLEARSEL] != NULL)
    {
        ((INT8U(*)(int)) g_PDKHandle[PDK_PRECLEARSEL])(BMCInst);
    }

   if(!ReclaimSEL)
   {
        /* Clear the fields of the SEL */
        BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID         = 0;
        BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID         = 0;
        BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex         = 0;
        m_sel->NumRecords   = 0;
        pBMCInfo->SELConfig.SELCnt = 0;
        if(g_corefeatures.extend_sel_support == ENABLED)
        {
        	if(g_corefeatures.extended_sel_sd_emmc_support)
        		sprintf( ExtendedSELCmd,"rm -rf %s%d%s",EMMCPATH,g_coremacros.Extended_SEL_partition_num,EXTENDEDSELFOLDER);
        	else
        		sprintf( ExtendedSELCmd,"rm -rf%s%s",SPIPATH,EXTENDEDSELFOLDER);
        	system(ExtendedSELCmd);
        }
        _fmemset (&m_sel->SELRecord [0], 0xFF, pBMCInfo->SELConfig.MaxSELRecord * sizeof (SELRec_T));
        if(g_corefeatures.sel_write_background == ENABLED)
        {
            PostSELToFlush(FLUSH_CLEAR_SEL,NULL,BMCInst);
        }
        else
        {
            FlushSEL (&m_sel->SELRecord [0], pBMCInfo->SELConfig.MaxSELRecord * sizeof (SELRec_T), nRet,BMCInst);
        }
        #if IPM_DEVICE == 1
        if (-1 == nRet)
        {
            g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
        }
        else
        {
            g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
        }
        #endif

        /*Update the Last Deleted SEL time*/
        pBMCInfo->SELConfig.LastEvtTS  = m_sel->EraseTimeStamp = GetSelTimeStamp (BMCInst);
        
        if(g_corefeatures.sel_write_background == ENABLED)
        {
            PostSELToFlush(FLUSH_ERASE_TIMESTAMP,NULL,BMCInst);
        }
        else
        {
            FlushSEL (&m_sel->EraseTimeStamp, sizeof(INT32U), nRet,BMCInst);
        }

        #if IPM_DEVICE == 1
        if (-1 == nRet)
        {
             g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
        }
        else
        {
             g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
        }
        #endif

    }
   else
    {
            BMC_GET_SHARED_MEM(BMCInst)->m_LastRecID = 0;
            BMC_GET_SHARED_MEM(BMCInst)->m_FirstRecID = 0;

            pBMCInfo->SELConfig.LastEvtTS = pBMCInfo->SELReclaimRepos.pSELReclaimInfo->EraseTimeStamp = GetSelTimeStamp (BMCInst);

            pBMCInfo->SELReclaimRepos.pSELReclaimInfo->NumRecords = 0;
            pBMCInfo->SELReclaimRepos.pSELReclaimInfo->LastFileIndex = 0;
            pBMCInfo->SELReclaimRepos.pSELReclaimInfo->MaxRecPos = 0;

            DeleteReclaimAllSELNode(&(SEL_RECLAIM_HEAD_NODE(BMCInst)),&(SEL_RECLAIM_TAIL_NODE(BMCInst)),BMCInst);

           SEL_RECLAIM_DIR(BMCInst,DirName);

            sprintf(RemDirName,"rm -rf %s/*",DirName);
            safe_system(RemDirName);

            nRet = SaveSELReclaimInfo(pBMCInfo->SELReclaimRepos.pSELReclaimInfo,BMCInst);
            #if IPM_DEVICE == 1
            if (-1 == nRet)
            {
                    g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SEL;
            }
            else
            {
                    g_BMCInfo[BMCInst].Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SEL;
            }
            #endif
    }

    nvrPefRecordDetails              = &pBMCInfo->PEFRecordDetailsConfig;
    pAMIcfg             = &pBMCInfo->AMIConfig;
    pAMIcfg->CircularSELFlag = 0;
    nvrPefRecordDetails->LastSELRecordID         = 0xffff;
    nvrPefRecordDetails->LastSWProcessedEventID  = 0xffff;
    nvrPefRecordDetails->LastBMCProcessedEventID = 0xffff;
    /* Flush the values to NVRAM */

    if(g_corefeatures.sel_write_background == ENABLED)
    {
        PostSELToFlush(FLUSH_LAST_RECID,NULL,BMCInst);
        PostSELToFlush(FLUSH_SW_PROC_EVT_ID,NULL,BMCInst);
        PostSELToFlush(FLUSH_BMC_PROC_EVT_ID,NULL,BMCInst);
        if(!ReclaimSEL)
        {
            PostSELToFlush(FLUSH_CIRCULAR_SEL_FLAG,NULL,BMCInst);
        }
    }
    else
    {
        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSELRecordID,
              pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastSWProcessedEventID,
              pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
              pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(INT16U),BMCInst);
        if(!ReclaimSEL)
        {
            FlushIPMI((INT8U*)&pBMCInfo->AMIConfig,(INT8U*)&pBMCInfo->AMIConfig.CircularSELFlag,
                      pBMCInfo->IPMIConfLoc.AMIConfigAddr,sizeof(INT8U),BMCInst);
        }
    }

    pClrSelRes->CompletionCode = CC_NORMAL;
    pClrSelRes->EraseProgress  = SEL_ERASE_COMPLETED;
    pBMCInfo->SELConfig.SELOverFlow = FALSE;
    pBMCInfo->SELConfig.RsrvIDCancelled = TRUE;
    pBMCInfo->SELConfig.selalmostfull = 0;
	  
		// To send notification to CIM      
    if(g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM] != NULL)
    {
        uint8 CMD;
        // set bits for SEL Event & Clear operation
        CMD = 0x24;
        ((int(*)(uint8, uint16))g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM])(CMD, 0);
    }

    /* Add an Clear SEL Log if needed */
    if(g_corefeatures.internal_sensor == ENABLED)
    {
        SetSELSensorReading ( EVENT_LOG_AREA_RESET,BMCInst);
        LogClearSELEvent(BMCInst);
    }

    if(g_PDKHandle[PDK_POSTCLEARSEL] != NULL)	
    {
        ((INT8U(*)(int)) g_PDKHandle[PDK_POSTCLEARSEL])(BMCInst);
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    return sizeof (ClearSELRes_T);
}


/*---------------------------------------
 * GetSELTime
 *---------------------------------------*/
int
GetSELTime (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    INT16 UTCOffset = 0;
    INT32U UTCTime = 0;
    _NEAR_ GetSELTimeRes_T* pGetSelTimeRes = (_NEAR_ GetSELTimeRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    pGetSelTimeRes->CompletionCode = CC_NORMAL;

    //get the UTC time 
    UTCTime = htoipmi_u32(GET_SYSTEM_TIME_STAMP ());
    //get the UTC Offset
    UTCOffset = GetUTC_Offset();
    //calculate the local time from UTC time and offset and return it 
    pGetSelTimeRes->Time = UTCTime + (UTCOffset * 60)/*UTC offset is in minute, so converting it in second*/;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    return sizeof (GetSELTimeRes_T);
}


/**
 * @fn setUTC_Offset
 * @brief Set the UTC offset in the linux kernel.
 * @param[in] UTCOffset - Offset value in minutes.
 * @retval 0, on success.
 *         1, on failure.
 */
static int setUTC_Offset (INT16 UTCOffset)
{
    int mins = 0, hrs = 0;
    char cmd[CMD_LEN], file[32];
    
    if ((SEL_UTC_MIN_RANGE > UTCOffset) || (SEL_UTC_MAX_RANGE < UTCOffset) || (0 != (UTCOffset % 15)))
    {
        return 1;
    }
    
    hrs = UTCOffset / 60;
    mins = UTCOffset % 60;
    
    TDBG ("hrs : %d, mins : %d\n", hrs, mins);
    
    if (0 > UTCOffset)
    {
        sprintf(file, "GMT-%d", (-1 * hrs));
        mins *= -1;
    }
    else
    {
        sprintf(file, "GMT+%d", hrs);
    }
    
    if (0 != mins)
    {
        sprintf(file, "%s:%d", file, mins);
    }
    
    snprintf (cmd, CMD_LEN, "%s %s/%s %s", LINK_CMD, ZONEINFO_DIR, file, LOCALTIME);
    
    TDBG ("Command : %s\n", cmd);
    return (safe_system (cmd));
}

/*---------------------------------------
 *  Updates the BMC TimeZone variable
 *---------------------------------------*/
int UpdateTimeZone (INT8U *TimeZone)
{
    struct tm *loctm = NULL;
    time_t Curtime = 0;

    time(&Curtime);
    loctm = localtime(&Curtime);

    //Update the time zone variable as per the offset value
    //offset 0 means GMT time
    //offset < 0, GMT- time, GMT- time zone, West of GMT
    //offset > 0, GMT+ time, GMT+ time zone, East of GMT
    if (loctm->tm_gmtoff == 0) 
    { 
        snprintf ((char*)TimeZone, TIME_ZONE_LEN, "%s/%s", MANUAL_OFFSETS, loctm->tm_zone); 
    } 
    else  if (loctm->tm_gmtoff < 0) 
    { 
        snprintf ((char*)TimeZone, TIME_ZONE_LEN, "%s%s", TIMEZONE_GMT_NVE, (loctm->tm_zone+4)); 
    } 
    else 
    { 
        snprintf ((char*)TimeZone, TIME_ZONE_LEN, "%s%s", TIMEZONE_GMT_PVE, (loctm->tm_zone+4)); 
    }

    return 0;
}


/*---------------------------------------
 * SetSELTime
 *---------------------------------------*/
int
SetSELTime (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    INT32U localTime = 0;
    _NEAR_ SetSELTimeReq_T* pSetSelTimeReq = (_NEAR_ SetSELTimeReq_T*) pReq;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    localTime = ipmitoh_u32 (pSetSelTimeReq->Time);

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    if(g_corefeatures.sel_clock_sync == ENABLED)
        SELTimeClockSync(PRE_CLK_SET, BMCInst);


    SET_SYSTEM_TIME_STAMP (&localTime);

    if(g_corefeatures.sel_clock_sync == ENABLED)
       SELTimeClockSync(POST_CLK_SET, BMCInst);


    /* Resetting the SELTimeUTCOffset to default value */
    if (0 != setUTC_Offset (0))
    {
        pRes [0] = CC_INV_DATA_FIELD;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (*pRes);
    }

    if( 0!= RestartDaemonByForce("crond","cron","/var/run/crond.reboot",SIGKILL))
    {
        pRes [0] = CC_UNSPECIFIED_ERR;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (*pRes);
    }

    if(g_corefeatures.time_zone_support == ENABLED)
    {
       UpdateTimeZone (pBMCInfo->GenConfig.TimeZone);
    }
    pBMCInfo->GenConfig.SELTimeUTCOffset = UNSPECIFIED_UTC_OFFSET;

    /*Write to NVRAM*/
    FlushIPMI((INT8U*)&pBMCInfo->GenConfig,(INT8U*)&pBMCInfo->GenConfig,pBMCInfo->IPMIConfLoc.GenConfigAddr,
                      sizeof(GENConfig_T),BMCInst);

    pRes [0] = CC_NORMAL;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
    return sizeof (*pRes);
}


/*---------------------------------------
 * GetSELTimeUTC_Offset
 *---------------------------------------*/
int
GetSELTimeUTC_Offset (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetSELTimeUTCOffsetRes_T* pGetSelTimeUTCOffsetRes = (_NEAR_ GetSELTimeUTCOffsetRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    pGetSelTimeUTCOffsetRes->CompletionCode = CC_NORMAL;
    pGetSelTimeUTCOffsetRes->UTCOffset = htoipmi_u16(pBMCInfo->GenConfig.SELTimeUTCOffset);
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    return sizeof (GetSELTimeUTCOffsetRes_T);
}



/*---------------------------------------
 * SetSELTimeUTC_Offset
 *---------------------------------------*/
int
SetSELTimeUTC_Offset (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ SetSELTimeUTCOffsetReq_T* pSetSELTimeUTCOffsetReq = (_NEAR_ SetSELTimeUTCOffsetReq_T*) pReq;
    _NEAR_ SetSELTimeUTCOffsetRes_T* pSetSELTimeUTCOffsetRes = (_NEAR_ SetSELTimeUTCOffsetRes_T*) pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    INT16 UTCOffset = 0;
    INT32U UTCTime = 0;
    INT32U localtime = 0;
    GetSELTimeRes_T GetSELTimeRes;

    memset( &GetSELTimeRes, 0, sizeof(GetSELTimeRes_T));

    UTCOffset = (INT16) ipmitoh_u16(pSetSELTimeUTCOffsetReq->UTCOffset);

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    //get the local time from GetSELTime command
    GetSELTime ( NULL, 0, (INT8U*)(&GetSELTimeRes), BMCInst);
    //check the response to find out it's success or failure
    if( CC_NORMAL != GetSELTimeRes.CompletionCode)
    {
        pSetSELTimeUTCOffsetRes->CompletionCode = GetSELTimeRes.CompletionCode;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (*pRes);
    }
    //Retrieve the local time in seconds
    localtime = GetSELTimeRes.Time;

    //calculate the UTC time
    UTCTime = localtime - ( UTCOffset * 60)/*UTC offset is in minute, so converting it in second*/;

    //Update the UTC time as system time
    SET_SYSTEM_TIME_STAMP(&UTCTime);

     if( 0!= RestartDaemonByForce("crond","cron","/var/run/crond.reboot",SIGKILL))
    {
        pRes [0] = CC_UNSPECIFIED_ERR;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
        return sizeof (*pRes);
    }

    //update the UTC offset
    //After UTC offset set successfully
    //we can calculate local time from UTC time(system time) and offset 
    if (((UTCOffset <= SEL_UTC_MAX_RANGE) && (UTCOffset >= SEL_UTC_MIN_RANGE)) || (UTCOffset == UNSPECIFIED_UTC_OFFSET))
    {
        if (0 != setUTC_Offset ((UTCOffset == UNSPECIFIED_UTC_OFFSET) ? 0 : UTCOffset))
        {
            pSetSELTimeUTCOffsetRes->CompletionCode = CC_INV_DATA_FIELD;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
            return sizeof (*pRes);
        }

        if(g_corefeatures.time_zone_support == ENABLED)
        {
            UpdateTimeZone (pBMCInfo->GenConfig.TimeZone);
        }

        pBMCInfo->GenConfig.SELTimeUTCOffset = UTCOffset;
        /*Write to NVRAM*/
        FlushIPMI((INT8U*)&pBMCInfo->GenConfig,(INT8U*)&pBMCInfo->GenConfig,pBMCInfo->IPMIConfLoc.GenConfigAddr,
                          sizeof(GENConfig_T),BMCInst);

        pSetSELTimeUTCOffsetRes->CompletionCode = CC_NORMAL;
    }
    else
    {
        pSetSELTimeUTCOffsetRes->CompletionCode = CC_PARAM_OUT_OF_RANGE;
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
    return sizeof (*pRes);
}


/*---------------------------------------
 * GetAuxiliaryLogStatus
 *---------------------------------------*/
int
GetAuxiliaryLogStatus(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    return 0;
}


/*---------------------------------------
 * SetAuxiliaryLogStatus
 *---------------------------------------*/
int
SetAuxiliaryLogStatus(_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    return 0;
}


/*---------------------------------------
 * GetNextSELEntry
 *---------------------------------------*/
static _FAR_ SELRec_T*
GetNextSELEntry (_FAR_ SELRec_T* rec, int BMCInst)
{
    SELEventNode *SELNode = NULL;
    //INT16U LastRecID = 0;

    //LOCK_BMC_SHARED_MEM(BMCInst);
    //LastRecID = 	BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID;
    //UNLOCK_BMC_SHARED_MEM(BMCInst) ;

    /* As SEL Record ID is incremental upto 0xFFFE,can't validate the Current Rec ID
        with Last Rec ID, because rec ID will be greater than last RecID if the Circular SEL is used. */
/*    if (!CircularSEL && rec->EvtRecord.hdr.ID >= LastRecID)
    {
        return 0;
    }
    else
    {*/
    if(g_corefeatures.del_sel_reclaim_support != ENABLED)
    {
        rec++;
        while (1)
        {
            if (rec->Valid == VALID_RECORD)
            {
                return rec;
            }
            else if (rec->Valid == STATUS_DELETE_SEL)
            {
                rec++;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        SELNode = SEL_RECORD_ADDR(BMCInst,rec->EvtRecord.hdr.ID).RecAddr;
        if((SELNode != NULL) && (SELNode->NextRec != NULL))
        {
            return (&(SELNode->NextRec)->SELRecord);
        }
        else
        {
            return NULL;
        }
    }

     return 0;

}

/*
 @ fn GetSELRec
 @ brief This function is used to get the SEL Record based Record ID
 @ params RecID[in] Record ID BMCInst[in]
 @ returns SELRecord pointer on success and 0 on failures
 */
static _FAR_ SELRec_T* GetSELRec(INT16U RecID,int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    int i=0;
    _FAR_ SELRepository_T* _FAR_ m_sel;
    struct SELEventNode *SELNode= NULL;

   if(g_corefeatures.del_sel_reclaim_support != ENABLED)
   {
    m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);

    for(i=0;i<pBMCInfo->SELConfig.MaxSELRecord;i++)
    {
        if((m_sel->SELRecord[i].EvtRecord.hdr.ID == RecID) && (m_sel->SELRecord[i].Valid== VALID_RECORD))
        {
            return &m_sel->SELRecord[i];
        }
    }
   }
   else if (RecID <= pBMCInfo->SELConfig.MaxSELRecord)
   {
        SELNode =SEL_RECORD_ADDR(BMCInst, RecID).RecAddr;
        if(NULL != SELNode)
        {
            return &SELNode->SELRecord;
        }
   }
    return 0;
}

/*
 @ fn FindRecOrder
 @ brief This function is used to Update the First and Last SEL Record ID for Circular SEL
 @ params BMCInst[in] - BMC Instance
 */
static _FAR_ void FindRecOrder(int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_ SELRepository_T* _FAR_ m_sel;
    int i=0,index=0,FirstIndex = 0,j=0;
    INT16U  LastRecID = 0,FirstRecID = 0,TempRecID = 0;

    m_sel = (_FAR_ SELRepository_T*) GetSDRSELNVRAddr ((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);

    /*Find the largetest and lowest Rec ID to find out First and last Rec ID*/
    while(i < pBMCInfo->SELConfig.MaxSELRecord)
    {
        if(m_sel->SELRecord[i].Valid == VALID_RECORD)
        {
            LastRecID = m_sel->SELRecord[i].EvtRecord.hdr.ID;
            FirstRecID = m_sel->SELRecord[i].EvtRecord.hdr.ID;
            i++;
            break;
        }
        else
        {
            i++;
        }
    }

    for(;i<pBMCInfo->SELConfig.MaxSELRecord;i++)
    {
        if(m_sel->SELRecord[i].Valid != VALID_RECORD)
        {
            continue;
        }

        if(m_sel->SELRecord[i].EvtRecord.hdr.ID > LastRecID)
        {
            LastRecID = m_sel->SELRecord[i].EvtRecord.hdr.ID;
            index = i;
        }
        if(m_sel->SELRecord[i].EvtRecord.hdr.ID < FirstRecID)
        {
            FirstRecID = m_sel->SELRecord[i].EvtRecord.hdr.ID;
            FirstIndex = i;
        }
    }

    if(pBMCInfo->AMIConfig.CircularSELFlag == TRUE)
    {
        /*Find the direction of LastRecord ID*/
        if(index == (pBMCInfo->SELConfig.MaxSELRecord-1))
        {
            if((m_sel->SELRecord[index].EvtRecord.hdr.ID - m_sel->SELRecord[0].EvtRecord.hdr.ID) == 1)
            {
                i = 0;
            }
            else
            {
                i = index -1;
            }
        }
        if((m_sel->SELRecord[index].EvtRecord.hdr.ID - m_sel->SELRecord[index +1].EvtRecord.hdr.ID) == 1)
        {
            i = index +1;
        }
        else
        {
            if(index == 0)
                i = pBMCInfo->SELConfig.MaxSELRecord -1;
            else
                i = index -1;
        }

        /*Find the direction of FirstRecord ID*/
        if(FirstIndex == (pBMCInfo->SELConfig.MaxSELRecord-1))
        {
            if((m_sel->SELRecord[FirstIndex].EvtRecord.hdr.ID - m_sel->SELRecord[0].EvtRecord.hdr.ID) == -1)
            {
                j = 0;
            }
            else
            {
                j = FirstIndex -1;
            }
        }
        if((m_sel->SELRecord[FirstIndex].EvtRecord.hdr.ID - m_sel->SELRecord[FirstIndex +1].EvtRecord.hdr.ID) == -1)
        {
            j = FirstIndex +1;
        }
        else
        {
            if(FirstIndex == 0)
                j = pBMCInfo->SELConfig.MaxSELRecord -1;
            else
                j = FirstIndex -1;
        }

        TempRecID = LastRecID;
        /*Find the FirstRecord ID */
        if((i > index && index != 0) || i == 0)
        {
            while( i != index)
            {
                if(m_sel->SELRecord[i].EvtRecord.hdr.ID == (TempRecID-1) || m_sel->SELRecord[i].EvtRecord.hdr.ID == TempRecID)
                {
                    TempRecID = m_sel->SELRecord[i].EvtRecord.hdr.ID;
                    if(m_sel->SELRecord[i].Valid == VALID_RECORD)
                    {
                        /*Update the Record ID only if it is valid*/
                        LastRecID = m_sel->SELRecord[i].EvtRecord.hdr.ID;
                    }
                }
                else
                {
                    break;
                }
                if(i == pBMCInfo->SELConfig.MaxSELRecord -1)
                    i =0;
                else
                    i++;
            }
        }
        else
        {
            while(i != index)
            {
                if(m_sel->SELRecord[i].EvtRecord.hdr.ID + 1== TempRecID || m_sel->SELRecord[i].EvtRecord.hdr.ID == TempRecID)
                {
                    TempRecID = m_sel->SELRecord[i].EvtRecord.hdr.ID;
                    if(m_sel->SELRecord[i].Valid == VALID_RECORD)
                    {
                        /*Update the Record ID only if it is valid*/
                        LastRecID = m_sel->SELRecord[i].EvtRecord.hdr.ID;
                    }
                }
                else
                {
                    break;
                }
                if(i == 0)
                    i = pBMCInfo->SELConfig.MaxSELRecord -1;
                else
                    i--;
            }
        }

        index = FirstIndex;
        TempRecID = FirstRecID;
        /*Find the LastRecord ID */
        if((j > FirstIndex && FirstIndex != 0) || j == 0)
        {
            while( j != FirstIndex)
            {
                if(m_sel->SELRecord[j].EvtRecord.hdr.ID == (TempRecID+1) || m_sel->SELRecord[j].EvtRecord.hdr.ID == TempRecID)
                {
                    TempRecID = m_sel->SELRecord[j].EvtRecord.hdr.ID;
                    if(m_sel->SELRecord[j].Valid == VALID_RECORD)
                    {
                        /*Update the Record ID only if it is valid*/
                        FirstRecID = m_sel->SELRecord[j].EvtRecord.hdr.ID;
                    }
                    index = j;
                }
                else
                {
                    break;
                }
                if(j == pBMCInfo->SELConfig.MaxSELRecord -1)
                    j =0;
                else
                    j++;
            }
        }
        else
        {
            while(j != FirstIndex)
            {
                if(m_sel->SELRecord[j].EvtRecord.hdr.ID - 1== TempRecID || m_sel->SELRecord[j].EvtRecord.hdr.ID == TempRecID)
                {
                    TempRecID = m_sel->SELRecord[j].EvtRecord.hdr.ID;
                    if(m_sel->SELRecord[j].Valid == VALID_RECORD)
                    {
                        /*Update the Record ID only if it is valid*/
                        FirstRecID = m_sel->SELRecord[j].EvtRecord.hdr.ID;
                    }
                    index = j;
                }
                else
                {
                    break;
                }
                if(j == 0)
                    j = pBMCInfo->SELConfig.MaxSELRecord -1;
                else
                    j--;
            }
        }

    }

    if(pBMCInfo->AMIConfig.CircularSELFlag == TRUE)
    {
        LOCK_BMC_SHARED_MEM(BMCInst);
        BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID = LastRecID;
        BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID = FirstRecID;
        BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex = index +1;
        UNLOCK_BMC_SHARED_MEM (BMCInst);
    }
    else
    {
        LOCK_BMC_SHARED_MEM(BMCInst);
        BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID = FirstRecID;
        BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID = LastRecID;
        BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex = index +1;
        UNLOCK_BMC_SHARED_MEM (BMCInst);
    }
}

/*---------------------------------------
 * GetSelTimeStamp
 *---------------------------------------*/
INT32U
GetSelTimeStamp(int BMCInst)
{
    INT16 UTCOffset;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    
    UTCOffset = (INT16) ipmitoh_u16(pBMCInfo->GenConfig.SELTimeUTCOffset);
    
    if (UNSPECIFIED_UTC_OFFSET == UTCOffset)
    {
        UTCOffset = 0;
    }
    return ((htoipmi_u32 (GET_SYSTEM_TIME_STAMP())) + (GetUTC_Offset() * 60));
}

#endif  /* SEL_DEVICE */

/*---------------------------------------------------------------------------
 * @fn SELTimeClockSync
 *
 * @brief This function is invoked when set SEL time, generate a pair of
 *        events(pre and post clock setting) correlating the timestamps
 *        for events occurring before and after the new clock value.
 *
 * @param   Action  - Specify pre-event or post-event.
 * @param   BMCInst - Index of BMC instance.
 *
 * @return  0	- if success.
 *			-1	- if error.
 *---------------------------------------------------------------------------*/
static INT8U SELTimeClockSync(INT8U Action, int BMCInst)
{
    SELEventRecord_T *EventRec = (SELEventRecord_T*)m_SysEventMsg;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U pRes[sizeof(AddSELRes_T)];

    EventRec->SensorNum  = BMC_GET_SHARED_MEM(BMCInst)->SysEvent_SensorNo;
    EventRec->EvtDirType = 0x6F;    // Sensor Specific
    EventRec->EvtData1   = 0x05;    // Offset 05h - Timestamp Clock Sync
    EventRec->EvtData2   = Action;
    EventRec->EvtData3   = 0xFF;

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);

    LockedAddSELEntry((INT8U*)EventRec, sizeof(SELEventRecord_T), pRes, FALSE,POST_SEL_AND_PEF, BMCInst);

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);

    return 0;
}


/*---------------------------------------------------------------------------
 * @fn SetSELPolicy
 *
 * @brief This function is invoked when switch SEL policy, make appropriate
 *        adjustment for environment variables that related to SEL buffer.
 *
 * @param   Policy  - Specify Linear SEL policy or Circular SEL policy.
 * @param   BMCInst - Index of BMC instance.
 *---------------------------------------------------------------------------*/
void
SetSELPolicy(INT8U Policy, int BMCInst)
{
    SELRepository_T* m_sel = NULL;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    if(!g_corefeatures.del_sel_reclaim_support)
    {
        m_sel = (SELRepository_T*)GetSDRSELNVRAddr((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), BMCInst);
    }

    switch (Policy)
    {
        case LINEAR_SEL:
            /* Reset LastRecID because 
            func will stop finding the next record if RecID >= LastRecRD,
               but RecID may greater than LastRecID in Circular SEL mode. */
/*            LOCK_BMC_SHARED_MEM(BMCInst);
            BMC_GET_SHARED_MEM (BMCInst)->m_LastRecID = m_sel->SELRecord[pBMCInfo->SELConfig.MaxSELRecord].EvtRecord.hdr.ID;
            BMC_GET_SHARED_MEM (BMCInst)->m_SELIndex = m_sel->NumRecords;
            BMC_GET_SHARED_MEM (BMCInst)->m_FirstRecID = m_sel->SELRecord[0].EvtRecord.hdr.ID;
            UNLOCK_BMC_SHARED_MEM(BMCInst);*/
            break;
 
        case CIRCULAR_SEL:
            /*Update the Last and First Record ID when switch back to Circular-linear-Circular cycle */
            if((!g_corefeatures.del_sel_reclaim_support) && (m_sel->NumRecords == pBMCInfo->SELConfig.MaxSELRecord))
            {
                FindRecOrder( BMCInst);
            }
            break;
    }

    pBMCInfo->AMIConfig.CircularSEL = Policy;
    FlushIPMI((INT8U*)&pBMCInfo->AMIConfig, (INT8U*)&pBMCInfo->AMIConfig.CircularSEL, pBMCInfo->IPMIConfLoc.AMIConfigAddr, sizeof(INT8U), BMCInst);
}

/**
*@fn GetUTC_Offset
*@brief returns UTC offset from local time
*/

int GetUTC_Offset()
{
    int UTCOffset;

    struct tm *loctm = NULL;
    time_t Curtime = 0;

    time(&Curtime);
    loctm = localtime(&Curtime);

    UTCOffset = loctm->tm_gmtoff / 60;
    return UTCOffset;
}

