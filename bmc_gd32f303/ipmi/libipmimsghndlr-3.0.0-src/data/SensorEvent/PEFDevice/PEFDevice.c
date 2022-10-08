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
 * PEFDevice.c
 * PEF Commands Handler
 *
 * Author: Govind Kothandapani <govindk@ami.com> 
 *       : Rama Bisa <ramab@ami.com>
 *       : Basavaraj Astekar <basavaraja@ami.com>
 *       : Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "PEFDevice.h"
#include "Support.h"
#include "Debug.h"
#include "MsgHndlr.h"
#include "IPMIDefs.h"
#include "NVRAccess.h"
#include "SharedMem.h"
#include "IPMI_PEF.h"
#include "IPMI_IPM.h"
#include "PEFTmr.h"
#include "PEF.h"
#include "SEL.h"
#include "IPMI_Main.h"
#include "Util.h"
#include"Ethaddr.h"
#include "IPMIConf.h"

/* Reserved bit macro definitions */
#define RESERVED_BITS_SETLASTPROCESSEDEVENTID           0xFE //(BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0)
#define RESERVED_BITS_ALERTIMMEDIATE_CH                 0xF0 //(BIT7 | BIT6 | BIT5 |BIT4)
#define RESERVED_BITS_ALERTIMMEDIATE_DESTSEL            0x30 //(BIT5 |BIT4)

#if PEF_DEVICE == 1

/*** Local Macro Definitions ***/
#define IPMI_PEF_VERSION            0x51
#define ARM_PEF_TMR_DISABLE         0x00
#define ARM_PEF_TEMP_DISABLE        0xFE
#define ARM_PEF_TMR_REQ_GET_TIME    0xFF

#define ALERT_IMD_OPERATION         0x80

#define ACT_SUPPORT_DIAG_INT        0x20
#define ACT_SUPPORT_OEM_ACTION      0x10
#define ACT_SUPPORT_POWER_CYC       0x08
#define ACT_SUPPORT_RESET           0x04
#define ACT_SUPPORT_POWER_DOWN      0x02
#define ACT_SUPPORT_ALERT           0x01

#define SET_IN_PROGRESS                 0

#define PEF_SET_COMPLETE                0
#define PEF_SET_IN_PROGRESS             1
#define PEF_COMMIT_WRITE                2

#define BIT4_BIT0_MASK             0x1f
#define BIT1_BIT0_MASK             0x3
#define RESERVED_VALUE_03      0x3
#define RESERVED_VALUE_01      0x1
#define PRECONFIGURED_FILTER   0x2
#define BIT7_BIT6_MASK         0xC0

#define RESERVED_SET_IN_PROGRESS   0x03

/* Reserved Bits */
#define RESERVED_VALUE_80			0x80
/*** Module Variables ***/

static const INT8U  m_PEFReqLen[] =
    {1, 1, 1, 1, 1, 1, 21, 2, 1, 4, 17, 1, 3}; /**< Contains PEF Coniguration parameter Length */

/* Reserved Bits table */
#define MAX_PEF_PARAMS_DATA  20
typedef struct
{
	INT8U	Params;
	INT8U	ReservedBits [MAX_PEF_PARAMS_DATA];
	INT8U	DataLen;
	
} PEFCfgRsvdBits_T;

static PEFCfgRsvdBits_T m_RsvdBitsCheck [] = {

    /* Param        Reserved Bits      Data Size   */
    { 0x00,              {0xFC},                 	0x01    },
    { 0x01,	             {0xF0 }, 					0x01    },		/* Volatile PEF ACtion global control */
    { 0x02,		         {0xC0 }, 					0x01	},		/* Non Volatile PEF Action global control */
    { 0x06,              {0x80},                  	0x01    },
    { 0x07,              {0x80},                  	0x01    },
    { 0x09,              {0x80},                  	0x01    },
    { 0x0A,              {0xFE},                  	0x01    },
    { 0x0C,              {0x80,0x80 ,0x80},     	0x03    },
    { 0x0D,              {0x80},     				0x01    }
};



static SELEventRecord_T m_AlertImmRecord = /**< Contains Event Record for Alert Immidiate command */
{
    {
        0xFFFF,             /*Record ID */
        0x2,                /*Type of the record */
        1                   /*Timestamp */
    },
    {
        0x40,               /*Generator ID */
        0x00
    },
    0x4,                    /*Revision*/
    0x12,                   /*Sensor type */
    0x51,                   /*Sensor number */
    0x6f,                   /*Event type */
    0x04,0xFF,0xFF          /*Event Data 1..3 */
};



/*---------------------------------------
 * GetPEFCapabilities
 *---------------------------------------*/
int 
GetPEFCapabilities (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetPEFCapRes_T* pGetPEFCapRes = (GetPEFCapRes_T*)pRes;

    pGetPEFCapRes->CompletionCode   = CC_NORMAL;
    pGetPEFCapRes->PEFVersion       = IPMI_PEF_VERSION;
    pGetPEFCapRes->ActionSupport    = ACT_SUPPORT_DIAG_INT | ACT_SUPPORT_OEM_ACTION | ACT_SUPPORT_POWER_CYC |
                                      ACT_SUPPORT_RESET | ACT_SUPPORT_POWER_DOWN |
                                      ACT_SUPPORT_ALERT,
    pGetPEFCapRes->TotalEntries     = MAX_EVT_FILTER_ENTRIES;

    return sizeof (GetPEFCapRes_T);
}


