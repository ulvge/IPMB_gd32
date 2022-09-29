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
 * ChassisCtrl.c
 * Chassis control functions.
 *
 *  Author: AMI MegaRAC PM Team
 ******************************************************************/
#define ENABLE_DEBUG_MACROS	0

#include "Types.h"
#include "OSPort.h"
#include "IPMI_Main.h"
#include "MsgHndlr.h"
#include "Message.h"
#include "Debug.h"
#include "ChassisCtrl.h"
#include "ChassisDevice.h"
#include "IPMI_IPM.h"
#include "IPMIDefs.h"
#include "App.h"
#include "SharedMem.h"
#include "ipmi_hal.h"
#include "Ethaddr.h"
#include "IPMIConf.h"
#include "PDKAccess.h"
#include "IfcSupport.h"
#include "SSIAccess.h"
#include "featuredef.h"
#include<sys/prctl.h>
#include <dlfcn.h>
#include "PDKCmdsAccess.h"

/*** Local definitions ***/
#define		INIT_CTRL_FN					0xFF
#define     ACPI_SET_SYS_PWR_STATE          0x80
#define AUTO_VIDEO_RECORD_CMD	0x0


/*** Prototype Declaration ***/
void OnPwrBtn           (int BMCInst);
void OnResetBtn         (int BMCInst);
void OnNMIBtn           (int BMCInst);
void OnS5State          (int BMCInst);
void OnS1State          (int BMCInst);
void OnProcHot          (int BMCInst);
void OnThermalTrip      (int BMCInst);
void OnFRB3Timer        (int BMCInst);
void PostToChassisTask  (INT32U State,int BMCInst);
void SetACPIState       (INT8U State,int BMCInst);

void InitChVolatilesettings(int BMCInst);


#if 0
/*--------------------------------------------------------------
 * InitChassisCtrl
 *--------------------------------------------------------------*/
int
InitChassisCtrl (void)
{
	/* Register the Interrupts	*/
	PDK_HookIntHandler (AC_LOST_HANDLER,	   OnACLost);
	PDK_HookIntHandler (PS_HANDLER,		       OnPSInterrupt);
	PDK_HookIntHandler (PWR_BTN_HANDLER,	   OnPwrBtn);
	PDK_HookIntHandler (RESET_BTN_HANDLER,	   OnResetBtn);
	PDK_HookIntHandler (S5_HANDLER,		       OnS5State);
	PDK_HookIntHandler (S1_HANDLER,		       OnS1State);
	PDK_HookIntHandler (PROC_HOT_HANDLER,	   OnProcHot);
	PDK_HookIntHandler (THERMAL_TRIP_HANDLER,  OnThermalTrip);
	PDK_HookIntHandler (FRB3_TIMER_HANDLER,    OnFRB3Timer);

    return  0;
}
#endif

