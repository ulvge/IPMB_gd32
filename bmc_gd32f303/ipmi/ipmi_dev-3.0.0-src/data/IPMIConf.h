/******************************************************************
 ******************************************************************
 ***                                                                                                           **
 ***    (C)Copyright 2005-2006, American Megatrends Inc.                            **
 ***                                                                                                           **
 ***    All Rights Reserved.                                                                          **
 ***                                                                                                           **
 ***    5555 , Oakbrook Pkwy, Norcross,                                                       **
 ***                                                                                                           **
 ***    Georgia - 30093, USA. Phone-(770)-246-8600.                                  **
 ***                                                                                                           **
 ******************************************************************
 ******************************************************************
 ******************************************************************
 *
 * IPMIConf.h
 * IPMI related configurations
 *
 *  Author: Winston <winstonv@amiindia.co.in>
 ******************************************************************/
 #ifndef _IPMIConf_H
 #define _IPMIConf_H

 #include "Types.h"
 #include "OSPort.h"
 #include "PMConfig.h"
 #include "Session.h"
 #include "SensorMonitor.h"
 #include "IPMI_SDR.h"
 #include "MsgHndlr.h"
 #include "RMCP+.h"
 #include "UDSIfc.h"
 #include "PEFTmr.h"
 #include "IPMI_SEL.h"
 #include "Terminal.h"
 #include "SerialIfc.h"

 #include "SDRRecord.h"
 #include "SELRecord.h"
 #include "MBMCInfo.h"



#define MAX_PLATFORMID_SIZE 20
#define MAX_DYNMC_TBL_COUNT 50
#define MAX_MGMT_DEV_LOC_REC 1


#define MAX_SOL_DATA_SIZE 248
#define MAX_SEN_NAME_SIZE 64
#define MAXFILESIZE 0x64
#define MAXDIRSIZE 0x64
#define MAX_HMAC_BUF_SIZE                   128
#define	IPMI_CONFIG_FILE	"/conf/BMC/IPMI.conf"
 #define   MAX_IPMI_IFCQ 256
 #define   IPMI_IFC_NAME_LEN   60
 #define   IFCENABLED       0x1
 #define   IFCDISABLED     0x0
 #define   CH_NOT_USED   0xFF
 #define MAX_NUM_CHANNELS		0xc
#define PRIMARY_IPMB_CHANNEL_TYPE 0x00
#define SMLINK_IPMB_CHANNEL_TYPE    0x04
#define THIRD_IPMB_CHANNEL_TYPE    0x0A
#define ICMB_CHANNEL_TYPE                   0x05
#define SMBUS_CHANNEL_TYPE                0x06
#define SERIAL_CHANNEL_TYPE                0x07
#define SMM_CHANNEL_TYPE                    0x08
#define LAN_RMCP_CHANNEL4_TYPE       0x09
#define SYS_CHANNEL_TYPE                     0x0f
#define USB_CHANNEL			0x0B
#define SYS_IFC_CHANNEL			0x0F
#define PRIMARY_IPMB_CHANNEL		0x00
#define MAX_IFC_NAME                          16
#define LAN1_INDEX 0
#define LAN2_INDEX 1
#define LAN3_INDEX 2
#define LAN4_INDEX 3
#define NM_PRIMARY_IPMB_BUS   0
#define NM_SECONDARY_IPMB_BUS 1
#define NM_THIRD_IPMB_BUS 2
#define MAX_IPMI_CONFIG_MAPS   64
#define MAX_IPMI_IFC_COUNT  20

#define EMMCPATH      "/mnt/sdmmc0p"

#define MAX_SIZE_KEY 56		/* For Blowfish Encryption Algorithm, the key can have maximum of 56 bytes. */

#define GET_LIB_META_INFO_SYMBOL "GetLibMetaInfo"
#define SOL_SEND_BUF  0x00
#define SOL_RECV_BUF  0x01
#define  MANUAL_OFFSETS  "Etc"

#define FLUSH_INI_DELETE_DAT 0xFF
#define FLUSH_TO_INI_ALONE 0xFE
#define FLUSH_OEM_CONFIGS_TO_INI 0xE0
#define FLUSH_TO_INI_ALONE_BMCINST 0xFD
//Note:- action parameter 0xE0 to 0xEF are used for flushing oem configs to ini
//Oems can make use of different action parameter to flush particular oem configurations

