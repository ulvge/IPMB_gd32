/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * WDT.c
 * WadtchDog TimeOut function
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0

#include "Types.h"
#include "WDT.h"
#include "Message.h"
#include "Debug.h"
#include "WDT.h"
#include "IPMIDefs.h"
#include "NVRAccess.h"
#include "IPMI_Sensor.h"
#include "Events.h"
#include "Platform.h"
#include "SharedMem.h"
#include "ChassisDevice.h"
#include "ChassisAPI.h"
#include"IPMI_KCS.h"
#include "IPMI_BT.h"
#include "ChassisCtrl.h"
#include "PDKAccess.h"
#include "IPMIConf.h"
#include "Util.h"
#include "LANConfig.h"
#include <sys/prctl.h>
#include "featuredef.h"
#include "featuredef.h"

/*** Local macro definitions ***/
#define PRE_TIMEOUT             1
#define WDT_TIMEOUT             2
#define PRETIMEOUT_ACTION_TAKEN 0x08
#define PSGOOD_RETRY_COUNT	2

#define TIMEOUT_NO_ACTION_TAKEN         0x00
#define TIMEOUT_ACTION_HARD_RESET       0x01
#define TIMEOUT_ACTION_POWER_DOWN       0x02
#define TIMEOUT_ACTION_POWER_CYCLE      0x03

#define PRE_TIMEOUT_SMI                 0x10
#define PRE_TIMEOUT_NMI_DIAG            0x20
#define PRE_TIMEOUT_MESSAGING_INTR      0x30

/*** Global variables **/
_FAR_ WDTTmrMgr_T   g_WDTTmrMgr;

/*----------------------------------------------------------------
 * WatchDog2 Event Offsets
 *---------------------------------------------------------------*/
#define IPMI_WDT_TIMER_EXPIRED			0x00   //Watch Dog Expired Status only offset 
#define IPMI_WDT_HARD_RESET				0x01
#define IPMI_WDT_POWER_DOWN				0x02
#define IPMI_WDT_POWER_CYCLE			0x03
#define IPMI_WDT_TIMER_INTERRUPT		0x08

#define MAX_WDT_MS_VALUE(Hertz)     (0xFFFFFFFF * 1000)/Hertz

/*** Function Prototypes ***/
static void WDTTmrAction (INT8U TmrAction, int BMCInst);

/* Declare Thread Mutex */
//OS_THREAD_MUTEX_DECLARE(hWDTSharedMemMutex);


/**
 * WDTTimerTask
 **/
