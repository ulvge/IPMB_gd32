/******************************************************************
 ******************************************************************
 ***                                                             **
 ***    (C)Copyright 2014, American Megatrends Inc.             **
 ***                                                             **
 ***    All Rights Reserved.                                     **
 ***                                                             **
 ***    5555 , Oakbrook Pkwy, Norcross,                          **
 ***                                                             **
 ***    Georgia - 30093, USA. Phone-(770)-246-8600.              **
 ***                                                             **
 ******************************************************************
 ******************************************************************
 ******************************************************************
 *
 * UnifiedFwUpdate.h
 * Unified Firmware Update related initialization
 *
 *  Author: Muthuchamy Kumar <muthusamyk@amiindia.co.in>
 ******************************************************************/
#ifndef _UNIFIEDFWUPDATE_H_
#define _UNIFIEDFWUPDATE_H_

#include <stdio.h>
#include <Types.h>

#define GET_PLUGIN_META_INFO_SYMBOL "GetPluginMetaInfo"
#define PLUGIN_LIB_PATH "/usr/local/lib/fwplugin"

#define MAX_DEV_NAME    20
#define MAX_PLUGIN_COUNT    20
#define MAX_SYMBOL_LEN      64
#define DEV_IDENTIFIER_LEN  20
#define UPDATE_MODE_INITIALIZED     1
#define ENTER_UPDATE_MODE           2
#define EXIT_UPDATEPMODE            3
#define PENDING_UPDATE_MODE         4


#ifdef __GNUC__
#define PACKED __attribute__ ((packed))
#else
#define PACKED
#pragma pack( 1 )
#endif


typedef struct
{
    INT8U Major;
    INT8U Minor;
    INT32U Aux;
}PACKED Version_T;

typedef struct
{
    INT8U DevType;
    INT8U Slot;
    INT8U DevIden[DEV_IDENTIFIER_LEN];
    Version_T Version;    
    INT8U RebootData; /*0 - BMC Reboot,1 - Host Reboot , 2 - reboot host and BMC, 3 - NO reboot */
}DevInfo_T;

typedef struct DeviceInfo_T
{
    INT8U Slot;
    INT8U Identifier[DEV_IDENTIFIER_LEN];
    Version_T Version;
    INT8U RebootReq; /*0 - BMC Reboot,1 - Host Reboot, 2 - reboot host and BMC, 3 - NO reboot*/
    struct DeviceInfo_T *pNext;
}DeviceInfo_T;

typedef int (*pGetDeviceList)(INT8U*, DeviceInfo_T**);
typedef int (*pUpdateDevice)(INT8U*, INT8U, INT8S*, Version_T *);
typedef int (*pGetUpdateStatus)(INT8U*, INT8U, INT8U*);
typedef struct
{
    INT8U DevType;
    INT8U PluginType; /*0 - BMC dependent, 1 - BIOS dependent*/
    INT8U OpCode;   /*OpCode to get the FilePath*/
    INT8S DevName[MAX_DEV_NAME];
    INT8S GetDevList[MAX_SYMBOL_LEN];
    INT8S UpdateComp[MAX_SYMBOL_LEN];
    INT8S UpdateStatus[MAX_SYMBOL_LEN];
}PACKED PluginMetaInfo_T;

typedef struct
{
    INT8U DevType;
    INT8S DevName[MAX_DEV_NAME];
    INT8U PluginType;
    INT8U OpCode;
    void* pHandle;
    pGetDeviceList GetDevList;
    pUpdateDevice UpdateComp;
    pGetUpdateStatus UpdateStatus;
}PACKED FirmwarePlugin_T;

typedef struct
{
    INT8U BusNo;
    INT8U SlaveAddr;
    INT8U DevID[4];
}I2CIfcInfo_T;

typedef union
{
    I2CIfcInfo_T I2CInfo;
}DevIfcInfo_T;

typedef struct
{
    INT8U DevType; /*I2C or SPI Interface device*/
    DevIfcInfo_T DevIfcInfo;
}CPLDDevInfo_T;

#ifndef __GNUC__
#pragma pack()
#endif

FirmwarePlugin_T    g_firmware[MAX_PLUGIN_COUNT];
extern int g_plugincount;
extern int UnifiedUpdateMode;
extern int UpdateHandle;
extern void* pCUILHandle;
extern int LoadFirmwareUpdatePlugin();
extern char* GetDevName(INT8U DevType);
extern INT8U GetDevType(char *DevName);
extern FirmwarePlugin_T* GetPluginInfo(INT8U DevType);

#endif
