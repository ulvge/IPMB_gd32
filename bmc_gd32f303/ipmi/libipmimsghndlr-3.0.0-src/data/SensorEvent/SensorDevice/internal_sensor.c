/*****************************************************************
 *****************************************************************


 *****************************************************************
 ******************************************************************
 *
 * intenal_sensor.c
 * Internal Sensor functions.
 * 
 *  Any internal Sensor implementation requires following
 * 
 *  SensorTypeCode		  - As per IPMI specificaiton. This
 * 							sensor will be registered only 
 *  				        if it is found in SDRs
 * 
 *  InitHook              - This hook will be used to initialize
 *                          the sensor for its default state.
 * 
 *  PreMonitorSensorHook  - this hook should provide the 
 *                          sensor reading for the core sensor
 *                          and should return -1 to stop
 *                          actual sensor reading in SensorMonitor.c
 * 
 *  PostMonitorSensorHook - this hook should provide the 
 * 							event generation capability for this
 *                          sensor and return -1 to stop 
 *                          generic event generation.
 * 
 *  SetSensorReadingHook  - This function will used wherever
 *                          applicable in the core to set a new state 
 *                          for this sensor.
 * 
 * 
 *
 *  Author: 
 ******************************************************************/
#define  ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "Debug.h"
#include "MsgHndlr.h"
#include "Sensor.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "IPMI_Sensor.h"
#include "SensorMonitor.h"
#include "SDRFunc.h"
#include "Util.h"
#include "IPMI_Main.h"
#include "SharedMem.h"
#include "SEL.h"
#include "WDT.h"
#include "IPMIConf.h"
#include "SSIAccess.h"
#include "featuredef.h"
#include <dlfcn.h>


/*** Local Definitions ***/

#define EVENT_MSG_LENGTH							0x09

#define SERV_INDEX_BASE     5

/*** Local typedefs ***/
typedef enum
{
    /* This enumuration should match the table entries */

    INTERNAL_SENSOR_SEL = 0,
    INTERNAL_SENSOR_WD2,
    INTERNAL_SENSOR_OPSTATE,
    INTERNAL_SENSOR_AGGREGATETHERMAL,
    INTERNAL_SENSOR_AGGREGATEFAULT,
    INTERNAL_SENSOR_SOLSTATE,
    INTERNAL_SENSOR_KVMSTATE,
    INTERNAL_SENSOR_CDMEDIASTATE,
    INTERNAL_SENSOR_FDMEDIASTATE,
    INTERNAL_SENSOR_HDMEDIASTATE,
    INTERNAL_SENSOR_RAIDROCTEMP1,
    INTERNAL_SENSOR_RAIDROCTEMP2,
    INTERNAL_SENSOR_RAIDEXPANDERTEMP1,
    INTERNAL_SENSOR_RAIDEXPANDERTEMP2,
    INTERNAL_SENSOR_RAIDBBUTEMP1,
    INTERNAL_SENSOR_RAIDBBUTEMP2
} InternalSensor_E;

/*** Local Variables ***/
 InternalSensorTbl_T m_internal_sensor_tbl [] = {

    {   SENSOR_TYPE_EVT_LOGGING,    /* Sensor Type code      */ 
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */ 
        0,                          /* Assertion tracking    */
        0,                          /* WDT use               */
        0,                          /* Assertion tracking    */
        0,                          /* WDT restarted         */
        SELSensorInit,              /* Init Sensor hook      */
        SELMonitor,                 /* Pre monitor hook      */
        SELEventLog,                /* Post monitor hook     */
        SELEventLogExt,             /* Post monitor Ext hook */
        0,                          /* Entity ID             */
        0,                          /* Entity Instance       */
        0,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_WATCHDOG2,      /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion tracking    */
        0,                          /* WDT use               */
        0,                          /* Assertion tracking    */	
        0,                          /* WDT restarted         */
        WD2SensorInit,              /* Init Sensor hook      */
        WD2Monitor,                 /* Pre monitor hook      */
        WD2DummyEventLog,           /* Dummy Post monitor hook*/
        WD2EventLogExt,             /* Post monitor Ext hook */
        0,                          /* Entity ID             */
        0,                          /* Entity Instance       */
        0,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_FRU_STATE,      /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        OpStateSensorInit,          /* Init Sensor hook      */
        OpStateMonitor,             /* Pre monitor hook      */
        OpStateEventLog,            /* Post monitor hook     */
        OpStateEventLogExt,         /* Post monitor Ext hook */
        0,                          /* Entity ID             */
        0,                           /* Entity Instance       */
        1,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_TEMP,           /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        AggregateThermalSensorInit, /* Init Sensor hook      */
        AggregateThermalMonitor,    /* Pre monitor hook      */
        AggregateThermalEventLog,   /* Post monitor hook     */
        AggregateThermalEventLogExt,/* Post monitor Ext hook */
        0,                          /* Entity ID             */
        0,                           /* Entity Instance       */
        1,
        0                           /* SensorOwnerLun       */
    },

    {   SENSOR_TYPE_MODULE_BOARD,   /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        AggregateFaultSensorInit,   /* Init Sensor hook      */
        AggregateFaultMonitor,      /* Pre monitor hook      */
        AggregateFaultEventLog,     /* Post monitor hook     */
        AggregateFaultEventLogExt,  /* Post monitor Ext hook */
        0,                          /* Entity ID             */
        0,                           /* Entity Instance       */
        1,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_SERV_STATE,     /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        ServiceStateSensorInit,     /* Init Sensor hook      */
        ServiceStateMonitor,        /* Pre monitor hook      */
        ServiceStateEventLog,       /* Post monitor hook     */
        ServiceStateEventLogExt,    /* Post monitor Ext hook */
        SERV_STATE_ENTITY_ID,       /* Entity ID             */
        SERV_INDEX_SOL,              /* Entity Instance       */
        1,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_SERV_STATE,     /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        ServiceStateSensorInit,     /* Init Sensor hook      */
        ServiceStateMonitor,        /* Pre monitor hook      */
        ServiceStateEventLog,       /* Post monitor hook     */
        ServiceStateEventLogExt,    /* Post monitor Ext hook */
        SERV_STATE_ENTITY_ID,       /* Entity ID             */
        SERV_INDEX_KVM,              /* Entity Instance       */
        1,
        0                           /* SensorOwnerLun        */
    },
 
    {   SENSOR_TYPE_SERV_STATE,     /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        ServiceStateSensorInit,     /* Init Sensor hook      */
        ServiceStateMonitor,        /* Pre monitor hook      */
        ServiceStateEventLog,       /* Post monitor hook     */
        ServiceStateEventLogExt,    /* Post monitor Ext hook */
        SERV_STATE_ENTITY_ID,       /* Entity ID             */
        SERV_INDEX_CDMEDIA,          /* Entity Instance       */
        1,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_SERV_STATE,     /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        ServiceStateSensorInit,     /* Init Sensor hook      */
        ServiceStateMonitor,        /* Pre monitor hook      */
        ServiceStateEventLog,       /* Post monitor hook     */
        ServiceStateEventLogExt,    /* Post monitor Ext hook */
        SERV_STATE_ENTITY_ID,       /* Entity ID             */
        SERV_INDEX_FDMEDIA,          /* Entity Instance       */
        1,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_SERV_STATE,     /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        ServiceStateSensorInit,     /* Init Sensor hook      */
        ServiceStateMonitor,        /* Pre monitor hook      */
        ServiceStateEventLog,       /* Post monitor hook     */
        ServiceStateEventLogExt,    /* Post monitor Ext hook */
        SERV_STATE_ENTITY_ID,       /* Entity ID             */
        SERV_INDEX_HDMEDIA,          /* Entity Instance       */
        1,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_TEMP,           /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        RAIDSensorInit,             /* Init Sensor hook      */
        RAIDMonitor,                /* Pre monitor hook      */
        RAIDEventLog,               /* Post monitor hook     */
        RAIDEventLogExt,            /* Post monitor Ext hook */
        RAID_ROC_ENTITY_ID,         /* Entity ID             */
        RAID_ROC_TEMP1,             /* Entity Instance       */
        2,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_TEMP,           /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        RAIDSensorInit,             /* Init Sensor hook      */
        RAIDMonitor,                /* Pre monitor hook      */
        RAIDEventLog,               /* Post monitor hook     */
        RAIDEventLogExt,            /* Post monitor Ext hook */
        RAID_ROC_ENTITY_ID,         /* Entity ID             */
        RAID_ROC_TEMP2,             /* Entity Instance       */
        2,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_TEMP,           /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        RAIDSensorInit,             /* Init Sensor hook      */
        RAIDMonitor,                /* Pre monitor hook      */
        RAIDEventLog,               /* Post monitor hook     */
        RAIDEventLogExt,            /* Post monitor Ext hook */
        RAID_EXPANDER_ENTITY_ID,    /* Entity ID             */
        RAID_EXPANDER_TEMP1,        /* Entity Instance       */
        2,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_TEMP,           /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        RAIDSensorInit,             /* Init Sensor hook      */
        RAIDMonitor,                /* Pre monitor hook      */
        RAIDEventLog,               /* Post monitor hook     */
        RAIDEventLogExt,            /* Post monitor Ext hook */
        RAID_EXPANDER_ENTITY_ID,    /* Entity ID             */
        RAID_EXPANDER_TEMP2,        /* Entity Instance       */
        2,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_TEMP,           /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        RAIDSensorInit,             /* Init Sensor hook      */
        RAIDMonitor,                /* Pre monitor hook      */
        RAIDEventLog,               /* Post monitor hook     */
        RAIDEventLogExt,            /* Post monitor Ext hook */
        RAID_BBU_ENTITY_ID,         /* Entity ID             */
        RAID_BBU_TEMP1,             /* Entity Instance       */
        2,
        0                           /* SensorOwnerLun        */
    },

    {   SENSOR_TYPE_TEMP,           /* Sensor Type code      */
        0,                          /* Sensor Reading        */
        0xFF,                       /* Sensor Number         */
        0,                          /* Assertion History     */
        0,                          /* WDT Tmr Use           */
        0,                          /* WDT Tmr Actions       */
        0,                          /* WDT restarted         */
        RAIDSensorInit,             /* Init Sensor hook      */
        RAIDMonitor,                /* Pre monitor hook      */
        RAIDEventLog,               /* Post monitor hook     */
        RAIDEventLogExt,            /* Post monitor Ext hook */
        RAID_BBU_ENTITY_ID,         /* Entity ID             */
        RAID_BBU_TEMP1,             /* Entity Instance       */
        2,
        0                           /* SensorOwnerLun        */
    },

};