/**
 * @brief Chassis control task.
**/
void* ChassisTask (void *pArg)
{
    int BMCInst =0,i;
    INT8U   Action, PSGood = 0;
    INT32U	State;
    MsgPkt_T    MsgPkt;
    static void *ApmlInit = NULL;
    static void *ApmlClose = NULL;
    void *dlHandle = NULL;
    _FAR_ BMCInfo_t* pBMCInfo = NULL;
    int retval = -1;
    int tempret=0;/*To check return value of snprintf, it is not used any where else*/
    int curThreadIndex = 0;

    UN_USED(pArg);
    prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

    if(g_corefeatures.ipmi_thread_monitor_support == ENABLED)
    {
        pthread_t thread_id = pthread_self();
        for(; curThreadIndex <= gthreadIndex; curThreadIndex++)
        {
            if(pthread_equal(gthreadIDs[curThreadIndex], thread_id))
            {
                IPMI_DBG_PRINT_2("ChassisCtrl.c: ChassisTask thread[ID: %d] found with index=%d....\n", thread_id, curThreadIndex);
                gthread_monitor_health[curThreadIndex].validThreadID = THREAD_ID_FOUND;
                gthread_monitor_health[curThreadIndex].BMCInst = BMCInst;
                tempret=snprintf(gthread_monitor_health[curThreadIndex].threadName,sizeof(gthread_monitor_health[curThreadIndex].threadName),"%s", __FUNCTION__);
		    if((tempret<0)||(tempret>=(signed)sizeof(gthread_monitor_health[curThreadIndex].threadName)))
	        {
	              TCRIT("Buffer Overflow");
	              return NULL;
	        }
                break;
            }
        }
    }
    IPMI_DBG_PRINT ("Chassis control task started\n");

    Update_ThreadID(THREAD_CAN_BE_STOPPED);

    /* Initialize Shared memory mutex handle */
    for(i = 1 ; i <= BMCInstCount; i++)
    {
        OS_THREAD_MUTEX_INIT(g_BMCInfo[i].CCSharedMemMutex, PTHREAD_MUTEX_RECURSIVE);
    }

    if(g_PDKHandle[PDK_ONTASKSTARTUP] != NULL)
    {
        ((void(*)(INT8U,int))g_PDKHandle[PDK_ONTASKSTARTUP]) (CHASSIS_CTRL_TASK_ID,0);
    }

    while (TRUE)
    {
        if(g_corefeatures.ipmi_thread_monitor_support == ENABLED && gthread_monitor_health[curThreadIndex].validThreadID == THREAD_ID_FOUND)
        {
            OS_THREAD_MUTEX_ACQUIRE(&ThreadMonitorMutex, WAIT_INFINITE);
            gthread_monitor_health[curThreadIndex].monitor = THREAD_STATE_MONITOR_NO;
            gthread_monitor_health[curThreadIndex].time = 0;
            OS_THREAD_MUTEX_RELEASE(&ThreadMonitorMutex);
        }
        /* Wait for any messages */
        if (0 == GetMsg (&MsgPkt, CHASSIS_CTRL_Q, WAIT_INFINITE, COMMON_QUEUE))
        {
            BMCInst = MsgPkt.Data[MsgPkt.Size -1];
            pBMCInfo = &g_BMCInfo[BMCInst];
            if(g_corefeatures.ipmi_thread_monitor_support == ENABLED && gthread_monitor_health[curThreadIndex].validThreadID == THREAD_ID_FOUND)
            {
                OS_THREAD_MUTEX_ACQUIRE(&ThreadMonitorMutex, WAIT_INFINITE);
                gthread_monitor_health[curThreadIndex].monitor = THREAD_STATE_MONITOR_YES;
                OS_THREAD_MUTEX_RELEASE(&ThreadMonitorMutex);
            }

            /* Check if any OEM Chassis Ctrl action needs to be taken */ 
            if (g_PDKHandle[PDK_OEMCHASSISCTRL] != NULL)
            {
                retval = -1;
                retval = ((int(*)(MsgPkt_T, int))g_PDKHandle[PDK_OEMCHASSISCTRL]) (MsgPkt, BMCInst);
                // Retval = 0 indicates, the chassis ctrl command has been handled in the OEM hook area.
                // Hence skipping the core chassis ctrl sequence
                if (retval == 0)
                {
                    continue;
                }
            }

            switch (MsgPkt.Param)
            {

                case CHASSIS_CTRL_ACTION: /*CHASSIS CONTROL ACTION*/
                    {
                        Action = MsgPkt.Data [0];
                        //Reason = MsgPkt.Data [1];

                        IPMI_DBG_PRINT_1 ("ChassisCtrl.c : Chassis Ctrl action %d\n", Action );

                        if(g_PDKHandle[PDK_GETPSGOOD] != NULL)
                        {
                            PSGood = ((int(*)(int))g_PDKHandle[PDK_GETPSGOOD]) (BMCInst);
                        }

                    switch (Action)
                    {
                        case CHASSIS_POWER_OFF:

                            if (TRUE != PSGood)
                            {
                            if(g_corefeatures.ssi_support == ENABLED)
                            {
                                /* Power is already off, no need to notify power control code.
                                 * Do queue Operational State condition.
                                 * If Operational State Machine already thinks power is already off,
                                 * this condition will just get ignored. */
                                if (g_SSIHandle[SSICB_QUEUECOND] != NULL)
                                {
                                    ((STATUS(*)(INT8U, INT8U, INT8U, int))g_SSIHandle[SSICB_QUEUECOND]) (DEFAULT_FRU_DEV_ID, COND_POWER_ON, 0, BMCInst);
                                }

                                LOCK_BMC_SHARED_MEM(BMCInst);
                                BMC_GET_SHARED_MEM(BMCInst)->PowerActionInProgress = FALSE;
                                UNLOCK_BMC_SHARED_MEM(BMCInst);
                            }
                                break;
                            }

                            if(g_PDKHandle[PDK_BEFOREPOWEROFFCHASSIS] != NULL)
                            {
                                if( 0xFF == ((int(*)(int))g_PDKHandle[PDK_BEFOREPOWEROFFCHASSIS]) (BMCInst))
                                {
                                    break;
                                }
                            }

                            /* Tracker #30193 related to wrong ACPI state setting fix */
                            SetACPIState (IPMI_ACPI_S5,BMCInst);

                            if (pBMCInfo->IpmiConfig.APMLSupport)
                            { 
                                if (GetLibrarySymbol(&dlHandle,"ApmlClose", APML_LIB_PATH, &ApmlClose) == 0)
                                {
                                    ((int(*)(int))ApmlClose)(BMCInst);
                                    dlclose(dlHandle);
                                }
                            }
                            
                            if(pBMCInfo->TriggerEvent.SysDCoffFlag)
                            {
                                if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE] != NULL)
                                {
                                    if( 0 > ((int(*)(int , int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE])(AUTO_VIDEO_RECORD_CMD, 0) )
                                    {
                                        IPMI_ERROR("Error in Writing data to Adviser\n");
                                    }
                                    else
                                        TDBG("Signal Sent Successfully to adviser to start video recording\n");
                                }
                                else
                                {
                                    TDBG("Signal Not Sent Successfully to adviser to start video recording\n");
                                }
                            }
                            if(pBMCInfo->SOLTriggerEvent.SysDCoffFlag)
                            {
                                if(0 == send_signal_by_name("uartmirrorlog",SIGUSR2))
                                {
                                    TDBG("Signal Sent successfully to UART Mirror Logging application for SOL archive\n");
                                }
                                else
                                {
                                    TDBG("Cannot Send Signal to UART Mirror Logging application for SOL archive\n");
                                }
                            }

                            if(g_PDKHandle[PDK_POWEROFFCHASSIS] != NULL)
                            {
                                ((int(*)(int))g_PDKHandle[PDK_POWEROFFCHASSIS]) (BMCInst);
                            }

                            if(g_PDKHandle[PDK_AFTERPOWEROFFCHASSIS] != NULL)
                            {
                                ((void(*)(int))g_PDKHandle[PDK_AFTERPOWEROFFCHASSIS]) (BMCInst);
                            }

                            /* We have to override the LAN access volatile setting when system is power off  */					
                            InitChVolatilesettings(BMCInst);					  
                            break;

                        case CHASSIS_POWER_ON:

                            if (TRUE == PSGood)
                            {
                                 if(g_corefeatures.ssi_support == ENABLED)
                                 {
                                    /* Power is already on, no need to notify power control code.
                                     * Do queue Operational State condition.
                                     * If Operational State Machine already thinks power is already on,
                                     * this condition will just get ignored. */
                                    if (g_SSIHandle[SSICB_QUEUECOND] != NULL)
                                    {
                                        ((STATUS(*)(INT8U, INT8U, INT8U, int))g_SSIHandle[SSICB_QUEUECOND]) (DEFAULT_FRU_DEV_ID, COND_POWER_ON, 1, BMCInst);
                                    }

                                    LOCK_BMC_SHARED_MEM(BMCInst);
                                    BMC_GET_SHARED_MEM(BMCInst)->PowerActionInProgress = FALSE;
                                    UNLOCK_BMC_SHARED_MEM(BMCInst);
                                 }
                                 break;
                            }

                            if(g_PDKHandle[PDK_BEFOREPOWERONCHASSIS] != NULL)
                            {
                                if( 0xFF == ((int(*)(int))g_PDKHandle[PDK_BEFOREPOWERONCHASSIS]) (BMCInst))
                                {
                                    break;
                                }
                            }

                            SetACPIState (IPMI_ACPI_S0,BMCInst);
                            
                            if(pBMCInfo->TriggerEvent.SysDConFlag)
                            {
                                if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE] != NULL)
                                {
                                    if( 0 > ((int(*)(int , int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE])(AUTO_VIDEO_RECORD_CMD, 0) )
                                    {
                                        IPMI_ERROR("Error in Writing data to Adviser\n");
                                    }
                                    else
                                        TDBG("Signal Sent Successfully to adviser to start video recording\n");
                               }
                               else
                               {
                                   TDBG("Signal Not Sent Successfully to adviser to start video recording\n");
                               }
                            }
                             if(pBMCInfo->SOLTriggerEvent.SysDConFlag)
                            {
                                    if(0 == send_signal_by_name("uartmirrorlog",SIGUSR2))
                                    {
                                        TDBG("Signal Sent successfully to UART Mirror Logging application for SOL archive\n");
                                    }
                                    else
                                    {
                                        TDBG("Cannot Send Signal to adviser to UART Mirror Logging application for SOL archive\n");
                            	}
                            }
 
                            if(g_PDKHandle[PDK_POWERONCHASSIS] != NULL)
                            {
                                ((int(*)(int))g_PDKHandle[PDK_POWERONCHASSIS]) (BMCInst);
                            }

                            if(g_PDKHandle[PDK_AFTERPOWERONCHASSIS] != NULL)
                            {
                                ((void(*)(int))g_PDKHandle[PDK_AFTERPOWERONCHASSIS]) (BMCInst);
                            }

                            if (pBMCInfo->IpmiConfig.APMLSupport)
                            { 
                                if (GetLibrarySymbol(&dlHandle,"ApmlInit", APML_LIB_PATH, &ApmlInit) == 0)
                                {
                                    ((int(*)(int))ApmlInit)(BMCInst);
                                    dlclose(dlHandle);
                                }
                            }
                            break;

                        case CHASSIS_POWER_CYCLE:

                            if (TRUE != PSGood)     { break; }

				if(pBMCInfo->ChassisConfig.PowerCycleInterval >= g_coremacros.ipmi_thread_monitor_timeout)
                                {
                                        OS_THREAD_MUTEX_ACQUIRE(&ThreadMonitorMutex, WAIT_INFINITE);
                                        gthread_monitor_health[curThreadIndex].monitor = THREAD_STATE_MONITOR_NO;
                                        OS_THREAD_MUTEX_RELEASE(&ThreadMonitorMutex);
                                }

                            if(g_PDKHandle[PDK_BEFOREPOWERCYCLECHASSIS] != NULL)
                            {
                                if( 0xFF == ((int(*)(int))g_PDKHandle[PDK_BEFOREPOWERCYCLECHASSIS]) (BMCInst))
                                {
                                    break;
                                }
                            }


                            /* We have to set  the ACPI power state properly durin the power cycle	to clear the PEF postpone timer */
                            SetACPIState (IPMI_ACPI_S5,BMCInst);
                            if (pBMCInfo->IpmiConfig.APMLSupport)
                            { 
                                if (GetLibrarySymbol(&dlHandle, "ApmlClose", APML_LIB_PATH, &ApmlClose) == 0)
                                {
                                    ((int(*)(int))ApmlClose)(BMCInst);
                                    dlclose(dlHandle);
                                }
                            }

                            if(g_PDKHandle[PDK_POWERCYCLECHASSIS] != NULL)
                            {
                                ((int(*)(int))g_PDKHandle[PDK_POWERCYCLECHASSIS]) (BMCInst);
                            }
                            SetACPIState (IPMI_ACPI_S0,BMCInst);

                            if(g_PDKHandle[PDK_AFTERPOWERCYCLECHASSIS] != NULL)
                            {
                                ((void(*)(int))g_PDKHandle[PDK_AFTERPOWERCYCLECHASSIS]) (BMCInst);
                            }
                            if (pBMCInfo->IpmiConfig.APMLSupport)
                            { 
                                if (GetLibrarySymbol(&dlHandle, "ApmlInit", APML_LIB_PATH, &ApmlInit) == 0)
                                {
                                    ((int(*)(int))ApmlInit)(BMCInst);
                                    dlclose(dlHandle);
                                }
                            }

                            /* We have to override the LAN access volatile setting when system is power cycle  */
                            InitChVolatilesettings(BMCInst);
                            break;

                        case CHASSIS_POWER_RESET:

                            if(g_PDKHandle[PDK_BEFORERESETCHASSIS] != NULL)
                            {
                                if( 0xFF == ((int(*)(int))g_PDKHandle[PDK_BEFORERESETCHASSIS]) (BMCInst))
                                {
                                    break;
                                }
                            }

                            /* We have to set  the ACPI power state properly durin the power reset in BMC  to clear the PEF postpone timer */
                            SetACPIState (IPMI_ACPI_LEGACY_ON,BMCInst);
                            if (pBMCInfo->IpmiConfig.APMLSupport)
                            { 
                                if (GetLibrarySymbol(&dlHandle, "ApmlClose", APML_LIB_PATH, &ApmlClose) == 0)
                                {
                                    ((int(*)(int))ApmlClose)(BMCInst);
                                    dlclose(dlHandle);
                                }
                            }
                            
                            if(pBMCInfo->TriggerEvent.SysResetFlag)
                            {
                                if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE] != NULL)
                                {
                                    if( 0 > ((int(*)(int , int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE])(AUTO_VIDEO_RECORD_CMD, 0) )
                                    {
                                        IPMI_ERROR("Error in Writing data to Adviser\n");
                                    }
                                    else
                                        TDBG("Signal Sent Successfully to adviser to start video recording\n");                       
                                }
                                else
                                {
                                    TDBG("Signal Not Sent Successfully to adviser to start video recording\n");
                                }
                            }
                            if(pBMCInfo->SOLTriggerEvent.SysResetFlag)
                            {
                                    if(0 == send_signal_by_name("uartmirrorlog",SIGUSR2))
                                    {
                                        TDBG("Signal Sent successfully to UART Mirror Logging application for SOL archive\n");
                                    }
                                    else
                                    {
                                        TDBG("Cannot Send Signal to adviser to UART Mirror Logging application for SOL archive\n");
                                    }
                            }

                            if(g_PDKHandle[PDK_RESETCHASSIS] != NULL)
                            {
                                ((int(*)(int))g_PDKHandle[PDK_RESETCHASSIS]) (BMCInst);
                            }
                            SetACPIState (IPMI_ACPI_S0,BMCInst);
                            if (pBMCInfo->IpmiConfig.APMLSupport)
                            { 
                                if (GetLibrarySymbol(&dlHandle, "ApmlInit", APML_LIB_PATH, &ApmlInit) == 0)
                                {
                                    ((int(*)(int))ApmlInit)(BMCInst);
                                    dlclose(dlHandle);
                                }
                            }

                            /* Acquire Shared memory   */ 
                            OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CCSharedMemMutex, SHARED_MEM_TIMEOUT);

                            if(g_PDKHandle[PDK_AFTERRESETCHASSIS] != NULL)
                            {
                                ((void(*)(int))g_PDKHandle[PDK_AFTERRESETCHASSIS]) (BMCInst);
                            }
                            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CCSharedMemMutex);

                            /* We have to override the LAN access volatile setting when system is power reset  */				
                            InitChVolatilesettings(BMCInst);
                            break;

                        case CHASSIS_DIAGNOSTIC_INTERRUPT :

                            if(g_PDKHandle[PDK_BEFOREDIAGINTERRUPT] != NULL)
                            {
                                if( 0xFF == ((int(*)(int))g_PDKHandle[PDK_BEFOREDIAGINTERRUPT]) (BMCInst))
                                {
                                    break;
                                }
                            }

                            if(g_PDKHandle[PDK_DIAGINTERRUPTCHASSIS] != NULL)
                            {
                                ((int(*)(int))g_PDKHandle[PDK_DIAGINTERRUPTCHASSIS]) (BMCInst);
                            }

                            if(g_PDKHandle[PDK_AFTERDIAGINTERRUPT] != NULL)
                            {
                                ((void(*)(int))g_PDKHandle[PDK_AFTERDIAGINTERRUPT]) (BMCInst);
                            }
                            break;

                        case CHASSIS_SOFT_OFF:

                            if (TRUE != PSGood)
                            {
                              if(g_corefeatures.ssi_support == ENABLED)
                              {
                                /* Power is already off, no need to notify power control code.
                                 * Do queue Operational State condition.
                                 * If Operational State Machine already thinks power is already off,
                                 * this condition will just get ignored. */
                                if (g_SSIHandle[SSICB_QUEUECOND] != NULL)
                                {
                                    ((STATUS(*)(INT8U, INT8U, INT8U, int))g_SSIHandle[SSICB_QUEUECOND]) (DEFAULT_FRU_DEV_ID, COND_POWER_ON, 0, BMCInst);
                                }

                                LOCK_BMC_SHARED_MEM(BMCInst);
                                BMC_GET_SHARED_MEM(BMCInst)->PowerActionInProgress = FALSE;
                                UNLOCK_BMC_SHARED_MEM(BMCInst);
                              }
                                break;
                            }

                            if(g_PDKHandle[PDK_BEFORESOFTOFFCHASSIS] != NULL)
                            {
                                if( 0xFF == ((int(*)(int))g_PDKHandle[PDK_BEFORESOFTOFFCHASSIS]) (BMCInst))
                                {
                                    break;
                                }
                            }
                            SetACPIState (IPMI_ACPI_S5,BMCInst);
                            if (pBMCInfo->IpmiConfig.APMLSupport)
                            { 
                                if (GetLibrarySymbol(&dlHandle, "ApmlClose", APML_LIB_PATH, &ApmlClose) == 0)
                                {
                                    ((int(*)(int))ApmlClose)(BMCInst);
                                    dlclose(dlHandle);
                                }
                            }

                        if(g_PDKHandle[PDK_SOFTOFFCHASSIS] != NULL)
                        {
                            ((void(*)(int))g_PDKHandle[PDK_SOFTOFFCHASSIS]) (BMCInst);
                        }

                        if(g_PDKHandle[PDK_AFTERSOFTOFFCHASSIS] != NULL)
                        {
                            ((void(*)(int))g_PDKHandle[PDK_AFTERSOFTOFFCHASSIS]) (BMCInst);
                        }

                        /* We have to override the LAN access volatile setting when system is power off  */				
                        InitChVolatilesettings(BMCInst);					 
                        break;

                        case CHASSIS_SMI_INTERRUPT:
                            if(g_PDKHandle[PDK_BEFORESMIINTERRUPT] != NULL)
                            {
                                if( 0xFF == ((int(*)(int))g_PDKHandle[PDK_BEFORESMIINTERRUPT]) (BMCInst))
                                {
                                    break;
                                }
                            }

                            if(g_PDKHandle[PDK_SMIINTERRUPTCHASSIS] != NULL)
                            {
                                ((int(*)(int))g_PDKHandle[PDK_SMIINTERRUPTCHASSIS]) (BMCInst);
                            }

                            if(g_PDKHandle[PDK_AFTERSMIINTERRUPT] != NULL)
                            {
                                ((void(*)(int))g_PDKHandle[PDK_AFTERSMIINTERRUPT]) (BMCInst);
                            }
                            break;

                        default:
                            IPMI_DBG_PRINT ("ChassisCtrl.c : Invalid Chassis control action\n");
                            break;
                    } /*switch (Action) chassis control action */
                    break;
                } /* case CHASSIS_CTRL_ACTION: */

            case ON_SYSTEM_EVENT_DETECTED:
                {
                    State = *((INT32U*)&MsgPkt.Data [0]);
                    if(g_PDKHandle[PDK_ONSYSTEMEVENTDETECTED] != NULL)
                    {
                        ((int(*)(INT32U,int))g_PDKHandle[PDK_ONSYSTEMEVENTDETECTED]) (State,BMCInst);
                    }
                    break;
                }

                // add power event detection so that we can use it to set the ACPI state via this event
                case ON_POWER_EVENT_DETECTED:
                SetACPIState (MsgPkt.Data[0],BMCInst);
                break;

                // on the restart cause change
                case ON_SET_RESTART_CAUSE:
                OnSetRestartCause(MsgPkt.Data[0], MsgPkt.Data[1],BMCInst);
                break;

                default :
                IPMI_DBG_PRINT_1 ("ChassisCtrl.c : Invalid Chassis task parameter %d\n", (int)MsgPkt.Param);

            } /*switch (MsgPkt.Param) */
        } /* if (0 == GetMsg (&MsgPkt, hChassis_Q, WAIT_INFINITE))*/

    } /* while (TRUE) */
}