void *
WDTTimerTask (void *pArg)
{
    int *inst = (int*) pArg;
    int BMCInst = *inst;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT32U WDTInterval = 0,WDTPreInterval = 0,jiffycmp = 0;
    long long jiffyvalue=0,jiffystart = 0;
    unsigned long Hertz = sysconf(_SC_CLK_TCK);
    int PSGoodRetry = 0;
    prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

    /* Get the Shared memory mutex handle */
    OS_THREAD_MUTEX_INIT(pBMCInfo->hWDTSharedMemMutex, PTHREAD_MUTEX_RECURSIVE);

    while(TRUE)
    {
        /* Will be waiting till the Watchdog timer is armed*/
        sem_wait(&pBMCInfo->WDTSem);

        /* If no timer to process return */
        if (FALSE == BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent)
        {
            continue;
        }

        WDTInterval = pBMCInfo->WDTConfig.InitCountDown * WDT_COUNT_MS;
        if ((pBMCInfo->WDTConfig.TmrActions & 0x70) == 0)
        {
            WDTPreInterval = WDTInterval+1;
        }
        else
        {
            WDTPreInterval = 1000 * pBMCInfo->WDTConfig.PreTimeOutInterval;
        }

        if(GetJiffySysCtlvalue(JIFFY_VALUE,&jiffyvalue) == 1)
        {
            IPMI_ERROR("Error in getting Jiffy value \n");
            return 0;
        }

        jiffystart = jiffyvalue;

        while(1)
        {

            if(GetJiffySysCtlvalue(JIFFY_VALUE,&jiffyvalue) == 1)
            {
                IPMI_ERROR("Error in getting Jiffy value \n");
                return 0;
            }

            if (pBMCInfo->HostOFFStopWDT == TRUE)
            {
                OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->hWDTSharedMemMutex, WAIT_INFINITE)
                BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning = FALSE;
                BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent = FALSE;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->hWDTSharedMemMutex);
                pBMCInfo->HostOFFStopWDT = FALSE;			
                break;
            }

            if(g_PDKHandle[PDK_GETPSGOOD] != NULL)
            {
                if (FALSE == ((int(*)(int))g_PDKHandle[PDK_GETPSGOOD]) (BMCInst))
                {
                    /*Retry the PSGood state to confirm the PSGood status.*/
                    while(PSGoodRetry < PSGOOD_RETRY_COUNT)
                    {
                        if(TRUE == ((int(*)(int))g_PDKHandle[PDK_GETPSGOOD]) (BMCInst))
                        {
                            break;
                        }
                        PSGoodRetry++;
                    }
                    if(PSGoodRetry == PSGOOD_RETRY_COUNT)
                    {
                        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->hWDTSharedMemMutex, WAIT_INFINITE)
                        BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning = FALSE;
                        BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent = FALSE;
                        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->hWDTSharedMemMutex);
                        break;
                    }
                }
                PSGoodRetry = 0;
            }

            if(pBMCInfo->SetWDTUpdated == TRUE && (TRUE == BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent))
            {
                jiffystart = jiffyvalue;
                WDTInterval = pBMCInfo->WDTConfig.InitCountDown * WDT_COUNT_MS;
                if ((pBMCInfo->WDTConfig.TmrActions & 0x70) == 0)
                {
                    WDTPreInterval = WDTInterval+1;
                    
                }
                else
                {
                    WDTPreInterval = 1000 * pBMCInfo->WDTConfig.PreTimeOutInterval;
                }
                pBMCInfo->SetWDTUpdated = FALSE;
            }
            else if(pBMCInfo->SetWDTUpdated == TRUE && (FALSE== BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent))
            {
                /* Restore Initial countdown value */
                g_WDTTmrMgr.TmrInterval = pBMCInfo->WDTConfig.InitCountDown;
                break;
            }

            if(jiffyvalue < jiffystart)
            {
                /* To handle Jiffy Overflow condition */
                jiffycmp = (MAX_WDT_MS_VALUE(Hertz) -jiffystart) + jiffyvalue;
            }
            else
            {
                jiffycmp = jiffyvalue - jiffystart;
            }

            if((jiffycmp > WDTInterval) ||(jiffycmp > (WDTInterval-WDTPreInterval)))
            {
                if((jiffycmp > (WDTInterval-WDTPreInterval)) && (pBMCInfo->WDTPreTmtStat == FALSE)
                    && ((pBMCInfo->WDTConfig.TmrActions & 0x70) != 0))
                {
                    /* take pretimeout actions */
                    if(pBMCInfo->TriggerEvent.WDTTimeExpire)
                    {
                        if(0 == send_signal_by_name("Adviserd",SIGUSR2))
                        {
                             TDBG("Signal Sent successfully to adviser to start recording video\n");
                        }
                        else
                        {
                             TDBG("Cannot Send Signal to adviser to start recording video\n");
                        }
                    }
                    WDTTmrAction (g_WDTTmrMgr.WDTTmr.TmrActions & 0x70, BMCInst);
                    pBMCInfo->WDTPreTmtStat = TRUE;
                }
                else if(jiffycmp > WDTInterval)
                {
                    g_WDTTmrMgr.TmrPresent = FALSE;
                    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->hWDTSharedMemMutex, WAIT_INFINITE)
                    BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning=FALSE;
                    BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent =FALSE;
                    // Modify ARP status to resume the thread
                    // after receiving set Watchdog Timer command
                    //BMC_GET_SHARED_MEM(BMCInst)->GratArpStatus = RESUME_ARPS;
                    
                    INT8U i = 0;

                    for (i = 0; i < MAX_LAN_CHANNELS; i++)
                    {
                        if((pBMCInfo->LanIfcConfig[i].Enabled == TRUE)
                            && (pBMCInfo->LanIfcConfig[i].Up_Status == LAN_IFC_UP))
                        {
                            BMC_GET_SHARED_MEM(BMCInst)->ArpSuspendStatus[i] = RESUME_ARPS;
                            UpdateArpStatus(pBMCInfo->LanIfcConfig[i].Ethindex, BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning, BMCInst);
                        }
                    }
                    
                    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->hWDTSharedMemMutex);
                    /* Take WDT timeout Actions */
                    pBMCInfo->WDTPreTmtStat = FALSE;
                    g_WDTTmrMgr.TmrInterval = 0;
                    if(pBMCInfo->TriggerEvent.WDTTimeExpire)
                    {
                        if(0 == send_signal_by_name("Adviserd",SIGUSR2))
                        {
                            TDBG("Signal Sent successfully to adviser to start recording video\n");
                        }
                        else
                        {
                            TDBG("Cannot Send Signal to adviser to start recording video\n");
                        }
                    }
                    WDTTmrAction (g_WDTTmrMgr.WDTTmr.TmrActions & 0x07, BMCInst);
                    break;
                 }
                
            }

            if((jiffycmp -WDTInterval) > 20)
            {
                /* Made to sleep for 100ms to reduce CPU Usage */
                usleep(WDT_SLEEP_TIME);
                g_WDTTmrMgr.TmrInterval = pBMCInfo->WDTConfig.InitCountDown - (jiffycmp/WDT_COUNT_MS);
            }
        }
    }

}