#define LIBCOMP_LIB_PATH        "/usr/local/lib/libcomponent_manager.so"

// mutex for WDT sensor struction update
pthread_mutex_t   sg_WDTSensorMutex = PTHREAD_MUTEX_INITIALIZER;


/*---------------------------------------
 * InitInternalSensors
 *---------------------------------------*/
int
InitInternalSensors (SensorInfo_T *pSensorInfo,int BMCInst)
{
    int i = 0;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U SSIComputeBladeSupport = g_corefeatures.ssi_support;

    /* Register core internal sensor handler
    * if we have a valid SDR for that sensor
    *  */	
    for (i = 0; i < pBMCInfo->InternalSensorTblSize; i++)
    {
        /* Check for a match */
        if (pBMCInfo->InternalSensorTbl[i].SensorTypeCode == pSensorInfo->SensorTypeCode)
        {
           if(SSIComputeBladeSupport)
           {
                /* Register the aggregated thermal sensor only if sensor type is Temp(01h) and event type is OEM(7Fh) */
                if (pSensorInfo->SensorTypeCode == SENSOR_TYPE_TEMP && pSensorInfo->EventTypeCode != 0x7F)
                    continue;

                /* Register service state sensors */
                if (pSensorInfo->SensorTypeCode == SENSOR_TYPE_SERV_STATE)
                {
                    if (pSensorInfo->EntityID != pBMCInfo->InternalSensorTbl[i].EntityID || 
                        pSensorInfo->EntiryInstance != pBMCInfo->InternalSensorTbl[i].EntityInst)
                        continue;
                }
           }

            if (pSensorInfo->SensorTypeCode == SENSOR_TYPE_TEMP)
            {
                if (pSensorInfo->EntityID != pBMCInfo->InternalSensorTbl[i].EntityID )
                    continue;
            }

            //printf ("Found Internal Sensor %x    and Sno : %x\n", pSensorInfo->SensorTypeCode, pSensorInfo->SensorNumber);

            /* Update Sensor Number */
            pBMCInfo->InternalSensorTbl[i].SensorNum = pSensorInfo->SensorNumber;
            /* Multi-LUN Support Update Sensor Owner LUN */
            pBMCInfo->InternalSensorTbl[i].SensorOwnerLun = pSensorInfo->SensorOwnerLun;

            /* Register Init Hook for this sensor */
            if (pSensorInfo->InitSensorHookCnt < MAX_INIT_SENSOR_HOOKS)
            {
                /* Add an init hook */
                pSensorInfo->pInitSensor [pSensorInfo->InitSensorHookCnt] = pBMCInfo->InternalSensorTbl [i].pInitSensor;
                pSensorInfo->InitSensorHookCnt++;
            }

            /* Register PreMonitor Hook for this sensor */
            /* Add a pre monitor hook */
             pSensorInfo->pPreMonitor = pBMCInfo->InternalSensorTbl [i].pPreMonitor;

            /* Register PostMonitor Hook for this sensor */
            /* Add a post monitor hook */
              pSensorInfo->pPostMonitor = pBMCInfo->InternalSensorTbl [i].pPostMonitor;

            /* Register PostMonitorExt Hook for this sensor */
            if (pSensorInfo->PostMonitorExtHookCnt < MAX_POST_MONITOR_EXT_HOOKS)
            {
                /* Add a post monitor ext hook */
                pSensorInfo->pPostMontiorExt[pSensorInfo->PostMonitorExtHookCnt] = pBMCInfo->InternalSensorTbl [i].pPostMonitorExt;
                pSensorInfo->PostMonitorExtHookCnt++;
            }
        }
    }

    return 0;
}

