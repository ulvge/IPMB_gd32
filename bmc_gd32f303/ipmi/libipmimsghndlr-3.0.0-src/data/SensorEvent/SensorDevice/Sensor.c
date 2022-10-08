/*****************************************************************
 *****************************************************************

 *****************************************************************
 ******************************************************************
 *
 * sensor.c
 * Sensor functions.
 *
 *  Author: 
 *          
 ******************************************************************/
#define ENABLE_DEBUG_MACROS 0

#include "Types.h"
#include "debug_print.h"
#include "MsgHndlr.h"
#include "Sensor.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "IPMI_Sensor.h"
#include "SensorMonitor.h"
#include "SDRFunc.h"
#include "SDR.h"
#include "Util.h"
#include "IPMI_Main.h"
//#include "SharedMem.h"
#include "AppDevice.h"
//#include "IPMI_KCS.h"
#include "IPMI_SEL.h"
#include "Message.h"
#include "OSPort.h"
#include "PEF.h"
#include "SEL.h"
#include "FRU.h"
//#include "PDKAccess.h"
//#include "PDKCmdsAccess.h"
#include "IPMIConf.h"
#include "IPMI_FRU.h"
#include "sensor_helpers.h"
//#include "featuredef.h"
#include "libipmi.h"

#include "fan/api_fan.h"
#include "adc/api_adc.h"
#include "tmp/api_tmp.h"
#include "adc/api_adc.h"

/* Reserved bit macro definitions */
#define RESERVED_BITS_SETSENSORTYPE 0X80				//(BIT7)
#define RESERVED_BITS_REARMSENSOR_REARMALLEVENTS 0X7F	//(BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0)
#define RESERVED_BITS_REARMSENSOR_REARMASSEVT2_1 0XF0	//(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_REARMSENSOR_REARMDEASSEVT2_1 0XF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_REARMSENSOR_REARMASSEVT2_2 0X80	//(BIT7)
#define RESERVED_BITS_REARMSENSOR_REARMDEASSEVT2_2 0X80 //(BIT7)
#define RESERVED_BITS_SETSENSORTHRESHOLDS 0XC0			//(BIT7 | BIT6)
#define RESERVED_BITS_SETSENEVTEN_FLAGS 0X0F			//(BIT3 | BIT2 | BIT1 | BIT0)
#define RESERVED_BITS_SETSENEVTEN_ASSERTIONMASK 0X8000
#define RESERVED_BITS_SETSENEVTEN_DEASSERTIONMASK 0X8000
#define RESERVED_BITS_SETSENRD_ASSEVTOCCBYTE_1 0xF0	  //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_SETSENRD_DEASSEVTOCCBYTE_1 0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)
#define RESERVED_BITS_SETSENRD_ASSEVTOCCBYTE_2 0x80	  //(BIT7)
#define RESERVED_BITS_SETSENRD_DEASSEVTOCCBYTE_2 0x80 //(BIT7)
#define SENSOR_THRESOLD_ACCESS_BITS 0x0c			  //(BIT2 | BIT3)
#define SENSOR_HYSTERESIS_ACCESS_BITS 0x30			  //(BIT4 | BIT5)
#if SENSOR_DEVICE == 1

/*** Local Definitions ***/
#define NUM_SENSORS(BMCInst) (g_BMCInfo.SenConfig.NumThreshSensors + g_BMCInfo.SenConfig.NumNonThreshSensors)

#define FULL_SDR_REC 0x01
#define COMPACT_SDR_REC 0x02
#define OEM_SDR_FRU_REC 0xf1
#define OEM_SDR_NM_REC 0x0D

#define FL_STATIC_SENSOR 0x00
#define FL_NUM_LUN(NUM_LUN) NUM_LUN

#define LWR_NON_CRIT 0x01
#define LWR_CRIT 0x02
#define LWR_NON_REC 0x04
#define UPR_NON_CRIT 0x08
#define UPR_CRIT 0x10
#define UPR_NON_REC 0x20

#define EVENT_MSG_MASK 0x80
#define SCAN_MASK 0x40
#define ENABLE_DISABLE_EVENT_MASK 0x30
#define DO_NOT_CHANGE 0x00
#define ENABLE_SELECTED_EVENT_MSG 0x10
#define DISABLE_SELECTED_EVENT_MSG 0x20

#define CC_INVALID_ATTEMPT_TO_SET 0x80
#define MIN_SET_SEN_READING_CMD_LEN 3
#define LEN_FOR_EVT_DATA 10
#define MAX_SET_SEN_READ_LEN 10
#define LEN_FOR_ASSERT_DATA 5
#define LEN_FOR_DEASSERT_DATA 7
#define LEN_FOR_SETSENSOR_DATA 3

#define NOT_FOUND_SCAN_DISABLED -1
#define NOT_FOUND_SCAN_ENABLED -2
#define ENTITY_FOUND 1
#define ENTITY_NOT_FOUND 0

#define DCMI_TEMP_READING 0x0
#define DCMI_INST_NUMBER 0x1

/*** Local typedefs ***/

/**
 * @struct SR_SensorInfo_T
 * @brief Sensor Information.
 **/
typedef struct
{
	/* CAUTION Order of members dependent on Response structures */
	INT8U M_LSB;
	INT8U M_MSB_Tolerance;
	INT8U B_LSB;
	INT8U B_MSB_Accuracy;
	INT8U Accuracy_MSB_Exp;
	INT8U RExp_BExp;

	INT8U PositiveHysterisis;
	INT8U NegativeHysterisis;

	INT8U LowerNonCritical;
	INT8U LowerCritical;
	INT8U LowerNonRecoverable;
	INT8U UpperNonCritical;
	INT8U UpperCritical;
	INT8U UpperNonRecoverable;

	INT8U EventFlags;
	INT16U AssertionMask;
	INT16U DeAssertionMask;

	INT8U EventTypeCode;

	INT16U ReadWriteThreshMask;
	INT8U SensorInit;
	INT8U SensorNum;

} SR_SensorInfo_T;

/*** Global Variables ***/
//INT8U g_NumThreshSensors;
//INT8U g_NumNonThreshSensors;
// INT8U g_FRUInfo[MAX_PDK_FRU_SUPPORTED];
//FRUInfo_T   *m_FRUInfo[MAX_PDK_FRU_SUPPORTED];
//INT8U m_total_frus=0;

//Sensor-Type  Sensor-specific-offset
static const INT8U sensor_presence[][2] = {
	{PROCESSOR, PROCESSOR_PRESENCE_DETECTED},
	{POWER_SUPPLY, POWER_SUPPLY_PRESENCE_DETECTED},
	{POWER_SUPPLY, POWER_SUPPLY_OUT_OF_RANGE_PRESENT},
	{MEMORY, MEMORY_PRESENCE_DETECTED},
	{ENTITY_PRESENCE, ENTITY_PRESENCE_ENTITY_PRESENT},
	{BATTERY, BATTERY_PRESENCE_DETECTED}};
#define SENSOR_PRESENCE_COUNT (sizeof(sensor_presence) / sizeof(sensor_presence[0]))

/*** Prototype Declaration ***/
static void FindNumSensors(int BMCInst);
static INT8U IpmiReadingDatConvert2Raw(INT8U sensor_type, INT16U value);

/*---------------------------------------
 * InitSensor
 *---------------------------------------*/