/**
 * @brief Post message to Chassis Task.
 * @param State - State value to be posted.
**/
void
PostToChassisTask (INT32U State,int BMCInst)
{
    MsgPkt_T	MsgPkt;

    MsgPkt.Param	= ON_SYSTEM_EVENT_DETECTED;
    MsgPkt.Size		= sizeof(State) + sizeof(INT8U);
    *((INT32U*)&MsgPkt.Data [0]) = State;
    MsgPkt.Data[sizeof(State)] = BMCInst;
    PostMsg (&MsgPkt, CHASSIS_CTRL_Q, COMMON_QUEUE);
}


/**
 * @fn OnPwrBtn
 * @brief Power Button event handler.
**/
void
OnPwrBtn (int BMCInst)
{
    PostToChassisTask (FP_PWR_BTN_ACTIVE,BMCInst);
}


/**
 * @fn OnResetBtn
 * @brief Reset button event handler.
**/
void
OnResetBtn (int BMCInst)
{
    PostToChassisTask (FP_RST_BTN_ACTIVE,BMCInst);
}

/**
 * @fn OnNMI
 * @brief NMI button event handler.
**/
void
OnNMIBtn (int BMCInst)
{
    PostToChassisTask (FP_NMI_BTN_ACTIVE,BMCInst);
}

