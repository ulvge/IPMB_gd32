/****************************************************************
 ****************************************************************
 *
 * IPMdevice.c
 * IPMDevice Commands Handler
 *
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include <string.h>
#include "Types.h"
#include "IPMDevice.h"
#include "MsgHndlr.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "IPMI_IPM.h"
#include "Util.h"
#include "WDT.h"
#include "IPMIConf.h"
#include "Sensor.h"
#include "bsp_gpio.h"


//common source for versions
//#include <version.h>

#if IPM_DEVICE == 1

/*** Local macro definitions ***/
#define ACPI_SET_SYS_PWR_STATE          0x80
#define ACPI_SET_DEV_PWR_STATE          0x80
#define ACPI_SYS_PWR_STATE_MASK         0x7F
#define ACPI_DEV_PWR_STATE_MASK         0x7F
#define ACPI_MAX_SYS_PWR_STATE			16
#define ACPI_MAX_DEV_PWR_STATE			7
#define ACPI_FLAG_SET					1
#define ACPI_FLAG_UNSET					0
#define ACPI_SET_SYS_NO_CHANGE          0x7F
#define ACPI_SET_DEV_NO_CHANGE          0x7F
/*** Global Variables ***/


//To get the data across the processes added in Shared memory structure in SharedMem.h
//INT8U     g_ACPISysPwrState;
//INT8U     g_ACPIDevPwrState;

/*** Module Variables ***/

#define WORKING_STATE      0x0
#define S1_SLEEPING_STATE  0x1

static const INT8U  MfgID[]       = {0x00,0x00,0x00};//     /**< Contains Manufacturer ID */
static const INT16U g_ProdID      = 0xaabb;


static void GetFirmwareVersion(unsigned int* Major,unsigned int* Minor,unsigned int* Rev)
{
    char aline[82];
    int AuxVer;
    int count = 0;
    
    if(count == 3)
        sscanf(aline,"FW_VERSION=%d.%d.%d.%d",Major,Minor,&AuxVer,Rev);
    else
        sscanf(aline,"FW_VERSION=%d.%d.%d",Major,Minor,Rev);    

    return;
}

int APPReserved (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    SendMsgRes_T* pSendMsgRes = (SendMsgRes_T*)pRes;
    pSendMsgRes->CompletionCode = CC_NORMAL;
    return  sizeof (*pRes);
}
/*---------------------------------------
 * GetDevID
 *---------------------------------------*/
int
GetDevID (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetDevIDRes_T*  pGetDevIDRes = (GetDevIDRes_T*) pRes;
    static unsigned int Major,Minor,Rev;
    static unsigned char MinorBCD;
    static int firsttime = 1;
 //   int inFlashMode = IsCardInFlashMode();
	int inFlashMode = 0;

    if(firsttime == 1)
    {
        //	printf("Calling get firmware version only for first time\n");
        GetFirmwareVersion(&Major,&Minor,&Rev);
        MinorBCD = ((Minor/10)<<4)+(Minor%10);
        /* Initialize/Read Mfg ID and Product ID */
//        if(g_PDKHandle[PDK_GETMFGPRODID] != NULL)
//        {
//            ((int(*)(INT8U *, INT16U *, INT8U))g_PDKHandle[PDK_GETMFGPRODID]) (MfgID, &g_ProdID, BMCInst);
//        }
        firsttime = 0;
    }

    pGetDevIDRes->CompletionCode      = CC_NORMAL;
    pGetDevIDRes->DeviceID            = GetDevAddr();
    pGetDevIDRes->DevRevision         = 0X81; // IPMI_DEV_REVISION;
    pGetDevIDRes->FirmwareRevision1 = Major | (inFlashMode << 7);
    pGetDevIDRes->FirmwareRevision2   = MinorBCD;  //DO NOT CHANGE THIS ASSIGNMENT. INSTEAD SET THE VALUE OF THE VARIABLE Minor IN THE FIRSTTIME LOOP
    pGetDevIDRes->IPMIVersion         = IPMI_VERSION;
    pGetDevIDRes->DevSupport          = 0xFF; // DEV_SUPPORT;
    pGetDevIDRes->ProdID              = g_ProdID;
    pGetDevIDRes->AuxFirmwareRevision = Rev;
    memcpy (pGetDevIDRes->MfgID, MfgID, sizeof (MfgID));

    return sizeof (GetDevIDRes_T);
}