//#define SNMP_LIBS "/usr/local/lib/libsnmpusers.so"

#define MAX_PLDM_BIOS_TBL_SIZE 8

typedef enum
{
    LAN_SESSION_TYPE=1,
    SERIAL_SESSION_TYPE,
    SERIAL_TERMINAL_SESSION_TYPE,
    KCS_SESSION_TYPE,
    BT_SESSION_TYPE,
    IPMB_SESSION_TYPE,
    UDS_SESSION_TYPE,
    USB_SESSION_TYPE,
    SSIF_SESSION_TYPE,
}IPMI_SESSIONTYPE;


/*---------------------- Local typedefs --------------------------*/
/* For storing the  management controller owner id to enable event */
typedef struct
{
    INT8U   OWNERID;
    INT16U  PowerNotification;
    BOOL  Status;

} PACKED  Mgmt_T;

typedef struct
{
    INT8U   SensorNum;
    INT8U   SensorTypeCode;
    INT8U   LowerNonCritical;
    INT8U   LowerCritical;
    INT8U   LowerNonRecoverable;
    INT8U   UpperNonCritical;
    INT8U   UpperCritical;
    INT8U   UpperNonRecoverable;

} PACKED SensorThresholds;
typedef struct
{
    int       buf_size;
    int       head;
    int       tail;
    int       is_full;
    INT8U*  p_buf;

}PACKED RingBuf_T;

typedef struct
{
    int BMCInst;
    int Len;
    char* Argument;
}PACKED BMCArg;



// typedef void (*pThreadFn_T) (pthread_t ThrdID);


typedef struct
{
    INT8U *pFlag;
    char   SectionName[50];
}PACKED FlagInit_T;

typedef struct
{
    INT8U SendMsgTimeout;
    INT8U SessionTimeOut;
    INT8U SOLSessionTimeOut;
    INT8U PrimaryIPMBBusNumber;
    INT8U SecondaryIPMBBusNumber;
    INT8U ThirdIPMBBusNumber;
    INTU ChassisTimerInterval;
    INT8U EEPROMBusNum;
    INT8U AlterPreConfiguredEntries;
    INT8U ChassisInterruptSupport ;
    INT8U InterruptSensorHandling;
    INT8U EventsForRearm ;
    char *pSOLPort;
    char *pSerialPort;
    INT8U MaxSession;
    INT8U SDRAllocationSize;
    INT8U SELAllocationSize;
    INT8U BMCSlaveAddr;
    INT8U MaxUsers;
    INT8U MaxChUsers;
    INT8U PrimaryIPMBSupport ;
    INT8U SecondaryIPMBSupport ;
    INT8U ThirdIPMBSupport ;
    INT8U SMMIfcSupport ;
    INT8U SerialIfcSupport ;
    INT8U SerialTerminalSupport;
    INT8U LANIfcSupport ;
    INT8U KCS1IfcSupport ;
    INT8U KCS2IfcSupport ;
    INT8U KCS3IfcSuppport ;
    INT8U SYSIfcSupport;
    INT8U SMBUSIfcSupport;
    INT8U ICMBIfcSupport ;
    INT8U MaxLanChannel ;
    INT8U KCSSMMChannel;
    INT8U USBIfcSupport ;
    INT8U SOLIfcSupport ;
    INT8U APMLSupport;
    INT8U DCMISupport ;
    INT8U HPMSupport;
    INT8U OPMASupport;
    INT8U GrpExtnSupport ;
    INT8U CardInFlashMode ;
    INT8U VLANIfcSupport;
    INT8U UDSIfcSupport;
    INT8U LinkDownResilentSupport;
    INT8U RearmSetSensorThreshold;
    INT8U APMLBusNumber;
    INT8U PrimaryIPMBAddr;
    INT8U SecondaryIPMBAddr;
    INT8U ThirdIPMBAddr;
    INT8U BTIfcSupport;
    INT8U ChassisTimerSupport;
    INT8U IPMIFirewallSupport;
    INT8U InternalPwrGoodMonitoring;
    INT8U SSIFBusNumber;
    INT8U SSIFAddr;
} IPMIConfig_T;