/*---------------------------------------
 * GetSensorNumFromSensorType
 *---------------------------------------*/
void* GetSensorInfoFromSensorType (INT8U SensorType, int BMCInst)
{
    int 		  i;
    INT8U   	  u8SensorNumber = 0xFF;
    SensorInfo_T* pSensorInfo = NULL;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    for (i = 0; i< pBMCInfo->InternalSensorTblSize ; i++)
    {
        /* Check if we have a match for this sensor type */
        if (pBMCInfo->InternalSensorTbl[i].SensorTypeCode == SensorType)
        {
            u8SensorNumber = pBMCInfo->InternalSensorTbl[i].SensorNum;		
        }
    }

    if (u8SensorNumber != 0xFF)
    {
        /* Get Sensor information */
        /* Assuming we don't ever receive NULL pointer for this sensor number */
        /* If this function is called there has to be a sensor information */
        pSensorInfo = GetSensorInfo(u8SensorNumber, pBMCInfo->InternalSensorTbl[i].SensorOwnerLun, BMCInst);
    }

    return (void*)pSensorInfo;	
}


/***********************************************************
 *               SEL Sensor Implementation
 ***************************************************************/



/*-----------------------------------------
 * SELSensorInit
 *-----------------------------------------*/
int 
SELSensorInit (void*  pSenInfo,int BMCInst)
{
    SensorInfo_T*  pSensorInfo = pSenInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    SELInfo_T	SelInfo;
    INT16U      RecordCount = 0;

    /* Get SEL Information */
    GetSELInfo (NULL, 0, (INT8U*)&SelInfo,BMCInst);

    pSensorInfo->PreviousState = 0;
    pSensorInfo->SensorReading = 0;

    RecordCount = (INT16U)((g_BMCInfo[BMCInst].SELConfig.MaxSELRecord/4)* 3);  

    /* Initialize the SEL Sensor based on current state */
    if (SelInfo.RecCt == 0)
    {
        /* The SEL has been cleared  */
        pSensorInfo->PreviousState = 0;
        pSensorInfo->SensorReading = 0;
    }
    else if (SelInfo.RecCt == g_BMCInfo[BMCInst].SELConfig.MaxSELRecord)
    {
        /* SEL is full*/
        pSensorInfo->PreviousState = 0;
        pSensorInfo->SensorReading = EVENT_SEL_IS_FULL;
    }    
    else if (SelInfo.OpSupport & OVERFLOW_FLAG)
    {
        /* SEL is full*/
    }
    else if((SelInfo.RecCt >=  RecordCount) && (SelInfo.RecCt <g_BMCInfo[BMCInst].SELConfig.MaxSELRecord ))
    {
        /* SEL is almost full */
        pSensorInfo->PreviousState = 0;
        pSensorInfo->SensorReading = EVENT_SEL_ALMOST_FULL;
    }

    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_SEL].SensorReading = pSensorInfo->SensorReading;

    
    return 0;

}

/*-----------------------------------------
 * SELMonitor 
 *-----------------------------------------*/
int 
SELMonitor (void *pSenInfo, INT8U* pReadFlags,int BMCInst)
{
	SensorInfo_T*  pSensorInfo = pSenInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    /*Fill Current Sensor reading  */
    pSensorInfo->SensorReading = pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_SEL].SensorReading;


    /* Sensor Reading is calculated, but still PostMonitorExt for futher process */
    *pReadFlags = 0;

    return -1;

}
/*-----------------------------------------
 * GetSELFullPercentage
 *-----------------------------------------*/
INT8U 
GetSELFullPercentage(int BMCInst)
{
    SELInfo_T	SelInfo;
    INT32U		Percentage = 0;

    /* Get SEL Information */
    GetSELInfo (NULL, 0, (INT8U*)&SelInfo,BMCInst);

    Percentage = SelInfo.RecCt * 100;
    Percentage /= g_BMCInfo[BMCInst].SELConfig.MaxSELRecord;

    /*return Percentage of SEL FULL */
    return (INT8U)Percentage;
}

/*-----------------------------------------
 * SELEventLog
 *-----------------------------------------*/
int 
SELEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    INT8U			EventMsg [EVENT_MSG_LENGTH];
    SensorInfo_T*   pSensorInfo,*pReqSensorInfo = pSenInfo;	


    /* Get Sensor information */
    /* Assuming we don't ever receive NULL pointer for this sensor number */
    /* If this function is called there has to be a sensor information */
    pSensorInfo = GetSensorInfo (pReqSensorInfo->SensorNumber, pReqSensorInfo->SensorOwnerLun, BMCInst);

    /* Generate Events for SEL Sensor if there is a state change */
    if (0 == (pReqSensorInfo->SensorReading ^ pSensorInfo->PreviousState))
    {
        /* No state !! No Events !!1, but still PostMonitorExt for futher process */
        pSensorInfo->SensorReading = (pReqSensorInfo->SensorReading == 0)? pReqSensorInfo->SensorReading : (1 << pReqSensorInfo->SensorReading);    	
        *pReadFlags = 0;
        return -1;
    }

    // make sure the assertion event log is enabled
    if ((pSensorInfo->AssertionEventEnablesByte1 & (1 << pSensorInfo->SensorReading)) == 0)
    {
        // event is not supported but still need PostMonitorExt to do futher process
        pSensorInfo->SensorReading = (pReqSensorInfo->SensorReading == 0)? pReqSensorInfo->SensorReading : (1 << pReqSensorInfo->SensorReading);
        *pReadFlags = 0;
        return -1;
    }

    /* construct an Event Message for this sensor */
    EventMsg [0] = 0x20;
    EventMsg [1] = pSensorInfo->SensorOwnerLun & VALID_LUN;
    /* EvMRev */
    EventMsg [2] = IPMI_EVM_REVISION;
    /* sensor type */
    EventMsg [3] = SENSOR_TYPE_EVT_LOGGING;
    /* sensor number */
    EventMsg [4] = pReqSensorInfo->SensorNumber;

    /* event direction|type */
    EventMsg [5] = pSensorInfo->EventTypeCode;
    EventMsg [6] = pReqSensorInfo->SensorReading;//(INT8U)*pSensorReading;	
    
    switch (pSensorInfo->SensorReading)
    {
        case EVENT_LOG_AREA_RESET:
        case EVENT_SEL_IS_FULL:		/* Intentional fall thru !!!! */
            /* Event Data 2 and 3 are unspecified */    	
            EventMsg [7] = 0xFF;		
            EventMsg [8] = 0xFF;
            break;

        case EVENT_SEL_ALMOST_FULL:
            /* Event Data 2 and 3 are unspecified */    	
            EventMsg [7] = 0xFF;		
            EventMsg [8] = GetSELFullPercentage(BMCInst);
            break;

    }
    
    /* Set Previous state to new state */
    //pSensorInfo->PreviousState = pSensorInfo->SensorReading = pReqSensorInfo->SensorReading;//*pSensorReading;
    pSensorInfo->PreviousState = pReqSensorInfo->SensorReading;//*pSensorReading;
    
    /* We can't delay in logging clear and Full Event so this will be
    * taken care in SEL.c itself */
    if (pReqSensorInfo->SensorReading == EVENT_SEL_ALMOST_FULL)
    {
        /* Post Event Message */
        PostEventMessage (&EventMsg[0],FALSE, sizeof (EventMsg),BMCInst);
    }
    pSensorInfo->SensorReading = (pReqSensorInfo->SensorReading == 0)? pReqSensorInfo->SensorReading : (1 << pReqSensorInfo->SensorReading);

    /* Sensor Event is generated but still need PostMonitorExt to do futher process */
    *pReadFlags = 0;
    return -1;

}