int InitSensor(int BMCInst)
{
	SDRRecHdr_T *sr;
	FullSensorRec_T *sfs;
	CompactSensorRec_T *scs;
	FRUDevLocatorRec_T *sfr;
	OEM_FRURec_T *sof;
	INT16U SharedRecs = 0;
	INT8U NodeManager = 0;
	INT16U LUN_SensorNum = 0; /* Multi-LUN support index */
	int i = 0, len = 0, fruvalid = 0;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	INT8U Index = 0, FruAddress = 0;
	SensorSharedMem_T *pSenSharedMem;
	char *str, *saveptr, *tempbuf;  
	UNUSED(fruvalid);

	memset(&pBMCInfo->SenConfig.HealthState, 0, sizeof(HealthState_T));
	;
	/* If we didnt  initalize the g_FRUInfo to 0xff and  if SDR type #11 is not present then
	 It makes infinity FRU test loop */
	// memset(pBMCInfo->FRUConfig.FRUInfo,0xff,MAX_PDK_FRU_SUPPORTED);

	/* Get the Sensor Shared Memory */
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	/* Create mutex for Sensor shared memory */
	// OS_THREAD_MUTEX_INIT(g_BMCInfo[BMCInst].SensorSharedMemMutex, PTHREAD_MUTEX_RECURSIVE);

	/*  Find No.of Threshold and Non Threshold Sensors */
	FindNumSensors(BMCInst);

	IPMI_DBG_PRINT("Init Sensor\n");

	if (0 == (pBMCInfo->SenConfig.NumThreshSensors + pBMCInfo->SenConfig.NumNonThreshSensors))
	{
		IPMI_WARNING("Sensor.c : There are no sensors\n");
		return -1;
	}

	/* Acquire Shared memory to populate sensor information  */
	OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo[BMCInst].SensorSharedMemMutex, WAIT_INFINITE)

	sr = SDR_GetFirstSDRRec(BMCInst);

	while (0 != sr)
	{

		// Populater sensor information for Threshold sensors
		if (sr->Type == FULL_SDR_REC)
		{
			sfs = (FullSensorRec_T *)sr;

			LUN_SensorNum = (((sfs->OwnerLUN & VALID_LUN) << 8) | sfs->SensorNum); /* Multi-LUN support */
			IPMI_DBG_PRINT("LUN_SensorNum = %x sfs->OwnerLUN = %x sfs->SensorNum = %x \n", LUN_SensorNum, sfs->OwnerLUN, sfs->SensorNum);

			if (sfs->OwnerID == pBMCInfo->IpmiConfig.BMCSlaveAddr)
			{
				pSenSharedMem->SensorInfo[LUN_SensorNum].IsSensorPresent = TRUE; /* Multi-LUN support */
				IPMI_DBG_PRINT_1("#### pSensorSharedMem->SensorInfo[%x].IsSensorPresent = TRUE ####\n", LUN_SensorNum);
				SR_LoadSDRDefaults(sr, (SensorInfo_T *)&pSenSharedMem->SensorInfo[LUN_SensorNum], BMCInst); /* Multi-LUN support */
				// SDR Record information
				pSenSharedMem->SensorInfo[LUN_SensorNum].SDRRec = sr; /* Multi-LUN support */

				/* Update Init Done flag */
				SET_SM_INIT_DONE(pSenSharedMem->SensorInfo[LUN_SensorNum].EventFlags); /* Multi-LUN support */

				if (pBMCInfo->IpmiConfig.DCMISupport == 1)
				{
					pSenSharedMem->SensorInfo[LUN_SensorNum].IsDCMITempsensor = TRUE;
					pSenSharedMem->SensorInfo[LUN_SensorNum].EntityID = sfs->EntityID;
					pSenSharedMem->SensorInfo[LUN_SensorNum].EntiryInstance = sfs->EntityIns;
					pSenSharedMem->SensorInfo[LUN_SensorNum].RecordID = sfs->hdr.ID;
				}
			}
			else if (NodeManager)
			{
				/* Seperate info of ME sensors into another array. */
				pSenSharedMem->ME_SensorInfo[LUN_SensorNum].IsSensorPresent = TRUE;
				IPMI_DBG_PRINT_1("#### pSensorSharedMem->ME_SensorInfo[%x].IsSensorPresent = TRUE ####\n", LUN_SensorNum);
				SR_LoadSDRDefaults(sr, (SensorInfo_T *)&pSenSharedMem->ME_SensorInfo[LUN_SensorNum], BMCInst);
				// SDR Record information
				pSenSharedMem->ME_SensorInfo[LUN_SensorNum].SDRRec = sr;

				/* Update Init Done flag */
				SET_SM_INIT_DONE(pSenSharedMem->ME_SensorInfo[LUN_SensorNum].EventFlags);
			}

			if (SENSOR_TYPE_SYSTEM_EVENT == sfs->SensorType)
			{
				//	BMC_GET_SHARED_MEM(BMCInst)->SysEvent_SensorNo =sfs->SensorNum;
			}
		}
		else if (sr->Type == COMPACT_SDR_REC)
		{
			scs = (CompactSensorRec_T *)sr;

			LUN_SensorNum = ((scs->OwnerLUN & VALID_LUN) << 8 | scs->SensorNum); /* Multi-LUN support */
			IPMI_DBG_PRINT("LUN_SensorNum = %x scs->OwnerLUN = %x scs->SensorNum = %x \n", LUN_SensorNum, scs->OwnerLUN, scs->SensorNum);

			if (scs->OwnerID == pBMCInfo->IpmiConfig.BMCSlaveAddr)
			{
				pSenSharedMem->SensorInfo[LUN_SensorNum].IsSensorPresent = TRUE; /* Multi-LUN support */
				IPMI_DBG_PRINT_1("#### pSensorSharedMem->SensorInfo[%x].IsSensorPresent = TRUE ####\n", LUN_SensorNum);
				SR_LoadSDRDefaults(sr, (SensorInfo_T *)&pSenSharedMem->SensorInfo[LUN_SensorNum], BMCInst); /* Multi-LUN support */

				// SDR Record information
				pSenSharedMem->SensorInfo[LUN_SensorNum].SDRRec = sr; /* Multi-LUN support */

				SharedRecs = ipmitoh_u16(((CompactSensorRec_T *)sr)->RecordSharing) &
							 SHARED_RECD_COUNT;
				IPMI_DBG_PRINT_2("Sen %x , Shared Count - %x\n", scs->SensorNum, SharedRecs);

				/* Update Init Done flag */
				SET_SM_INIT_DONE(pSenSharedMem->SensorInfo[LUN_SensorNum].EventFlags); /* Multi-LUN support */

				/* Check if Record is shared */
				if (SharedRecs > 1)
				{
					for (i = 1; i < SharedRecs; i++)
					{
						pSenSharedMem->SensorInfo[LUN_SensorNum + i].IsSensorPresent = TRUE;							/* Multi-LUN support */
						SR_LoadSDRDefaults(sr, (SensorInfo_T *)&pSenSharedMem->SensorInfo[LUN_SensorNum + i], BMCInst); /* Multi-LUN support */
						pSenSharedMem->SensorInfo[LUN_SensorNum + i].SensorNumber = scs->SensorNum + i;					/* Multi-LUN support */

						// SDR Record information
						pSenSharedMem->SensorInfo[LUN_SensorNum + i].SDRRec = sr; /* Multi-LUN support */

						/* Update Init Done flag */
						SET_SM_INIT_DONE(pSenSharedMem->SensorInfo[LUN_SensorNum + i].EventFlags); /* Multi-LUN support */
					}
				}

				if (pBMCInfo->IpmiConfig.DCMISupport == 1)
				{
					pSenSharedMem->SensorInfo[LUN_SensorNum].IsDCMITempsensor = TRUE;
					pSenSharedMem->SensorInfo[LUN_SensorNum].EntityID = scs->EntityID;
					pSenSharedMem->SensorInfo[LUN_SensorNum].EntiryInstance = scs->EntityIns;
					pSenSharedMem->SensorInfo[LUN_SensorNum].RecordID = scs->hdr.ID;
				}
			}
			else if (NodeManager)
			{
				pSenSharedMem->ME_SensorInfo[LUN_SensorNum].IsSensorPresent = TRUE;
				IPMI_DBG_PRINT_1("#### pSensorSharedMem->ME_SensorInfo[%x].IsSensorPresent = TRUE ####\n", LUN_SensorNum);
				SR_LoadSDRDefaults(sr, (SensorInfo_T *)&pSenSharedMem->ME_SensorInfo[LUN_SensorNum], BMCInst);

				// SDR Record information
				pSenSharedMem->ME_SensorInfo[LUN_SensorNum].SDRRec = sr;

				SharedRecs = ipmitoh_u16(((CompactSensorRec_T *)sr)->RecordSharing) &
							 SHARED_RECD_COUNT;
				IPMI_DBG_PRINT_2("Sen %x , Shared Count - %x\n", scs->SensorNum, SharedRecs);

				/* Update Init Done flag */
				SET_SM_INIT_DONE(pSenSharedMem->ME_SensorInfo[LUN_SensorNum].EventFlags);

				/* Check if Record is shared */
				if (SharedRecs > 1)
				{
					for (i = 1; i < SharedRecs; i++)
					{
						pSenSharedMem->ME_SensorInfo[LUN_SensorNum + i].IsSensorPresent = TRUE;
						SR_LoadSDRDefaults(sr, (SensorInfo_T *)&pSenSharedMem->ME_SensorInfo[LUN_SensorNum + i], BMCInst);
						pSenSharedMem->ME_SensorInfo[LUN_SensorNum + i].SensorNumber = scs->SensorNum + i;

						// SDR Record information
						pSenSharedMem->ME_SensorInfo[LUN_SensorNum + i].SDRRec = sr;

						/* Update Init Done flag */
						SET_SM_INIT_DONE(pSenSharedMem->ME_SensorInfo[LUN_SensorNum + i].EventFlags);
					}
				}
			}

			if (SENSOR_TYPE_SYSTEM_EVENT == scs->SensorType)
			{
				//	BMC_GET_SHARED_MEM(BMCInst)->SysEvent_SensorNo =scs->SensorNum;
			}
		}
		else if (sr->Type == FRU_DEVICE_LOCATOR_SDR_REC)
		{
			sfr = (FRUDevLocatorRec_T *)sr;
			/* Collecting the Logical FRU Device ID */
			if ((sfr->AccessLUNBusID & 0x80) == 0x80)
			{
				//	if(g_PDKHandle[PDK_FRUGETDEVADDRESS] != NULL)
				if (0)
				{
					//	if(((int(*)(INT8U,INT8U*,int))g_PDKHandle[PDK_FRUGETDEVADDRESS]) (sfr->FRUIDSlaveAddr,&FruAddress,BMCInst) == -1)
					{
						IPMI_WARNING("FRU Device Address cannot be found for FRU ID %x \n", sfr->FRUIDSlaveAddr);
						sr = SDR_GetNextSDRRec(sr, BMCInst);
						continue;
					}
				}
				else
				{
					sr = SDR_GetNextSDRRec(sr, BMCInst);
					continue;
				}
				// pBMCInfo->FRUConfig.FRUInfo[Index++]=sfr->FRUIDSlaveAddr;
				// pBMCInfo->FRUConfig.FRUInfo[Index]= 0xff;
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]=(FRUInfo_T*)malloc(sizeof(FRUInfo_T));
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->IsInternalFRU=0;
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->Size=FRU_FILE_SIZE;
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->AccessType=DEV_ACCESS_MODE_IN_BYTES;
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->Offset=0;
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->DeviceID=sfr->FRUIDSlaveAddr;
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->Type=FRU_TYPE_EEPROM;
				// //pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->NVRFile=NULL;
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->SlaveAddr=FruAddress;
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->DeviceType=sfr->DevType;
				// pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->BusNumber = (sfr->AccessLUNBusID & 0x07);
				// strncpy( (char *)pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->FRUName, sfr->DevIdStr, MAX_ID_STR_LEN);

				//PDK Hook to overwrite FRUInfo struct
				//if (g_PDKHandle[PDK_INITFRUINFO] != NULL)
				#if 0
				{
				//	if (((int (*)(FRUDevLocatorRec_T *, int))g_PDKHandle[PDK_INITFRUINFO])(sfr, BMCInst) == -1)
					{
						IPMI_WARNING("Failed to override FRU Info properties \n");
					}
				}
				pBMCInfo->FRUConfig.total_frus++;
				#endif
			}
		}
		else if (sr->Type == OEM_SDRFRU_REC)
		{
			sof = (OEM_FRURec_T *)sr;

			if (sof->OEM_Fru == OEM_SDR_FRU_REC)
			{
				for (i = 0; i < MAX_FRU_SDR_STR_SIZE; i++)
				{
					if (sof->FilePath[i] == ':')
					{
						fruvalid = TRUE;
					}
				}

				// if (fruvalid == TRUE)
				// {
				// 	pBMCInfo->FRUConfig.FRUInfo[Index++] = sof->DeviceID;
				// 	pBMCInfo->FRUConfig.FRUInfo[Index] = 0xff;
				// 	pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus] = (FRUInfo_T *)malloc(sizeof(FRUInfo_T));
				// 	pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->IsInternalFRU = 0;
				// 	pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->Size = sof->Size;
				// 	pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->AccessType = sof->AccessType;
				// 	pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->Offset = 0;
				// 	pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->DeviceID = sof->DeviceID;
				// 	pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->Type = FRU_TYPE_NVR;
				// 	str = sof->FilePath;
				// 	tempbuf = strtok_r(str, ":", &saveptr);
				// 	strcpy(pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->NVRFile, tempbuf);
				// 	len = strlen(tempbuf);
				// 	strcpy(pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->FRUName, &sof->FilePath[++len]);
				// 	pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->SlaveAddr = 0;
				// 	pBMCInfo->FRUConfig.m_FRUInfo[pBMCInfo->FRUConfig.total_frus]->DeviceType = 0;
				// 	pBMCInfo->FRUConfig.total_frus++;
				// }
				// fruvalid = 0;
			}
		}

		/* Get the next record */
		sr = SDR_GetNextSDRRec(sr, BMCInst);
	}

	/* Release mutex for Sensor shared memory */
	//OS_RELEASE_MUTEX(m_hSensorSharedMemMutex)
	// OS_THREAD_MUTEX_RELEASE(&g_BMCInfo[BMCInst].SensorSharedMemMutex);

	IPMI_DBG_PRINT("Initilized Sensor\n");

	return 0;
}

/*-----------------------------------------
 * SR_FindSDR
 *-----------------------------------------*/
SDRRecHdr_T *
SR_FindSDR(INT8U SensorNum, INT8U SensorOwnerLUN, INT8U SensorOwnerID, int BMCInst) /*  Multi-LUN support to find the SDR based on OwnerLUN, OwnerID, and Sensor Number */
{

	return NULL;
}

/*-----------------------------------------
 * SR_LoadSDRDefaults
 * CAUTION !! this function has to be called
 * after acquiring a sensor shared memory mutex
 *-----------------------------------------*/
extern void
SR_LoadSDRDefaults(SDRRecHdr_T *sr, SensorInfo_T *pSensorInfo, int BMCInst)
{
}

/*-----------------------------------------
 * GetDevSDRInfo
 *-----------------------------------------*/
void SR_GetDevSDRInfo(GetSDRInfoRes_T *GetSDRInfoRes, int BMCInst)
{
	GetSDRInfoRes->NumSensor = NUM_SENSORS(BMCInst);
	GetSDRInfoRes->Flags = FL_STATIC_SENSOR + FL_NUM_LUN(0);
	GetSDRInfoRes->TimeStamp = 0;
	return;
}

/*---------------------------------------
 * GetDevSDRInfo
 *---------------------------------------*/
int GetDevSDRInfo(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	SDRRecHdr_T *SDRRec;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	GetSDRInfoReq_T *pGetDevSDRInfoReq = (GetSDRInfoReq_T *)pReq;
	GetSDRInfoRes_T *pGetDevSDRInfoRes = (GetSDRInfoRes_T *)pRes;
	CompactSensorRec_T *pCompactSDRRec = NULL;
	FullSensorRec_T *pFullSDRRec = NULL;
	INT8U NumSDRs = 0;
	INT8U NumSensors = 0;
	INT16U OwnerLUN = 0;
	INT8U SensorLun = 0;
	INT8U NumSharedSensors = 0;

	if (ReqLen > 1)
	{
		pGetDevSDRInfoRes->CompletionCode = CC_REQ_INV_LEN;
		return sizeof(INT8U);
	}

	pGetDevSDRInfoRes->CompletionCode = CC_NORMAL;
	if (ReqLen == 0)
	{ // no reqest paremeter
		pGetDevSDRInfoRes->NumSensor = SENSOR_NUM;
	}
	else if ((pGetDevSDRInfoReq->Operation & 0x01) == 0)
	{ // get sdr count
		pGetDevSDRInfoRes->NumSensor = SENSOR_NUM;
	}
	else
	{ // get sensor count
		pGetDevSDRInfoRes->NumSensor = SENSOR_NUM;
	}
	return sizeof(GetSDRInfoRes_T); 
	#if 0

	/* Check for proper operation flag before proceeding with task */
	if ((pGetDevSDRInfoReq->Operation != SDR_INFO_SDR_COUNT) &&
		(pGetDevSDRInfoReq->Operation != SDR_INFO_SENSOR_COUNT) && (ReqLen == 1))
	{
		pGetDevSDRInfoRes->CompletionCode = CC_INV_DATA_FIELD;
		return sizeof(INT8U);
	}

	// If multiple LUNs are supported, then we parse the OwnerLUN from the TLS
	// We need to fetch the sensor count for the requester's LUN
	//	if(g_corefeatures.more_than_256_sensors == ENABLED)
	//	{
	//		OS_THREAD_TLS_GET(g_tls.NetFnLUN,OwnerLUN);

	//		if (OwnerLUN != 0xFF)
	//		SensorLun = (OwnerLUN & VALID_LUN);
	//		else
	//		SensorLun = 0;
	//	}
	//	else
	//	{
	//		SensorLun = 0;
	//	}

	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);

	/* Count number of full and compact sensors */
	SDRRec = SDR_GetFirstSDRRec(BMCInst);
	while (0 != SDRRec)
	{
		/* Found a SDR. Increment the number of SDRs counter */
		NumSDRs++;

		/* Parse by the type of SDR found */
		switch (SDRRec->Type)
		{
		/* For Full SDR Record, if the SDR LUN matches with the requester's LUN */
		/* Then, increment the number of Sensors counter */
		case FULL_SDR_REC:
		{
			pFullSDRRec = (FullSensorRec_T *)(SDRRec);

			if (pFullSDRRec->OwnerLUN == SensorLun)
				NumSensors++;

			break;
		}

		/* For Compact SDR Record, if the SDR LUN matches with the requester's LUN */
		/* Then, increment the number of sensors counter */
		case COMPACT_SDR_REC:
		{
			pCompactSDRRec = (CompactSensorRec_T *)(SDRRec);

			if (pCompactSDRRec->OwnerLUN == SensorLun)
			{
				NumSensors++;

				NumSharedSensors = ipmitoh_u16(pCompactSDRRec->RecordSharing) & SHARED_RECD_COUNT;
				if (NumSharedSensors > 1)
				{
					NumSensors += (NumSharedSensors - 1);
				}
			}

			break;
		}
		}

		/* Get the next record */
		SDRRec = SDR_GetNextSDRRec(SDRRec, BMCInst);
	}

	/* Fill in response data */
	pGetDevSDRInfoRes->CompletionCode = CC_NORMAL;

	if (pGetDevSDRInfoReq->Operation == SDR_INFO_SDR_COUNT)
		pGetDevSDRInfoRes->NumSensor = NumSDRs;
	else
		pGetDevSDRInfoRes->NumSensor = NumSensors;

	pGetDevSDRInfoRes->Flags = (0x01 << SensorLun); /* BIT7 = 0 (static sensors), BIT0 = 1 (on LUN 0) */
	pGetDevSDRInfoRes->TimeStamp = 0;

	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

	return sizeof(GetSDRInfoRes_T);  
