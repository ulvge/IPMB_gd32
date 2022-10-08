/****************************************************************
 ****************************************************************
 **                                                            **

 ****************************************************************
 *****************************************************************
 *
 * SensorMonitor.c
 * Sensor Monitor
 *
 * Author: 
 *
 *****************************************************************/
#define  ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "MsgHndlr.h"
#include "IPMI_Main.h"
#include "Debug.h"
#include "IPMIDefs.h"
#include "OSPort.h"
#include "IPMI_Sensor.h"
#include "Sensor.h"
#include "Message.h"
#include "IPMI_SEL.h"
#include "SEL.h"
#include "PEF.h"
#include "SDRFunc.h"
#include "HWConfig.h"
#include "SharedMem.h"
#include "Support.h"
#include "NVRData.h"
#include "Util.h"
#include "AppDevice.h"
#include "IPMI_KCS.h"
#include "IPMI_BT.h"
#include "SensorMonitor.h"
#include "ipmi_hal.h"
#include "SensorAPI.h"
#include "IPMBIfc.h"
#include "IPMIConf.h"
#include "PDKAccess.h"
#include <dlfcn.h>
#include <sys/prctl.h>
#include "featuredef.h"

#if SENSOR_DEVICE == 1

/*** Local Definitions ***/
#define FULL_SDR_REC                0x01
#define COMPACT_SDR_REC             0x02
#define EVENT_MSG_LENGTH            9

#define TEMP_SENSOR_TYPE            0x01
#define VOLT_SENSOR_TYPE            0x02
#define FAN_SENSOR_TYPE             0x04
#define POWER_SENSOR_TYPE           0x08
#define CURRENT_SENSOR_TYPE         0x03
#define MONITOR_ASIC_IC_SENSOR_TYPE 0x26
#define OEM_SENSOR_TYPE             0xC0
#define OTHER_UNITS_SENSOR_TYPE     0x0B

#define LOWER_CRITICAL_GNG_LOW         0x02
#define UPPER_CRITICAL_GNG_HIGH        0x09 
#define LOWER_NON_CRITICAL_GNG_LOW     0x00
#define UPPER_NON_CRITICAL_GNG_HIGH    0x07
#define LOWER_NON_RECOVERABLE_GNG_LOW  0x04 
#define UPPER_NON_RECOVERABLE_GNG_HIGH 0x0B

#define SENSOR_MONITOR_INTERVAL     1

/*** Variable used for sensor scanning bit ***/
#define MAX_SENSOR_MONITOR_LOOP_COUNT 4

/*** Prototype Declaration ***/
static void MonitorTSensors (SensorInfo_T*  pSensorInfo,int BMCInst);
static void MonitorNonTSensors (SensorInfo_T*  pSensorInfo,int BMCInst);
static void InitSensorPDKHooks (int BMCInst);
static void SendSignalToAdviser(SELEventRecord_T *pSelRecord,int BMCInst);
static int PreEventLog (SensorInfo_T* pSensorInfo, INT8U* pEventData,int BMCInst);
static void SM_ReArmSensor      (ReArmSensorReq_T* pReArmSensorReq,int BMCInst);
static void GenerateDeassertionEvent(INT16U EventsToRearm, SensorInfo_T* pSensorInfo,int BMCInst);
static void ProcessSMMessages   (int BMCInst);
static int SwapSensorThresholds (int BMCInst, INT8U SensorNum, INT8U OwnerLUN, INT8U OwnerID );

#if (TERMINAL_MODE_SUPPORT == 1)
static void UpdateThresHealthState (SensorInfo_T*     pSensorInfo, INT8U Level,int BMCInst);
static void UpdateOverallHealthState (int BMCInst);
#endif


/*
 * @fn InitPowerOnTick
 * @brief Initialize the power on tick counter, 
 *        should be invoked when ever power on occurs.
 */
void InitPowerOnTick (int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    pBMCInfo->SenConfig.PowerOnTick = 0;
}

/*
 * @fn InitSysResetTick
 * @brief Initialize the System Reset tick counter, 
 *        should be invoked when ever power reset occurs.
 */
void InitSysResetTick (int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    pBMCInfo->SenConfig.SysResetTick = 0;
}

/*-------------------------------------------
 * InitSensorMonitor
 *------------------------------------------*/
int
InitSensorMonitor (int BMCInst)
{ 

    /* initilize the softprocessor sensor table */
    IPMI_HAL_INIT_SENSOR (BMCInst);

    /* initilize the sensor devices */
    IPMI_HAL_INIT_DEVICES (BMCInst);

    /* Init Sensor Pre and Post Monitor hooks */
    InitSensorPDKHooks (BMCInst);

    IPMI_DBG_PRINT ("Initilized Sensor Monitor\n");
    
    if( NULL != g_PDKHandle[PDK_POSTINITSENSORMONITOR] )
    {
        ((void (*)(int)) g_PDKHandle[PDK_POSTINITSENSORMONITOR]) (BMCInst);
    }

    return TRUE;
}

/*-------------------------------------------
 * GetSensorInfo
 * 
 * This function will be used by the Sensor
 * hook functions to manipulate Sensor information
 * This function DOES NOT lock sensor information
 *------------------------------------------*/
SensorInfo_T*
GetSensorInfo (INT8U SensorNum, INT8U OwnerLUN, int BMCInst)
{
    SensorSharedMem_T* pSMSharedMem = (SensorSharedMem_T*)&g_BMCInfo[BMCInst].SensorSharedMem;
    return &pSMSharedMem->SensorInfo [((OwnerLUN & VALID_LUN) << 8 | SensorNum)];	
}

static void *
SensorMonitorTimer(void *pArg)
{
    MsgPkt_T Msg;
    int lasterrstate=0;
    int misscount=0;
    HQueue_T hSMHndlr_Q;
    int *inst = (int*) pArg;
    int BMCInst = *inst;

    Msg.Param = PARAM_SENSOR_SCAN;
    Msg.Size = 0;
    prctl(PR_SET_NAME,__FUNCTION__,0,0,0);
     
    while(1)
    {
        /* Delay between monitoring cycles */
        OS_TIME_DELAY_HMSM (0, 0, SENSOR_MONITOR_INTERVAL, 0);

        // Send the message to the SensorMonitor task that it is time
        // for a sensor scan.
        GetQueueHandle(SM_HNDLR_Q,&hSMHndlr_Q,BMCInst);
        if ( -1 != hSMHndlr_Q )
        {
            if (!g_BMCInfo[BMCInst].SenConfig.MonitorBusy)
            {
                if (lasterrstate == 1)
                {
                    //					printf("INFO: Sensor Monitor Task Resumed after %d * %d seconds\n",
                    //										misscount,SENSOR_MONITOR_INTERVAL);
                    lasterrstate = 0;
                }
                misscount=0;
                g_BMCInfo[BMCInst].SenConfig.MonitorBusy = 1;
                PostMsg(&Msg, SM_HNDLR_Q,BMCInst);
            }
            else
            {
                if (lasterrstate == 0)
                {
                    //					printf("ERROR: Sensor Monitor Task Pending. Cannot Post another \n");
                    lasterrstate =1;
                }
                misscount++;
            }
        }
    }
    
    return NULL;
}
/*-------------------------------------------
 * SensorMonitorTask
 *----------------------               --------------------*/
void*
SensorMonitorTask (void *pArg)
{
    INT8U  		SensorTick = 0;
    int    		i;
	INT16U  		SensorNum = 0;
    int *inst = (int*) pArg;
    int BMCInst = *inst;
    SensorInfo_T*        pSensorInfo;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    SensorSharedMem_T*    pSMSharedMem;
    int retval = 0;

    prctl(PR_SET_NAME,__FUNCTION__,0,0,0);
    int curThreadIndex = 0;

    if(g_corefeatures.ipmi_thread_monitor_support == ENABLED)
    {
        pthread_t thread_id = pthread_self();
        for(; curThreadIndex <= gthreadIndex; curThreadIndex++)
        {
            if(pthread_equal(gthreadIDs[curThreadIndex], thread_id))
            {
                IPMI_DBG_PRINT_2("SensorMonitor.c: SensorMonitorTask thread[ID: %d] found with index=%d.\n", thread_id, curThreadIndex);
                gthread_monitor_health[curThreadIndex].validThreadID = THREAD_ID_FOUND;
                gthread_monitor_health[curThreadIndex].BMCInst = BMCInst;
                strcpy(gthread_monitor_health[curThreadIndex].threadName, __FUNCTION__);
                break;
            }
        }
    }

    pBMCInfo->SenConfig.MonitorBusy = 0;
    pBMCInfo->SenConfig.SensorMonitorLoopCount = 0;
    pBMCInfo->SenConfig.InitAgentRearm = FALSE;

    /* Get the Sensor Shared Memory */
    pSMSharedMem = (SensorSharedMem_T*)&pBMCInfo->SensorSharedMem;
    
    /* Diable Global Sensor monitoring during initialization
     * Global Scanning will be enabled when Msghandler
     * does all initialization */
    pSMSharedMem->GlobalSensorScanningEnable = FALSE;
    
    /* Create mutex for Sensor shared memory */
    OS_THREAD_MUTEX_INIT(pBMCInfo->m_hSMSharedMemMutex, PTHREAD_MUTEX_RECURSIVE); 

    InitSensorMonitor (BMCInst);

    /* This table helps us to reduce unnecessary looping in sensor scanning  */
    pBMCInfo->SenConfig.ValidSensorCnt = 0;
    IPMI_DBG_PRINT_2("\n\n******* MAX_SENSOR_NUMBERS = %d VALID_LUN = 0x%x ******\n\n",MAX_SENSOR_NUMBERS, VALID_LUN);
    for ( i = 0; i < MAX_SENSOR_NUMBERS; i++)
    {
        /* Check if sensor present */   
        if (!pSMSharedMem->SensorInfo [i].IsSensorPresent) { continue; }

        if(g_PDKHandle[PDK_BEFOREVALIDSENSORLIST] != NULL)
    	{
            retval = ((int (*)(void *, int, int)) g_PDKHandle[PDK_BEFOREVALIDSENSORLIST]) ((void *)&(pSMSharedMem->SensorInfo[i]), i, BMCInst);
		    if (retval != 0)
                continue;
        }

        pBMCInfo->SenConfig.ValidSensorList [pBMCInfo->SenConfig.ValidSensorCnt] = i;
        pBMCInfo->SenConfig.OwnerIDList [pBMCInfo->SenConfig.ValidSensorCnt] =  pBMCInfo->IpmiConfig.BMCSlaveAddr;
        pBMCInfo->SenConfig.ValidSensorCnt++;
    }

    /* Decide whether to monitor the sensors with ownerID =  NMDevSlaveAddress. */
    /* Putting them into ValidSensorList : compares threshold, logs SEL, and displays on WEB. */
    /* Don't put them into ValidSensorList : don't compare threshold, don't logs SEL, and don't displays on WEB. */
    if ((g_corefeatures.node_manager == ENABLED) && (ENABLED == g_corefeatures.monitor_sensors_belong_to_me)){
        for ( i = 0; i < MAX_ME_SENSOR_NUMBERS; i++)
        {
            /* Check if sensor present */   
            if (!pSMSharedMem->ME_SensorInfo[i].IsSensorPresent) { continue; }

            if(g_PDKHandle[PDK_BEFOREVALIDSENSORLIST] != NULL)
            {
                retval = ((int (*)(void *, int, int)) g_PDKHandle[PDK_BEFOREVALIDSENSORLIST]) ((void *)&(pSMSharedMem->ME_SensorInfo[i]), i, BMCInst);
                if (retval != 0)
                    continue;
            }

            pBMCInfo->SenConfig.ValidSensorList [pBMCInfo->SenConfig.ValidSensorCnt] = i;
            pBMCInfo->SenConfig.OwnerIDList [pBMCInfo->SenConfig.ValidSensorCnt] = pBMCInfo->NMConfig.NMDevSlaveAddress;	
            pBMCInfo->SenConfig.ValidSensorCnt++;
        }
    }

    /* Acquire Shared memory   */
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->m_hSMSharedMemMutex, SHARED_MEM_TIMEOUT);

    /* Update Sensor Properties & initialize sensors */
    for (i = 0; i < pBMCInfo->SenConfig.ValidSensorCnt; i++)
    {
        if ((g_corefeatures.node_manager == ENABLED)&&(pBMCInfo->NMConfig.NMDevSlaveAddress == pBMCInfo->SenConfig.OwnerIDList[i] )){
            pSensorInfo = (SensorInfo_T*)&pSMSharedMem->ME_SensorInfo [pBMCInfo->SenConfig.ValidSensorList [i]];
        }else{
            pSensorInfo = (SensorInfo_T*)&pSMSharedMem->SensorInfo [pBMCInfo->SenConfig.ValidSensorList [i]];
        }  

		SensorNum = pBMCInfo->SenConfig.ValidSensorList[i];	/* Multi-LUN support*/
		if(pBMCInfo->SenConfig.ValidSensorList[i] > 255)	/* Multi-LUN support*/
		{
			SensorNum = pBMCInfo->SenConfig.ValidSensorList[i];
			pSensorInfo->SensorNumber = (SensorNum & 0xff);
			pSensorInfo->SensorOwnerLun = ((SensorNum >> 8) & VALID_LUN);
		}
		IPMI_DBG_PRINT_4("i = %x SensorNum = %x pSensorInfo->SensorOwnerLun = %x pSensorInfo->SensorNumber = %x\n", i, SensorNum, pSensorInfo->SensorOwnerLun, pSensorInfo->SensorNumber);

		LoadSensorProperties (pBMCInfo->SenConfig.ValidSensorList [i],  pBMCInfo->SenConfig.OwnerIDList[i],BMCInst);
        if(g_PDKHandle[PDK_LOADOEMSENSORDEFAULT] != NULL)
        {
            ((void(*)(SensorInfo_T*,int))g_PDKHandle[PDK_LOADOEMSENSORDEFAULT]) (pSensorInfo,BMCInst);
        }
        /* Enable internal Sensor if it has a valid SDR */
        ReInitSensor(pSensorInfo, pBMCInfo->SenConfig.OwnerIDList[i], BMCInst);
        if(g_corefeatures.internal_sensor == ENABLED){
            if(pBMCInfo->IpmiConfig.BMCSlaveAddr== pBMCInfo->SenConfig.OwnerIDList[i])    /* Only for BMC internal sensors */
            {
                InitInternalSensors (pSensorInfo,BMCInst);
                InitSensorHook (pSensorInfo,BMCInst);
            }
        }
    }

    /* Release mutex for Sensor shared memory */
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->m_hSMSharedMemMutex);

    /* Syncronize with Msghandler by releasing the SM mutex*/
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SMmutex);

    // Create the thread that will notify us when a scan is needed.
    gthreadIndex++;
    IPMI_DBG_PRINT_1("SensorMonitor.c: Creating SensorMonitorTimer thread with index %d\n", gthreadIndex);
    OS_CREATE_TASK_THREAD(SensorMonitorTimer, (void*)&BMCInst, err, gthreadIDs[gthreadIndex]);

    if(g_PDKHandle[PDK_REGISTERSENSORHOOKS] != NULL)
    {
        ((void (*)(int)) g_PDKHandle[PDK_REGISTERSENSORHOOKS]) (BMCInst);
    }
    while (1)
    {
        pBMCInfo->SenConfig.InitAgentRearm = FALSE;
        if(g_corefeatures.ipmi_thread_monitor_support == ENABLED && gthread_monitor_health[curThreadIndex].validThreadID == THREAD_ID_FOUND)
        {
            OS_THREAD_MUTEX_ACQUIRE(&ThreadMonitorMutex, WAIT_INFINITE);
            gthread_monitor_health[curThreadIndex].monitor = THREAD_STATE_MONITOR_NO;
            gthread_monitor_health[curThreadIndex].time = 0;
            OS_THREAD_MUTEX_RELEASE(&ThreadMonitorMutex);
        }

        ProcessSMMessages(BMCInst);

        if(g_corefeatures.ipmi_thread_monitor_support == ENABLED && gthread_monitor_health[curThreadIndex].validThreadID == THREAD_ID_FOUND)
        {
            OS_THREAD_MUTEX_ACQUIRE(&ThreadMonitorMutex, WAIT_INFINITE);
            gthread_monitor_health[curThreadIndex].monitor = THREAD_STATE_MONITOR_YES;
            OS_THREAD_MUTEX_RELEASE(&ThreadMonitorMutex);
        }

        if(g_PDKHandle[PDK_PREMONITORALLSENSORS] != NULL)
        {
            if( -1 == ((int(*)(bool,int)) g_PDKHandle[PDK_PREMONITORALLSENSORS])(pBMCInfo->SenConfig.InitAgentRearm,BMCInst) )  // PDK_PreMonitorAllSensors (g_BMCInfo[BMCInst].SenConfig.InitAgentRearm))
            {
                IPMI_DBG_PRINT ("PDK_PreMonitorAllSensors returned -1\n");
                //Clear the MonitorBusy flag before continue to GetMsg
                //This is required to avoid the break of Sensor Monitor thread in middle 
                pBMCInfo->SenConfig.MonitorBusy = 0;
                continue;
            }
        }

        /* Monitor all sensors */
        for (i = 0; i < pBMCInfo->SenConfig.ValidSensorCnt; i++)
        {
            //	    	usleep (40 * 1000);
            /* Check if Global Scanning Enabled or Disabled */
            if (pSMSharedMem->GlobalSensorScanningEnable)
            {
                SensorNum = pBMCInfo->SenConfig.ValidSensorList [i];

                if((g_corefeatures.node_manager == ENABLED)&&(pBMCInfo->NMConfig.NMDevSlaveAddress == pBMCInfo->SenConfig.OwnerIDList[i] )){
                    pSensorInfo = (SensorInfo_T*)&pSMSharedMem->ME_SensorInfo[SensorNum];
                }else{
                    pSensorInfo = (SensorInfo_T*)&pSMSharedMem->SensorInfo[SensorNum];
                }

                OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->m_hSMSharedMemMutex, SHARED_MEM_TIMEOUT);

                /* ReInit Sensor if required . Check BIT0 for reinitialization */
                if (0 != (pSensorInfo->EventFlags & BIT0))
                {
                    ReInitSensor(pSensorInfo,  pBMCInfo->SenConfig.OwnerIDList[i], BMCInst);

                    // Clear any previous assertion/deassertion history.
                    pSensorInfo->AssertionEventOccuredByte1 = 0;
                    pSensorInfo->AssertionEventOccuredByte2 = 0;
                    pSensorInfo->DeassertionEventOccuredByte1 = 0;
                    pSensorInfo->DeassertionEventOccuredByte2 = 0;
                    pSensorInfo->AssertionHistoryByte1 = 0;
                    pSensorInfo->AssertionHistoryByte2 = 0;
                    pSensorInfo->DeassertionHistoryByte1 = 0;
                    pSensorInfo->DeassertionHistoryByte2 = 0;

                }

                // Check SDR scanning bit
                if ((0 != (pSensorInfo->InternalFlags & BIT0)) || (0 == (pSensorInfo->EventFlags & BIT6)))
                {
                    pSensorInfo->Err = CC_SDR_REC_NOT_PRESENT;

                    /* Release mutex for Sensor shared memory */
                    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->m_hSMSharedMemMutex);
                    //Clear the MonitorBusy flag before continue to GetMsg
                    //This is required to avoid the break of Sensor Monitor thread in middle 
                    pBMCInfo->SenConfig.MonitorBusy = 0;    
                    continue;
                }

                if(g_PDKHandle[PDK_PREMONITORINDIVIDUALSENSOR] != NULL)
                {
                	// Invoke the Check Sensortick to verify whether scan time reached. 
                	if (-1 == ((int(*)(SensorInfo_T*,int)) g_PDKHandle[PDK_PREMONITORINDIVIDUALSENSOR])(pSensorInfo,BMCInst))
                	{
                		/* Clear the Error status for this scan */
                		pSensorInfo->Err = 0;

                		/* Release mutex for Sensor shared memory */
                		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->m_hSMSharedMemMutex);
                		//Clear the MonitorBusy flag before continue to GetMsg
                		//This is required to avoid the break of Sensor Monitor thread in middle 
                		pBMCInfo->SenConfig.MonitorBusy = 0;                		
                		continue;
                	}
                }

                /* Monitoring cycle */
                if (THRESHOLD_SENSOR_CLASS  == pSensorInfo->EventTypeCode)
                {
                    if ((0 == (SensorTick %  pSensorInfo->SensorMonitorInterval)) &&
                        (pBMCInfo->SenConfig.PowerOnTick >= pSensorInfo->PowerOnDelay) &&
                        (pBMCInfo->SenConfig.SysResetTick >= pSensorInfo->SysResetDelay)) 
                    { 
                        OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo[BMCInst].SensorSharedMemMutex, WAIT_INFINITE);
                        /* Monitor threshold sensor */
                        MonitorTSensors (pSensorInfo,BMCInst); 
                        OS_THREAD_MUTEX_RELEASE(&g_BMCInfo[BMCInst].SensorSharedMemMutex);
                    }
                }
                else
                {
                    if ((0 == (SensorTick %  pSensorInfo->SensorMonitorInterval)) &&
                        (pBMCInfo->SenConfig.PowerOnTick >= pSensorInfo->PowerOnDelay) &&
                        (pBMCInfo->SenConfig.SysResetTick >= pSensorInfo->SysResetDelay))
                    { 
                        OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo[BMCInst].SensorSharedMemMutex, WAIT_INFINITE);
                        /* Monitor Non threshold sensor */
                        MonitorNonTSensors (pSensorInfo,BMCInst);
                        OS_THREAD_MUTEX_RELEASE(&g_BMCInfo[BMCInst].SensorSharedMemMutex);
                    }
                }

                /* Release mutex for Sensor shared memory */
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->m_hSMSharedMemMutex);

            }

    } 

