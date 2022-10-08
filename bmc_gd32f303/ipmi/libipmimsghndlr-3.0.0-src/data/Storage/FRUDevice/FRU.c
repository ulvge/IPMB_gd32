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
 * fru.c
 * fru functions.
 *
 *  Author: Rama Bisa <ramab@ami.com>
 *
 ******************************************************************/
#define ENABLE_DEBUG_MACROS 0
#include "Types.h"
#include "FRU.h"
#include "IPMI_FRU.h"
#include "MsgHndlr.h"
#include "SharedMem.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "Debug.h"
#include "NVRData.h"
#include "IPMDevice.h"
#include "NVRAccess.h"
#include "IPMI_IPM.h"
#include "IPMI_SDRRecord.h"
#include "SDRFunc.h"
#include "Util.h"
#include "PDKAccess.h"
#include "IPMIConf.h"



#if FRU_DEVICE == 1

//extern INT8U     g_SelfTestByte;
//extern   INT8U g_FRUInfo[MAX_PDK_FRU_SUPPORTED];

/*-----------------------------------------------------
 * GetFRUAreaInfo
 *----------------------------------------------------*/
int
GetFRUAreaInfo (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    int Ret=0;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->FRUConfig.FRUMutex,WAIT_INFINITE);
    if(g_PDKHandle[PDK_GETFRUAREAINFO] != NULL)
    {
        Ret = ((int(*)(INT8U*,INT8U,INT8U*,int))g_PDKHandle[PDK_GETFRUAREAINFO]) (pReq, ReqLen, pRes,BMCInst);
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->FRUConfig.FRUMutex);

    return Ret;

}


/*-----------------------------------------------------
 * Read FRU Data
 *----------------------------------------------------*/
int
ReadFRUData (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    int Ret=0;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->FRUConfig.FRUMutex,WAIT_INFINITE);
    if(g_PDKHandle[PDK_READFRUDATA] != NULL)
    {
        Ret = ((int(*)(INT8U*,INT8U,INT8U*,int))g_PDKHandle[PDK_READFRUDATA]) (pReq, ReqLen, pRes,BMCInst);
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->FRUConfig.FRUMutex);

    return Ret;

}


/*-----------------------------------------------------
 * Write FRU Data
 *----------------------------------------------------*/
int
WriteFRUData (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst)
{
    int Ret=0;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->FRUConfig.FRUMutex,WAIT_INFINITE);
    if(g_PDKHandle[PDK_WRITEFRUDATA] != NULL)
    {
        Ret = ((int(*)(INT8U*,INT8U,INT8U*,int))g_PDKHandle[PDK_WRITEFRUDATA]) (pReq, ReqLen, pRes,BMCInst);
    }
    OS_THREAD_MUTEX_RELEASE(&pBMCInfo->FRUConfig.FRUMutex);

    return Ret;

}

/*-----------------------------------------------------
 * InitFRU
 *-----------------------------------------------------*/
int
InitFRU (int BMCInst)
{
    FRUReadReq_T   FRUReadReq;
    INT8U Frudata[64];
    INT8U Checksum=0;
    int ret=0,i,j;
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    //pBMCInfo->FRUConfig.total_frus = 1;

    for(i=0;pBMCInfo->FRUConfig.FRUInfo[i]!=0xff;i++)
    {
        FRUReadReq.FRUDeviceID=pBMCInfo->FRUConfig.FRUInfo[i];
        FRUReadReq.Offset=0x0;
        FRUReadReq.CountToRead=sizeof(FRUCommonHeader_T);
        ret=ReadFRUData(( INT8U *)&FRUReadReq,4,Frudata,BMCInst);
        switch(Frudata[0])
        {
            case FRU_DEVICE_NOT_FOUND :
                continue;

            case FRU_NOT_ACCESSIBLE:
                g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_CANNOT_ACCESS_FRU;
                break;

            case FRU_ACCESSIBLE:
                for(j=2;j<10;j++)
                {
                Checksum+=Frudata[j];
                }

                if(0 !=Checksum )
                {
                    g_BMCInfo[BMCInst].Msghndlr.SelfTestByte |= GST_FRU_CORRUPTED;
                    break;
                }
        }
    }

    if(g_PDKHandle[PDK_INITFRU] != NULL)
    {
         ret = ((int(*)(int))g_PDKHandle[PDK_INITFRU]) (BMCInst);
    }

    return ret;

}



#endif /* FRU_DEVICE */