/**
 * @brief S5 state event handler.
**/
void
OnS5State (int BMCInst)
{
    PostToChassisTask (S5_SIGNAL_ACTIVE,BMCInst);
}


/**
 * @brief S1 State event handler.
**/
void
OnS1State (int BMCInst)
{
    PostToChassisTask (S1_SIGNAL_ACTIVE,BMCInst);
}


/**
 * @brief Processor-Hot event handler.
**/
void
OnProcHot (int BMCInst)
{
    PostToChassisTask (PROC_HOT_ACTIVE,BMCInst);
}


/**
 * @brief Thermal-trip event handler.
**/
void
OnThermalTrip (int BMCInst)
{
    PostToChassisTask (THERMAL_TRIP_ACTIVE,BMCInst);
}


/**
 * @brief FRB3 Timer event handler.
**/
void
OnFRB3Timer (int BMCInst)
{
    PostToChassisTask (FRB3_TIMER_HALT,BMCInst);
}

/**
 * @brief Set ACPI Power State.
 * @param State - Power State.
**/
void
SetACPIState (INT8U State,int BMCInst)
{
    MsgPkt_T                MsgPkt;
    SetACPIPwrStateReq_T*   pACPIReq;

    IPMI_DBG_PRINT ("ChassisCtrl.c : Changing system ACPI power state\n");

    /* Frame a Set ACPI power state Request */
    pACPIReq = (SetACPIPwrStateReq_T*)MsgPkt.Data;
    pACPIReq->ACPISysPwrState = State | ACPI_SET_SYS_PWR_STATE;
    pACPIReq->ACPIDevPwrState = 0;

    /* Frame a Interface request to Message Handler Queue */
    MsgPkt.Param        = PARAM_IFC;
    MsgPkt.Cmd          = CMD_SET_ACPI_PWR_STATE;
    MsgPkt.NetFnLUN     = NETFN_APP << 2;
    MsgPkt.Privilege    = PRIV_LOCAL;
    MsgPkt.Channel      = SYS_IFC_CHANNEL;
    MsgPkt.Size         = sizeof (SetACPIPwrStateReq_T);
    MsgPkt.SrcQ[0] = '\0';

    /* Post the Request to Message Handler Queue to Set ACPI power state */
    PostMsg (&MsgPkt,MSG_HNDLR_Q, BMCInst);
}