#if (TERMINAL_MODE_SUPPORT == 1)
        /* Calculate Health State for terminal mode */ 
        UpdateOverallHealthState (BMCInst);
#endif

        /* PDK Hook after monitoring all Sensors */
        if(g_PDKHandle[PDK_POSTMONITORALLSENSORS] != NULL)
        {
            ((int(*)(INT8U,INT16 *,int)) g_PDKHandle[PDK_POSTMONITORALLSENSORS])(pBMCInfo->SenConfig.NumThreshSensors + pBMCInfo->SenConfig.NumNonThreshSensors, NULL,BMCInst);
        }

        SensorTick++;
        pBMCInfo->SenConfig.PowerOnTick++;
        pBMCInfo->SenConfig.SysResetTick++;
        pBMCInfo->SenConfig.MonitorBusy = 0;

        /* Init of Sensor scanning bit happens only once for a boot */
        if (pBMCInfo->SenConfig.SensorMonitorLoopCount == MAX_SENSOR_MONITOR_LOOP_COUNT)
        {
            IPMI_DBG_PRINT("Initializing Sensor Scanning bit is under progress.. \n");
            /* Acquire Shared memory   */
            OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->m_hSMSharedMemMutex, SHARED_MEM_TIMEOUT);
            pBMCInfo->SenConfig.SensorMonitorLoopCount++;
            InitSensorScanningBit(BMCInst);
            /* Release mutex for Sensor shared memory */
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->m_hSMSharedMemMutex);
            IPMI_DBG_PRINT("Initializing Sensor Scanning bit is Completed. \n");
        }
        else if (pBMCInfo->SenConfig.SensorMonitorLoopCount < MAX_SENSOR_MONITOR_LOOP_COUNT)
        {
            pBMCInfo->SenConfig.SensorMonitorLoopCount++;
        }

    } 

}


/*---------------------------------------------------
 *PostEventMessage
 *---------------------------------------------------*/
int
PostEventMessage (INT8U *EventMsg, INT8U sysifcflag,INT8U size,int BMCInst)
{
    INT8U               SelReq [sizeof(SELEventRecord_T)];
    INT8U               SelRes [sizeof(AddSELRes_T)]; //*curchannel;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    MsgPkt_T            MsgToPEF;
    SELEventRecord_T*   SelRecord = (SELEventRecord_T*) SelReq;
    AddSELRes_T*        AddSelRes = (AddSELRes_T*) SelRes;
     BMCSharedMem_T* pSharedMem = BMC_GET_SHARED_MEM(BMCInst);

    if (0xFF == pSharedMem->EvRcv_SlaveAddr)
    {
        // If Slave address is 0xFF then Event generation should be disabled entirely.
        return 0;
    }

    if (pBMCInfo->IpmiConfig.BMCSlaveAddr != pSharedMem->EvRcv_SlaveAddr)
    {
        // Forward the event to the specified slave address.
        IPMI_DBG_PRINT_1 ("Event occured and transfered to 0x%X slave address", pSharedMem->EvRcv_SlaveAddr);
        IPMI_DBG_PRINT_BUF (EventMsg, size);
        
        //Adjustment made due the reason that GeneratorID is a single byte in IPMB.
        EventMsg[1] = EventMsg[0];

        MsgToPEF.Param = IPMB_EVT_MSG_REQUEST;
        MsgToPEF.Size  = size - 1;
        _fmemcpy ((INT8U*)MsgToPEF.Data, EventMsg + 1,
                  size - 1);

        PostMsg(&MsgToPEF, IPMB_PRIMARY_IFC_Q,BMCInst);
        return 0;
    }

    SelRecord->hdr.Type = 0x02;
    SelRecord->hdr.TimeStamp = GetSelTimeStamp (BMCInst);
    _fmemcpy (SelRecord->GenID, EventMsg, size);
    IPMI_DBG_PRINT_BUF (SelReq, sizeof (SELEventRecord_T));
#if SEL_DEVICE == 1
    g_BMCInfo[BMCInst].SELConfig.SenMonSELFlag = 0;
 //   OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SELConfig.SELMutex, WAIT_INFINITE);
    LockedAddSELEntry(SelReq, sizeof (SELEventRecord_T), SelRes,sysifcflag,POST_SEL_AND_PEF,BMCInst);
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SELConfig.SELMutex);
    g_BMCInfo[BMCInst].SELConfig.SenMonSELFlag = 0;
#endif /*SEL_DEVICE*/

    SendSignalToAdviser(SelRecord,BMCInst);

    IPMI_DBG_PRINT_1 ("SEL RECORD ID = %x\n", AddSelRes->RecID);
    SelRecord->hdr.ID = AddSelRes->RecID;
    /* Post message to Platform Event Filter */
    MsgToPEF.Param = PARAM_SENSOR_EVT_MSG;
    MsgToPEF.Size  = sizeof (SELEventRecord_T);
    _fmemcpy ((INT8U*)MsgToPEF.Data, (INT8U*)SelReq,
              sizeof(SELEventRecord_T));
    IPMI_DBG_PRINT ("Posting Message to PEF\n");
    //PostMsgNonBlock (&MsgToPEF, hPEFTask_Q);

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->EventMutex,WAIT_INFINITE);
    /* Post to the Event message queue only if event message buffer enabled */
    if ((BMC_GET_SHARED_MEM (BMCInst)->GlobalEnables & 0x04) &&
        (BMC_GET_SHARED_MEM (BMCInst)->NumEvtMsg < EVT_MSG_BUF_SIZE))
    {
        PostMsgNonBlock (&MsgToPEF, EVT_MSG_Q,BMCInst);
        BMC_GET_SHARED_MEM (BMCInst)->NumEvtMsg++;
    }

    if(BMC_GET_SHARED_MEM (BMCInst)->GlobalEnables & 0x04)
    {
        /* If Event MessageBuffer is Full set the SMS_ATN bit */
        if (BMC_GET_SHARED_MEM (BMCInst)->NumEvtMsg >= EVT_MSG_BUF_SIZE)
        {
            //SET_SMS_ATN ();
            if (pBMCInfo->IpmiConfig.KCS1IfcSupport == 1)
            {
                SET_SMS_ATN (0, BMCInst);
                if(g_corefeatures.kcs_obf_bit == ENABLED)
                {
                    SET_OBF (0, BMCInst);
                }
            }

            if (pBMCInfo->IpmiConfig.KCS2IfcSupport == 1)
            {
                SET_SMS_ATN (1, BMCInst);
                if(g_corefeatures.kcs_obf_bit == ENABLED)
                {
                    SET_OBF (1, BMCInst);
                }
            }

            if (pBMCInfo->IpmiConfig.KCS3IfcSuppport == 1)
            {
                SET_SMS_ATN (2, BMCInst);
                if(g_corefeatures.kcs_obf_bit == ENABLED)
                {
                    SET_OBF (2, BMCInst);
                }
            }
            
            if(pBMCInfo->IpmiConfig.BTIfcSupport == 1 )
            {
                SET_BT_SMS_ATN (0, BMCInst);
            }
        }
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->EventMutex);
    return 0;
}

/**
 * @brief Get Sensor Properties.
**/
void
LoadSensorProperties (INT16U SensorNum, INT8U OwnerID, int BMCInst)    /* Multi-LUN support - SensorNum is equal to (LUN << 8) | Sensor Number */
{
    SensorProperties_T   SensorProperties;
    SensorInfo_T*        pSensorInfo;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    SensorSharedMem_T* pSMSharedMem = (SensorSharedMem_T*)&g_BMCInfo[BMCInst].SensorSharedMem;

    if ((g_corefeatures.node_manager == ENABLED)&&(pBMCInfo->NMConfig.NMDevSlaveAddress == OwnerID )){
        /*  Fetch different array according to ownerID. */
        pSensorInfo = (SensorInfo_T*)&pSMSharedMem->ME_SensorInfo [SensorNum];
    }else{
        pSensorInfo = (SensorInfo_T*)&pSMSharedMem->SensorInfo [SensorNum];
    }

    if ((g_corefeatures.node_manager == ENABLED)&&(pBMCInfo->NMConfig.NMDevSlaveAddress == OwnerID )){
        /* Fill different data according to ownerID. */
        /* ME sensors have static MonitorInterval and SensorState. HAL API is for BMC sensor only. */
        pSensorInfo->SensorMonitorInterval    = 1;
        pSensorInfo->SensorState              = 1;
    }else{
        /* HAL API is for BMC sensor only. */    	
        /* Get Sensor Properties from Softprocessor */
        IPMI_HAL_GET_SENSOR_PROPERTIES ( SensorNum, &SensorProperties,BMCInst);/* Multi-LUN support */
        /* Update Sensor properties to shared memory */
        pSensorInfo->SensorMonitorInterval    = (SensorProperties.MonitorInterval == 0) ? 1 : SensorProperties.MonitorInterval; 
        pSensorInfo->SensorState              = SensorProperties.MonitorState;
    }

    if (pSensorInfo->SensorReadType != THRESHOLD_SENSOR_CLASS)
    {
       /* Get the normal default value for the digital sensors */
       pSensorInfo->PreviousState 	   = ipmitoh_u16 (SensorProperties.NormalValue);
    }

    pSensorInfo->PowerOnDelay = SensorProperties.PowerOnDelay;
    pSensorInfo->SysResetDelay = SensorProperties.SysResetDelay;

}