extern xQueueHandle g_chassisCtrl_Queue;
/*---------------------------------------
 * ColdReset
 *---------------------------------------*/
int
ColdReset (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    g_BMCInfo.Msghndlr.ColdReset = 1;
    *pRes = CC_NORMAL;  
    BaseType_t err = pdFALSE;
				
    SamllMsgPkt_T Msg;
    Msg.Cmd  = CHASSIS_POWER_RESET;
    Msg.Size = 1;
    err = xQueueSend(g_chassisCtrl_Queue, (char *)&Msg, 10);
    if (err == pdPASS){
	    LOG_E("MCU RESET by ipmi creteTimer failed");
	}
    return sizeof (*pRes);
}


/*---------------------------------------
 * WarmReset
 *---------------------------------------*/
int
WarmReset (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{

	*pRes = CC_NORMAL;
	return sizeof (*pRes);
}


/*---------------------------------------
 * GetSelfTestResults
 *---------------------------------------*/
int
GetSelfTestResults (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetSelfTestRes_T*  pGetSelfTest = (GetSelfTestRes_T*) pRes;

    pGetSelfTest->CompletionCode  = CC_NORMAL;

    return sizeof (GetSelfTestRes_T);
}


/*---------------------------------------
 * MfgTestOn
 *---------------------------------------*/
int
MfgTestOn (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{

    g_BMCInfo.Msghndlr.ManufacturingTestOnMode = TRUE;

    *pRes = CC_NORMAL;
    return sizeof (*pRes);
}


/*---------------------------------------
 * SetACPIPwrState
 *---------------------------------------*/
int
SetACPIPwrState (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
	return -1;
}


/*---------------------------------------
 * GetACPIPwrState
 *---------------------------------------*/
int
GetACPIPwrState (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetACPIPwrStateRes_T*  pGetACPIRes = (GetACPIPwrStateRes_T*) pRes;

    pGetACPIRes->CompletionCode  = CC_NORMAL;

//    LOCK_BMC_SHARED_MEM(BMCInst);
//    pGetACPIRes->ACPISysPwrState = BMC_GET_SHARED_MEM (BMCInst)-> m_ACPISysPwrState;
 //   pGetACPIRes->ACPIDevPwrState =  BMC_GET_SHARED_MEM (BMCInst)->m_ACPIDevPwrState;
//    UNLOCK_BMC_SHARED_MEM (BMCInst);

    return sizeof (GetACPIPwrStateRes_T);
}


/*---------------------------------------
 * GetDevGUID
 *---------------------------------------*/
int
GetDevGUID (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    GetDevGUIDRes_T*  pGetDevGUIDRes = (GetDevGUIDRes_T*) pRes;

    pGetDevGUIDRes->CompletionCode  = CC_NORMAL;
//    LOCK_BMC_SHARED_MEM (BMCInst);
//    _fmemcpy (pGetDevGUIDRes->Node, BMC_GET_SHARED_MEM (BMCInst)->DeviceGUID, 16);
//    UNLOCK_BMC_SHARED_MEM (BMCInst);

    return sizeof (GetDevGUIDRes_T);
}


INT8U GetDevAddr()
{
    return g_BMCInfo.IpmiConfig.BMCSlaveAddr;
}

void SetDevAddr(INT8U device_addr)
{
    g_BMCInfo.IpmiConfig.BMCSlaveAddr = device_addr;
}

#endif  /* IPM_DEVICE */
