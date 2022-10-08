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
 * PEFPTmr.c
 * PEF Postpone Timer
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "IPMI_Main.h"
#include "SharedMem.h"
#include "Debug.h"
#include "PEFTmr.h"
#include "NVRAccess.h"
#include "PEF.h"
#include "Ethaddr.h"
#include "IPMIConf.h"
#include "SEL.h"

#define ACPI_SYS_PWR_STATE_MASK         0x7F

#define ARM_PEF_TEMP_DISABLE        0xFE

/*** Global Variable ***/
//PEFTmrMgr_T           g_PEFTmrMgr;

/**
 * PEFTimerTask
 **/
void
PEFTimerTask (int BMCInst)
{
    PEFRecordDetailsConfig_T* pNVRPEFRecordDetailsConfig;
    INT8U  sysPwrState;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    pNVRPEFRecordDetailsConfig = &pBMCInfo->PEFRecordDetailsConfig;

    LOCK_BMC_SHARED_MEM(BMCInst);
    sysPwrState = BMC_GET_SHARED_MEM(BMCInst)->m_ACPISysPwrState & ACPI_SYS_PWR_STATE_MASK;
    UNLOCK_BMC_SHARED_MEM(BMCInst);

    /* Disable and reset PEF timer whenever System enters a sleep state / is powered down / is reset*/
    if ((sysPwrState == IPMI_ACPI_S1) || (sysPwrState == IPMI_ACPI_S2) || (sysPwrState == IPMI_ACPI_S3) ||
        (sysPwrState == IPMI_ACPI_S4) || (sysPwrState == IPMI_ACPI_S5) || (sysPwrState == IPMI_ACPI_S4_S5) ||
        (sysPwrState == IPMI_ACPI_SLEEPING_S1_S3) || (sysPwrState == IPMI_ACPI_G1_SLEPPING_S1_S4) ||
        (sysPwrState == IPMI_ACPI_G3))
    {
        pBMCInfo->PefConfig.PEFTmrMgr.TmrArmed = FALSE;
        pBMCInfo->PefConfig.PEFTmrMgr.StartTmr = FALSE;
        pBMCInfo->PefConfig.PEFTmrMgr.InitCountDown = 0;
        pBMCInfo->PefConfig.PEFTmrMgr.TmrInterval = 0;
        pBMCInfo->PefConfig.PEFTmrMgr.TakePEFAction = TRUE;
        return;
    }
	
	/* won't go further if PEF is temporary disabled */
    if (g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrInterval == ARM_PEF_TEMP_DISABLE)
        return;
		
    if ((pNVRPEFRecordDetailsConfig->LastSELRecordID != pNVRPEFRecordDetailsConfig->LastSWProcessedEventID) && 
        (TRUE == pBMCInfo->PefConfig.PEFTmrMgr.TmrArmed))
    {
        pBMCInfo->PefConfig.PEFTmrMgr.StartTmr = TRUE;
        pBMCInfo->PefConfig.PEFTmrMgr.TakePEFAction = FALSE;

        IPMI_DBG_PRINT_1 ("PEFTmr Value = %X\n", g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrInterval);

        /* check for timeout and Decrement pre timeout interval */
        if (0 != pBMCInfo->PefConfig.PEFTmrMgr.TmrInterval)
        {
            pBMCInfo->PefConfig.PEFTmrMgr.TmrInterval--;
            return;
        }
        else
        {    /* Timeout Action */
            pBMCInfo->PefConfig.PEFTmrMgr.TmrArmed = FALSE;
            pBMCInfo->PefConfig.PEFTmrMgr.StartTmr = FALSE;
            pBMCInfo->PefConfig.PEFTmrMgr.TakePEFAction = TRUE;

            CheckLastSELRecordID (BMCInst);
        }
    }
    else if (pNVRPEFRecordDetailsConfig->LastSELRecordID == pNVRPEFRecordDetailsConfig->LastSWProcessedEventID)
    {
        pBMCInfo->PefConfig.PEFTmrMgr.StartTmr = FALSE; //stop timer
        pBMCInfo->PefConfig.PEFTmrMgr.TmrInterval = pBMCInfo->PefConfig.PEFTmrMgr.InitCountDown; //rearm timer to initial value
        pBMCInfo->PefConfig.PEFTmrMgr.TmrArmed = TRUE;
        pBMCInfo->PefConfig.PEFTmrMgr.StartTmr = TRUE; //restart timer
        pBMCInfo->PefConfig.PEFTmrMgr.TakePEFAction = FALSE;
        return ;
    }

    if (FALSE == pBMCInfo->PefConfig.PEFTmrMgr.TmrArmed)
    {
        pBMCInfo->PefConfig.PEFTmrMgr.StartTmr = FALSE;
        pBMCInfo->PefConfig.PEFTmrMgr.TakePEFAction = TRUE;
        return;
    }
}


/**
 * PETAckTimerTask
 **/