/**
 * @brief ReInitSensor.
**/
void
ReInitSensor (SensorInfo_T*  pSensorInfo, INT8U SensorOwnerID, int BMCInst)
{
    SDRRecHdr_T* 	    sr;
    INT8U				SensorNum = 0;
    struct stat buf;
    char SensorFileName[MAX_SEN_NAME_SIZE];
    
    memset(SensorFileName,0,MAX_SEN_NAME_SIZE);
        
    SENSORTHRESH_FILE(BMCInst, SensorFileName);

    /* Get SDR for that sensor */
    sr = SR_FindSDR (pSensorInfo->SensorNumber, pSensorInfo->SensorOwnerLun, SensorOwnerID, BMCInst);	/* Multi-LUN support */

    if (sr == NULL)
    {
        IPMI_WARNING ("Invalid Sensor number %x. \n", pSensorInfo->SensorNumber);
        SET_SM_INIT_DONE(pSensorInfo->EventFlags);
        return;
    }


    /* Save Sensor number in case of Shared count sensor */
    SensorNum = pSensorInfo->SensorNumber;

    /* Get SDR for that sensor */	
    SR_LoadSDRDefaults (sr,pSensorInfo,BMCInst);

    /* Restore Sensor number in case of Shared count sensor */
    pSensorInfo->SensorNumber = SensorNum;

    /* Invoke init sensor hooks registered if any */
    InitSensorHook (pSensorInfo,BMCInst);	

    SET_SM_INIT_DONE(pSensorInfo->EventFlags);
    
    if(0 == stat(SensorFileName,&buf))
    {
        SwapSensorThresholds(BMCInst,pSensorInfo->SensorNumber, pSensorInfo->SensorOwnerLun, SensorOwnerID);
    }
}

/*-------------------------------------------
 * InitSensorPDKHooks
 *------------------------------------------*/
void
InitSensorPDKHooks (int BMCInst)
{
    int 			i;
    SensorSharedMem_T* pSMSharedMem = (SensorSharedMem_T*)&g_BMCInfo[BMCInst].SensorSharedMem;

    /* Initialize PDK Sensor callback routines  */
    for ( i = 0; i < MAX_SENSOR_NUMBERS; i++)
    {
        /* Check if sensor present */   
        if (!pSMSharedMem->SensorInfo [i].IsSensorPresent) { continue; }

        pSMSharedMem->SensorInfo [i].InitSensorHookCnt  = 0;		
        pSMSharedMem->SensorInfo [i].PostMonitorExtHookCnt = 0;
        pSMSharedMem->SensorInfo [i].PreEventLogHookCnt = 0;
    }

}


/**
 * @brief Pre Monitor sensors.
**/
int
PreMonitorSensor (void*  pSenInfo,INT8U* pReadFlags,int BMCInst)
{
	//SensorInfo_T*  pSensorInfo = pSenInfo;	
	//printf("######## Default PostMonitorSensor called for Sensor[%x]---------\n",pSensorInfo->SensorNumber);
	return 0;
}


/**
 * @brief Post Monitor sensors.
**/
int
PostMonitorSensor (void*  pSenInfo,INT8U* pReadFlags,int BMCInst)
{
	//SensorInfo_T*  pSensorInfo = pSenInfo;
	//printf("####### Default Post MonitorSensor called for sensor [%x]\n",pSensorInfo->SensorNumber);
	return 0;
}

/**
 * @brief Init sensors Hooks.
**/
int
InitSensorHook (SensorInfo_T*  pSensorInfo,int BMCInst)
{
    int 	Hooklist,retval=0;
            
    /* Call all Init sensor Hook   */
    for ( Hooklist = 0; Hooklist < pSensorInfo->InitSensorHookCnt; Hooklist++ )
    {
        if (pSensorInfo->pInitSensor [Hooklist] == NULL) { continue; }
        retval = pSensorInfo->pInitSensor [Hooklist](pSensorInfo,BMCInst);
    }

    pSensorInfo->Err = 0;

    return retval;
}

/**
 * @brief Pre Sensor Event Log.
**/
static int
PreEventLog (SensorInfo_T* pSensorInfo, INT8U* pEventData,int BMCInst)
{
    int 	Hooklist;
    INT8U   RdFlag = 0; 
        
    /* Call all PreEventLog Hook for sensors */
    for (Hooklist = 0; Hooklist < pSensorInfo->PreEventLogHookCnt; Hooklist++)
    {
        if (pSensorInfo->pPreEventLog[Hooklist])
        {
           pSensorInfo->pPreEventLog[Hooklist](pSensorInfo, pEventData, &RdFlag,BMCInst);
        }
    }

    pSensorInfo->Err = 0;

    return RdFlag;
}

#if 0
/* Compare two sensor values.
 * Returns -1 if val1 < val2
 * Returns 0 if val1 == val2
 * Returns 1 if val1 > val2
 */

static int
CompareValues(BOOL isSigned, INT8U val1, INT8U val2)
{
    int retval = 0; // default to equal
    
    /* Do comparison based on isSigned flag */
    if (FALSE == isSigned)
    {
        // Unsigned comparison
        if (val1 < val2)
        {
            retval = -1;
        }
        else if (val1 > val2)
        {
            retval = 1;
        }
    }
    else
    {
        // Signed comparison
        INT16  sval1, sval2;
        
        sval1 = (INT8)val1;
        sval2 = (INT8)val2;
        
        if (sval1 < sval2)
        {
            retval = -1;
        }
        else if (sval1 > sval2)
        {
            retval = 1;
        }
    }

    return retval;
}
#endif
// Return a high deassertion threshold.
// This routine takes into account signed sensors as well as
// values wrapping around boundaries.
static
INT16 GetHighDeassertValue(bool IsSigned, INT16 Value, INT8U Hysteresis)
{
    INT16 deassertValue = (Value - Hysteresis);
    
    if (FALSE == IsSigned)
    {
        if (deassertValue < 0)
            deassertValue = 0;
    }
    else
    {
        if (deassertValue < -128)
            deassertValue = -128;
    }
    
    return deassertValue;
}

// Return a low deassertion threshold.
// This routine takes into account signed sensors as well as
// values wrapping around boundaries.
static
INT16 GetLowDeassertValue(bool IsSigned, INT16 Value, INT8U Hysteresis)
{
    INT16 deassertValue = (Value + Hysteresis);
    if (FALSE == IsSigned)
    {
        if (deassertValue > 255)
            deassertValue = 255;
    }
    else
    {
        if (deassertValue > 127)
            deassertValue = 127;
    }
    
    return deassertValue;
}

