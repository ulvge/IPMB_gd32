/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2014, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
*
* IPMI_RAIDInfo.h
* RAID Component Info commands
*
* Author: Muthuchamy K <muthusamyk@amiindia.co.in>
*
******************************************************************/

#ifndef __IPMIRAIDINFO_H__
#define __IPMIRAIDINFO_H__

//#include "coreTypes.h"
#include "Types.h"

#define GET_RAID_CONTROLLER_COUNT       0x01
#define GET_RAID_CONTROLLER_INFO        0x02
#define GET_CONTROLLER_PHY_DEV_COUNT    0x03
#define GET_CONTROLLER_PHY_DEV_INFO     0x04
#define GET_CONTROLLER_LOG_DEV_COUNT    0x05
#define GET_CONTROLLER_LOG_DEV_INFO     0x06
#define GET_CONTROLLER_BBU_INFO         0x07
#define GET_RAID_EVENT_REPO_INFO        0x08
#define GET_RAID_EVENT_RECORD           0x09
#define GET_RAID_ALL_EVENT_RECORD       0x0a
#define CLEAR_RAID_EVENT_LOG            0x0b


#define MAX_ADAP_NAME_LEN       80
#define MAX_SERIAL_NUM_LEN      32
#define MAX_FW_VERSION_LEN      32
#define MAX_VENDOR_ID           8
#define MAX_PD_SERIAL_LEN       20
#define MAX_PD_PRODUCT_ID_LEN   16
#define MAX_LOG_DEV_NAME_LEN    16
#define MAX_ELEMENT_NUM         256
#define MAX_BBU_SERIAL_LEN      32
#define MAX_EVENT_DESC          128
#define MAX_RAID_ENTRIES        0x5000
#define FULL_RAID_EVENT         0xFF
#define PARTIAL_RAID_EVENT      0x00


#define AVAGO_VENDOR_ID       4096

typedef struct
{
    INT32U CtrlID;
    INT16U DevID;
}PACKED Device_T;

typedef struct
{
    INT32U  RecID;
}PACKED GetEvtRec_T;

typedef union
{
    INT32U CtrlID;
    Device_T DevInfo;
    GetEvtRec_T GetEvt;
    INT32U  Noofentryretrieved;
}PACKED RAIDInfoReq_T;

typedef struct
{
    INT8U Param;
    INT8U SetSelector;
    INT8U BlockSelector;
    RAIDInfoReq_T ParamData;
}AMIGetRAIDConfigReq_T;

typedef struct
{
    INT32U  CtrlID;
    INT8U   AdapterName[MAX_ADAP_NAME_LEN];
    INT8U   SerialNum[MAX_SERIAL_NUM_LEN];
    INT8U   PkgVersion[MAX_FW_VERSION_LEN];
    INT8U   BIOSVersion[MAX_FW_VERSION_LEN];
    INT8U   UEFIVersion[MAX_FW_VERSION_LEN];
    INT8U   ExpanderVersion[MAX_FW_VERSION_LEN];
    INT8U   SEEPROMVersion[MAX_FW_VERSION_LEN];
    INT8U   CPLDVersion[MAX_FW_VERSION_LEN];
    INT16U  PCIVendorID;
    INT16U  PCIDeviceID;
    INT16U  PCISubVendorID;
    INT16U  PCISubSytemID;
    INT16U  TmmStatus;
    INT8U   ROCTemp;
    INT8U   ExpanderTemp;
    INT8U   CtrlHealth;
}PACKED RAIDInfo_T;

typedef struct
{
    INT32U  CtrlID;
    INT16U  DevID;
    INT8U   Type;
    INT8U   State;
    INT8U   VendorID[MAX_VENDOR_ID];
    INT8U   ProductID[MAX_PD_PRODUCT_ID_LEN];
    INT8U   SerialNum[MAX_PD_SERIAL_LEN];
    INT8U   Revision_Level[4];
    INT8U   Slot;
    INT8U   Present;
    INT8U   LEDStatus;
    INT8U   InterfaceType;
    INT8U   Cache;
    INT8U   Speed;
    uint64  size64;
    INT32U  BlockSize;
    INT8U   LinkSpeed;
    INT8U   PowerState;
    INT8U   Temperature;
    INT8U   Smart;
}PACKED PhyDevInfo_T;

typedef struct
{
    INT16U  ElementID;
    INT8U   Type;
}PACKED Element;

typedef struct
{
    INT32U  CtrlID;
    INT16U  DevID;
    INT8U   LDName[MAX_LOG_DEV_NAME_LEN];
    INT8U   Type;
    INT8U   State;
    INT8U   StripeSize;
    INT8U   AccessPolicy;
    INT8U   ReadPolicy;
    INT8U   WritePolicy;
    INT8U   CachePolicy;
    INT8U   BGI;
    INT8U   SSD_Caching;
    INT8U   Progress;
    INT8U   BadBlocks;
    uint64  Size;
    INT8U   ElementsNum;
    Element ElementList[MAX_ELEMENT_NUM];
}PACKED LogicalDevInfo_T;

typedef struct
{
    INT32U  CtrlID;
//    INT8U   SerialNum[MAX_BBU_SERIAL_LEN];
    INT8U   BBUType;
    INT16U  Status;
    INT16U  Temperature;
    INT16U  Voltage;
    INT16U  Current;
}PACKED BBUInfo_T;

typedef struct
{
    INT32U  CtrlID;
    INT8U   DevCount;
}PACKED DeviceCount_T;

typedef struct
{
    INT32U  RecID;
    INT32U  RecLength;
    INT32U  TimeStamp;
    INT8U   CompType;
    INT32U  CtrlID;
    INT32U  EvtCode;
    INT8U   EvtType;
    INT8U   EvtClass;
    INT8U   EvtDesc[MAX_EVENT_DESC];

}PACKED RAIDEvtRcd_T;

typedef struct
{
    INT16U  CompEvtRecSize; //Including CompBookHdr 
    INT32U  MaxEvtCount;
    INT32U  CurrentCount;
}PACKED RAIDRepos_T;

typedef struct
{
    INT32U  Noofentries;
    INT8U   Status;
}PACKED RAIDGetAllEntry_T;

typedef union
{
    RAIDInfo_T          CtrlInfo;
    PhyDevInfo_T        Phy;
    LogicalDevInfo_T    Logical;
    DeviceCount_T       Device;
    BBUInfo_T           BBUInfo;
    RAIDEvtRcd_T        RAIDEvt;
    RAIDRepos_T         RAIDReposInfo;
    RAIDGetAllEntry_T   RAIDGetAllEntry;
}RAIDInfoRes_T;

typedef struct
{
    INT8U CompletionCode;
    RAIDInfoRes_T ParamData;
}AMIGetRAIDConfigRes_T;

#endif