/*---------------------------------------------------------------------
 *
 * SELEventLogExt
 *
 * This function is called by the PostMonitorExt hook function with the
 * local sensor data struction passed from the Sensor Monioring Task. 
 * Any changes in the sensor data structure will be cpoied into the master
 * sensor storage.
 *
 *--------------------------------------------------------------------*/
int SELEventLogExt (void* pSenInfo, INT8U* pReadFlags,int BMCInst)
{
    SensorInfo_T* 	pSensorInfo = pSenInfo;
    INT8U           u8EventOffset;

    /* clear the "Unable to read" bit in bit 5 */
    pSensorInfo->EventFlags &= 0xDF;

    // it is an event only sensor, the sensor event should be automatically deasserted.
    // clear the assertion occured bits, and setup the de-assertion occured bits only 
    // when the change is occured.
    if (pSensorInfo->SensorReading ^ pSensorInfo->PreviousState)
    {
        // clear all Event Log event occured flags due to the infomation only event
        pSensorInfo->AssertionEventOccuredByte1 = 0x0;
        pSensorInfo->AssertionEventOccuredByte2 = 0x0;

        // clear the deassertion occured bit first
        u8EventOffset = pSensorInfo->SensorReading;

        if (u8EventOffset < 8)
        {
            pSensorInfo->DeassertionEventOccuredByte1 &= ~(INT8U)(1 << u8EventOffset);

            // and set the deassertion bit if deassertion mask bit is set
            if (pSensorInfo->DeassertionEventEnablesByte1 & (1 << u8EventOffset))
            {
                pSensorInfo->DeassertionEventOccuredByte1 |= (INT8U)(1 << u8EventOffset);
            }
        }
        else
        {
            u8EventOffset -= 8;

            pSensorInfo->DeassertionEventOccuredByte2 &= ~(INT8U)(1 << u8EventOffset);

            // and set the deassertion bit if deassertion mask bit is set
            if (pSensorInfo->DeassertionEventEnablesByte2 & (1 << u8EventOffset))
            {
                pSensorInfo->DeassertionEventOccuredByte2 |= (INT8U)(1 << u8EventOffset);
            }
        }

        // clear the senser reading to deassert the event
        SetSELSensorReading(0,BMCInst);
        pSensorInfo->SensorReading = 0;
    }

    // Set Previous state to new state
    pSensorInfo->PreviousState = pSensorInfo->SensorReading;

    /* Sensor Event is generated so don't proceed regular Event generation */
    *pReadFlags = 1;
    return 0;
}

/*-----------------------------------------
 * SetSELSensorReading
 *-----------------------------------------*/
int 
SetSELSensorReading (INT16U Reading,int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    /* Set New sensor reading */
    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_SEL].SensorReading =  Reading;
    return 0;
}

/***********************************************************
 *               Watchdog2 Sensor Implementation
 ***************************************************************/

/*-----------------------------------------
 * WD2SensorInit
 *-----------------------------------------*/
int 
WD2SensorInit (void*  pSenInfo,int BMCInst)
{
    SensorInfo_T*  pSensorInfo = pSenInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    /* Get SEL Information */

    pSensorInfo->PreviousState = 0;
    pSensorInfo->SensorReading = 0;

    // don't wipe out the event assertion history, it could be some pending events
    // before the initAgent call due to the system reset
    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].SensorReading = pSensorInfo->SensorReading;

    //printf ("WD2 Sensor Init done\n");
    //printf ("Previous state = %x\n", pSensorInfo->PreviousState);
    //printf ("Reading        = %x\n", m_internal_sensor_tbl [INTERNAL_SENSOR_WD2].SensorReading);    
    
    return 0;

}

/*-----------------------------------------
 * WD2Monitor 
 *-----------------------------------------*/
int 
WD2Monitor (void* pSenInfo, INT8U* pReadFlags,int BMCInst)
{
	 SensorInfo_T*  pSensorInfo = pSenInfo;
     _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    /*Fill Current Sensor reading  */
    pSensorInfo->SensorReading = pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].SensorReading;

    /* Sensor Reading is calculated don't proceed regular monitoring */
    *pReadFlags = 1;		
    return -1;

}

/*------------------------------------------------------------------
 * WD2PostEventLog - Dummy post monitor hook to disable normal
 *    		     monitoring
 * ----------------------------------------------------------------*/
int
WD2DummyEventLog (void* pSenInfo, INT8U* pReadFlags,int BMCInst)
{
    return -1;
}

/*---------------------------------------------------------------------
 * WD2EventLog
 *--------------------------------------------------------------------*/