/**
 * @brief Monitor Threshold sensors.
 * @param i - Sensor index.
**/
static void
MonitorTSensors (SensorInfo_T*     pSensorInfo,int BMCInst)
{
    INT8U                   EventData1;
    INT8U                   EventData3;
    INT8U                   EvData1;
    INT8U                   SendDeassertion;
    INT8U                   SendAssertion;
    INT8U                   EventMsg [EVENT_MSG_LENGTH];
    INT8U                   Level;
    INT8U                   HealthLevel;
    INT8U                   StartLevel;
    INT16                   Value;
    INT8U                   AssertionEventOccuredByte1;
    INT8U                   AssertionEventOccuredByte2;
    INT8U                   DeassertionEventOccuredByte1;
    INT8U                   DeassertionEventOccuredByte2;
    INT8U                   DeassertionEventEnablesByte1;
    INT8U                   DeassertionEventEnablesByte2;
    INT8U                   AssertionEventEnablesByte1;
    INT8U                   AssertionEventEnablesByte2;
    INT16                   WarningLow;
    INT16                   WarningHigh;
    INT8U                   NegHysteresis;
    INT8U                   PosHysteresis;
    INT16                   CriticalHigh;
    INT16                   CriticalLow;
    INT16                   NonRecoverableHigh;
    INT16                   NonRecoverableLow;
    //INT8U                   SensorLevel;
    INT8U                   ReadableThreshMask;
    INT16U		    		SensorReading;
    INT8U  		    		PSGood = 0;

    INT8U                       AssertionHistoryByte1;
    INT8U                       AssertionHistoryByte2;
    INT8U                       DeassertionHistoryByte1;
    INT8U                       DeassertionHistoryByte2;
    INT8U                   OEMField;
//    INT8U			    SensorNum;	
    BOOL                SignedSensor = 0; // 1 if sensor has signed values
    INT16               SensorMax = 255;
    INT16               SensorMin = 0;
    //INT16U              OrgSensorValue;
    INT16               DeassertThreshold;
    // Added for Sensor Override capability
    int                 Override = 0;
    INT8U ReadFlags = 0;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    /* Get Sensor Reading from SP only if it not a settable sensor */
    if (0 == GET_SETTABLE_SENSOR_BIT(pSensorInfo->SensorInit))
    {
        /* get the power state of host */		

        if(g_PDKHandle[PDK_GETPSGOOD] != NULL)
        {
            PSGood = ((int(*)(int))g_PDKHandle[PDK_GETPSGOOD]) (BMCInst);
        }

        /* If sensor is not need to monitor on stand by power, and if Server */
        /* is power off then return setting error "sensor not acceble at present state" */
        if  (( 0 == (pSensorInfo->SensorState & 0x1) ) && ( 0 == PSGood)) 
        {
            // Response cannot be provided in this state
            pSensorInfo->Err = CC_PARAM_NOT_SUP_IN_CUR_STATE ; 
            pSensorInfo->EventFlags |= BIT5;
            return;
        }
        else
	{
	    if (g_PDKHandle[PDK_SENSOR_OVERRIDE] != NULL)
	    {
	    	// If the hook is there, then call the hook.
	    	Override = ((int (*)(INT8U, INT8U, INT16U *, int)) g_PDKHandle[PDK_SENSOR_OVERRIDE])(pSensorInfo->SensorNumber, pSensorInfo->SensorOwnerLun, &pSensorInfo->SensorReading, BMCInst);
	    }

	    if ((Override == 0) || (g_PDKHandle[PDK_SENSOR_OVERRIDE] == NULL))
	    {
	    	/* Call PDK_PreMonitorSensor and skip default sensor reading if it returns -1 or non zero */
            	if (0 == pSensorInfo->pPreMonitor (pSensorInfo,&ReadFlags,BMCInst))
            	{
                    if(g_corefeatures.cached_sensor_reading == ENABLED)
                    {
                    	OS_THREAD_MUTEX_RELEASE(&g_BMCInfo[BMCInst].SensorSharedMemMutex);
                    }
                
		    if (g_corefeatures.node_manager == ENABLED)
                    {
                        FullSensorRec_T*      sfs=NULL;
                        CompactSensorRec_T*   scs=NULL;

                        if (FULL_SDR_REC==pSensorInfo->SDRRec->Type)
                        {
                            sfs = (FullSensorRec_T*)pSensorInfo->SDRRec;
                            if (sfs->OwnerID != pBMCInfo->NMConfig.NMDevSlaveAddress)
                            {
                                pSensorInfo->Err  = IPMI_HAL_GET_SENSOR_READING ((((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8) | pSensorInfo->SensorNumber),  &SensorReading,BMCInst);	/* Multi-LUN support */
                                if(HAL_ERR_SKIP_SENSOR !=  pSensorInfo->Err )
							    {
							        /* Skip the sensors whose reading is fetched through ME, example: PECI */	

                                    if (g_corefeatures.cached_sensor_reading == ENABLED)
                                    {
                                        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
                                        pSensorInfo->SensorReading = SensorReading;
                                        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
                                    }
                                    else
                                    {
                                        pSensorInfo->SensorReading = SensorReading;
                                    }
							    }                                    
                            }
                        }
                        else if (COMPACT_SDR_REC==pSensorInfo->SDRRec->Type)
                        {
                            scs = (CompactSensorRec_T*)pSensorInfo->SDRRec;
                            if (scs->OwnerID != pBMCInfo->NMConfig.NMDevSlaveAddress)
                            {
                                pSensorInfo->Err  = IPMI_HAL_GET_SENSOR_READING ((((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8) | pSensorInfo->SensorNumber),  &SensorReading,BMCInst);	/* Multi-LUN support */
							    if(HAL_ERR_SKIP_SENSOR !=  pSensorInfo->Err )
							    {
								    /* Skip the sensors whose reading is fetched through ME, example: PECI */
                                    if (g_corefeatures.cached_sensor_reading == ENABLED)
                                    {
                                        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
                                        pSensorInfo->SensorReading = SensorReading;
                                        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
                                    }
                                    else
                                    {
                                        pSensorInfo->SensorReading = SensorReading;
                                    }
							    }                                    
                            }
                       }
                   }
                   else
                   {
                       /* read the sensor */
                       pSensorInfo->Err  = IPMI_HAL_GET_SENSOR_READING ((((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8) | pSensorInfo->SensorNumber),  &SensorReading,BMCInst);	/* Multi-LUN support */
                       if (g_corefeatures.cached_sensor_reading == ENABLED)
                       {
                           OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
                           pSensorInfo->SensorReading = SensorReading;
                           OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
                       }
                       else
                       {
                           pSensorInfo->SensorReading = SensorReading;
                       }
                   }
                   if (g_corefeatures.cached_sensor_reading == ENABLED)
                   {
                       OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo[BMCInst].SensorSharedMemMutex,WAIT_INFINITE);
                   }
                
		   /* Do we have some error  in reading ?? */
                   if ( 0 == pSensorInfo->Err )
                   {
                       // get the power state again 
                       if (g_PDKHandle[PDK_GETPSGOOD] != NULL)
                       {
                           PSGood = ((int(*)(int))g_PDKHandle[PDK_GETPSGOOD]) (BMCInst);
                       }
    
                       /* check to see the power state of the server to confirm its still in power good state */
                       if (( 0 == (pSensorInfo->SensorState & 0x01) ) && (0 == PSGood ))
                       {
                           // Response cannot be provided in this state
                           pSensorInfo->Err = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                           pSensorInfo->pPostMonitor  (pSensorInfo,&ReadFlags,BMCInst);
                           pSensorInfo->EventFlags |= BIT5;
                           return;
                       }
                    
		       // Update pSensorInfo here and comment it below where it was being overwritten by the local variable
                       // Maintain full 16 bit value in case post processing needs the full value.
                       if (pBMCInfo->IpmiConfig.OPMASupport == 1)
                          pSensorInfo->SensorReading += pSensorInfo->SenReadingOffset; 
                   }
                   else
                   {
                       pSensorInfo->pPostMonitor  (pSensorInfo,&ReadFlags,BMCInst);
                       if( HAL_ERR_SKIP_SENSOR != pSensorInfo->Err ){
                           /* Skip the sensors whose reading is fetched through ME, example: PECI */
                           pSensorInfo->EventFlags |= BIT5;
                           return;
                       }
                   }
                }
                else
                {
                    // get the power state again 
                    if (g_PDKHandle[PDK_GETPSGOOD] != NULL)
                    {
                        PSGood = ((int(*)(int))g_PDKHandle[PDK_GETPSGOOD]) (BMCInst);
                    }

                    /* check to see the power state of the server to confirm its still in power good state */
                    if  (( 0 == (pSensorInfo->SensorState & 0x01) ) && (0 == PSGood ))
                    {
                        // Response cannot be provided in this state
                        pSensorInfo->Err = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                        pSensorInfo->pPostMonitor  (pSensorInfo,&ReadFlags,BMCInst);
                        pSensorInfo->EventFlags |= BIT5;
                        return;
                    }
                }
            }
	}

        if (g_corefeatures.ssi_support == ENABLED)
        {
            /* Aggregate thermal sensor readings to represent the overall health of blade. */
            if (pSensorInfo->SensorTypeCode == SENSOR_TYPE_TEMP)
            {
                if (g_PDKHandle[PDK_AGGREGATETHERMAL] != NULL)
                    ((void(*)(SensorInfo_T*, int))g_PDKHandle[PDK_AGGREGATETHERMAL])(pSensorInfo, BMCInst);
            }
        }
    }

    /* Call PDK_PostMonitorSensor and dont monitor the sensor if it returns -1*/
    // Do not call PostMonitor if we are overriding the sensor value since it may change it.
    if ((0 == Override) && (0 != pSensorInfo->pPostMonitor  (pSensorInfo,&ReadFlags,BMCInst)))
    {
        if (0 == pSensorInfo->Err)
        {
            pSensorInfo->EventFlags &= ~BIT5;
        }
        else
        {
            pSensorInfo->EventFlags |= BIT5;
        }
        return;
    }

    /* Call PDK_PostMonitorSensor and dont monitor the sensor if it returns -1*/
    /* Skip the sensors whose reading is fetched through ME, example: PECI */    
    if((0 != pSensorInfo->Err) &&( HAL_ERR_SKIP_SENSOR != pSensorInfo->Err ))
    {
        //IPMI_DBG_PRINT_1 ("Error Accessing sensor %d\n", (pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber);
        pSensorInfo->EventFlags |= BIT5;
        return;
    }

    // Make sure the reading is only 8 bits.
    pSensorInfo->SensorReading &= 0x00ff;
    
#if 0
    IPMI_DBG_PRINT_2  ("SensorNo = %02x     Reading = %02x\n", \
                       (pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 |  pSensorInfo->SensorNumber, pSensorInfo->SensorReading);
#endif

    do
    {
        EvData1                      = 0;
        EventData1                   = 0;
        EventData3                   = 0;
        SendDeassertion              = 0;
        SendAssertion                = 0;
        //SensorLevel                  = 0;
        Level                        = pSensorInfo->EventLevel;
        Value                        = pSensorInfo->SensorReading;
        AssertionEventOccuredByte1   = pSensorInfo->AssertionEventOccuredByte1;
        AssertionEventOccuredByte2   = pSensorInfo->AssertionEventOccuredByte2;
        DeassertionEventOccuredByte1 = pSensorInfo->DeassertionEventOccuredByte1;
        DeassertionEventOccuredByte2 = pSensorInfo->DeassertionEventOccuredByte2;
        AssertionEventEnablesByte1   = pSensorInfo->AssertionEventEnablesByte1;
        AssertionEventEnablesByte2   = pSensorInfo->AssertionEventEnablesByte2;
        DeassertionEventEnablesByte1 = pSensorInfo->DeassertionEventEnablesByte1;
        DeassertionEventEnablesByte2 = pSensorInfo->DeassertionEventEnablesByte2;
        WarningLow                   = pSensorInfo->LowerNonCritical;
        WarningHigh                  = pSensorInfo->UpperNonCritical;
        NegHysteresis                = pSensorInfo->NegHysteresis;
        PosHysteresis                = pSensorInfo->PosHysteresis;
        CriticalHigh                 = pSensorInfo->UpperCritical;
        CriticalLow                  = pSensorInfo->LowerCritical;
        NonRecoverableHigh           = pSensorInfo->UpperNonRecoverable;
        NonRecoverableLow            = pSensorInfo->LowerNonRecoverable;
        OEMField				= pSensorInfo->OEMField;
        //SensorNum				=pSensorInfo->SensorNumber;

        if(1==OEMField)
        {
            API_SensorAverage(pSensorInfo->SensorNumber, pSensorInfo->SensorOwnerLun, &pSensorInfo->SensorReading, BMCInst);
        }

        ReadableThreshMask           = (INT8U)(pSensorInfo->SettableThreshMask);

        SignedSensor = (0 != (pSensorInfo->InternalFlags & BIT1));
        if (SignedSensor)
        {
            // These are max and min sensor readings for a signed sensor
            SensorMax =  127;
            SensorMin = -128;

            //
            // All Thresholds and the sensor value need to be sign extended to 16 bits
            // for signed sensors (if the value is negative).
            //
            WarningLow                = (INT16)((INT8)pSensorInfo->LowerNonCritical);
            WarningHigh               = (INT16)((INT8)pSensorInfo->UpperNonCritical);
            CriticalHigh                 = (INT16)((INT8)pSensorInfo->UpperCritical);
            CriticalLow                  = (INT16)((INT8)pSensorInfo->LowerCritical);
            NonRecoverableHigh    = (INT16)((INT8)pSensorInfo->UpperNonRecoverable);
            NonRecoverableLow     = (INT16)((INT8)pSensorInfo->LowerNonRecoverable);
            Value                          = (INT16)((INT8)pSensorInfo->SensorReading);
        }

        //
        // For unused thresholds, set them to a value that the sensor value cannot
        // have in an 8 bit value so no match can happen.
        //
        if (0 == (ReadableThreshMask & BIT5))
        {
            // Set to a value that the sensor cannot have
            NonRecoverableHigh = SensorMax + 1;
        }
        if (0 == (ReadableThreshMask & BIT2))
        {
            // Set to a value that the sensor cannot have
            NonRecoverableLow = SensorMin - 1;
        }
        if (0 == (ReadableThreshMask & BIT4))
        {
            CriticalHigh = NonRecoverableHigh;
        }
        if (0 == (ReadableThreshMask & BIT1))
        {
            CriticalLow  = NonRecoverableLow;
        }
        if (0 == (ReadableThreshMask & BIT3))
        {
            WarningHigh = CriticalHigh;
        }
        if (0 == (ReadableThreshMask & BIT0))
        {
            WarningLow = CriticalLow;
        }

        StartLevel = Level;

        switch (Level)
        {
            case SENSOR_STATUS_NORMAL:
                if (Value <= WarningLow)
                {
                    IPMI_DBG_PRINT_3("#%02x LNC(A) - Value: %02x Threshold: %02x\n", 
                    ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)WarningLow);
                    /* deassert WarningLow going High */
                    EvData1 = 0x51;
                    if   (DeassertionEventEnablesByte1 & 0x02)
                    {
                        SendDeassertion = 1;
                    }
                    /* remove WarnignLow going Low deassertion */
                    DeassertionEventOccuredByte1 &= ~0x01;
                    /* deassert WarningLow going High */
                    DeassertionEventOccuredByte1 |= 0x02;

                    /* assert WarningLow going Low */
                    EventData1 = 0x50;
                    if(AssertionEventEnablesByte1 & 0x01)
                    {
                        SendAssertion = 1;
                    }
                    Level = SENSOR_STATUS_WARNING_LOW;
                    EventData3 = WarningLow;
                    /* set de/assertion event */
                    /* remove WarningLow going High assertion */
                    AssertionEventOccuredByte1 &= ~0x02;
                    /* assert WarningLow going Low */
                    AssertionEventOccuredByte1 |= 0x01;
                    //SensorLevel = LWR_NON_CRITICAL_GOING_LOW_CTRL_FN;
                }
                if (Value >= WarningHigh)
                {
                    IPMI_DBG_PRINT_3("#%02x UNC(A) - Value: %02x Threshold: %02x\n",
                    ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)WarningHigh);
                    /* deassert WarningHigh going Low */
                    EvData1 = 0x56;
                    if (DeassertionEventEnablesByte1 & 0x40)
                    {
                        SendDeassertion = 1;
                    }

                    /* remove WarningHigh going High deassertion */
                    DeassertionEventOccuredByte1 &= ~0x80;
                    /*  deassert WarningHigh going Low */
                    DeassertionEventOccuredByte1 |= 0x40;

                    /* assert WarningHigh going High */
                    EventData1 = 0x57;
                    if (AssertionEventEnablesByte1 & 0x80)
                    {
                        SendAssertion = 1;
                    }
                    /* set de/assertion occured */
                    /* remove WarningHigh going Low assertion */
                    AssertionEventOccuredByte1 &= ~0x40;
                    /* assert WarningHigh going High */
                    AssertionEventOccuredByte1 |= 0x80;
                    Level = SENSOR_STATUS_WARNING_HIGH;
                    /* Threshold that caused event */
                    EventData3 = WarningHigh;
                    //SensorLevel = UPPER_NON_CRITICAL_GOING_HIGH_CTRL_FN;
                }
                break;

                case SENSOR_STATUS_WARNING_HIGH:
                    DeassertThreshold = GetHighDeassertValue(SignedSensor, WarningHigh, PosHysteresis);
                    if(Value < DeassertThreshold)
                    {
                        IPMI_DBG_PRINT_3("#%02x UNC(D) - Value: %02x Threshold: %02x\n",
                            ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)DeassertThreshold);
                        /* deassert WarningHigh going High */
                        EvData1 = 0x57;
                        if (DeassertionEventEnablesByte1 & 0x80)
                        {
                            SendDeassertion = 1;
                        }
                        /* remove WarningHigh going Low deassertion */
                        DeassertionEventOccuredByte1 &= ~0x40;
                        /* deassert WarningHigh going High */
                        DeassertionEventOccuredByte1 |= 0x80;

                        /* assert WaningHigh going Low */
                        EventData1 = 0x56;
                        if (AssertionEventEnablesByte1 & 0x40)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_NORMAL;
                        EventData3 = WarningHigh;
                        /* remove WarningHigh going High assertion */
                        AssertionEventOccuredByte1 &= ~0x80;
                        /* assert WarningHigh going Low */
                        AssertionEventOccuredByte1 |= 0x40;
                      //  SensorLevel = UPPER_NON_CRITICAL_GOING_LOW_CTRL_FN;
                    }
                    if (Value >= CriticalHigh)
                    {
                        IPMI_DBG_PRINT_3("#%02x UC(A) - Value: %02x Threshold: %02x\n",
                            ((pSensorInfo->SensorOwernLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)CriticalHigh);
                        /* deassert CriticalHigh going Low */
                        EvData1 = 0x58;
                        if (DeassertionEventEnablesByte2 & 0x01)
                        {
                            SendDeassertion = 1;
                        }

                        /* remove CriticalHigh going High deassertion */
                        DeassertionEventOccuredByte2 &= ~0x02;
                        /* deassert CriticalHigh going Low */
                        DeassertionEventOccuredByte2 |= 0x01;
                        /*  assert CriticalHigh going High */
                        EventData1 = 0x59;
                        if (AssertionEventEnablesByte2 & 0x02)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_CRITICAL_HIGH;
                        EventData3 = CriticalHigh;
                        /* set de/assertion event occured */
                        /* remove CriticalHigh going Low assertion */
                        AssertionEventOccuredByte2 &= ~0x01;
                        /* assert CriticalHigh going High */
                        AssertionEventOccuredByte2 |= 0x02;
                        //SensorLevel = UPPER_CRITICAL_GOING_HIGH_CTRL_FN;
                    }
                    break;

                case SENSOR_STATUS_WARNING_LOW:
                    DeassertThreshold = GetLowDeassertValue(SignedSensor, WarningLow, NegHysteresis);
                    if (Value > DeassertThreshold)
                    {
                        IPMI_DBG_PRINT_3("#%02x LNC(D) - Value: %02x Threshold: %02x\n", 
                        ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8| pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)DeassertThreshold);
                        /* deassert WarningLow going Low */
                        EvData1 = 0x50;
                        if (DeassertionEventEnablesByte1 & 0x01)
                        {
                            SendDeassertion = 1;
                        }

                        /* remove WarningLow going High deassertion */
                        DeassertionEventOccuredByte1 &= ~0x02;
                        /* deassert WarningLow going Low */
                        DeassertionEventOccuredByte1 |= 0x01;

                        /* assert WarningLow going High */
                        EventData1 = 0x51;
                        if (AssertionEventEnablesByte1 & 0x02)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_NORMAL;
                        EventData3 = WarningLow;
                        /* set de/assertion event */
                        /* remove WarningLow going Low assertion */
                        AssertionEventOccuredByte1 &= ~0x01;
                        /* assert WarningLow going High */
                        AssertionEventOccuredByte1 |= 0x02;
                        //SensorLevel = LWR_NON_CRITICAL_GOING_HIGH_CTRL_FN;
                    }
                    if (Value <= CriticalLow)
                    {
                        IPMI_DBG_PRINT_3("#%02x LC(A) - Value: %02x Threshold: %02x\n",
                        ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)CriticalLow);
                        /* deassert CriticalLow going High */
                        EvData1 = 0x53;
                        if (DeassertionEventEnablesByte1 & 0x08)
                        {
                            SendDeassertion = 1;
                        }

                        /* remove CriticalLow going Low deassertion */
                        DeassertionEventOccuredByte1 &= ~0x04;
                        /* deassert CriticalHigh going High */
                        DeassertionEventOccuredByte1 |= 0x08;

                        /* assert CriticalLow going Low */
                        EventData1 = 0x52;
                        if (AssertionEventEnablesByte1 & 0x04)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_CRITICAL_LOW;
                        EventData3 = CriticalLow;
                        /* remove CriticalLow going High assertion */
                        AssertionEventOccuredByte1 &= ~0x08;
                        /* assert CriticalLow going Low */
                        AssertionEventOccuredByte1 |= 0x04;
                        //SensorLevel = LWR_CRITICAL_GOING_LOW_CTRL_FN;
                    }
                    break;

                case SENSOR_STATUS_CRITICAL_HIGH:
                    DeassertThreshold = GetHighDeassertValue(SignedSensor, CriticalHigh, PosHysteresis);
                    if (Value < DeassertThreshold)
                    {
                        IPMI_DBG_PRINT_3("#%02x UC(D) - Value: %02x Threshold: %02x\n",
                        ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)DeassertThreshold);
                        /* deassert CriticalHigh going High */
                        EvData1 = 0x59;
                        if (DeassertionEventEnablesByte2 & 0x02)
                        {
                            SendDeassertion = 1;
                        }

                        /* remove CriticalHigh going Low deassertion */
                        DeassertionEventOccuredByte2 &= ~0x01;
                        /* deassert CriticalHigh going High */
                        DeassertionEventOccuredByte2 |= 0x02;

                        /* assert CriticalHigh going Low */
                        EventData1 = 0x58;
                        if (AssertionEventEnablesByte2 & 0x01)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_WARNING_HIGH;
                        EventData3 = CriticalHigh;
                        /* set de/assertion event occured */
                        /* remove CriticalHigh going High assertion */
                        AssertionEventOccuredByte2 &= ~0x02;
                        /* assert CriticalHigh going Low */
                        AssertionEventOccuredByte2 |= 0x01;
                        //SensorLevel = UPPER_CRITICAL_GOING_LOW_CTRL_FN;
                    }
                    if (Value >= NonRecoverableHigh)
                    {
                        IPMI_DBG_PRINT_3("#%02x UNR(A) - Value: %02x Threshold: %02x\n",
                        ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)NonRecoverableHigh);
                        /*  deassert NonRecoverableHigh going Low */
                        EvData1 = 0x5A;
                        if (DeassertionEventEnablesByte2 & 0x04)
                        {
                            SendDeassertion = 1;
                        }
                        /* remove NonRecoverableHigh going High deassertion */
                        DeassertionEventOccuredByte2 &= ~0x08;
                        /* deassert NonRecoverableHigh going Low */
                        DeassertionEventOccuredByte2 |= 0x04;

                        /* assert NonRecoverableHigh going High */
                        EventData1 = 0x5B;
                        if (AssertionEventEnablesByte2 & 0x08)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_NONRECOVERABLE_HIGH;
                        EventData3 = NonRecoverableHigh;
                        /* remove NonRecoverableHigh going Low assertion */
                        AssertionEventOccuredByte2 &= ~0x04;
                        /* assert NonRecoverableHigh going High */
                        AssertionEventOccuredByte2 |= 0x08;
                        //SensorLevel = UPPER_NON_RECOVERABLE_GOING_HIGH_CTRL_FN;
                    }
                    break;

                case SENSOR_STATUS_CRITICAL_LOW:
                    DeassertThreshold = GetLowDeassertValue(SignedSensor, CriticalLow, NegHysteresis);
                    if (Value > DeassertThreshold)
                    {
                        IPMI_DBG_PRINT_3("#%02x LC(D) - Value: %02x Threshold: %02x\n",
                        ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)DeassertThreshold);
                        /* deassert CriticalLow going Low */
                        EvData1 = 0x52;
                        if (DeassertionEventEnablesByte1 & 0x04)
                        {
                            SendDeassertion = 1;
                        }

                        /* remove CriticalLow going High deassertion */
                        DeassertionEventOccuredByte1 &= ~0x08;
                        /* deassert CriticalLow going Low */
                        DeassertionEventOccuredByte1 |= 0x04;

                        /* assert CriticalLow going High */
                        EventData1 = 0x53;
                        if (AssertionEventEnablesByte1 & 0x08)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_WARNING_LOW;
                        EventData3 = CriticalLow;
                        /* remove CriticalLow going Low assertion */
                        AssertionEventOccuredByte1 &= ~0x04;
                        /* assert CriticalLow going High */
                        AssertionEventOccuredByte1 |= 0x08;
                        //SensorLevel = LWR_CRITICAL_GOING_HIGH_CTRL_FN;
                    }
                    if (Value <= NonRecoverableLow)
                    {
                        IPMI_DBG_PRINT_3("#%02x LNR(A) - Value: %02x Threshold: %02x\n",
                        ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)NonRecoverableLow);
                        /* deassert NonRecoverableLow going High */
                        EvData1 = 0x55;
                        if (DeassertionEventEnablesByte1 & 0x20)
                        {
                            SendDeassertion = 1;
                        }

                        /* remove NonRecoverableLow going Low deassertion */
                        DeassertionEventOccuredByte1 &= ~0x10;
                        /* deassert NonRecoverableLow going High */
                        DeassertionEventOccuredByte1 |= 0x20;

                        /* assert NonRecoverableLow going Low */
                        EventData1 = 0x54;
                        if (AssertionEventEnablesByte1 & 0x10)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_NONRECOVERABLE_LOW;
                        EventData3 = NonRecoverableLow;
                        /* remove NonRecoverableLow going High assertion */
                        AssertionEventOccuredByte1 &= ~0x20;
                        /* assert NonRecoverableLow going Low */
                        AssertionEventOccuredByte1 |= 0x10;
                        //SensorLevel = LWR_NON_RECOVERABLE_GOING_LOW_CTRL_FN;
                    }
                    break;

                case SENSOR_STATUS_NONRECOVERABLE_HIGH:
                    DeassertThreshold = GetHighDeassertValue(SignedSensor, NonRecoverableHigh, PosHysteresis);
                    if (Value < DeassertThreshold)
                    {
                        IPMI_DBG_PRINT_3("#%02x UNR(D) - Value: %02x Threshold: %02x\n",
                        ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)DeassertThreshold);
                        /* deassert NonRecoverableHigh going High */
                        EvData1 = 0x5B;
                        if (DeassertionEventEnablesByte2 & 0x08)
                        {
                            SendDeassertion = 1;
                        }

                        /* remove NonRecoverableHigh going Low deassertion */
                        DeassertionEventOccuredByte2 &= ~0x04;
                        /* deassert NonRecoverableHigh going High */
                        DeassertionEventOccuredByte2 |= 0x08;

                        /* assert NonRecoverableHigh going Low */
                        EventData1 = 0x5A;
                        if (AssertionEventEnablesByte2 & 0x04)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_CRITICAL_HIGH;
                        EventData3 = NonRecoverableHigh;
                        /* remove NonRecoverableHigh going High assertion */
                        AssertionEventOccuredByte2 &= ~0x08;
                        /* assert NonRecoverableHigh going Low */
                        AssertionEventOccuredByte2 |= 0x04;
                        //SensorLevel = UPPER_NON_RECOVERABLE_GOING_LOW_CTRL_FN;
                    }
                    break;

                case SENSOR_STATUS_NONRECOVERABLE_LOW:
                    DeassertThreshold = GetLowDeassertValue(SignedSensor, NonRecoverableLow, NegHysteresis);
                    if (Value > DeassertThreshold)
                    {
                        IPMI_DBG_PRINT_3("#%02x LNR(D) - Value: %02x Threshold: %02x\n",
                        ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), (INT8U)Value, (INT8U)DeassertThreshold);
                        /* deassert NonRecoverableLow going Low */
                        EvData1 = 0x54;
                        if (DeassertionEventEnablesByte1 & 0x10)
                        {
                            SendDeassertion = 1;
                        }

                        /* remove NonRecoverableLow going High deassertion */
                        DeassertionEventOccuredByte1 &= ~0x20;
                        /* deassert NonRecoverableLow going Low */
                        DeassertionEventOccuredByte1 |= 0x10;

                        /* assert NonRecoverableLow going High */
                        EventData1 = 0x55;
                        if (AssertionEventEnablesByte1 & 0x20)
                        {
                            SendAssertion = 1;
                        }
                        Level = SENSOR_STATUS_CRITICAL_LOW;
                        EventData3 = NonRecoverableLow;
                        /* remove NonRecoverableLow going Low assertion */
                        AssertionEventOccuredByte1 &= ~0x10;
                        /* assert NonRecoverableLow going High */
                        AssertionEventOccuredByte1 |= 0x20;
                        //SensorLevel = LWR_NON_RECOVERABLE_GOING_HIGH_CTRL_FN;
                    }
                    break;

                case SENSOR_STATUS_FATAL:
                    break;

        } /* switch(m_SensorEventLevel[i]) */

        pSensorInfo->EventLevel = Level;
        pSensorInfo->AssertionEventOccuredByte1 = AssertionEventOccuredByte1;
        pSensorInfo->AssertionEventOccuredByte2 = AssertionEventOccuredByte2;
        pSensorInfo->DeassertionEventOccuredByte1 = DeassertionEventOccuredByte1;
        pSensorInfo->DeassertionEventOccuredByte2 = DeassertionEventOccuredByte2;

        AssertionHistoryByte1   = 0;
        AssertionHistoryByte2   = 0;
        DeassertionHistoryByte1 = 0;
        DeassertionHistoryByte2 = 0;
        EventMsg [0] = 0x20;
        EventMsg [1] = (pSensorInfo->SensorOwnerLun & 0x03);	/* Multi-LUN support */
        /* EvMRev */
        EventMsg [2] = IPMI_EVM_REVISION;
        /* sensor type */
        EventMsg [3] = pSensorInfo->SensorTypeCode;
        /* sensor number */
        EventMsg [4] = pSensorInfo->SensorNumber;

        /* event direction|type */
        EventMsg [5] = 0x01;

        EventMsg [6] = 0;	

        /* Set Event Data only if it is a settable sensor */
        if (0 == GET_SETTABLE_SENSOR_BIT(pSensorInfo->SensorInit))
        {
            /* Set Sensor Event Data based on the Operation byte */
            switch (GET_EVENT_DATA_OP(pSensorInfo->Operation))
            {
                case WRITE_NO_EVTDATA1:
                    /* Event Data 1 */
                    EventMsg [6] =  pSensorInfo->EvtData1;
                    /* Intentional Fall thru */
                    case WRITE_EVTDATA1:
                    /* Event Data 1 */
                    EventMsg [6] |= EventData1;
                    /* Update EvtData fields */
                    /* Current Reading */
                    EventMsg [7] = pSensorInfo->EvtData2;
                    /* Trigger that caused event */
                    EventMsg [8] = pSensorInfo->EvtData3;
                    break;

                case USE_SM_EVTDATA:
                    /* Event Data 1 */
                    EventMsg [6] = EventData1;
                    /* Current Reading */
                    EventMsg [7] = Value;
                    /* Trigger that caused event */
                    EventMsg [8] = EventData3;
                    break;
            }
        }
        else
        {
            /* Event Data 1 */
            EventMsg [6] = EventData1;
            /* Current Temperature */
            EventMsg [7] = Value;
            /* Trigger that caused event */
            EventMsg [8] = EventData3;
        }

        if (SendAssertion)
        {
            if ((EventData1 & 0x0f) < 8)    
            {
                AssertionHistoryByte1 = (1 << (EventData1 & 0x0f));
            }
            else
            {
                AssertionHistoryByte2 = (1 << ((EventData1 & 0x0f) - 8));
            }

            /* For Manual Rearm Sensor - Check for already generated events in the History bytes 
            For Auto Rearm Sensor -History bytes are not valid */
            if ( (0 == (AssertionHistoryByte1 & pSensorInfo->AssertionHistoryByte1)) &&
                (0 == (AssertionHistoryByte2 & pSensorInfo->AssertionHistoryByte2)) )
            {
                /* if sensor is manual arming */
                if (0 == (pSensorInfo->SensorCaps & BIT6))
                {
                    /* Update assertion History */
                    pSensorInfo->AssertionHistoryByte1 |= AssertionHistoryByte1;
                    pSensorInfo->AssertionHistoryByte2 |= AssertionHistoryByte2;
                }
                /* Call PreEventLog hook function(s) and don't do
                sensor SEL event log if the return is not zero */
                if (0 == PreEventLog(pSensorInfo, EventMsg,BMCInst))
                {
                    /* Is event message generation enabled ? */
                    if (0 != (pSensorInfo->EventFlags & BIT7))
                    {
                        /* Post Event Message */
                        if (0 != PostEventMessage (EventMsg,FALSE, sizeof (EventMsg),BMCInst))
                        {
                            IPMI_DBG_PRINT ("Event Generation Failed\n");
                        }
                        else
                        {
                            IPMI_DBG_PRINT ("Generating Event\n");
                        }
                    }
                }

            }
            SendAssertion = 0;
        }
        
        if (SendDeassertion)
        {
            if ((EvData1 & 0x0f) < 8)    
            {
                DeassertionHistoryByte1 = (1 << (EvData1 & 0x0f));
            }
            else
            {
                DeassertionHistoryByte2 = (1 << ((EvData1 & 0x0f) - 8));
            }

            /* For Manual Rearm Sensor - Check for already generated events in the History bytes 
            For Auto Rearm Sensor -History bytes are not valid */
            if ( (0 == (DeassertionHistoryByte1 & pSensorInfo->DeassertionHistoryByte1)) &&
            (0 == (DeassertionHistoryByte2 & pSensorInfo->DeassertionHistoryByte2)) )
            {

                /* Event Data 1 */
                EventMsg [6] = EvData1;
                EventMsg [5] = 0x81;

                /* if sensor is manual arming */
                if (0 == (pSensorInfo->SensorCaps & BIT6))
                {
                    /* Update Deassertion History */
                    pSensorInfo->DeassertionHistoryByte1 |= DeassertionHistoryByte1;
                    pSensorInfo->DeassertionHistoryByte2 |= DeassertionHistoryByte2;
                }

                /* Is event message generation enabled ? */
                if (0 == PreEventLog(pSensorInfo, EventMsg,BMCInst))
                {
                    /* Call PreEventLog hook function(s) and don't do
                    sensor SEL event log if the return is not zero */
                    if (0 != (pSensorInfo->EventFlags & BIT7))
                    {
                        /* Post Event Message Here */
                        if (0 != PostEventMessage (EventMsg,FALSE, sizeof (EventMsg),BMCInst))
                        {
                            IPMI_DBG_PRINT ("Event Generation Failed\n");
                        }
                        else
                        {
                            IPMI_DBG_PRINT ("Generating Event\n");
                        }
                    }
                }
            }
            SendDeassertion = 0;
        }
    }
    while (StartLevel != Level);

