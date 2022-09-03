/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 *****************************************************************
 *
 * IPMI_Main.h
 * IPMI_Main external declarations.
 *
 * Author: Bakka Ravinder Reddy <bakkar@ami.com>
 *
 *****************************************************************/
#ifndef IPMI_MAIN_H
#define IPMI_MAIN_H

#include "Types.h"

#pragma pack( 1 )

#define AMI_BIN_PATH        	"/usr/local/bin/"
#define MSG_PIPES_PATH      	"/var/"
#define NV_DIR_PATH		"/conf/BMC"
#define NV_EXLOG_DIR_PATH   	"/extlog/BMC"
#define SP_BIN_PATH         	"/conf/"
#define MUTEX_PATH          	"/var/"
#define DEFAULT_CONFIG_PATH	"/etc/defconfig/"
#define IPMI_PID_PATH 		"/var/IPMIPid"

/* Task ID */

#define MSG_HNDLR_TASK_ID       1
#define CHASSIS_CTRL_TASK_ID    2
#define CHASSIS_TIMER_TASK_ID   3
#define KCS_IFC_TASK_ID         4
#define LAN_IFC_TASK_ID         5
#define SERIAL_IFC_TASK_ID      6
#define IPMB_IFC_TASK_ID        7
#define SOL_IFC_TASK_ID         8
#define TERMINAL_IFC_TASK_ID    9
#define PEF_TASK_ID             10
#define TIMER_TASK_ID           11
#define SSIF_IFC_TASK_ID        12


/* Task Path    */
#define TERMINAL_IFC_TASK   AMI_BIN_PATH "Terminal"


/*Messaging pipes path*/

#define MSG_HNDLR_Q         MSG_PIPES_PATH "MsgHndlrQ"
#define RCV_MSG_Q_01        MSG_PIPES_PATH "RcvMsgQ01"
#define RCV_MSG_Q_10        MSG_PIPES_PATH "RcvMsgQ10"
#define RCV_MSG_Q_11        MSG_PIPES_PATH "RcvMsgQ11"
#define RCV_MSG_Q_11SSIF     MSG_PIPES_PATH "RcvMsgQ11SSIF"
#define EVT_MSG_Q           MSG_PIPES_PATH "EvtMsgQ"
#define PEF_RES_Q           MSG_PIPES_PATH "PEFResQ"
#define PEF_TASK_Q          MSG_PIPES_PATH "PEFTaskQ"
#define PEND_TASK_Q          MSG_PIPES_PATH "PendTaskQ"


#define KCS1_RES_Q          MSG_PIPES_PATH "KCS1ResQ"
#define KCS2_RES_Q          MSG_PIPES_PATH "KCS2ResQ"
#define KCS3_RES_Q          MSG_PIPES_PATH "KCS3ResQ"
#define IPMB_PRIMARY_IFC_Q  MSG_PIPES_PATH "IPMBPrimaryIfcQ"
#define IPMB_PRIMARY_RES_Q  MSG_PIPES_PATH "IPMBPrimaryResQ"
#define ICMB_IFC_Q          MSG_PIPES_PATH "ICMBIfcQ"
#define ICMB_RES_Q          MSG_PIPES_PATH "ICMBResQ"
#define LAN_IFC_Q           MSG_PIPES_PATH "LANIfcQ"
#define LAN_RES_Q           MSG_PIPES_PATH "LANResQ"
#define LAN_MON_Q          MSG_PIPES_PATH "LANMonQ"
#define SERIAL_IFC_Q        MSG_PIPES_PATH "SerialIfcQ"
#define SERIAL_RES_Q        MSG_PIPES_PATH "SerialResQ"
#define SOL_IFC_Q           MSG_PIPES_PATH "SOLIfcQ"
#define CHASSIS_CTRL_Q      MSG_PIPES_PATH "ChassisCtrlQ"
#define PDK_API_Q           MSG_PIPES_PATH "PDKAPIQ"
#define USB_RES_Q           MSG_PIPES_PATH "USBResQ"
#define IPMB0_Recv_Q        MSG_PIPES_PATH "IPMB0RecvQ"
#define BT_RES_Q              MSG_PIPES_PATH "BTResQ"
#define IPMI_START_SYNC_Q MSG_PIPES_PATH "IpmiSyncQ"
#define TIMER_Q    MSG_PIPES_PATH "TimerQ"
#define LCEVTLOG_Q          MSG_PIPES_PATH "LCEvtLogQ"
#define SSIF_RES_Q          MSG_PIPES_PATH "SSIFResQ"


