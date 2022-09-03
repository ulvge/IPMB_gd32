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
//_FAR_ INT8U     g_ACPISysPwrState;
//_FAR_ INT8U     g_ACPIDevPwrState;

/*** Module Variables ***/

#define WORKING_STATE      0x0
#define S1_SLEEPING_STATE  0x1

static const INT8U  MfgID[]       = {0x00,0x00,0x00};//     /**< Contains Manufacturer ID */
static const INT16U g_ProdID      = 0xaabb;


static void GetFirmwareVersion(unsigned int* Major,unsigned int* Minor,unsigned int* Rev)
{
    char aline[82];
    int AuxVer;
    int i = 0, count = 0;
    
    if(count == 3)
        sscanf(aline,"FW_VERSION=%d.%d.%d.%d",Major,Minor,&AuxVer,Rev);
    else
        sscanf(aline,"FW_VERSION=%d.%d.%d",Major,Minor,Rev);    

    return;
}

/*---------------------------------------
 * GetDevID
 *---------------------------------------*/
int
GetDevID (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetDevIDRes_T*  pGetDevIDRes = (_NEAR_ GetDevIDRes_T*) pRes;
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


/*---------------------------------------
 * ColdReset
 *---------------------------------------*/
int
ColdReset (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    g_BMCInfo.Msghndlr.ColdReset = 1;
    /* PDK Module Post Set Reboot Cause*/
//    if(g_PDKHandle[PDK_SETREBOOTCAUSE] != NULL)
//    {
//        ((INT8U(*)(INT8U,int)) g_PDKHandle[PDK_SETREBOOTCAUSE])(SETREBOOTCAUSE_COLD_WARM_RESET_CMD,BMCInst);
//    }
		LOG_E("MCU RESET");
	  NVIC_SystemReset();
    *pRes = CC_NORMAL;
    return sizeof (*pRes);
}


/*---------------------------------------
 * WarmReset
 *---------------------------------------*/
int
WarmReset (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

	*pRes = CC_NORMAL;
	return sizeof (*pRes);
}


/*---------------------------------------
 * GetSelfTestResults
 *---------------------------------------*/
int
GetSelfTestResults (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetSelfTestRes_T*  pGetSelfTest = (_NEAR_ GetSelfTestRes_T*) pRes;

    pGetSelfTest->CompletionCode  = CC_NORMAL;

    return sizeof (GetSelfTestRes_T);
}


/*---------------------------------------
 * MfgTestOn
 *---------------------------------------*/
int
MfgTestOn (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{

    g_BMCInfo.Msghndlr.ManufacturingTestOnMode = TRUE;

    *pRes = CC_NORMAL;
    return sizeof (*pRes);
}


/*---------------------------------------
 * SetACPIPwrState
 *---------------------------------------*/
int
SetACPIPwrState (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
	return -1;
}


/*---------------------------------------
 * GetACPIPwrState
 *---------------------------------------*/
int
GetACPIPwrState (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetACPIPwrStateRes_T*  pGetACPIRes = (_NEAR_ GetACPIPwrStateRes_T*) pRes;

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
GetDevGUID (_NEAR_ INT8U* pReq, INT8U ReqLen, _NEAR_ INT8U* pRes,_NEAR_ int BMCInst)
{
    _NEAR_ GetDevGUIDRes_T*  pGetDevGUIDRes = (_NEAR_ GetDevGUIDRes_T*) pRes;

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