typedef struct
{
    INT8U     MatchedEventSeverity;
    INT8U     AlertStr[ALERT_STR_MAX_BLOCKS* ALERT_STR_BLOCK_SIZE];
    INT16U   LANAlertSequence; /**< Contains Event severity from PEF configuration */
    INT32U         SpecificTrap; /* SNMP Specific Trap field */
    PEFTmrMgr_T    PEFTmrMgr;
    INT16U LastProcessedIDs[MAX_DEFERRED_ALERTS];
//    pthread_mutex_t PEFSharedMemMutex;
} PEF_T;

typedef struct
{
    INT16U DCMISamplingTimer;
    INT16U DCMITimer;
    INT16U DCMICurrentPower;
    INT16U DCMIMinPower;
    INT16U DCMIMaxPower;
    INT16U DCMIPrevPower;
    INT16U DCMIAvgPower;
    int FirstSample;
    INT8U   DCMIThermalTimer;
    INT8U   DCMIThermalFirstSample;
    INT8U   DCMICurrTempReading[MAX_TEMP_INSTANCE];
}DCMI_T;

typedef struct
{
    INT16U  SelReservationID;
    INT32U  LastEvtTS;
    INT8U    SELLimit;
    INT16U  PartialAddRecordID;
    INT8U    PartialAddRecOffset;
    INT8U    PartialAdd;
    INT8U    SenMonSELFlag;
    INT16U  MaxSELRecord;
    INT8U    RsrvIDCancelled;
    BOOL SELOverFlow;
    SELEventRecord_T  SelPartialAddRecord;
    INT8U SELEventMsg [16];
    int selalmostfull;
    INT16U  SELCnt; /*This variable is used only when SEL reclaim feature is not enabled.*/
 //   pthread_mutex_t SELMutex;
} SEL_T;

typedef struct
{
    INT8U     SDRError;
    BOOL      UpdatingSDR;
    INT8U     UpdatingChannel;
    SDRRepositoryAllocInfo_T    RepositoryAllocInfo;
    SDRRepositoryInfo_T       RepositoryInfo;
    INT8U   TrackPOffset;
    INT16U  TrackRecID;
    INT16U  ReservationID;
    INT8U IPMB_Seqnum;
    INT8U     PartAddbytes;
    INT16U LatestRecordID;
    INT16U NumMarkedRecords;
    SDRRepository_T*   SDRRAM;
 //   pthread_mutex_t SDRMutex;
} SDR_T;

typedef struct
{
    INT8U    IPMBBusNum;
    INT8S    IPMBBusName[64];
    int  IPMBSlaveFd;
    int  IPMBMasterFd;
    INT8U	m_IPMBSeqNo;

} IPMB_T;

typedef struct
{
	INT8U   SerialPkt [MAX_SERIAL_PKT_SIZE];
	INT16U  SerialPktIx;
	INT16U  SerialPktLen;
	INT8U   CurState;
	INT8U   PrevByteEsc;
	INT8U   CurMode;
	int     serial_fd;
    INT32U  SerialSessionID;
	INT8U   SerialSessionActive;
    INT32U  TerminalSessionID;
    INT8U   SessionActivated;
	INT8U   TerminalPkt [MAX_SERIAL_PKT_SIZE];
	INT8U   TerminalPktIx;
	INT8U   TerminalIfc;
	BOOL    ErrCode;
	INT8U   ErrIndex;
    INT8U   TCurState;
	INT8U   TPrevByteEsc;
	INT8U   CursorPos;
    int     terminal_fd;
    INT8U   CurSwitchDir; //should sync in PDK_SwitchEMPMux, record the status of UART switch, MUX_2_BMC, MUX_2_SYS.
}SERIAL_T;

