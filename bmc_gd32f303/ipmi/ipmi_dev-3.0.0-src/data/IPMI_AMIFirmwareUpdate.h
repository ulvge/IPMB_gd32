/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2012-2013, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************
 ****************************************************************
 *IPMI_AMIFirmware.h
 * AMI Firmware Update specific IPMI Commands
 *
 * Author: Abhitesh <abhiteshk@amiindia.co.in>
 *
 *****************************************************************/
#ifndef IPMI_AMI_FIRMWARE_DEVICE_H_
#define IPMI_AMI_FIRMWARE_DEVICE_H_

#define FWUPDATE_CONFIG 		"/conf/firmwareupdate.conf"
#define LISTOFUPTATEBYBIOS_2        	"/conf/listofupdatebybios.ini"

#define MAXDATA   256

typedef struct
{
    INT8U DeviceType;
	INT16U  DataOffset;
} PACKED AMIGetReleaseNoteReq_T;
    
typedef struct
{   
    INT8U   CompletionCode;
    INT16U  NextOffset;
    INT16U  DataLen;
    INT8U   ChunkData[128];
} PACKED AMIGetReleaseNoteRes_T;

typedef struct
{
    INT8U FWSubCommand;
    INT8U FWCommandData[256];
}PACKED AMIFWCommandReq_T;

typedef struct
{   
    INT8U CompletionCode;
    INT8U FWResData[256];
} PACKED AMIFWCommandRes_T;

typedef struct
{
    INT8U Major;
    INT8U Minor;
    INT32U Aux;
}PACKED Ver_T;

typedef struct 
{
    INT8U       DevType;
    INT8U       Slot;
    INT8U       Identifier[20];
    INT8U       DevStatus;
    INT8U       OpStatus;
    Ver_T   BundleVer;
    Ver_T   CurrentVer;
}ImageInform_T;

typedef struct 
{
	INT8U ImageCnt;
	ImageInform_T ImageInfo[256];
}GetAllImageInfo_T;

typedef struct 
{
	 INT8U UpdateFlag;
	 INT8U UpdateCnt;
	 INT8U DevType;
	 INT8U Slot;
	 INT8U	Identifier[20];
 }UpdateCompReq_T;

typedef struct
{
    INT8U DevType;
    INT8U Slot;
    INT8U Identifier[20];
    INT8U Status;
    INT32U Timestamp;
    INT16U UTCOffset;
    Ver_T Version;
    INT8U UpdatePercentage;
}GetUpdateStatus_T;

typedef struct
{
	INT8U UpdateCnt;
    GetUpdateStatus_T UpdateStatus [256];
}GetAllUpdateStatusInfo_T;

typedef struct RemoteImgInfo_T
{
	INT8U ShareType;
	INT8U UserName[16];
	INT8U Pswd[16];
	INT8U ShareName[64];
	INT8U FileName[128];
	INT8U IpAddr[16];
 }FWRemoteImg_T;

#define IPMI_AMI_SET_UPDATE_MODE                    0
#define IPMI_AMI_GET_IMAGES_INFO                    1
#define IPMI_AMI_SET_NETWORK_SHARE_CONFIG           2
#define IPMI_AMI_GET_NETWORK_SHARE_CONFIG           3
#define IPMI_AMI_SET_NETWORK_SHARE_OPERATION        4
#define IPMI_AMI_SET_UPDATE_COMPONENT               5
#define IPMI_AMI_GET_UPDATE_COMP_STATUS             6
#define IPMI_AMI_GET_UPDATE_COMP_FOR_BIOS           7
#define IPMI_AMI_SET_UPDATE_COMP_STATUS_BY_BIOS     8
#define IPMI_AMI_GET_COMPONENT_NAME                 9
#define IPMI_AMI_SET_CANCEL_COMPONENT_UPDATE		10
#define IPMI_AMI_GET_VALIDATE_BUNDLE                11
#define IPMI_AMI_REARM_FIRMWARE_TIMER               12

#endif