/*-------------------------------------
 * OnSetRestartCause
 *-------------------------------------*/
void OnSetRestartCause(INT8U u8SysRestartCause, INT8U u8MadeChange, int BMCInst)
{
    BootOptions_T*  pBootOptions;
    bool			bRestart = TRUE;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    IPMI_DBG_PRINT_1 ("ChassisCtrl.c : OnSetRestartCause %d\n", u8SysRestartCause);

    // boot option is in the shared memory
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->RestartcauseMutex, SHARED_MEM_TIMEOUT);

    // save the indication of the restart action before the actual execution of the restart,
    // don't make any changes if the message is from the PWR_GGOD deassertion detection
    if (u8SysRestartCause != RESTART_CAUSE_POWER_DOWN)
    {
        BMC_GET_SHARED_MEM(BMCInst)->u8MadeChange = u8MadeChange;	/* Buffer overflow: false positive */
    }

    // get the semi-volatile value of the BMC boot flag valid bit clearing
    pBootOptions = &(BMC_GET_SHARED_MEM(BMCInst)->sBootOptions);

    /* We shoudl not the clear the ValidBoot flag depend upon the paramter #3. Its BIOS reponsiblity to clear this*/
    // see IPMI Spec Table 28-14 selector#5.
    switch (u8SysRestartCause)
    {
        case RESTART_CAUSE_POWER_BUTTON:
            BMC_GET_SHARED_MEM(BMCInst)->SysRestartCaused= TRUE;	/* Buffer overflow: false positive */
            if ((pBootOptions->BootFlagValidBitClearing & BMC_BOOT_FLAG_POWER_BUTTON) == 0)
            {
                (pBootOptions->BootFlags).BootFlagsValid = (pBootOptions->BootFlags).BootFlagsValid & 0x3f;
            }
            break;

        case RESTART_CAUSE_RESET_BUTTON:
        case RESTART_CAUSE_SOFT_RESET:
            BMC_GET_SHARED_MEM(BMCInst)->SysRestartCaused= TRUE;	/* Buffer overflow: false positive */
            if ((pBootOptions->BootFlagValidBitClearing & BMC_BOOT_FLAG_RESET_BUTTON) == 0)
            {
                (pBootOptions->BootFlags).BootFlagsValid = (pBootOptions->BootFlags).BootFlagsValid & 0x3f;
            }
            break;

        case RESTART_CAUSE_WDT_EXPIRATION:
            BMC_GET_SHARED_MEM(BMCInst)->SysRestartCaused= TRUE;	/* Buffer overflow: false positive */
            if ((pBootOptions->BootFlagValidBitClearing & BMC_BOOT_FLAG_WDT_EXPIRATION) == 0)
            {
                (pBootOptions->BootFlags).BootFlagsValid = (pBootOptions->BootFlags).BootFlagsValid & 0x3f;
            }
            break;

        case RESTART_CAUSE_CHASSIS_CTRL:
            BMC_GET_SHARED_MEM(BMCInst)->SysRestartCaused= TRUE;	/* Buffer overflow: false positive */
            break;

        case RESTART_CAUSE_PEF_RESET:
        case RESTART_CAUSE_PEF_POWER_CYCLE:
            BMC_GET_SHARED_MEM(BMCInst)->SysRestartCaused= TRUE;	/* Buffer overflow: false positive */
            if ((pBootOptions->BootFlagValidBitClearing & BMC_BOOT_FLAG_PEF) == 0)
            {
                (pBootOptions->BootFlags).BootFlagsValid = (pBootOptions->BootFlags).BootFlagsValid & 0x3f;
            }
            break;

        case RESTART_CAUSE_OEM:
        case RESTART_CAUSE_AUTO_ALWAYS_ON:
        case RESTART_CAUSE_AUTO_PREV_STATE:
        case RESTART_CAUSE_POWER_DOWN:
            BMC_GET_SHARED_MEM(BMCInst)->SysRestartCaused= TRUE;	/* Buffer overflow: false positive */
            break;

        default:
            // do nothing for other cases
            bRestart = FALSE;
            break;
    }

    // if a system reset, the BMC will go to the "Set Complete" state
    if (bRestart == TRUE)
    {
        pBootOptions->u8SetInProgress = BMC_BOOT_OPTION_SET_COMPLETE;
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->RestartcauseMutex);
}