#endif
}

/*---------------------------------------
 * GetDevSDR
 *---------------------------------------*/
int GetDevSDR(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	return GetSDR(pReq, ReqLen, pRes, BMCInst);
}

/*---------------------------------------
 * GetDevSDR
 *---------------------------------------*/
int ReserveDevSDRRepository(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	return ReserveSDRRepository(pReq, ReqLen, pRes, BMCInst);
}

/*---------------------------------------
 * SetSensorType
 *---------------------------------------*/
int SetSensorType(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	//pRes [0] = CC_INV_CMD;
	//return sizeof (*pRes);
	SetSensorTypeReq_T *pSetSensorTypeReq = (SetSensorTypeReq_T *)pReq;
	SetSensorTypeRes_T *pSetSensorTypeRes = (SetSensorTypeRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;

	/* Check for the reserved bytes should b zero */

	if (0 != (pSetSensorTypeReq->EventTypeCode & RESERVED_BITS_SETSENSORTYPE))
	{
		pSetSensorTypeRes->CompletionCode = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

	OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo.SensorSharedMemMutex, WAIT_INFINITE);
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	if (!pSenSharedMem->SensorInfo[pSetSensorTypeReq->SensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&g_BMCInfo.SensorSharedMemMutex);

		/* return sdr record not present completion code */
		pSetSensorTypeRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	pSenSharedMem->SensorInfo[pSetSensorTypeReq->SensorNum].SensorTypeCode = pSetSensorTypeReq->SensorType;
	pSenSharedMem->SensorInfo[pSetSensorTypeReq->SensorNum].EventTypeCode = pSetSensorTypeReq->EventTypeCode;

	OS_THREAD_MUTEX_RELEASE(&g_BMCInfo.SensorSharedMemMutex);

	pSetSensorTypeRes->CompletionCode = CC_NORMAL;

	return sizeof(*pSetSensorTypeRes);
}

/*---------------------------------------
 * GetSensorType
 *---------------------------------------*/
int GetSensorType(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	//pRes [0] = CC_INV_CMD;
	// return sizeof (*pRes);

	GetSensorTypeReq_T *pGetSensorTypeReq = (GetSensorTypeReq_T *)pReq;
	GetSensorTypeRes_T *pGetSensorTypeRes = (GetSensorTypeRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSenSharedMem->SensorInfo[pGetSensorTypeReq->SensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		/* return sdr record not present completion code */
		pGetSensorTypeRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	pGetSensorTypeRes->CompletionCode = CC_NORMAL;
	pGetSensorTypeRes->SensorType = pSenSharedMem->SensorInfo[pGetSensorTypeReq->SensorNum].SensorTypeCode;
	pGetSensorTypeRes->EventTypeCode = pSenSharedMem->SensorInfo[pGetSensorTypeReq->SensorNum].EventTypeCode;

	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	return sizeof(*pGetSensorTypeRes);
}

/*---------------------------------------
 * ReArmSensor
 *---------------------------------------*/
int ReArmSensor(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	MsgPkt_T Msg;
	HQueue_T hSMHndlr_Q;
	ReArmSensorReq_T ReArmSensorReq;
	ReArmSensorReq_T *pReArmSensorReq = &ReArmSensorReq;
	ReArmSensorRes_T *pReArmSensorRes = (ReArmSensorRes_T *)pRes;

	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;

	_fmemset(&ReArmSensorReq, 0, sizeof(ReArmSensorReq_T));
	_fmemcpy(&ReArmSensorReq, pReq, ReqLen);

	/* if request length is invalid */
	if ((ReqLen < 2) || (ReqLen > sizeof(ReArmSensorReq_T)))
	{
		/* return request invalid length completion code */
		pReArmSensorRes->CompletionCode = CC_REQ_INV_LEN;
		return sizeof(*pRes);
	}

	/* Check for the reserved bytes should b zero */

	if (0 != (pReArmSensorReq->ReArmAllEvents & RESERVED_BITS_REARMSENSOR_REARMALLEVENTS))
	{
		pReArmSensorRes->CompletionCode = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	/* Check for the reserved bits */
	if (pSenSharedMem->SensorInfo[pReArmSensorReq->SensorNum].SensorReadType == THRESHOLD_SENSOR_CLASS)
	{
		if ((pReArmSensorReq->ReArmAssertionEvents2 & RESERVED_BITS_REARMSENSOR_REARMASSEVT2_1) ||
			(pReArmSensorReq->ReArmDeassertionEvents2 & RESERVED_BITS_REARMSENSOR_REARMDEASSEVT2_1))
		{
			pReArmSensorRes->CompletionCode = CC_INV_DATA_FIELD;
			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
			return sizeof(*pRes);
		}
	}
	else
	{
		if ((pReArmSensorReq->ReArmAssertionEvents2 & RESERVED_BITS_REARMSENSOR_REARMASSEVT2_2) ||
			(pReArmSensorReq->ReArmDeassertionEvents2 & RESERVED_BITS_REARMSENSOR_REARMDEASSEVT2_2))
		{
			pReArmSensorRes->CompletionCode = CC_INV_DATA_FIELD;
			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
			return sizeof(*pRes);
		}
	}

	if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	{
		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
		{
			pReArmSensorRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
			return sizeof(*pRes);
		}
	}

	if (!pSenSharedMem->SensorInfo[pReArmSensorReq->SensorNum].IsSensorPresent)
	{
		/* return sdr record not present completion code */
		pReArmSensorRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
		return sizeof(*pRes);
	}

	Msg.Param = PARAM_REARM_SENSOR;
	Msg.Size = sizeof(ReArmSensorReq_T);
	_fmemcpy(Msg.Data, &ReArmSensorReq, sizeof(ReArmSensorReq_T));

	/* Post to sensormonitor task Thread to rearm this sensor */
	//	GetQueueHandle(SM_HNDLR_Q,&hSMHndlr_Q,BMCInst);
	//	if ( -1 != hSMHndlr_Q )
	//	{
	//		PostMsg(&Msg, SM_HNDLR_Q,BMCInst);

	//		/* return normal completion code */
	//		pReArmSensorRes->CompletionCode = CC_NORMAL;
	//	} else
	//	{
	/* return normal completion code */
	pReArmSensorRes->CompletionCode = 0xFF;
	//	}
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	return (sizeof(ReArmSensorRes_T));
}

/*---------------------------------------
 * GetSensorEventStatus
 *---------------------------------------*/
int GetSensorEventStatus(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	GetSensorEventStatusReq_T *pSensorEventStatusReq = (GetSensorEventStatusReq_T *)pReq;
	GetSensorEventStatusRes_T *pSensorEventStatusRes = (GetSensorEventStatusRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	{
		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
		{
			pSensorEventStatusRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
			return sizeof(*pRes);
		}
	}

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		/* return sdr record not present completion code */
		pSensorEventStatusRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	pSensorEventStatusRes->CompletionCode = CC_NORMAL;
	pSensorEventStatusRes->Flags = (pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].EventFlags & 0xe0);

	/* Set optional response bytes to zero if event messages are disabled for this sensor */
	if (0 == ((pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].EventFlags) & BIT7))
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorEventStatusRes->AssertionEvents1 = 0;
		pSensorEventStatusRes->AssertionEvents2 = 0;
		pSensorEventStatusRes->DeassertionEvents1 = 0;
		pSensorEventStatusRes->DeassertionEvents2 = 0;
		return sizeof(GetSensorEventStatusRes_T);
	}

	if (0 == ((pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].SensorCaps) & BIT6))
	{
		/* Get sensor event status history */
		pSensorEventStatusRes->AssertionEvents1 =
			(pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].AssertionHistoryByte1 &
			 pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].AssertionEventEnablesByte1);
		pSensorEventStatusRes->AssertionEvents2 =
			(pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].AssertionHistoryByte2 &
			 pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].AssertionEventEnablesByte2);
		pSensorEventStatusRes->DeassertionEvents1 =
			(pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].DeassertionHistoryByte1 &
			 pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].DeassertionEventEnablesByte1);
		pSensorEventStatusRes->DeassertionEvents2 =
			(pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].DeassertionHistoryByte2 &
			 pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].DeassertionEventEnablesByte2);
	}
	else
	{
		/* Get present sensor event status */
		pSensorEventStatusRes->AssertionEvents1 =
			(pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].AssertionEventOccuredByte1 &
			 pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].AssertionEventEnablesByte1);
		pSensorEventStatusRes->AssertionEvents2 =
			(pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].AssertionEventOccuredByte2 &
			 pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].AssertionEventEnablesByte2);
		pSensorEventStatusRes->DeassertionEvents1 =
			(pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].DeassertionEventOccuredByte1 &
			 pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].DeassertionEventEnablesByte1);
		pSensorEventStatusRes->DeassertionEvents2 =
			(pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].DeassertionEventOccuredByte2 &
			 pSenSharedMem->SensorInfo[pSensorEventStatusReq->SensorNum].DeassertionEventEnablesByte2);
	}
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	return sizeof(GetSensorEventStatusRes_T);
}

/*---------------------------------------
 * SetSensorHysterisis
 *---------------------------------------*/
int SetSensorHysterisis(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	SetSensorHysterisisReq_T *pSensorHysReq =
		(SetSensorHysterisisReq_T *)pReq;
	SetSensorHysterisisRes_T *pSensorHysRes =
		(SetSensorHysterisisRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	{
		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
		{
			pSensorHysRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
			return sizeof(*pRes);
		}
	}

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorHysRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	if (pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].SensorReadType != THRESHOLD_SENSOR_CLASS)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
		pSensorHysRes->CompletionCode = CC_ILLEGAL_CMD_FOR_SENSOR_REC;
		return sizeof(*pRes);
	}

	if (BIT5 != (pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].SensorCaps & (BIT5 | BIT4)))
	{
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
		//set operation is not allowed
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

	/* Set the hysterisis values */
	pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].PosHysteresis = pSensorHysReq->PositiveHysterisis;
	pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].NegHysteresis = pSensorHysReq->NegativeHysterisis;

	/* Release mutex for Sensor shared memory */
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	pSensorHysRes->CompletionCode = CC_NORMAL;
	return sizeof(SetSensorHysterisisRes_T);
}

/*---------------------------------------
 * GetSensorHysterisis
 *---------------------------------------*/
int GetSensorHysterisis(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	GetSensorHysterisisReq_T *pSensorHysReq =
		(GetSensorHysterisisReq_T *)pReq;
	GetSensorHysterisisRes_T *pSensorHysRes =
		(GetSensorHysterisisRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	{
		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
		{
			pSensorHysRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
			return sizeof(*pRes);
		}
	}

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorHysRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	if (pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].SensorReadType != THRESHOLD_SENSOR_CLASS)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
		pSensorHysRes->CompletionCode = CC_ILLEGAL_CMD_FOR_SENSOR_REC;
		return sizeof(*pRes);
	}

	if (SENSOR_HYSTERESIS_ACCESS_BITS == (pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].SensorCaps & (BIT5 | BIT4)))
	{
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
		//set operation is not allowed
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

	/* Get the hysterisis values */
	pSensorHysRes->PositiveHysterisis = pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].PosHysteresis;
	pSensorHysRes->NegativeHysterisis = pSenSharedMem->SensorInfo[pSensorHysReq->SensorNum].NegHysteresis;

	/* Release mutex for Sensor shared memory */
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	pSensorHysRes->CompletionCode = CC_NORMAL;

	return sizeof(GetSensorHysterisisRes_T);
}

/*---------------------------------------
 * SetSensorThresholds
 *---------------------------------------*/