#if (TERMINAL_MODE_SUPPORT == 1)
    //
    // Update the current Health state
    // We do this by examining the currently pending Assertions
    // and assigning the appropriate health state.
    //
    if (0 == (pSensorInfo->SensorCaps & BIT6))
    {
        /* Manual Rearm sensor: Get assertions from history */
        AssertionHistoryByte1 = pSensorInfo->AssertionHistoryByte1;
        AssertionHistoryByte2 = pSensorInfo->AssertionHistoryByte2;
    }
    else
    {
        /* Auto Rearm sensor: Get current assertions */
        AssertionHistoryByte1 = pSensorInfo->AssertionEventOccuredByte1;
        AssertionHistoryByte2 = pSensorInfo->AssertionEventOccuredByte2;
    }

    // The following checks need to be done from lowest severity to highest.
    HealthLevel = SENSOR_STATUS_NORMAL;
    if (0 != (AssertionHistoryByte1 & 0x01 & AssertionEventEnablesByte1))
        HealthLevel = SENSOR_STATUS_WARNING_LOW;
    if (0 != (AssertionHistoryByte1 & 0x80 & AssertionEventEnablesByte1))
        HealthLevel = SENSOR_STATUS_WARNING_HIGH;
    if (0 != (AssertionHistoryByte1 & 0x04 & AssertionEventEnablesByte1))
        HealthLevel = SENSOR_STATUS_CRITICAL_LOW;
    if (0 != (AssertionHistoryByte2 & 0x02 & AssertionEventEnablesByte2))
        HealthLevel = SENSOR_STATUS_CRITICAL_HIGH;
    if (0 != (AssertionHistoryByte1 & 0x10 & AssertionEventEnablesByte1))
        HealthLevel = SENSOR_STATUS_NONRECOVERABLE_LOW;
    if (0 != (AssertionHistoryByte2 & 0x08 & AssertionEventEnablesByte2))
        HealthLevel = SENSOR_STATUS_NONRECOVERABLE_HIGH;

    pSensorInfo->HealthLevel = HealthLevel;
    
    UpdateThresHealthState (pSensorInfo, HealthLevel,BMCInst);
#endif
    if(g_corefeatures.ssi_support == ENABLED)
    {
        /* Aggregate specified sensor readings to represent the overall health of blade. */
        if (g_PDKHandle[PDK_AGGREGATEFAULT] != NULL)
        {
            ((void(*)(SensorInfo_T*, int))g_PDKHandle[PDK_AGGREGATEFAULT])(pSensorInfo, BMCInst);
        }
    }

    /* Post Monitor hook to override clearing the "Reading Not Available" Hook */
    Override = 0;
    if (g_PDKHandle[PDK_OVERRIDE_CLR_READING_NA] != NULL)
    {
        Override = ((int (*)(SensorInfo_T*, int))g_PDKHandle[PDK_OVERRIDE_CLR_READING_NA])(pSensorInfo, BMCInst);
        if (Override == -1)
            return;
    }

    // Make sure the "Reading Not Available" bit is clear.
    pSensorInfo->EventFlags &= ~BIT5;

}