/**
 * @brief Takes action depending on the argument.
 * @param TmrAction Timeout action.
 **/
static void
WDTTmrAction (INT8U TmrAction, int BMCInst)
{
    int i = 0;
    INT8U  u8EventOffset;
    INT8U  readFlags = 0;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    _FAR_ SensorInfo_T*        pSensorInfo;    
    SensorSharedMem_T*    pSMSharedMem;

    if (TmrAction & 0x70)
    {
        pBMCInfo->WDTConfig.PreTimeoutActionTaken = g_WDTTmrMgr.WDTTmr.PreTimeoutActionTaken = PRETIMEOUT_ACTION_TAKEN;
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
    else
    {
        pBMCInfo->WDTConfig.ExpirationFlag |=  0x1 << (pBMCInfo->WDTConfig.TmrUse & 0x07);

        // Based on the IPMI spec 27.7, the WDT expiration flag should not be preserved on any AC cycle.
        // update the WDT expiration flag in the global memory pool, not persistant in the flash
        g_WDTTmrMgr.WDTTmr.ExpirationFlag |=  0x1 << (pBMCInfo->WDTConfig.TmrUse & 0x07);
    }
    // setup sensor event offset
    u8EventOffset = TmrAction;

    if(g_corefeatures.wdt_flush_support == ENABLED )
    {
        FlushIPMI((INT8U*)&pBMCInfo->WDTConfig,(INT8U*)&pBMCInfo->WDTConfig,pBMCInfo->IPMIConfLoc.WDTDATAddr,
                       sizeof(WDTConfig_T),BMCInst);
    }

    if(g_PDKHandle[PDK_BEFOREWATCHDOGACTION] != NULL)
    {
        if( (((int(*)(INT8U,WDTTmrMgr_T*,int))g_PDKHandle[PDK_BEFOREWATCHDOGACTION]) (TmrAction, &g_WDTTmrMgr,BMCInst)) != 0)
        {
            return;
        }
    }
    else
    {
    	if (g_PDKHandle[PDK_PREWATCHDOGACTION] != NULL)
	{
        	if( (((int(*)(INT8U*, WDTTmrMgr_T*, int))g_PDKHandle[PDK_PREWATCHDOGACTION]) (&TmrAction, &g_WDTTmrMgr,BMCInst)) != 0)
        	{
            		return;
        	}
    	}
    }

    switch (TmrAction)
    {
        case TIMEOUT_NO_ACTION_TAKEN:
            break;

        case TIMEOUT_ACTION_HARD_RESET:
            //Added for GetSystem RestartCause //
            pBMCInfo->ChassisConfig.SysRestartCause = RESTART_CAUSE_WDT_EXPIRATION;
            //API_OnSetRestartCause(RESTART_CAUSE_WDT_EXPIRATION, TRUE);
            OnSetRestartCause(pBMCInfo->ChassisConfig.SysRestartCause, TRUE,BMCInst);

            // perform the chassis control action
            Platform_HostColdReset (BMCInst);
            break;

        case TIMEOUT_ACTION_POWER_DOWN:
            //Added for GetSystem RestartCause //
            pBMCInfo->ChassisConfig.SysRestartCause = RESTART_CAUSE_WDT_EXPIRATION;
            //API_OnSetRestartCause(RESTART_CAUSE_WDT_EXPIRATION, TRUE);
            OnSetRestartCause(pBMCInfo->ChassisConfig.SysRestartCause, TRUE,BMCInst);

            // perform the chassis control action
            Platform_HostPowerOff (BMCInst);
            break;

        case TIMEOUT_ACTION_POWER_CYCLE:
            //Added for GetSystem RestartCause //
            pBMCInfo->ChassisConfig.SysRestartCause = RESTART_CAUSE_WDT_EXPIRATION;
            //API_OnSetRestartCause(RESTART_CAUSE_WDT_EXPIRATION, TRUE);
            OnSetRestartCause(pBMCInfo->ChassisConfig.SysRestartCause, TRUE,BMCInst);

            // perform the chassis control action
            Platform_HostPowerCycle (BMCInst);
            break;

        case PRE_TIMEOUT_NMI_DIAG:
            // perform the NMI
            Platform_HostDiagInt (BMCInst);

            // fall through

        case PRE_TIMEOUT_SMI:
        case PRE_TIMEOUT_MESSAGING_INTR:
            // setup sensor event offset to be the timer interrupt (08h)
            u8EventOffset = IPMI_WDT_TIMER_INTERRUPT;				
            break;
        default:
            IPMI_ERROR("Invalid Timer Action for watchdog.\n");
            break;
    }
    // save the system restart cause even if there is no change
    FlushIPMI((INT8U*)&pBMCInfo->ChassisConfig,(INT8U*)&pBMCInfo->ChassisConfig,pBMCInfo->IPMIConfLoc.ChassisConfigAddr,sizeof(ChassisConfig_T),BMCInst);
    // pass the WDT event offset
    if(g_corefeatures.internal_sensor == ENABLED)
    {
        SetWD2SensorReading((1 << u8EventOffset), g_WDTTmrMgr.WDTTmr.TmrUse,g_WDTTmrMgr.WDTTmr.TmrActions,BMCInst);
	
	/* Get the Sensor Shared Memory */
	pSMSharedMem = (_FAR_ SensorSharedMem_T*)&pBMCInfo->SensorSharedMem;
	for (i = 0; i < pBMCInfo->SenConfig.ValidSensorCnt; i++)
	{	
	    /* When the SensorType is WatchDog, calling WD2EventLog here to create EventLog instead through SensorMonitor */    
	    if(pSMSharedMem->SensorInfo[pBMCInfo->SenConfig.ValidSensorList [i]].SensorTypeCode == SENSOR_TYPE_WATCHDOG2)
	    {
		pSensorInfo = (SensorInfo_T*)&pSMSharedMem->SensorInfo[pBMCInfo->SenConfig.ValidSensorList [i]];
		WD2EventLog(pSensorInfo,&readFlags,BMCInst);
	    }
	}
    }

    /* Clearing don't log bit after Watchdog timeout */
    g_WDTTmrMgr.WDTTmr.TmrUse &= 0x7F;

    if(g_PDKHandle[PDK_WATCHDOGACTION] != NULL)
    {
        ((void(*)(INT8U,WDTTmrMgr_T*,int))g_PDKHandle[PDK_WATCHDOGACTION]) (TmrAction,&g_WDTTmrMgr,BMCInst);
    }

}


/*-----------------------------------------------------------------
 * @fn StopWDTTimer
 *
 * @brief This is provided to stop Watchdog Timer.
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
StopWDTTimer (int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    g_WDTTmrMgr.TmrPresent = FALSE;
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->hWDTSharedMemMutex, WAIT_INFINITE);
    BMC_GET_SHARED_MEM(BMCInst)->IsWDTRunning = FALSE;
    BMC_GET_SHARED_MEM(BMCInst)->IsWDTPresent = FALSE;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->hWDTSharedMemMutex);
}