int 
WD2EventLog (void* pSenInfo, INT8U* pReadFlags,int BMCInst)
{
    SensorInfo_T*  		pSensorInfo = pSenInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U				EventMsg [EVENT_MSG_LENGTH];
    INT8U				u8EventOccured;
    INT16U				u16EventHistory;
    INT8U				u8EventEnabled;
    INT8U				u8TmrUse;
    INT8U				u8TmrActions;
    int                 i;
    // mutex the WDT sensor event flags
    pthread_mutex_lock(&sg_WDTSensorMutex);

    // do nothing if there is no pending event to be generated
    if (pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u16AssertionHistory)
    {
        /* construct an Event Message for this sensor */
        EventMsg [0] = 0x20;
        EventMsg [1] = 0x00;
        /* EvMRev */
        EventMsg [2] = IPMI_EVM_REVISION;
        /* sensor type */
        EventMsg [3] = SENSOR_TYPE_WATCHDOG2;
        /* sensor number */
        EventMsg [4] = pSensorInfo->SensorNumber;

        /* event direction|type */
        EventMsg [5] = pSensorInfo->EventTypeCode;

        /* Event Data 3 are unspecified */    	
        EventMsg [8] = 0xFF;

        //-------------------------------------------------------------------
        //
        // handle the WDT pre-timeout interrupt first
        //
        //-------------------------------------------------------------------
        u8TmrUse = pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8TmrUse;
        u8TmrActions = pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8TmrActions;
        u16EventHistory = pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u16AssertionHistory;
        u8EventOccured = pSensorInfo->AssertionEventOccuredByte2;
        u8EventEnabled = pSensorInfo->AssertionEventEnablesByte2;

        // do the pre-timeout interrupt SEL event first in offset 8, only if 
        // the pre-timeout occured but the event has not been generated
        if (((u16EventHistory >> EVENT_TIMER_INT) & 0x1) && (u8EventOccured & 0x1) == 0)
        {
            // make sure the WDT Log is enabled, and event logging is enabled
            if (0 == (u8TmrUse & 0x80) && (u8EventEnabled & 0x1))
            {
                // event offset plus the ED2 support
                EventMsg [6] = EVENT_TIMER_INT | 0xC0;	

                /* Event Data 2 provides more information on Timer int */    	
                EventMsg [7] = (u8TmrUse & 0x07) | (u8TmrActions & 0xF0);		

                /* Post Event Message */
                if (0 == (g_WDTTmrMgr.WDTTmr.TmrUse & 0x80))
                {
                    PostEventMessage (&EventMsg[0],FALSE, sizeof (EventMsg),BMCInst);
                }
            }

            // set the pre-timeout interrupt event occured flag in bit 0
            pSensorInfo->AssertionEventOccuredByte2 |= 0x1;

            // clear the pre-timeout interrupt event generated flag
            pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u16AssertionHistory &= 0xFEFF; 
        }

        //-------------------------------------------------------------------
        //
        // now handle the current WDT timeout events
        //
        //-------------------------------------------------------------------
        u16EventHistory &= 0xFF;
        u8EventOccured = pSensorInfo->AssertionEventOccuredByte1;
        u8EventEnabled = pSensorInfo->AssertionEventEnablesByte1;

        // then do all timeout SEL events, if have not been generated
        if (u16EventHistory)
        {
            // walk through all timeout events
            for (i = 0; i < 8; i++)
            {
                if ((u16EventHistory & (1 << i)) && (u8EventOccured & (1 << i)) == 0)
                {
                    // make sure the WDT Log is enabled, and event logging is enabled 
                    if (0 == (u8TmrUse & 0x80) && (u8EventEnabled & (1 << i)))
                    {
                        // event offset plus the ED2 support
                        EventMsg [6] = i | 0xC0;

                        /* Event Data 2 provides more information on Timer int */    	
                        EventMsg [7] = (u8TmrUse & 0x07) | (u8TmrActions & 0xF0);

                        /* Post Event Message */
                        if (0 == (u8TmrUse & 0x80))
                        {
                            PostEventMessage (&EventMsg[0],FALSE, sizeof (EventMsg),BMCInst);
                        }
                    }

                    // set the timeout event occured offset even if the SEL event
                    // is not allowed to be generated
                    pSensorInfo->AssertionEventOccuredByte1 |= (1 << i);
                }

                // clear the WDT timeout event generated flag
                pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u16AssertionHistory &= ~(1 << i);

            }

            // Clear the "Don't Log" bit after WDT timeout action or system reset.
            // Don't clear the "Don't Log" bit if the WDT SET / RESET came after 
            // the timeout was occurred but before the event was generated. That is,
            // we are processing the pending events here while a new WDT is set.
            if (pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8Restarted == 0)
            {
                if (0 != (g_WDTTmrMgr.WDTTmr.TmrUse & 0x80))
                {
                    g_WDTTmrMgr.WDTTmr.TmrUse &= 0x7F;
                }
            }
        }
    }
    
    if (pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8Restarted)
    {
    	pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8Restarted = 0;
        u8EventOccured = pSensorInfo->AssertionEventOccuredByte1 = 0;
        u8EventOccured = pSensorInfo->AssertionEventOccuredByte2 = 0;
    }
    pthread_mutex_unlock(&sg_WDTSensorMutex);

    /* Set Previous state to new state */
    pSensorInfo->PreviousState = pSensorInfo->SensorReading;

    /* clear the "Unable to read" bit in bit 5 */
    pSensorInfo->EventFlags &= 0xDF;

    /* Sensor Event is generated so don't proceed regular Event generation */
    *pReadFlags = 1;
    return -1;

}

/*---------------------------------------------------------------------
 *
 * WD2EventLogExt
 *
 * This function is called by the PostMonitorExt hook function with the
 * local sensor data struction passed from the Sensor Monioring Task. 
 * Any changes in the sensor data structure will be cpoied into the master
 * sensor storage.
 *
 *--------------------------------------------------------------------*/
int 
WD2EventLogExt (void* pSenInfo, INT8U* pReadFlags,int BMCInst)
{

	return 0;
}

/*---------------------------------------------------------------------
 *
 * SetWD2SensorReading
 *
 * This function stores the current WDT sensor reading, and keep 
 * tracking those un-logged assertions.
 *
 *--------------------------------------------------------------------*/
int 
SetWD2SensorReading (INT16U Reading, INT8U u8TmrUse, INT8U u8TmrActions,int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    // mutex the WDT sensor event flags
    pthread_mutex_lock(&sg_WDTSensorMutex);

    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u16AssertionHistory |= Reading;

    // this section is used to solve the issue when the WDT timer expired but event was
    // not generated yet while a new WDT was set via IPMI command. The implementation
    // here assumes we can only track one type of WDT timer event within one second.
    // That is, the buffer depth is set to one.
    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8TmrUse = u8TmrUse;
    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8TmrActions = u8TmrActions;
    /* Set New sensor reading */
    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].SensorReading = Reading;

    pthread_mutex_unlock(&sg_WDTSensorMutex);

    return 0;
}

/*---------------------------------------------------------------------
 *
 * RestartWD2Sensor
 *
 * This function is called when the WDT2 sensor structure needs to be
 * initialized, including the internal sensor assertion flag. Mostly, 
 * the SetWDT / ResetWDT command or the sensor init is executed.
 *
 *--------------------------------------------------------------------*/
int 
RestartWD2Sensor(int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    // mutex the WDT sensor event flags
    pthread_mutex_lock(&sg_WDTSensorMutex);

    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8Restarted = 1;

    // when we get here, all pending events are generated.
    // if the WDT is restarted, re-arm the WDT sensor by 
    // clear the event occurred in the sensorInfo, and reset
    // the WDT re-arm flag
    if (pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8Restarted)
    {
        pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].SensorReading = 0;

        pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8TmrUse = 0;
        pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_WD2].u8TmrActions = 0;
    }

    // don't wipe out the event assertion history, it could be some pending events
    // before the "auto re-arm" is called.
    //	m_internal_sensor_tbl[INTERNAL_SENSOR_WD2].u16AssertionHistory = 0;
    //	m_internal_sensor_tbl[INTERNAL_SENSOR_WD2].SensorReading = 0;

    pthread_mutex_unlock(&sg_WDTSensorMutex);

    return 0;
}


/*-----------------------------------------
 * OpStateSensorInit
 *-----------------------------------------*/
int 
OpStateSensorInit (void* pSenInfo, int BMCInst)
{
    SensorInfo_T* pSensorInfo = pSenInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    pSensorInfo->PreviousState = 0;
    pSensorInfo->SensorReading = 0;

    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_OPSTATE].SensorReading = pSensorInfo->SensorReading;

    return 0;
}

/*-----------------------------------------
 * OpStateMonitor 
 *-----------------------------------------*/