/*---------------------------------------
 * ArmPEFPostponeTimer
 *---------------------------------------*/
int
ArmPEFPostponeTimer (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    ArmPEFTmrRes_T*  pArmPEFTmrRes = (ArmPEFTmrRes_T*)pRes;
    // PEFConfig_T*     pNVRPEFConfig;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->PefConfig.PEFSharedMemMutex,WAIT_INFINITE);
   // pNVRPEFConfig = &pBMCInfo->PEFConfig;

    switch (*pReq)
    {
        case ARM_PEF_TEMP_DISABLE:
            IPMI_DBG_PRINT ("ARM_PEF_TMR_DISABLE\n");
            /* Disable PEF Temporarily */
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrInterval   = ARM_PEF_TEMP_DISABLE;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.InitCountDown = ARM_PEF_TEMP_DISABLE;

            // Timer Interval and InitCountdown not set to ARM_PEF_TEMP_DISABLE if originally PEF was disabled
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrArmed      = FALSE;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.StartTmr      = FALSE;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TakePEFAction = FALSE;
            break;

        case ARM_PEF_TMR_DISABLE:
            IPMI_DBG_PRINT ("ARM_PEF_TMR_TEMP_DISABLE\n");
            /* Check if PEF Temporary Disable was called earlier and then Disable the PEFTmr */
            if (g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrInterval == ARM_PEF_TEMP_DISABLE)
            {
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TakePEFAction   = TRUE;
            }
            else
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TakePEFAction   = FALSE;

            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrArmed        = FALSE;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.StartTmr        = FALSE;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrInterval     = ARM_PEF_TMR_DISABLE;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.InitCountDown   = ARM_PEF_TMR_DISABLE;
            break;

        case ARM_PEF_TMR_REQ_GET_TIME:
            IPMI_DBG_PRINT ("ARM_PEF_TMR_REQ_GET_TIME\n");
            break;

        default:
            /* Other values correspond to arm timer */
            IPMI_DBG_PRINT ("DEFAULT\n");
            /* Check if PEF Temporary Disable was called earlier and then set the PEFTmr */
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.InitCountDown   = *pReq;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrInterval     = *pReq;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrArmed        = TRUE;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.StartTmr        = FALSE;
            g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TakePEFAction   = FALSE;
    }

    pArmPEFTmrRes->PresentTmrVal    = g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrInterval;
    pArmPEFTmrRes->CompletionCode   = CC_NORMAL;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);

    return sizeof (ArmPEFTmrRes_T);
}


/*---------------------------------------
 * SetPEFConfigParams
 *---------------------------------------*/