int SetSensorThresholds(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	SetSensorThresholdReq_T *pSensorThreshReq =
		(SetSensorThresholdReq_T *)pReq;
	SetSensorThresholdRes_T *pSensorThreshRes =
		(SetSensorThresholdRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	INT8U SettableMask = 0;
	INT16U LUNSensorNum = 0;
	INT16U OwnerLUN = 0;

	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	//	if(g_corefeatures.more_than_256_sensors == ENABLED)
	//	{
	//		OS_THREAD_TLS_GET(g_tls.NetFnLUN,OwnerLUN);

	//		if (OwnerLUN != 0xFF)
	//		LUNSensorNum = ((OwnerLUN & VALID_LUN) << 8 | pSensorThreshReq->SensorNum);
	//		else
	//		LUNSensorNum = pSensorThreshReq->SensorNum;
	//	}
	//	else
	//	{
	//		LUNSensorNum = pSensorThreshReq->SensorNum;
	//	}

	/* Check for the reserved bytes should b zero */

	if (0 != (pSensorThreshReq->SetFlags & RESERVED_BITS_SETSENSORTHRESHOLDS))
	{
		pSensorThreshRes->CompletionCode = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

	if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	{
		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
		{
			pSensorThreshRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
			return sizeof(*pRes);
		}
	}

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSenSharedMem->SensorInfo[LUNSensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorThreshRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	if (pSenSharedMem->SensorInfo[LUNSensorNum].SensorReadType != THRESHOLD_SENSOR_CLASS)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
		pSensorThreshRes->CompletionCode = CC_ILLEGAL_CMD_FOR_SENSOR_REC;
		return sizeof(*pRes);
	}

	if (BIT3 != (pSenSharedMem->SensorInfo[LUNSensorNum].SensorCaps & (BIT2 | BIT3)))
	{
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
		//set operation is not allowed
		*pRes = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

	pSensorThreshRes->CompletionCode = CC_NORMAL;
	SettableMask = (INT8U)(pSenSharedMem->SensorInfo[LUNSensorNum].SettableThreshMask >> 8);
	/* Set the threshold values */
	//if (pSenSharedMem->SensorInfo [pSensorThreshReq->SensorNum].SensorInit & BIT5)
	if ((SettableMask | pSensorThreshReq->SetFlags) == SettableMask) //<<Modified to support "set sensor enable>>
	{
		if ((pSensorThreshReq->SetFlags & LWR_CRIT) && (SettableMask & LWR_CRIT))
		{
			pSenSharedMem->SensorInfo[LUNSensorNum].LowerCritical = pSensorThreshReq->LowerCritical;
		}
		if ((pSensorThreshReq->SetFlags & LWR_NON_CRIT) && (SettableMask & LWR_NON_CRIT))
		{
			pSenSharedMem->SensorInfo[LUNSensorNum].LowerNonCritical = pSensorThreshReq->LowerNonCritical;
		}
		if ((pSensorThreshReq->SetFlags & LWR_NON_REC) && (SettableMask & LWR_NON_REC))
		{
			pSenSharedMem->SensorInfo[LUNSensorNum].LowerNonRecoverable = pSensorThreshReq->LowerNonRecoverable;
		}
		if ((pSensorThreshReq->SetFlags & UPR_CRIT) && (SettableMask & UPR_CRIT))
		{
			pSenSharedMem->SensorInfo[LUNSensorNum].UpperCritical = pSensorThreshReq->UpperCritical;
		}
		if ((pSensorThreshReq->SetFlags & UPR_NON_CRIT) && (SettableMask & UPR_NON_CRIT))
		{
			pSenSharedMem->SensorInfo[LUNSensorNum].UpperNonCritical = pSensorThreshReq->UpperNonCritical;
		}
		if ((pSensorThreshReq->SetFlags & UPR_NON_REC) && (SettableMask & UPR_NON_REC))
		{
			pSenSharedMem->SensorInfo[LUNSensorNum].UpperNonRecoverable = pSensorThreshReq->UpperNonRecoverable;
		}

		if (pBMCInfo->IpmiConfig.RearmSetSensorThreshold == 1)
		{
			/*  Since Changes in Threshold  .We have to monitor the sensor again .*/
			/* Already the Sensor  reached the particular state . so we have to reset for generate the event according to  the new Event mask */
			if (0 == (pSenSharedMem->SensorInfo[LUNSensorNum].SensorCaps & BIT6))
			{
				/* Manual ReARM Sensor */
				pSenSharedMem->SensorInfo[LUNSensorNum].AssertionHistoryByte1 = 0;
				pSenSharedMem->SensorInfo[LUNSensorNum].AssertionHistoryByte2 = 0;
				pSenSharedMem->SensorInfo[LUNSensorNum].DeassertionHistoryByte1 = 0;
				pSenSharedMem->SensorInfo[LUNSensorNum].DeassertionHistoryByte2 = 0;
				pSenSharedMem->SensorInfo[LUNSensorNum].EventLevel = SENSOR_STATUS_NORMAL;
				pSenSharedMem->SensorInfo[LUNSensorNum].HealthLevel = SENSOR_STATUS_NORMAL;
			}
			else
			{
				/* Auto ReARM Sensor */
				pSenSharedMem->SensorInfo[LUNSensorNum].EventLevel = SENSOR_STATUS_NORMAL;
				pSenSharedMem->SensorInfo[LUNSensorNum].HealthLevel = SENSOR_STATUS_NORMAL;
				pSenSharedMem->SensorInfo[LUNSensorNum].AssertionEventOccuredByte1 = 0;
				pSenSharedMem->SensorInfo[LUNSensorNum].AssertionEventOccuredByte2 = 0;
				pSenSharedMem->SensorInfo[LUNSensorNum].DeassertionEventOccuredByte1 = 0;
				pSenSharedMem->SensorInfo[LUNSensorNum].DeassertionEventOccuredByte2 = 0;
			}
		}

		pSensorThreshRes->CompletionCode = CC_NORMAL;
	}
	else
	{
		pSensorThreshRes->CompletionCode = CC_ILLEGAL_CMD_FOR_SENSOR_REC;
	}
	/* Release mutex for Sensor shared memory */
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	// To send notification to CIM
	//	if(g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM] != NULL)
	//	{
	//		uint8 CMD;
	//		// Set bits for SDR event & Modify operation
	//		CMD = 0x12;
	//		((int(*)(uint8,uint16))g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM])(CMD, LUNSensorNum);
	//	}

	return sizeof(SetSensorThresholdRes_T);
}

/*---------------------------------------
 * GetSensorThresholds
 *---------------------------------------*/
int GetSensorThresholds(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	GetSensorThresholdReq_T *pSensorThreshReq =
		(GetSensorThresholdReq_T *)pReq;
	GetSensorThresholdRes_T *pSensorThreshRes =
		(GetSensorThresholdRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	INT16U LUNSensorNum = 0;
	INT16U OwnerLUN = 0;

	GetSensorThresholdRes_T *thresholds_res = (GetSensorThresholdRes_T *)pRes;

	thresholds_res->CompletionCode = CC_NORMAL;
	if(pSensorThreshReq->SensorNum > SENSOR_NUM){
		thresholds_res->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(INT8U);
	}

	thresholds_res->GetFlags = 0x3F;

	if(pSensorThreshReq->SensorNum >0 && pSensorThreshReq->SensorNum <= SENSOR_TEMP_NUM){
		thresholds_res->LowerNonRecoverable = IpmiReadingDatConvert2Raw(IPMI_UNIT_DEGREES_C, 20);
		thresholds_res->LowerCritical = IpmiReadingDatConvert2Raw(IPMI_UNIT_DEGREES_C, 30);
		thresholds_res->LowerNonCritical = IpmiReadingDatConvert2Raw(IPMI_UNIT_DEGREES_C, 40);

		thresholds_res->UpperNonCritical = IpmiReadingDatConvert2Raw(IPMI_UNIT_DEGREES_C, 150);
		thresholds_res->UpperCritical = IpmiReadingDatConvert2Raw(IPMI_UNIT_DEGREES_C, 160);
		thresholds_res->UpperNonRecoverable = IpmiReadingDatConvert2Raw(IPMI_UNIT_DEGREES_C, 200);	
	}else if(pSensorThreshReq->SensorNum > SENSOR_TEMP_NUM && pSensorThreshReq->SensorNum <= SENSOR_NUM){
		INT8U LowerCritical, UpperCritical;
		switch(pSensorThreshReq->SensorNum - SENSOR_TEMP_NUM-1)
		{
		case 0:  // 3.3
			LowerCritical = 120;
			UpperCritical = 136;
			break;
		case 1:  // 1.8
			LowerCritical = 130;
			UpperCritical = 150;
			break;
		case 2:  // 1.1
			LowerCritical = 80;
			UpperCritical = 90;
			break;
		case 3:  // 0.9
			LowerCritical = 65;
			UpperCritical = 75;
			break;
		default:
			break;
		}
		thresholds_res->LowerNonRecoverable = 0;
		thresholds_res->LowerCritical = LowerCritical;
		thresholds_res->LowerNonCritical = LowerCritical;

		thresholds_res->UpperNonCritical = UpperCritical;
		thresholds_res->UpperCritical = UpperCritical;
		thresholds_res->UpperNonRecoverable = 255;	
	}


	return sizeof(GetSensorThresholdRes_T);
#if 0
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	//	if(g_corefeatures.more_than_256_sensors == ENABLED)
	//	{
	//		OS_THREAD_TLS_GET(g_tls.NetFnLUN,OwnerLUN);

	//		if (OwnerLUN != 0xFF)
	//		LUNSensorNum = ((OwnerLUN & VALID_LUN) << 8 | pSensorThreshReq->SensorNum);
	//		else
	//		LUNSensorNum = pSensorThreshReq->SensorNum;
	//	}
	//	else
	//	{
	//		LUNSensorNum = pSensorThreshReq->SensorNum;
	//	}

	IPMI_DBG_PRINT_2("GetSensorThresholds: OwnerLUN = %x OwnerLUNSensorNum = 0x%4x \n", OwnerLUN, LUNSensorNum);

	if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	{
		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
		{
			pSensorThreshRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
			return sizeof(*pRes);
		}
	}

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSenSharedMem->SensorInfo[LUNSensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorThreshRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	if (pSenSharedMem->SensorInfo[LUNSensorNum].SensorReadType != THRESHOLD_SENSOR_CLASS)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
		pSensorThreshRes->CompletionCode = CC_ILLEGAL_CMD_FOR_SENSOR_REC;
		return sizeof(*pRes);
	}

	_fmemset(pSensorThreshRes, 0, sizeof(GetSensorThresholdRes_T));

	if (SENSOR_THRESOLD_ACCESS_BITS != (pSenSharedMem->SensorInfo[LUNSensorNum].SensorCaps & (BIT2 | BIT3)))
	{
		pSensorThreshRes->GetFlags = pSenSharedMem->SensorInfo[LUNSensorNum].SettableThreshMask & 0xFF;

		/* Get the Threshold values according to readable threshold flag */
		if (pSensorThreshRes->GetFlags & BIT0)
			pSensorThreshRes->LowerNonCritical = pSenSharedMem->SensorInfo[LUNSensorNum].LowerNonCritical;
		if (pSensorThreshRes->GetFlags & BIT1)
			pSensorThreshRes->LowerCritical = pSenSharedMem->SensorInfo[LUNSensorNum].LowerCritical;
		if (pSensorThreshRes->GetFlags & BIT2)
			pSensorThreshRes->LowerNonRecoverable = pSenSharedMem->SensorInfo[LUNSensorNum].LowerNonRecoverable;
		if (pSensorThreshRes->GetFlags & BIT3)
			pSensorThreshRes->UpperNonCritical = pSenSharedMem->SensorInfo[LUNSensorNum].UpperNonCritical;
		if (pSensorThreshRes->GetFlags & BIT4)
			pSensorThreshRes->UpperCritical = pSenSharedMem->SensorInfo[LUNSensorNum].UpperCritical;
		if (pSensorThreshRes->GetFlags & BIT5)
			pSensorThreshRes->UpperNonRecoverable = pSenSharedMem->SensorInfo[LUNSensorNum].UpperNonRecoverable;
	}

	/* Release mutex for Sensor shared memory */
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
	pSensorThreshRes->CompletionCode = CC_NORMAL;

	return sizeof(GetSensorThresholdRes_T);
#endif	
}

/*---------------------------------------
 * GetSensorReadingFactors
 *---------------------------------------*/
int GetSensorReadingFactors(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	GetSensorReadingFactorReq_T *pSensorFactorsReq =
		(GetSensorReadingFactorReq_T *)pReq;
	GetSensorReadingFactorRes_T *pSensorFactorsRes =
		(GetSensorReadingFactorRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	// if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	// {
	// 	if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
	// 	{
	// 		pSensorFactorsRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
	// 		return sizeof(*pRes);
	// 	}
	// }

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSenSharedMem->SensorInfo[pSensorFactorsReq->SensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		// OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorFactorsRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	_fmemcpy(&(pSensorFactorsRes->M_LSB), &(pSenSharedMem->SensorInfo[pSensorFactorsReq->SensorNum].M_LSB),
			 sizeof(GetSensorReadingFactorRes_T) - (2 * sizeof(INT8U)));
	pSensorFactorsRes->CompletionCode = CC_NORMAL;

	/* Release mutex for Sensor shared memory */
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	return sizeof(GetSensorReadingFactorRes_T);
}

/*---------------------------------------
 * SetSensorEventEnable
 *---------------------------------------*/
int SetSensorEventEnable(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	SensorInfo_T *pSensorInfo;
	INT8U LocalReq[6];
	SetSensorEventEnableReq_T *pSensorEvtEnReq;
	SetSensorEventEnableRes_T *pSensorEvtEnRes =
		(SetSensorEventEnableRes_T *)pRes;
	INT16U AssertMask, DeassertMask;
	INT16U ValidMask = htoipmi_u16(0x0FFF);
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	INT16U LUNSensorNum = 0;
	INT16U OwnerLUN = 0;

	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	{
		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
		{
			pSensorEvtEnRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
			return sizeof(*pRes);
		}
	}

	AssertMask = DeassertMask = 0;

	memset(LocalReq, 0, sizeof(LocalReq));
	memcpy(LocalReq, pReq, ReqLen);
	pSensorEvtEnReq = (SetSensorEventEnableReq_T *)LocalReq;

	//	if(g_corefeatures.more_than_256_sensors == ENABLED)
	//	{
	//		OS_THREAD_TLS_GET(g_tls.NetFnLUN,OwnerLUN);

	//		if (OwnerLUN != 0xFF)
	//		LUNSensorNum = ((OwnerLUN & VALID_LUN) << 8 | pSensorEvtEnReq->SensorNum);
	//		else
	//		LUNSensorNum = pSensorEvtEnReq->SensorNum;
	//	}
	//	else
	//	{
	//		LUNSensorNum = pSensorEvtEnReq->SensorNum;
	//	}

	/* Atleast two bytes are expected remaining bytes (3,4,5,6) are optional */
	if ((ReqLen < sizeof(INT16U)) || (ReqLen > sizeof(SetSensorEventEnableReq_T)))
	{
		pSensorEvtEnRes->CompletionCode = CC_REQ_INV_LEN;
		return sizeof(*pRes);
	}

	/* Check for the reserved bytes should b zero */

	if (0 != (pSensorEvtEnReq->Flags & RESERVED_BITS_SETSENEVTEN_FLAGS))
	{
		pSensorEvtEnRes->CompletionCode = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}

	if (ReqLen > sizeof(INT16U))
	{
		/* Check for the reserved bits */
		if (pSenSharedMem->SensorInfo[LUNSensorNum].SensorReadType == THRESHOLD_SENSOR_CLASS)
		{
			if ((pSensorEvtEnReq->AssertionMask & ~ValidMask) ||
				(pSensorEvtEnReq->DeAssertionMask & ~ValidMask))
			{
				pSensorEvtEnRes->CompletionCode = CC_INV_DATA_FIELD;
				return sizeof(*pRes);
			}
		}
		else
		{
			if ((pSensorEvtEnReq->AssertionMask & RESERVED_BITS_SETSENEVTEN_ASSERTIONMASK) ||
				(pSensorEvtEnReq->DeAssertionMask & RESERVED_BITS_SETSENEVTEN_DEASSERTIONMASK))
			{
				pSensorEvtEnRes->CompletionCode = CC_INV_DATA_FIELD;
				return sizeof(*pRes);
			}
		}
	}

	/* Get the sensor Info for the sensor */
	//	if(g_corefeatures.more_than_256_sensors == ENABLED)
	//	{
	//		pSensorInfo = GetSensorInfo (pSensorEvtEnReq->SensorNum, OwnerLUN, BMCInst);
	//	}
	//	else
	//	{
	//		pSensorInfo = GetSensorInfo (pSensorEvtEnReq->SensorNum, 0x0, BMCInst);
	//	}

	if (NULL == pSensorInfo)
	{
		pSensorEvtEnRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	// If not threshold, adjust mask
	if (pSensorInfo->EventTypeCode != 0x01)
	{
		ValidMask = htoipmi_u16(0x7FFF);
	}

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSensorInfo->IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorEvtEnRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	/* Disable Events and scanning based on the flags */
	if (0 == (pSensorEvtEnReq->Flags & EVENT_MSG_MASK))
	{
		/* DisableAllEventSensors () */
	}

	if ((0 == (pSensorEvtEnReq->Flags & SCAN_MASK)) && (SCAN_MASK == (pSensorInfo->EventFlags & SCAN_MASK)))
	{
		// Check sensor accepts the ‘enable/disable scanning’
		if (0 == (pSensorInfo->SensorInit & BIT6))
		{
			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
			pSensorEvtEnRes->CompletionCode = CC_INV_DATA_FIELD;
			return sizeof(*pRes);
		}
		pSensorInfo->EventFlags |= BIT5; ///* Bit 5 -  Unable to read           */
	}
	else if ((SCAN_MASK == (pSensorEvtEnReq->Flags & SCAN_MASK)) && (0 == (pSensorInfo->EventFlags & SCAN_MASK)))
	{
		// Check sensor accepts the ‘enable/disable scanning’
		if (0 == (pSensorInfo->SensorInit & BIT6))
		{
			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
			pSensorEvtEnRes->CompletionCode = CC_INV_DATA_FIELD;
			return sizeof(*pRes);
		}
		pSensorInfo->EventFlags &= ~BIT5; ///* Bit 5 -  Unable to read           */
	}
	pSensorInfo->EventFlags &= ~(EVENT_MSG_MASK | SCAN_MASK);
	pSensorInfo->EventFlags |= (pSensorEvtEnReq->Flags & (EVENT_MSG_MASK | SCAN_MASK));

	AssertMask =
		htoipmi_u16(((pSenSharedMem->SensorInfo[LUNSensorNum].AssertionEventEnablesByte2 << 8) |
					 (pSenSharedMem->SensorInfo[LUNSensorNum].AssertionEventEnablesByte1)) &
					ValidMask);

	DeassertMask =
		htoipmi_u16(((pSenSharedMem->SensorInfo[LUNSensorNum].DeassertionEventEnablesByte2 << 8) |
					 (pSenSharedMem->SensorInfo[LUNSensorNum].DeassertionEventEnablesByte1)) &
					ValidMask);

	/* Enable disable assertion based on the flag */
	if (ENABLE_DISABLE_EVENT_MASK == (pSensorEvtEnReq->Flags & ENABLE_DISABLE_EVENT_MASK))
	{
		// Flags [5:4] - 11b Reserved.
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorEvtEnRes->CompletionCode = CC_INV_DATA_FIELD;
		return sizeof(*pRes);
	}
	else if (ENABLE_SELECTED_EVENT_MSG == (pSensorEvtEnReq->Flags & ENABLE_DISABLE_EVENT_MASK))
	{
		AssertMask |= ipmitoh_u16(pSensorEvtEnReq->AssertionMask);
		DeassertMask |= ipmitoh_u16(pSensorEvtEnReq->DeAssertionMask);
	}
	else if (DISABLE_SELECTED_EVENT_MSG == (pSensorEvtEnReq->Flags & ENABLE_DISABLE_EVENT_MASK))
	{
		AssertMask &= ~ipmitoh_u16(pSensorEvtEnReq->AssertionMask);
		DeassertMask &= ~ipmitoh_u16(pSensorEvtEnReq->DeAssertionMask);
	}

	//For Threshold class sensors upper word bits are reserved
	if (pSensorInfo->EventTypeCode == THRESHOLD_SENSOR_CLASS)
	{
		pSensorInfo->AssertionEventEnablesByte2 &= 0xF0;
		pSensorInfo->AssertionEventEnablesByte2 |= (AssertMask >> 8);
	}
	else
	{
		pSensorInfo->AssertionEventEnablesByte2 = (AssertMask >> 8);
	}
	pSensorInfo->AssertionEventEnablesByte1 = (AssertMask)&0xFF;

	//For Threshold class sensors upper word bits are reserved
	if (pSensorInfo->EventTypeCode == THRESHOLD_SENSOR_CLASS)
	{
		pSensorInfo->DeassertionEventEnablesByte2 &= 0xF0;
		pSensorInfo->DeassertionEventEnablesByte2 |= (DeassertMask >> 8);
	}
	else
	{
		pSensorInfo->DeassertionEventEnablesByte2 = (DeassertMask >> 8);
	}

	pSensorInfo->DeassertionEventEnablesByte1 = (DeassertMask);

	// For threshold sensors, reset the threshold state machine for the sensor
	// in order to pickup any newly enabled events.
	if (pSensorInfo->EventTypeCode == THRESHOLD_SENSOR_CLASS)
	{
		pSensorInfo->EventLevel = SENSOR_STATUS_NORMAL;
		pSensorInfo->HealthLevel = SENSOR_STATUS_NORMAL;
	}

	pSensorEvtEnRes->CompletionCode = CC_NORMAL;

	/* Release mutex for Sensor shared memory */
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	return sizeof(SetSensorEventEnableRes_T);
}

/*---------------------------------------
 * GetSensorEventEnable
 *---------------------------------------*/
int GetSensorEventEnable(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	GetSensorEventEnableReq_T *pSensorEvtEnReq =
		(GetSensorEventEnableReq_T *)pReq;
	GetSensorEventEnableRes_T *pSensorEvtEnRes =
		(GetSensorEventEnableRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	INT16U ValidMask = htoipmi_u16(0x0FFF);

	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	{
		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
		{
			pSensorEvtEnRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
			return sizeof(*pRes);
		}
	}

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSenSharedMem->SensorInfo[pSensorEvtEnReq->SensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorEvtEnRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	if (pSenSharedMem->SensorInfo[pSensorEvtEnReq->SensorNum].EventTypeCode != 0x01)
	{
		ValidMask = htoipmi_u16(0x7FFF);
	}
	/* Get the assertion enables */
	pSensorEvtEnRes->AssertionMask =
		htoipmi_u16(((pSenSharedMem->SensorInfo[pSensorEvtEnReq->SensorNum].AssertionEventEnablesByte2 << 8) |
					 (pSenSharedMem->SensorInfo[pSensorEvtEnReq->SensorNum].AssertionEventEnablesByte1)) &
					ValidMask);

	/* Get the deassertion enables */
	pSensorEvtEnRes->DeAssertionMask =
		htoipmi_u16(((pSenSharedMem->SensorInfo[pSensorEvtEnReq->SensorNum].DeassertionEventEnablesByte2 << 8) |
					 (pSenSharedMem->SensorInfo[pSensorEvtEnReq->SensorNum].DeassertionEventEnablesByte1)) &
					ValidMask);

	/* Set the flags */
	pSensorEvtEnRes->Flags = (pSenSharedMem->SensorInfo[pSensorEvtEnReq->SensorNum].EventFlags & 0xc0);

	pSensorEvtEnRes->CompletionCode = CC_NORMAL;

	/* Release mutex for Sensor shared memory */
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	return sizeof(GetSensorEventEnableRes_T);
}

/**
 * @fn CheckForEntityPresence
 * @brief This function checks for the entity presence bit
 * 			or the entity presence sensor.
 * @param[in] SensorNum - Sensor number for reading.
 * @param[in] EventTypeCode - Event type code of the sdr.
 * @param[in] SensorType - Sensor type of the sdr.
 * @retval 	ENTITY_FOUND, if present,
 * 			ENTITY_NOT_FOUND, if not present,
 * 			NOT_FOUND_SCAN_DISABLED, if not able to find,
 * 			NOT_FOUND_SCAN_ENABLED, if Scanning bit enabled but failed.
 */
static int
CheckForEntityPresence(INT8U SensorNum, INT8U OwnerLUN, INT8U EventTypeCode, INT8U SensorType, int BMCInst)
{
	int RetVal = NOT_FOUND_SCAN_ENABLED;
	SensorInfo_T *pSensorInfo;
	int i;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	SensorSharedMem_T *pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem;
	int override = 0;

	pSensorInfo = &(pSenSharedMem->SensorInfo[((OwnerLUN & VALID_LUN) << 8 | SensorNum)]);

	TDBG("Entered %s : Sensor Type : %x, SensorInit : %x, Reading : %x\n",
		 __func__, SensorType, pSensorInfo->SensorInit,
		 pSensorInfo->SensorReading);

	// Hook to override the default values with any OEM values
	// We will pass the pointer to the SensorInfo of the particular sensor we are operating on.
	// We will also pass the pointer to the SDR Record that will updated into the SensorInfo structure
	//	if (g_PDKHandle[PDK_OVERRIDE_CHK_ENTITY_PRESENCE] != NULL)
	//	{
	//		override = ((int(*)(INT8U, INT8U, INT8U, INT8U, int))g_PDKHandle[PDK_OVERRIDE_CHK_ENTITY_PRESENCE])
	//		(SensorNum, OwnerLUN, EventTypeCode, SensorType, BMCInst);
	//		if (override < 0)
	//		return RetVal;
	//	}

	if (BIT0 == (BIT0 & pSensorInfo->SensorInit))
	{
		// If Event/Reading type code is 0x08 then look for
		// reading DEVICE_PRESENT to state the Entity Presence
		if (GENERIC_EVENT_TYPE_DEV_PRESENCE == EventTypeCode)
		{
			if (DEVICE_PRESENT == pSensorInfo->SensorReading)
			{
				RetVal = ENTITY_FOUND;
			}
			else
			{
				RetVal = ENTITY_NOT_FOUND;
			}
		}
		// If Event/Reading type code is 0x09 then look for
		// reading DEVICE_ENABLED to state the Entity Enabled
		else if (GENERIC_EVENT_TYPE_DEV_AVAILABLE == EventTypeCode)
		{
			if (DEVICE_ENABLED == pSensorInfo->SensorReading)
			{
				RetVal = ENTITY_FOUND;
			}
			else
			{
				RetVal = ENTITY_NOT_FOUND;
			}
		}
		// If Event/Reading Type code is 0x6f then look for the
		// special sensors like Processor, Memory etc.,
		else if (EVENT_TYPE_SENSOR_SPECIFIC == EventTypeCode)
		{
			for (i = 0; i < SENSOR_PRESENCE_COUNT; i++)
			{
				if (SensorType == sensor_presence[i][0])
				{
					if (pSensorInfo->SensorReading & sensor_presence[i][1])
						RetVal = ENTITY_FOUND;
					else
						RetVal = ENTITY_NOT_FOUND;
				}
			}
		}

		TDBG("Leaving : %s with %d\n", __func__, RetVal);
		return RetVal;
	}
	TDBG("Leaving : %s with %d\n", __func__, NOT_FOUND_SCAN_DISABLED);
	return NOT_FOUND_SCAN_DISABLED;
}

/**
 * @fn IsSensorPresence
 * @brief Check the presence bit for the entity or checks with 
 * 	  entity presence sensor to identify Entity presence
 * @param[in] EntityID - Entity id of the sensor.
 * @param[in] EntityIns - Entity instance of the sensor.
 * @param[in] SensorNum - Sensor number.
 * @retval	1, if present
 * 		0, if not present
 * 		-1, if not able to find.
 */
static int
IsSensorPresence(INT8U EntityID, INT8U EntityIns, INT8U SensorNum, INT8U OwnerLUN, int BMCInst)
{
	SDRRecHdr_T *sr = NULL;
	CommonSensorRec_T *scs = NULL;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;

	int RetVal, tmpRet = -1;

	TDBG("Entered : %s with EntityID : %x, SensorNum : %x, OwnerLUN : %x\n", __func__, EntityID, SensorNum, OwnerLUN);

	sr = SDR_GetFirstSDRRec(BMCInst);

	while (0 != sr)
	{
		// Populate sensor information for Threshold sensors
		if (FULL_SDR_REC == sr->Type ||
			COMPACT_SDR_REC == sr->Type)
		{
			/* Populate sensor information for full or Compact Record */
			scs = (CommonSensorRec_T *)sr;
			TDBG("COMPACT SDR REC SensorNum : %x, OwnerLUN : %x, EntityID : %x, EventTypeCode : %x\n",
				 scs->SensorNum, scs->OwnerLUN, scs->EntityID, scs->EventTypeCode);

			// Check for EntityId and EntityIns
			if ((pBMCInfo->IpmiConfig.BMCSlaveAddr == scs->OwnerID) &&
				(EntityID == scs->EntityID) &&
				(EntityIns == scs->EntityIns) &&
				((SensorNum != scs->SensorNum) || (OwnerLUN != scs->OwnerLUN)))
			{
				RetVal = CheckForEntityPresence(scs->SensorNum, scs->OwnerLUN, scs->EventTypeCode,
												scs->SensorType, BMCInst);
				if (NOT_FOUND_SCAN_DISABLED != RetVal &&
					NOT_FOUND_SCAN_ENABLED != RetVal)
				{
					return RetVal;
				}
				else if (NOT_FOUND_SCAN_ENABLED == RetVal)
				{
					// An Entity is present if there is at least one active
					// sensor for the Entity (and there is no explicit sensor saying
					// the Entity is 'absent').
					// A sensor is 'active' if scanning is enabled.
					// We can return this value only after searching all the sensors.
					tmpRet = 1;
				}
			}
		}
		/* Get the next record */
		sr = SDR_GetNextSDRRec(sr, BMCInst);
	}
	TDBG("Leaving : %s with %d\n", __func__, tmpRet);
	return tmpRet;
}

/**
 * @fn IsEntityAssociationPresence
 * @brief Check the EntityID and Entity instance to see if the entity is 
 * 	  a container entity in an entity-association. If so, check to 
 * 	  see if any of the contained entities are present, if so, assume 
 * 	  the container entity exists.
 * @param[in] EntityID - Entity id of the sensor.
 * @param[in] EntityIns - Entity instance of the sensor.
 * @retval	1, if present
 * 		0, if not present
 * 		-1, if not able to find.
 */
static int
IsEntityAssociationPresence(INT8U EntityID, INT8U EntityIns)
{
	TDBG("Entered : %s\n", __func__);
	// TODO: Entity Association record has to be handle later.
	TDBG("Leaving : %s with -1\n", __func__);
	return -1;
}

/**
 * @fn IsFRUPresence
 * @brief Check the entity to see if FRU device is present.
 * @param[in] EntityID - Entity id of the sensor.
 * @param[in] EntityIns - Entity instance of the sensor.
 * @retval	1, if present
 * 		0, if not present
 * 		-1, if not able to find.
 */
static int
IsFRUPresence(INT8U EntityID, INT8U EntityIns, int BMCInst)
{
	//	SDRRecHdr_T* sr = NULL;
	//	FRUDevLocatorRec_T* frudl;
	//	FRUReadReq_T FRUReadReq;
	//	INT8U FRUReadRes[64];

	//	TDBG("Entered : %s\n", __func__);

	//	sr = SDR_GetFirstSDRRec (BMCInst);
	//	while (0 != sr)
	//	{
	//		// Check for FRU Device locator SDR Record
	//		if (sr->Type == FRU_DEVICE_LOCATOR_SDR_REC)
	//		{
	//			frudl = (FRUDevLocatorRec_T*) sr;
	//			TDBG("if Success : EntityID : %x, EntityIns : %x\n",
	//					frudl->EntityID, frudl->EntityIns);
	//			// If EntityID and EntityIns are equal try to read the fru data.
	//			if(frudl->EntityID == EntityID &&
	//					frudl->EntityIns == EntityIns)
	//			{
	//				FRUReadReq.FRUDeviceID=frudl->FRUIDSlaveAddr;
	//				FRUReadReq.Offset=0x0;
	//				FRUReadReq.CountToRead=sizeof(FRUCommonHeader_T);
	//				ReadFRUData((INT8U *)&FRUReadReq, sizeof(FRUReadReq_T), FRUReadRes,BMCInst);
	//				if (((FRUReadRes_T *)FRUReadRes)->CompletionCode == FRU_ACCESSIBLE)
	//				{
	//					TDBG("Leaving : %s with 1\n", __func__);
	//					return 1;
	//				}
	//				else
	//				{
	//					TDBG("Leaving : %s with 0\n", __func__);
	//					return 0;
	//				}
	//			}
	//		}
	//		sr = SDR_GetNextSDRRec (sr,BMCInst);
	//	}
	//	TDBG("Leaving : %s with -1\n", __func__);
	return -1;
}

/**
 * @fn InitSensorScanningBit
 * @brief Initializes all the sensor's Scanning bit with respect 
 *        to the presence of the entity
 * @retval 0.
 */

int InitSensorScanningBit(int BMCInst)
{
	SDRRecHdr_T *sr = NULL;
	CommonSensorRec_T *scs = NULL;
	int RetVal;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	SensorSharedMem_T *pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem;
	INT16U LUNSensorNum;

	TDBG("Entered : %s\n", __func__);
	sr = SDR_GetFirstSDRRec(BMCInst);

	while (0 != sr)
	{
		// Populater sensor information for Threshold sensors
		if (FULL_SDR_REC == sr->Type ||
			COMPACT_SDR_REC == sr->Type)
		{
			scs = (CommonSensorRec_T *)sr;

			if ((pBMCInfo->IpmiConfig.BMCSlaveAddr == scs->OwnerID))
			{
				//                TDBG("Check for Entity Presence : SensorNum : %x, EntityId : %x, "
				//                    "EntityIns : %x, SensorCaps : %x\n",
				//                    scs->SensorNum, scs->EntityID, scs->EntityIns,
				//                    (scs->SensorCaps & 0x80));
				RetVal = 1;
				if (BIT7 == (BIT7 & scs->SensorCaps))
				{
					/* As per IPMI Spec Section 40.2
					 * Entity presence can be detected if any one of the following point is
					 * Satisfied,
					 * 1. If there is an active sensor that includes a presence bit,
					 *    or the entity has an active Entity Presence sensor,
					 *    use the sensor to determine the presence of the entity.
					 * 2. Check the SDRs to see if the entity is a container entity
					 *    in an entity-association. If so, check to see if any of the
					 *    contained entities are present, if so, assume the container
					 *    entity exists. Note that this may need to be iterative.
					 * 3. The entity is present is there is a FRU device for the
					 *    entity, and the FRU device is present.
					 */

					RetVal = IsSensorPresence(scs->EntityID, scs->EntityIns, scs->SensorNum, scs->OwnerLUN, BMCInst);
					if (-1 == RetVal)
					{
						RetVal = IsEntityAssociationPresence(scs->EntityID, scs->EntityIns);
						if (-1 == RetVal)
						{
							RetVal = IsFRUPresence(scs->EntityID, scs->EntityIns, BMCInst);
						}
					}
				}
				LUNSensorNum = ((scs->OwnerLUN & VALID_LUN) << 8 | scs->SensorNum);
				/*
				 * [7] - 0b = All Event Messages disabled from this sensor
				 * [6] - 0b = sensor scanning disabled
				 * [5] - 1b = reading/state unavailable
				 */
				if (1 == RetVal)
				{
					if ((BIT0 | BIT1) == (pSenSharedMem->SensorInfo[LUNSensorNum].SensorInit & (BIT0 | BIT1)))
					{
						/* Enabling Sensor Scanning and Event Messages*/
						pSenSharedMem->SensorInfo[LUNSensorNum].EventFlags = EVENT_AND_SCANNING_ENABLE;
					}
					else if (BIT1 == (pSenSharedMem->SensorInfo[LUNSensorNum].SensorInit & BIT1))
					{
						/* Enabling Event Messages */
						pSenSharedMem->SensorInfo[LUNSensorNum].EventFlags = EVENT_MSG_MASK;
					}
					else if (BIT0 == (pSenSharedMem->SensorInfo[LUNSensorNum].SensorInit & BIT0))
					{
						/*Enabling Scanning*/
						pSenSharedMem->SensorInfo[LUNSensorNum].EventFlags = SCAN_MASK;
					}
					if (0 == (pSenSharedMem->SensorInfo[LUNSensorNum].SensorInit & BIT0))
					{
						/* Reading Unavailable */
						pSenSharedMem->SensorInfo[LUNSensorNum].EventFlags |= READING_UNAVAILABLE;
					}
				}
				else
				{
					pSenSharedMem->SensorInfo[LUNSensorNum].EventFlags = READING_UNAVAILABLE;
					if (BIT1 == (pSenSharedMem->SensorInfo[LUNSensorNum].SensorInit & BIT1))
					{
						/* Enabling Event Messages */
						pSenSharedMem->SensorInfo[LUNSensorNum].EventFlags |= EVENT_MSG_MASK;
					}
				}
				//                TDBG("\nSensor Scanning Bit for sensor %x : %x\n", scs->SensorNum,
				//                pSenSharedMem->SensorInfo[scs->SensorNum].EventFlags);
			}
		}
		/* Get the next record */
		sr = SDR_GetNextSDRRec(sr, BMCInst);
	}
	TDBG("Leaving : %s with 0\n", __func__);
	return 0;
}

/* Compare two sensor values.
 * Returns -1 if val1 < val2
 * Returns 0 if val1 == val2
 * Returns 1 if val1 > val2
 */
int CompareValues(BOOL isSigned, INT8U val1, INT8U val2)
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
		INT8S sval1, sval2;

		sval1 = (INT8S)val1;
		sval2 = (INT8S)val2;

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

/*-----------------------------------------
 * GetSensorReading
 *-----------------------------------------*/
int GetSensorReading(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	GetSensorReadingReq_T *pSensorReadReq =
		(GetSensorReadingReq_T *)pReq;
	GetSensorReadingRes_T *pSensorReadRes =
		(GetSensorReadingRes_T *)pRes;
	INT16U SensorReading;
	bool SensorIsSigned = FALSE;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	INT16U LUNSensorNum = 0;
	INT16U OwnerLUN = 0;
	INT16U tmp_value = 0;
	INT16U voltage_value = 0;
	bool get_res = false;    
    ADCChannlesRes adcRes;


	GetSensorReadingRes_T *sensor_reading_res = (GetSensorReadingRes_T *)pRes;

	sensor_reading_res->CompletionCode = CC_NORMAL;
	sensor_reading_res->Flags = 0xC0;
	sensor_reading_res->ComparisonStatus = 0x00;
	sensor_reading_res->OptionalStatus = 0x00;

	get_res = adc_getValByChannel(pSensorReadReq->SensorNum, &adcRes);
	if(!get_res){
		sensor_reading_res->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}
	sensor_reading_res->SensorReading = IpmiReadingDatConvert2Raw(adcRes.sensorUnitType, adcRes.adcVal);
	return sizeof(GetSensorReadingRes_T);
#if 0	
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	//	if(g_corefeatures.more_than_256_sensors == ENABLED)
	//	{
	//		OS_THREAD_TLS_GET(g_tls.NetFnLUN,OwnerLUN);

	//		if (OwnerLUN != 0xFF)
	//		LUNSensorNum = ((OwnerLUN & VALID_LUN) << 8 | pSensorReadReq->SensorNum);
	//		else
	//		LUNSensorNum = pSensorReadReq->SensorNum;
	//	}
	//	else
	//	{
	//		LUNSensorNum = pSensorReadReq->SensorNum;
	//	}
	LUNSensorNum = pSensorReadReq->SensorNum;

	IPMI_DBG_PRINT_2("GetSensorReading : OwnerLUN = %x LUNSensorNum = 0x%2x \n", OwnerLUN, LUNSensorNum);

	//	if( pBMCInfo->IpmiConfig.OPMASupport == 1)
	//	{
	//		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
	//		{
	//			pSensorReadRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
	//			return sizeof (*pRes);
	//		}
	//	}

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	//	if(g_PDKHandle[PDK_GETSPECIFICSENSORREADING] != NULL)
	//	{
	//		if( 0 != ((int(*)(INT8U *,INT8U,INT8U *,int))g_PDKHandle[PDK_GETSPECIFICSENSORREADING]) (pReq, ReqLen, pRes,BMCInst))
	//		{
	//			/* Release mutex for Sensor shared memory */
	//			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	//			// Sensor is handled inside PDK so do not continue
	//			pSensorReadRes->CompletionCode = CC_NORMAL;
	//			return sizeof (GetSensorReadingRes_T);
	//		}
	//	}

	if (!pSenSharedMem->SensorInfo[LUNSensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorReadRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	pSensorReadRes->Flags = pSenSharedMem->SensorInfo[LUNSensorNum].EventFlags & 0xe0;

	if (0 != (pSenSharedMem->SensorInfo[LUNSensorNum].EventFlags & BIT5))
	{
		pSensorReadRes->SensorReading = 0;
		pSensorReadRes->CompletionCode = CC_NORMAL;
		pSensorReadRes->ComparisonStatus = (((INT8U)(pSensorReadRes->SensorReading & 0x00FF)) & ((INT8U)(pSenSharedMem->SensorInfo[LUNSensorNum].SettableThreshMask & 0x00FF)));
		pSensorReadRes->OptionalStatus = (((INT8U)(pSensorReadRes->SensorReading >> 8)) & ((INT8U)(pSenSharedMem->SensorInfo[LUNSensorNum].SettableThreshMask >> 8)));
		// For Discrete sensor, [7] - reserved. Returned as 1b. Ignore on read.
		pSensorReadRes->OptionalStatus |= 0x80;
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		return sizeof(GetSensorReadingRes_T);
	}

	pSensorReadRes->CompletionCode = CC_NORMAL;

	//	if(g_corefeatures.cached_sensor_reading == ENABLED)
	//	{
	//		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
	//		OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->CachSenReadMutex,WAIT_INFINITE);
	//	}
	SensorReading = pSenSharedMem->SensorInfo[LUNSensorNum].SensorReading;

	//	if(g_corefeatures.cached_sensor_reading == ENABLED)
	//	{
	//		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->CachSenReadMutex);
	//		OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);
	//	}
	pSensorReadRes->SensorReading = 0;

	SensorIsSigned =
		(0 != (pSenSharedMem->SensorInfo[LUNSensorNum].InternalFlags & BIT1));

	if (THRESHOLD_SENSOR_CLASS == pSenSharedMem->SensorInfo[LUNSensorNum].EventTypeCode)
	{
		pSensorReadRes->SensorReading = (SensorReading & 0x00FF);
		pSensorReadRes->ComparisonStatus = 0x00;
		if ((pSenSharedMem->SensorInfo[LUNSensorNum].DeassertionEventEnablesByte2 & BIT6) == BIT6)
		{
			if (CompareValues(SensorIsSigned, pSensorReadRes->SensorReading,
							  pSenSharedMem->SensorInfo[LUNSensorNum].UpperNonRecoverable) >= 0)
			{
				pSensorReadRes->ComparisonStatus |= BIT5;
			}
		}
		if ((pSenSharedMem->SensorInfo[LUNSensorNum].DeassertionEventEnablesByte2 & BIT5) == BIT5)
		{
			if (CompareValues(SensorIsSigned, pSensorReadRes->SensorReading,
							  pSenSharedMem->SensorInfo[LUNSensorNum].UpperCritical) >= 0)
			{
				pSensorReadRes->ComparisonStatus |= BIT4;
			}
		}
		if ((pSenSharedMem->SensorInfo[LUNSensorNum].DeassertionEventEnablesByte2 & BIT4) == BIT4)
		{
			if (CompareValues(SensorIsSigned, pSensorReadRes->SensorReading,
							  pSenSharedMem->SensorInfo[LUNSensorNum].UpperNonCritical) >= 0)
			{
				pSensorReadRes->ComparisonStatus |= BIT3;
			}
		}
		if ((pSenSharedMem->SensorInfo[LUNSensorNum].AssertionEventEnablesByte2 & BIT6) == BIT6)
		{
			if (CompareValues(SensorIsSigned, pSensorReadRes->SensorReading,
							  pSenSharedMem->SensorInfo[LUNSensorNum].LowerNonRecoverable) <= 0)
			{
				pSensorReadRes->ComparisonStatus |= BIT2;
			}
		}
		if ((pSenSharedMem->SensorInfo[LUNSensorNum].AssertionEventEnablesByte2 & BIT5) == BIT5)
		{
			if (CompareValues(SensorIsSigned, pSensorReadRes->SensorReading,
							  pSenSharedMem->SensorInfo[LUNSensorNum].LowerCritical) <= 0)
			{
				pSensorReadRes->ComparisonStatus |= BIT1;
			}
		}
		if ((pSenSharedMem->SensorInfo[LUNSensorNum].AssertionEventEnablesByte2 & BIT4) == BIT4)
		{
			if (CompareValues(SensorIsSigned, pSensorReadRes->SensorReading,
							  pSenSharedMem->SensorInfo[LUNSensorNum].LowerNonCritical) <= 0)
			{
				pSensorReadRes->ComparisonStatus |= BIT0;
			}
		}

		pSensorReadRes->ComparisonStatus &=
			((pSenSharedMem->SensorInfo[LUNSensorNum].SettableThreshMask) & 0xFF);
		pSensorReadRes->OptionalStatus = 0;
		// For Threshold sensor, [7:6] - reserved. Returned as 1b. Ignore on read.
		pSensorReadRes->ComparisonStatus |= 0xC0;
	}
	else
	{
		pSensorReadRes->ComparisonStatus = (((INT8U)(SensorReading & 0x00FF)) & ((INT8U)(pSenSharedMem->SensorInfo[LUNSensorNum].SettableThreshMask & 0x00FF)));
		pSensorReadRes->OptionalStatus = (((INT8U)(SensorReading >> 8)) & ((INT8U)(pSenSharedMem->SensorInfo[LUNSensorNum].SettableThreshMask >> 8)));
		// For Discrete sensor, [7] - reserved. Returned as 1b. Ignore on read.
		pSensorReadRes->OptionalStatus |= 0x80;
	}

	/* Release mutex for Sensor shared memory */
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	return sizeof(GetSensorReadingRes_T);
#endif	
}

/*-----------------------------------------
 * SetSensorReading
 *-----------------------------------------*/
int SetSensorReading(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
	SetSensorReadingReq_T *pSensorReadReq =
		(SetSensorReadingReq_T *)pReq;
	SetSensorReadingRes_T *pSensorReadRes =
		(SetSensorReadingRes_T *)pRes;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	/* Check for the reserved bits */
	if (pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].SensorReadType == THRESHOLD_SENSOR_CLASS)
	{
		if (((ReqLen >= LEN_FOR_ASSERT_DATA) && (pSensorReadReq->AssertionEventOccuredByte2 & RESERVED_BITS_SETSENRD_ASSEVTOCCBYTE_1)) ||
			((ReqLen >= LEN_FOR_DEASSERT_DATA) && (pSensorReadReq->DeAssertionEventOccuredByte2 & RESERVED_BITS_SETSENRD_DEASSEVTOCCBYTE_1)))
		{
			pSensorReadRes->CompletionCode = CC_INV_DATA_FIELD;
			return sizeof(*pRes);
		}
	}
	else
	{
		if (((ReqLen >= LEN_FOR_ASSERT_DATA) && (pSensorReadReq->AssertionEventOccuredByte2 & RESERVED_BITS_SETSENRD_ASSEVTOCCBYTE_2)) ||
			((ReqLen >= LEN_FOR_DEASSERT_DATA) && (pSensorReadReq->DeAssertionEventOccuredByte2 & RESERVED_BITS_SETSENRD_DEASSEVTOCCBYTE_2)))
		{
			pSensorReadRes->CompletionCode = CC_INV_DATA_FIELD;
			return sizeof(*pRes);
		}
	}

	if (pBMCInfo->IpmiConfig.OPMASupport == 1)
	{
		if (pSenSharedMem->GlobalSensorScanningEnable == FALSE)
		{
			pSensorReadRes->CompletionCode = CC_PARAM_NOT_SUP_IN_CUR_STATE;
			return sizeof(*pRes);
		}
	}

	/* Acquire Shared memory   */
	OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex, WAIT_INFINITE);

	if (!pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].IsSensorPresent)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorReadRes->CompletionCode = CC_SDR_REC_NOT_PRESENT;
		return sizeof(*pRes);
	}

	if (ReqLen < MIN_SET_SEN_READING_CMD_LEN)
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorReadRes->CompletionCode = CC_REQ_INV_LEN;
		return sizeof(*pRes);
	}

	/* Check if the sensor is settable */
	if (0 == GET_SETTABLE_SENSOR_BIT(pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].SensorInit))
	{
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

		pSensorReadRes->CompletionCode = CC_INVALID_ATTEMPT_TO_SET;
		return sizeof(*pRes);
	}

	pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].Operation = pSensorReadReq->Operation;

	/* Set Sensor Event Data based on the Operation byte */
	switch (GET_EVENT_DATA_OP(pSensorReadReq->Operation))
	{
	case WRITE_NO_EVTDATA1:
		pSensorReadReq->EvtData1 &= 0xF0;
	/* Intentional Fall thru */
	case WRITE_EVTDATA1:
		if (LEN_FOR_EVT_DATA != ReqLen)
		{
			/* Release mutex for Sensor shared memory */
			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

			pSensorReadRes->CompletionCode = CC_REQ_INV_LEN;
			return sizeof(*pRes);
		}
		/* Update EvtData fields */
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].EvtData1 = pSensorReadReq->EvtData1;
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].EvtData2 = pSensorReadReq->EvtData2;
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].EvtData3 = pSensorReadReq->EvtData3;
		break;
	}

	/* Check Length for Assertion Set Opetation */
	if (0 != GET_ASSERT_EVT_OP(pSensorReadReq->Operation))
	{
		if ((ReqLen < LEN_FOR_ASSERT_DATA) || (ReqLen > MAX_SET_SEN_READ_LEN))
		{
			/* Release mutex for Sensor shared memory */
			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

			pSensorReadRes->CompletionCode = CC_REQ_INV_LEN;
			return sizeof(*pRes);
		}
	}

	/* Set Sensor Assertion Event based on the Operation byte */
	switch (GET_ASSERT_EVT_OP(pSensorReadReq->Operation))
	{
	case CLEAR_ASSERT_BITS:
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].AssertionEventOccuredByte1 &= pSensorReadReq->AssertionEventOccuredByte1;
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].AssertionEventOccuredByte2 &= pSensorReadReq->AssertionEventOccuredByte2;
		break;

	case SET_ASSERT_BITS:
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].AssertionEventOccuredByte1 |= pSensorReadReq->AssertionEventOccuredByte1;
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].AssertionEventOccuredByte2 |= pSensorReadReq->AssertionEventOccuredByte2;
		break;

	case WRITE_ASSERT_BITS:
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].AssertionEventOccuredByte1 |= pSensorReadReq->AssertionEventOccuredByte1;
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].AssertionEventOccuredByte2 |= pSensorReadReq->AssertionEventOccuredByte2;
		break;
	}

	/* Check Length for Assertion Set Opetation */
	if (0 != GET_DEASSERT_EVT_OP(pSensorReadReq->Operation))
	{
		if ((ReqLen < LEN_FOR_DEASSERT_DATA) || (ReqLen > MAX_SET_SEN_READ_LEN))
		{
			/* Release mutex for Sensor shared memory */
			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

			pSensorReadRes->CompletionCode = CC_REQ_INV_LEN;
			return sizeof(*pRes);
		}
	}

	/* Set Sensor DeAssertion Event based on the Operation byte */
	switch (GET_DEASSERT_EVT_OP(pSensorReadReq->Operation))
	{
	case CLEAR_DEASSERT_BITS:
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].DeassertionEventOccuredByte1 &= pSensorReadReq->DeAssertionEventOccuredByte1;
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].DeassertionEventOccuredByte2 &= pSensorReadReq->DeAssertionEventOccuredByte2;
		break;

	case SET_DEASSERT_BITS:
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].DeassertionEventOccuredByte1 |= pSensorReadReq->DeAssertionEventOccuredByte1;
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].DeassertionEventOccuredByte2 |= pSensorReadReq->DeAssertionEventOccuredByte2;
		break;

	case WRITE_DEASSERT_BITS:
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].DeassertionEventOccuredByte1 |= pSensorReadReq->DeAssertionEventOccuredByte1;
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].DeassertionEventOccuredByte2 |= pSensorReadReq->DeAssertionEventOccuredByte2;
		break;
	}

	/* Check Length for Set Sensor Reading Operation */
	if (0 != GET_SETSENSOR_OP(pSensorReadReq->Operation))
	{
		if ((ReqLen < LEN_FOR_SETSENSOR_DATA) || (ReqLen > MAX_SET_SEN_READ_LEN))
		{
			/* Release mutex for Sensor shared memory */
			OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

			pSensorReadRes->CompletionCode = CC_REQ_INV_LEN;
			return sizeof(*pRes);
		}

		/* Set new Sensor Reading */
		pSenSharedMem->SensorInfo[pSensorReadReq->SensorNum].SensorReading = pSensorReadReq->SensorReading;
	}

	pSensorReadRes->CompletionCode = CC_NORMAL;

	/* Release mutex for Sensor shared memory */
	OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

	return sizeof(SetSensorReadingRes_T);
}