int 
OpStateMonitor (void *pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    SensorInfo_T* pSensorInfo = pSenInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    STATUS Status = ST_OK;
    INT8U CurrentState;

    /* Get current Operational State */
    if (g_SSIHandle[SSICB_GETCURRSTAT] != NULL)
    {
        Status = ((STATUS(*)(INT8U, INT8U*, int))g_SSIHandle[SSICB_GETCURRSTAT]) (DEFAULT_FRU_DEV_ID, &CurrentState, BMCInst);
    }

    if (Status != ST_OK)
    {
        TINFO("%s: SSICB_GetCurrentState returned Status = %d\n", __FUNCTION__, Status);
        CurrentState = 0; /* Stay at M0 */
    }

    /* Set sensor reading by current state */
    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_OPSTATE].SensorReading = (INT16U)((0x0001 << CurrentState) | 0x8000);
    pSensorInfo->SensorReading = pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_OPSTATE].SensorReading;

    /* Sensor Reading is calculated don't proceed regular monitoring */
    *pReadFlags = 1;
    return -1;
}

/*-----------------------------------------
 * OpStateEventLog
 *-----------------------------------------*/
int 
OpStateEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    SensorInfo_T* pSensorInfo = pSenInfo;
    OpStateFruObj_T *pFruObj;
    INT8U EventMsg[EVENT_MSG_LENGTH];

    /* Get Operational State FRU object */
    if (g_SSIHandle[SSICB_GETFRUOBJ] != NULL)
    {
        ((STATUS(*)(INT8U, OpStateFruObj_T **, int))g_SSIHandle[SSICB_GETFRUOBJ]) (DEFAULT_FRU_DEV_ID, &pFruObj, BMCInst);
    }

    /* Construct an Event Message for this sensor */
    EventMsg [0] = (pBMCInfo->IPMBAddr << 1);           /* Generator ID   */
    EventMsg [1] = 0x00;                                /* Generator ID   */
    EventMsg [2] = IPMI_EVM_REVISION;                   /* EvMRev         */
    EventMsg [3] = SENSOR_TYPE_FRU_STATE;               /* Sensor Type    */
    EventMsg [4] = pSensorInfo->SensorNumber;           /* Sensor Number  */
    EventMsg [5] = pSensorInfo->EventTypeCode;          /* Event Dir|Type */

    /* Event Data */
    EventMsg [6] = (0xE0 |(pFruObj->CurrentState & 0x0F));
    EventMsg [7] = ((pFruObj->ChangeCause << 4) | pFruObj->PreviousState);
    EventMsg [8] = pFruObj->FruId;

    /* Post event message if the state changes */
    if (pSensorInfo->PreviousState ^ pSensorInfo->SensorReading)
    {
        PostEventMessage (&EventMsg[0], FALSE,sizeof(EventMsg), BMCInst);
        pSensorInfo->PreviousState = pSensorInfo->SensorReading;
    }

    /* Sensor Event is generated so don't proceed regular Event generation */
    *pReadFlags = 1;
    return -1;
}

/*-----------------------------------------
 * OpStateEventLogExt
 *-----------------------------------------*/
int
OpStateEventLogExt (void* pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    return 0;
}

/*-----------------------------------------
 * AggregateThermalSensorInit
 *-----------------------------------------*/
int 
AggregateThermalSensorInit (void* pSenInfo, int BMCInst)
{
    SensorInfo_T* pSensorInfo = pSenInfo;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    pSensorInfo->PreviousState = 0;
    pSensorInfo->SensorReading = 0;

    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_AGGREGATETHERMAL].SensorReading = pSensorInfo->SensorReading;

    return 0;
}

/*-----------------------------------------
 * AggregateThermalMonitor 
 *-----------------------------------------*/
int 
AggregateThermalMonitor (void *pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    SensorInfo_T* pSensorInfo = pSenInfo;
    INT8U CurrentState;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    /* Get current Aggregated Thermal State */
    LOCK_BMC_SHARED_MEM(BMCInst);
    CurrentState = (INT8U)BMC_GET_SHARED_MEM(BMCInst)->AggregatedThermalState;
    UNLOCK_BMC_SHARED_MEM(BMCInst);

    /* Set sensor reading by current state */
    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_AGGREGATETHERMAL].SensorReading = (INT16U)((0x0001 << CurrentState) | 0x8000);
    pSensorInfo->SensorReading = pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_AGGREGATETHERMAL].SensorReading;

    /* Sensor Reading is calculated don't proceed regular monitoring */
    *pReadFlags = 1;
    return -1;
}

/*-----------------------------------------
 * AggregateThermalEventLog
 *-----------------------------------------*/
int 
AggregateThermalEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    SensorInfo_T* pSensorInfo = pSenInfo;
    INT8U EventMsg[EVENT_MSG_LENGTH];

    /* Construct an Event Message for this sensor */
    EventMsg [0] = (pBMCInfo->IPMBAddr << 1);           /* Generator ID   */
    EventMsg [1] = 0x00;                                /* Generator ID   */
    EventMsg [2] = IPMI_EVM_REVISION;                   /* EvMRev         */
    EventMsg [3] = SENSOR_TYPE_TEMP;                    /* Sensor Type    */
    EventMsg [4] = pSensorInfo->SensorNumber;           /* Sensor Number  */
    EventMsg [5] = pSensorInfo->EventTypeCode;          /* Event Dir|Type */

    /* Event Data */
    EventMsg [6] = (0x06 << 4) + (INT8U)pSensorInfo->SensorReading;
    EventMsg [7] = (0x0F << 4) + (INT8U)pSensorInfo->PreviousState;;
    if (g_PDKHandle[PDK_GETSLOTID] != NULL)
        EventMsg [8] = ((INT8U(*)(int))g_PDKHandle[PDK_GETSLOTID])(BMCInst);

    /* Post event message if the state changes */
    if (pSensorInfo->PreviousState ^ pSensorInfo->SensorReading)
    {
        PostEventMessage (&EventMsg[0], FALSE,sizeof(EventMsg), BMCInst);
        pSensorInfo->PreviousState = pSensorInfo->SensorReading;
    }

    /* Sensor Event is generated so don't proceed regular Event generation */
    *pReadFlags = 1;
    return -1;
}

/*-----------------------------------------
 * AggregateThermalEventLogExt
 *-----------------------------------------*/
int
AggregateThermalEventLogExt (void* pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    return 0;
}

/*-----------------------------------------
 * AggregateFaultSensorInit
 *-----------------------------------------*/
int 
AggregateFaultSensorInit (void* pSenInfo, int BMCInst)
{
    SensorInfo_T* pSensorInfo = pSenInfo;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    pSensorInfo->PreviousState = 0;
    pSensorInfo->SensorReading = 0;

    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_AGGREGATEFAULT].SensorReading = pSensorInfo->SensorReading;

    return 0;
}

/*-----------------------------------------
 * AggregateFaultMonitor 
 *-----------------------------------------*/
int 
AggregateFaultMonitor (void *pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    SensorInfo_T* pSensorInfo = pSenInfo;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    /* Set sensor reading by Aggregated Fault State */
    LOCK_BMC_SHARED_MEM(BMCInst);
    pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_AGGREGATEFAULT].SensorReading = BMC_GET_SHARED_MEM(BMCInst)->AggregatedFaultState;
    UNLOCK_BMC_SHARED_MEM(BMCInst);

    pSensorInfo->SensorReading = pBMCInfo->InternalSensorTbl[INTERNAL_SENSOR_AGGREGATEFAULT].SensorReading;

    /* Sensor Reading is calculated don't proceed regular monitoring */
    *pReadFlags = 1;
    return -1;
}