int 
SetPEFConfigParams (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    INT8U               ParamSel;
      PEFConfig_T*        pNvrPefConfig;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    int					i = 0, j = 0;
    INT8U tmp=0;
      INT8U  m_PEF_SetInProgress; /**< Contains setting PEF configuration status */
    int ret;
    /* Store ParamSel */
    ParamSel = pReq[0];

    /* Check the Length of the request data */
    if (((ParamSel < 13) && ((ReqLen - 1) != m_PEFReqLen[ParamSel])) || 
         (ReqLen == 0))
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof (*pRes);
    }

    if (pReq[0] & RESERVED_VALUE_80)
    {
        /* Alarm !!! Somebody is trying to set Reseved Bits */
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    /* Check for Unsupported  parameter */
    if (ParamSel > 95)
    {
        *pRes = CC_PEF_PARAM_NOT_SUPPORTED;
        return sizeof (*pRes);
    }
    
    /* Check for Reserved Bits */
    for (i = 0; i < sizeof (m_RsvdBitsCheck)/ sizeof (m_RsvdBitsCheck[0]); i++)
    {
        /* Check if this Parameter Selector needs Reserved bit checking !! */
        if (m_RsvdBitsCheck[i].Params == ParamSel)	
        {
            IPMI_DBG_PRINT_2 ("Param - %x, DataLen - %x\n", ParamSel, m_RsvdBitsCheck[i].DataLen);

            for (j = 0; j < m_RsvdBitsCheck[i].DataLen; j++)
            {
                IPMI_DBG_PRINT_2 ("Cmp  %x,  %x\n", pReq[1+j], m_RsvdBitsCheck[i].ReservedBits[j]);    			
                if ( 0 != (pReq[1+j] & m_RsvdBitsCheck[i].ReservedBits[j]))
                {
                    /* Alarm !!! Somebody is trying to set Reseved Bits */
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof (*pRes);
                }
            }
        }
    }
    if((ParamSel == SET_IN_PROGRESS) && (pReq[1] == RESERVED_SET_IN_PROGRESS))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->PefConfig.PEFSharedMemMutex,WAIT_INFINITE);
    /*Get NVRAM PEF Configuration parameters */
    pNvrPefConfig = &pBMCInfo->PEFConfig;
    if ( g_PDKHandle[PDK_BEFORESETPEFCFG] != NULL )
    {
        ret =((int(*)(INT8U *, INT8U, INT8U *,int))(g_PDKHandle[PDK_BEFORESETPEFCFG]))(pReq, ReqLen,pRes,BMCInst);
        if(ret != 0)
        {
            return ret;
        }
    }

    switch (ParamSel)
    {
        case SET_IN_PROGRESS:
            /*Parameter 0 volatile */
             /* Only Set inProgress and Set Complete are implemented */
            if( (PEF_SET_IN_PROGRESS != pReq[1]) && (PEF_SET_COMPLETE != pReq[1]) )
            {
                *pRes = CC_PARAM_NOT_SUPPORTED;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
            m_PEF_SetInProgress = BMC_GET_SHARED_MEM(BMCInst)->m_PEF_SetInProgress;
            if ((m_PEF_SetInProgress == PEF_SET_IN_PROGRESS) &&
                (pReq[1] == PEF_SET_IN_PROGRESS))
            {
                /* Trying to Set in Progress When already in progress */
                *pRes = CC_PEF_SET_IN_PROGRESS ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
            BMC_GET_SHARED_MEM(BMCInst)->m_PEF_SetInProgress = pReq[1];
            break;

        case PEF_CONTROL:
            /*Parameter 1 NVRAM */
            pNvrPefConfig->PEFControl = pReq[1];
            
            if((pNvrPefConfig->PEFControl & 0x01) && ( g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrInterval == ARM_PEF_TEMP_DISABLE))
            {
            	g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TakePEFAction   = TRUE;
            	g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrArmed        = FALSE;
            	g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.StartTmr        = FALSE;
            	g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.TmrInterval     = ARM_PEF_TMR_DISABLE;
                g_BMCInfo[BMCInst].PefConfig.PEFTmrMgr.InitCountDown   = ARM_PEF_TMR_DISABLE;
            }
            break;

        case PEF_ACTION_CONTROL:
            /*Parameter 2 NVRAM */
            pNvrPefConfig->PEFActionGblControl = pReq[1];
            break;

        case PEF_STARTUP_DELAY:
            /* Parameter 3 NVRAM */
            pNvrPefConfig->PEFStartupDly = pReq[1];
            break;

        case PEF_ALERT_STARTUP_DELAY:
            /*Parameter 4 NVRAM */
            pNvrPefConfig->PEFAlertStartupDly = pReq[1];
            break;

        case NUM_EVT_ENTRIES:
            /* Parameter 5 Read only */
            *pRes = CC_PEF_ATTEMPT_TO_SET_READ_ONLY_PARAM;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
            return sizeof (*pRes);
            break;

        case EVT_ENTRY:
            /* Parameter 6 NVRAM*/
            /* set selector 0 is reserved for this parameter */
            if (0 == pReq[1])  
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            if(pReq [1] > MAX_EVT_FILTER_ENTRIES)
            {
                *pRes=CC_PARAM_OUT_OF_RANGE;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof(*pRes);
            }
            /* Make set selector 0 based for array access */
            pReq[1]--;


            /* In Event filter table, Data1  BIT0-BIT4 are reserved  */
            /* In BIT6 ,BIT5	01B and 11B are reserved */
            if(((pReq[2] & BIT4_BIT0_MASK) != 0) || (((pReq[2] >> 5) & BIT1_BIT0_MASK )== RESERVED_VALUE_03 ||((pReq[2] >> 5) & BIT1_BIT0_MASK )== RESERVED_VALUE_01 ))
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }


            /* We cannot configure  the Preconfigured filter entry */
            /* s/w cannot add the preconfigureable Event filter */
            //188564 - Per FW architect's interpretation, this flag should be settable and clearable
            //                 by system software. It shall serve as a warning to other software that the entry 
            //                 was set purposefully, but shall not interfere with resetting the entry.
            //                 This is an interpretation of a "gray area" in the IPMI spec.
            if(pBMCInfo->IpmiConfig.AlterPreConfiguredEntries == 0)
            {
                if(PRECONFIGURED_FILTER == GetBits(pReq[2],0x60) || PRECONFIGURED_FILTER == GetBits(pNvrPefConfig->EvtFilterTblEntry[pReq[1]].FilterConfig,0x60))
                {
                    *pRes = CC_INV_DATA_FIELD ;
                    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                    return sizeof (*pRes);
                }
            }

            /* Severity Should be 0 2 4 8 10 0x20 */
            /* Logic : 2's complement of  2 power n is same given no */
            tmp=pReq[5];
            if((((~tmp) + 1) & pReq[5]) != pReq[5])
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            /* According to the Severity 0x40 and 0x80 is undefined */
            if(0!=(pReq[5] & BIT7_BIT6_MASK))
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            /*Set respective Event Filter Entry*/
            _fmemcpy(&pNvrPefConfig->EvtFilterTblEntry[pReq[1]],
                     &pReq[2],
                     sizeof (EvtFilterTblEntry_T));
            break;

        case EVT_ENTRY_BYTE1:
            /* Parameter 7 NVRAM*/
            /* set selector 0 is reserved for this parameter */
            if (0 == pReq[1])
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
            /* Make set selector 0 based for array access */
            pReq[1]--;

            /* In Event filter table, Data1  BIT0-BIT4 are reserved  */
            /* In BIT6 ,BIT5  01B and 11B are reserved */
            if(((pReq[2] & 0x1f) != 0) || (((pReq[2] >> 5) & 0x3 )== 0x3 ||((pReq[2] >>5) & 0x3 )== 0x1 ))
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            if( GetBits(pReq[2],0x60)  != GetBits( pNvrPefConfig->EvtFilterTblEntry[pReq[1]].FilterConfig ,0x60))
            {
                /* We should not change the Event filter Configuration (i.e)  s/w configurable , Preconfigurable Event filter */ 		
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
            /* Load first byte(Filter config) only */
            pNvrPefConfig->EvtFilterTblEntry[pReq[1]].FilterConfig = pReq[2];
            break;

        case GUID:
            /* Parameter 10 NVRAM */
            /* Fill the data that describes the use of GUID field */
            _fmemcpy (pNvrPefConfig->SystemGUID, &pReq[1], MAX_SIZE_PET_GUID);
            break;

        case NUM_ALERT_ENTRIES:
            /* Parameter 8 NVRAM */
            *pRes = CC_PEF_ATTEMPT_TO_SET_READ_ONLY_PARAM;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
            return sizeof (*pRes);
            break;
    
        case ALERT_ENTRY:
            /* Parameter 9 NVRAM */
            /* set selector 0 is reserved for this parameter */
            /*  Invalid Policy  .It should be 0-4  */
            /* Policy number 0 is reserved */
            if( (0 == pReq[1]) ||( ( pReq[2]  & 0x7  ) > 4) || ((pReq[2] & 0xF0) == 0 ))
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
	    /* Alert Policy Entry Number should be less than Maximum Alert Policies Supported*/
	    if((pReq[1] & 0x7F) > MAX_ALERT_POLICY_ENTRIES) 
	    {
        	*pRes = CC_PARAM_OUT_OF_RANGE ;
	        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
       		return sizeof (*pRes);
	    }	    
            /* Make set selector 0 based for array access */
            pReq[1]--;
            
            /*Set specified Alert Entry */
            _fmemcpy(&pNvrPefConfig->AlertPolicyTblEntry[pReq[1]],
                     &pReq[2],
                     sizeof (AlertPolicyTblEntry_T));
            break;

        case NUM_ALERT_STR:
            /* Parameter 11 Read only*/
            *pRes = CC_PEF_ATTEMPT_TO_SET_READ_ONLY_PARAM;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
            return sizeof (*pRes);
            break;

        case ALERT_STRING_KEYS:
            /* parameter 12 */
            /* if Set selector is 0 then set for volatile else non volatile */

            /* Boundary  condtion for check Set selector within MAx_ALERT_STR */
            if(pReq[1]> MAX_ALERT_STRINGS)
            {
                *pRes = CC_PARAM_OUT_OF_RANGE ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            if( (pReq[2] > MAX_EVT_FILTER_ENTRIES) || (pReq[3] >MAX_ALERT_STRINGS))
            {
                *pRes = CC_PARAM_OUT_OF_RANGE;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

             if (0 != pReq[1])
            {
                /* Make set selector 0 based for array access */
                pReq[1]--;
                /*Set the Event Filter No & Alert string Set parameters */
                pNvrPefConfig->AlertStringEntry[pReq[1]].EventFilterSel = pReq[2];
                pNvrPefConfig->AlertStringEntry[pReq[1]].AlertStringSet = pReq[3];
            }
            else
            {
                /* Volatile settings */
                BMC_GET_SHARED_MEM (BMCInst)->AlertStringEntry.EventFilterSel = pReq[2];
                BMC_GET_SHARED_MEM (BMCInst)->AlertStringEntry.AlertStringSet = pReq[3];
            }
            break;

        case ALERT_STRINGS:
            /* Parameter 13 NVRAM */
            if(ReqLen < 4)
            {
                *pRes = CC_REQ_INV_LEN;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            /* Boundary  condtion for check Set selector within MAx_ALERT_STR */
            if(pReq[1]> MAX_ALERT_STRINGS)
            {
                *pRes = CC_PARAM_OUT_OF_RANGE ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
            /* Boundary condition to check the block selector within MAX_LIMIT */

            if(0 == pReq[2] || pReq[2] >ALERT_STR_MAX_BLOCKS)
            {
                *pRes = CC_PARAM_OUT_OF_RANGE ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            /* Boundary Condition to check Alert string with in Limit */
            if(ReqLen -3 > ALERT_STR_BLOCK_SIZE)
            {
                *pRes = CC_REQ_INV_LEN ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
            /* Make Block selector 0 based for array access */
            pReq[2]--;

            /* if Set selector is 0 then set for volatile else non volatile */
            if (0 != pReq[1])
            {
                /* Make set selector 0 based for array access */
                pReq[1]--;
                /* Load the appropriate block */
                memset(pNvrPefConfig->AlertStringEntry[pReq[1]].AlertString[pReq[2]],0,ALERT_STR_BLOCK_SIZE);

                _fmemcpy(pNvrPefConfig->AlertStringEntry[pReq[1]].AlertString[pReq[2]],
                &pReq[3],
                ReqLen - 3);
            }
            else
            {
                /*  volatile setting */
                AlertStringTbl_T* pAlertStr = &BMC_GET_SHARED_MEM (BMCInst)->AlertStringEntry;
                memset(pAlertStr->AlertString[pReq[2]],0,ALERT_STR_BLOCK_SIZE);
                _fmemcpy(pAlertStr->AlertString[pReq[2]], &pReq[3], ReqLen - 3);
            }
            break;

        default:
            *pRes = CC_INV_DATA_FIELD;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
            return sizeof (*pRes);
    }

    FlushIPMI((INT8U*)&pBMCInfo->PEFConfig,(INT8U*)&pBMCInfo->PEFConfig,
                     pBMCInfo->IPMIConfLoc.PEFConfigAddr,sizeof(PEFConfig_T),BMCInst);
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
    *pRes = CC_NORMAL;
    return sizeof (*pRes);
}


/*---------------------------------------
 * GetPEFConfigParams
 *---------------------------------------*/
int
GetPEFConfigParams (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetPEFConfigReq_T*  pGetConfigReq = (GetPEFConfigReq_T*)pReq;
      PEFConfig_T*        pNvrPefConfig;
    INT8U               ParamSel, Index=0;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    /* Store ParamSel */
    ParamSel = pGetConfigReq->ParamSel & 0x7F;

    /* Check for Unsupported  parameter */
    if (ParamSel > 95)
    {
        *pRes = CC_PEF_PARAM_NOT_SUPPORTED;
        return sizeof (*pRes);
    }

    /*Load completion code & revision */
    pRes[Index++] = CC_NORMAL;
    pRes[Index++] = PARAMETER_REVISION_FORMAT ;

    /* Check Revision only parameter */
    if (pGetConfigReq->ParamSel & 0x80)
    {
        if((MAX_PEF_CONF_PARAM < ParamSel))
        {
            *pRes = CC_PARAM_NOT_SUPPORTED;
            return sizeof (*pRes);
        }
        return (Index * sizeof (INT8U));
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->PefConfig.PEFSharedMemMutex,WAIT_INFINITE);
    /* Get Pef config parameters from NVRAM */
    pNvrPefConfig = &pBMCInfo->PEFConfig;

    switch (ParamSel)
    {
        case EVT_ENTRY:
            /*Parameter 6 NVRAM */
        case EVT_ENTRY_BYTE1:
            /*Parameter 7 NVRAM */
        case ALERT_ENTRY:
            /*Parameter 9 NVRAM */
        case ALERT_STRING_KEYS:
            /*Parameter 12 NVRAM */
        case  ALERT_STRINGS:
            /*Parameter 13 NVRAM */
            break;
        default:
            if((pGetConfigReq->SetSel != 0) || (pGetConfigReq->BlockSel !=0))
            {
                *pRes = CC_INV_DATA_FIELD ;
                return sizeof (*pRes);
            }
            break;

    }

    switch (ParamSel)
    {
        case SET_IN_PROGRESS:
            /*Parameter 0 volatile*/
            pRes[Index++] = BMC_GET_SHARED_MEM(BMCInst)->m_PEF_SetInProgress;
            break;

        case PEF_CONTROL:
            /*Parameter 1 Non volatile*/
            pRes[Index++] = pNvrPefConfig->PEFControl;
            break;

        case PEF_ACTION_CONTROL:
            /* Parameter 2 NVRAM */
            pRes[Index++] = pNvrPefConfig->PEFActionGblControl;
            break;

        case PEF_STARTUP_DELAY:
            /* Parameter 3 NVRAM */
            pRes[Index++] = pNvrPefConfig->PEFStartupDly;
            break;

        case PEF_ALERT_STARTUP_DELAY:
            /*Parameter 4 NVRAM */
            pRes[Index++] = pNvrPefConfig->PEFAlertStartupDly;
            break;

        case NUM_EVT_ENTRIES:
            /*Parameter 5 Read only */
            pRes[Index++] = MAX_EVT_FILTER_ENTRIES;
            break;

        case EVT_ENTRY:
            /*Parameter 6 NVRAM */
            /* set selector 0 is reserved for this parameter */
            /*	Block Selector should be zero for this parameter */
            if ((0 == pGetConfigReq->SetSel) ||(0 !=pGetConfigReq->BlockSel )) 
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }


            if(pGetConfigReq->SetSel > MAX_EVT_FILTER_ENTRIES)
            {
                *pRes = CC_PARAM_OUT_OF_RANGE ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            /*Add set selector */
            pRes[Index++] = pGetConfigReq->SetSel;
            /*Make set selector 0 based for array access*/
            pGetConfigReq->SetSel--;

            /*Fill response with the required parameter from Event Filter Table*/
            _fmemcpy(&pRes[Index],
            &pNvrPefConfig->EvtFilterTblEntry[pReq[1]],
            sizeof (EvtFilterTblEntry_T));
            Index += sizeof (EvtFilterTblEntry_T);
            break;

        case EVT_ENTRY_BYTE1:
            /*Parameter 7 NVRAM */
            /* set selector 0 is reserved for this parameter */
            if (0 == pGetConfigReq->SetSel)
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
            //#30993
            if(pGetConfigReq->SetSel >  MAX_EVT_FILTER_ENTRIES)
            {
                *pRes = CC_PARAM_OUT_OF_RANGE ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            /*	Block Selector should be zero for this parameter */
            if(0 !=pGetConfigReq->BlockSel )
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
            /*Load set selector */
            pRes[Index++] = pGetConfigReq->SetSel;
            /*Make set selector 0 based for array access*/
            pGetConfigReq->SetSel--;

            /*Load filter configuration */
            pRes[Index++] = pNvrPefConfig->EvtFilterTblEntry[pGetConfigReq->SetSel].FilterConfig;
            break;

        case GUID:
            /*Parameter 10 Read only */
            /* Add Global Unique ID and its usage field (first byte) */
            _fmemcpy(&pRes[Index], pNvrPefConfig->SystemGUID, MAX_SIZE_PET_GUID);
            Index += MAX_SIZE_PET_GUID;
            break;

        case NUM_ALERT_ENTRIES:
            /*Parameter 8 Read only */
            pRes[Index++] = MAX_ALERT_POLICY_ENTRIES;
            break;

        case ALERT_ENTRY:
            /*Parameter 9 NVRAM */
            /* set selector 0 is reserved for this parameter */
            /*	Block Selector should be zero for this parameter */
            if ( (0 == pGetConfigReq->SetSel  ) || (0 !=pGetConfigReq->BlockSel ))
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }
            if(MAX_ALERT_POLICY_ENTRIES  < pGetConfigReq->SetSel)
            {
                *pRes = CC_PARAM_OUT_OF_RANGE ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            /* Wehave to check the Block number only If it require the block selector */
            /* Check for block number */
            /* Since Block Selector is 1 Based . we have to support 1-MAX	 */
            if (pGetConfigReq->BlockSel > ALERT_STR_MAX_BLOCKS)
            {
                *pRes = CC_PARAM_OUT_OF_RANGE;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }


            /* Load Set Selector */
            pRes[Index++] = pGetConfigReq->SetSel;
            /*Make set selector 0 based for array access*/
            pGetConfigReq->SetSel--;
            /* Load Alert entry */
            _fmemcpy(&pRes[Index],
            &pNvrPefConfig->AlertPolicyTblEntry[pGetConfigReq->SetSel],
            sizeof (AlertPolicyTblEntry_T));
            Index += sizeof (AlertPolicyTblEntry_T);
            break;

        case NUM_ALERT_STR:
            /*Parameter 11 Read only */
            /*Load Total Strings */
            pRes[Index++] = MAX_ALERT_STRINGS;
            break;

        case ALERT_STRING_KEYS:
            /*Parameter 12 NVRAM */
            if((pGetConfigReq->BlockSel  != 0) || ((pGetConfigReq->SetSel & 0x80) == 0x80))
            {
                *pRes = CC_INV_DATA_FIELD;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            if(pGetConfigReq->SetSel > MAX_ALERT_STRINGS)
            {
                *pRes = CC_PARAM_OUT_OF_RANGE;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            /*Load Alert string keys response */
            pRes[Index++] = pGetConfigReq->SetSel;
            /* if Set selector 0  then volatile setting else non volatile */
            if (0 != pGetConfigReq->SetSel)
            {
                /*Make set selector 0 based for array access*/
                pGetConfigReq->SetSel--;
                pRes[Index++] = pNvrPefConfig->AlertStringEntry[pReq[1]].EventFilterSel;
                pRes[Index++] = pNvrPefConfig->AlertStringEntry[pReq[1]].AlertStringSet; 
            }
            else 
            {
                /* Get setting from volatile */
                pRes[Index++] = BMC_GET_SHARED_MEM (BMCInst)->AlertStringEntry.EventFilterSel;
                pRes[Index++] = BMC_GET_SHARED_MEM (BMCInst)->AlertStringEntry.AlertStringSet; 
            }
            break;

        case  ALERT_STRINGS:
            /*Parameter 13 NVRAM */
            if (pGetConfigReq->BlockSel == 0) 
            {
                *pRes = CC_INV_DATA_FIELD ;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }


            /* Wehave to check the Block number only If it require the block selector */
            /* Check for block number */
            /* Since Block Selector is 1 Based . we have to support 1-MAX	 */
            if ((pGetConfigReq->BlockSel > ALERT_STR_MAX_BLOCKS)  || (pGetConfigReq->SetSel > MAX_ALERT_STRINGS ))
            {
                *pRes = CC_PARAM_OUT_OF_RANGE;
                OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
                return sizeof (*pRes);
            }

            /*Copy Set Selector */
            pRes[Index++] = pGetConfigReq->SetSel;

            /*Copy block selector */
            pRes[Index++] = pGetConfigReq->BlockSel;

            /*Make block selector 0 based for array access*/
            pGetConfigReq->BlockSel--;

            /* if Set selector 0  then volatile setting else non volatile */
            if (0 != pGetConfigReq->SetSel)
            {
                /*Make set selector 0 based for array access*/
                pGetConfigReq->SetSel--;
                /*Copy the specified block */
                _fmemcpy ((char*)&pRes[Index],
                (const char*)pNvrPefConfig->AlertStringEntry[pGetConfigReq->SetSel].AlertString[pGetConfigReq->BlockSel],ALERT_STR_BLOCK_SIZE);
            }
            else
            {
                /*Copy the specified block */
                _fmemcpy((char*)&pRes[Index],
                (const char*)BMC_GET_SHARED_MEM (BMCInst)->AlertStringEntry.AlertString[pGetConfigReq->BlockSel],
                ALERT_STR_BLOCK_SIZE);
            }

            Index += ALERT_STR_BLOCK_SIZE;
            break;

        default:
            *pRes = CC_INV_DATA_FIELD;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
            return sizeof (*pRes);
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
    return (Index * sizeof (INT8U));
}

/*---------------------------------------
 * SetLastProcessedEventId
 *---------------------------------------*/
int 
SetLastProcessedEventId (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    SetLastEvtIDReq_T*  pSetLastEvtIDReq = (SetLastEvtIDReq_T*)pReq;
      PEFRecordDetailsConfig_T*        pNvrPEFRecordDetailsConfig;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    struct SELEventNode *SELNode= NULL;
    
    IPMI_DBG_PRINT ("SetLastProcessedEventId\n");
    
    /* Check for the reserved bytes should b zero */
    if  ( 0 !=  (pSetLastEvtIDReq->SetRecIDType & RESERVED_BITS_SETLASTPROCESSEDEVENTID ) )
    {
         pRes[0] = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }
    TDBG("%d\n",pSetLastEvtIDReq->RecordID);

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->PefConfig.PEFSharedMemMutex,WAIT_INFINITE);
    if(g_corefeatures.del_sel_reclaim_support == ENABLED)
    {  
        if(pBMCInfo->SELConfig.MaxSELRecord > pSetLastEvtIDReq->RecordID)
        {
           SELNode = SEL_RECORD_ADDR(BMCInst,pSetLastEvtIDReq->RecordID).RecAddr;
        }
        if(SELNode==NULL && pSetLastEvtIDReq->RecordID != 0xFFFF )
        {   
            /* the record ID does not exists so not allowing it to set*/
            TDBG("Record does not exists\n");
            goto SUCCESS;
            
        }
    }
    /*Get non volatile PEF configuration */
    pNvrPEFRecordDetailsConfig = &pBMCInfo->PEFRecordDetailsConfig;
    
    /*Check whose Record ID to set(SW or BMC) */
    if (pSetLastEvtIDReq->SetRecIDType & 0x01)
    {
        /*Set Last BMC processed Event ID */
        pNvrPEFRecordDetailsConfig->LastBMCProcessedEventID = pSetLastEvtIDReq->RecordID;
        /*Write to NVRAM*/
        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                  pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID),BMCInst);
    }
    else
    {
        /*Set Last SW processed Event ID */
        pNvrPEFRecordDetailsConfig->LastSWProcessedEventID  = pSetLastEvtIDReq->RecordID;
        /*Write to NVRAM*/
        FlushIPMI((INT8U*)&pBMCInfo->PEFRecordDetailsConfig,(INT8U*)&pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID,
                  pBMCInfo->IPMIConfLoc.PEFRecordDetailsConfigAddr,sizeof(pBMCInfo->PEFRecordDetailsConfig.LastBMCProcessedEventID),BMCInst);
    }
SUCCESS:
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
    *pRes = CC_NORMAL;  
    return sizeof (*pRes);
}



/*---------------------------------------
 * GetLastProcessedEventId
 *---------------------------------------*/
int 
GetLastProcessedEventId (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetLastEvtIDRes_T*  pGetLastEvtIDRes = (GetLastEvtIDRes_T*)pRes;
      PEFRecordDetailsConfig_T*        pNvrPEFRecordDetailsConfig;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    /*Get non volatile PEF configuration */
    pNvrPEFRecordDetailsConfig = &pBMCInfo->PEFRecordDetailsConfig;
    
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->PefConfig.PEFSharedMemMutex,WAIT_INFINITE);
    /* Load response information */
    pGetLastEvtIDRes->CompletionCode     = CC_NORMAL;
    pGetLastEvtIDRes->LastSELRecord      = pNvrPEFRecordDetailsConfig->LastSELRecordID;
    pGetLastEvtIDRes->LastSWProcessedID  = pNvrPEFRecordDetailsConfig->LastSWProcessedEventID;
    pGetLastEvtIDRes->LastBMCProcessedID = pNvrPEFRecordDetailsConfig->LastBMCProcessedEventID;
    pGetLastEvtIDRes->RecentTimestamp    = pNvrPEFRecordDetailsConfig->LastProcessedTimestamp;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);

    /*return length */
    return sizeof (GetLastEvtIDRes_T);
}

/*---------------------------------------
 * AlertImmediate
 *---------------------------------------*/
int 
AlertImmediate (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    MsgPkt_T		MsgPkt;
    INT8U EthIndex=0;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    AlertImmReq_T*  pAlertImmReq = (AlertImmReq_T*)pReq;
    AlertImmRes_T*  pAlertImmRes = (AlertImmRes_T*)pRes;
    SELEventRecord_T*  pAlertSELRec = (SELEventRecord_T*)MsgPkt.Data;
    ChannelInfo_T*      pChannelInfo;

    if(0x3 != ReqLen  && 0xB != ReqLen)  //0x3 for mandatory field and with  0xb  for optional field
    {
        pAlertImmRes->CompletionCode = CC_REQ_INV_LEN;
        return sizeof (*pRes);
    }

    /* Check for the reserved bytes should b zero */

    if ( ( 0 !=  (pAlertImmReq->ChannelNo & RESERVED_BITS_ALERTIMMEDIATE_CH ) ) ||
         ( 0 !=  (pAlertImmReq->DestSel & RESERVED_BITS_ALERTIMMEDIATE_DESTSEL ) ) )
    {
         pAlertImmRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof (*pRes);
    }

    /* check if any IPMI messaging session is already in progress */
    pChannelInfo = getChannelInfo (pAlertImmReq->ChannelNo & 0x0F,BMCInst);
    if(pChannelInfo->ActiveSession > 1) 	
    {
        pAlertImmRes->CompletionCode = CC_SESSION_IN_PROGRESS;
        return sizeof (*pRes);
    }
    
    EthIndex= GetEthIndex((pAlertImmReq->ChannelNo & 0x0F), BMCInst);
    if(0xff == EthIndex)
    {
        *pRes = CC_DEST_UNAVAILABLE;
        return sizeof (INT8U);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->PefConfig.PEFSharedMemMutex,WAIT_INFINITE);
    if(SEND_ALERT_IMM ==GetBits (pAlertImmReq->DestSel, 0xC0))
    {
        /*set the alert Immediate in Progress */
        if( ALERT_IMM_IN_PROGRESS == BMC_GET_SHARED_MEM(BMCInst)->LANAlertStatus[EthIndex] )
        {
            pAlertImmRes->CompletionCode = CC_SET_IN_PROGRESS;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
            return sizeof (*pRes);
        }
        BMC_GET_SHARED_MEM(BMCInst)->LANAlertStatus[EthIndex] = ALERT_IMM_IN_PROGRESS;
        MsgPkt.Channel = pAlertImmReq->ChannelNo & 0x0F;
        MsgPkt.Channel |=  ( pAlertImmReq->DestSel   & 0x0F )<< 4;
        MsgPkt.Cmd  =  pAlertImmReq->AlertStrSel;
        MsgPkt.Param   = PARAM_ALERT_IMM;
        strcpy ((char *)MsgPkt.SrcQ, PEF_RES_Q);
        if( ReqLen ==sizeof(AlertImmReq_T)) 
        {
            pAlertSELRec->hdr.TimeStamp = GetSelTimeStamp (BMCInst);
            pAlertSELRec->hdr.Type= 0x2;
            pAlertSELRec->hdr.ID =0xFFFF;
            _fmemcpy (&MsgPkt.Data[sizeof(SELRecHdr_T)+1], &(pAlertImmReq->GenID),(sizeof (SELEventRecord_T) -sizeof(SELRecHdr_T)));
        }else
        {
            m_AlertImmRecord.hdr.TimeStamp = GetSelTimeStamp (BMCInst);
            _fmemcpy (MsgPkt.Data, &m_AlertImmRecord, sizeof (SELEventRecord_T));
        }

        MsgPkt.Size = sizeof (SELEventRecord_T);

        /* Send to PEF queue */
        if (0 != PostMsgNonBlock (&MsgPkt, PEF_TASK_Q,BMCInst))
        {
            pAlertImmRes->CompletionCode = CC_COULD_NOT_PROVIDE_RESP;
            OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
            return sizeof (*pRes);
        }

        /* Wait for response */
        pAlertImmRes->CompletionCode = CC_NORMAL;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
        
		if (0 != GetMsg (&MsgPkt, PEF_RES_Q, WAIT_INFINITE, BMCInst))
		{
			IPMI_DBG_PRINT ("In AlertImmediate - What's wrong here?\n");
		}
        return sizeof (*pRes);
    }
    else if (GET_ALERT_IMM_STATUS ==GetBits (pAlertImmReq->DestSel, 0xC0))
    {
        pAlertImmRes->Status =	BMC_GET_SHARED_MEM(BMCInst)->LANAlertStatus[EthIndex];
        pAlertImmRes->CompletionCode = CC_NORMAL;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
        return sizeof (AlertImmRes_T);
    }
    else if( CLEAR_ALERT_IMM_STATUS ==GetBits (pAlertImmReq->DestSel, 0xC0))
    {
        BMC_GET_SHARED_MEM(BMCInst)->LANAlertStatus[EthIndex] = 0x0;
        pAlertImmRes->CompletionCode = CC_NORMAL;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
        return sizeof (*pRes);
    }else 
    {
        pAlertImmRes->CompletionCode = CC_INV_DATA_FIELD;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
        return sizeof (*pRes);
    }
}


/*---------------------------------------
 * PETAcknowledge
 *---------------------------------------*/
int PETAcknowledge (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    PETAckReq_T* pPETAckReq = (PETAckReq_T*)pReq;
            MsgPkt_T     MsgPkt;

    IPMI_DBG_PRINT ("PETAcknowledge\n");

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->PefConfig.PEFSharedMemMutex,WAIT_INFINITE);
    MsgPkt.Param = PARAM_PET_ACK;
    MsgPkt.Size  = sizeof (PETAckReq_T);
    strcpy ((char *)MsgPkt.SrcQ, PEF_RES_Q);

    _fmemcpy (MsgPkt.Data, pPETAckReq, sizeof (PETAckReq_T));

    if (0 != PostMsgNonBlock (&MsgPkt, PEF_TASK_Q,BMCInst))
    {
        *pRes = CC_COULD_NOT_PROVIDE_RESP;
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
        return sizeof (*pRes);
    }

    if (0 != GetMsg (&MsgPkt, PEF_RES_Q, WAIT_INFINITE, BMCInst))
    {
        IPMI_DBG_PRINT ("What's wrong here?\n");
    }
    *pRes = MsgPkt.Param;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->PefConfig.PEFSharedMemMutex);
    return sizeof (*pRes);
}

#endif  /* PEF_DEVICE */