void
PETAckTimerTask (int BMCInst)
{
    INT8U               EthIndex;
    MsgPkt_T            MsgPkt;
    PETAckTimeOutMgr_T* pPETAckMgr;
    INT16U              i;

    /* lock PETAck Manager */
   LOCK_BMC_SHARED_MEM(BMCInst);
    
    pPETAckMgr = BMC_GET_SHARED_MEM(BMCInst) ->PETAckMgr;

    for (i=0; i<MAX_PET_ACK; i++)
    {
        /* Check Tmr present or not */
        if (FALSE == pPETAckMgr[i].Present)
            continue;
        /* Check timeout value */
        if (0 != pPETAckMgr[i].AckTimeOut)
        {
            pPETAckMgr[i].AckTimeOut--;
            continue;
        }
        /* Check retries */
        if (0 != pPETAckMgr[i].Retries)
        {
            MsgPkt.Param = PARAM_RETRY_ALERT;
            MsgPkt.Size  = sizeof (SELEventRecord_T);
            pPETAckMgr[i].Retries--;

            /* We have to reinitalise the PEF tmr Ack timeout*/
            pPETAckMgr[i].AckTimeOut=pPETAckMgr[i].RetryInterval;
        }
        else
        {
            /* Take timeout action */
            pPETAckMgr[i].Present = FALSE;
            EthIndex= GetEthIndex(pPETAckMgr[i].Channel, BMCInst);

            /* Update the LanAlert Status */
            BMC_GET_SHARED_MEM(BMCInst)->LANAlertStatus[EthIndex] = ALERT_IMM_TIMEOUT_FAILURE;
            MsgPkt.Param = PARAM_PET_NO_ACK;
            MsgPkt.Size  = sizeof (SELEventRecord_T) + sizeof(INT16U);
            /* Copy the sequence number to locate the deferred alert from table */
            *(INT16U *)(MsgPkt.Data + sizeof(SELEventRecord_T)) = pPETAckMgr[i].SequenceNum;
        }
        MsgPkt.Channel= pPETAckMgr[i].Channel ;
        MsgPkt.Channel	 |=( (pPETAckMgr[i].DestSel & 0x0F) << 4);

        _fmemcpy (MsgPkt.Data,(INT8U*)&pPETAckMgr[i].EvtRecord,sizeof (SELEventRecord_T));
        PostMsgNonBlock (&MsgPkt, PEF_TASK_Q,BMCInst);
    }

    UNLOCK_BMC_SHARED_MEM(BMCInst);
}

/**
 * PEFStartDlyTimerTask
 **/
void
PEFStartDlyTimerTask(int BMCInst)
{
	INT8U  sysPwrState = 0;
	static INT8U PresysPwrState = 0;
    static INT8U EnableCountStartupDly = FALSE;
    static INT8U EnableCountAlertStartupDly = FALSE;
    PEFConfig_T* pPEFConfig;
    
	BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

     pPEFConfig = &pBMCInfo->PEFConfig;
	
    
    LOCK_BMC_SHARED_MEM(BMCInst);
    sysPwrState = BMC_GET_SHARED_MEM(BMCInst)->m_ACPISysPwrState & ACPI_SYS_PWR_STATE_MASK;
    UNLOCK_BMC_SHARED_MEM(BMCInst);
      
    if(((sysPwrState == IPMI_ACPI_S0) && ((PresysPwrState == IPMI_ACPI_S4)||(PresysPwrState == IPMI_ACPI_S5)))||(pBMCInfo->PefConfig.PEFTmrMgr.ResetFlag == 1))
    {
        EnableCountStartupDly = TRUE;
        EnableCountAlertStartupDly = TRUE;
        pBMCInfo->PefConfig.PEFTmrMgr.StartDlyTmr = pPEFConfig->PEFStartupDly;
        pBMCInfo->PefConfig.PEFTmrMgr.AlertStartDlyTmr = pPEFConfig->PEFAlertStartupDly;
        pBMCInfo->PefConfig.PEFTmrMgr.ResetFlag = 0;
    }
    
    if(EnableCountStartupDly == TRUE && pBMCInfo->PefConfig.PEFTmrMgr.StartDlyTmr > 0)
    {
        pBMCInfo->PefConfig.PEFTmrMgr.StartDlyTmr--;
    }

    if(EnableCountAlertStartupDly == TRUE && pBMCInfo->PefConfig.PEFTmrMgr.AlertStartDlyTmr > 0)
    {
        pBMCInfo->PefConfig.PEFTmrMgr.AlertStartDlyTmr--;
    }
    
    if( pBMCInfo->PefConfig.PEFTmrMgr.StartDlyTmr == 0 )
        EnableCountStartupDly = FALSE;
    
    if(pBMCInfo->PefConfig.PEFTmrMgr.AlertStartDlyTmr == 0)
        EnableCountAlertStartupDly = FALSE;

	PresysPwrState = sysPwrState;
   
}