/*-------------------------------------------
 * GetSensorSDR
 *-------------------------------------------*/
SDRRecHdr_T *
SR_GetSensorSDR(INT8U SensorNum, int BMCInst)
{
	SDRRecHdr_T *SDRRec;

	/* Search for the record containing the sensor */
	SDRRec = SDR_GetFirstSDRRec(BMCInst);
	while (0 != SDRRec)
	{
		switch (SDRRec->Type)
		{
		case FULL_SDR_REC:
			if (((FullSensorRec_T *)SDRRec)->SensorNum == SensorNum)
			{
				return SDRRec;
			}
			break;

		case COMPACT_SDR_REC:
		{
			INT16U SharedRecs = ipmitoh_u16(((CompactSensorRec_T *)SDRRec)->RecordSharing) &
								SHARED_RECD_COUNT;
			if ((SensorNum == ((CompactSensorRec_T *)SDRRec)->SensorNum) ||
				((SensorNum >= (((CompactSensorRec_T *)SDRRec)->SensorNum)) &&
				 (SensorNum < (((CompactSensorRec_T *)SDRRec)->SensorNum + SharedRecs))))
			{
				return SDRRec;
			}
		}
		break;

		default:
			break;
		}

		/* Get the next record */
		SDRRec = SDR_GetNextSDRRec(SDRRec, BMCInst);
		if (0 == SDRRec)
		{
			return 0;
		}
	}

	return 0;
}