/**
 * @brief Monitor Non-threshold sensors.
 * @param i - Sensor index.
*/
static void
MonitorNonTSensors (SensorInfo_T*  pSensorInfo,int BMCInst)
{
    INT16U                      Event, OffsetBit;
    INT8U                       EventOffset;
    //INT8U                       SendDeassertion;
    //INT8U                       SendAssertion;
    INT8U                       EventMsg [EVENT_MSG_LENGTH];
    INT8U 		 	PSGood = 0;
    GetSensorEventEnableRes_T   SenEventEnable;
    SensorInfo_T*				pSharedSensorInfo = NULL;
    INT8U                   AssertionHistoryByte1;
    INT8U                   AssertionHistoryByte2;
    INT8U                   DeassertionHistoryByte1;
    INT8U                   DeassertionHistoryByte2;
    INT16U                  OrgSensorValue=0;
    INT16U                  PreviousState = pSensorInfo->PreviousState;
    INT16U                  SensorReading = 0;
    INT8U ReadFlags=0;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    int Override = 0;

        /* Get Sensor Reading from SP only if it not a settable sensor */
        if (0 == GET_SETTABLE_SENSOR_BIT(pSensorInfo->SensorInit))
        {
            /* get the power state of host */		
            if(g_PDKHandle[PDK_GETPSGOOD] != NULL)
            {
                PSGood = ((int(*)(int))g_PDKHandle[PDK_GETPSGOOD]) (BMCInst);
            }

            /* If sensor is not need to monitor on stand by power, and if Server */
            /* is power off then return setting error "sensor not acceble at present state" */
            if  (( 0 == (pSensorInfo->SensorState & 0x01)) && (0 == PSGood )) 
            {
                pSensorInfo->Err = CC_PARAM_NOT_SUP_IN_CUR_STATE ; 
                pSensorInfo->EventFlags |= BIT5;
                return;
            }
            else
	    {
	    	if (g_PDKHandle[PDK_SENSOR_OVERRIDE] != NULL)
		{
	            // If the hook is there, then call the hook.
	            Override = ((int (*)(INT8U, INT8U, INT16U *, int)) g_PDKHandle[PDK_SENSOR_OVERRIDE])(pSensorInfo->SensorNumber, pSensorInfo->SensorOwnerLun, &pSensorInfo->SensorReading, BMCInst);
		}

	        if ((Override == 0) || (g_PDKHandle[PDK_SENSOR_OVERRIDE] == NULL))
	        {
                    FullSensorRec_T*      sfs=NULL;
                    CompactSensorRec_T*   scs=NULL;

                    OrgSensorValue = pSensorInfo->SensorReading;
                    /* Call PreMonitorSensor and skip default sensor reading if it returns -1 or non zero */
                    if (0 == pSensorInfo->pPreMonitor(pSensorInfo,&ReadFlags,BMCInst))
                    {
                        if (g_corefeatures.cached_sensor_reading == ENABLED)
                        {
                            OS_THREAD_MUTEX_RELEASE(&g_BMCInfo[BMCInst].SensorSharedMemMutex);
                        }
                        
			if (g_corefeatures.node_manager == ENABLED)
                        {
                            if (FULL_SDR_REC==pSensorInfo->SDRRec->Type)
                    	    {
                            	sfs = (FullSensorRec_T*)pSensorInfo->SDRRec;
                        	if (sfs->OwnerID != pBMCInfo->NMConfig.NMDevSlaveAddress)
                        	{
				                    pSensorInfo->Err  = IPMI_HAL_GET_SENSOR_READING ((((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8) | pSensorInfo->SensorNumber),  &SensorReading,BMCInst);	/* Multi-LUN support */
				    
    							    if(HAL_ERR_SKIP_SENSOR !=  pSensorInfo->Err )       /*  Skip the sensors whose reading is fetched through ME*/
    							    {
                                        if (pBMCInfo->IpmiConfig.OPMASupport == 1)
                                        {
                                            if (g_corefeatures.cached_sensor_reading == ENABLED)
                                            {
                                                OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
                                                pSensorInfo->SensorReading = SensorReading + pSensorInfo->SenReadingOffset;
                                    	        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
                                	        }
                                	        else
                                	        {
                                        	    pSensorInfo->SensorReading = SensorReading + pSensorInfo->SenReadingOffset;
                                    	    }
                                	    }
                                	    else
                                	    {
                                        	if (g_corefeatures.cached_sensor_reading == ENABLED)
                                    	    {
                                        	    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
                                                pSensorInfo->SensorReading = SensorReading;
                                        	    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
                                	        }
                                	        else
                                	        {
                                    	        pSensorInfo->SensorReading = SensorReading;
                                	        }
                                	    }
                            	        IPMI_DBG_PRINT("owner id ==BMC, keep read \n");
    							    }
                        	}
                        	else
                        	{
                            	    IPMI_DBG_PRINT("owner id ==NM, skip read \n");
                        	}
                    	    }
                    	    else if (COMPACT_SDR_REC==pSensorInfo->SDRRec->Type)
                    	    {
                        	scs = (CompactSensorRec_T*)pSensorInfo->SDRRec;
                        	if (scs->OwnerID != pBMCInfo->NMConfig.NMDevSlaveAddress)
                        	{
                                pSensorInfo->Err  = IPMI_HAL_GET_SENSOR_READING ((((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8) | pSensorInfo->SensorNumber),  &SensorReading,BMCInst);	/* Multi-LUN support */
                                if(HAL_ERR_SKIP_SENSOR !=  pSensorInfo->Err )/* Skip the sensors whose reading is fetched through ME*/
                                {

                            	    if (pBMCInfo->IpmiConfig.OPMASupport == 1)
                            	    {
                                   	    if (g_corefeatures.cached_sensor_reading == ENABLED)
                                	    {
                                   	        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
                                    	    pSensorInfo->SensorReading = SensorReading + pSensorInfo->SenReadingOffset;
                                    	    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
                                	    }
                                	    else
                                	    {
                                    	    pSensorInfo->SensorReading = SensorReading + pSensorInfo->SenReadingOffset;
                                	    }
                            	    }
                            	    else
                            	    {
                                	    if (g_corefeatures.cached_sensor_reading == ENABLED)
                                	    {
                                    	    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
                                    	    pSensorInfo->SensorReading = SensorReading;
                                    	    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
                                	    }
                                	    else
                                	    {
                                    	    pSensorInfo->SensorReading = SensorReading;
                                	    }
                            	    }
                                }
                        	}
                    	    }
                  	}
                  	else
                  	{
                    	    /* read the sensor using softprocessor */
                    	    pSensorInfo->Err  = IPMI_HAL_GET_SENSOR_READING ((((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8) | pSensorInfo->SensorNumber),  &SensorReading,BMCInst);	/* Multi-LUN support */
                    	    if (pBMCInfo->IpmiConfig.OPMASupport == 1)
                    	    {
                        	if (g_corefeatures.cached_sensor_reading == ENABLED)
                        	{
                            	    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
                            	    pSensorInfo->SensorReading = SensorReading + pSensorInfo->SenReadingOffset;   
                            	    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
                        	}
                        	else
                        	{
                            	    pSensorInfo->SensorReading = SensorReading + pSensorInfo->SenReadingOffset;
                        	}
                    	    }
                    	    else
                    	    {
                        	if (g_corefeatures.cached_sensor_reading == ENABLED)
                        	{
                            	    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
                            	    pSensorInfo->SensorReading = SensorReading;
                            	    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
                        	}
                        	else
                        	{
                            	    pSensorInfo->SensorReading = SensorReading;
                        	}
                    	    }
                  	}

                    	if (g_corefeatures.cached_sensor_reading == ENABLED)
                    	{
                            OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo[BMCInst].SensorSharedMemMutex,WAIT_INFINITE);
                    	}

                    	if ( 0 == pSensorInfo->Err )
                    	{
                            // Get the power state again and check
                            if (g_PDKHandle[PDK_GETPSGOOD] != NULL)
                            {
                            	PSGood = ((int(*)(int))g_PDKHandle[PDK_GETPSGOOD]) (BMCInst);
                            }
    
                            /* check to see the power state of the server to confirm its still in power good state */
                            if  (( 0 == (pSensorInfo->SensorState & 0x01) ) && ( 0 == PSGood ))
                            {
                                pSensorInfo->Err = CC_PARAM_NOT_SUP_IN_CUR_STATE ;
                                pSensorInfo->pPostMonitor (pSensorInfo,&ReadFlags,BMCInst);
                                pSharedSensorInfo = API_GetSensorInfo (pSensorInfo->SensorNumber, pSensorInfo->SensorOwnerLun, BMCInst);	
                                pSensorInfo->PreviousState 	=  pSharedSensorInfo->PreviousState;						 
                                pSensorInfo->EventFlags |= BIT5;
                                return;
                            }
                        }
                    }
                    else
                    {
                        // get the power state again 
                        if (g_PDKHandle[PDK_GETPSGOOD] != NULL)
                        {
                            PSGood = ((int(*)(int))g_PDKHandle[PDK_GETPSGOOD]) (BMCInst);
                        }
    
                        /* check to see the power state of the server to confirm its still in power good state */
                        if  (( 0 == (pSensorInfo->SensorState & 0x01) ) && (0 == PSGood ))
                        {
                            // Response cannot be provided in this state
                            pSensorInfo->Err = CC_PARAM_NOT_SUP_IN_CUR_STATE;
                            pSensorInfo->pPostMonitor (pSensorInfo,&ReadFlags,BMCInst);
                            pSensorInfo->SensorReading = OrgSensorValue; // Restore reading
                            pSensorInfo->EventFlags |= BIT5;
                            return;
                        }
                    }
                }
            }
	}

    /* Call PostMonitorSensor and dont monitor the sensor if it returns -1*/
    if (0 != pSensorInfo->pPostMonitor (pSensorInfo,&ReadFlags,BMCInst))
    {
        if (0 == pSensorInfo->Err)
        {
            pSensorInfo->EventFlags &= ~BIT5;
        }
        else
        {
            pSensorInfo->EventFlags |= BIT5;
        }
        return;
    }
    /* Could be possible that the Sensor callback function could have updated
    * the previous state */   	 
    pSensorInfo->PreviousState 	=  PreviousState;


    /* if Err != 0 && Err != HAL_ERR_SKIP_SENSOR,  do not monitor the sensor   */
    if((0 != pSensorInfo->Err) && (HAL_ERR_SKIP_SENSOR !=  pSensorInfo->Err ))
    {
        pSensorInfo->EventFlags |= BIT5;
        return ;
    }
    
#if 0
    IPMI_DBG_PRINT_3 ("SensorNo = %x     Reading = %x %x\n", \
                      pSensorInfo->SensorNumber, \
                      pSensorInfo->SensorReading, \
                      pSensorInfo->PreviousState);
#endif
    /* Get SensorEventEnable Values */
    SenEventEnable.AssertionMask    = (pSensorInfo->AssertionEventEnablesByte2 << 8);
    SenEventEnable.AssertionMask    |= (pSensorInfo->AssertionEventEnablesByte1);
    SenEventEnable.DeAssertionMask  = (pSensorInfo->DeassertionEventEnablesByte2 << 8);
    SenEventEnable.DeAssertionMask  |= (pSensorInfo->DeassertionEventEnablesByte1);

    /* if there is no State change  */
    if (0 == (Event = pSensorInfo->SensorReading ^ pSensorInfo->PreviousState))
    {
        /* Post Monitor hook to override clearing the "Reading Not Available" Hook */
        Override = 0;
        if (g_PDKHandle[PDK_OVERRIDE_CLR_READING_NA] != NULL)
        {
            Override = ((int (*)(SensorInfo_T*, int))g_PDKHandle[PDK_OVERRIDE_CLR_READING_NA])(pSensorInfo, BMCInst);
            if (Override == -1)
                return;
        }

        // Make sure the "Reading Not Available" bit is clear.
        pSensorInfo->EventFlags &= ~BIT5;
        
        return;
    }

    EventMsg [0] = 0x20;
    EventMsg [1] = (pSensorInfo->SensorOwnerLun & VALID_LUN);	/* Multi-LUN support */
    EventMsg [2] = IPMI_EVM_REVISION;

    OffsetBit = 1;
    /* check for the event and take actions */
    for (EventOffset = 0; EventOffset < 16; EventOffset++)
    {

        if (!(OffsetBit & Event))
        {
            OffsetBit = OffsetBit << 1;
            continue;
        }

        //SendAssertion   = 0;
        //SendDeassertion = 0;

        AssertionHistoryByte1 = 0;
        DeassertionHistoryByte1 = 0;
        AssertionHistoryByte2 = 0;
        DeassertionHistoryByte2 = 0;

        EventMsg [3] = pSensorInfo->SensorTypeCode;
        EventMsg [4] = pSensorInfo->SensorNumber;
        EventMsg [5] = pSensorInfo->EventTypeCode;
        EventMsg [6] = EventOffset;
        EventMsg [7] = 0xFF;
        EventMsg [8] = 0xFF;

        /* is Assertion event? */
        if ((pSensorInfo->SensorReading >> EventOffset) & 0x01)
        {
            if (EventOffset < 8)
            {
                pSensorInfo->AssertionEventOccuredByte1 |=(INT8U) (1 << EventOffset);
                pSensorInfo->DeassertionEventOccuredByte1 &= ~(INT8U) (1 << EventOffset);
            }else
            {
                pSensorInfo->AssertionEventOccuredByte2 |=  (INT8U) (1 << (EventOffset - 8));
                pSensorInfo->DeassertionEventOccuredByte2 &= ~ (INT8U) (1 << (EventOffset - 8));
            }

            if (OffsetBit & SenEventEnable.AssertionMask)
            {
                /* If assertion is caused by offset 0-7 */
                if (EventOffset < 8)
                {
                    /* Track it's history in AssertionHistoryByte1 */
                    AssertionHistoryByte1   = (INT8U) (1 << EventOffset);
                }
                else /* Else assertion is caused by offsets 8-15 */
                {
                    /* Track it's history in AssertionHistoryByte2 */
                    AssertionHistoryByte2   = (INT8U) (1 << (EventOffset - 8));
                }

                /* For Manual Rearm Sensor - Check for already generated events in the History bytes
                For Auto Rearm Sensor -History bytes are not valid */
                if ( (0 == (AssertionHistoryByte1 & pSensorInfo->AssertionHistoryByte1)) &&
                (0 == (AssertionHistoryByte2 & pSensorInfo->AssertionHistoryByte2)) )
                {
                    /* If sensor is manual arming */
                    if (0 == (pSensorInfo->SensorCaps & BIT6))
                    {
                        /* Update assertion history variables appropriately (assertion will not occur again until rearmed) */
                        pSensorInfo->AssertionHistoryByte1 |= AssertionHistoryByte1;
                        pSensorInfo->AssertionHistoryByte2 |= AssertionHistoryByte2;
                    }

                    IPMI_DBG_PRINT_3 ("Sensor #%02d Assertion (Type: 0x%02X, Offset: 0x%02X) - ", \
                    ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), pSensorInfo->SensorTypeCode, EventOffset);

                    /* Event direction|type = assertion */
                    EventMsg [5] &= ~BIT7;

                    /* Call PreEventLog hook function(s) and don't do
                    sensor SEL event log if the return is not zero */
                    if (0 == PreEventLog(pSensorInfo, EventMsg,BMCInst))
                    {
                    /* Is event message generation enabled ? */
                    if (0 != (pSensorInfo->EventFlags & BIT7))
                    {
                        /* Post Event Message */
                        if (0 != PostEventMessage (EventMsg,FALSE,sizeof (EventMsg),BMCInst))
                        {
                            IPMI_DBG_PRINT ("Event Generation Failed\n");
                        }
                        else
                        {
                            IPMI_DBG_PRINT ("Generating Event\n");
                        }
                    }
                    }
                }
            }
        }
        /* is Deassertion event? */
        if(!((pSensorInfo->SensorReading >> EventOffset) & 0x01))
        {
            if (EventOffset < 8)
            {
                pSensorInfo->AssertionEventOccuredByte1 &= ~(INT8U) (1 << EventOffset);
                pSensorInfo->DeassertionEventOccuredByte1 |= (INT8U) (1 << EventOffset);
            }else
            {
                pSensorInfo->AssertionEventOccuredByte2 &= ~ (INT8U) (1 << (EventOffset - 8));
                pSensorInfo->DeassertionEventOccuredByte2 |= (INT8U) (1 << (EventOffset - 8));
            }

            if(OffsetBit & SenEventEnable.DeAssertionMask )
            {
                /* If deassertion is caused by offset 0-7 */
                if (EventOffset < 8)
                {
                    /* Track it's history in DeassertionHistoryByte1 */
                    DeassertionHistoryByte1 = (INT8U) (1 << EventOffset);
                }
                else /* Else deassertion is caused by offsets 8-15 */
                {
                    /* Track it's history in DeassertionHistoryByte2 */
                    DeassertionHistoryByte2 = (INT8U) (1 << (EventOffset - 8));
                    }

                /* For Manual Rearm Sensor - Check for already generated events in the History bytes
                For Auto Rearm Sensor -History bytes are not valid */
                if ( (0 == (DeassertionHistoryByte1 & pSensorInfo->DeassertionHistoryByte1)) &&
                (0 == (DeassertionHistoryByte2 & pSensorInfo->DeassertionHistoryByte2)) )
                {

                    IPMI_DBG_PRINT_3 ("Sensor #%02d Deassertion (Type: 0x%02X, Offset: 0x%02X) - ", \
                    ((pSensorInfo->SensorOwnerLun & VALID_LUN) << 8 | pSensorInfo->SensorNumber), pSensorInfo->SensorTypeCode, EventOffset);

                    /* Event direction|type = deassertion */
                    EventMsg [5] |= BIT7;

                    /* If sensor is manual arming */
                    if (0 == (pSensorInfo->SensorCaps & BIT6))
                    {
                        /* Update deassertion history variables appropriately (deassertion will not occur again until rearmed) */
                        pSensorInfo->DeassertionHistoryByte1 |= DeassertionHistoryByte1;
                        pSensorInfo->DeassertionHistoryByte2 |= DeassertionHistoryByte2;
                    }

                    /* Call PreEventLog hook function(s) and don't do
                    sensor SEL event log if the return is not zero */
                    if (0 == PreEventLog(pSensorInfo, EventMsg,BMCInst))
                    {
                        /* Is event message generation enabled ? */
                        if (0 != (pSensorInfo->EventFlags & BIT7))
                        {
                            /* Post Event Message */
                            if (0 != PostEventMessage (EventMsg,FALSE,sizeof (EventMsg),BMCInst))
                            {
                                IPMI_DBG_PRINT ("Event Generation Failed\n");
                            }
                            else
                            {
                                IPMI_DBG_PRINT ("Generating Event\n");
                            }
                        }
                    }
                }
            }
        }
        OffsetBit = OffsetBit << 1;
    }
    pSensorInfo->PreviousState = pSensorInfo->SensorReading;

    /* Post Monitor hook to override clearing the "Reading Not Available" Hook */
    Override = 0;
    if (g_PDKHandle[PDK_OVERRIDE_CLR_READING_NA] != NULL)
    {
        Override = ((int (*)(SensorInfo_T*, int))g_PDKHandle[PDK_OVERRIDE_CLR_READING_NA])(pSensorInfo, BMCInst);
        if (Override == -1)
            return;
    }

    // Make sure the "Reading Not Available" bit is clear.
    pSensorInfo->EventFlags &= ~BIT5;
    
}


#if (TERMINAL_MODE_SUPPORT == 1)

/*---------------------------------------------------
 *UpdateSysHealthState
**---------------------------------------------------*/
static void
UpdateThresHealthState (SensorInfo_T*     pSensorInfo, INT8U Level, int BMCInst)
{
    INT8U Status;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    /* Calculate Status */
    Status = (Level + 1) / 2;
    if (Status > HEALTH_STATUS_NR)
    {
        Status = HEALTH_STATUS_UN;
    }

    /* Get Individual Status */
    switch (pSensorInfo->SensorTypeCode)
    {
    case TEMP_SENSOR_TYPE:

        if (Status > pBMCInfo->SenConfig.HealthState.TemperatureSys)
        {
            pBMCInfo->SenConfig.HealthState.TemperatureSys = Status;
        }
        break;

    case VOLT_SENSOR_TYPE:

        if (Status > pBMCInfo->SenConfig.HealthState.Volatge)
        {
            pBMCInfo->SenConfig.HealthState.Volatge = Status;
        }
        break;

    case FAN_SENSOR_TYPE:

        if (Status > pBMCInfo->SenConfig.HealthState.CoolingSys)
        {
            pBMCInfo->SenConfig.HealthState.CoolingSys = Status;
        }
        break;
    case POWER_SENSOR_TYPE:
    case CURRENT_SENSOR_TYPE:

        if (Status > pBMCInfo->SenConfig.HealthState.PowerSubSys)
        {
            pBMCInfo->SenConfig.HealthState.PowerSubSys = Status;
        }
        break;
    case MONITOR_ASIC_IC_SENSOR_TYPE:
        break;
    case OEM_SENSOR_TYPE:
        break;
    case OTHER_UNITS_SENSOR_TYPE:
        //
        // For the Other Units type we need to check the Units2
        // value to determine which subsystem it belongs to.
        // Refer to the IPMI 2.0 spec table 43.17.
        // NOTE: Only a small number are handled here!
        //
        switch (pSensorInfo->Units2)
        {
        case 1:  // Degrees C
        case 2:  // Degrees F
        case 3:  // Degrees K
            if (Status > pBMCInfo->SenConfig.HealthState.TemperatureSys)
            {
                pBMCInfo->SenConfig.HealthState.TemperatureSys = Status;
            }
            break;
        case 4:  // Volts
            if (Status > pBMCInfo->SenConfig.HealthState.Volatge)
            {
                pBMCInfo->SenConfig.HealthState.Volatge = Status;
            }
            break;
        case 5:  // Amps
        case 6:  // Watts
            if (Status > pBMCInfo->SenConfig.HealthState.PowerSubSys)
            {
                pBMCInfo->SenConfig.HealthState.PowerSubSys = Status;
            }
            break;
        default:
            IPMI_ERROR("Cannot set Health State for Sensor Type 0x0B (Units2=%d)!\n",
                (int)pSensorInfo->Units2);
            break;
        }
        break;
    default:
        TDBG("Cannot set Health State for Sensor Type 0x%02x!\n",
            pSensorInfo->SensorTypeCode);
        break;
    }
}

/**
 * @brief Update health state.
**/
static void
UpdateOverallHealthState (int BMCInst)
{
    INT8U* pHealthSt = (INT8U*)&(BMC_GET_SHARED_MEM (BMCInst)->HealthState);
    int          i;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    /* Get Individual Status */
    _fmemcpy (pHealthSt, (INT8U*)&pBMCInfo->SenConfig.HealthState, sizeof (HealthState_T));

    pHealthSt [0] = HEALTH_STATUS_OK;
    /*Get Overall Status */
    for (i = 1; i < sizeof (HealthState_T); i++ )
    {
        if (pHealthSt [i] > pHealthSt [0])
        {
            pHealthSt [0] = pHealthSt [i];
        }
    }

    if (pBMCInfo->IpmiConfig.OPMASupport == 1)
    {
        if ( HEALTH_STATUS_OK  ==  pHealthSt [0]  )
        {
            if(g_PDKHandle[PDK_GLOWFAULTLED] != NULL)
            {
                 ((int(*)(int,int,INT8U,int))g_PDKHandle[PDK_GLOWFAULTLED]) (LED_OVERALL_HEALTH , LED_STATE_OFF, pHealthSt [i] ,BMCInst);
            }
        }
        else
        {
            /* If the status of the system is ot ok then glow the LED */
            for (i = 1; i < sizeof (HealthState_T); i++ )
            {
                if ( HEALTH_STATUS_OK != pHealthSt [i])
                {
                    if(g_PDKHandle[PDK_GLOWFAULTLED] != NULL)
                    {
                        ((int(*)(int,int,INT8U,int))g_PDKHandle[PDK_GLOWFAULTLED]) ( i , LED_STATE_ON, pHealthSt [i] ,BMCInst);
                    }
                }
            }
        }
    }

    /*Reinitialise Status */
    _fmemset ((INT8U*)&pBMCInfo->SenConfig.HealthState, 0, sizeof (HealthState_T));

    return;
}
#endif /* #if (TERMINAL_MODE_SUPPORT == 1) */

/**
 * @brief Generate Deassertion Event.
 **/
static void
GenerateDeassertionEvent(INT16U EventsToRearm, SensorInfo_T* pSensorInfo,int BMCInst)
{

    INT8U     EventMsg [EVENT_MSG_LENGTH];
    INT8U 	EventData3, offset, ReadableThreshMask;
    INT8U     NonRecoverableHigh =0, NonRecoverableLow=0, CriticalHigh=0, CriticalLow=0, WarningHigh=0, WarningLow =0;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    EventMsg [0] = pBMCInfo->IpmiConfig.BMCSlaveAddr;
    EventMsg [1] = (pSensorInfo->SensorOwnerLun & VALID_LUN);
    /* EvMRev */
    EventMsg [2] = IPMI_EVM_REVISION;

    if(pSensorInfo->EventTypeCode == THRESHOLD_SENSOR_CLASS)
    {

        WarningLow                   = pSensorInfo->LowerNonCritical;
        WarningHigh                  = pSensorInfo->UpperNonCritical;
        CriticalHigh                 = pSensorInfo->UpperCritical;
        CriticalLow                  = pSensorInfo->LowerCritical;
        NonRecoverableHigh           = pSensorInfo->UpperNonRecoverable;
        NonRecoverableLow            = pSensorInfo->LowerNonRecoverable;
        ReadableThreshMask           = (INT8U)(pSensorInfo->SettableThreshMask);

        if (0 == (ReadableThreshMask & BIT5))
        {
            NonRecoverableHigh = 0xFF;
        }
        if (0 == (ReadableThreshMask & BIT2))
        {
            NonRecoverableLow = 0x00;
        }
        if (0 == (ReadableThreshMask & BIT4))
        {
            CriticalHigh = NonRecoverableHigh;
        }
        if (0 == (ReadableThreshMask & BIT1))
        {
            CriticalLow  = NonRecoverableLow;
        }
        if (0 == (ReadableThreshMask & BIT3))
        {
            WarningHigh = CriticalHigh;
        }
        if (0 == (ReadableThreshMask & BIT0))
        {
            WarningLow = CriticalLow;
        }
    }

    /* sensor type */
    EventMsg [3] = pSensorInfo->SensorTypeCode;
    /* sensor number */
    EventMsg [4] = pSensorInfo->SensorNumber;

    for(offset =0; EventsToRearm !=0x00; ++offset, EventsToRearm>>=1)
    {
        if( !(EventsToRearm & 0x01) )
        {
            continue;	/* Move to next event */
        }

        /* Event Data 1 */
        EventMsg [6] = offset;
        if (pSensorInfo->EventTypeCode == THRESHOLD_SENSOR_CLASS)
        {
            /* event direction|type */
            EventMsg [5] = 0x81;

            /* Current Temperature */
            EventMsg [7] = pSensorInfo->SensorReading;

            switch(offset)
            {
                case 0x00:
                case 0x01:
                    EventData3=  WarningLow;
                    break;

                case 0x02:
                case 0x03:
                    EventData3=  CriticalLow;
                    break;

                case 0x04:
                case 0x05:
                    EventData3=  NonRecoverableLow;
                    break;

                case 0x06:
                case 0x07:
                    EventData3=  WarningHigh;
                    break;

                case 0x08:
                case 0x09:
                    EventData3=  CriticalHigh ;
                    break;

                case 0x0A:
                case 0x0B:
                    EventData3=  NonRecoverableHigh;
                    break;

                default:
                    EventData3=   0x00;
                    break;
            }
            /* Trigger that caused event */
            EventMsg [8] = EventData3;
        }
        else /* For all other Non Threshold Sensors */
        {
            /* event direction|type */
            EventMsg [5] = pSensorInfo->EventTypeCode | BIT7;

            EventMsg [7] = 0xFF;
            EventMsg [8] = 0xFF;
        }

        /* Check with hook (if present) to see if we continue */
        if (0 == PreEventLog(pSensorInfo, EventMsg,BMCInst))
        {
            /* Post Event Message */
            if (0 != PostEventMessage (EventMsg,FALSE,sizeof (EventMsg),BMCInst))
            {
                IPMI_DBG_PRINT ("Rearm Event Generation Failed\n");
            }
            else
            {
                IPMI_DBG_PRINT ("Rearm Generating Event\n");
            }
        }
    }

}

/*----------------------------------------------------------
 * SM_ReArmSensor
 *----------------------------------------------------------*/
static void
SM_ReArmSensor(ReArmSensorReq_T* pReArmSensorReq,int BMCInst)
{
    INT8U       i, SensorNum;
    INT16U      EventsToRearm=0, CurrentState, Tmp;
    SensorInfo_T *pSensorInfo;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    //    GetSensorEventEnableRes_T SenEventEnable;
    SensorNum = pReArmSensorReq->SensorNum;
    SensorSharedMem_T* pSMSharedMem = (SensorSharedMem_T*)&g_BMCInfo[BMCInst].SensorSharedMem;

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->m_hSMSharedMemMutex, SHARED_MEM_TIMEOUT);
    // If the Init Agent is initiating the rearm, set the global flag so that
    // Hook routines can tell the difference between a single rearm and a
    // rearming of all sensors from Init Agent.
    if (0xFF == SensorNum)
    {
        g_BMCInfo[BMCInst].SenConfig.InitAgentRearm = TRUE;
        if(pBMCInfo->IpmiConfig.EventsForRearm == 1)
        {
            // This is needed so sensors with a PreEventLog hook know that any deassertions
            // generated are from the InitAgent.
            if(g_PDKHandle[PDK_PREMONITORALLSENSORS] != NULL)
            {
                ((int(*)(bool,int)) g_PDKHandle[PDK_PREMONITORALLSENSORS])(g_BMCInfo[BMCInst].SenConfig.InitAgentRearm,BMCInst);
               
            }
        }
    }
    for ( i=0; i<=g_BMCInfo[BMCInst].SenConfig.ValidSensorCnt; ++i)
    {

        /* Is this the sensor we want to re-arm? */
        if ((pBMCInfo->SenConfig.ValidSensorList[i] == SensorNum) || (0xFF == SensorNum))
        {
            pSensorInfo  = &(pSMSharedMem->SensorInfo[pBMCInfo->SenConfig.ValidSensorList[i]]);
            if ( pSensorInfo->EventTypeCode ==THRESHOLD_SENSOR_CLASS)
            {	/* Manual Rearm Sensor ? */
                if ( 0 ==( pSensorInfo->SensorCaps & BIT6))
                {
                    /* If all events for this sensor should be rearmed... */
                    if (0 == (pReArmSensorReq->ReArmAllEvents & BIT7))
                    {
                        if(pBMCInfo->IpmiConfig.EventsForRearm == 1)
                        {         
                            EventsToRearm = ((pSensorInfo->DeassertionEventEnablesByte2<<8)+pSensorInfo->DeassertionEventEnablesByte1)
                                                         & (((pSensorInfo->AssertionHistoryByte2<<8) + pSensorInfo->AssertionHistoryByte1)
                                                        ^ ((pSensorInfo->DeassertionHistoryByte2<<8)+pSensorInfo->DeassertionHistoryByte1) );
                        }
                        /* Rearm all events for this sensor */

                        pSensorInfo->AssertionHistoryByte1   = 0;
                        pSensorInfo->AssertionHistoryByte2   = 0;
                        pSensorInfo->DeassertionHistoryByte1 = 0;
                        pSensorInfo->DeassertionHistoryByte2 = 0;
                    }
                    else
                    {
                        if(pBMCInfo->IpmiConfig.EventsForRearm == 1)
                        {            
                            EventsToRearm =(((pReArmSensorReq->ReArmAssertionEvents2<<8)+pReArmSensorReq->ReArmAssertionEvents1)
                                                        & ((pSensorInfo->DeassertionEventEnablesByte2<<8)+pSensorInfo->DeassertionEventEnablesByte1))
                                                        & (((pSensorInfo->AssertionHistoryByte2<<8) + pSensorInfo->AssertionHistoryByte1)
                                                        ^ ((pSensorInfo->DeassertionHistoryByte2<<8)+pSensorInfo->DeassertionHistoryByte1));
                        }
                        /* Rearm select events for this sensor */
                        pSensorInfo->AssertionHistoryByte1   &= ~(pReArmSensorReq->ReArmAssertionEvents1);
                        pSensorInfo->AssertionHistoryByte2   &= ~(pReArmSensorReq->ReArmAssertionEvents2);
                        pSensorInfo->DeassertionHistoryByte1 &= ~(pReArmSensorReq->ReArmDeassertionEvents1);
                        pSensorInfo->DeassertionHistoryByte2 &= ~(pReArmSensorReq->ReArmDeassertionEvents2);
                    }
                }
                else  /* Auto Rearm Sensor */
                {
                    if(pBMCInfo->IpmiConfig.EventsForRearm == 1)
                    {         
                        EventsToRearm = ((pSensorInfo->AssertionEventOccuredByte2<<8) + pSensorInfo->AssertionEventOccuredByte1)
                                                    & ((pSensorInfo->DeassertionEventEnablesByte2<<8)+pSensorInfo->DeassertionEventEnablesByte1);
                    }
                }
                /* Reset the threshold sensor (this will cause sensormonitor to regenerate events) */
                pSensorInfo->EventLevel                     = SENSOR_STATUS_NORMAL;
                pSensorInfo->HealthLevel                    = SENSOR_STATUS_NORMAL;
                pSensorInfo->AssertionEventOccuredByte1     = 0;
                pSensorInfo->AssertionEventOccuredByte2     = 0;
                pSensorInfo->DeassertionEventOccuredByte1   = 0;
                pSensorInfo->DeassertionEventOccuredByte2   = 0;

                if(pBMCInfo->IpmiConfig.EventsForRearm == 1)
                {         
                    /* Is event message generation enabled ? */
                    if (0 != (pSensorInfo->EventFlags & BIT7))
                    {
                        GenerateDeassertionEvent( EventsToRearm, pSensorInfo,BMCInst);
                    }
                }
            }	
            else
            {
                /* Rearm the non threshold sensor */

                /* Get current sensor reading */
                CurrentState = pSensorInfo->SensorReading;

                /* Get the assertion history (assertion sticky bits) */
                _fmemcpy (&Tmp,  &(pSensorInfo->AssertionHistoryByte1), 2);
                //EventHistory = ipmitoh_u16 (Tmp); /* Convert to big endian */

                /* Manual Rearm Sensor ? */
                if ( 0 ==( pSensorInfo->SensorCaps & BIT6))
                {
                    /* If all events for this sensor should be rearmed... */
                    if (0 == (pReArmSensorReq->ReArmAllEvents & BIT7))
                    {
                        if(pBMCInfo->IpmiConfig.EventsForRearm == 1)
                        {            
                            EventsToRearm =((pSensorInfo->DeassertionEventEnablesByte2<<8)+pSensorInfo->DeassertionEventEnablesByte1)
                                                        & ( ((pSensorInfo->AssertionHistoryByte2<<8) + pSensorInfo->AssertionHistoryByte1)
                                                        ^ ((pSensorInfo->DeassertionHistoryByte2<<8)+pSensorInfo->DeassertionHistoryByte1) );
                        }
                        /* Modify sensors PreviousState value so that appropriate events are generated
                        on the next montioring cycle after a rearm. */
                        pSensorInfo->PreviousState =  0x00;

                        /* Rearm all events for this sensor */
                        pSensorInfo->AssertionHistoryByte1   = 0;
                        pSensorInfo->AssertionHistoryByte2   = 0;
                        pSensorInfo->DeassertionHistoryByte1 = 0;
                        pSensorInfo->DeassertionHistoryByte2 = 0;
                    }
                    else
                    {
                        if(pBMCInfo->IpmiConfig.EventsForRearm == 1)
                        {
                            EventsToRearm =(((pReArmSensorReq->ReArmAssertionEvents2<<8)+pReArmSensorReq->ReArmAssertionEvents1)
                                                        & ((pSensorInfo->DeassertionEventEnablesByte2<<8)+pSensorInfo->DeassertionEventEnablesByte1))
                                                        & ( ((pSensorInfo->AssertionHistoryByte2<<8) + pSensorInfo->AssertionHistoryByte1) 
                                                        ^((pSensorInfo->DeassertionHistoryByte2<<8)+pSensorInfo->DeassertionHistoryByte1) );
                        }
                        /* Modify sensors PreviousState value so that appropriate events are generated
                        on the next montioring cycle after a rearm. */
                        pSensorInfo->PreviousState &=  ~((pReArmSensorReq->ReArmAssertionEvents2<<8)+(pReArmSensorReq->ReArmAssertionEvents1)) ;

                        /* Rearm select events for this sensor */
                        pSensorInfo->AssertionHistoryByte1   &= ~(pReArmSensorReq->ReArmAssertionEvents1);
                        pSensorInfo->AssertionHistoryByte2   &= ~(pReArmSensorReq->ReArmAssertionEvents2);
                        pSensorInfo->DeassertionHistoryByte1 &= ~(pReArmSensorReq->ReArmDeassertionEvents1);
                        pSensorInfo->DeassertionHistoryByte2 &= ~(pReArmSensorReq->ReArmDeassertionEvents2);
                    }
                }
                else	/* Auto Rearm Sensor */
                {
                    if (0 == (pReArmSensorReq->ReArmAllEvents & BIT7))
                    {
                        EventsToRearm = CurrentState & ((pSensorInfo->DeassertionEventEnablesByte2<<8)+pSensorInfo->DeassertionEventEnablesByte1);
                    }
                    else
                    {
                        EventsToRearm = CurrentState & ((pReArmSensorReq->ReArmAssertionEvents2<<8)+pReArmSensorReq->ReArmAssertionEvents1)
                                                                            & ((pSensorInfo->DeassertionEventEnablesByte2<<8)+pSensorInfo->DeassertionEventEnablesByte1);
                    }
                    /* Modify sensors PreviousState value so that appropriate events are generated
                    on the next montioring cycle after a rearm. */
                    pSensorInfo->PreviousState = (CurrentState ^ EventsToRearm);
                }

                if(pBMCInfo->IpmiConfig.EventsForRearm == 1)
                {         
                    /* Is event message generation enabled ? */
                    if ( 0 != (pSensorInfo->EventFlags & BIT7))
                    {
                        GenerateDeassertionEvent( EventsToRearm, pSensorInfo,BMCInst);
                    }
                }
            }

            // Set the Inprogress bit after rearm as per IPMI spec
            pSensorInfo->EventFlags |= BIT5;

        }
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->m_hSMSharedMemMutex);
    return;
}

