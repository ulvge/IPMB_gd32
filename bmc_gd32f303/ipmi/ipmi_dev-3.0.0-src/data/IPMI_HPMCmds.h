/* ***************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2012, American Megatrends Inc.             **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200,  Norcross,       **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************
 ****************************************************************
 *
 * IPMI_HPMCmds.h
 * IPMI HPM.1 Type Definitions.
 *
 * Author: Joey Chen <JoeyChen@ami.com.tw> 
 *         Rajaganesh R <rajaganeshr@amiindia.co.in>
 *
 *****************************************************************/
#ifndef IPMI_HPMCMDS_H
#define IPMI_HPMCMDS_H

#include "Types.h"



/* PICMG defined Group Extension Command */
#define PICMG_IDENTIFIER				0x00

/* HPM1 Revision per PICMG HPM.1 R1.0 SPEC */
#define HPM1_REV						0x00

#define DESC_STRING_LEN					12

#define INIT_UPLOAD_FOR_COMPARE         3
#define INIT_UPLOAD_FOR_UPGRADE			2
#define INIT_PREPARE_COMPONENTS			1
#define INIT_BACKUP_COMPONENTS          0

#define HPM_LONG_DURATION_CMD           1
#define HPM_SHORT_DURATION_CMD          2

typedef enum {
	COMPONENT_ID_0 = 0x00,
	COMPONENT_ID_1,
	COMPONENT_ID_2,
	COMPONENT_ID_3,
	COMPONENT_ID_4,
	COMPONENT_ID_5,
	COMPONENT_ID_6,
	COMPONENT_ID_7,
	COMPONENT_ID_MAX
} Component_ID_E;

typedef enum {
	GENERAL_COMPONENT_PROPERTIES,
	CURRENT_VERSION,
	DESCRIPTION_STRING,
	ROLLBACK_FW_VERSION,
	DEFERRED_UPG_FW_VERSION,
	RESERVED,
	OEM_COMPONENT_PROPERTIES = 192
} Component_Prop_Selector_E;

#pragma pack(1)


typedef struct {
	INT8U	Identifier;						/* 00h for PICMG */
} PACKED GetTargetUpgradeCapablitiesReq_T;

typedef struct {

    INT8U   CompletionCode;
    INT8U   Identifier;						
    INT8U   HPM1Ver;
    INT8U   IPMCGlobalCapablities;
    INT8U   UpgradeTimeout;					/* in 5s interval,00h if not supported */
	INT8U   SelfTestTimeout;				/* in 5s interval,00h if not supported */
	INT8U   Rollbackimeout;					/* in 5s interval,00h if not supported */
	INT8U   InaccessiblityTimeout;			/* in 5s interval,00h if not supported */
	INT8U	ComponentsPresent;

} PACKED GetTargetUpgradeCapablitiesRes_T;

typedef struct {

	INT8U	Identifier;						
	INT8U	ComponentID;
	INT8U	ComponentPropSelector;

} PACKED GetComponentPropertiesReq_T;

typedef struct {

	INT8U	FWRev1;
	INT8U	FWRev2;
	INT32U	AuxillaryFWRevInfo;

} PACKED FirmwareVersion_T;

typedef struct {
    INT8U               CompletionCode;
    INT8U               Identifier;
    FirmwareVersion_T   CurrFW;
} PACKED GetCompPropFWRes_T;

typedef struct {
    INT8U               CompletionCode;
    INT8U               Identifier;
    INT8U               DescString[DESC_STRING_LEN];
} PACKED GetCompPropDescStrRes_T;

typedef struct {
    INT8U           	CompletionCode;
    INT8U           	Identifier;
    INT8U				GeneralCompProp;
} PACKED GetCompPropGeneralRes_T;

typedef struct {

	INT8U	Identifier;						
	INT8U	Components;
	INT8U	UpgradeAction;

} PACKED InitiateUpgActionReq_T;

typedef struct {

	INT8U	CompletionCode;
	INT8U	Identifier;						

} PACKED InitiateUpgActionRes_T;

typedef struct {

	INT8U	Identifier;						

} PACKED QuerySelfTestResultsReq_T;

typedef struct {

	INT8U	CompletionCode;
	INT8U	Identifier;						
	INT8U	SelfTestResult1;
	INT8U	SelfTestResult2;

} PACKED QuerySelfTestResultsRes_T;

typedef struct {

    INT8U           Identifier;             
    INT8U           BlkNumber;
    INT8U           FWData[64];

} PACKED UploadFirmwareBlkReq_T;

typedef struct {

    INT8U   CompletionCode;
    INT8U   Identifier;                    

} PACKED UploadFirmwareBlkRes_T;

typedef struct {

    INT8U   Identifier;                     
    INT8U   Component;
    INT32U  ImageSize;

} PACKED FinishFWUploadReq_T;

typedef struct {

    INT8U   CompletionCode;
    INT8U   Identifier;                     

} PACKED FinishFWUploadRes_T;

typedef struct {

    INT8U   Identifier;                     

} PACKED ActivateFWReq_T;

typedef struct {

    INT8U   CompletionCode;
    INT8U   Identifier;                     

} PACKED ActivateFWRes_T;

typedef struct {

    INT8U   Identifier;                     

} PACKED AbortFirmwareUpgradeReq_T;

typedef struct {

    INT8U   CompletionCode;
    INT8U   Identifier;                    

} PACKED AbortFirmwareUpgradeRes_T;

typedef struct {

    INT8U   Identifier;                    

} PACKED GetUpgradeStatusReq_T;

typedef struct {

    INT8U   CompletionCode;
    INT8U   Identifier;                    
    INT8U   CmdInProgress;
    INT8U   LastCmdCC;

} PACKED GetUpgradeStatusRes_T;

typedef struct {

    INT8U   Identifier;                    

} PACKED QueryRollbackStatusReq_T;

typedef struct {

    INT8U   CompletionCode;
    INT8U   Identifier;                    
    INT8U   RollbackStatus;

} PACKED QueryRollbackStatusRes_T;

typedef struct {

    INT8U   Identifier;                     

} PACKED InitiateManualRollbackReq_T;

typedef struct {
    
    INT8U   CompletionCode;
    INT8U   Identifier;                   

} PACKED InitiateManualRollbackRes_T;

typedef struct
{
    INT8U   CmdCC;         /* Completion Code of Last Executed Command     */
    INT8U   CmdInProgress; /* Cmd no in progress if its long duration cmd  */
                           /* also to be used for identifying previous cmd */
    INT8U   CmdDuration;   /* flag for long | short duration commands      */  
} PACKED HPMCmdStatus_T;

typedef enum
{
    HPM_EMPTY = 0x00,
    INIT_UPLOAD,
    UPLOAD_FW_BLK,
    FLASHING,    
    VERIFYING,
    UPG_DONE,
    ACTIVATION_PENDING
} HPMStates_E;

typedef struct 
{
    HPMStates_E State;
    INT8U       UpAction;
    INT8U       Abort;  
} PACKED FWUpgState_T;

typedef enum {
    FWVER_TYPE_CURRENT = 0x00,
	FWVER_TYPE_ROLLBACK, 
	FWVER_TYPE_DEFER,
    FWVER_TYPE_MAX
} FwVersionType_E;

#pragma pack()

#endif  /* IPMI_HPMCMDS_H */