/*-----------------------------------------
 * AggregateFaultEventLog
 *-----------------------------------------*/
int 
AggregateFaultEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    SensorInfo_T* pSensorInfo = pSenInfo;
    INT8U EventMsg[EVENT_MSG_LENGTH];

    /* Construct an Event Message for this sensor */
    EventMsg[0] = (pBMCInfo->IPMBAddr << 1);            /* Generator ID   */
    EventMsg[1] = 0x00;                                 /* Generator ID   */
    EventMsg[2] = IPMI_EVM_REVISION;                    /* EvMRev         */
    EventMsg[3] = SENSOR_TYPE_MODULE_BOARD;             /* Sensor Type    */
    EventMsg[4] = pSensorInfo->SensorNumber;            /* Sensor Number  */
    EventMsg[5] = pSensorInfo->EventTypeCode;           /* Event Dir|Type */

    /* Event Data */
    EventMsg[6] = (INT8U)pSensorInfo->SensorReading;
    EventMsg[7] = 0xFF;
    EventMsg[8] = 0xFF;

    /* Post event message if the state changes */
    if (pSensorInfo->PreviousState ^ pSensorInfo->SensorReading)
    {
        PostEventMessage (&EventMsg[0],FALSE,sizeof(EventMsg), BMCInst);
        pSensorInfo->PreviousState = pSensorInfo->SensorReading;
    }

    /* Sensor Event is generated so don't proceed regular Event generation */
    *pReadFlags = 1;
    return -1;
}

/*-----------------------------------------
 * AggregateFaultEventLogExt
 *-----------------------------------------*/
int
AggregateFaultEventLogExt (void* pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    return 0;
}

/*-----------------------------------------
 * ServiceStateSensorInit
 *-----------------------------------------*/
int 
ServiceStateSensorInit (void* pSenInfo, int BMCInst)
{
    SensorInfo_T* pSensorInfo = pSenInfo;
    INT8U ServIndex = 0;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    /* Get Service Index according to Entity Instance */
    switch (pSensorInfo->EntiryInstance)
    {
        case SERV_INDEX_SOL:     ServIndex = INTERNAL_SENSOR_SOLSTATE;     break;
        case SERV_INDEX_KVM:     ServIndex = INTERNAL_SENSOR_KVMSTATE;     break;
        case SERV_INDEX_CDMEDIA: ServIndex = INTERNAL_SENSOR_CDMEDIASTATE; break;
        case SERV_INDEX_FDMEDIA: ServIndex = INTERNAL_SENSOR_FDMEDIASTATE; break;
        case SERV_INDEX_HDMEDIA: ServIndex = INTERNAL_SENSOR_HDMEDIASTATE; break;
    }

    pSensorInfo->PreviousState = 0;
    pSensorInfo->SensorReading = 0;

    pBMCInfo->InternalSensorTbl[ServIndex].SensorReading = pSensorInfo->SensorReading;

    return 0;
}

/*-----------------------------------------
 * ServiceStateMonitor 
 *-----------------------------------------*/
int 
ServiceStateMonitor (void *pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    SensorInfo_T* pSensorInfo = pSenInfo;
    INT8U ServIndex = 0;
    INT16U SensorReading = 0;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    /* Get Service Index & Service State according to Entity Instace */
    LOCK_BMC_SHARED_MEM(BMCInst);
    switch (pSensorInfo->EntiryInstance)
    {
        case SERV_INDEX_SOL:
            ServIndex = INTERNAL_SENSOR_SOLSTATE;
            SensorReading = BMC_GET_SHARED_MEM(BMCInst)->SOLState;
            break;

        case SERV_INDEX_KVM:
            ServIndex = INTERNAL_SENSOR_KVMSTATE;
            SensorReading = BMC_GET_SHARED_MEM(BMCInst)->KVMState;
            break;

        case SERV_INDEX_CDMEDIA:
            ServIndex = INTERNAL_SENSOR_CDMEDIASTATE;
            SensorReading = BMC_GET_SHARED_MEM(BMCInst)->CDMediaState;
            break;

        case SERV_INDEX_FDMEDIA:
            ServIndex = INTERNAL_SENSOR_FDMEDIASTATE;
            SensorReading = BMC_GET_SHARED_MEM(BMCInst)->FDMediaState;
            break;

        case SERV_INDEX_HDMEDIA:
            ServIndex = INTERNAL_SENSOR_HDMEDIASTATE;
            SensorReading = BMC_GET_SHARED_MEM(BMCInst)->HDMediaState;
            break;
    }
    UNLOCK_BMC_SHARED_MEM(BMCInst);

    /* Set sensor reading */
    pBMCInfo->InternalSensorTbl[ServIndex].SensorReading = SensorReading;
    pSensorInfo->SensorReading = SensorReading;

    /* Sensor Reading is calculated don't proceed regular monitoring */
    *pReadFlags = 1;
    return -1;
}

/*-----------------------------------------
 * ServiceStateEventLog
 *-----------------------------------------*/
int 
ServiceStateEventLog (void *pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    SensorInfo_T* pSensorInfo = pSenInfo;
    INT8U EventMsg[EVENT_MSG_LENGTH];

    /* Construct an Event Message for this sensor */
    EventMsg[0] = (pBMCInfo->IPMBAddr << 1);            /* Generator ID   */
    EventMsg[1] = 0x00;                                 /* Generator ID   */
    EventMsg[2] = IPMI_EVM_REVISION;                    /* EvMRev         */
    EventMsg[3] = SENSOR_TYPE_SERV_STATE;               /* Sensor Type    */
    EventMsg[4] = pSensorInfo->SensorNumber;            /* Sensor Number  */
    EventMsg[5] = pSensorInfo->EventTypeCode;           /* Event Dir|Type */

    /* Event Data */
    EventMsg[6] = (INT8U)pSensorInfo->SensorReading;
    EventMsg[7] = (INT8U)(pSensorInfo->SensorReading >> 8);
    EventMsg[8] = 0xFF;

    /* Post event message if the state changes */
    if (pSensorInfo->PreviousState ^ pSensorInfo->SensorReading)
    {
        PostEventMessage (&EventMsg[0],FALSE,sizeof(EventMsg), BMCInst);
        pSensorInfo->PreviousState = pSensorInfo->SensorReading;
    }

    /* Sensor Event is generated so don't proceed regular Event generation */
    *pReadFlags = 1;
    return -1;
}

/*-----------------------------------------
 * ServiceStateEventLogExt
 *-----------------------------------------*/
int
ServiceStateEventLogExt (void* pSenInfo, INT8U* pReadFlags, int BMCInst)
{
    return 0;
}

/*-----------------------------------------
 * RAIDSensorInit
 *-----------------------------------------*/