typedef struct
{
    INT8U   GratArpCnt;
    INT8U   ArpSuspendReq;
    INT8U   SetInProgress;
    INT8U   TmrSet;
    INT8U   SelfTestByte;
    INT8U   ManufacturingTestOnMode;
    INT8U   WarmReset;
    INT8U   ColdReset;
    INT8U   BBlk;
    INT8U   ChassisControl; // = 0xFF;
    INT8U   ChassisIdentify;
    INT32U  ChassisIdentifyTimeout;
    INT8U   ChassisIdentifyForce;
    INT8U   FireWallRequired;// = TRUE;  //This Flag decides if firewall required on a specific channnel.
    //INT8U   CurKCSIfcNum;
    //INT8U   CurSessionType;
    //INT8U   MsgHndlrTblSize; // = 7;
    //INT8U   GroupExtnMsgHndlrTblSize;// = 0;
    //INT8U   TimerTaskTblSize;// = 7;

} MSGHNDLR_T;

typedef struct
{
    INT8U NMSupport;
    INT8U NM_IPMBBus;
    INT8U NMDevSlaveAddress;
}NMInfo_T;

typedef struct
{
    INT8U   TmrSet;
}WDT_T;

typedef struct
{
    INT16U NumThreshSensors;
    INT16U NumNonThreshSensors;
    int MonitorBusy;//=0;
    INT16U SensorMonitorLoopCount;
    bool  InitAgentRearm; // = FALSE;
    INT16U ValidSensorCnt;
    INT16U ValidSensorList[MAX_SENSOR_NUMBERS + MAX_ME_SENSOR_NUMBERS]; /*  Both BMC sensor and ME sensor will temperary store here, for sensor monitor. */
    INT8U OwnerIDList[MAX_SENSOR_NUMBERS + MAX_ME_SENSOR_NUMBERS];      /*  Use OwnerIDList to distinguish ME sensors and BMC sensors.  */
    HealthState_T  HealthState;
    INT32U PowerOnTick;
    INT32U SysResetTick;
    
} SENSOR_T;

typedef struct
{
    volatile int    cts;
    int             sol_fd ;
    INT32U          cur_tick  ;
    INT8U           sol_retry ;
    INT8U			sol_ack_to_ticks;
    INT8U			send_char_threshold ;
    INT8U			send_char_timeout;
    int             SOLSessionActive;
    int	            activate_sol;
    int		        is_sending ;
    INT8U	        send_seq_num ;
    INT8U	        sent_char_count;
    INT32U	        last_sent_tick;
    INT32U	        first_byte_received_tick;
    int             retry   ;
    INT8U           MSVTEsc; /* contains escape bytes */
     INT8U recv_seq_num      ;
    INT8U recv_char_count   ;
    int   last_packet_nack    ;
    int   send_ack        ;
    INT8U ack_seq_num     ;
//    INT8U   SOLBuf [MAX_SOL_DATA_SIZE + 4 + sizeof (LANRMCPPkt_T) + sizeof (SessionHdr2_T) + 32 + 100];
    int StopReadingSerialData;
    RingBuf_T  m_buf [2];
} SOL_T;


typedef struct
{
    INT8U		total_frus;
//    INT8U       FRUInfo[MAX_PDK_FRU_SUPPORTED];
//    FRUInfo_T	*m_FRUInfo[MAX_PDK_FRU_SUPPORTED];
//    pthread_mutex_t FRUMutex;
}FRU_T;

typedef struct
{
    INT32U CurTimerTick;
    INT32U BootValidTimerTick;
    INT32U MinutesTick;
}Timer_T;

typedef struct
{
    INT32U UserInfoAddr;
    INT32U DcmicfgAddr;
    INT32U WDTDATAddr;
    INT32U PEFConfigAddr;
    INT32U ChassisConfigAddr;
    INT32U SMConfigAddr;
    INT32U BridgeMgmtAddr;
    INT32U SystemInfoConfigAddr;
    INT32U LANCfsAddr;
    INT32U RMCPPlusAddr;
    INT32U SOLCfgAddr;
    INT32U FFCmdConfigTblAddr;
    INT32U OPMAConfigAddr;
    INT32U SmtpConfigAddr;
    INT32U GenConfigAddr;
    INT32U ChConfigAddr;
    INT32U TriggerEventAddr;
    INT32U LoginAuditCfgAddr;
    INT32U AMIConfigAddr;
    INT32U IPMIConfLocAddr;
    INT32U SSIConfigAddr;
    INT32U VersionConfigAddr;
    INT32U EncUserPasswdAddr;
    INT32U LANIFCConfigAddr;
    INT32U BONDConfigAddr;
    INT32U PEFRecordDetailsConfigAddr;
    INT32U DualImageConfigAddr;
}PACKED IPMICfgLoc_T;

