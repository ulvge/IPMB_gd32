/*****************************************************************
 *****************************************************************
 *
 * SDR.c
 * SDR functions.
 *
 *  Author: 
 *
 ******************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "debug_print.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMI_SDR.h"
#include "SDR.h"
#include "SDRFunc.h"
#include "SEL.h"
#include "IPMIDefs.h"
#include "IPMDevice.h"
#include "IPMI_Events.h"
#include "Events.h"
#include "Sensor.h"
#include "NVRAccess.h"
#include "IPMI_IPM.h"
#include "Util.h"
#include "IPMI_SensorEvent.h"
#include "IPMBIfc.h"
#include "IPMIConf.h"
//#include "PDKAccess.h"
//#include "SharedMem.h"
#include "IPMI_Storage.h"
#include "sensor/api_sensor.h"
#include "libipmi.h"
#include "api_subdevices.h"
//#include "PDKCmdsAccess.h"
//#include "featuredef.h"
//#include "cmdselect.h"

/* Reserved bit macro definitions */
#define RESERVED_BITS_PARTIALADDSDR 0xF0 //(BIT7 | BIT6 | BIT5 | BIT4)

#if SDR_DEVICE == 1

#define SUPPORT_MULTI_CONTROLLER 1
/*** Local Definitions ***/

#define SDR_FIRST_FREE_BYTE(BMCInst) (((INT8U *)g_BMCInfo.SDRConfig.SDRRAM) + g_BMCInfo.SDRConfig.SDRRAM->Size)

#define MAX_RES_LEN 128
#define SDR_VERSION 0x51
#define CMD_RUN_INITIALIZATION_AGENT 0x2C
#define MANAGEMENT_DEV_LOC_REC 0x12

#define PARTIAL_ADD_SDR_SUPPORT 0x04
#define RESERVE_SDR_SUPPORT 0x02
#define GET_SDR_REPOSITORY_ALLOC_SUPPORT 0x01
#define NON_MODAL_SUPPORT 0x20
#define NO_SUPPORT 0x00
#define DELETE_SDR_SUPPORT 0x08

#define MAX_OEM_REC_LEN 64

#define SDR_ALLOC_UNIT_SIZE 64 //0x10
#define SDR_MAX_RECORD_SIZE 0x80
#define SDR_ERASE_COMPLETED 0x01
#define SDR_INIT_COMPLETED 0x01

#define ENABLE_SELECTED_EVENT_MSG 0x10
#define DISABLE_SELECTED_EVENT_MSG 0x20

#define OEM_SDR_NM_REC 0x0D

/**
 * @brief Size of SDR Record
**/
#define SDR_SIZE(SDRRecHdr) (sizeof(SDRRecHdr_T) + (SDRRecHdr)->Len)

/**
 * @brief Size the SDR Record occupies in the memory
**/
#define SDR_SIZE_IN_MEM(SDRRecHdr) \
    (((SDR_SIZE(SDRRecHdr) & 0x0f) == 0) ? SDR_SIZE(SDRRecHdr) : (SDR_SIZE(SDRRecHdr) & 0xf0) + 0x10)

#define IPMB_ONCE_READ_MAX_LEN 0x50
#define RECODER_MAX_NUM sizeof(g_sensor_sdr) / sizeof(FullSensorRec_T)

//Recoder ID:must++
FullSensorRec_T g_sensor_sdr[] =
{    
   {        /* SDR Record header SYS0_FAN */
        0x0001, //Recoder ID
        0x51,   //SDR Version
        0x01,   //Record Type
        64,     //Record Length = full sensor: 64
        /* Record Key Bytes*/
        0x20,                   /* Sensor Owner ID                  */
        0x0,                      /* Sensor Owner LUN                 */
        FAN_CHANNEL_1,               /* Sensor Number                    */
        /* Record Body Bytes*/
        0x1d,                   /* Entity ID                        */
        0x1,                 /* Entity Instance                  */
        0x7f,                 /* Sensor Initialization            */
        0x68,                 /* Sensor Capabilities              */
        0x04,                        /* Sensor Type                      */
        0x01,                        /* Event / Reading Type             */
        0x285,               /* Lower Threshold Reading Mask     */
        0x285,               /* Upper Threshold Reading Mask     */
        0x3f3f,               /* Settable/Readable Threshold Mask */
        (0x0   << 6) + (0x0 << 3) + (0x0 << 1) + 0x0,/* Sensor Units 1  */
        IPMI_UNIT_RPM,        /* Sensor Units 2 - Base Unit       */
        0,                   /* Sensor Units 3 - Modifier Unit   */
        0x0,                   /* Linearization                    */
        0x50 & 0xFF,         /* M                                */ 
        ((0x50 >> 2) & 0xC0) + (0x0 & 0x3F),   /* M, Tolerance        */
        0x0 & 0xFF,         /* B                                */
        ((0x0 >> 2) & 0xC0) + (0x0  & 0x3F),   /* B, Accuracy  */
        ((0x0 >> 2) & 0xF0) + (0x0 & 0x0C) + 0x0,    /* Accuracy, Accuracy exponent      */
        (0x0 << 4) + (0x0 & 0x0F),          /*R exponent, B exponent           */
        0x0,                      /* Analog Characteristics Flags     */
        0x97,          /* Nominal Reading                  */
        0xff,              /* Normal Maximum                   */
        0x0 ,             /* Normal Minimum                   */
        0xff ,                 /* Sensor Maximum Reading           */
        0x0 ,                 /* Sensor Minimum Reading           */
        0xff ,   /* Upper Non-Recoverable Threshold  */
        0xc8,           /* Upper Critical Threshold         */
        0x7d ,      /* Upper Non-Critical Threshold     */
        0x0 ,   /* Lower Non-Recoverable Threshold  */
        0x0,           /* Lower Critical Threshold         */
        0x0,       /* Lower Non-Critical Threshold     */
        0x0,     /* Positive - threshold Hysteresis value*/
        0x0,     /* Negative - threshold Hysteresis value*/
        0x0 ,                        /* Reserved                         */
        0x0,                         /* Reserved                         */
        0x0,                      /* OEM                              */
        0xc0 + sizeof "SYS0_FAN",   /* ID String Type / Length Code     */
        "SYS0_FAN" 
    },
   
    {        /* SDR Record header P1V8 VCC*/
        0x0002, //Recoder ID
        0x51,   //SDR Version
        0x01,   //Record Type
        64,     //Record Length = full sensor: 64

        /* Record Key Bytes */
        0x20, //Sensor Owner ID   
        0x00, //Sensor Owner LUN
        ADC_CHANNEL_P1V8, //Sensor number  

        /* Record Body Bytes */
        0x07,           //Entity ID
        0x03,           //Entity Instance
        0x7F,           //Sensor Initialization
        0x68,           //Sensor Capabilities
        0x02,           //Sensor Type
        0x01,           //Event/Read Type
        0x7A95,         //Lower Threshold Reading Mask
        0x7A95,         //Upper Threshold Reading Mask
        0x3F3F,         //Settable/Readable Threshold Mask
        0x20,           //0x00,                 //Sensor Units 1
        IPMI_UNIT_VOLTS,  //Sensor Units 2 -Base Unit
        0x00,           //Sensor Units 3 -Modifier Unit
        0x00,           //Linearization
        0x81,             //0x14,            //M
        0x00,           //M,Tolerance
        0x00 & 0xff,    //B
        0x3E & 0xFF,    //B,Accuracy
        0x34,           //Accuracy,Accuracy exponent
        (0x0C << 4) + (0x0 & 0x0F), //R exponent,B exponent 
        0x00,           //Analog Characteristics Flags
        0x00,           //Nominal Reading
        0x00,           //Normal Maximum
        0x00,           //Normal Minimum
        0xFF,           //Sensor Maximum Reading
        0x00,           //Sensor Minimum Reading
        0xa0,           //Upper Non-Recoverable Threshold
        0x99,           //Upper Critical Threshold
        0x93,           //Upper Non-Critical Threshold
        0x77,           //Lower Non-Recoverable Threshold
        0x7e,           //Lower Critical Threshold
        0x85,           //Lower Non-Critical Threshold
        0x00,           //Positive-threshold Hysteresis calue
        0x00,           //Negative-threshold Hysteresis calue
        0x00,           //Reserved
        0x00,           //Reserved
        0x00,           //OEM
        0xC0 + sizeof "P1V8 VCC",
        "P1V8 VCC"
    },
    
    {        /* SDR Record header X100_Tmp */
        0x0003, //Recoder ID
        0x51,   //SDR Version
        0x01,   //Record Type
        64,     //Record Length = full sensor: 64

        /* Record Key Bytes */
        0x20, //Sensor Owner ID   
        0x00, //Sensor Owner LUN
        ADC_CHANNEL_TEMP_X100, //Sensor number  

        /* Record Body Bytes */
        0x07,           //Entity ID
        0x03,           //Entity Instance
        0x7F,           //Sensor Initialization
        0x68,           //Sensor Capabilities
        0x02,           //Sensor Type
        0x01,           //Event/Read Type
        0x7A95,         //Lower Threshold Reading Mask
        0x7A95,         //Upper Threshold Reading Mask
        0x3F3F,         //Settable/Readable Threshold Mask
        0x20,           //0x00,                 //Sensor Units 1
        IPMI_UNIT_DEGREES_C,  //Sensor Units 2 -Base Unit
        0x00,           //Sensor Units 3 -Modifier Unit
        0x00,           //Linearization
        5,             //0x14,            //M
        0x01,           //M,Tolerance
        0x00 & 0xff,    //B
        0x3E & 0xFF,    //B,Accuracy
        0x34,           //Accuracy,Accuracy exponent
        (0x0F << 4) + 0, //R exponent,B exponent
        0x00,           //Analog Characteristics Flags
        0x00,           //Nominal Reading
        0x00,           //Normal Maximum
        0x00,           //Normal Minimum
        0xFF,           //Sensor Maximum Reading
        0x00,           //Sensor Minimum Reading
        0xC8,           //Upper Non-Recoverable Threshold
        0xAA,           //Upper Critical Threshold
        0x96,           //Upper Non-Critical Threshold
        0x00,           //Lower Non-Recoverable Threshold
        0x00,           //Lower Critical Threshold
        0x00,           //Lower Non-Critical Threshold
        0x00,           //Positive-threshold Hysteresis calue
        0x00,           //Negative-threshold Hysteresis calue
        0x00,           //Reserved
        0x00,           //Reserved
        0x00,           //OEM
        0xC0 + sizeof "X100_Tmp",
        "X100_Tmp"
    },
    
    {        /* SDR Record header P12V*/
        0x00004, //Recoder ID
        0x51,   //SDR Version
        0x01,   //Record Type
        64,     //Record Length = full sensor: 64

        /* Record Key Bytes */
        0x20, //Sensor Owner ID   
        0x00, //Sensor Owner LUN
        ADC_CHANNEL_P12V, //Sensor number  

        /* Record Body Bytes */
        0x07,           //Entity ID
        0x03,           //Entity Instance
        0x7F,           //Sensor Initialization
        0x68,           //Sensor Capabilities
        0x02,           //Sensor Type
        0x01,           //Event/Read Type
        0x7A95,         //Lower Threshold Reading Mask
        0x7A95,         //Upper Threshold Reading Mask
        0x3F3F,         //Settable/Readable Threshold Mask
        0x20,           //0x00,                 //Sensor Units 1
        IPMI_UNIT_VOLTS,  //Sensor Units 2 -Base Unit
        0x00,           //Sensor Units 3 -Modifier Unit
        0x00,           //Linearization
        0x8e,             //0x14,            //M
        0x00,           //M,Tolerance
        0x00 & 0xff,    //B
        0x3E & 0xFF,    //B,Accuracy
        0x34,           //Accuracy,Accuracy exponent   
        (0x0d << 4) + (0x0 & 0x0F), //R exponent,B exponent 
        0x00,           //Analog Characteristics Flags
        0x00,           //Nominal Reading
        0x00,           //Normal Maximum
        0x00,           //Normal Minimum
        0xFF,           //Sensor Maximum Reading
        0x00,           //Sensor Minimum Reading
        0x6e,           //Upper Non-Recoverable Threshold
        0x65,           //Upper Critical Threshold
        0x5d,           //Upper Non-Critical Threshold
        0x3b,           //Lower Non-Recoverable Threshold
        0x44,           //Lower Critical Threshold
        0x4c,           //Lower Non-Critical Threshold
        0x00,           //Positive-threshold Hysteresis calue
        0x00,           //Negative-threshold Hysteresis calue
        0x00,           //Reserved
        0x00,           //Reserved
        0x00,           //OEM
        0xC0 + sizeof "P12V",
        "P12V"
    },
    
    {        /* SDR Record header SUB_DEVICE_MODE_POWER X100_Tmp */
        0x0005, //Recoder ID
        0x51,   //SDR Version
        0x01,   //Record Type
        64,     //Record Length = full sensor: 64

        /* Record Key Bytes */
        0x20, //Sensor Owner ID   
        0x00, //Sensor Owner LUN
        ADC_CHANNEL_TEMP_X100, //Sensor number  

        /* Record Body Bytes */
        0x07,           //Entity ID
        0x03,           //Entity Instance
        0x7F,           //Sensor Initialization
        0x68,           //Sensor Capabilities
        0x02,           //Sensor Type
        0x01,           //Event/Read Type
        0x7A95,         //Lower Threshold Reading Mask
        0x7A95,         //Upper Threshold Reading Mask
        0x3F3F,         //Settable/Readable Threshold Mask
        0x20,           //0x00,                 //Sensor Units 1
        IPMI_UNIT_DEGREES_C,  //Sensor Units 2 -Base Unit
        0x00,           //Sensor Units 3 -Modifier Unit
        0x00,           //Linearization
        5,             //0x14,            //M
        0x01,           //M,Tolerance
        0x00 & 0xff,    //B
        0x3E & 0xFF,    //B,Accuracy
        0x34,           //Accuracy,Accuracy exponent
        (0x0F << 4) + 0, //R exponent,B exponent
        0x00,           //Analog Characteristics Flags
        0x00,           //Nominal Reading
        0x00,           //Normal Maximum
        0x00,           //Normal Minimum
        0xFF,           //Sensor Maximum Reading
        0x00,           //Sensor Minimum Reading
        0xC8,           //Upper Non-Recoverable Threshold
        0xAA,           //Upper Critical Threshold
        0x96,           //Upper Non-Critical Threshold
        0x00,           //Lower Non-Recoverable Threshold
        0x00,           //Lower Critical Threshold
        0x00,           //Lower Non-Critical Threshold
        0x00,           //Positive-threshold Hysteresis calue
        0x00,           //Negative-threshold Hysteresis calue
        0x00,           //Reserved
        0x00,           //Reserved
        SUB_DEVICE_MODE_POWER,           //OEM
        0xC0 + sizeof "X100_Tmp",
        "X100_Tmp"
    },
    
    {        /* SDR Record header SUB_DEVICE_MODE_POWER P12V*/
        0x00006, //Recoder ID
        0x51,   //SDR Version
        0x01,   //Record Type
        64,     //Record Length = full sensor: 64

        /* Record Key Bytes */
        0x20, //Sensor Owner ID   
        0x00, //Sensor Owner LUN
        ADC_CHANNEL_P12V, //Sensor number  

        /* Record Body Bytes */
        0x07,           //Entity ID
        0x03,           //Entity Instance
        0x7F,           //Sensor Initialization
        0x68,           //Sensor Capabilities
        0x02,           //Sensor Type
        0x01,           //Event/Read Type
        0x7A95,         //Lower Threshold Reading Mask
        0x7A95,         //Upper Threshold Reading Mask
        0x3F3F,         //Settable/Readable Threshold Mask
        0x20,           //0x00,                 //Sensor Units 1
        IPMI_UNIT_VOLTS,  //Sensor Units 2 -Base Unit
        0x00,           //Sensor Units 3 -Modifier Unit
        0x00,           //Linearization
        0x8e,             //0x14,            //M
        0x00,           //M,Tolerance
        0x00 & 0xff,    //B
        0x3E & 0xFF,    //B,Accuracy
        0x34,           //Accuracy,Accuracy exponent   
        (0x0d << 4) + (0x0 & 0x0F), //R exponent,B exponent 
        0x00,           //Analog Characteristics Flags
        0x00,           //Nominal Reading
        0x00,           //Normal Maximum
        0x00,           //Normal Minimum
        0xFF,           //Sensor Maximum Reading
        0x00,           //Sensor Minimum Reading
        0x6e,           //Upper Non-Recoverable Threshold
        0x65,           //Upper Critical Threshold
        0x5d,           //Upper Non-Critical Threshold
        0x3b,           //Lower Non-Recoverable Threshold
        0x44,           //Lower Critical Threshold
        0x4c,           //Lower Non-Critical Threshold
        0x00,           //Positive-threshold Hysteresis calue
        0x00,           //Negative-threshold Hysteresis calue
        0x00,           //Reserved
        0x00,           //Reserved
        SUB_DEVICE_MODE_POWER,           //OEM
        0xC0 + sizeof "P12V",
        "P12V"
    },
    
     #if 0
    {        /* SDR Record header */
        0x0007, //Recoder ID
        0x51,   //SDR Version
        0x01,   //Record Type
        64,     //Record Length = full sensor: 64

        /* Record Key Bytes */
        0x20, //Sensor Owner ID   
        0x00, //Sensor Owner LUN
        0x07, //Sensor number  

        /* Record Body Bytes */
        0x07,           //Entity ID
        0x03,           //Entity Instance
        0x7F,           //Sensor Initialization
        0x68,           //Sensor Capabilities
        0x02,           //Sensor Type
        0x01,           //Event/Read Type
        0x7A95,         //Lower Threshold Reading Mask
        0x7A95,         //Upper Threshold Reading Mask
        0x3F3F,         //Settable/Readable Threshold Mask
        0x20,           //0x00,                 //Sensor Units 1
        IPMI_UNIT_VOLTS,  //Sensor Units 2 -Base Unit
        0x00,           //Sensor Units 3 -Modifier Unit
        0x00,           //Linearization
        16,             //0x14,            //M
        0x00,           //M,Tolerance
        0x00 & 0xff,    //B
        0x3E & 0xFF,    //B,Accuracy
        0x34,           //Accuracy,Accuracy exponent
        (14 << 4) + 13, //R exponent,B exponent
        0x00,           //Analog Characteristics Flags
        0x00,           //Nominal Reading
        0x00,           //Normal Maximum
        0x00,           //Normal Minimum
        0xFF,           //Sensor Maximum Reading
        0x00,           //Sensor Minimum Reading
        0xA3,           //Upper Non-Recoverable Threshold
        0x95,           //Upper Critical Threshold
        0x87,           //Upper Non-Critical Threshold
        0x00,           //Lower Non-Recoverable Threshold
        0x00,           //Lower Critical Threshold
        0x00,           //Lower Non-Critical Threshold
        0x00,           //Positive-threshold Hysteresis calue
        0x00,           //Negative-threshold Hysteresis calue
        0x00,           //Reserved
        0x00,           //Reserved
        0x00,           //OEM
        0xC0 + sizeof "0.9V",
        "0.9V"
    },
    #endif
};