void InitChVolatilesettings(int BMCInst)
{
    INT8U                   index,LANIndex=0;
    //_FAR_ BMCSharedMem_T*   pSharedMem;
    ChannelInfo_T *pChannelInfo=NULL;
    ChannelInfo_T *pNVRChannelInfo =NULL;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    //pSharedMem = BMC_GET_SHARED_MEM (BMCInst);

    /* Acquire Shared memory   */ 
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CCSharedMemMutex, SHARED_MEM_TIMEOUT);

    for(index=0;index<MAX_NUM_CHANNELS;index++)
    {
        if(pBMCInfo->NVRChcfgInfo[index].ChType != 0xff && pBMCInfo->ChConfig[index].ChType != 0xff)
        {
            pNVRChannelInfo = &pBMCInfo->NVRChcfgInfo[index].ChannelInfo;
            pChannelInfo = &pBMCInfo->ChConfig[index].ChannelInfo;
            pChannelInfo->Alerting = pNVRChannelInfo->Alerting;
            pChannelInfo->PerMessageAuth = pNVRChannelInfo->PerMessageAuth;
            pChannelInfo->UserLevelAuth = pNVRChannelInfo->UserLevelAuth;
            pChannelInfo->AccessMode = pNVRChannelInfo->AccessMode;
            pChannelInfo->MaxPrivilege = pNVRChannelInfo->MaxPrivilege;
            
            if(IsLANChannel(pChannelInfo->ChannelNumber, BMCInst))
            {
                _fmemcpy (pChannelInfo->AuthType,
                    &(pBMCInfo->LANCfs[LANIndex].AuthTypeEnables), sizeof(AuthTypeEnables_T));            
                LANIndex++;
            }

            if (pChannelInfo->ChannelNumber == pBMCInfo->SERIALch && pBMCInfo->IpmiConfig.SerialIfcSupport == 1)
            {
                _fmemcpy (pChannelInfo->AuthType,
                    &pBMCInfo->SMConfig.AuthTypeEnable, sizeof(AuthTypeEnables_T));
            }
            
            pChannelInfo->ChannelIndexRam=index;
            pChannelInfo->ChannelIndexNvRam=index;
        }
    }

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CCSharedMemMutex);


}