/*----------------------------------------------------------
 * ProcessSMMessages
 *----------------------------------------------------------*/
static void
ProcessSMMessages(int BMCInst)
{
    static bool s_bRearmAllPending = FALSE;
    static ReArmSensorReq_T    ReArmSensorReq;
    MsgPkt_T Msg;
    bool  exit = FALSE;

    do
    {
        // If there is a rearm all pending, do it and then return to do another
        // sensor scan to update the sensor statuses.
        if (FALSE != s_bRearmAllPending)
        {
            ReArmSensorReq.SensorNum = 0xff; /* Set sensor # to ff to rearm all sensors */
            ReArmSensorReq.ReArmAllEvents = 0; /* Rearm all events */

            SM_ReArmSensor(&ReArmSensorReq,BMCInst);
            s_bRearmAllPending = FALSE;
            break;
        }
        Msg.Param = 0xffffffff;
        Msg.Size = 0;

        /* See if we need to handle a message posted to the sensor monitor task
        Wait up to the Sensor Monitor Interval
        */
        if(0 != GetMsg(&Msg, SM_HNDLR_Q, WAIT_INFINITE, BMCInst))
        {
            /* Should not normally get here! */
            // If we do, sleep for 100mS and then try to continue.
            OS_TIME_DELAY_HMSM (0, 0, 0, 100);
            continue;
        }

        /* Take action based on ? */
        switch (Msg.Param)
        {
            case PARAM_REARM_ALL_SENSORS :
                // Set the flag to show that we need to do the rearm when we come back
                // to this function again after the sensor scan.
                s_bRearmAllPending = TRUE;
                exit = TRUE;
                break;

            case PARAM_REARM_SENSOR :
                _fmemcpy(&ReArmSensorReq, Msg.Data, sizeof(ReArmSensorReq));
                SM_ReArmSensor(&ReArmSensorReq,BMCInst);
                break;

            case PARAM_HANDLE_IRQ :
                // Need to get sensor index from sensor based on IRQ. Sometime like...
                //            SensorNum = PDK_GetIRQSensor(IrqNum);
                // Get ix and determine if thresh or nonthresh based on SensorNum
                //            MonitorTSensor (ix);
                //            or
                //            MonitorNonTSensor (ix);

                break;

            case PARAM_SENSOR_SCAN :
                // Time for sensor scan.
                exit = TRUE;
                break;

            default:
                IPMI_ERROR ("SensorMonitor.c : ERROR: Invalid request: 0x%08x\n", Msg.Param);
                break;
        } /* switch (pMsg->Param) */
    }while(FALSE == exit);
    
    return;
}