/*** Prototype Declarations ***/
static BOOL PreCheckSDRUpdateModeCmd(INT8U *pRes, int BMCInst);
static INT16U SDR_ReserveSDRRepository(int BMCInst);
static SDRRecHdr_T *GetLastSDRRec(int BMCInst);
static void UpdateRepositoryInfo(int BMCInst);
static SDRRecHdr_T *SDR_GetSDRRec(INT16U RecID, INT16U ReservationID, int BMCInst);

static void SDRInitAgent(int BMCInst);

static INT8U ValidateSDRSize(INT8U SDRType, INT8U Size);

const INT8U SDRSize[][2] = {
    //      { SDR Type,                 Maximum Length },
    {FULL_SDR_REC, 64},
    {COMPACT_SDR_REC, 48},
    {EVENT_ONLY_SDR_REC, 33},
    {ENTITY_ASSOCIATION_SDR_REC, 16},
    {DEV_REL_ENTITY_ASSOCIATION_SDR_REC, 32},
    {GENERIC_DEVICE_LOCATOR_SDR_REC, 32},
    {FRU_DEVICE_LOCATOR_SDR_REC, 32},
    {MGMT_CTRL_DEV_LOCATOR_SDR_REC, 32},
    {MGMT_CTRL_CONFIRMATION_SDR_REC, 32},
    {BMC_MSG_CHANNEL_INFO_REC, 16},
};

static INT16U SDR_AddSDRRec(SDRRecHdr_T *pSDRRecHdr, int BMCInst);

static INT16U SDR_PartialAddSDR(INT8U *SDRData, INT8U Offset, INT8U Size, INT8U IsLast, INT16U ReservationID, int BMCInst);

#define CLEAR_SDR_INITIATE_ERASE 0xAA
#define CLEAR_SDR_GET_STATUS 0x00
static int SDR_ClearSDRRepository(INT16U ReservationID, INT8U InitOrStatus, int BMCInst);

static INT16U SDR_DeleteSDR(INT16U ReservationID, INT16U RecID, int BMCInst);

INT8U GetSDRRepositoryNum(void)
{
    return RECODER_MAX_NUM;
}
/*---------------------------------------
 * GetSDRRepositoryInfo
 *---------------------------------------*/
int GetSDRRepositoryInfo(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    SDRRepositoryInfo_T *pSDRRepInfoRes =
        (SDRRepositoryInfo_T *)pRes;

    //    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    // _fmemcpy (pSDRRepInfoRes, &g_BMCInfo.SDRConfig.RepositoryInfo, sizeof (SDRRepositoryInfo_T));
    // pSDRRepInfoRes->CompletionCode = CC_NORMAL;
    //    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

    pSDRRepInfoRes->CompletionCode = CC_NORMAL; // completion code

    pSDRRepInfoRes->Version = 0x51;
    pSDRRepInfoRes->RecCt = 10;//GetSDRRepositoryNum();
    pSDRRepInfoRes->FreeSpace = 0xF025;
    pSDRRepInfoRes->AddTimeStamp = 0x00;
    pSDRRepInfoRes->EraseTimeStamp = 0x00;
    pSDRRepInfoRes->OpSupport = 0xE0;

    // pSDRRepInfoRes->get_sdr_repository_allo_info_supported = 0x1;
    // pSDRRepInfoRes->reserve_sdr_repository_supported = 0x1;
    // pSDRRepInfoRes->partial_add_sdr_supported = 0x1;
    // pSDRRepInfoRes->delete_sdr_supported = 0x0;
    // pSDRRepInfoRes->modal_update_support = 0x1;
    // pSDRRepInfoRes->overflow_flag = 0x0;

    return sizeof(SDRRepositoryInfo_T);
}

/*---------------------------------------
 * GetSDRRepositoryAllocInfo
 *---------------------------------------*/
int GetSDRRepositoryAllocInfo(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    SDRRepositoryAllocInfo_T *pSDRRepAllocInfoRes =
        (SDRRepositoryAllocInfo_T *)pRes;

    //    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    _fmemcpy(pSDRRepAllocInfoRes, &g_BMCInfo.SDRConfig.RepositoryAllocInfo,
             sizeof(SDRRepositoryAllocInfo_T));
    pSDRRepAllocInfoRes->CompletionCode = CC_NORMAL;
    //    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

    return sizeof(SDRRepositoryAllocInfo_T);
}

/*---------------------------------------
 * ReserveSDRRepository
 *---------------------------------------*/
int ReserveSDRRepository(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    ReserveSDRRepositoryRes_T *pResSDRRepRes =
        (ReserveSDRRepositoryRes_T *)pRes;

    //    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    /* Shouldnt allow to reserve when the SDR is in update mode */
    //   if( TRUE == g_BMCInfo.SDRConfig.UpdatingSDR )
    //   {
    //           pResSDRRepRes->CompletionCode = CC_SDR_REP_IN_UPDATE_MODE;
    //           return sizeof (pResSDRRepRes->CompletionCode);
    //   }
    g_BMCInfo.SDRConfig.PartAddbytes = 0;

    // pResSDRRepRes->ReservationID  = SDR_ReserveSDRRepository (BMCInst);
    pResSDRRepRes->ReservationID = 0xFF;
    pResSDRRepRes->CompletionCode = CC_NORMAL;

    //   OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
    return sizeof(ReserveSDRRepositoryRes_T);
}

/*---------------------------------------
 * GetSDR
 *---------------------------------------*/