/**
 * @brief Update global variables with number sensors.
 **/
static void
FindNumSensors(int BMCInst)
{
	SDRRecHdr_T *pSDRRec;
	FullSensorRec_T *pFSR;
	CompactSensorRec_T *pCSR;

	pFSR = 0;
	pCSR = 0;

	/* Get First SDR Record */
	pSDRRec = SDR_GetFirstSDRRec(BMCInst);
	while (0 != pSDRRec)
	{
		switch (pSDRRec->Type)
		{
		case FULL_SDR_REC:

			pFSR = (FullSensorRec_T *)pSDRRec;
			if (THRESHOLD_SENSOR_CLASS == pFSR->EventTypeCode)
			{
				g_BMCInfo.SenConfig.NumThreshSensors++;
			}
			else
			{
				g_BMCInfo.SenConfig.NumNonThreshSensors++;
			}
			break;

		case COMPACT_SDR_REC:

			pCSR = (CompactSensorRec_T *)pSDRRec;
			if (THRESHOLD_SENSOR_CLASS == pCSR->EventTypeCode)
			{
				g_BMCInfo.SenConfig.NumThreshSensors += (ipmitoh_u16(pCSR->RecordSharing) &
														 SHARED_RECD_COUNT);
			}
			else
			{
				g_BMCInfo.SenConfig.NumNonThreshSensors += (ipmitoh_u16(pCSR->RecordSharing) &
															SHARED_RECD_COUNT);
			}
			break;

		default:

			break;
		}

		/* Get the next record */
		pSDRRec = SDR_GetNextSDRRec(pSDRRec, BMCInst);
	}

	IPMI_DBG_PRINT_1("Thereshold  Sensors	= %d\n", g_BMCInfo.SenConfig.NumThreshSensors);
	IPMI_DBG_PRINT_1("Non Thershold Sensors	= %d\n", g_BMCInfo.SenConfig.NumNonThreshSensors);

	return;
}