typedef int(*flushini)(int);
typedef int(*flushchini)(int,int);

typedef struct
{

    INT8U *IniconfigAddr;

    flushini Flushini_Hndl;
    flushchini Flushchini_Hndl;
    INT8U Index;
}PACKED IPMIConfigMap_T;

/* The CDF(Configuration Device File) related structures configured using MDS should be 
     maintained in top of BMCInfo_t structure. The IPMI configuration version is maintained in
     IPMI_CONFIG_VERSION macro and the version number should be incremented in counts
     of '1' whenever there are changes in CDF files used in MDS and  in CDF related structures */
typedef struct
{
//    VersionConfig_T VersionConfig;  //CDF structure
//    ChassisConfig_T     ChassisConfig;   //CDF structure
//    BridgeMgmtSHM_T     BridgeMgmt;   //CDF structure
//    UserInfo_T UserInfo[MAX_USER_CFG_MDS];  //CDF structure
//    WDTConfig_T         WDTConfig;       //CDF structure
//    PEFConfig_T         PEFConfig;          //CDF structure
//    SMConfig_T SMConfig;                    //CDF structure
//    SystemInfoConfig_T   SystemInfoConfig;      //CDF structure
 //   LANConfig_T         LANCfs[MAX_LAN_CHANNELS];       //CDF structure
//    RMCPPlus_T          RMCPPlus[MAX_LAN_CHANNELS];     //CDF structure
//    SOLConfig_T         SOLCfg[MAX_LAN_CHANNELS];       //CDF structure
//    FFCmdConfigTbl_T    FFCmdConfigTbl [MAX_FF_CMD_CFGS];   //CDF structure
//    OPMA_Config_T  OPMAConfig;          //CDF structure
 //   Smtp_Config_T SmtpConfig[MAX_LAN_CHANNELS];     //CDF structure
//    GENConfig_T GenConfig;          //CDF structure
//    ChcfgInfo_T ChConfig[MAX_NUM_CHANNELS];     //CDF structure
//    DCMICfg_T Dcmicfg;      //CDF structure
//    LoginAuditConfig_T LoginAuditCfg;       //CDF structure
//    TriggerEventCfg_T TriggerEvent;         //CDF structure
//    AMIConfig_T AMIConfig;                      //CDF structure
//    LANIFCConfig_T LanIfcConfig[MAX_LAN_CHANNELS];		 //CDF structure
//    BondIface BondConfig;		 //CDF structure
//    PEFRecordDetailsConfig_T PEFRecordDetailsConfig;		 //CDF structure
//    char EthIndex[64];
//    char EthIndexValues[64][64];
//    char par_path[64];
    int    LANIfccount;
    INT8U *pStorageNVRAM;
    INT8U *pNVRUsrConfig;
    INT8U PrimaryIPMBCh;
    INT8U SecondaryIPMBCh;
    INT8U ThirdIPMBCh;
    INT8U RMCPLAN1Ch ;
    INT8U RMCPLAN2Ch ;
    INT8U RMCPLAN3Ch ;
    INT8U RMCPLAN4Ch ;
    INT8U ICMBCh ;
    INT8U SMBUSCh ;
    INT8U SERIALch ;
    INT8U SMMCh ;
    INT8U SYSCh ;
    IPMIConfig_T IpmiConfig;
    INT8U *pNVRDCMICfg;
    SessionTblInfo_T SessionTblInfo;
//    UDSSessionTblInfo_T UDSSessionTblInfo;
//    BMCSharedMem_T BMCSharedMem;
    SensorSharedMem_T SensorSharedMem;
//    SocketTbl_T  *pSocketTbl;
//    SocketTbl_T  *pUDSocketTbl;
    //PEF_T PefConfig;
    //SEL_T SELConfig;
    SDR_T SDRConfig;
    IPMB_T IPMBConfig;
    ///SOL_T SOLConfig;
 //   LAN_T LANConfig;
    MSGHNDLR_T Msghndlr;
    SENSOR_T SenConfig;
 //   HAL_T HALConfig;
    FRU_T FRUConfig;
//    UDS_T UDSConfig;
    SERIAL_T SerialConfig;
//    DCMI_T DCMIConfig;
//    NMInfo_T NMConfig;
//    WDT_T WDTCfg;
//    Timer_T TimerCfg;
 //   pthread_mutex_t BMCSharedMemMutex;
//    pthread_mutex_t SensorSharedMemMutex;
 //   pthread_mutex_t CachSenReadMutex;
//    pthread_mutex_t SessionTblMutex;
 //   pthread_mutex_t CCSharedMemMutex;
 //   pthread_mutex_t RestartcauseMutex;
 //   pthread_mutex_t hCTSharedMemLEDTmr;
 //   pthread_mutex_t hAPISharedMemMutex;
  //  pthread_mutex_t ResetSharedMemMutex;
//    pthread_mutex_t m_hSMSharedMemMutex;
//    pthread_mutex_t hWDTSharedMemMutex;
//    pthread_mutex_t ThreadSyncMutex;
//    pthread_mutex_t SMmutex;
//    pthread_mutex_t CmdHndlrMutex;
//    pthread_mutex_t PendBridgeMutex;
//    pthread_mutex_t OBSMSharedMemMutex;
//    pthread_mutex_t UDSSessionTblMutex;
//    pthread_mutex_t UDSSocketTblMutex;
//    pthread_mutex_t ChassisMutex;
//    pthread_mutex_t EventMutex;
//    pthread_mutex_t BMCMsgMutex;
//    pthread_mutex_t ChUserMutex;
//    sem_t WDTSem;
    int SetWDTUpdated;
    int WDTPreTmtStat;
//    DynamicLoader_T g_DynamicInfoTable[MAX_DYNMC_TBL_COUNT];
    INT32U g_DynamicInfoTableCount ;
    //DynamicLoader_T DynamicInfoTable[100];
    //INT32U DynamicInfoTableCount ;

    INT8U MsgHndlrTblSize;
    INT8U GroupExtnMsgHndlrTblSize;
    INT8U TimerTaskTblSize;


    MsgHndlrTbl_T MsgHndlrTbl[15]; // m_MsgHndlrTbl
//    GroupExtnMsgHndlrTbl_T GroupExtnMsgHndlrTbl [10];
//    TimerTaskTbl_T    TimerTaskTbl [20];

    INT32U CurTimerTick;
    int kcsfd[3]; //Will be removed once fix is done in hal driver
//    ChcfgInfo_T NVRChcfgInfo[MAX_NUM_CHANNELS];
//    IPMICfgLoc_T IPMIConfLoc;
    Mgmt_T MgmtTbl[MAX_MGMT_DEV_LOC_REC];
    void * dlpar_handle;
    
    INT8U               InternalSensorTblSize;
//    InternalSensorTbl_T InternalSensorTbl[INT_SENSOR_NUM];

    //INT8U g_sol_ack_to_ticks;
//    SSIConfig_T         SSIConfig;
    INT8U               SlotID;
    INT8U               IPMBAddr;
    INT8U    HostOFFStopWDT;    // This flag is set to TRUE, when Chassis is powered off
    INT8U    SendMsgSeqNum;
//    EncryptedUserInfo_T EncryptedUserInfo[MAX_USER_CFG_MDS];
 //   pfunc g_PDKPARHandle[MAX_PDKPAR_HANDLE];
//    IPMIConfigMap_T Ipmiconfigmap [MAX_IPMI_CONFIG_MAPS];
//    SELReclaimRepository_T SELReclaimRepos;
//    INT8U BridgeMsgKCSIfc;
//    DualImageCfg_T DualImageCfg;
//    INT8U extended_log_conf;
   //BIOS_Control_buffer_T dataBuffer;
} BMCInfo_t;
 extern BMCInfo_t g_BMCInfo;
 
 typedef struct
  {
  	int InstanceID[MAX_PLDM_BIOS_TBL_SIZE];
  	INT32U DataTransferHandle[MAX_PLDM_BIOS_TBL_SIZE];
  	INT8U TransferFlagState[MAX_PLDM_BIOS_TBL_SIZE];
  	INT8U TableType[MAX_PLDM_BIOS_TBL_SIZE];
  	INT32U ReadOffset[MAX_PLDM_BIOS_TBL_SIZE];
  	INT32U WriteOffset[MAX_PLDM_BIOS_TBL_SIZE]; 
  	INT16U TimeOut[MAX_PLDM_BIOS_TBL_SIZE];
 // 	pthread_mutex_t PLDMTblMutex;
 } PACKED PLDMConfig_T;