#define SM_HNDLR_Q		MSG_PIPES_PATH "SM_Hndlr_Q"
#define VLAN_IFC_Q 		MSG_PIPES_PATH "VLAN_Info_Notify_Q"
#define UDS_IFC_Q     MSG_PIPES_PATH "UDS_IFC_Q"
#define UDS_RES_Q     MSG_PIPES_PATH "UDS_RES_Q"
#define SOCKET_PATH     MSG_PIPES_PATH "UDSocket"
#define OP_STATE_Q          MSG_PIPES_PATH "OpStateQ"
/*CMM OBSM Queue*/
#define OBSM_TASK_Q				MSG_PIPES_PATH "OBSMTaskQ"
#define OBSM_TASK_RES_Q			MSG_PIPES_PATH "OBSMTaskResQ"
#define OBSM_PWR_TASK_Q			MSG_PIPES_PATH "OBSMPwrTaskQ"
#define OBSM_COOLING_TASK_Q		MSG_PIPES_PATH "OBSMCoolingTaskQ"
#define OBSM_COOLING_TASK_RES_Q	MSG_PIPES_PATH "OBSMCoolingTaskResQ"
/*CMM OBSM TASK ID*/
#define OBSM_COOLING_TASK_ID	14
#define OBSM_TASK_ID			12
#define OBSM_PWR_TASK_ID		13

/**
 * Shared memory access key.
**/
#define BMC_SHARED_MEM              0x01
#define NVR_SHARED_MEM              0x02
#define SENSOR_SHARED_MEM           0x05

/**
 * Semaphore access Key
**/
#define PET_ACK_MANAGER_SEM_KEY     0x01

/**
 * @defgroup qs IPMI Message Queues
 * IPMI message queues constitute the communication model between
 * various tasks in the software.
 * @{
**/

#if SUPPORT_IPMB0_IFC == 1
/**
 * @var _FAR_ HQueue_T hIPMBFaultRecov_Q
 * @brief IPMB Fault recovery queue.
**/
extern _FAR_ HQueue_T      hIPMBFaultRecov_Q;
#endif


extern int BMCInstCount;

#define THREAD_TIMER_INTERVAL       10
#define THREAD_STATE_MONITOR_NO     0
#define THREAD_STATE_MONITOR_YES    1
#define THREAD_ID_FOUND             1
#define THREAD_ID_NOT_FOUND         0
#define MAX_THREAD_MON_CNT          150

typedef struct
{
    char monitor; /* Flag to indicate whether monitoring is needed or not */
    int time;	  /* No. of seconds that have elapsed since the start of the monitoring  */
    char validThreadID; /* Flag to check whether the thread is found/valid or not */
    int BMCInst; /*BMC instance related to the thread, if common for all instances then store 0*/
    char threadName[30]; /*Stores thread name*/
}ThreadMonitor;

typedef struct
{
    INT32U threadID; // thread ID of the timed out thread
    int BMCInst; //BMC instance related to the timed out thread, if common for all instances then store 0
    char threadName[30]; //thread name
    INT32U timeStamp; //time stamp of the incident
}ThreadMonitorRestartInfo;


/**
 * @brief MsgHndlr Task when used as a thread.
**/
extern void *MsgHndlr (void *pArg);

extern void *FlushIPMIToIni(void *pArg);

/**
 * @brief Chassis control Task when used as a thread.
**/
extern  void* ChassisTask (void *pArg);
/**
 * @brief Chassi Timer Task when used as a thread.
**/
extern  void* ChassisTimer (void *pArg);

/**
 * @brief IPMBIfc Task when used as a thread.
**/
extern void *IPMBIfcTask (void *Addr);

/**
 * @brief KCSIfc Task when used as a thread.
**/
extern void *KCSIfcTask (void *Addr);


/**
 * @brief SOLIfc Task when used as a thread.
**/
extern void *SOLIfcTask (void *Addr);

/**
 * @brief USBIfc Task when used as a thread.
**/
extern void *USBIfcTask (void *Addr);


/**
 * @brief SerialIfc Task when used as a thread.
**/
extern void SerialIfcTask (void *Addr);

/**
 * @brief Timer Task when used as a thread.
**/
extern void *TimerTask (void *pArg);

/**
 * @brief Host Reset Function when used as a thread.
**/
extern void *Host_Reset_function(void *pArg);

#pragma pack( )

/** @} */

#endif /* IPMI_MAIN_H */