int GetRecordIdsforDCMISensor(INT8U EntityID, INT8U SensorType, INT8U EntityInstance,
							  INT8U StartingEntityInstance, INT16U *pBuf, INT8U *pTotalValidInstances, int BMCInst)
{
	int i, ValidInstances;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	if (0 != EntityInstance)
	{
		*pTotalValidInstances = 0;
		ValidInstances = 0;
		/* Acquire Shared memory to populate sensor information  */
		//OS_ACQUIRE_MUTEX(m_hSensorSharedMemMutex, SHARED_MEM_TIMEOUT);
		OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo.SensorSharedMemMutex, WAIT_INFINITE);

		for (i = 0; i < MAX_SENSOR_NUMBERS + 1; i++)
		{
			if ((TRUE == pSenSharedMem->SensorInfo[i].IsDCMITempsensor) && (pSenSharedMem->SensorInfo[i].SDRRec->Type == FULL_SDR_REC))
			{
				if (SensorType != pSenSharedMem->SensorInfo[i].SensorTypeCode)
				{
					/*Check the Sensor type*/
					continue;
				}

				if ((((EntityID == DCMI_INLET_TEMP_ENTITY_ID) || (EntityID == IPMI_INLET_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_INLET_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_INLET_TEMP_ENTITY_ID))) ||
					(((EntityID == DCMI_CPU_TEMP_ENTITY_ID) || (EntityID == IPMI_CPU_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_CPU_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_CPU_TEMP_ENTITY_ID))) ||
					(((EntityID == DCMI_BASEBOARD_TEMP_ENTITY_ID) || (EntityID == IPMI_BASEBOARD_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_BASEBOARD_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_BASEBOARD_TEMP_ENTITY_ID))))
				{
					*pTotalValidInstances += 1;
					if (EntityInstance == pSenSharedMem->SensorInfo[i].EntiryInstance)
					{
						pBuf[0] = pSenSharedMem->SensorInfo[i].RecordID;
						ValidInstances = 1;
					}
				}
			}
		}
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&g_BMCInfo.SensorSharedMemMutex);
		return ValidInstances;
	}
	else
	{
		*pTotalValidInstances = 0;
		ValidInstances = 0;
		/* Acquire Shared memory to populate sensor information  */
		//OS_ACQUIRE_MUTEX(m_hSensorSharedMemMutex, SHARED_MEM_TIMEOUT);
		OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo.SensorSharedMemMutex, WAIT_INFINITE);

		for (i = StartingEntityInstance; i < MAX_SENSOR_NUMBERS + 1; i++)
		{
			if ((TRUE == pSenSharedMem->SensorInfo[i].IsDCMITempsensor) && (pSenSharedMem->SensorInfo[i].SDRRec->Type == FULL_SDR_REC))
			{
				if (SensorType != pSenSharedMem->SensorInfo[i].SensorTypeCode)
				{
					/*Check the Sensor type*/
					continue;
				}

				if ((((EntityID == DCMI_INLET_TEMP_ENTITY_ID) || (EntityID == IPMI_INLET_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_INLET_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_INLET_TEMP_ENTITY_ID))) ||
					(((EntityID == DCMI_CPU_TEMP_ENTITY_ID) || (EntityID == IPMI_CPU_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_CPU_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_CPU_TEMP_ENTITY_ID))) ||
					(((EntityID == DCMI_BASEBOARD_TEMP_ENTITY_ID) || (EntityID == IPMI_BASEBOARD_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_BASEBOARD_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_BASEBOARD_TEMP_ENTITY_ID))))
				{
					*pTotalValidInstances += 1;
					if (ValidInstances < 8)
					{
						pBuf[ValidInstances] = pSenSharedMem->SensorInfo[i].RecordID;
						ValidInstances += 1;
					}
				}
			}
		}
		/* Release mutex for Sensor shared memory */
		//OS_RELEASE_MUTEX(m_hSensorSharedMemMutex);
		OS_THREAD_MUTEX_RELEASE(&g_BMCInfo.SensorSharedMemMutex);
		return (ValidInstances > 8) ? 8 : ValidInstances;
	}
}