int RAIDSensorInit (void*  pSenInfo,int BMCInst)
{
    SensorInfo_T*  pSensorInfo = pSenInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT8U Index = 0;

    pSensorInfo->PreviousState = 0;
    pSensorInfo->SensorReading = 0;

    switch(pSensorInfo->EntityID)
    {
        case RAID_ROC_ENTITY_ID:
            switch(pSensorInfo->EntiryInstance)
            {
                case RAID_ROC_TEMP1:
                    Index = INTERNAL_SENSOR_RAIDROCTEMP1;
                    break;
                case RAID_ROC_TEMP2:
                    Index = INTERNAL_SENSOR_RAIDROCTEMP2;
                    break;
            }
            break;

        case RAID_EXPANDER_ENTITY_ID:
            switch(pSensorInfo->EntiryInstance)
            {
                case RAID_EXPANDER_TEMP1:
                    Index = INTERNAL_SENSOR_RAIDEXPANDERTEMP1;
                    break;
                case RAID_EXPANDER_TEMP2:
                    Index = INTERNAL_SENSOR_RAIDEXPANDERTEMP2;
                    break;
            }
            break;

        case RAID_BBU_ENTITY_ID:
            switch(pSensorInfo->EntiryInstance)
            {
                case RAID_BBU_TEMP1:
                    Index = INTERNAL_SENSOR_RAIDBBUTEMP1;
                    break;
                case RAID_BBU_TEMP2:
                    Index = INTERNAL_SENSOR_RAIDBBUTEMP2;
                    break;
            }
            break;

    }

    pBMCInfo->InternalSensorTbl[Index].SensorReading = pSensorInfo->SensorReading;

    return 0;

}

/*-----------------------------------------
 * RIADMonitor 
 *-----------------------------------------*/
int 
RAIDMonitor (void* pSenInfo, INT8U* pReadFlags,int BMCInst)
{
    SensorInfo_T*  pSensorInfo = pSenInfo;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    void* libcomphandle = NULL;
    int (*GetROCReading)(INT8U*, INT8U, INT8U) = NULL;
    INT8U   Index = 0;
    int RetVal = 0;
    device_tbl_t    *pdev_tbl;
    hal_t           hal;

    if(g_corefeatures.raid_component_support != ENABLED )
    {
        pSensorInfo->Err = HAL_ERR_READ;
        *pReadFlags = 1;
        return -1;
    }

    switch(pSensorInfo->EntityID)
    {
        case RAID_ROC_ENTITY_ID:
            libcomphandle = dlopen(LIBCOMP_LIB_PATH, RTLD_NOW);
            if(libcomphandle != NULL)
            {
                GetROCReading = dlsym(libcomphandle,"GetROC1Temp");
            
                /*Fill Current Sensor reading  */
                if(GetROCReading)
                {
                    /* get the pointer to sensor properity table */
                    pdev_tbl = get_sensor_table (pSensorInfo->SensorNumber, BMCInst);
                    if((pdev_tbl == NULL) || (pdev_tbl->device_init == NULL))
                        break;

                    pdev_tbl->device_init(&hal);

                    RetVal = GetROCReading((INT8U*)&pSensorInfo->SensorReading, hal.i2c.bus, hal.i2c.slave_addr);
                    /*Disable the Sensor Scanning Bit*/
                    if(RetVal == 1)
                    {
                        pSensorInfo->Err = HAL_ERR_READ;
                    }
                    else if(RetVal == 0)
                    {
                        pSensorInfo->Err = 0;
                    }
                }
                dlclose(libcomphandle);
            }

            switch(pSensorInfo->EntiryInstance)
            {
                case RAID_ROC_TEMP1:
                    Index = INTERNAL_SENSOR_RAIDROCTEMP1;
                    break;
                case RAID_ROC_TEMP2:
                    Index = INTERNAL_SENSOR_RAIDROCTEMP2;
                    break;
            }

            break;

        case RAID_EXPANDER_ENTITY_ID:
            libcomphandle = dlopen(LIBCOMP_LIB_PATH, RTLD_NOW);
            if(libcomphandle != NULL)
            {
                GetROCReading = dlsym(libcomphandle,"GetExpander1Temp");
            
                /*Fill Current Sensor reading  */
                if(GetROCReading)
                {
                    /* get the pointer to sensor properity table */
                    pdev_tbl = get_sensor_table (pSensorInfo->SensorNumber, BMCInst);
                    if((pdev_tbl == NULL) || (pdev_tbl->device_init == NULL))
                        break;

                    pdev_tbl->device_init(&hal);

                    RetVal = GetROCReading((INT8U*)&pSensorInfo->SensorReading, hal.i2c.bus, hal.i2c.slave_addr);
                    /*Disable the Sensor Scanning Bit*/
                    if(RetVal == 1)
                    {
                        pSensorInfo->Err = HAL_ERR_READ;
                    }
                    else if(RetVal == 0)
                    {
                        pSensorInfo->Err = 0;
                    }
                }
                dlclose(libcomphandle);
            }

            switch(pSensorInfo->EntiryInstance)
            {
                case RAID_EXPANDER_TEMP1:
                    Index = INTERNAL_SENSOR_RAIDEXPANDERTEMP1;
                    break;
                case RAID_EXPANDER_TEMP2:
                    Index = INTERNAL_SENSOR_RAIDEXPANDERTEMP2;
                    break;
            }

            break;

        case RAID_BBU_ENTITY_ID:
            libcomphandle = dlopen(LIBCOMP_LIB_PATH, RTLD_NOW);
            if(libcomphandle != NULL)
            {
                GetROCReading = dlsym(libcomphandle,"GetBBU1Temp");
            
                /*Fill Current Sensor reading  */
                if(GetROCReading)
                {
                    /* get the pointer to sensor properity table */
                    pdev_tbl = get_sensor_table (pSensorInfo->SensorNumber, BMCInst);
                    if((pdev_tbl == NULL) || (pdev_tbl->device_init == NULL))
                        break;

                    pdev_tbl->device_init(&hal);

                    RetVal = GetROCReading((INT8U*)&pSensorInfo->SensorReading, hal.i2c.bus, hal.i2c.slave_addr);
                    /*Disable the Sensor Scanning Bit*/
                    if(RetVal == 1)
                    {
                        pSensorInfo->Err = HAL_ERR_READ;
                    }
                    else if(RetVal == 0)
                    {
                        pSensorInfo->Err = 0;
                    }
                }
                dlclose(libcomphandle);
            }

            switch(pSensorInfo->EntiryInstance)
            {
                case RAID_BBU_TEMP1:
                    Index = INTERNAL_SENSOR_RAIDBBUTEMP1;
                    break;
                case RAID_BBU_TEMP2:
                    Index = INTERNAL_SENSOR_RAIDBBUTEMP2;
                    break;
            }

            break;
    }

    TDBG("Getting the RAID Temp %d Entity ID %d",pSensorInfo->SensorReading, pSensorInfo->EntityID);

    pBMCInfo->InternalSensorTbl[Index].SensorReading = pSensorInfo->SensorReading;

    /* Sensor Reading is calculated don't proceed regular monitoring */
    *pReadFlags = 1;
    return -1;

}

/*---------------------------------------------------------------------
 * RAIDEventLog
 *--------------------------------------------------------------------*/
int 
RAIDEventLog (void* pSenInfo, INT8U* pReadFlags,int BMCInst)
{
    return 0;
}

/*---------------------------------------------------------------------
 * RAIDEventLogExt
 *--------------------------------------------------------------------*/
int 
RAIDEventLogExt (void* pSenInfo, INT8U* pReadFlags,int BMCInst)
{
    return 0;
}