/* MBMCInfo_t structure will hold the variables common for all the instances of BMC*/
typedef struct
{
    INT8U    PwdEncKey[MAX_SIZE_KEY];
    int flashpipereq;
    int sbmcinst;
    INT8U SrcSessionHndl;
    int FlashType;
    FileTrans_T FileUpTrack[MAX_FILE_TRANSFER];
    FileTrans_T FileDownTrack[MAX_FILE_TRANSFER];
//    pthread_mutex_t FileTransMutex;
//    sem_t FileTransSem;
    PLDMConfig_T PLDMConfig;
}MBMCInfo_t;


typedef struct
{
    INT8U Action;
    INT8U Params;
    INT8U BMCInstance;
    INT8U Size;
}PACKED FlushToIni_T;

 typedef struct
{
    INT8U chno; // channel no
    int Privilege;
} ChannelPriv_t;

extern ChannelPriv_t g_ChannelPrivtbl[MAX_NUM_CHANNELS];
extern char PlatformID [MAX_PLATFORMID_SIZE];
extern INT32U IPMITimeout;
//extern CoreFeatures_T g_corefeatures;
//extern CoreMacros_T g_coremacros;
extern char  g_FlashingImage;
extern MBMCInfo_t g_MBMCInfo;
extern void *dlibbackup_handle;