int GetDCMITempReading(INT8U EntityID, INT8U SensorType, INT8U EntityInstance,
					   INT8U StartingEntityInstance, INT8U *pBuf, INT8U *pTotalValidInstances, int BMCInst)
{
	int i, ValidInstances, j = 0;
	SensorSharedMem_T *pSenSharedMem;
	BMCInfo_t *pBMCInfo = &g_BMCInfo;
	SDRRecHdr_T *pSDRRec;
	FullSensorRec_T *FullSDR;
	float convreading = 0;
	INT8U MinReading = 0, MaxReading = 0, Linear = 0;

	pSenSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;

	if (0 != EntityInstance)
	{
		*pTotalValidInstances = 0;
		ValidInstances = 0;
		OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo.SensorSharedMemMutex, WAIT_INFINITE);

		for (i = 0; i < MAX_SENSOR_NUMBERS + 1; i++)
		{
			if ((TRUE == pSenSharedMem->SensorInfo[i].IsDCMITempsensor) && (pSenSharedMem->SensorInfo[i].SDRRec->Type == FULL_SDR_REC))
			{

				if (SensorType != pSenSharedMem->SensorInfo[i].SensorTypeCode)
				{
					/*Check the Sensor type*/
					continue;
				}

				if ((((EntityID == DCMI_INLET_TEMP_ENTITY_ID) || (EntityID == IPMI_INLET_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_INLET_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_INLET_TEMP_ENTITY_ID))) ||
					(((EntityID == DCMI_CPU_TEMP_ENTITY_ID) || (EntityID == IPMI_CPU_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_CPU_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_CPU_TEMP_ENTITY_ID))) ||
					(((EntityID == DCMI_BASEBOARD_TEMP_ENTITY_ID) || (EntityID == IPMI_BASEBOARD_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_BASEBOARD_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_BASEBOARD_TEMP_ENTITY_ID))))
				{
					*pTotalValidInstances += 1;
					if (EntityInstance == pSenSharedMem->SensorInfo[i].EntiryInstance)
					{
						if (!(pSenSharedMem->SensorInfo[i].EventFlags & 0x40) || (pSenSharedMem->SensorInfo[i].EventFlags & 0x20))
						{
							LOG_W("event flag is disabled\n");
							pBuf[DCMI_TEMP_READING] = 0;
							pBuf[DCMI_INST_NUMBER] = 0;
						}
						else
						{
							pSDRRec = GetSDRRec(pSenSharedMem->SensorInfo[i].SDRRec->ID, BMCInst);

							FullSDR = (FullSensorRec_T *)pSDRRec;
							MinReading = FullSDR->MinReading;
							MaxReading = FullSDR->MaxReading;
							Linear = FullSDR->Linearization;

							ipmi_conv_reading(pSenSharedMem->SensorInfo[i].SDRRec->Type, pSenSharedMem->SensorInfo[i].SensorReading, &convreading, MinReading, MaxReading, pSenSharedMem->SensorInfo[i].Units1, Linear, pSenSharedMem->SensorInfo[i].M_LSB,
											  pSenSharedMem->SensorInfo[i].B_LSB, pSenSharedMem->SensorInfo[i].M_MSB_Tolerance, pSenSharedMem->SensorInfo[i].B_MSB_Accuracy, pSenSharedMem->SensorInfo[i].RExp_BExp);

							pBuf[DCMI_TEMP_READING] = (INT8S)convreading;
							pBuf[DCMI_INST_NUMBER] = EntityInstance;
						}
						ValidInstances = 1;
					}
				}
			}
		}
		/* Release mutex for Sensor shared memory */
		OS_THREAD_MUTEX_RELEASE(&g_BMCInfo.SensorSharedMemMutex);
		return ValidInstances;
	}
	else
	{
		*pTotalValidInstances = 0;
		ValidInstances = 0;

		OS_THREAD_MUTEX_ACQUIRE(&g_BMCInfo.SensorSharedMemMutex, WAIT_INFINITE);

		for (i = StartingEntityInstance; i < MAX_SENSOR_NUMBERS + 1; i++)
		{
			if ((TRUE == pSenSharedMem->SensorInfo[i].IsDCMITempsensor) && (pSenSharedMem->SensorInfo[i].SDRRec->Type == FULL_SDR_REC))
			{

				if (SensorType != pSenSharedMem->SensorInfo[i].SensorTypeCode)
				{
					/*Check the Sensor type*/
					continue;
				}

				if ((((EntityID == DCMI_INLET_TEMP_ENTITY_ID) || (EntityID == IPMI_INLET_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_INLET_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_INLET_TEMP_ENTITY_ID))) ||
					(((EntityID == DCMI_CPU_TEMP_ENTITY_ID) || (EntityID == IPMI_CPU_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_CPU_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_CPU_TEMP_ENTITY_ID))) ||
					(((EntityID == DCMI_BASEBOARD_TEMP_ENTITY_ID) || (EntityID == IPMI_BASEBOARD_TEMP_ENTITY_ID)) && ((pSenSharedMem->SensorInfo[i].EntityID == DCMI_BASEBOARD_TEMP_ENTITY_ID) || (pSenSharedMem->SensorInfo[i].EntityID == IPMI_BASEBOARD_TEMP_ENTITY_ID))))
				{
					*pTotalValidInstances += 1;
					if (ValidInstances < 8)
					{
						if (!(pSenSharedMem->SensorInfo[i].EventFlags & 0x40) || (pSenSharedMem->SensorInfo[i].EventFlags & 0x20))
						{
							pBuf[DCMI_TEMP_READING + j] = 0;
							pBuf[DCMI_INST_NUMBER + j] = 0;
							j = j + 2;
						}
						else
						{
							pSDRRec = GetSDRRec(pSenSharedMem->SensorInfo[i].SDRRec->ID, BMCInst);
							FullSDR = (FullSensorRec_T *)pSDRRec;
							MinReading = FullSDR->MinReading;
							MaxReading = FullSDR->MaxReading;
							Linear = FullSDR->Linearization;

							ipmi_conv_reading(pSenSharedMem->SensorInfo[i].SDRRec->Type, pSenSharedMem->SensorInfo[i].SensorReading, &convreading, MinReading, MaxReading, pSenSharedMem->SensorInfo[i].Units1, Linear, pSenSharedMem->SensorInfo[i].M_LSB,
											  pSenSharedMem->SensorInfo[i].B_LSB, pSenSharedMem->SensorInfo[i].M_MSB_Tolerance, pSenSharedMem->SensorInfo[i].B_MSB_Accuracy, pSenSharedMem->SensorInfo[i].RExp_BExp);

							pBuf[j++] = (INT8S)convreading;
							pBuf[j++] = pSenSharedMem->SensorInfo[i].EntiryInstance;
							TDBG("j value %d\n", j);
						}
						ValidInstances += 1;
					}
				}
			}
		}

		OS_THREAD_MUTEX_RELEASE(&g_BMCInfo.SensorSharedMemMutex);
		return (ValidInstances > 8) ? 8 : ValidInstances;
	}
}
#endif /* SENSOR_DEVICE */

INT8U IpmiReadingDatConvert2Raw(INT8U sensor_type, INT16U value)
{
	INT8U res;

	switch (sensor_type)
	{
	case IPMI_UNIT_RPM:
		if(value > 8000)
		{
			value = 8000;
		}
		res = (INT8U)(value / 32);
		break;
	case IPMI_UNIT_VOLTS:
		res = value>>4;
		break;
	case IPMI_UNIT_AMPS:  
		res = value;
		break;
	case IPMI_UNIT_DEGREES_C:
		res = value;
		break;
	default:
		break;
	}

	return res;
}