int GetSDR(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    SDRRecHdr_T *pSDRRec;
    GetSDRReq_T *pGetSDRReq = (GetSDRReq_T *)pReq;
    GetSDRRes_T *pGetSDRRes = (GetSDRRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    INT8U SDRLen = 0; //*curchannel;

    if (pGetSDRReq->RecID > RECODER_MAX_NUM)
    {                                                        // support max RECODER_MAX_NUM records
        pGetSDRRes->CompletionCode = CC_SDR_REC_NOT_PRESENT; // code
        return sizeof(INT8U);
    }
    if (pGetSDRReq->Size > IPMB_ONCE_READ_MAX_LEN)
    {                                                            // request read bytes exceeed supported
        pGetSDRRes->CompletionCode = CC_CANNOT_RETURN_REQ_BYTES; // code
        return sizeof(INT8U);
    }
    else if (pGetSDRReq->RecID == 0)
    { // when rq record id = 0, get first record header
        FullSensorRec_T *pFullSdr = &g_sensor_sdr[0];
		pGetSDRReq->RecID = g_sensor_sdr[0].hdr.ID;
        //  MgmtCtrlrDevLocator_T* dev_locator = (MgmtCtrlrDevLocator_T*)&device_locator;
        pGetSDRRes->CompletionCode = CC_NORMAL;
        g_sensor_sdr[0].hdr.Len = sizeof(FullSensorRec_T) - pGetSDRReq->Offset;
        pGetSDRRes->NextRecID = SDR_GetNextSDRId(pFullSdr, BMCInst);
        memcpy(&pRes[sizeof(GetSDRRes_T)], ((INT8U *)pFullSdr) + pGetSDRReq->Offset, pGetSDRReq->Size);
        return  sizeof(GetSDRRes_T) + pGetSDRReq->Size;
    }
    else
    { // according id to send diff sdr type dat, depend on slavle device support which one
        FullSensorRec_T *pCurrentSdr = ReadSensorRecByID(pGetSDRReq->RecID, BMCInst);
        if (pCurrentSdr == NULL){
            // support max RECODER_MAX_NUM records
            pGetSDRRes->CompletionCode = CC_SDR_REC_NOT_PRESENT; // code
            return sizeof(INT8U);
        }
        pCurrentSdr->hdr.Len = sizeof(FullSensorRec_T) - pGetSDRReq->Offset;

        pGetSDRRes->NextRecID = SDR_GetNextSDRId(pCurrentSdr, BMCInst);
        pGetSDRRes->CompletionCode = CC_NORMAL;
        memcpy(&pRes[sizeof(GetSDRRes_T)], ((INT8U *)pCurrentSdr)+ pGetSDRReq->Offset, pGetSDRReq->Size);
        return sizeof(GetSDRRes_T) + pGetSDRReq->Size;
    }
//    return 0;

//    //******************************************************************************

//    //    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
//    if (TRUE != PreCheckSDRUpdateModeCmd(pRes, BMCInst))
//    {
//        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
//        return sizeof(INT8U); /* error code set in func */
//    }

//    /* If the Offset is Not zero then its A partial get.. / if SDR is in update mode
//    	then check for the reservation id 		*/

//    if ((pGetSDRReq->Offset == 0) && (FALSE == g_BMCInfo.SDRConfig.UpdatingSDR))
//    {
//        pSDRRec = GetSDRRec(pGetSDRReq->RecID, BMCInst);
//    }
//    else
//    {
//        pSDRRec = SDR_GetSDRRec(pGetSDRReq->RecID, pGetSDRReq->ReservationID, BMCInst);
//    }

//    if (0 == pSDRRec)
//    {
//        //        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
//        pGetSDRRes->CompletionCode = g_BMCInfo.SDRConfig.SDRError;
//        return sizeof(INT8U);
//    }

//    SDRLen = pSDRRec->Len + sizeof(SDRRecHdr_T);

//    if ((0xFF == pGetSDRReq->Size) && (SDRLen >= pGetSDRReq->Offset))
//    {
//        pGetSDRReq->Size = SDRLen - pGetSDRReq->Offset;
//    }

//    //    OS_THREAD_TLS_GET(g_tls.CurChannel,curchannel);
//    /* Check for Max Request Bytes */
//    if ((pGetSDRReq->Size > SDRLen) || (pGetSDRReq->Offset > SDRLen) || (pGetSDRReq->Size > (SDRLen - pGetSDRReq->Offset)))
//    {
//        //       OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
//        pGetSDRRes->CompletionCode = CC_CANNOT_RETURN_REQ_BYTES;
//        return sizeof(INT8U);
//    }

//    /* Copy the response */
//    pGetSDRRes->CompletionCode = CC_NORMAL;
//    pGetSDRRes->NextRecID = SDR_GetNextSDRId(pSDRRec->ID, BMCInst);
//    _fmemcpy(pGetSDRRes + 1,
//             ((INT8U *)pSDRRec) + pGetSDRReq->Offset,
//             pGetSDRReq->Size);
//    //    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

//    return sizeof(GetSDRRes_T) + pGetSDRReq->Size;
}

/*---------------------------------------
 * AddSDR
 *---------------------------------------*/
int AddSDR(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    INT16U RecID;
    SDRRecHdr_T *pSDRRec = (SDRRecHdr_T *)pReq;
    AddSDRRes_T *pAddSDRRes = (AddSDRRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    /*Check the Minimum length of the SDR record*/
    if (ReqLen < sizeof(SDRRecHdr_T))
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    /*Check the SDR length*/
    if ((ReqLen - sizeof(SDRRecHdr_T)) != pSDRRec->Len)
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    if (TRUE != PreCheckSDRUpdateModeCmd(pRes, BMCInst))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        return sizeof(INT8U); /* error code set in func */
    }
    RecID = SDR_AddSDRRec(pSDRRec, BMCInst);
    if (INVALID_RECORD_ID == RecID)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        pAddSDRRes->CompletionCode = g_BMCInfo.SDRConfig.SDRError;
        return sizeof(pAddSDRRes->CompletionCode);
    }
    pAddSDRRes->CompletionCode = CC_NORMAL;
    pAddSDRRes->RecID = RecID;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

    // To send notification to CIM
    //    if(g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM] != NULL)
    //    {
    //    	uint8 CMD;
    //	// Set bits for SDR Event & Add operation
    //	CMD = 0x11;
    //	((int(*)(uint8, uint16))g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM])(CMD, RecID);
    //    }

    return sizeof(AddSDRRes_T);
}

/*---------------------------------------
 * PartialAddSDR
 *---------------------------------------*/
int PartialAddSDR(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    INT8U Size;
    INT16U RecID;
    PartialAddSDRReq_T *pPartialAddReq = (PartialAddSDRReq_T *)pReq;
    PartialAddSDRRes_T *pPartialAddRes = (PartialAddSDRRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    if (ReqLen < (sizeof(PartialAddSDRReq_T) + 1))
    {
        pPartialAddRes->CompletionCode = CC_REQ_INV_LEN;
        return sizeof(pPartialAddRes->CompletionCode);
    }

    /* Check for the reserved bytes should b zero */

    if (0 != (pPartialAddReq->Progress & RESERVED_BITS_PARTIALADDSDR))
    {
        pPartialAddRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    if (TRUE != PreCheckSDRUpdateModeCmd(pRes, BMCInst))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        return sizeof(INT8U); /* error code set in func */
    }

    Size = ReqLen - sizeof(PartialAddSDRReq_T);
    if (0 == pPartialAddReq->Offset)
    {
        g_BMCInfo.SDRConfig.TrackPOffset = 0;
        g_BMCInfo.SDRConfig.TrackRecID = pPartialAddReq->RecID;
    }
    else if ((g_BMCInfo.SDRConfig.TrackPOffset) != pPartialAddReq->Offset || g_BMCInfo.SDRConfig.TrackRecID != pPartialAddReq->RecID)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        pPartialAddRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(pPartialAddRes->CompletionCode);
    }

    RecID = SDR_PartialAddSDR((INT8U *)(pPartialAddReq + 1),
                              pPartialAddReq->Offset,
                              Size,
                              pPartialAddReq->Progress & 0xf,
                              pPartialAddReq->ReservationID, BMCInst);

    if (RecID == INVALID_RECORD_ID)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        pPartialAddRes->CompletionCode = g_BMCInfo.SDRConfig.SDRError;
        return sizeof(pPartialAddRes->CompletionCode);
    }

    pPartialAddRes->CompletionCode = CC_NORMAL;
    pPartialAddRes->RecID = RecID;

    g_BMCInfo.SDRConfig.TrackPOffset += Size;
    g_BMCInfo.SDRConfig.TrackRecID = RecID; //We are updating the available or alloted   Record ID
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
    return sizeof(PartialAddSDRRes_T);
}

/*---------------------------------------
 * DeleteSDR
 *---------------------------------------*/
int DeleteSDR(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    DeleteSDRReq_T *pDeleteSDRReq = (DeleteSDRReq_T *)pReq;
    DeleteSDRRes_T *pDeleteSDRRes = (DeleteSDRRes_T *)pRes;
    INT16U RecID;

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    if (TRUE != PreCheckSDRUpdateModeCmd(pRes, BMCInst))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        return sizeof(INT8U); /* error code set in func */
    }
    RecID = SDR_DeleteSDR(pDeleteSDRReq->ReservationID, pDeleteSDRReq->RecID, BMCInst);
    if (RecID == INVALID_RECORD_ID)
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        pDeleteSDRRes->CompletionCode = g_BMCInfo.SDRConfig.SDRError;
        return sizeof(pDeleteSDRRes->CompletionCode);
    }
    pDeleteSDRRes->CompletionCode = CC_NORMAL;
    pDeleteSDRRes->RecID = RecID;

    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
    // To send notification to CIM
    //    if(g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM] != NULL)
    //    {
    //	uint8 CMD;
    //	// Set bits for SDR Event & Delete operation
    //	CMD = 0x13;
    //	((int(*)(uint8, uint16))g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM])(CMD, RecID);
    //
    //    }

    return sizeof(DeleteSDRRes_T);
}

/*---------------------------------------
 * ClearSDRRepository
 *---------------------------------------*/