extern int FlushWDT(int BMCInst);
extern int FlushPEF(int BMCInst);
extern int FlushPEFRecordDetails(int BMCInst);
extern int FlushChassis(int BMCInst);
extern int FlushSerial(int BMCInst);
extern int FlushBridge(int BMCInst);
extern int FlushSysInfo(int BMCInst);
extern int FlushLAN(int BMCInst,int ChIndex);
extern int FlushRMCP(int BMCInst,int ChIndex);
extern int FlushSOL(int BMCInst,int ChIndex);
extern int FlushFirewall(int BMCInst);
extern int FlushOPMA(int BMCInst);
extern int FlushSMTP(int BMCInst,int ChIndex);
extern int FlushGEN(int BMCInst);
extern int FlushCHANNEL(int ChNum,int BMCInst);
extern int FlushUSRCFG(int BMCInst);
extern int FlushDCMICFG(int BMCInst);
extern int FlushTRIGGEREVTCFG(int BMCInst);
extern int FlushLOGINAUDITCFG(int BMCInst);
extern int FlushAMIConfig(int BMCInst);
extern int FlushSSIConfig(int BMCInst);
extern int FlushVersionConfig(int BMCInst);
extern int FlushEncUsrPswdCfg(int BMCInst);
extern int FlushLANIfcConfig(int BMCInst);
extern int FlushBondCfg(int BMCInst);
extern int FlushDualImageCfg(int BMCInst);
 /**
 *@fn InitIPMIConfig
 *@brief This function is invoked to get the IPMI Configurations
 *@return Returns 0 on success
 */
 extern  int InitIPMIConfig (int BMCInst);\
 int InstallSignal(int SigNum,void (*sighandler)(int sig));
// extern IPMIQueue_T g_IPMIIfcQueue[MAX_IPMI_IFCQ];

/*********************** oem define ******************************/
 //GPIO define

 //slot ID
 #define GPIO_GA0					16	//GPIOC0
 #define GPIO_GA1					17	//GPIOC1
 #define GPIO_GA2					18	//GPIOC2
 #define GPIO_GA3					19	//GPIOC3
 #define GPIO_GA4					20	//GPIOC4

 #define BMC_LED					63	//GPIOH7



#endif //_IPMIConf_H