/**
 * *@fn SendSignalToAdviser
 * *@brief Sends the signal to adivser according to the configuration done
 * *@param Sel Record,BMCInstance Value
 * */
static void SendSignalToAdviser(SELEventRecord_T *pSelRecord,int BMCInst)
{
    INT8U EventLevel;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    SensorInfo_T *pSensorInfo = NULL;


    if(THRESHOLD_SENSOR_CLASS == pSelRecord->EvtDirType)
    {
        EventLevel = pSelRecord->EvtData1 & 0x0F;

        switch(pSelRecord->SensorType)
        {
	    case FAN_SENSOR_TYPE:
		if ((pBMCInfo->TriggerEvent.FanTroubled & 0x01) == 0x01)
		{
		    if ((EventLevel == LOWER_CRITICAL_GNG_LOW) || (EventLevel == UPPER_CRITICAL_GNG_HIGH))
		    {
			if(0 == send_signal_by_name("Adviserd",SIGUSR2))
			{
			    TDBG("Signal Sent to Adviser to record video due to status change in FAN\n");
			}
			else
			{
			    TDBG("Signal Not Sent to Adviser........ PID might be wrong\n");
			}
		    }
		}
		break;
            case VOLT_SENSOR_TYPE:
            case TEMP_SENSOR_TYPE:
                if(pBMCInfo->TriggerEvent.CriticalFlag)
                {
                    if((EventLevel == LOWER_CRITICAL_GNG_LOW ) || (EventLevel == UPPER_CRITICAL_GNG_HIGH ))
                    {
                        if(0 == send_signal_by_name("Adviserd",SIGUSR2))
                        {
                            TDBG("Signal Sent to Adviser for Video Recording\n");
                        }
                        else
                        {
                           TDBG("Signal Not Sent to Adviser for some other reason\n");
                        }
                    }
                }
                 if(pBMCInfo->TriggerEvent.NONCriticalFlag)
                 {
                     if((EventLevel == LOWER_NON_CRITICAL_GNG_LOW ) || (EventLevel == UPPER_NON_CRITICAL_GNG_HIGH ))
                     {
                         if(0 == send_signal_by_name("Adviserd",SIGUSR2))
                         {
                             TDBG("Signal Sent to Adviser to Video Recording\n");
                         }
                         else
                         {
                            TDBG("Signal Not Sent to Adviser for some other reason\n");
                         }
                     }
                 }
                 if(pBMCInfo->TriggerEvent.NONRecoverableFlag)
                 {
                     if((EventLevel == LOWER_NON_RECOVERABLE_GNG_LOW ) || (EventLevel == UPPER_NON_RECOVERABLE_GNG_HIGH ))
                     {
                         if(0 == send_signal_by_name("Adviserd",SIGUSR2))
                         {
                             TDBG("Signal Sent to Adviser to Video Recording\n");
                         }
                         else
                         {
                            TDBG("Signal Not Sent to Adviser for some other reason\n");
                         }
                     }
                 }
                 break;
             default:
                 TDBG("Don't Send the Signal\n");
         }
     }
     else if(GENERIC_EVENT_TYPE_DEV_PRESENCE == (pSelRecord->EvtDirType & 0x0F) )
     {
         switch(pSelRecord->SensorType)
        {
            case FAN_SENSOR_TYPE:
                if ((pBMCInfo->TriggerEvent.FanTroubled & 0x02) == 0x02)
                {
                    pSensorInfo = GetSensorInfo(pSelRecord->SensorNum, (pSelRecord->GenID[1] & VALID_LUN),BMCInst);
                    if(NULL == pSensorInfo)
                    {
                       TDBG("Sensor Information Not Available\n");
                       break;
                    }
                    if((pSensorInfo->PreviousState) && (!pSensorInfo->SensorReading))
                    {
                         if(0 == send_signal_by_name("Adviserd",SIGUSR2))
                        {
                             TDBG("Signal Sent to Adviser to record video due to status change in FAN\n");
                        }
                        else
                        {
                            TDBG("Signal Not Sent to Adviser........ PID might be wrong\n");
                        }
                    }
                }
                break;
            default:
                TDBG("Don't Send the Signal for digital discrete sensors status change except FAN\n");
        }
     }
  return;
}



/**
 * *@fn Swap SensorThresholds
 * *@brief Swaps Sensor Thresholds from NVRAM to RAM
 * *@param BMCInstance Value
 * *@return Zero if successful and -1 if failure
 * */

static int SwapSensorThresholds (int BMCInst, INT8U SensorNum, INT8U OwnerLUN, INT8U OwnerID )
{
	FILE *sensorfp;
	SensorThresholds SensorRecord;
	SensorSharedMem_T*  pSenSharedMem;
	BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
	pSenSharedMem = (SensorSharedMem_T*)&pBMCInfo->SensorSharedMem;
	char SensorFileName[MAX_SEN_NAME_SIZE];
    INT16U LUNSensorNum;
	
	memset(SensorFileName,0,MAX_SEN_NAME_SIZE);
	
	SENSORTHRESH_FILE(BMCInst, SensorFileName);
	
	
	sensorfp = fopen(SensorFileName,"rb");
	if(sensorfp == NULL)
  {
	    return -1;
  }
	
	OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo[BMCInst].SensorSharedMemMutex, WAIT_INFINITE);
	
	LUNSensorNum = ((OwnerLUN & VALID_LUN) << 8 | SensorNum);

	while((sizeof(SensorThresholds) == fread((char *)&SensorRecord,sizeof(char),sizeof(SensorThresholds),sensorfp)))
	{
	 if(pSenSharedMem->SensorInfo[LUNSensorNum].EventTypeCode == THRESHOLD_SENSOR_CLASS)
   {	
			if(pSenSharedMem->SensorInfo[LUNSensorNum].SensorNumber == SensorRecord.SensorNum)
			{
				pSenSharedMem->SensorInfo[LUNSensorNum].LowerCritical = SensorRecord.LowerCritical;
				pSenSharedMem->SensorInfo[LUNSensorNum].LowerNonCritical = SensorRecord.LowerNonCritical;
				pSenSharedMem->SensorInfo[LUNSensorNum].LowerNonRecoverable = SensorRecord.LowerNonRecoverable;
				pSenSharedMem->SensorInfo[LUNSensorNum].UpperCritical = SensorRecord.UpperCritical;
				pSenSharedMem->SensorInfo[LUNSensorNum].UpperNonCritical = SensorRecord.UpperNonCritical;
				pSenSharedMem->SensorInfo[LUNSensorNum].UpperNonRecoverable = SensorRecord.UpperNonRecoverable;
			}
		}
		memset(&SensorRecord,0,sizeof(SensorThresholds));
	}
	
	fclose(sensorfp);
	
	OS_THREAD_MUTEX_RELEASE(&g_BMCInfo[BMCInst].SensorSharedMemMutex);

	
	return 1;
}
#endif /*SENSOR_DEVICE == 1*/