int ClearSDRRepository(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    int status;
    ClearSDRReq_T *pClrSDRReq = (ClearSDRReq_T *)pReq;
    ClearSDRRes_T *pClrSDRRes = (ClearSDRRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    //    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    if (TRUE != PreCheckSDRUpdateModeCmd(pRes, BMCInst))
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        return sizeof(INT8U); /* error code set in func */
    }

    if (pClrSDRReq->CLR[0] != 'C' || pClrSDRReq->CLR[1] != 'L' || pClrSDRReq->CLR[2] != 'R')
    {
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        pClrSDRRes->CompletionCode = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    status = SDR_ClearSDRRepository(pClrSDRReq->ReservationID, pClrSDRReq->InitOrStatus, BMCInst);
    if (0 != status)
    {
        //       OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
        pClrSDRRes->CompletionCode = g_BMCInfo.SDRConfig.SDRError;
        return sizeof(pClrSDRRes->CompletionCode);
    }

    pClrSDRRes->CompletionCode = CC_NORMAL;
    pClrSDRRes->EraseProgress = SDR_ERASE_COMPLETED;
    //   OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

    return sizeof(ClearSDRRes_T);
}

/*---------------------------------------
 * GetSDRRepositoryTime
 *---------------------------------------*/
int GetSDRRepositoryTime(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    INT32U SDRTime;
    GetSDRRepositoryTimeRes_T *pGetSDRTimeRes =
        (GetSDRRepositoryTimeRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    //    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    /* Get the time */
    //    SDRTime = GetSelTimeStamp (BMCInst);
    pGetSDRTimeRes->CompletionCode = CC_NORMAL;
    pGetSDRTimeRes->Time = SDRTime;
    //    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

    return sizeof(GetSDRRepositoryTimeRes_T);
}

/*--------------------------------------
 *  * SetSDRRepositoryTime
 *   * -------------------------------------*/
int SetSDRRepositoryTime(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{

    SetSDRRepositoryTimeReq_T *pSetSDRTimeReq = (SetSDRRepositoryTimeReq_T *)pReq;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    pSetSDRTimeReq->Time = ipmitoh_u32(pSetSDRTimeReq->Time);

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    SET_SYSTEM_TIME_STAMP(&pSetSDRTimeReq->Time);

    //    if( 0!= RestartDaemonByForce("crond","cron","/var/run/crond.reboot",SIGKILL))
    //    {
    //        pRes [0] = CC_UNSPECIFIED_ERR;
    //        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);
    //        return sizeof (*pRes);
    //    }

    /* Resetting the SELTimeUTCOffset to default value */
    //    pBMCInfo->GenConfig.SELTimeUTCOffset = UNSPECIFIED_UTC_OFFSET;

    /*Write to NVRAM*/
    //    FlushIPMI((INT8U*)&pBMCInfo->GenConfig,(INT8U*)&pBMCInfo->GenConfig,pBMCInfo->IPMIConfLoc.GenConfigAddr,
    //                      sizeof(GENConfig_T),BMCInst);
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

    pRes[0] = CC_NORMAL;
    return sizeof(*pRes);
}

/*---------------------------------------
 * EnterSDRUpdateMode
 *---------------------------------------*/
int EnterSDRUpdateMode(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    EnterSDRUpdateModeRes_T *pEnterSDRUpdateModeRes =
        (EnterSDRUpdateModeRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    INT8U curchannel;

    OS_THREAD_TLS_GET(g_tls.CurChannel, curchannel);
    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    g_BMCInfo.SDRConfig.UpdatingSDR = TRUE;
    g_BMCInfo.SDRConfig.UpdatingChannel = curchannel & 0xF;
    pEnterSDRUpdateModeRes->CompletionCode = CC_NORMAL;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

    return sizeof(EnterSDRUpdateModeRes_T);
}

/*------------------------------------------
 * ExitSDRUpdateMode
 *------------------------------------------*/
int ExitSDRUpdateMode(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    ExitSDRUpdateModeRes_T *pExitSDRUpdateModeRes =
        (ExitSDRUpdateModeRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    if (TRUE != PreCheckSDRUpdateModeCmd(pRes, BMCInst))
    {
        return sizeof(INT8U); /* error code set in func */
    }

    g_BMCInfo.SDRConfig.UpdatingSDR = FALSE;

    pExitSDRUpdateModeRes->CompletionCode = CC_NORMAL;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

    return sizeof(ExitSDRUpdateModeRes_T);
}

/*-------------------------------------------------
 * RunInitializationAgent
 *--------------------------------------------------*/
int RunInitializationAgent(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst)
{
    RunInitAgentRes_T *pRunInitAgentRes =
        (RunInitAgentRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    int i = 0, retval = 0;
    SensorInfo_T *pSensorInfo;
    INT16U SensorNum = 0;
    SensorSharedMem_T *pSMSharedMem = (SensorSharedMem_T *)&pBMCInfo->SensorSharedMem;

    /* Reserved bit Checking */
    if ((pReq[0] & 0xFE) != 0x00)
    {
        pRes[0] = CC_INV_DATA_FIELD;
        return sizeof(*pRes);
    }

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SDRConfig.SDRMutex, WAIT_INFINITE);
    if (0 != pReq[0])
    {
        pBMCInfo->SenConfig.NumThreshSensors = 0;
        pBMCInfo->SenConfig.NumNonThreshSensors = 0;
        memset(pSMSharedMem, 0, sizeof(SensorSharedMem_T));
        pBMCInfo->SenConfig.ValidSensorCnt = 0;
        memset(pBMCInfo->SenConfig.ValidSensorList, 0, MAX_SENSOR_NUMBERS);
        //       InitSensor(BMCInst);
        for (i = 0; i < MAX_SENSOR_NUMBERS; i++)
        {
            /* Check if sensor present */
            if (!pSMSharedMem->SensorInfo[i].IsSensorPresent)
            {
                continue;
            }

            //            if(g_PDKHandle[PDK_BEFOREVALIDSENSORLIST] != NULL)
            //            {
            //                retval = ((int (*)(void *, int, int)) g_PDKHandle[PDK_BEFOREVALIDSENSORLIST]) ((void *)&(pSMSharedMem->SensorInfo[i]), i, BMCInst);
            //                if (retval != 0)
            //                    continue;
            //            }

            pBMCInfo->SenConfig.ValidSensorList[pBMCInfo->SenConfig.ValidSensorCnt] = i;
            pBMCInfo->SenConfig.OwnerIDList[pBMCInfo->SenConfig.ValidSensorCnt] = pBMCInfo->IpmiConfig.BMCSlaveAddr;
            pBMCInfo->SenConfig.ValidSensorCnt++;
        }

        /* Acquire Shared memory   */
        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->m_hSMSharedMemMutex, SHARED_MEM_TIMEOUT);

        /* Update Sensor Properties & initialize sensors */
        for (i = 0; i < pBMCInfo->SenConfig.ValidSensorCnt; i++)
        {
            pSensorInfo = (SensorInfo_T *)&pSMSharedMem->SensorInfo[pBMCInfo->SenConfig.ValidSensorList[i]];
            SensorNum = pBMCInfo->SenConfig.ValidSensorList[i]; /* Multi-LUN support*/
            if (pBMCInfo->SenConfig.ValidSensorList[i] > 255)   /* Multi-LUN support */
            {
                SensorNum = pBMCInfo->SenConfig.ValidSensorList[i];
                pSensorInfo->SensorNumber = (SensorNum & 0xff);
                pSensorInfo->SensorOwnerLun = ((SensorNum >> 8) & VALID_LUN);
            }
            //            IPMI_DBG_PRINT_4("i = %x SensorNum = %x pSensorInfo->SensorOwnerLun = %x pSensorInfo->SensorNumber = %x\n", i, SensorNum, pSensorInfo->SensorOwnerLun, pSensorInfo->SensorNumber);

            //            LoadSensorProperties (pBMCInfo->SenConfig.ValidSensorList [i],  pBMCInfo->SenConfig.OwnerIDList[i], BMCInst);
            //            if(g_PDKHandle[PDK_LOADOEMSENSORDEFAULT] != NULL)
            //            {
            //                ((void(*)(SensorInfo_T*,int))g_PDKHandle[PDK_LOADOEMSENSORDEFAULT]) (pSensorInfo,BMCInst);
            //            }
            /* Enable internal Sensor if it has a valid SDR */
            //            ReInitSensor(pSensorInfo, pBMCInfo->SenConfig.OwnerIDList[i], BMCInst);
            //            if(g_corefeatures.internal_sensor == ENABLED)
            //            {
            //                InitInternalSensors (pSensorInfo,BMCInst);
            //                InitSensorHook (pSensorInfo,BMCInst);
            //            }
        }

        /* Release mutex for Sensor shared memory */
        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->m_hSMSharedMemMutex);
        //        pSMSharedMem->GlobalSensorScanningEnable = ENABLED;
        SDRInitAgent(BMCInst);
        // The sensor monitor loop has been made 0 for
        // initializing the sensor scanning bit.
        pBMCInfo->SenConfig.SensorMonitorLoopCount = 0;
    }
    pRunInitAgentRes->CompletionCode = CC_NORMAL;
    pRunInitAgentRes->Status = SDR_INIT_COMPLETED;
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SDRConfig.SDRMutex);

    return sizeof(RunInitAgentRes_T);
}

/*-----------------------------------------
 * GetSDRRec
 *-----------------------------------------*/
SDRRecHdr_T *
GetSDRRec(INT16U RecID, int BMCInst)
{
    SDRRecHdr_T *pSDRRec;

    /* If ID == 0x0000  return first record */
    if (0 == RecID)
    {
        pSDRRec = SDR_GetFirstSDRRec(BMCInst);
        if (0 == pSDRRec) /* is SDR empty? */
        {
            g_BMCInfo.SDRConfig.SDRError = CC_SDR_REC_NOT_PRESENT;
            return 0;
        }
        return pSDRRec;
    }

    /* If ID == 0xFFFF return the last record */
    if (0xFFFF == RecID)
    {
        return GetLastSDRRec(BMCInst);
    }

    pSDRRec = SDR_GetFirstSDRRec(BMCInst);
    while (TRUE)
    {
        if (0 == pSDRRec)
        {
            g_BMCInfo.SDRConfig.SDRError = CC_SDR_REC_NOT_PRESENT;
            return 0;
        }
        if (pSDRRec->ID == RecID)
        {
            return pSDRRec;
        }
        pSDRRec = SDR_GetNextSDRRec(pSDRRec, BMCInst);
    }
}

/*-----------------------------------------------------
 * InitSDR
 *-----------------------------------------------------*/
int InitSDR(int BMCInst)
{
    INT8U *pSDRRec;
    SDRRecHdr_T *pSDRRecord;
    int i; /* Multi-LUN support index will have more than 256 sensors*/

    INT8U OEM_Recdata[MAX_OEM_REC_LEN];
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    //    pBMCInfo->SDRConfig.SDRRAM = (SDRRepository_T*) GetSDRSELNVRAddr(NVRH_SDR, BMCInst);
    //    pSDRRec    = GetSDRSELNVRAddr (NVRH_SDR + sizeof(SDRRepository_T), BMCInst);
    pBMCInfo->SDRConfig.SDRRAM->NumRecords = 0;
    pBMCInfo->SDRConfig.LatestRecordID = 0;
    pBMCInfo->SDRConfig.SDRRAM->Size = 16; /* SDR Repository Header */
#if (0x01 == MARK_FOR_DELETION_SUPPORT)
    pBMCInfo->SDRConfig.NumMarkedRecords = 0;
#endif
    if (pBMCInfo->SDRConfig.RepositoryInfo.OpSupport & OVERFLOW_FLAG)
    {
        pBMCInfo->SDRConfig.RepositoryInfo.OpSupport &= ~OVERFLOW_FLAG;
    }

    //    IPMI_DBG_PRINT ("Init SDR\n");

    while (TRUE)
    {
#if (0x01 == MARK_FOR_DELETION_SUPPORT)
        if ((0x5A == ((E2ROMHdr_T *)pSDRRec)->Valid) || (0x01 == ((E2ROMHdr_T *)pSDRRec)->Valid))
        {
            //count the records marked for deletion.
            if (0x01 == ((E2ROMHdr_T *)pSDRRec)->Valid)
            {
                pBMCInfo->SDRConfig.NumMarkedRecords++;
            }
            else
            {
                pBMCInfo->SDRConfig.SDRRAM->NumRecords++;
            }
#else
        if (0x5A == ((E2ROMHdr_T *)pSDRRec)->Valid)
        {
            pBMCInfo->SDRConfig.SDRRAM->NumRecords++;
#endif

            pBMCInfo->SDRConfig.LatestRecordID = ((SDRRecHdr_T *)(((E2ROMHdr_T *)pSDRRec) + 1))->ID;
            pBMCInfo->SDRConfig.SDRRAM->Size += ((E2ROMHdr_T *)pSDRRec)->Len;
            pSDRRec += ((E2ROMHdr_T *)pSDRRec)->Len;
        }
        /* if No more Records */
        else
        {
            break;
        }
    }

    /* Check if SDR (valid record count) Empty */
    if (0 == pBMCInfo->SDRConfig.SDRRAM->NumRecords)
    {
#if IPM_DEVICE == 1
        pBMCInfo->Msghndlr.SelfTestByte |= GST_SDR_EMPTY;
#endif

        IPMI_DBG_PRINT("SDR EMPTY \n");
    }

    /* Update the SDR Erase Time Stamp */
    //   pBMCInfo->SDRConfig.RepositoryInfo.EraseTimeStamp = pBMCInfo->GenConfig.SDREraseTime ;
    //   pBMCInfo->SDRConfig.RepositoryInfo.AddTimeStamp  =	pBMCInfo->SDRConfig.SDRRAM->AddTimeStamp;

    /* Update the repository information */
    UpdateRepositoryInfo(BMCInst);

    pSDRRecord = SDR_GetFirstSDRRec(BMCInst);
    for (i = 0; i < pBMCInfo->SDRConfig.SDRRAM->NumRecords && pSDRRecord != NULL; i++)
    {
        if ((OEM_SDRFRU_REC == pSDRRecord->Type) || (OEM_SDRNM_REC == pSDRRecord->Type))
        {
            INT8U Rec_len;
            Rec_len = pSDRRecord->Len;
            memcpy(OEM_Recdata, pSDRRecord, sizeof(SDRRecHdr_T) + Rec_len);
            //         if(g_PDKHandle[PDK_PROCESSOEMRECORD] != NULL)
            //        {
            //            ((void(*)(INT8U *,int))g_PDKHandle[PDK_PROCESSOEMRECORD]) ((INT8U*)&OEM_Recdata,BMCInst);
            //        }
        }
        //      if(g_corefeatures.node_manager == ENABLED)
        //      {
        //        if(OEM_SDRNM_REC == pSDRRecord->Type)
        //        {
        //         OEM_NMRec_T* sonm;
        //         sonm =(OEM_NMRec_T*) ( (UINT8*)pSDRRecord );

        //         if(sonm->RecordSubType == OEM_SDR_NM_REC)
        //           pBMCInfo->NMConfig.NMDevSlaveAddress = sonm->NMDevSlaveAddress;
        //        }
        //      }
        //      if(g_PDKHandle[PDK_PROCESSSENSORDATARECORD] != NULL)
        //      {
        //          ((void(*)(INT8U *,int))g_PDKHandle[PDK_PROCESSSENSORDATARECORD]) ((INT8U*)pSDRRecord,BMCInst);
        //      }

        pSDRRecord = SDR_GetNextSDRRec(pSDRRecord, BMCInst);
    }
    //
    //    if(g_PDKHandle[PDK_AFTERSDRINIT] != NULL)
    //    {
    //        ((void(*)(INT8U))g_PDKHandle[PDK_AFTERSDRINIT]) (BMCInst);
    //    }
    //
    return 0;
}

/*----------------------------------------------------------*
 * SDR_GetNextSDRId
 *----------------------------------------------------------*/
INT16U SDR_GetNextSDRId(FullSensorRec_T *pCurrentSdr, int BMCInst)
{
    FullSensorRec_T *pSDRRecNext;
    for (INT16U i = 1; i < GetSDRRepositoryNum(); i++)
    {
        pSDRRecNext = pCurrentSdr + i;
        if (pSDRRecNext->OEMField == SubDevice_GetMyMode()){
            if ( ((UINT32)pSDRRecNext) >= ((UINT32)&g_sensor_sdr + sizeof(g_sensor_sdr)))
            {
                return 0xFFFF; // The last record , next id set to  0xFFFF
            }
            return pSDRRecNext->hdr.ID;
        }
    }
    return 0xFFFF; // The last record , next id set to  0xFFFF
}

/*--------------------------------------------------
 * SDR_GetFirstSDRRec
 *--------------------------------------------------*/
SDRRecHdr_T *
SDR_GetFirstSDRRec(int BMCInst)
{
    return ReadSDRRepository(NULL, BMCInst);
}

/*--------------------------------------------------
 * SDR_GetNextSDRRec
 *-------------------------------------------------*/
SDRRecHdr_T *
SDR_GetNextSDRRec(SDRRecHdr_T *pSDRRec, int BMCInst)
{
    return ReadSDRRepository(pSDRRec, BMCInst);
}

/*--------------------------------------------------
 * ReadSDRRepository
 *-------------------------------------------------*/
SDRRecHdr_T *
ReadSDRRepository(SDRRecHdr_T *pSDRRec, int BMCInst)
{
    E2ROMHdr_T *InternalHdr;
    INT8U SDRSize = 0;
    SDRRecHdr_T *pSDRRes = NULL;
		int i;

    if (pSDRRec == NULL)
    {
        return (SDRRecHdr_T *)&g_sensor_sdr[0];
    }
    else
    {
        for(i=0; i<sizeof(g_sensor_sdr)/sizeof(FullSensorRec_T); i++)
        {
            if(pSDRRec->ID == g_sensor_sdr[i].hdr.ID)
            {
                return  (SDRRecHdr_T *)&g_sensor_sdr[i];
            }
        }
        //        IPMI_DBG_PRINT_1("Reading the next record of %x \n", pSDRRec->ID);
    }

    return NULL;
}

/*--------------------------------------------------
 * ReadSensorRecByID
 *-------------------------------------------------*/
FullSensorRec_T *
ReadSensorRecByID(INT8U id, int BMCInst)
{
    SDRRecHdr_T SDRRec_T;
    SDRRec_T.ID = id;
    
    FullSensorRec_T *pSdr;
    pSdr = (FullSensorRec_T *)ReadSDRRepository(&SDRRec_T, BMCInst);
    return pSdr;
}
/*--------------------------------------------------
 * ReadSensorRecByID
 *-------------------------------------------------*/
FullSensorRec_T *
ReadSensorRecBySensorNum(INT8U destMode, INT8U sensorName, int BMCInst)
{
    FullSensorRec_T *pSdr;
    for(int i=0; i < GetSDRRepositoryNum(); i++)
    {
        if(destMode != g_sensor_sdr[i].OEMField) {
            continue;
        }
        if(sensorName == g_sensor_sdr[i].SensorNum)
        {
            return  &g_sensor_sdr[i];
        }
    }
    return pSdr;
}
/*--------------------------------------------------
 * WriteSDRRepository
 *-------------------------------------------------*/
void WriteSDRRepository(SDRRecHdr_T *pSDRRec, INT8U Offset, INT8U Size, INT8U SdrSize, int BMCInst)
{
    E2ROMHdr_T *InternalHdr = (E2ROMHdr_T *)SDR_FIRST_FREE_BYTE(BMCInst);
    INT8U *WriteAddr;

    /* Update Validity and Length */
    if (Offset < sizeof(SDRRecHdr_T) && ((Offset + Size) >= sizeof(SDRRecHdr_T)))
    {
        InternalHdr->Valid = 0x5A;
        InternalHdr->Len = SdrSize;
    }

    WriteAddr = (INT8U *)(InternalHdr + 1);
    memset((WriteAddr + Offset), 0, SdrSize);
    _fmemcpy((WriteAddr + Offset), pSDRRec, Size);

    return;
}

/**
 * @brief Cehck the SDR update mode.
 * @param pRes - Response.
 * @return TRUE/FALSE
**/
static BOOL
PreCheckSDRUpdateModeCmd(INT8U *pRes, int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    INT8U curchannel;

    OS_THREAD_TLS_GET(g_tls.CurChannel, curchannel);
    if (TRUE == g_BMCInfo.SDRConfig.UpdatingSDR)
    {
        if (pBMCInfo->IpmiConfig.SYSIfcSupport == 0x01 && SYS_IFC_CHANNEL == (curchannel & 0xF))
        {
            if (SYS_IFC_CHANNEL != g_BMCInfo.SDRConfig.UpdatingChannel)
            {
                *pRes = CC_NODE_BUSY;
                return FALSE;
            }
        }
        else
        {
            return TRUE;
        }
    }

    return TRUE;
}

/**
 * @brief Reserve the SDR Repository.
 * @return Reservation ID.
**/
static INT16U
SDR_ReserveSDRRepository(int BMCInst)
{
    if (0xffff == g_BMCInfo.SDRConfig.ReservationID)
    {
        g_BMCInfo.SDRConfig.ReservationID = 1;
    }
    else
    {
        g_BMCInfo.SDRConfig.ReservationID = rand();
    }
    g_BMCInfo.SDRConfig.TrackPOffset = 0;
    return g_BMCInfo.SDRConfig.ReservationID;
}

/**
 * @brief Get an SDR record.
 * @param RecID         - SDR record ID.
 * @param ReservationID - SDR Reservation ID.
 * @return SDR record.
**/
static SDRRecHdr_T *
SDR_GetSDRRec(INT16U RecID, INT16U ReservationID, int BMCInst)
{

    /* Bail out with error, if both the conditions are False (Reservation not valid)
    1. Reservation ID matches, not equal to 0x00 & SDR is not in Update mode  /
    2. SDR is in Update mode & Reservation ID equals to 0x00
    */
    // if ( !(((g_BMCInfo.SDRConfig.ReservationID == ReservationID) && (0x00 != ReservationID) && (FALSE == g_BMCInfo.SDRConfig.UpdatingSDR)) ||
    // ((TRUE == g_BMCInfo.SDRConfig.UpdatingSDR) && (0x00 == ReservationID))))
    // {
    //     if( TRUE == g_BMCInfo.SDRConfig.UpdatingSDR )
    //     {
    //         g_BMCInfo.SDRConfig.SDRError = CC_SDR_REP_IN_UPDATE_MODE;
    //     }else /* Reservation ID doesn't match, return invalid reservation ID */
    //     {
    //         g_BMCInfo.SDRConfig.SDRError = CC_INV_RESERVATION_ID;
    //     }
    //     return 0;
    // }

    return GetSDRRec(RecID, BMCInst);
}

static INT8U
ValidateSDRSize(INT8U SDRType, INT8U Size)
{
    int i;

    //    IPMI_DBG_PRINT_2("ValidateSDR Type : 0x%X, Size : 0x%X\n", SDRType, Size);

    if ((SDRType == OEM_SDRFRU_REC) || (SDRType == OEM_SDRNM_REC))
        return TRUE;

    for (i = 0; i < sizeof(SDRSize) / 2; i++)
    {
        if ((SDRSize[i][0] == SDRType) && (SDRSize[i][1] >= Size))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief Add an SDR record.
 * @param pSDRRec - SDR record.
 * @return Record ID.
**/
static INT16U
SDR_AddSDRRec(SDRRecHdr_T *pSDRRec, int BMCInst)
{
    INT16U SDRSize;
    INT8U AllocSize;
    int nRet = 0;

    if (FALSE == ValidateSDRSize(pSDRRec->Type, pSDRRec->Len + sizeof(SDRRecHdr_T)))
    {
        g_BMCInfo.SDRConfig.SDRError = CC_REQ_INV_LEN;
        return INVALID_RECORD_ID;
    }

    AllocSize = SDR_SIZE(pSDRRec) + sizeof(E2ROMHdr_T);
    if (0 != (AllocSize % SDR_ALLOC_UNIT_SIZE))
    {
        SDRSize = AllocSize + (SDR_ALLOC_UNIT_SIZE - (AllocSize % SDR_ALLOC_UNIT_SIZE));
    }
    else
    {
        SDRSize = AllocSize;
    }

    if (SDRSize > htoipmi_u16(g_BMCInfo.SDRConfig.RepositoryInfo.FreeSpace))
    {
        g_BMCInfo.SDRConfig.RepositoryInfo.OpSupport |= OVERFLOW_FLAG;
        g_BMCInfo.SDRConfig.SDRError = CC_OUT_OF_SPACE;
        return INVALID_RECORD_ID;
    }

    /* Update the record ID */
    pSDRRec->ID = g_BMCInfo.SDRConfig.LatestRecordID + 1;

    /* Add this record to the end of the repository */
    WriteSDRRepository(pSDRRec, 0, SDR_SIZE(pSDRRec), SDRSize, BMCInst);

    /* Flush the sdr */
//    FlushSDR (SDR_FIRST_FREE_BYTE(BMCInst), SDRSize, nRet,BMCInst);
#if IPM_DEVICE == 1
    if (-1 == nRet)
    {
        g_BMCInfo.Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SDR;
    }
    else
    {
        g_BMCInfo.Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SDR;
    }
#endif

    /* Update the fields in SDR Repository */
    g_BMCInfo.SDRConfig.SDRRAM->NumRecords++;
    g_BMCInfo.SDRConfig.LatestRecordID++;

#if IPM_DEVICE == 1
    if (g_BMCInfo.Msghndlr.SelfTestByte & GST_SDR_EMPTY)
    {
        g_BMCInfo.Msghndlr.SelfTestByte &= ~GST_SDR_EMPTY;
    }
#endif

    g_BMCInfo.SDRConfig.SDRRAM->Size += SDRSize;
    //    g_BMCInfo.SDRConfig.SDRRAM->AddTimeStamp        = GetSelTimeStamp(BMCInst);
    //    g_BMCInfo.SDRConfig.RepositoryInfo.AddTimeStamp = GetSelTimeStamp(BMCInst);

    /* Store the SDR Add Time Stamp in SDR.dat */
    //	FlushSDR (&g_BMCInfo.SDRConfig.SDRRAM->AddTimeStamp,sizeof(INT32U), nRet,BMCInst);

    /* Update the repository info */
    UpdateRepositoryInfo(BMCInst);

    /* Invalidate Reservation ID if any */
    g_BMCInfo.SDRConfig.ReservationID = 0;
    return pSDRRec->ID;
}

/**
 * @brief Add partial SDR record.
 * @param SDRData   - SDR Data.
 * @param Offset    - SDR offset.
 * @param Size      - Size of the data.
 * @param IsLast    - Is this a last record?
 * @param ReservationID - SDR reservation id.
 * @return the record id.
**/
static INT16U
SDR_PartialAddSDR(INT8U *SDRData, INT8U Offset, INT8U Size,
                  INT8U IsLast, INT16U ReservationID, int BMCInst)

{
    INT16U RecID;
    SDRRecHdr_T *pSDRRecInNVR;
    INT8U SdrSize = 0;
    int nRet;

    /* Bail out with error, if both the conditions are False (Reservation not valid)
    1. Reservation ID matches, not equal to 0x00 & SDR is not in Update mode  /
    2. SDR is in Update mode & Reservation ID equals to 0x00
    */
    if (!(((g_BMCInfo.SDRConfig.ReservationID == ReservationID) && (0x00 != ReservationID) && (FALSE == g_BMCInfo.SDRConfig.UpdatingSDR)) ||
          ((TRUE == g_BMCInfo.SDRConfig.UpdatingSDR) && (0x00 == ReservationID))))
    {
        if (TRUE == g_BMCInfo.SDRConfig.UpdatingSDR)
        {
            g_BMCInfo.SDRConfig.SDRError = CC_SDR_REP_IN_UPDATE_MODE;
        }
        else //Reservation ID doesn't match, return invalid reservation ID.
        {
            g_BMCInfo.SDRConfig.SDRError = CC_INV_RESERVATION_ID;
        }

        if (g_BMCInfo.SDRConfig.ReservationID == 0)
        {
            if (g_BMCInfo.SDRConfig.PartAddbytes != 0)
            {
                /*Reset the SDR Record value and SDR valid flag to invalid*/
                *(SDR_FIRST_FREE_BYTE(BMCInst) + 1) = 0xFF;
                memset(SDR_FIRST_FREE_BYTE(BMCInst) + sizeof(E2ROMHdr_T), 0, sizeof(SDRRecHdr_T));
                g_BMCInfo.SDRConfig.PartAddbytes = 0;
            }
        }
        return INVALID_RECORD_ID;
    }

    pSDRRecInNVR = (SDRRecHdr_T *)(SDR_FIRST_FREE_BYTE(BMCInst) +
                                         sizeof(E2ROMHdr_T));
    RecID = g_BMCInfo.SDRConfig.LatestRecordID + 1;

    /* If the header is received completely, validate the size */
    if ((Offset + Size) >= sizeof(SDRRecHdr_T))
    {
        if (Offset < sizeof(SDRRecHdr_T))
        {
            SDRRecHdr_T SDRHdr;

            memcpy((INT8U *)&SDRHdr, (INT8U *)pSDRRecInNVR, Offset);
            memcpy(((INT8U *)&SDRHdr) + Offset, SDRData, sizeof(SDRRecHdr_T) - Offset);

            ((SDRRecHdr_T *)pSDRRecInNVR)->ID = SDRHdr.ID = RecID;
            memcpy(SDRData, ((INT8U *)&SDRHdr) + Offset, sizeof(SDRRecHdr_T) - Offset);

            if (FALSE == ValidateSDRSize(SDRHdr.Type, SDRHdr.Len + sizeof(SDRRecHdr_T)))
            {
                g_BMCInfo.SDRConfig.SDRError = CC_REQ_INV_LEN;
                return INVALID_RECORD_ID;
            }
            SdrSize = SDRHdr.Len + sizeof(SDRRecHdr_T) + sizeof(E2ROMHdr_T);
            if (0 != (SdrSize % SDR_ALLOC_UNIT_SIZE))
            {
                SdrSize += (SDR_ALLOC_UNIT_SIZE - (SdrSize % SDR_ALLOC_UNIT_SIZE));
            }
            if (SdrSize > ipmitoh_u16(g_BMCInfo.SDRConfig.RepositoryInfo.FreeSpace))
            {
                g_BMCInfo.SDRConfig.RepositoryInfo.OpSupport |= OVERFLOW_FLAG;
                g_BMCInfo.SDRConfig.SDRError = CC_OUT_OF_SPACE;
                return INVALID_RECORD_ID;
            }
            g_BMCInfo.SDRConfig.PartAddbytes = SdrSize;

            if ((Offset + Size) > (SDRHdr.Len + sizeof(SDRRecHdr_T)))
            {
                g_BMCInfo.SDRConfig.SDRError = CC_INV_DATA_FIELD;
                return INVALID_RECORD_ID;
            }
        }
        else if ((Offset + Size) > (pSDRRecInNVR->Len + sizeof(SDRRecHdr_T)))
        {
            g_BMCInfo.SDRConfig.SDRError = CC_INV_DATA_FIELD;
            return INVALID_RECORD_ID;
        }
    }

    if ((IsLast == 1) && ((Offset + Size) != (pSDRRecInNVR->Len + sizeof(SDRRecHdr_T))))
    {
        g_BMCInfo.SDRConfig.SDRError = CC_INCOMPLETE_WRITTEN_BYTES;
        return INVALID_RECORD_ID;
    }

    WriteSDRRepository((SDRRecHdr_T *)SDRData, Offset, Size, SdrSize, BMCInst);

    /* If this is the last data update the data fields and flush. */
    if (IsLast == 1)
    {
        /* Flush the sdr */
//        FlushSDR (SDR_FIRST_FREE_BYTE(BMCInst), g_BMCInfo.SDRConfig.PartAddbytes, nRet,BMCInst);
#if IPM_DEVICE == 1
        if (-1 == nRet)
        {
            g_BMCInfo.Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SDR;
        }
        else
        {
            g_BMCInfo.Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SDR;
        }
#endif

        /* Update the fields in SDR Repository */
        g_BMCInfo.SDRConfig.SDRRAM->NumRecords++;
        g_BMCInfo.SDRConfig.LatestRecordID++;
        g_BMCInfo.SDRConfig.TrackPOffset = 0;

#if IPM_DEVICE == 1
        if (g_BMCInfo.Msghndlr.SelfTestByte & GST_SDR_EMPTY)
        {
            g_BMCInfo.Msghndlr.SelfTestByte &= ~GST_SDR_EMPTY;
        }
#endif

        g_BMCInfo.SDRConfig.SDRRAM->Size += g_BMCInfo.SDRConfig.PartAddbytes;
        //        g_BMCInfo.SDRConfig.SDRRAM->AddTimeStamp        = GetSelTimeStamp(BMCInst);
        //        g_BMCInfo.SDRConfig.RepositoryInfo.AddTimeStamp = GetSelTimeStamp(BMCInst);

        /* Update the repository info */
        UpdateRepositoryInfo(BMCInst);
        g_BMCInfo.SDRConfig.PartAddbytes = 0;

        /* Invalidate Reservation ID if any */
        g_BMCInfo.SDRConfig.ReservationID = 0;
        // To send notification to CIM
        //	if(g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM] != NULL)
        //        {
        //                uint8 CMD;
        //		// Set bits for SDR Event & Add operation
        //             CMD = 0x11;
        //             ((int(*)(uint8, uint16))g_PDKCIMEventHandle[PDKCIMEVENT_NOTIFYSERVERUPDATETOCIM])(CMD, RecID);
        //        }
    }

    return RecID;
}

/**
 * @brief Delete the SDR.
 * @param ReservationID - SDR reservation id.
 * @param RecordID        - SDR Record ID, which needs to be deleted
 * @return 0 if success, -1 if error.
**/
static INT16U
SDR_DeleteSDR(INT16U ReservationID, INT16U RecID, int BMCInst)
{

    //#define MARK_FOR_DELETION
    SDRRecHdr_T *pSDRRec = NULL;
    int nRet;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
#if (0x01 == MARK_FOR_DELETION_SUPPORT)
    E2ROMHdr_T *pRec = NULL;
#else
    INT8U *pDestLocation = NULL;
    INT8U *pSrcData = NULL;
    INT8U *pFreeBytes = NULL;
    INT8U deleteSDR_Size;
    INT16U size;
#endif

    /* Bail out with error, if both the conditions are False (Reservation not valid)
    1. Reservation ID matches, not equal to 0x00 & SDR is not in Update mode  /
    2. SDR is in Update mode & Reservation ID equals to 0x00
    */
    if (!(((g_BMCInfo.SDRConfig.ReservationID == ReservationID) && (0x00 != ReservationID) && (FALSE == g_BMCInfo.SDRConfig.UpdatingSDR)) ||
          ((TRUE == g_BMCInfo.SDRConfig.UpdatingSDR) && (0x00 == ReservationID))))
    {
        if (TRUE == g_BMCInfo.SDRConfig.UpdatingSDR)
        {
            g_BMCInfo.SDRConfig.SDRError = CC_SDR_REP_IN_UPDATE_MODE;
        }
        else /* Reservation ID doesn't match, return invalid reservation ID */
        {
            g_BMCInfo.SDRConfig.SDRError = CC_INV_RESERVATION_ID;
        }
        return INVALID_RECORD_ID;
    }

    while (TRUE)
    {
        pSDRRec = ReadSDRRepository(pSDRRec, BMCInst);
        if (pSDRRec == 0)
        {
            g_BMCInfo.SDRConfig.SDRError = CC_SDR_REC_NOT_PRESENT;
            return INVALID_RECORD_ID;
        }
        if (pSDRRec->ID == RecID)
        {
#if (0x01 == MARK_FOR_DELETION_SUPPORT)
            pRec = ((E2ROMHdr_T *)pSDRRec) - 1;
            pRec->Valid = 0x01; /* Mark this record for deletion */
            FlushSDR((INT8U *)pRec, sizeof(pRec->Valid), nRet, BMCInst);
#if IPM_DEVICE == 1
            if (-1 == nRet)
            {
                g_BMCInfo.Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SDR;
            }
            else
            {
                g_BMCInfo.Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SDR;
            }
#endif
            g_BMCInfo.SDRConfig.SDRRAM->NumRecords--;
            g_BMCInfo.SDRConfig.NumMarkedRecords++;
#else
            /* Delete SDR - delete the current SDR, and rearrange the subsequent SDR's */
            pDestLocation = (INT8U *)(((E2ROMHdr_T *)pSDRRec) - 1);
            deleteSDR_Size = (((E2ROMHdr_T *)pSDRRec) - 1)->Len;

            pSrcData = pDestLocation + deleteSDR_Size;

            pFreeBytes = (INT8U *)SDR_FIRST_FREE_BYTE(BMCInst);
            size = (INT8U *)pFreeBytes - (INT8U *)pDestLocation;

            _fmemcpy(pDestLocation, pSrcData, size - deleteSDR_Size);
            _fmemset((pFreeBytes - deleteSDR_Size), '\0', deleteSDR_Size);
            g_BMCInfo.SDRConfig.SDRRAM->Size -= deleteSDR_Size;
//            FlushSDR(pDestLocation, size, nRet,BMCInst);
#if IPM_DEVICE == 1
            if (-1 == nRet)
            {
                g_BMCInfo.Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SDR;
            }
            else
            {
                g_BMCInfo.Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SDR;
            }
#endif
            g_BMCInfo.SDRConfig.SDRRAM->NumRecords--;
#endif
            /* Update the SDR Erase Timestamp */
            //            g_BMCInfo.SDRConfig.SDRRAM->EraseTimeStamp        = GetSelTimeStamp (BMCInst);
            //            g_BMCInfo.SDRConfig.RepositoryInfo.EraseTimeStamp = GetSelTimeStamp(BMCInst);

            /* Store the SDR Ease Time Stamp in NVR */
            //            pBMCInfo->GenConfig.SDREraseTime = g_BMCInfo.SDRConfig.RepositoryInfo.EraseTimeStamp;
            //            FlushIPMI((INT8U*)&pBMCInfo->GenConfig,(INT8U*)&pBMCInfo->GenConfig,pBMCInfo->IPMIConfLoc.GenConfigAddr,
            //                              sizeof(GENConfig_T),BMCInst);

            /* Update the repository info */
            UpdateRepositoryInfo(BMCInst);

            /* Invalidate Reservation ID if any */
            g_BMCInfo.SDRConfig.ReservationID = 0;
            return RecID;
        }
    }
}

/**
 * @brief Clear the SDR repository.
 * @param ReservationID - SDR reservation id.
 * @param InitOrStatus - Initiate erase or get status flag
 * @return 0 if success, -1 if error.
**/
static int
SDR_ClearSDRRepository(INT16U ReservationID, INT8U InitOrStatus, int BMCInst)
{
    int nRet, i;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    /* Bail out with error, if both the conditions are False (Reservation not valid)
    1. Reservation ID matches, not equal to 0x00 & SDR is not in Update mode  /
    2. SDR is in Update mode & Reservation ID equals to 0x00
    */
    if (!(((g_BMCInfo.SDRConfig.ReservationID == ReservationID) && (0x00 != ReservationID) && (FALSE == g_BMCInfo.SDRConfig.UpdatingSDR)) ||
          ((TRUE == g_BMCInfo.SDRConfig.UpdatingSDR) && (0x00 == ReservationID))))
    {
        if (TRUE == g_BMCInfo.SDRConfig.UpdatingSDR)
        {
            g_BMCInfo.SDRConfig.SDRError = CC_SDR_REP_IN_UPDATE_MODE;
        }
        else //Reservation ID doesn't match, return invalid reservation ID.
        {
            g_BMCInfo.SDRConfig.SDRError = CC_INV_RESERVATION_ID;
        }
        return -1;
    }

    // AAh = initiate erase, 00h = get erasure status.
    if (InitOrStatus != CLEAR_SDR_INITIATE_ERASE &&
        InitOrStatus != CLEAR_SDR_GET_STATUS)
    {
        g_BMCInfo.SDRConfig.SDRError = CC_INV_DATA_FIELD;
        return -1;
    }
    else if (InitOrStatus == CLEAR_SDR_GET_STATUS)
    {
        return 0;
    }
    _fmemset(((INT8U *)g_BMCInfo.SDRConfig.SDRRAM + 16), 0xFF, (pBMCInfo->IpmiConfig.SDRAllocationSize * 1024) - 16);

    /* Flush the NVRAM */
//    FlushSDR (g_BMCInfo.SDRConfig.SDRRAM, (pBMCInfo->IpmiConfig.SDRAllocationSize * 1024), nRet,BMCInst);
#if IPM_DEVICE == 1
    if (-1 == nRet)
    {
        g_BMCInfo.Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_SDR;
    }
    else
    {
        g_BMCInfo.Msghndlr.SelfTestByte &= ~GST_CANNOT_ACCESS_SDR;
    }
#endif

    /* Clear the fields of the SDR */
    g_BMCInfo.SDRConfig.SDRRAM->NumRecords = 0;
    g_BMCInfo.SDRConfig.LatestRecordID = 0;
#if (0x01 == MARK_FOR_DELETION_SUPPORT)
    g_BMCInfo.SDRConfig.NumMarkedRecords = 0;
#endif
    if (g_BMCInfo.SDRConfig.RepositoryInfo.OpSupport & OVERFLOW_FLAG)
    {
        g_BMCInfo.SDRConfig.RepositoryInfo.OpSupport &= ~OVERFLOW_FLAG;
    }

#if IPM_DEVICE == 1
    g_BMCInfo.Msghndlr.SelfTestByte |= GST_SDR_EMPTY;
#endif

    g_BMCInfo.SDRConfig.SDRRAM->Size = 16;
    //    g_BMCInfo.SDRConfig.SDRRAM->EraseTimeStamp        = GetSelTimeStamp (BMCInst);
    //    g_BMCInfo.SDRConfig.RepositoryInfo.EraseTimeStamp = GetSelTimeStamp(BMCInst);

    /* Clear All FRU Config */
    for (i = 0; i < pBMCInfo->FRUConfig.total_frus; i++)
    {
        //	if(pBMCInfo->FRUConfig.FRUInfo[i] != 0xFF){
        //		free(pBMCInfo->FRUConfig.m_FRUInfo[i]);
        //		pBMCInfo->FRUConfig.FRUInfo[i] = 0xFF;
        //	}
    }
    pBMCInfo->FRUConfig.total_frus = 0;

    /* Store the SDR Ease Time Stamp in NVR */
    //    pBMCInfo->GenConfig.SDREraseTime = g_BMCInfo.SDRConfig.RepositoryInfo.EraseTimeStamp;
    //    FlushIPMI((INT8U*)&pBMCInfo->GenConfig,(INT8U*)&pBMCInfo->GenConfig,pBMCInfo->IPMIConfLoc.GenConfigAddr,
    //                      sizeof(GENConfig_T),BMCInst);

    /* Update the Repository Info */
    UpdateRepositoryInfo(BMCInst);

    /* Invalidate Reservation ID if any */
    g_BMCInfo.SDRConfig.ReservationID = 0;
    return 0;
}

/**
 * @brief Get the last SDR record.
 * @return the last SDR record.
**/
static SDRRecHdr_T *
GetLastSDRRec(int BMCInst)
{
    SDRRecHdr_T *pSDRRec;

    pSDRRec = SDR_GetFirstSDRRec(BMCInst);
    while (TRUE)
    {
        SDRRecHdr_T *pCurSDRRec;

        /* Save this record and get the next record */
        pCurSDRRec = pSDRRec;
        pSDRRec = SDR_GetNextSDRRec(pSDRRec, BMCInst);

        /* If we have reached past the last return the last record */
        if (0 == pSDRRec)
        {
            return pCurSDRRec;
        }
    }
}

/**
 * @brief Update SDR repository information.
**/
static void
UpdateRepositoryInfo(int BMCInst)
{
    BMCInfo_t *pBMCInfo = &g_BMCInfo;
    /* Update the version */
    g_BMCInfo.SDRConfig.RepositoryInfo.Version = SDR_VERSION;

    /* Update the record count */
    g_BMCInfo.SDRConfig.RepositoryInfo.RecCt = htoipmi_u16(g_BMCInfo.SDRConfig.SDRRAM->NumRecords);

    /* Update the free space available */
    g_BMCInfo.SDRConfig.RepositoryInfo.FreeSpace = htoipmi_u16(((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024) - g_BMCInfo.SDRConfig.SDRRAM->Size));

    /* Update the the operation support */
    g_BMCInfo.SDRConfig.RepositoryInfo.OpSupport = NO_SUPPORT;

    //    if(GetCommandEnabledStatus(NETFN_STORAGE,NULL,CMD_RESERVE_SDR_REPOSITORY,BMCInst) == 0)
    //        {
    //           g_BMCInfo.SDRConfig.RepositoryInfo.OpSupport|=RESERVE_SDR_SUPPORT;
    //        }
    //    if(GetCommandEnabledStatus(NETFN_STORAGE,NULL,CMD_GET_SDR_REPOSITORY_ALLOCATION_INFO,BMCInst) == 0)
    //        {
    //           g_BMCInfo.SDRConfig.RepositoryInfo.OpSupport|=NON_MODAL_SUPPORT+GET_SDR_REPOSITORY_ALLOC_SUPPORT;
    //        }
    //    if(GetCommandEnabledStatus(NETFN_STORAGE,NULL,CMD_PARTIAL_ADD_SDR,BMCInst) == 0)
    //        {
    //           g_BMCInfo.SDRConfig.RepositoryInfo.OpSupport|=PARTIAL_ADD_SDR_SUPPORT;
    //        }
    //    if(GetCommandEnabledStatus(NETFN_STORAGE,NULL,CMD_DELETE_SDR,BMCInst) == 0)
    //        {
    //           g_BMCInfo.SDRConfig.RepositoryInfo.OpSupport|=DELETE_SDR_SUPPORT;
    //     }

    /* Update the repository allocation information */
    g_BMCInfo.SDRConfig.RepositoryAllocInfo.NumAllocUnits = htoipmi_u16(((pBMCInfo->IpmiConfig.SDRAllocationSize * 1024) - 16) / SDR_ALLOC_UNIT_SIZE); /* Because 16 bytes are reserved for header info */
    g_BMCInfo.SDRConfig.RepositoryAllocInfo.AllocUnitSize = htoipmi_u16(SDR_ALLOC_UNIT_SIZE);
    g_BMCInfo.SDRConfig.RepositoryAllocInfo.NumFreeAllocUnits = htoipmi_u16(ipmitoh_u16(g_BMCInfo.SDRConfig.RepositoryInfo.FreeSpace) /
                                                                            SDR_ALLOC_UNIT_SIZE);
    g_BMCInfo.SDRConfig.RepositoryAllocInfo.LargestFreeBlock = htoipmi_u16(ipmitoh_u16(g_BMCInfo.SDRConfig.RepositoryInfo.FreeSpace) /
                                                                           SDR_ALLOC_UNIT_SIZE);
    g_BMCInfo.SDRConfig.RepositoryAllocInfo.MaxRecSize = SDR_MAX_RECORD_SIZE / SDR_ALLOC_UNIT_SIZE;
}

/**
 * @brief SDR initialization agent.
**/
static void
SDRInitAgent(int BMCInst)
{
    INT8U i, Count = 0;
    INT8U CmdRes[MAX_RES_LEN];
    SDRRecHdr_T *pSDRRec;
    HQueue_T IfcQ;
    BMCInfo_t *pBMCInfo = &g_BMCInfo;

    //    if(g_PDKHandle[PDK_BEFOREINITAGENT] != NULL)
    //    {
    //         if(-1 == ((int(*)(int))g_PDKHandle[PDK_BEFOREINITAGENT]) (BMCInst))
    //        {
    //            return;
    //        }
    //    }

#if SET_EVENT_RECEIVER == 1
    /* Turn off Event Generation */
    {
        //        SetEvtRcvReq_T SetEvtRcvReq = { 0xFF, 0 };
        //        SetEventReceiver ((INT8U*)&SetEvtRcvReq,
        //                          sizeof (SetEvtRcvReq_T), CmdRes,BMCInst);
    }
#endif
    /* Get the MMC Controller addresses*/
    pSDRRec = SDR_GetFirstSDRRec(BMCInst);

    //        if(0 != GetQueueHandle(LAN_IFC_Q,&IfcQ,BMCInst))
    //        {
    //            IPMI_WARNING("Error in getting LanIfcQ Handle \n");
    //        }

    for (i = 0; i < pBMCInfo->SDRConfig.SDRRAM->NumRecords && pSDRRec != NULL; i++)
    {
        if (MANAGEMENT_DEV_LOC_REC == pSDRRec->Type)
        {
            MgmtCtrlrDevLocator_T *pMgmtDevLocRec = (MgmtCtrlrDevLocator_T *)pSDRRec;
            if (pBMCInfo->IpmiConfig.BMCSlaveAddr != pMgmtDevLocRec->DevSlaveAddr)
            {
                pBMCInfo->MgmtTbl[Count].OWNERID = pMgmtDevLocRec->DevSlaveAddr;
                pBMCInfo->MgmtTbl[Count++].PowerNotification = pMgmtDevLocRec->PowerStateNotification;
            }
        }
        pSDRRec = SDR_GetNextSDRRec(pSDRRec, BMCInst);
    }
    pBMCInfo->MgmtTbl[Count].OWNERID = 0xff;

#if SUPPORT_MULTI_CONTROLLER == 1
    /* Sending the Set Event Reciver Disable Command to all MMC in IPMB BUS */
    if (pBMCInfo->IpmiConfig.PrimaryIPMBSupport == 0x01)
    {
        for (i = 0; i < Count; i++)
        {
            MsgPkt_T Req;
            IPMIMsgHdr_T *pIPMIMsgHdr = 0;
            SetEvtRcvReq_T *SetEvtRcvReq = 0;
            memset(&Req, 0, sizeof(MsgPkt_T));
            pIPMIMsgHdr = (IPMIMsgHdr_T *)Req.Data;
            SetEvtRcvReq = (SetEvtRcvReq_T *)&Req.Data[sizeof(IPMIMsgHdr_T)];
            pIPMIMsgHdr->ResAddr = pBMCInfo->MgmtTbl[i].OWNERID;
            pIPMIMsgHdr->NetFnLUN = (NETFN_SENSOR << 2) | BMC_LUN_00;
            pIPMIMsgHdr->ChkSum = ~(pIPMIMsgHdr->ResAddr + pIPMIMsgHdr->NetFnLUN) + 1;
            pIPMIMsgHdr->ReqAddr = pBMCInfo->IpmiConfig.BMCSlaveAddr;
            pBMCInfo->SDRConfig.IPMB_Seqnum++;
            if (pBMCInfo->SDRConfig.IPMB_Seqnum >= 0x3F)
            {
                pBMCInfo->SDRConfig.IPMB_Seqnum = 0xFF;
            }
            pIPMIMsgHdr->RqSeqLUN = pBMCInfo->SDRConfig.IPMB_Seqnum << 2; /* Request Seq Num */
            pIPMIMsgHdr->RqSeqLUN |= BMC_LUN_00;                          /* LUN =00 */
            pIPMIMsgHdr->Cmd = CMD_SET_EVENT_RECEIVER;
            SetEvtRcvReq->RcvSlaveAddr = 0xFF;
            SetEvtRcvReq->RcvLUN = BMC_LUN_00;
            Req.Param = BRIDGING_REQUEST;
            Req.Channel = PRIMARY_IPMB_CHANNEL;
            Req.Data[sizeof(IPMIMsgHdr_T) + sizeof(SetEvtRcvReq_T)] =
                CalculateCheckSum2(Req.Data, sizeof(IPMIMsgHdr_T) + sizeof(SetEvtRcvReq_T));
            Req.Size = sizeof(IPMIMsgHdr_T) + sizeof(SetEvtRcvReq_T) + 1; //Plus 1 for Check sum
            /* Post Msg to IPMB  task  to send the set event  receiver	*/
            if (-1 != IfcQ)
            {
                //                    if(0 != PostMsg(&Req, IPMB_PRIMARY_IFC_Q,BMCInst))
                //                    {
                //                        IPMI_WARNING ("SDR.c : Error posting message to IPMBIfc_Q\n");
                //                    }
                pBMCInfo->MgmtTbl[i].Status = TRUE;
            }
        }
    }

#endif

#if SUPPORT_MULTI_CONTROLLER == 1
    /* Sending the Run Initalization Command to all MMC in IPMB BUS */
    if (pBMCInfo->IpmiConfig.PrimaryIPMBSupport == 0x01)
    {
        for (i = 0; i < Count; i++)
        {
            MsgPkt_T Req;
            IPMIMsgHdr_T *pIPMIMsgHdr = 0;
            memset(&Req, 0, sizeof(MsgPkt_T));
            pIPMIMsgHdr = (IPMIMsgHdr_T *)Req.Data;
            pIPMIMsgHdr->ResAddr = pBMCInfo->MgmtTbl[i].OWNERID;
            pIPMIMsgHdr->NetFnLUN = (NETFN_STORAGE << 2) | BMC_LUN_00;
            /* Get Device ID */
            //		 pIPMIMsgHdr->NetFnLUN	 = (NETFN_APP<< 2) | BMC_LUN_00;
            pIPMIMsgHdr->ChkSum = ~(pIPMIMsgHdr->ResAddr + pIPMIMsgHdr->NetFnLUN) + 1;
            pIPMIMsgHdr->ReqAddr = pBMCInfo->IpmiConfig.BMCSlaveAddr;
            Req.Channel = PRIMARY_IPMB_CHANNEL;

            g_BMCInfo.SDRConfig.IPMB_Seqnum++;
            if (g_BMCInfo.SDRConfig.IPMB_Seqnum >= 0x3F)
            {
                g_BMCInfo.SDRConfig.IPMB_Seqnum = 0xFF;
            }
            pIPMIMsgHdr->RqSeqLUN = g_BMCInfo.SDRConfig.IPMB_Seqnum << 2; /* Request Seq Num */
            pIPMIMsgHdr->RqSeqLUN |= BMC_LUN_00;                          /* LUN =00 */
            Req.Param = BRIDGING_REQUEST;
            pIPMIMsgHdr->Cmd = CMD_RUN_INITIALIZATION_AGENT;
            Req.Data[sizeof(IPMIMsgHdr_T)] = 0x1;
            Req.Data[sizeof(IPMIMsgHdr_T) + 1] =
                CalculateCheckSum2(Req.Data, sizeof(IPMIMsgHdr_T) + 1);
            Req.Size = sizeof(IPMIMsgHdr_T) + 2; //Plus 2 for  req data's

            if (-1 != IfcQ)
            {
                //                    if(0 != PostMsg(&Req, IPMB_PRIMARY_IFC_Q,BMCInst))
                //                    {
                //                        IPMI_WARNING ("SDR.c : Error posting message to IPMBIfc_Q\n");
                //                    }
                pBMCInfo->MgmtTbl[i].Status = TRUE;
            }
        }
    }
#endif

    pSDRRec = SDR_GetFirstSDRRec(BMCInst);
    for (i = 0; i < pBMCInfo->SDRConfig.SDRRAM->NumRecords && pSDRRec != NULL; i++)
    {
        if (FULL_SDR_REC == pSDRRec->Type)
        {
            /* For all records of type 01h */
            FullSensorRec_T *pFullRec = (FullSensorRec_T *)pSDRRec;
            INT16U ValidMask = htoipmi_u16(0x0FFF);
            // If not a threshold sensor, then 15 event bits are used.
            if (pFullRec->EventTypeCode != 0x01)
            {
                ValidMask = htoipmi_u16(0x7FFF);
            }
            /* Disable Event & Scanning messages, if Disable is supported */
            if (0x03 != (pFullRec->SensorCaps & 0x03))
            {
                SetSensorEventEnableReq_T SetSEReq;
				UNUSED(SetSEReq);

                // disable all events for sensor
                SetSEReq.SensorNum = pFullRec->SensorNum;
                SetSEReq.Flags = DISABLE_SELECTED_EVENT_MSG; // Event Disabled
                SetSEReq.AssertionMask = ValidMask;
                SetSEReq.DeAssertionMask = ValidMask;

                //     SetSensorEventEnable ((INT8U*)&SetSEReq,
                //     sizeof (SetSensorEventEnableReq_T), CmdRes,BMCInst);
            }

            /* Set Sensor Type */
#if SET_SENSOR_TYPE != UNIMPLEMENTED
            if (pFullRec->SensorInit & BIT2)
            {
                /* Set Sensor Type */
                SetSensorTypeReq_T SetSensorTypeReq;
                SetSensorTypeReq.SensorNum = pFullRec->SensorNum;
                SetSensorTypeReq.SensorType = pFullRec->SensorType;
                SetSensorTypeReq.EventTypeCode = pFullRec->EventTypeCode;
                SetSensorType((INT8U *)&SetSensorTypeReq, sizeof(SetSensorTypeReq), CmdRes, BMCInst);
            }
#endif

            /* Set Sensor Thresholds */
            {
                SetSensorThresholdReq_T SetThresholdReq;

                SetThresholdReq.SensorNum = pFullRec->SensorNum;
                SetThresholdReq.SetFlags = ((pFullRec->DiscreteReadingMask >> 8) & 0x3F);
                if (pFullRec->SensorInit & BIT4)
                {
                    SetThresholdReq.LowerNonCritical = pFullRec->LowerNonCritical;
                    SetThresholdReq.LowerCritical = pFullRec->LowerCritical;
                    SetThresholdReq.LowerNonRecoverable = pFullRec->LowerNonRecoverable;
                    SetThresholdReq.UpperNonCritical = pFullRec->UpperNonCritical;
                    SetThresholdReq.UpperCritical = pFullRec->UpperCritical;
                    SetThresholdReq.UpperNonRecoverable = pFullRec->UpperNonRecoverable;

                    SetSensorThresholds((INT8U *)&SetThresholdReq,
                                        sizeof(SetSensorThresholdReq_T), CmdRes, BMCInst);
                }
            }

            /* Set Sensor Hysteresis */
            {
                SetSensorHysterisisReq_T SetHysteresisReq;

                SetHysteresisReq.SensorNum = pFullRec->SensorNum;
                SetHysteresisReq.Reserved = 0;
                if (pFullRec->SensorInit & BIT3) //Added for DR#31091
                {
                    SetHysteresisReq.PositiveHysterisis = pFullRec->PositiveHysterisis;
                    SetHysteresisReq.NegativeHysterisis = pFullRec->NegativeHysterisis;

                    SetSensorHysterisis((INT8U *)&SetHysteresisReq,
                                        sizeof(SetSensorHysterisisReq_T), CmdRes, BMCInst);
                }
            }

            /* Enable Event & Scanning messages, if Disabled */
            if (0x03 != (pFullRec->SensorCaps & 0x03))
            {
                SetSensorEventEnableReq_T SetSEReq;
				UNUSED(SetSEReq);

                SetSEReq.SensorNum = pFullRec->SensorNum;
                SetSEReq.Flags = (pFullRec->SensorInit & BIT6) | ((pFullRec->SensorInit & BIT5) << 2) | ENABLE_SELECTED_EVENT_MSG; // Event Enabled
                SetSEReq.AssertionMask = (pFullRec->AssertionEventMask & ValidMask);
                SetSEReq.DeAssertionMask = (pFullRec->DeAssertionEventMask & ValidMask);

                //                SetSensorEventEnable ((INT8U*)&SetSEReq,
                //                                                        sizeof (SetSensorEventEnableReq_T), CmdRes,BMCInst);
            }
        }

        if (pSDRRec->Type == COMPACT_SDR_REC)
        {
            CompactSensorRec_T *pCompactRec = (CompactSensorRec_T *)pSDRRec;
            INT16U ValidMask = htoipmi_u16(0x0FFF);
            // If not a threshold sensor, then 15 event bits are used.
            if (pCompactRec->EventTypeCode != 0x01)
            {
                ValidMask = htoipmi_u16(0x7FFF);
            }

            /* Disable Event & Scanning messages, if Disable is supported */
            if (0x03 != (pCompactRec->SensorCaps & 0x03))
            {
                SetSensorEventEnableReq_T SetSEReq;  
				UNUSED(SetSEReq);

                SetSEReq.SensorNum = pCompactRec->SensorNum;
                SetSEReq.Flags = DISABLE_SELECTED_EVENT_MSG; // Event Disabled
                SetSEReq.AssertionMask = ValidMask;
                SetSEReq.DeAssertionMask = ValidMask;

                //                SetSensorEventEnable ((INT8U*)&SetSEReq,
                //                sizeof (SetSensorEventEnableReq_T), CmdRes,BMCInst);
            }

            /* Set Sensor Type */
#if SET_SENSOR_TYPE != UNIMPLEMENTED
            if (pCompactRec->SensorInit & BIT2)
            {
                /* Set Sensor Type */
                SetSensorTypeReq_T SetSensorTypeReq;
                SetSensorTypeReq.SensorNum = pCompactRec->SensorNum;
                SetSensorTypeReq.SensorType = pCompactRec->SensorType;
                SetSensorTypeReq.EventTypeCode = pCompactRec->EventTypeCode;
                SetSensorType((INT8U *)&SetSensorTypeReq, sizeof(SetSensorTypeReq), CmdRes, BMCInst);
            }
#endif

            /* Set Sensor Hysteresis */
            {
                SetSensorHysterisisReq_T SetHysteresisReq;

                SetHysteresisReq.SensorNum = pCompactRec->SensorNum;
                SetHysteresisReq.Reserved = 0;
                SetHysteresisReq.PositiveHysterisis = pCompactRec->PositiveHysteris;
                SetHysteresisReq.NegativeHysterisis = pCompactRec->NegativeHysterisis;

                SetSensorHysterisis((INT8U *)&SetHysteresisReq,
                                    sizeof(SetSensorHysterisisReq_T), CmdRes, BMCInst);
            }

            /* Enable Event & Scanning messages, if Disabled */
            if (0x03 != (pCompactRec->SensorCaps & 0x03))
            {
                SetSensorEventEnableReq_T SetSEReq;
				UNUSED(SetSEReq);

                SetSEReq.SensorNum = pCompactRec->SensorNum;
                SetSEReq.Flags = (pCompactRec->SensorInit & BIT6) | ((pCompactRec->SensorInit & BIT5) << 2) | ENABLE_SELECTED_EVENT_MSG; // Event Enabled
                SetSEReq.AssertionMask = (pCompactRec->AssertionEventMask & ValidMask);
                SetSEReq.DeAssertionMask = (pCompactRec->DeAssertionEventMask & ValidMask);

                //                SetSensorEventEnable ((INT8U*)&SetSEReq,
                //                                                        sizeof (SetSensorEventEnableReq_T), CmdRes,BMCInst);
            }
        }
        pSDRRec = SDR_GetNextSDRRec(pSDRRec, BMCInst);
    }

#if SET_EVENT_RECEIVER == 0
    /* Enable Event Generation */
    {
        //        SetEvtRcvReq_T SetEvtRcvReq = { 0x20, 0 };
        //        SetEventReceiver ((INT8U*)&SetEvtRcvReq,
        //                                        sizeof (SetEvtRcvReq_T), CmdRes,BMCInst);
    }
#endif

#if SUPPORT_MULTI_CONTROLLER == 1

    /* Sending the Set Event Reciver enable Command to all MMC in IPMB BUS */
    for (i = 0; i < Count; i++)
    {
        MsgPkt_T Req;
        IPMIMsgHdr_T *pIPMIMsgHdr = (IPMIMsgHdr_T *)Req.Data;
        SetEvtRcvReq_T *SetEvtRcvReq = (SetEvtRcvReq_T *)&Req.Data[sizeof(IPMIMsgHdr_T)];
        pIPMIMsgHdr->ResAddr = pBMCInfo->MgmtTbl[i].OWNERID;
        pIPMIMsgHdr->NetFnLUN = (NETFN_SENSOR << 2) | BMC_LUN_00;
        pIPMIMsgHdr->ChkSum = ~(pIPMIMsgHdr->ResAddr + pIPMIMsgHdr->NetFnLUN) + 1;
        pIPMIMsgHdr->ReqAddr = pBMCInfo->IpmiConfig.BMCSlaveAddr;
        Req.Channel = PRIMARY_IPMB_CHANNEL;

        g_BMCInfo.SDRConfig.IPMB_Seqnum++;
        if (g_BMCInfo.SDRConfig.IPMB_Seqnum >= 0x3F)
        {
            g_BMCInfo.SDRConfig.IPMB_Seqnum = 0xFF;
        }
        pIPMIMsgHdr->RqSeqLUN = g_BMCInfo.SDRConfig.IPMB_Seqnum << 2; /* Request Seq Num */
        pIPMIMsgHdr->RqSeqLUN |= BMC_LUN_00;                          /* LUN =00 */
        pIPMIMsgHdr->Cmd = CMD_SET_EVENT_RECEIVER;
        SetEvtRcvReq->RcvSlaveAddr = pBMCInfo->IpmiConfig.BMCSlaveAddr;
        SetEvtRcvReq->RcvLUN = BMC_LUN_00;
        Req.Param = BRIDGING_REQUEST;
        Req.Data[sizeof(IPMIMsgHdr_T) + sizeof(SetEvtRcvReq_T)] =
            CalculateCheckSum2(Req.Data, sizeof(IPMIMsgHdr_T) + sizeof(SetEvtRcvReq_T));
        Req.Size = sizeof(IPMIMsgHdr_T) + sizeof(SetEvtRcvReq_T) + 1; // plus 1 for checksum

        /* Post Msg to IPMB  task  to send the set event  receiver  */
        if (-1 != IfcQ)
        {
            //            if(0 != PostMsg(&Req, IPMB_PRIMARY_IFC_Q,BMCInst))
            //            {
            //                IPMI_WARNING ("SDR.c : Error posting message to IPMBIfc_Q\n");
            //            }
            //            pBMCInfo->MgmtTbl[i].Status =TRUE;
        }
        //PDK_SMCDelay(pBMCInfo->MgmtTbl[i].OWNERID);
        //PDK_SMCDelay(0x40);
        //sleep(3);
    }

#endif

    //    if(g_PDKHandle[PDK_AFTERINITAGENT] != NULL)
    //    {
    //         ((int(*)(INT8U,int))g_PDKHandle[PDK_AFTERINITAGENT]) (CmdRes[0],BMCInst);
    //    }

    return;
}

#endif /* SDR_DEVICE */
