/*****************************************************************
 *****************************************************************
 **                                                             **
 **     (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                             **
 **             All Rights Reserved.                            **
 **                                                             **
 **         6145-F, Northbelt Parkway, Norcross,                **
 **                                                             **
 **         Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                             **
 *****************************************************************
 *****************************************************************
 ******************************************************************
 ******************************************************************
 *
 * AMIDevice.c
 *
 * Author: Basavaraj Astekar <basavaraja@ami.com>
 *
 ******************************************************************/
#define ENABLE_DEBUG_MACROS     0

#include <stdio.h>
#include <unistd.h>
#include <linux/kernel.h>
#include <sys/sysinfo.h>
#include <dlfcn.h>
#include <openssl/pem.h>
#include <sys/socket.h>
#include <netdb.h>

#include "Types.h"
#include "OSPort.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "IPMI_AppDevice.h"
#include "IPMI_AMIDevice.h"
#include "IPMI_AMI.h"
#include "AMIDevice.h"
#include "SharedMem.h"
#include "dbgout.h"
#include "ipmi_userifc.h"
//#include "sshutil.h"
#include "Platform.h"
#include "Indicators.h"
#include "Debug.h"
#include "flashlib.h"
#include "NVRAccess.h"
#include "MsgHndlr.h"
#include "PMConfig.h"
#include "YafuStatus.h"
#include "Sensor.h"
#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include "PDKCmds.h"
#include "IPMIConf.h"
#include "PDKAccess.h"
#include "Ethaddr.h"
#include "PDKCmdsAccess.h"
#include "flshfiles.h"
#include "PendTask.h"
#include "IPMI_AMIConf.h"
#include "featuredef.h"
#include<sys/prctl.h>
#include "ubenv.h"
#include "fwinfo.h"
#include "blowfish.h"
//#include "ldapconf.h"
//#include "activedir_cfg.h"
#include "ubenv.h"
#include "safesystem.h"
#include "usb_ioctl.h"
#include "vmedia_instance.h"
#include "ncml.h"
#include "versionmgt.h"
#include "Util.h"
#include "racsessioninfo.h"

#include <sys/stat.h>
#include <string.h>
#include "IPMI_AMILicense.h"
#include "CyoEncode.h"
#include "Encode.h"

//For enabling/Disabling continuous recording to record pre-crash/pre-reset video
#define START_CONT_RCD 0x5
#define STOP_CONT_RCD 0x6
//static  _FAR_ SensorSharedMem_T*    pSenSharedMem; 

/* For BIOS flashing
 * see firmware/apps/gpio_utilities
 * included by way of libami/flashlib
 */
extern int assert_host_reset();
extern int write_one_bios_block( int blocknum, unsigned char *data );
extern int read_one_bios_block( int blocknum, unsigned char *data );
extern int deassert_host_reset();
extern unsigned long CalculateChksum (char *data, unsigned long size);
void*  EraseCopyFlash	  (void*);
void*  VerifyFlashStatus	  (void*);
void* YAFUTimerTask(void *pArg);
int YAFUTimerAction();

/* For Firmware update
 */
//#define INVALID_IP_ADDR "0.0.0.0"
//#define NO_ADDRESS 0xFFFFFFFF
#define FLASH_TIMEOUT_IN_SECONDS 1200
//#define SET_FIRM_REQ_BYTES 200

/* U-Boot Memory Test */
#define UBOOT_MEMTEST_ENABLE_VAR    "do_memtest"
#define UBOOT_MEMTEST_STATUS_VAR    "memtest_pass"
#define MEMTEST_PASS    "yes"
#define MEMTEST_FAIL    "no"
#define MEMTEST_ENABLE  "1"

#define MAX_IPV6_COUNT  8

FlashTimerInfo_T	g_FlashTimerInfo;

#define MEMGETINFO              _IOR('M', 1, struct mtd_info_user)
#define MEMERASE                _IOW('M', 2, struct erase_info_user)
#define MEMLOCK                 _IOW('M', 5, struct erase_info_user)
#define MEMUNLOCK               _IOW('M', 6, struct erase_info_user)

#define MAX_CMDLINE	50
#define MAX_MODULE			16


#define INVALID_IP_ADDR "0.0.0.0"
#define SET_FIRM_REQ_BYTES 200

/* YafuTimer Related definitions*/
#define UBOOT_MAX_SIZE	65536
/* Timer timeouts for diff interfaces and flasing types(Interactive and ForceFlash)*/
#define YAFU_LAN_IA_TIMER_COUNT	3600
#define YAFU_LAN_FF_TIMER_COUNT	3600
#define YAFU_USB_IA_TIMER_COUNT	180
#define YAFU_USB_FF_TIMER_COUNT	1800
#define YAFU_KCS_IA_TIMER_COUNT	5400
#define YAFU_KCS_FF_TIMER_COUNT	5400

#define YAFU_MSECS_PER_TICK		1000
#define YAFU_FLASH_SUCCEEDED	1
#define YAFU_FLASH_CHUNK	1024

/* Yafu Flashing Types Interactive/Generic*/
#define YAFU_INTERACTIVE_FLASH	1
#define BMCNODE			0 //To support backward compatability

/* Virtual Device Status*/
#define GET_VIRTUAL_DEVICE_STATUS		1
#define SET_VIRTUAL_DEVICE_STATUS		2


#ifndef CONFIG_SPX_FEATURE_IPMI_NO_YAFU_SUPPORT

#define DEFAULT_MTD_DEVICE	"/dev/mtd0"
#define SIGNED_HASH_SIZE    128
//#define SYSLOG_RESTART "/etc/init.d/rsyslog restart"

/*Firewall conf file*/

BOOL  FMHComp;
BOOL ActivateFlashStatus =0x00;
int LastStatCode = 0x00;
int VerifyFlashStatusCode = 0x00;
INT8U *allocmem;
int DualImgPreserveConf=TRUE;
INT8U *WriteMemOff=NULL;
unsigned long Sizetocpy=0;
unsigned long Flashoffset;
unsigned long Fixoffset;
INT8U *VWriteMemOff=NULL;
INT32U ImgSize = 0;
unsigned long Sizetoverify;
unsigned long VFlashoffset;
unsigned long VOffset;
unsigned long VFixoffset;
int gDeviceNode = 1;
INT8U PreserveFlag=0;
static int session_count = 0;

/* Yafu Timer relating variables*/
static env_t environment;
unsigned long YafuTimerCnt = 0;
char UBootCfg[UBOOT_MAX_SIZE];
int YafuflashStatus = 0;
int SetBootParam = 0;
extern unsigned long  FlashStart, EnvStart, EnvSize;
int YafuFlashMode=0;

int Vstart=0;
int Vthreadstate=0;

int ECFstart=0;
int ECFthreadstate=0;
pthread_t             threadid, threadid_yafu;
pthread_t             threadidv; /*Verify Flash*/

pthread_mutex_t     YafuThreadMutex;
pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      condv  = PTHREAD_COND_INITIALIZER; /*Verify Flash*/
pthread_mutex_t     mutexv = PTHREAD_MUTEX_INITIALIZER; /*Verify Flash*/


struct erase_info_user {
        uint32_t start;
        uint32_t length;
};


struct mtd_info_user{
    unsigned char type;
    unsigned long flags;
    unsigned long size;
    unsigned long erasesize;
    unsigned long oobblock;
    unsigned long oobsize;
    unsigned long ecctype;
    unsigned long eccsize;
};


/* Initialize the Global structure with default MTD information */
AMIYAFUSwitchFlashDeviceRes_T gAMIYAFUSwitchFlashDevice;

#endif

static const INT8U m_DualResBits [] = {2,1,2,1,2,1,1,2};

extern IfcName_T Ifcnametable[MAX_LAN_CHANNELS];

static const INT8U TriggerEventParamLength[] ={
     1,    /*Length for Critical Flag*/
     1,    /*Length for Non Critical Flag*/
     1,    /*Length for Non Recoverable Flag*/
     1,    /*Length for Fan Troubled Flag*/
     1,    /*Length for WDT Expire Flag*/
     1,    /*Length for System DC On Flag*/
     1,    /*Length for System DC Off Flag*/
     1,    /*Length for System DC Reset Flag*/
     1+4,  /*Length for Specific date time Flag*/
     1,    /*Length for LPC Reset Flag*/
     1,    /*Length for Pre crash/reset video recording Flag*/
};

KeyPath_T PubKeyPathInfo[] = {
    {SINGED_IMAGE, SINGED_IMAGE_KEY_PATH},
    {BUNDLE_IMAGE, BUNDLE_KEY_PATH},
};


char  g_FlashingImage;
char  g_RunningImage;
char  g_InactiveImage;
static unsigned char g_FWBootSelector = 0xFF;
char g_rebootstatus=TRUE;
#define CONF_FWVERSION_FILE        "/conf/Fwversion"
#define EVENT_MSG_LENGTH            9

#ifndef CONFIG_SPX_FEATURE_IPMI_NO_YAFU_SUPPORT
int InitYafuVar()
{
	gAMIYAFUSwitchFlashDevice.CompletionCode = 0x0;
	strcpy(gAMIYAFUSwitchFlashDevice.MTDName,DEFAULT_MTD_DEVICE);
	gAMIYAFUSwitchFlashDevice.FlashInfo.FlashSize = g_coremacros.global_used_flash_size;
	gAMIYAFUSwitchFlashDevice.FlashInfo.FlashAddress = g_coremacros.global_used_flash_start;
	gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize = g_coremacros.global_erase_blk_size;
	gAMIYAFUSwitchFlashDevice.FlashInfo.FlashProductID = 0;
	gAMIYAFUSwitchFlashDevice.FlashInfo.FlashWidth = 8;
	gAMIYAFUSwitchFlashDevice.FlashInfo.FMHCompliance = 1;
	gAMIYAFUSwitchFlashDevice.FlashInfo.Reserved = 0;
	gAMIYAFUSwitchFlashDevice.FlashInfo.NoEraseBlks = 0;

	return 0;
}
#endif

//static int Decrypt_LicenseKey(unsigned char *LicenseKey,int size, char *DecrypteLicenseKey);
//static int IsDuplicateKey(unsigned char *LicenseKey);

/**
 * FlashTimerTask
 **/
void
FlashTimerTask (int BMCInst)
{
    static int m_InitFlashVars = 0;

    if (0 == m_InitFlashVars)
    {
        m_InitFlashVars 					 = 1;
        g_FlashTimerInfo.TimeOut 			 = 0;
        g_FlashTimerInfo.FlashProcessStarted = 0;
    }

    if (0 == g_FlashTimerInfo.FlashProcessStarted)
    {
        g_FlashTimerInfo.TimeOut 			 = 0;
        return;
    }

    if (0 == g_FlashTimerInfo.TimeOut)
    {
        IPMI_DBG_PRINT ("Flashing Process Timed out \n");
        g_FlashTimerInfo.FlashProcessStarted = 0;
        reboot (LINUX_REBOOT_CMD_RESTART);
    }
    g_FlashTimerInfo.TimeOut--;

}

/**
 ** RestartBMC Via Flasher
 **/
void RestartBMC_Flasher()
{
    int cmdpipe = -1;
    FlasherCmd Cmd;
    if (-1 == (cmdpipe = open (FLASHER_PIPE, O_WRONLY)))
    {
        perror ("Error while opening fifo");
        TCRIT ("Failed");
    }

    Cmd.Command = FLSH_CMD_ABORT_FLASH;
    Cmd.Data[0] = RESET_BMC;
    if (sizeof (FlasherCmd) != write (cmdpipe, &Cmd, sizeof (FlasherCmd)))
    {
        perror ("Error writing Prep command");
        TCRIT ("Failed");
    }

    close(cmdpipe);
}

/**
 ** @fn DefaultSettingsForDualImageSupport
 ** @brief  To set the flashing image as inactive image defaultly.
 ** @param[in] - void.
 ** @retval   Will return 0 on success.
 **
 **/
int DefaultSettingsForDualImageSupport(int BMCInst)
{
    DUALIMGINFO_STRUCT info;
    char envval[5] = {0};
    int bootselector;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    
    if( GetDualImageOptions(&info) == -1)
    {
        TCRIT("Unable to get GetDualImageOptions");
        return -1;
    }

    /* running and inactive images are remain same till next reboot*/
    g_RunningImage  = info.runningImg;
    g_InactiveImage = info.InactiveImg;

    //default upload image
    g_FlashingImage = pBMCInfo->DualImageCfg.FwUploadSelector;
    if(g_FlashingImage == AUTO_INACTIVE_IMAGE)
    {
        g_FlashingImage = info.InactiveImg;
    }
    

    if( GetUBootParam("bootselector",envval) == 0 )
    {
        sscanf(envval,"%d",&bootselector);
        //Checking for valid Boot selector option
        if( CheckForBootOption(bootselector) == 0)
        {
            g_FWBootSelector = bootselector;
        }
    }
    SetFwUploadSelector(pBMCInfo->DualImageCfg.FwUploadSelector);
    return 0;
}


/**
 * @fn SetMostRecentlyProgFW
 * @brief Sets the most recently programmed firmware image to env variable recentlyprogfw.
 * @param[in]     - Nill
 * @retval      0 - on success.
 *             -1 - on failure case
 */
int SetMostRecentlyProgFW(void)
{
    char envval[5];

    memset(envval,0,sizeof(envval));

    if(( g_FlashingImage == IMAGE_1) || (g_FlashingImage == IMAGE_BOTH)) {

        sprintf(envval,"%d",IMAGE_1);
    }
    else if( g_FlashingImage == IMAGE_2) {

        sprintf(envval,"%d",IMAGE_2);
    }
    else {

        return -1;
    }

    if( SetUBootParam("recentlyprogfw",envval) != 0 )
    {
        TCRIT("Unable to set env variable updation\n");
        return -1;
    }

return 0;
}

#ifndef CONFIG_SPX_FEATURE_IPMI_NO_YAFU_SUPPORT
int Get_Cuurent_Bios_Img_Info(FlashDetails *biosinfo, char *MTD_Device)
{
    struct mtd_info_user user_info;
    int retval = -1;
    int MTDDev = 0;
    MTDDev = open (MTD_Device, O_RDONLY);
    if (MTDDev < 0)
    {
        TCRIT ("Cannot open mtd raw device %s. Exiting...\n", MTD_Device);
        return retval;
    }
    memset(&user_info, 0x00, sizeof(struct mtd_info_user));
    if (ioctl(MTDDev, MEMGETINFO, &user_info) < 0)
    {
        printf("Unable to get MTD info via IOCTL(MEMGETINFO) ... %s\n", strerror(errno));
        close(MTDDev);
        return retval;
    }
    biosinfo->FlashSize= user_info.size;
    biosinfo->FlashAddress =  CONFIG_SPX_BIOS_FLASH_START ;
    biosinfo->FlashEraseBlkSize = user_info.erasesize ;
    retval = 0;
    close(MTDDev);
    return retval;
}

int Default_SwitchBIOS_Settings(AMIYAFUSwitchFlashDeviceRes_T *pAMIYAFUSwitchFlashDevice_Res)
{
    char MTD_Device[128];
    FlashDetails biosinfo;
    ActivateFlashStatus = 0x00;

    StartInitFlashSetting();

    //find the MTD number for HOST SPI
    if (FindHostBiosDeviceName(MTD_Device) != 0)
        return -1;

    strcpy((char *)&pAMIYAFUSwitchFlashDevice_Res->MTDName[0], MTD_Device);
    if(Get_Cuurent_Bios_Img_Info(&biosinfo, MTD_Device) < 0)
        return -1;

     pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FlashSize = biosinfo.FlashSize;
     pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FlashAddress = biosinfo.FlashAddress;
     pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FlashEraseBlkSize = biosinfo.FlashEraseBlkSize;

     return 0;

}

int Default_SwitchBMC_Settings(AMIYAFUSwitchFlashDeviceRes_T *pAMIYAFUSwitchFlashDevice_Res)
{
    strcpy((char *)&pAMIYAFUSwitchFlashDevice_Res->MTDName[0], (char *)DEFAULT_MTD_DEVICE);
    pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FlashSize = g_coremacros.global_flash_size;
    pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FlashAddress = g_coremacros.global_used_flash_start;
    pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FlashEraseBlkSize = g_coremacros.global_erase_blk_size;
    pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FMHCompliance = 0x01;

    return 0;
}

int Default_SwitchCPLD_Settings(AMIYAFUSwitchFlashDeviceRes_T *pAMIYAFUSwitchFlashDevice_Res)
{
	  char MTD_Device[128]="/dev/jtag0";
    strcpy((char *)&pAMIYAFUSwitchFlashDevice_Res->MTDName[0], (char *)MTD_Device);
    pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FlashSize = g_coremacros.global_used_flash_size;
    pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FlashAddress = g_coremacros.global_used_flash_start;
    pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FlashEraseBlkSize = g_coremacros.global_erase_blk_size;
    pAMIYAFUSwitchFlashDevice_Res->FlashInfo.FMHCompliance = 0x01;

    return 0;
}

int
Switch_Device (int SPIDevice, INT8U *pRes)
{
    AMIYAFUSwitchFlashDeviceRes_T *pAMIYAFUSwitchFlashDevice_Res = (AMIYAFUSwitchFlashDeviceRes_T *)pRes;
    int retval = -1;
    unsigned long ulIDCode = 0;

    memset((char *)pAMIYAFUSwitchFlashDevice_Res, 0, sizeof(AMIYAFUSwitchFlashDeviceRes_T));

    switch(SPIDevice)
    {
       case BIT0://BMC
       case BMCNODE:
           retval=Default_SwitchBMC_Settings(pAMIYAFUSwitchFlashDevice_Res);
           if (g_PDKHandle[PDK_SWITCH_SPI] != NULL)
           {
                   retval = ((int(*)(int,char *,char,INT8U*))g_PDKHandle[PDK_SWITCH_SPI]) (SPIDevice,&g_FlashingImage,g_InactiveImage,pRes);
                   if (retval != 0)
                   {
                           TCRIT("Unable to identify the SPI Partition from the MTD Mapping\n");
                           goto exit_gracefully;
                   }
           }
           break;
       case BIT1://BIOS
           retval = Default_SwitchBIOS_Settings(pAMIYAFUSwitchFlashDevice_Res);
           if (g_PDKHandle[PDK_SWITCH_SPI] != NULL)
           {
                   retval = ((int(*)(int,char *,char,INT8U*))g_PDKHandle[PDK_SWITCH_SPI]) (SPIDevice,&g_FlashingImage,g_InactiveImage,pRes);
                   if (retval != 0)
                   {
                           TCRIT("Unable to identify the SPI Partition from the MTD Mapping\n");
                           goto exit_gracefully;
                   }
           }
           break;
       case BIT2://CPLD
           retval = Default_SwitchCPLD_Settings(pAMIYAFUSwitchFlashDevice_Res);
           if (g_PDKHandle[PDK_GETCPLDIDCODE] != NULL)
           {
                   retval = ((int(*)(unsigned long*))g_PDKHandle[PDK_GETCPLDIDCODE]) (&ulIDCode);
                   if (retval == -1)
                   {
                           TCRIT("Unable to identify the CPLD IDCode\n");
                           goto exit_gracefully;
                   }
                   else
                   {
                   	if(access("/var/yafu_cpld_update_selection", F_OK) != 0)
                   		safe_system("touch /var/yafu_cpld_update_selection");
                   	retval = 0;
                   }
           }
           else
           	retval = -1;
           break;
       default:
           TCRIT("Invalid SPI Device Node.\n");
           break;
    }
exit_gracefully:
	return retval;
} 
	
/*---------------------------------------
 * AMIYAFUSwitchFlashDevice
 *---------------------------------------*/
int AMIYAFUSwitchFlashDevice(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{
	AMIYAFUSwitchFlashDeviceReq_T *pAMIYAFUSwitchFlashDevice_Req = (AMIYAFUSwitchFlashDeviceReq_T *)pReq;
	AMIYAFUSwitchFlashDeviceRes_T *pAMIYAFUSwitchFlashDevice_Res = (AMIYAFUSwitchFlashDeviceRes_T *)pRes;
	int SPIDevice = (int)(pAMIYAFUSwitchFlashDevice_Req->SPIDevice);
	int ErrCode = 0;
	
	ErrCode = Switch_Device(SPIDevice, (INT8U *)pAMIYAFUSwitchFlashDevice_Res);
	if (ErrCode != 0)
	{
		TCRIT("Unable to Switch to the appropriate SPI Device\n");
		pAMIYAFUSwitchFlashDevice_Res->CompletionCode = YAFU_CC_READ_ERR;
		return sizeof(INT8U);
	}

	pAMIYAFUSwitchFlashDevice_Res->CompletionCode = CC_NORMAL;

	memcpy((char *)&gAMIYAFUSwitchFlashDevice, (char *)pAMIYAFUSwitchFlashDevice_Res, sizeof(AMIYAFUSwitchFlashDeviceRes_T));
	gDeviceNode = SPIDevice;

	return (sizeof(AMIYAFUSwitchFlashDeviceRes_T));
}

/*---------------------------------------
 * AMIYAFUActivateFlashDevice
 *---------------------------------------*/
int AMIYAFUActivateFlashDevice(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{
    AMIYAFUActivateFlashDeviceReq_T *pAMIYAFUActivateFlashDevice_Req = (AMIYAFUActivateFlashDeviceReq_T *)pReq;
    AMIYAFUActivateFlashDeviceRes_T *pAMIYAFUActivateFlashDevice_Res = (AMIYAFUActivateFlashDeviceRes_T *)pRes;
    INT8U ActivateNode =  pAMIYAFUActivateFlashDevice_Req->ActivateNode;
    int ErrCode = 0;
    int RetCode = 0;
	
    if(g_PDKHandle[PDK_ACTIVATE_SPI] != NULL)
    {
        ErrCode = ((int(*)(int,INT8U))g_PDKHandle[PDK_ACTIVATE_SPI]) (BMCInst,ActivateNode);
       /* Control will not come here at all, if BMC is flashed & activated */
        /* If CPLD is flashed & activated, then re-program/re-configure will be done */
        if (ErrCode != 0)
        {
                /* If ErrCode is > 0 (errors returned from JBC Player), then treat as failure in CPLD configuration */
                if (ErrCode > 0)
                {
                        /*CPLDStatCode = RetCode = YAFU_CC_IN_DEACTIVATE;*/
                        RetCode = YAFU_CC_IN_DEACTIVATE;
                        TCRIT("AMIYAFUActivateFlashDevice CPLD configure failed\n");
                }
                /* If ErrCode is -2, then it means, we need to initiate BMC Reboot using YAFU Command */
                else if (ErrCode == -2)
                {
	                TCRIT("AMIYAFUActivateFlashDevice Rebooting BMC\n");
	                RestartBMC_Flasher();
                }
                /* If ErrCode is -3, then it means, flashing/activation is already in progress */
                else if (ErrCode == -3)
                {
                        RetCode = YAFU_CC_DEV_OPEN_ERR;
                        TCRIT("AMIYAFUActivateFlashDevice Flashing/Activation in progress\n");
                }
                /* If ErrCode is < 0 (as returned from PDK Hook, other than -2), then treat as failure in BMC/BIOS Activation */
                else
                {
                        RetCode = YAFU_FLASH_ERASE_FAILURE;
                        printf("AMIYAFUActivateFlashDevice Activating the upgraded Flash devices Failed");
                        TCRIT("Unable to activate the flash device.\n");
                }
        }
        else
        {
            /*CPLDStatCode = RetCode = YAFU_CC_NORMAL; */
           RetCode = YAFU_CC_NORMAL;

            if (ActivateFlashStatus == 1)
                    ActivateFlashStatus = 0;
        }
    }
    else
    {
        RetCode = YAFU_NOT_SUPPORTED;
        if(ActivateNode & BIT2) //CPLD
        {
            // Need to handle for CPLD case.
        }

        if(ActivateNode & BIT1) //BIOS
        {
            // Need to handle for BIOS case.
        }

        if(ActivateNode & BIT0) // BMC
        {
            RetCode = 0;
            TCRIT("AMIYAFUActivateFlashDevice Rebooting BMC\n");
            RestartBMC_Flasher();
        }
    }

    pAMIYAFUActivateFlashDevice_Res->CompletionCode = RetCode;
    return sizeof(AMIYAFUActivateFlashDeviceRes_T);
}

/*---------------------------------------
 * AMIYAFURestoreFlashDevice
 *---------------------------------------*/
int AMIYAFURestoreFlashDevice(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{
    int retval = 0;
    AMIYAFUSwitchFlashDeviceRes_T *pAMIYAFUSwitchFlashDevice_Res = (AMIYAFUSwitchFlashDeviceRes_T *)pRes;
    AMIYAFUResetDeviceReq_T *pAMIYAFUResetDeviceReq = (AMIYAFUResetDeviceReq_T *)pReq;
    YafuflashStatus = YAFU_FLASH_SUCCEEDED;

    AMISetCacheVersion(gDeviceNode);

    if (g_PDKHandle[PDK_REVERT_SPI] != NULL)
    {
       	retval = ((int(*)(INT8U))g_PDKHandle[PDK_REVERT_SPI]) (PreserveFlag);
	if (retval != 0) 
	{
		TCRIT("Unable to clear the flash mapping information\n");
		pAMIYAFUSwitchFlashDevice_Res->CompletionCode = YAFU_CC_READ_ERR;
		return sizeof(INT8U);
	}
    }

    if(gDeviceNode == BIT0 || gDeviceNode == 0x00)
    {
        if(g_corefeatures.preserve_config == ENABLED)
        {
            /* If conf is flashed then call presercfg application */
            if((gDeviceNode == BIT0 && (PreserveFlag&BIT1) == 0x00) ||
               (gDeviceNode == 0x00 && pAMIYAFUResetDeviceReq->WaitSec == 0x02))
            {
                TINFO("Trying to preserve config on full flash using preservecfg application");
                safe_system("/usr/local/bin/preservecfg 2");
            }
        }
    }

    if(g_corefeatures.extendedlog_support == ENABLED && g_corefeatures.extlog_medium_type_sd == ENABLED)
    {
       /* if extlog is flashed and medium type is sd card then remove all the content from extlog */
       if(((PreserveFlag&BIT2) == 0x00) && (gDeviceNode == BIT0 || gDeviceNode == 0x00))
	{

          safe_system("rm -f /extlog/*");
          sync();
	}
    }

    pAMIYAFUSwitchFlashDevice_Res->CompletionCode = CC_NORMAL;

    return (sizeof(AMIYAFUSwitchFlashDeviceRes_T));
}

/*---------------------------------------
 * AMIYAFUNotAcks
 *---------------------------------------*/

int AMIYAFUNotAcks(INT8U *pRes,INT16U ErrorCode,INT32U Seqnum)
{
    AMIYAFUNotAck* pAMIYAFUNotAcknowledge =(AMIYAFUNotAck*)pRes;
    pAMIYAFUNotAcknowledge->CompletionCode = YAFU_ERR_STATE;
    pAMIYAFUNotAcknowledge->NotAck.Seqnum = Seqnum;
    pAMIYAFUNotAcknowledge->NotAck.YafuCmd = CMD_AMI_YAFU_COMMON_NAK;
    pAMIYAFUNotAcknowledge->NotAck.Datalen = 0x02;
    pAMIYAFUNotAcknowledge->NotAck.CRC32chksum = 0x00;
    pAMIYAFUNotAcknowledge->ErrorCode = ErrorCode;
    return (sizeof(AMIYAFUNotAck));
}

/*---------------------------------------
 * AMIYAFUGetFlashInfo
 *---------------------------------------*/
int AMIYAFUGetFlashInfo ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{
       AMIYAFUGetFlashInfoReq_T *pAMIYAFUFlashInfoReq = (AMIYAFUGetFlashInfoReq_T *)pReq;
       AMIYAFUGetFlashInfoRes_T* pAMIYAFUGetFlashInfo = (AMIYAFUGetFlashInfoRes_T*)pRes;

	pAMIYAFUGetFlashInfo->CompletionCode = YAFU_CC_NORMAL;
       pAMIYAFUGetFlashInfo->FlashInfoRes.Seqnum = pAMIYAFUFlashInfoReq->FlashInfoReq.Seqnum;
	pAMIYAFUGetFlashInfo->FlashInfoRes.YafuCmd= pAMIYAFUFlashInfoReq->FlashInfoReq.YafuCmd;

	pAMIYAFUGetFlashInfo->FlashInfo.FlashSize = g_coremacros.global_flash_size;
	pAMIYAFUGetFlashInfo->FlashInfo.FlashAddress = gAMIYAFUSwitchFlashDevice.FlashInfo.FlashAddress;
	pAMIYAFUGetFlashInfo->FlashInfo.FlashEraseBlkSize = gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize;
	pAMIYAFUGetFlashInfo->FlashInfo.FlashProductID = 0;
	pAMIYAFUGetFlashInfo->FlashInfo.FlashWidth = 8;
	pAMIYAFUGetFlashInfo->FlashInfo.FMHCompliance = gAMIYAFUSwitchFlashDevice.FlashInfo.FMHCompliance;

       if(pAMIYAFUGetFlashInfo->FlashInfo.FMHCompliance == 0x01)
	   	FMHComp = 1;

      	pAMIYAFUGetFlashInfo->FlashInfo.Reserved = 0;
	pAMIYAFUGetFlashInfo->FlashInfo.NoEraseBlks = ((gAMIYAFUSwitchFlashDevice.FlashInfo.FlashSize)/(gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize));

       if((pAMIYAFUGetFlashInfo->FlashInfo.NoEraseBlks * pAMIYAFUGetFlashInfo->FlashInfo.FlashEraseBlkSize) == pAMIYAFUGetFlashInfo->FlashInfo.FlashSize)
             pAMIYAFUGetFlashInfo->FlashInfoRes.Datalen= 0x20;
       else
             pAMIYAFUGetFlashInfo->FlashInfoRes.Datalen = 0x20 + pAMIYAFUGetFlashInfo->FlashInfo.NoEraseBlks;

      pAMIYAFUGetFlashInfo->FlashInfoRes.CRC32chksum = CalculateChksum((char *)&pAMIYAFUGetFlashInfo->FlashInfo,sizeof(pAMIYAFUGetFlashInfo->FlashInfo));

       return( sizeof( AMIYAFUGetFlashInfoRes_T ) );

}

/*---------------------------------------
 * AMIYAFUGetFirmwareInfo
 *---------------------------------------*/
#define FW_INFO_FILE "/proc/ractrends/Helper/FwInfo"
static void GetFirmwareVersion(unsigned int* Major,unsigned int* Minor,unsigned int* Rev,unsigned int* ProductID)
{
    char aline[100];
    int AuxVer = 0;  
    int i = 0, count = 0, ret = 0;

    FILE* fp = fopen(FW_INFO_FILE,"rb");
	if(fp == NULL)
	{
		TCRIT("Unable to find firmware version info!!!\n");
		*Major = 0;
		*Minor = 0;
		*Rev = 0;
		return ;
	}


	fgets(aline,93,fp);

    for(i = 0; i < 93; i++) 
    {
        if (aline[i] == '\0')
            break;
        if (aline[i] == '.')
            ++count;
    }

    if(count == 3)
        sscanf(aline,"FW_VERSION=%d.%d.%d.%d",Major,Minor,&AuxVer,Rev);
    else
        sscanf(aline,"FW_VERSION=%d.%d.%d",Major,Minor,Rev);   

    while(fgets(aline,93,fp) != NULL)
    {
        ret = sscanf(aline,"FW_PRODUCTID=%d",ProductID);
        if (ret == 1)
            break;
    } 

	fclose(fp);

return;
}


int AMIYAFUGetFirmwareInfo ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )  //Added by winston for YAFU */
{
       static unsigned int Major,Minor,Rev,ProductID;
//	static unsigned char MinorBCD;

       AMIYAFUGetFirmwareInfoReq_T *pAMIYAFUGetFirmwareInfoReq = (AMIYAFUGetFirmwareInfoReq_T *)pReq;
       AMIYAFUGetFirmwareInfoRes_T* pAMIYAFUGetFirmwareInfo = (AMIYAFUGetFirmwareInfoRes_T*)pRes;
       static int firsttime = 1;

	if(firsttime == 1)
	{
		GetFirmwareVersion(&Major,&Minor,&Rev,&ProductID);
        //	MinorBCD = ((Minor/10)<<4)+(Minor%10);
		firsttime = 0;
	}

      
       pAMIYAFUGetFirmwareInfo->CompletionCode = CC_NORMAL;
	pAMIYAFUGetFirmwareInfo->FirmwareinfoRes.Seqnum = pAMIYAFUGetFirmwareInfoReq->FirmwareinfoReq.Seqnum;
	pAMIYAFUGetFirmwareInfo->FirmwareinfoRes.YafuCmd= pAMIYAFUGetFirmwareInfoReq->FirmwareinfoReq.YafuCmd;
	pAMIYAFUGetFirmwareInfo->FirmwareinfoRes.Datalen= 0x26;
	pAMIYAFUGetFirmwareInfo->FirmwareinfoRes.CRC32chksum = 0x00;
	pAMIYAFUGetFirmwareInfo->FirmwareInfo.FirmMajVersion =  (INT8U)Major;
	pAMIYAFUGetFirmwareInfo->FirmwareInfo.FirmMinVersion = (INT8U)Minor;
	pAMIYAFUGetFirmwareInfo->FirmwareInfo.FirmAuxVersion = Rev;
	pAMIYAFUGetFirmwareInfo->FirmwareInfo.FirmBuildNum = 0;
	pAMIYAFUGetFirmwareInfo->FirmwareInfo.Reserved = 0x00;
	strcpy((char *)pAMIYAFUGetFirmwareInfo->FirmwareInfo.FirmwareName,"Rom.ima");
	pAMIYAFUGetFirmwareInfo->FirmwareInfo.FirmwareSize = gAMIYAFUSwitchFlashDevice.FlashInfo.FlashSize;
	pAMIYAFUGetFirmwareInfo->FirmwareInfo.ProductID = (INT32U)ProductID;
       pAMIYAFUGetFirmwareInfo->FirmwareinfoRes.CRC32chksum = CalculateChksum((char *)&pAMIYAFUGetFirmwareInfo->FirmwareInfo,sizeof(pAMIYAFUGetFirmwareInfo->FirmwareInfo));

       return( sizeof( AMIYAFUGetFirmwareInfoRes_T ) );
}

/*---------------------------------------
 *	AMIYAFUGetFMHInfo
 *---------------------------------------*/
int AMIYAFUGetFMHInfo ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{
        INT8U *FMHDetails = NULL;
        AMIYAFUGetFMHInfoReq_T *pAMIYAFUGetFMHInfoReq = (AMIYAFUGetFMHInfoReq_T *)pReq;
        AMIYAFUGetFMHInfoRes_T* pAMIYAFUGetFMHInfo = (AMIYAFUGetFMHInfoRes_T*)pRes;
        INT8U TotalFMH = 0;
        INT16U  NoOfFMH = 0;

        pAMIYAFUGetFMHInfo->NumFMH = 0x00;

        FMHDetails = malloc(MAX_FMHLENGTH);
        if(FMHDetails == NULL)
        {
          TCRIT("Error in malloc of FMHDetails");
          return -1;
        }

        if(pAMIYAFUGetFMHInfoReq->FMHReq.Datalen != 0x00)
        {
                goto exit;
        }

        if(GetAllFMHInfo(gAMIYAFUSwitchFlashDevice.MTDName,gDeviceNode, FMHDetails, &NoOfFMH) != 0 )
        {
                goto exit;
        }
        pAMIYAFUGetFMHInfo->NumFMH =  NoOfFMH;
        TotalFMH = NoOfFMH & 0x00FF;
        TotalFMH +=  NoOfFMH>>8;

        pAMIYAFUGetFMHInfo->FMHRes.Datalen= 0x04 + (TotalFMH* 64);

        memcpy (( INT8U*) (pAMIYAFUGetFMHInfo + 1),( INT8U*)FMHDetails,(TotalFMH * 64) );

        pAMIYAFUGetFMHInfo->CompletionCode = YAFU_CC_NORMAL;
        pAMIYAFUGetFMHInfo->FMHRes.Seqnum = pAMIYAFUGetFMHInfoReq->FMHReq.Seqnum;
        pAMIYAFUGetFMHInfo->FMHRes.YafuCmd= pAMIYAFUGetFMHInfoReq->FMHReq.YafuCmd;
        pAMIYAFUGetFMHInfo->Reserved = 0x00;
        pAMIYAFUGetFMHInfo->FMHRes.CRC32chksum = CalculateChksum((char *)&pAMIYAFUGetFMHInfo->Reserved,pAMIYAFUGetFMHInfo->FMHRes.Datalen);

        if (FMHDetails != NULL)
                free (FMHDetails);
        return( sizeof( AMIYAFUGetFMHInfoRes_T ) + (TotalFMH * 64));

exit:
                pAMIYAFUGetFMHInfo->CompletionCode = CC_INV_DATA_FIELD;
                if (FMHDetails != NULL)
                {
                         free(FMHDetails);
                }
                return sizeof (*pRes);
}


/*---------------------------------------
 * AMIYAFUGetStatus
 *---------------------------------------*/
int AMIYAFUGetStatus  ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{
    AMIYAFUGetStatusReq_T *pAMIYAFUGetStatusReq =(AMIYAFUGetStatusReq_T *)pReq;
    AMIYAFUGetStatusRes_T* pAMIYAFUGetStatus = (AMIYAFUGetStatusRes_T*)pRes;


    pAMIYAFUGetStatus->GetStatusRes.Seqnum = pAMIYAFUGetStatusReq->GetStatusReq.Seqnum;
    pAMIYAFUGetStatus->GetStatusRes.YafuCmd= pAMIYAFUGetStatusReq->GetStatusReq.YafuCmd;
    pAMIYAFUGetStatus->LastStatusCode =(INT16U) LastStatCode;
    pAMIYAFUGetStatus->YAFUState = 0x00;


    pAMIYAFUGetStatus->Mode = 0x00;
    pAMIYAFUGetStatus->Reserved = 0x00;
    pAMIYAFUGetStatus->GetStatusRes.Datalen=8;
    pAMIYAFUGetStatus->Message[0] = 0;
    pAMIYAFUGetStatus->CompletionCode = YAFU_CC_NORMAL;
    pAMIYAFUGetStatus->GetStatusRes.CRC32chksum = CalculateChksum((char *)&pAMIYAFUGetStatus->LastStatusCode,(INT32U)pAMIYAFUGetStatus->GetStatusRes.Datalen);


     return ( sizeof( AMIYAFUGetStatusRes_T ) );
}


/*---------------------------------------
 * YAFUTimerAction
 *---------------------------------------*/
int YAFUTimerAction()
{

    /* Preserving the UBoot configurations*/
    char *mtdDevice = MTDDEVICE;
    int mtd;

    mtd = open (mtdDevice, O_RDWR);
    if (mtd < 0)    
    {
        TCRIT ("Cannot open mtd raw device %s. Exiting...\n", mtdDevice);
        return errno;
    }
	
    /*  Write back environment into flash   */
    if(WriteEnv (&environment,mtd, (EnvStart - FlashStart), EnvSize, GetMacrodefine_getint("CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE", 0)) != 0)
    {
        TCRIT ("Cannot write back u-boot environment preserved into flash\n");
		CloseEnv(&environment,mtd);
        return -1;
    }
    CloseEnv(&environment,mtd);

    // Deactivate Flash Mode
    ActivateFlashStatus = 0x00;
    LastStatCode = 0x00;

    if(g_corefeatures.dual_image_support == ENABLED && (gDeviceNode == 1 || gDeviceNode == 0x00))
    {
        SetMostRecentlyProgFW();
		if(g_corefeatures.fail_safe_booting == ENABLED)
			ClearFailsafeBootErrorCodes();
    }

// Resetting the BMC
    /*  Reboot system here  */
    sleep(1);
    reboot (LINUX_REBOOT_CMD_RESTART);
    return 0;
}

/*---------------------------------------
 * YAFUTimerTask
 *---------------------------------------*/
void* YAFUTimerTask(void *pArg)
{
    INT8U YafuInterfaceType=0;
    INT8U *cursessiontype = (INT8U*)pArg;
    prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

    if(cursessiontype != NULL)
        YafuInterfaceType = *cursessiontype;

    if( YafuInterfaceType == LAN_SESSION_TYPE)
    {
	if( YafuFlashMode == YAFU_INTERACTIVE_FLASH)
		YafuTimerCnt = YAFU_LAN_IA_TIMER_COUNT;
        else
		YafuTimerCnt = YAFU_LAN_FF_TIMER_COUNT ;
    }
    else if( YafuInterfaceType == KCS_SESSION_TYPE)
    {
	if( YafuFlashMode == YAFU_INTERACTIVE_FLASH)
		YafuTimerCnt = YAFU_KCS_IA_TIMER_COUNT;
	else
		YafuTimerCnt = YAFU_KCS_FF_TIMER_COUNT ;
    }		
    else if(YafuInterfaceType == USB_SESSION_TYPE)
    {
	if( YafuFlashMode == YAFU_INTERACTIVE_FLASH)
		YafuTimerCnt = YAFU_USB_IA_TIMER_COUNT;
	else
		YafuTimerCnt = YAFU_USB_FF_TIMER_COUNT;
    }
    else
    {
	TDBG(" Flash request received from unknown Interface, so terminating!! \n");
	return NULL;
    }
	
    TINFO("YafuTimer has started with Timeout = %ld\n",YafuTimerCnt);
    while (1)
    {
    	if(YafuflashStatus == YAFU_FLASH_SUCCEEDED)
		break;
        YafuTimerCnt--;
        usleep (1000 * YAFU_MSECS_PER_TICK);
	if( !YafuTimerCnt)
 	{
 		TDBG("Flashing process has Timed OUT, so preserving the UBoot and resetting the BMC\n");
		YAFUTimerAction();
		break;
 	}
    }
    return NULL;

}


/*---------------------------------------
 * AMIYAFUActivateFlashMode
 *---------------------------------------*/
int AMIYAFUActivateFlashMode ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{
    int RetVal = 0, mtdDev=0;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    AMIYAFUActivateFlashModeReq_T *pAMIYAFUActivateFlashReq = (AMIYAFUActivateFlashModeReq_T *)pReq;
    AMIYAFUActivateFlashModeRes_T* pAMIYAFUActivateFlash = (AMIYAFUActivateFlashModeRes_T*)pRes;
    _FAR_ SensorSharedMem_T*	pSenSharedMem; 
    INT8U sessiontype=0;
    int ErrVal = 0;
    
    if(CalculateChksum((char *)&pAMIYAFUActivateFlashReq->Mode,sizeof(INT16U)) != pAMIYAFUActivateFlashReq->ActivateflashReq.CRC32chksum)
    {
          LastStatCode=YAFU_INVALID_CHKSUM;
          return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUActivateFlashReq->ActivateflashReq.Seqnum));
    }
   

   /* Removing the file if it exists already, this file denotes that going to flash active image*/
   if(access("/var/activeimage", F_OK) == 0)
       unlink("/var/activeimage");

   if(g_FlashingImage == g_RunningImage)
       safe_system("touch /var/activeimage");

   if (g_PDKHandle[PDK_ACTIVATEFLASHMODE] != NULL)
   {
        ErrVal = ((int(*)(INT16U, INT16U, int *, int))g_PDKHandle[PDK_ACTIVATEFLASHMODE]) (pAMIYAFUActivateFlashReq->Mode&0xff, pAMIYAFUActivateFlashReq->Mode>>8, &RetVal, BMCInst);
   }
   
   if(session_count == 0)
   {
	   session_count++;
   }
   else
   {
	   
	   *pRes = CC_DEV_IN_FIRMWARE_UPDATE_MODE;
	   return sizeof(INT8U);
	   
   }

   if ((g_PDKHandle[PDK_ACTIVATEFLASHMODE] == NULL) || (ErrVal == 0) || ((gDeviceNode == BIT0 || gDeviceNode == 0x00) && (g_FlashingImage == g_RunningImage) && ((pAMIYAFUActivateFlashReq->Mode>>8) == 0)))
        RetVal = PrepareFlashArea (FLSH_CMD_PREP_YAFU_FLASH_AREA, g_corefeatures.dual_image_support);

    if ((RetVal == 0) || (RetVal == 4))
    {
         ActivateFlashStatus = 0x01;

         if((access("/var/yafu_bios_update_selection", F_OK) != 0) && (access("/var/yafu_cpld_update_selection", F_OK) != 0))
        {
            YafuFlashMode = pAMIYAFUActivateFlashReq->Mode&0xff;
            if(threadid_yafu == 0)
            {
                OS_THREAD_TLS_GET(g_tls.CurSessionType,sessiontype);
                TINFO("Creating Thread for YafuTimerTask\n");
                pthread_create(&threadid_yafu, NULL, YAFUTimerTask,(void *)&sessiontype);
            }

            SetBootParam = 1;
            if((RetVal = InitEnv(&environment,&mtdDev,&SetBootParam)) != 0)
            {
                printf("Cannot init U-Boot Env in YAFUActivateFlashMode for YafuTimerThread\n");
                return RetVal;
            }
            close(mtdDev);

            RetVal =0;
        }
    }
    else
    {
         ActivateFlashStatus = 0x00;
    }

   if( g_corefeatures.online_flashing_support != ENABLED)
   {
       pSenSharedMem = (_FAR_ SensorSharedMem_T*)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;   /* Create mutex for Sensor shared memory */
       OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex,WAIT_INFINITE);
       pSenSharedMem->GlobalSensorScanningEnable = FALSE;
       /* Release mutex for Sensor shared memory */
       OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);
   }

	if(pAMIYAFUActivateFlashReq->ActivateflashReq.Datalen == 0x02)
		pAMIYAFUActivateFlash->ActivateflashRes.Datalen= 0x02;
	else
	{
	    LastStatCode=YAFU_CC_INVALID_DATLEN;
           return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUActivateFlashReq->ActivateflashReq.Seqnum));
    }
    LOCK_BMC_SHARED_MEM(BMCInst);
    g_MBMCInfo.FlashType= YAFU_FLASH;
    UNLOCK_BMC_SHARED_MEM(BMCInst);
    pAMIYAFUActivateFlash->CompletionCode = YAFU_CC_NORMAL;
    pAMIYAFUActivateFlash->ActivateflashRes.Seqnum = pAMIYAFUActivateFlashReq->ActivateflashReq.Seqnum;
    pAMIYAFUActivateFlash->ActivateflashRes.YafuCmd= pAMIYAFUActivateFlashReq->ActivateflashReq.YafuCmd;
    pAMIYAFUActivateFlash->ActivateflashRes.CRC32chksum = 0x00;
    pAMIYAFUActivateFlash->Delay = 0x00;

    return ( sizeof( AMIYAFUActivateFlashModeRes_T ) );
}

/*-------------------------------------------
 * AMIYAFUDualImgSup
 *------------------------------------------*/
int AMIYAFUDualImgSup(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{
    AMIYAFUDualImgSupReq_T *pAMIYAFUDualImgSupReq = (AMIYAFUDualImgSupReq_T *)pReq;
    AMIYAFUDualImgSupRes_T *pAMIYAFUDualImgSupRes = (AMIYAFUDualImgSupRes_T *)pRes;

    if(g_corefeatures.dual_image_support == ENABLED)
    {
        if(CalculateChksum((char *)&pAMIYAFUDualImgSupReq->PreserveConf,pAMIYAFUDualImgSupReq->DualImgSupReq.Datalen) != pAMIYAFUDualImgSupReq->DualImgSupReq.CRC32chksum)
        {
             LastStatCode=YAFU_INVALID_CHKSUM;
             return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUDualImgSupReq->DualImgSupReq.Seqnum));
        }

        pAMIYAFUDualImgSupRes->CompletionCode = YAFU_CC_NORMAL;
        if((pAMIYAFUDualImgSupReq->PreserveConf == TRUE) || (pAMIYAFUDualImgSupReq->PreserveConf == FALSE))
        {
            DualImgPreserveConf = pAMIYAFUDualImgSupReq->PreserveConf;
        }
        else
        {
            LastStatCode = YAFU_CC_INVALID_DATA;
            return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATA,pAMIYAFUDualImgSupReq->DualImgSupReq.CRC32chksum));
        }
    }
    else
    {
        *pRes = CC_INV_CMD;
        return sizeof(INT8U);
    }

    return sizeof(AMIYAFUDualImgSupRes_T);
}

/*---------------------------------------
 * AMIYAFUAllocateMemory
 *---------------------------------------*/
int AMIYAFUAllocateMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
	//char command[64];
	//FILE *fp = NULL;
	struct sysinfo s_info;
	int error;
	INT32U  free_mem=0;


    AMIYAFUAllocateMemoryReq_T  *pAMIYAFUAllocateMemoryReq = (AMIYAFUAllocateMemoryReq_T *)pReq;

if (ActivateFlashStatus == 0x01)
{
   AMIYAFUAllocateMemoryRes_T* pAMIYAFUAllocateMemory = (AMIYAFUAllocateMemoryRes_T*)pRes;

   if(CalculateChksum((char *)&pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc,sizeof(INT32U)) != pAMIYAFUAllocateMemoryReq->AllocmemReq.CRC32chksum)
  {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUAllocateMemoryReq->AllocmemReq.Seqnum));
  }

    if(pAMIYAFUAllocateMemoryReq->AllocmemReq.Datalen== 0x04)
		pAMIYAFUAllocateMemory->AllocmemRes.Datalen = 0x04;
    else
    {
              LastStatCode=YAFU_CC_INVALID_DATLEN;
              return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUAllocateMemoryReq->AllocmemReq.Seqnum));
    
    }

    if(pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc > (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize) )
    {
        /*strcpy( command, "free | grep Mem | awk '{print $4}'" );
        fp = popen( command, "r" );
        if( fp == NULL )
        	return -1;
        fscanf(fp, "%ld", &free_mem );
        pclose(fp);*/
	error=sysinfo(&s_info);
	if(error !=0)
	{
	   fprintf(stderr,"\nError in getting free RAM Memory using Sysinfo system call\n Error Code:%d\n",error);
	   LastStatCode=YAFU_CC_GET_MEM_ERR;
	   return(AMIYAFUNotAcks(pRes,YAFU_CC_GET_MEM_ERR,pAMIYAFUAllocateMemoryReq->AllocmemReq.Seqnum));
	}
	free_mem=s_info.freeram;

        /*Reserve the one block of memory for future allocation to fix the page fault in EraseCopyFlash function*/
        free_mem = free_mem - gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize;

        if(pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc > free_mem)
        {
            fprintf(stderr,"\nFlash Image Size(%d)  is greater than Free Memory(%d) in RAM \n\n",(INT32U)pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc,free_mem);
            pAMIYAFUAllocateMemory->Addofallocmem = (INT32U)0xfffffffe;
        }
        else
        {
    allocmem =(INT8U *) malloc ( pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc);

    if(allocmem == NULL)
		pAMIYAFUAllocateMemory->Addofallocmem = (INT32U)0xffffffff;
            else
	        	pAMIYAFUAllocateMemory->Addofallocmem = (INT32U)allocmem ;
        }
        ImgSize = pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc;
    }
    else
    {

        allocmem =(INT8U *) malloc ( pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc);

        if(allocmem == NULL)
	    	pAMIYAFUAllocateMemory->Addofallocmem = (INT32U)0xffffffff;
        else
		pAMIYAFUAllocateMemory->Addofallocmem = (INT32U)allocmem ;

        ImgSize = pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc;
    }

    pAMIYAFUAllocateMemory->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode= (int)pAMIYAFUAllocateMemory->CompletionCode;
    pAMIYAFUAllocateMemory->AllocmemRes.Seqnum = pAMIYAFUAllocateMemoryReq->AllocmemReq.Seqnum;
    pAMIYAFUAllocateMemory->AllocmemRes.YafuCmd= pAMIYAFUAllocateMemoryReq->AllocmemReq.YafuCmd;
    pAMIYAFUAllocateMemory->AllocmemRes.CRC32chksum = CalculateChksum((char *)&pAMIYAFUAllocateMemory->Addofallocmem,sizeof(INT32U));

    return ( sizeof(AMIYAFUAllocateMemoryRes_T));
}
else
{
    LastStatCode = YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUAllocateMemoryReq->AllocmemReq.Seqnum));
    
}

}

/*---------------------------------------
 * AMIYAFUFreeMemory
 *---------------------------------------*/
int AMIYAFUFreeMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{
   AMIYAFUFreeMemoryReq_T *pAMIYAFUFreeMemoryReq = (AMIYAFUFreeMemoryReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
    INT32U *memtofree;

    AMIYAFUFreeMemoryRes_T* pAMIYAFUFreeMemory	=(AMIYAFUFreeMemoryRes_T*)pRes;

   if(CalculateChksum((char *)&pAMIYAFUFreeMemoryReq->AddrtobeFreed,sizeof(INT32U)) != pAMIYAFUFreeMemoryReq->FreememReq.CRC32chksum)
  {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUFreeMemoryReq->FreememReq.Seqnum));

  }

    if(pAMIYAFUFreeMemoryReq->FreememReq.Datalen== 0x04)
          pAMIYAFUFreeMemory->FreememRes.Datalen= 0x01;
    else
    {
            LastStatCode = YAFU_CC_INVALID_DATLEN;
            return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUFreeMemoryReq->FreememReq.Seqnum));
    }

   memtofree = (INT32U *)pAMIYAFUFreeMemoryReq->AddrtobeFreed;

   free(memtofree);

    pAMIYAFUFreeMemory->Status =0x00;
    pAMIYAFUFreeMemory->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = pAMIYAFUFreeMemory->CompletionCode;
    pAMIYAFUFreeMemory->FreememRes.Seqnum = pAMIYAFUFreeMemoryReq->FreememReq.Seqnum;
    pAMIYAFUFreeMemory->FreememRes.YafuCmd= pAMIYAFUFreeMemoryReq->FreememReq.YafuCmd;
    pAMIYAFUFreeMemory->FreememRes.CRC32chksum = CalculateChksum((char *)&pAMIYAFUFreeMemory->Status,sizeof(INT8U));

    return (sizeof(AMIYAFUFreeMemoryRes_T));
}
else
{
    LastStatCode = YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUFreeMemoryReq->FreememReq.Seqnum));   
    
}


}

/*---------------------------------------
 * AMIYAFUReadFlash
 *---------------------------------------*/

int AMIYAFUReadFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{

    AMIYAFUReadFlashReq_T *pAMIYAFUReadFlashReq = (AMIYAFUReadFlashReq_T *)pReq;


if(ActivateFlashStatus == 0x01)
{
     int fd =0;
     //INT8U *offset;
     INT8U *Buf = 0;
     DWORD StartOffset = 0;
     Buf = NULL;
    AMIYAFUReadFlashRes_T* pAMIYAFUReadFlash =(AMIYAFUReadFlashRes_T*)pRes;

   if(CalculateChksum((char *)&pAMIYAFUReadFlashReq->offsettoread,pAMIYAFUReadFlashReq->ReadFlashReq.Datalen) != pAMIYAFUReadFlashReq->ReadFlashReq.CRC32chksum)
  {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum));   
  }

    if(pAMIYAFUReadFlashReq->ReadFlashReq.Datalen!= 0x07)
   {
        LastStatCode =  YAFU_CC_INVALID_DATLEN ;
        return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum));   
   }

   // offset = (INT8U *) pAMIYAFUReadFlashReq->offsettoread;
    
    Buf = malloc(pAMIYAFUReadFlashReq->Sizetoread);

    if(Buf == NULL)
    {
      	LastStatCode = YAFU_CC_ALLOC_ERR;
        return (AMIYAFUNotAcks(pRes,YAFU_CC_ALLOC_ERR,pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum));               
    }

    if(g_corefeatures.dual_image_support == ENABLED && (gDeviceNode == 1 || gDeviceNode == 0x00))
    {
        if( g_FlashingImage == IMAGE_2 )
            StartOffset = g_coremacros.global_used_flash_size;
    }

      fd=open(gAMIYAFUSwitchFlashDevice.MTDName, O_RDONLY);
      if(fd == -1)
      {
                LastStatCode = YAFU_CC_DEV_OPEN_ERR;
              IPMI_ERROR ("Amiyafuupdate:Unable to open %s device/n", gAMIYAFUSwitchFlashDevice.MTDName);
		printf("Unable to open %s device\n", gAMIYAFUSwitchFlashDevice.MTDName);
		if(Buf != NULL)
		{
		    free(Buf);
		}
              return (AMIYAFUNotAcks(pRes,YAFU_CC_DEV_OPEN_ERR,pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum));                 
      	}

    if ( lseek (fd,StartOffset + pAMIYAFUReadFlashReq->offsettoread, SEEK_SET) == -1)
    {
	    LastStatCode = YAFU_CC_SEEK_ERR;
           fprintf (stderr,
            "seek error on %s: %s\n",
            gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
           if(Buf != NULL)
           {
               free(Buf);
           }
           if(fd != -1)
           {
               close(fd);
           }
            return (AMIYAFUNotAcks(pRes,YAFU_CC_SEEK_ERR,pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum));                 

    }

     if( (read(fd,Buf,pAMIYAFUReadFlashReq->Sizetoread)) != pAMIYAFUReadFlashReq->Sizetoread)
     {

        LastStatCode = YAFU_CC_READ_ERR;
         fprintf (stderr,
            "Bytes read error %s: %s\n",
            gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
         if(Buf != NULL)
          {
              free(Buf);
          }
          if(fd != -1)
          {
              close(fd);
          }
         return (AMIYAFUNotAcks(pRes,YAFU_CC_READ_ERR,pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum));                 
	
     	}


     pAMIYAFUReadFlash->ReadFlashRes.Datalen= pAMIYAFUReadFlashReq->Sizetoread;

     pAMIYAFUReadFlash->CompletionCode = YAFU_CC_NORMAL;
     LastStatCode = pAMIYAFUReadFlash->CompletionCode;
     pAMIYAFUReadFlash->ReadFlashRes.Seqnum = pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum;
     pAMIYAFUReadFlash->ReadFlashRes.YafuCmd= pAMIYAFUReadFlashReq->ReadFlashReq.YafuCmd;
     pAMIYAFUReadFlash->ReadFlashRes.CRC32chksum = CalculateChksum((char *)&Buf,sizeof(pAMIYAFUReadFlashReq->Sizetoread));

    memcpy (( INT8U*) (pAMIYAFUReadFlash + 1),
              ( INT8U*)Buf,
              pAMIYAFUReadFlashReq->Sizetoread );

    free(Buf);
    if(fd != -1)
    {
        close(fd);
    }
    return (sizeof(AMIYAFUReadFlashRes_T) + pAMIYAFUReadFlashReq->Sizetoread);
}
else
{
     LastStatCode = YAFU_CC_IN_DEACTIVATE;
     return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum));                 
}

}


/**
 **@fn SetDefaultTimezone
 **@brief This command is used set the defalut timezone value.
 **@param TimeZone - Timezone string -initial zone name.
 **@param BMCInst - BMC Instance Value
 **@return  0 on success -1 on failure
 **/
int 
SetDefaultTimezone (INT8U *TimeZone, int BMCInst)
{
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
    INT16 UTCOffset = 0;
    time_t Curtime = 0;
    char Cmd [ZONE_PATH_SIZE];
    char Zonepath [ZONE_PATH_SIZE];
    struct stat sb, buf;
    struct tm *loctm = NULL;
    INT8U Size = 0;
    INT8U TimeZoneIdentifier[TIME_ZONE_IDENTIFIER_SIZE + 1]; //variable holds GMT+[] value
    INT8U UTCTimeZone [TIME_ZONE_LEN + 1];//Holds modified time zone
    char Filename[MAXFILESIZE];

    if (NULL == TimeZone)
    {
        return -1;
    }
    
    IPMI_CONFIGS_FILE(BMCInst,Filename);
    memset (Zonepath, 0, ZONE_PATH_SIZE);
    memset ( UTCTimeZone, 0, (TIME_ZONE_LEN + 1));
    memset (TimeZoneIdentifier, 0, (TIME_ZONE_IDENTIFIER_SIZE + 1));
    memset (Cmd, 0, ZONE_PATH_SIZE);

    //Copy input time zone as UTC time zone
    //if only input is GMT+/GMT- time zone will be modified
    memcpy( UTCTimeZone, TimeZone, TIME_ZONE_LEN);

    //Check if Time Zone is GMT+ or GMT-
    //if yes then switch them with UTC time zones
    if ( 0 == memcmp ( UTCTimeZone, TIMEZONE_GMT_PVE, strlen(TIMEZONE_GMT_PVE)))
    {
        Size = strlen (TIMEZONE_GMT_PVE);
        //Copy the identifier situated after GMT+
        memcpy( TimeZoneIdentifier, ( TimeZone + Size), TIME_ZONE_IDENTIFIER_SIZE);
        //make utc time zone empty before filling with modified time zone
        memset ( UTCTimeZone, 0, (TIME_ZONE_LEN + 1));
        //Switch GMT+ to Etc/GMT-
        snprintf ( (char *)UTCTimeZone, TIME_ZONE_LEN, "%s%s", TIMEZONE_OFFSET_NVE,(char*)TimeZoneIdentifier);
    }
    else if ( 0 == memcmp ( UTCTimeZone, TIMEZONE_GMT_NVE, strlen(TIMEZONE_GMT_NVE)))
    {
        Size = strlen (TIMEZONE_GMT_NVE);
        //Copy the identifier situated after GMT-
        memcpy( TimeZoneIdentifier, ( TimeZone + Size), TIME_ZONE_IDENTIFIER_SIZE);
        //make utc time zone empty before filling with modified time zone
        memset ( UTCTimeZone, 0, (TIME_ZONE_LEN + 1));
        //Switch GMT- to Etc/GMT+
        snprintf ( (char *)UTCTimeZone, TIME_ZONE_LEN, "%s%s", TIMEZONE_OFFSET_PVE,(char*)TimeZoneIdentifier);
    }

    //Construct the time zone file location
    snprintf (Zonepath, ZONE_PATH_SIZE, "%s%s", TIME_ZONE_PATH,(char*)UTCTimeZone);

    /*Verify the file presence*/ 
    if (stat (Zonepath, &sb))
    {
        return -1;
    }
    if (!S_ISREG (sb.st_mode))
    {
        return -1;
    }

    //construct the time zone command and run the command
    snprintf (Cmd, ZONE_PATH_SIZE, "%s %s %s", LINK_CMD, Zonepath, LOCALTIME);
    safe_system (Cmd);

    /*Getting UTC offset and updating*/
    time(&Curtime);
    loctm = localtime(&Curtime);
    UTCOffset = (loctm->tm_gmtoff/MAX_MINS);
    if (((UTCOffset <= SEL_UTC_MAX_RANGE) && (UTCOffset >= SEL_UTC_MIN_RANGE)) || (UTCOffset == UNSPECIFIED_UTC_OFFSET))
    {
        pBMCInfo->GenConfig.SELTimeUTCOffset = UTCOffset;
    }

    //Copy the actual time zone input as time zone
    memcpy (pBMCInfo->GenConfig.TimeZone, TimeZone, sizeof (pBMCInfo->GenConfig.TimeZone)); 
    /*Write to NVRAM*/ 
    if (stat(Filename, &buf) == 0)
    {
        FlushIPMI((INT8U*)&pBMCInfo->GenConfig,(INT8U*)&pBMCInfo->GenConfig,pBMCInfo->IPMIConfLoc.GenConfigAddr, 
                          sizeof(GENConfig_T),BMCInst); 
    }

    return 0;
}


/*---------------------------------------
 * AMIYAFUWriteFlash
 *---------------------------------------*/
int AMIYAFUWriteFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst)
{

    AMIYAFUWriteFlashReq_T *pAMIYAFUWriteFlashReq = (AMIYAFUWriteFlashReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
    //INT8U *OffsetToWrite;
    int fd = -1;
    DWORD StartOffset = 0;

    AMIYAFUWriteFlashRes_T* pAMIYAFUWriteFlash = (AMIYAFUWriteFlashRes_T*)pRes;

   if(CalculateChksum((char *)&pAMIYAFUWriteFlashReq->offsettowrite,pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen) != pAMIYAFUWriteFlashReq->WriteFlashReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUWriteFlashReq->WriteFlashReq.Seqnum));                 
   }

    //OffsetToWrite = (INT8U *)pAMIYAFUWriteFlashReq->offsettowrite;

    if(g_corefeatures.dual_image_support == ENABLED && (gDeviceNode == 1 || gDeviceNode == 0x00))
    {
        if( g_FlashingImage == IMAGE_2 )
            StartOffset = g_coremacros.global_used_flash_size;
    }

     fd=open(gAMIYAFUSwitchFlashDevice.MTDName, O_RDWR);
     if (fd == -1)
     {
          LastStatCode = YAFU_CC_DEV_OPEN_ERR;
	   printf("ERROR: open failed (%s)\n", strerror(errno));
          return (AMIYAFUNotAcks(pRes,YAFU_CC_DEV_OPEN_ERR,pAMIYAFUWriteFlashReq->WriteFlashReq.Seqnum));                 
     }

        if ( lseek (fd, StartOffset +  pAMIYAFUWriteFlashReq->offsettowrite, SEEK_SET) == -1)
	{
	     LastStatCode = YAFU_CC_SEEK_ERR;
	     fprintf (stderr,
            "seek error on %s: %s\n",
            gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
            close(fd);
            return (AMIYAFUNotAcks(pRes,YAFU_CC_SEEK_ERR,pAMIYAFUWriteFlashReq->WriteFlashReq.Seqnum));                 
      }

        if(write(fd,(pReq + sizeof(AMIYAFUWriteFlashReq_T)),(pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen - 5)) != (pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen - 5))
        {
            LastStatCode = YAFU_CC_WRITE_ERR;
            close(fd);
            return (AMIYAFUNotAcks(pRes,YAFU_CC_SEEK_ERR,pAMIYAFUWriteFlashReq->WriteFlashReq.Seqnum));                 
        }

    pAMIYAFUWriteFlash->SizeWritten = (pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen - 5);
    pAMIYAFUWriteFlash->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = pAMIYAFUWriteFlash->CompletionCode;
    pAMIYAFUWriteFlash->WriteFlashRes.Seqnum = pAMIYAFUWriteFlashReq->WriteFlashReq.Seqnum;
    pAMIYAFUWriteFlash->WriteFlashRes.YafuCmd= pAMIYAFUWriteFlashReq->WriteFlashReq.YafuCmd;
    pAMIYAFUWriteFlash->WriteFlashRes.Datalen= 0x02;
    pAMIYAFUWriteFlash->WriteFlashRes.CRC32chksum = CalculateChksum((char *)&(pAMIYAFUWriteFlash->SizeWritten),sizeof(INT16U));

    close(fd);
    return (sizeof(AMIYAFUWriteFlashRes_T));
}
else
{
    LastStatCode = YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUWriteFlashReq->WriteFlashReq.Seqnum));                 
}

}

/*---------------------------------------
 * AMIYAFUEraseFlash
 *---------------------------------------*/
int AMIYAFUEraseFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{

    AMIYAFUErashFlashReq_T *pAMIYAFUEraseFlashReq = (AMIYAFUErashFlashReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
    INT32U offset;
    int fd = -1;
    DWORD StartOffset = 0;
    struct erase_info_user		erase_info;

    AMIYAFUErashFlashRes_T* pAMIYAFUEraseFlash =(AMIYAFUErashFlashRes_T*)pRes;

    if(CalculateChksum((char *)&pAMIYAFUEraseFlashReq->Blknumtoerase,sizeof(INT32U)) != pAMIYAFUEraseFlashReq->EraseFlashReq.CRC32chksum)
   {
        LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUEraseFlashReq->EraseFlashReq.Seqnum));
   }

    if(pAMIYAFUEraseFlashReq->EraseFlashReq.Datalen== 0x04)
          pAMIYAFUEraseFlash->EraseFlashRes.Datalen= 0x01;
    else
    {
	   LastStatCode= YAFU_CC_INVALID_DATLEN;
          return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUEraseFlashReq->EraseFlashReq.Seqnum));
    }

    offset = ((pAMIYAFUEraseFlashReq->Blknumtoerase) -1) * (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);

    if(g_corefeatures.dual_image_support == ENABLED && (gDeviceNode == 1 || gDeviceNode == 0x00))
    {
        if( g_FlashingImage == IMAGE_2 )
            StartOffset = g_coremacros.global_used_flash_size;
    }

    fd=open(gAMIYAFUSwitchFlashDevice.MTDName, O_RDWR);
    if (fd == -1)
    {
          LastStatCode = YAFU_CC_DEV_OPEN_ERR;
          printf("ERROR: open failed (%s)\n", strerror(errno));
          return (AMIYAFUNotAcks(pRes,YAFU_CC_DEV_OPEN_ERR,pAMIYAFUEraseFlashReq->EraseFlashReq.Seqnum));
    }

    erase_info.start = StartOffset +  offset;
    erase_info.length = (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);

     if((ioctl (fd, MEMERASE, &erase_info)) != -1)
     {
        pAMIYAFUEraseFlash->Status = 0x00;
     }
     else
     {
        pAMIYAFUEraseFlash->Status = 0x01;
     }


    pAMIYAFUEraseFlash->CompletionCode =YAFU_CC_NORMAL;
    LastStatCode = (INT16U) pAMIYAFUEraseFlash->CompletionCode;
    pAMIYAFUEraseFlash->EraseFlashRes.Seqnum = pAMIYAFUEraseFlashReq->EraseFlashReq.Seqnum;
    pAMIYAFUEraseFlash->EraseFlashRes.YafuCmd= pAMIYAFUEraseFlashReq->EraseFlashReq.YafuCmd;
    pAMIYAFUEraseFlash->EraseFlashRes.CRC32chksum = CalculateChksum((char *)&(pAMIYAFUEraseFlash->Status),sizeof(INT8U));
    close(fd);
    return (sizeof(AMIYAFUErashFlashRes_T));
}
else
{
    LastStatCode =  YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUEraseFlashReq->EraseFlashReq.Seqnum));
}

}

/*---------------------------------------
 * AMIYAFUProtectFlash
 *---------------------------------------*/
int AMIYAFUProtectFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{

    AMIYAFUProtectFlashReq_T *pAMIYAFUProtectFlashReq = (AMIYAFUProtectFlashReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
   INT32U offset;
   int fd;
   DWORD StartOffset = 0;
   struct erase_info_user		 erase_info;

   AMIYAFUProtectFlashRes_T* pAMIYAFUProtectFlash = (AMIYAFUProtectFlashRes_T*)pRes;


   if(CalculateChksum((char *)&pAMIYAFUProtectFlashReq->Blknum,pAMIYAFUProtectFlashReq->ProtectFlashReq.Datalen) != pAMIYAFUProtectFlashReq->ProtectFlashReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUProtectFlashReq->ProtectFlashReq.Seqnum));                     
   }

    if(pAMIYAFUProtectFlashReq->ProtectFlashReq.Datalen== 0x05)
          pAMIYAFUProtectFlash->ProtectFlashRes.Datalen= 0x01;
    else
    {
        LastStatCode = YAFU_CC_INVALID_DATLEN;
        return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUProtectFlashReq->ProtectFlashReq.Seqnum));                     
    }

    if(access("/var/yafu_bios_update_selection", F_OK) != 0)
    {
        if(g_corefeatures.dual_image_support == ENABLED && (gDeviceNode == 1 || gDeviceNode == 0x00))
        {
            if( g_FlashingImage == IMAGE_2 )
                StartOffset = g_coremacros.global_used_flash_size;
        }
    }
     fd=open(gAMIYAFUSwitchFlashDevice.MTDName, O_RDWR);
     if (fd == -1) {
	  LastStatCode =  YAFU_CC_DEV_OPEN_ERR;
         printf("ERROR: open failed (%s)\n", strerror(errno));
         return (AMIYAFUNotAcks(pRes,YAFU_CC_DEV_OPEN_ERR,pAMIYAFUProtectFlashReq->ProtectFlashReq.Seqnum));                     

    }

     if(pAMIYAFUProtectFlashReq->Blknum == 0xffffffff)
     {
         erase_info.start = (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashAddress);
         erase_info.length = (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashSize);
     }
     else
     {
          offset = StartOffset + ( ((pAMIYAFUProtectFlashReq->Blknum)-1) * (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize) );

          erase_info.start = offset;
          erase_info.length = (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashSize);
     }

     if(pAMIYAFUProtectFlashReq->Protect ==  0x01)
     {
     	 if((ioctl (fd, MEMLOCK, &erase_info)) != -1)
	 	 pAMIYAFUProtectFlash->Status = 0x00;
    	 else
	 	 pAMIYAFUProtectFlash->Status = 0x01;
     }
     else if(pAMIYAFUProtectFlashReq->Protect == 0x00)
     {
             if((ioctl (fd, MEMUNLOCK, &erase_info)) != -1)
	 	 pAMIYAFUProtectFlash->Status = 0x00;
    	     else
	 	 pAMIYAFUProtectFlash->Status = 0x01;
     }


    pAMIYAFUProtectFlash->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = pAMIYAFUProtectFlash->CompletionCode;
    pAMIYAFUProtectFlash->ProtectFlashRes.Seqnum = pAMIYAFUProtectFlashReq->ProtectFlashReq.Seqnum;
    pAMIYAFUProtectFlash->ProtectFlashRes.YafuCmd= pAMIYAFUProtectFlashReq->ProtectFlashReq.YafuCmd;
    pAMIYAFUProtectFlash->ProtectFlashRes.CRC32chksum =  CalculateChksum((char *)&(pAMIYAFUProtectFlash->Status),sizeof(INT8U));
    close(fd);

    return (sizeof(AMIYAFUProtectFlashRes_T));
}
else
{
     LastStatCode = YAFU_CC_IN_DEACTIVATE;
     return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUProtectFlashReq->ProtectFlashReq.Seqnum));
}

}
void*  EraseCopyFlash	(void *pArg)
{

	int fd;
	INT32U i=0,j;
	INT8U *Buf = NULL;
	INT8U *TmpBuf = NULL;
	struct erase_info_user erase_info;
    DWORD StartOffset = 0;
	void *dl_handle = NULL;
        int (*dl_func)(int, unsigned long, INT8U*, unsigned long, INT32U);
	int retval = 0;
	int ErrCode = 0;
        INT8U FlashError=0;

	prctl(PR_SET_NAME,__FUNCTION__,0,0,0);
	while(1)
	{
		ECFthreadstate=1;
		if(ECFstart==1)
		{

      if(access("/var/yafu_cpld_update_selection", F_OK) == 0)
      {
      	if (g_PDKHandle[PDK_SETCPLDFWUPDATE] != NULL)
      	{
      		int ret_ctrl;

          ret_ctrl = ((int(*)(INT8U*, unsigned long))g_PDKHandle[PDK_SETCPLDFWUPDATE]) (WriteMemOff, Sizetocpy);
          if (ret_ctrl != 0)
          {
          	LastStatCode = YAFU_CC_WRITE_ERR;
          	printf("\nCpld program fail[%x]. \n\n",(unsigned int) LastStatCode);
          	ECFstart=0;
          	
          	continue;
          }

          Flashoffset += Sizetocpy;
          WriteMemOff += Sizetocpy;
          
          LastStatCode = YAFU_ECF_SUCCESS;
          ECFstart=0;
          continue;
        }
      }
      			
			if(access("/var/yafu_bios_update_selection", F_OK) != 0)
            		{
                		if(g_corefeatures.dual_image_support == ENABLED)
                		{
                    			if( g_FlashingImage == IMAGE_2 && (gDeviceNode == 1 || gDeviceNode == 0x00))
                    			{
			                        StartOffset = g_coremacros.global_used_flash_size;
                        			#ifdef CONFIG_SPX_FEATURE_YAFUFLASH_FLASH_UBOOT_ON_ACTIVE_IMAGE
				                        if((PreserveFlag&BIT0) == 0)
                                				StartOffset = 0;
                        			#endif
                    			}
                		}
            		}

			// This hook can be used to perform any OEM flashing routines
			// Returning 0 from this hook will revert back to performing default flashing sequence
			// Returning <non-zero> from this hook will validate the ErrCode from the hook, and return appropriately. Default action will be skipped
			// BMCInst is not required for this hook, because flashing can be done by only 1 instance at a time.
			if (g_PDKHandle[PDK_OEMFLASH] != NULL)
   			{
        			retval = ((int(*)(int, INT8U*, unsigned long, int*))g_PDKHandle[PDK_OEMFLASH]) 
									(gDeviceNode, WriteMemOff, Sizetocpy, &ErrCode);
	
				if (retval != 0)
				{
					if ( ErrCode == 2 )
					{
						LastStatCode = YAFU_FLASHING_SAME_IMAGE;
					}
					else if (ErrCode != 0)
					{
						LastStatCode = YAFU_FLASH_ERASE_FAILURE;
					}
					else
					{
						LastStatCode = YAFU_ECF_SUCCESS;
					}
			                        		
					ECFstart=0;
					continue;
				}
   			}

            		fd=open(gAMIYAFUSwitchFlashDevice.MTDName, O_RDWR);
			if (fd == -1)
			{
				LastStatCode = YAFU_CC_DEV_OPEN_ERR;
				printf("ERROR: open failed (%s)\n", strerror(errno));
				return (void*)0;
			}

			if(access("/var/bios_update_selection", F_OK) != 0)
			{
				FlashError = 0;
				// Loop through the entire file size. Each iteration will handle a block of data
				// The block is set to Erase Block Size configured in the PRJ
			for(j=0;j < (Sizetocpy/(gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize));j++)
			{
				// Allocate a buffer equivalent to the Erase Block Size configured in the PRJ
				Buf = malloc((gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize));
				if(Buf == NULL)
				{
                                   OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
					LastStatCode = YAFU_CC_ALLOC_ERR;
                                   OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
					ECFstart=0;
					FlashError = 1;
					break;
				}
				// Assign the allocated buffer to a temporary pointer
				TmpBuf = Buf;

				// Seek to the required offset in the MTD Device
				if ( lseek (fd,StartOffset + Flashoffset, SEEK_SET) == -1)
				{
                                   OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
					LastStatCode = YAFU_CC_SEEK_ERR;
                                   OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
					fprintf (stderr,
					"seek error on %s: %s\n",
					gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
					ECFstart=0;
					FlashError = 1;
					free(Buf);
					break;
				}

				// Read a block of data from the MTD Device.
				if( (read(fd,Buf,(gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize))) != (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize))
				{
                                   OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
					LastStatCode= YAFU_CC_READ_ERR;
                                   OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
					fprintf (stderr,
					"Bytes read error %s: %s\n",
					gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
					ECFstart=0;
					FlashError = 1;
					free(Buf);
					break;
				}

				// Compares the read buffer block with a block of data read from the memory.
				// Previously it would compare the entire block in a byte-by-byte manner
				// Now, we have modified it to use memcmp() on a smaller chunk in the block.
				// The chunk size has been defined as 1024 Bytes.
				// Smaller chunks is to allow smaller processing time for each iteration
				for(i=0;i<(gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize / YAFU_FLASH_CHUNK);i++) 
				{
	                                if (memcmp(Buf, WriteMemOff, YAFU_FLASH_CHUNK) != 0)
       	                                {
               	                                WriteMemOff = WriteMemOff - (i * YAFU_FLASH_CHUNK);
               	                                break;
                       	                }

                                       	Buf += YAFU_FLASH_CHUNK;
                                       	WriteMemOff += YAFU_FLASH_CHUNK;
                               	}

				// Comparing the buffer has been completed. Now, free the allocated buffer
                               	Buf = TmpBuf;
                               	free (Buf);

				// If the buffers were same for the entire block, then we would skip that block.  Continue with the next block again
				// If the buffers were found different in the memcmp(), then we would proceed to flash that buffer into the flash device
                      		if (i == (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize / YAFU_FLASH_CHUNK))
                               	{
                                       	printf("Skipping the erase block %ld\n",(Flashoffset / (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize)));
                                       	Flashoffset += (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);
                                       	continue;
				}

				// This stage starts the erase/write cycle on the flash device for each block
				// Fill the erase structure with the flash device offset and the erase block size
				erase_info.start = StartOffset + Flashoffset;
				printf("Upgrading the block =  %ld\n",(Flashoffset / (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize)));
				erase_info.length = (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);

				// Erase the offset using the file descriptor ioctl.
				if((ioctl (fd, MEMERASE, &erase_info)) == -1)
				{
                                   OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
					LastStatCode = YAFU_FLASH_ERASE_FAILURE;
                                   OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
					ECFstart=0;
					FlashError = 1;
					break;
				}

				// Seek to the appropriate offset
				if ( lseek (fd, StartOffset + Flashoffset, SEEK_SET) == -1)
				{
                                   OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
					LastStatCode = YAFU_CC_SEEK_ERR;
                                   OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
					fprintf (stderr,
					"seek error on %s: %s\n",
					gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
					ECFstart=0;
					FlashError = 1;
					break;
				}

				// This code set loops through the block of data, and writes the block of data into the flash deviec
				// The write into the flash device is done in smaller chunks instead of performing for the whole block in a single shot
				// Previously, we would perform write() call on the whole block of data.
				// On platforms with low SPI clock frequency, writing that big block of data was taking more time in the driver layer
				// This was devoiding other interfaces from handling the commands quickly.
				// Some OEMs required IPMB reponse within 110ms which was timing out during flashing.
				// Writing in smaller chunks would bring back control to user space again, thereby giving some cpu time to otehr interface threads also
				// With this change, the IPMB response was being procesed and sent within 110ms in 99% of the cases.
				for (i = 0; i < (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize / YAFU_FLASH_CHUNK); i++)
        	                {
    	                        	if (write (fd, WriteMemOff, YAFU_FLASH_CHUNK) != YAFU_FLASH_CHUNK)
                        	        {
                                 		OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
                                        	LastStatCode = YAFU_CC_WRITE_ERR;
                                   		OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
                                               	ECFstart=0;
                                               	FlashError = 1;
                                               	break;
                                       	}
                                       	WriteMemOff += YAFU_FLASH_CHUNK;
                               	}

                               	Flashoffset += (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);
			}
			}
			else
			{
				dl_handle = NULL;

                dl_handle = dlopen((char *)PDKAPP_LIB, RTLD_LAZY);

                if (dl_handle)
                {
                    dl_func = dlsym(dl_handle,"PDK_YafuDoFullBiosFlash");

                if(NULL != dl_func)
                {
                    retval = dl_func(fd,Flashoffset,WriteMemOff,Sizetocpy, gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);
                    if(retval != 0)
                    {
                        OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
                        LastStatCode = YAFU_FLASH_ERASE_FAILURE;
                        OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
                        ECFstart=0;
                    }
                }
                else
                {
			FlashError = 0;
                    for(j=0;j < (Sizetocpy/(gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize));j++)
                    {
                        Buf = malloc((gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize));
                        if(Buf == NULL)
                        {
                            OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
                            LastStatCode = YAFU_CC_ALLOC_ERR;
                            OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
                            ECFstart=0;
			    	FlashError = 1;
                            break;
                        }
			TmpBuf = Buf;

                        if ( lseek (fd, Flashoffset, SEEK_SET) == -1)
                        {
                            OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
                            LastStatCode = YAFU_CC_SEEK_ERR;
                            OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
                            fprintf (stderr,
                                "seek error on %s: %s\n",
                                gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
                            ECFstart=0;
				FlashError = 1;
                            break;
                        }

                        if( (read(fd,Buf,(gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize))) != (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize))
                        {
                            OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
                            LastStatCode= YAFU_CC_READ_ERR;
                            OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
                                fprintf (stderr,
                                "Bytes read error %s: %s\n",
                                gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
                            ECFstart=0;
			    	FlashError = 1;
                            break;
                        }

			for (i = 0; i < (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize / YAFU_FLASH_CHUNK); i++)
			{
	                	if (memcmp(Buf, WriteMemOff, YAFU_FLASH_CHUNK) != 0)
        	                {
                	        	WriteMemOff = WriteMemOff - (i * YAFU_FLASH_CHUNK);
                	                break;
                        	}

                                Buf += YAFU_FLASH_CHUNK;
                                WriteMemOff += YAFU_FLASH_CHUNK;
                        }

                        Buf = TmpBuf;
                        free (Buf);

                        if (i == (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize / YAFU_FLASH_CHUNK))
                        {
                        	printf("Skipping the erase block %ld\n",
						(Flashoffset / (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize)));
                                Flashoffset += (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);
                                continue;
                        }

                        erase_info.start =Flashoffset;
                        printf("Upgrading the block =  %ld\n",(Flashoffset / (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize)));
                        erase_info.length = (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);

                        if((ioctl (fd, MEMERASE, &erase_info)) == -1)
                        {
                            OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
                            LastStatCode = YAFU_FLASH_ERASE_FAILURE;
                            OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
                            ECFstart=0;
				FlashError = 1;
                            break;
                        }

                        if ( lseek (fd,Flashoffset, SEEK_SET) == -1)
                        {
                            OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
                            LastStatCode = YAFU_CC_SEEK_ERR;
                            OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
                            fprintf (stderr,
                            "seek error on %s: %s\n",
                            gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
                            ECFstart=0;
				FlashError = 1;
                            break;
                        }

                        for (i = 0; i < (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize / YAFU_FLASH_CHUNK); i++)
                        {
                        	if (write (fd, WriteMemOff, YAFU_FLASH_CHUNK) != YAFU_FLASH_CHUNK)
                                {
                            		OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex,WAIT_INFINITE);
                                        LastStatCode = YAFU_CC_WRITE_ERR;
                            		OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
                                        ECFstart=0;
                                        FlashError = 1;
                                        break;
                                }
                                WriteMemOff += YAFU_FLASH_CHUNK;
                         }

                         Flashoffset += (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);
                    }
                }
                dlclose(dl_handle);
            }
        }
        close(fd);
        ECFstart=0;
			if (FlashError == 0)
			{
        OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex, WAIT_INFINITE);
        LastStatCode = YAFU_ECF_SUCCESS;
        OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
			}
            }
		else
		{
			pthread_mutex_lock(&mutex);
			pthread_cond_wait(&cond, &mutex);
			pthread_mutex_unlock(&mutex);
		}
	}

}

int CheckImageSign()
{
    INT8U *HashString = NULL,*pHash = NULL,*AllocatedMem = NULL;
    static RSA *RSAPubKey=NULL;
    INT32U RemBytes = 0;
    FILE *pubfp  = NULL;
    int  nRet = 0;
    unsigned char strHashOfImg[SHA_DIGEST_LENGTH]={0};
    unsigned char strBuf[BUFSIZE]={0};
    SHA_CTX c;
    int Count = 0;
    char str[SHA_DIGEST_LENGTH*2] = {0};

      RemBytes = ImgSize % gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize;
      if(RemBytes == SIGNED_HASH_SIZE) // Consider that the image is signed image
      {
          if (ENABLED == g_corefeatures.signed_hashed_image_support)
          {
             HashString = malloc(RemBytes);

             /* Copy the Hash String from Image */
             memcpy(HashString,allocmem+(ImgSize-RemBytes),RemBytes);

             /* Open the Public Key File and get the Key first */
            pubfp = fopen(TEMP_PUB_KEY,"rb");
            if(pubfp==NULL)
            {
                IPMI_ERROR("Cannot open file public.pem\n");
                free(HashString);
                return -1;
            }
            if (!PEM_read_RSA_PUBKEY(pubfp, &RSAPubKey, NULL, NULL))
            {
                    IPMI_ERROR("Error loading RSA Public Key File.\n");
                    free(HashString);
                    fclose(pubfp);
                    return -1;
            }

            fclose(pubfp);

            pHash = malloc(RSA_size(RSAPubKey));
            if(!(pHash))
            {
                IPMI_ERROR("Memory allocation failed!!\n");
                free(HashString);
                return -1;
            }
            memset(pHash,0,RemBytes);

            nRet = RSA_public_decrypt(RemBytes, HashString, pHash,RSAPubKey,RSA_PKCS1_PADDING);
            if(nRet==-1)
            {
                IPMI_ERROR("Decrypting the enc-hash failed with return code %d..\n",nRet);
                free(pHash);
                free(HashString);
                return -1;
            }

            if(allocmem == NULL)
            {
                IPMI_ERROR("Decrypting the enc-hash failed with return code %d..\n",nRet);
                free(pHash);
                free(HashString);
                return -1;
            }

            AllocatedMem = allocmem;

            //returns 1 on success and 0 otherwise
            if(SHA1_Init(&c)!=1)
            {
                free(pHash);
                free(HashString);
                return -1;
            }

            for(Count=0;Count<((ImgSize)/(BUFSIZE));Count++)
            {
                nRet = BUFSIZE;
                memcpy(strBuf,AllocatedMem,nRet);
                if(SHA1_Update(&c,strBuf,(unsigned long)nRet)!=1)
                {
                    IPMI_ERROR("Decrypting the enc-hash failed with return code %d..\n",nRet);
                    free(pHash);
                    free(HashString);
                    return -1;
                }
                AllocatedMem += BUFSIZE;
            }

            memset(strHashOfImg,0,SHA_DIGEST_LENGTH);

            if(SHA1_Final(strHashOfImg,&c)!=1)
            {
               IPMI_ERROR("Error in SHA1 Final\n");
                free(pHash);
                free(HashString);
                return -1;
            }

            for(Count=0;Count<SHA_DIGEST_LENGTH;Count++)
            {
                if ( Count == 0)
                {
                    sprintf(str,"%02x",strHashOfImg[Count]);
                }
                else
                {
                    sprintf(str,"%s%02x",str,strHashOfImg[Count]);
                }
                TDBG("%x",strHashOfImg[Count]);
            }
            if(memcmp(str,(char *)pHash,(SHA_DIGEST_LENGTH*2))==0)
            {
                 IPMI_ERROR("IMAGE IS VALID\n");
                 free(pHash);
                 free(HashString);
            }
            else
            {
                IPMI_ERROR("INVALID IMAGE\n");
                free(pHash);
                free(HashString);
                return -1;
            }
        }
        else
        {
            return -2;
        }
      }
    return 0;
}

/*---------------------------------------
 * AMIYAFUEraseCopyFlash
 *---------------------------------------*/
int AMIYAFUEraseCopyFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{

    AMIYAFUEraseCopyFlashReq_T *pAMIYAFUEraseCopyFlashReq = (AMIYAFUEraseCopyFlashReq_T *)pReq;
    int Ret = 0;

if(ActivateFlashStatus == 0x01)
{
    INT8U *MemOffset = NULL;

    AMIYAFUEraseCopyFlashRes_T* pAMIYAFUEraseCopyFlash = (AMIYAFUEraseCopyFlashRes_T*)pRes;

    if(CalculateChksum((char *)&pAMIYAFUEraseCopyFlashReq->Memoffset,pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Datalen) != pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.CRC32chksum)
    {
        LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Seqnum));
    }

    if(pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Datalen == 0x0c)
    {
          pAMIYAFUEraseCopyFlash->EraseCpyFlashRes.Datalen = 0x04;
    }
    else
    {
          LastStatCode = YAFU_CC_INVALID_DATLEN;
          return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Seqnum));
    }

     MemOffset = (INT8U *) pAMIYAFUEraseCopyFlashReq->Memoffset;

        if(ECFstart==0)
        {
              if(threadid == 0) // Thread is not Created Initialize the Mutex
              {
                    OS_THREAD_MUTEX_INIT(YafuThreadMutex, PTHREAD_MUTEX_RECURSIVE);
                    Ret = CheckImageSign();
                    if(Ret == -1)
                    {
                      LastStatCode = YAFU_CC_INVALID_SIGN_IMAGE;
                      return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_SIGN_IMAGE,pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Seqnum));
                    }
                    else if(Ret == -2)
                    {
                       LastStatCode = YAFU_CC_SIGNED_SUPP_NOT_ENABLED;
                      return (AMIYAFUNotAcks(pRes,YAFU_CC_SIGNED_SUPP_NOT_ENABLED,pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Seqnum));
                    }
              }

              WriteMemOff=MemOffset;
              Sizetocpy=pAMIYAFUEraseCopyFlashReq->Sizetocopy;
              Flashoffset=pAMIYAFUEraseCopyFlashReq->Flashoffset;
              Fixoffset=pAMIYAFUEraseCopyFlashReq->Flashoffset;
              //PreserveFlag=pAMIYAFUEraseCopyFlashReq->PreserveFlag;

                if(access("/var/bios_update_selection", F_OK) == 0)
                {
                    int MTDDev;
                    MTDDev=open(gAMIYAFUSwitchFlashDevice.MTDName, O_RDWR);

                    if (MTDDev == -1)
                    {
                        LastStatCode = YAFU_CC_DEV_OPEN_ERR;
                        printf("ERROR: open failed (%s)\n", strerror(errno));
                        return (AMIYAFUNotAcks(pRes,YAFU_CC_DEV_OPEN_ERR,pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Seqnum));
                    }

                    if(YafuVerifyBiosImage(MTDDev, Flashoffset, WriteMemOff, Sizetocpy, gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize) != 0 )
                    {
                        LastStatCode = YAFU_CC_WRITE_ERR;
                        close(MTDDev);
                        return (AMIYAFUNotAcks(pRes,YAFU_CC_WRITE_ERR,pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Seqnum));
                    }
                        close(MTDDev);
                }

                ECFstart=1;
                OS_THREAD_MUTEX_ACQUIRE(&YafuThreadMutex, WAIT_INFINITE);
                if(ECFthreadstate==0)
                {
                    //OS_CREATE_THREAD (EraseCopyFlash, NULL, NULL);
                    pthread_create(&threadid, NULL, EraseCopyFlash, NULL);
                }
                else
                {
                    pthread_mutex_lock(&mutex);
                    pthread_cond_broadcast(&cond);
                    pthread_mutex_unlock(&mutex);
                }
                
                if((LastStatCode != YAFU_ECF_SUCCESS) && (LastStatCode != YAFU_FLASH_ERASE_FAILURE) && (LastStatCode != YAFU_FLASHING_SAME_IMAGE))
                {
                    LastStatCode = pAMIYAFUEraseCopyFlash->CompletionCode = YAFU_CC_NORMAL;
                }
            }
            else
            {
                LastStatCode = pAMIYAFUEraseCopyFlash->CompletionCode = YAFU_ECF_PROGRESS;
            }
    OS_THREAD_MUTEX_RELEASE(&YafuThreadMutex);
    pAMIYAFUEraseCopyFlash->Sizecopied =  pAMIYAFUEraseCopyFlashReq->Sizetocopy;
    pAMIYAFUEraseCopyFlash->EraseCpyFlashRes.Seqnum = pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Seqnum;
    pAMIYAFUEraseCopyFlash->EraseCpyFlashRes.YafuCmd= pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.YafuCmd;
    pAMIYAFUEraseCopyFlash->EraseCpyFlashRes.CRC32chksum = CalculateChksum((char *)&(pAMIYAFUEraseCopyFlash->Sizecopied),sizeof(INT32U));

    return (sizeof(AMIYAFUEraseCopyFlashRes_T));
}
else
{
    LastStatCode = YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Seqnum));
}


}
int AMIYAFUGetECFStatus( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
	AMIYAFUGetECFStatusReq_T *pAMIYAFUGetECFStatusReq = (AMIYAFUGetECFStatusReq_T *)pReq;
    AMIYAFUGetECFStatusRes_T* pAMIYAFUGetECFStatus = (AMIYAFUGetECFStatusRes_T*)pRes;

	if ((LastStatCode != YAFU_CC_NORMAL) && (LastStatCode != YAFU_ECF_SUCCESS) && (LastStatCode != YAFU_FLASHING_SAME_IMAGE))

	{
		return (AMIYAFUNotAcks(pRes,LastStatCode,pAMIYAFUGetECFStatusReq->GetECFStatusReq.Seqnum));
	}
	
	pAMIYAFUGetECFStatus->CompletionCode=YAFU_CC_NORMAL;
	pAMIYAFUGetECFStatus->Status =(INT16U) LastStatCode;
    pAMIYAFUGetECFStatus->Progress= ((Flashoffset-Fixoffset)*100)/(Sizetocpy);
    pAMIYAFUGetECFStatus->GetECFStatusRes.Datalen=0x00;
	pAMIYAFUGetECFStatus->GetECFStatusRes.Seqnum = pAMIYAFUGetECFStatusReq->GetECFStatusReq.Seqnum;
    pAMIYAFUGetECFStatus->GetECFStatusRes.YafuCmd= pAMIYAFUGetECFStatusReq->GetECFStatusReq.YafuCmd;
	pAMIYAFUGetECFStatus->GetECFStatusRes.CRC32chksum = CalculateChksum((char *)&pAMIYAFUGetECFStatus->Status,(INT32U)pAMIYAFUGetECFStatus->GetECFStatusRes.Datalen);
    return ( sizeof( AMIYAFUGetECFStatusRes_T ) );

}

void*  VerifyFlashStatus	(void *pArg)
{
	
    int fd = 0;
    INT32U i,j;
    INT8U *FlashBuf = NULL;
    INT8U *TmpFlashBuf = NULL;
    DWORD StartOffset = 0;
    int retval = 0;
    int ErrCode = 0;
    INT8U VerifyError=0;
    prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

	while(1)
	{
		Vthreadstate=1;
		if(Vstart==1)
		{

      if(access("/var/yafu_cpld_update_selection", F_OK) == 0)
      {
        Vstart=0;             
			  VerifyFlashStatusCode = YAFU_VERIFY_SUCCESS;
			  
        continue;
       }

			if(access("/var/yafu_bios_update_selection", F_OK) != 0)
			{
				if(g_corefeatures.dual_image_support == ENABLED && (gDeviceNode == 1 || gDeviceNode == 0x00))
				{
					if( g_FlashingImage == IMAGE_2 )
						StartOffset = g_coremacros.global_used_flash_size;
				}
			}

			// This hook can be used to perform any OEM verification routines
			// Returning 0 from this hook will revert back to performing default verification sequence
			// Returning <non-zero> from this hook will validate the ErrCode from the hook, and return appropriately. Default action will be skipped
			// BMCInst is not required for this hook, because flashing can be done by only 1 instance at a time.
			if (g_PDKHandle[PDK_OEMVERIFY] != NULL)
   			{
        			retval = ((int(*)(int, INT8U*, unsigned long, int*))g_PDKHandle[PDK_OEMVERIFY])
								(gDeviceNode, VWriteMemOff, Sizetoverify, &ErrCode);

				if (retval != 0)
				{
					if (ErrCode != 0)
					{
						LastStatCode = YAFU_VERIFY_FAILURE;
						VerifyError = 1;
					}
					else
					{
						LastStatCode = YAFU_VERIFY_SUCCESS;
						 VerifyError = 0;
					}
					goto skip_verify;
				}
   			}

			fd = open(gAMIYAFUSwitchFlashDevice.MTDName, O_RDONLY);
			if (fd == -1) {
				VerifyFlashStatusCode = YAFU_CC_DEV_OPEN_ERR;
				printf("ERROR: open failed (%s)\n", strerror(errno));
				return (void*)0; 		  
			}
			VerifyError=0;

			// Loop through the entire image size in memory.  The verify would be handled in blocks of data, each erase block in size
			for(j=0;j<(Sizetoverify/(gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize));j++)
			{
				// Allocate a buffer for storing the block of data to be read from flash.  It will be erase block size
				FlashBuf = malloc ((gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize));
				if(FlashBuf == NULL)
				{
					VerifyFlashStatusCode = YAFU_CC_ALLOC_ERR;
					Vstart=0;
					VerifyError=1;
					break;						

				}
				// Assign it to a tempory pointer for future use
				TmpFlashBuf = FlashBuf;

				// Seek the MTD Device to the appropriate offset
				if ( lseek (fd, StartOffset + VFlashoffset, SEEK_SET) == -1)
				{
					VerifyFlashStatusCode = YAFU_CC_SEEK_ERR;

					fprintf (stderr,
					"seek error on %s: %s\n",
					gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
					Vstart=0;
					free(FlashBuf);
					VerifyError=1;
					break;
				}

				// Read a full erase block of data from the flash device. 
				// Since read is faster than write, we can read a whole block of data in a single shot
				if( (read(fd,FlashBuf,(gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize)))!= (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize))
				{
					VerifyFlashStatusCode = YAFU_CC_READ_ERR;
					fprintf (stderr,
					"Bytes read error %s: %s\n",
					gAMIYAFUSwitchFlashDevice.MTDName, strerror (errno));
					Vstart=0;
					free(FlashBuf);
                    			VerifyError=1;
					break;

				}

				// Compares the read buffer block with a block of data read from the memory.
				// Previously it would compare the entire block in a byte-by-byte manner
				// Now, we have modified it to use memcmp() on a smaller chunk in the block.
				// The chunk size has been defined as 1024 Bytes.
				// Smaller chunks is to allow smaller processing time for each iteration
                                for (i = 0; i < (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize / YAFU_FLASH_CHUNK); i++)
                                {
                                        if (memcmp(FlashBuf, VWriteMemOff, YAFU_FLASH_CHUNK) != 0)
                                        {
                                                VWriteMemOff = VWriteMemOff - (i * YAFU_FLASH_CHUNK);
                                                break;
                                        }

                                        FlashBuf += YAFU_FLASH_CHUNK;
                                        VWriteMemOff += YAFU_FLASH_CHUNK;
                                }

				// If the data was found to be same between the data read from flash and the data in memory,
				// then, it means the block is successfully written. So proceed with then block
				// If the block was found different, then stop this process with an error, since verify failed
                                if (i == (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize / YAFU_FLASH_CHUNK))
                                {
                                        VOffset = (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);
                                }
                                else
                                {
                                        TDBG("FlashBuf after verify: Flash = 0x%08lX Mem = 0x%08lX\n",(INT32U)FlashBuf, (INT32U)VWriteMemOff);
                                        TDBG("Verify Failure \n");
                                        VOffset = (INT32U )VWriteMemOff;
                                        VerifyError = 1;
                                        VerifyFlashStatusCode = YAFU_INVALID_CHKSUM;
                                        break;
                                }

                                FlashBuf = TmpFlashBuf;
                                free (FlashBuf);
                                VFlashoffset += (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);
			}

skip_verify:
			close(fd);
			// This hook can be used to perform an OEM actions if required upon completion of verification phase
			// This will be the last access point before the flashing sequence exits
			// PDK_OEMSaveFMH has not been added separately.  Instead the FMH can be saved from within this POST_VERIFY hook itself	
			// BMCInst is not required for this hook, because flashing can be done by only 1 instance at a time.
			retval = 0;	
			if (g_PDKHandle[PDK_POST_VERIFY] != NULL)
			{
                        	retval = ((int(*)(int, INT8U, int))g_PDKHandle[PDK_POST_VERIFY]) (gDeviceNode, VerifyError, VerifyFlashStatusCode);
                        	if (retval != 0)
                        	{
                                	TCRIT("Error in handling the Post Verify OEM Sequence\n");
                        	}
			}
			
			Vstart=0;
			if (VerifyError==0)
			{
			    VerifyFlashStatusCode = YAFU_VERIFY_SUCCESS;
			}
		}
		else
		{
			pthread_mutex_lock(&mutexv);
			pthread_cond_wait(&condv, &mutexv);
			pthread_mutex_unlock(&mutexv);
		}
	}

}


/*---------------------------------------
 * AMIYAFUVerifyFlash
 *---------------------------------------*/
int AMIYAFUVerifyFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{

    AMIYAFUVerifyFlashReq_T *pAMIYAFUVerifyFlashReq = (AMIYAFUVerifyFlashReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
    INT8U  *MemOffset;

    AMIYAFUVerifyFlashRes_T* pAMIYAFUVerfyFlash = (AMIYAFUVerifyFlashRes_T*)pRes;


   if(CalculateChksum((char *)&pAMIYAFUVerifyFlashReq->Memoffset,(3 * sizeof(INT32U))) != pAMIYAFUVerifyFlashReq->VerifyFlashReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
	  printf("Error in verify flash \n");
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUVerifyFlashReq->VerifyFlashReq.Seqnum));
   }


    if(pAMIYAFUVerifyFlashReq->VerifyFlashReq.Datalen== 0x0c)
          pAMIYAFUVerfyFlash->VerifyFlashRes.Datalen= 0x04;
    else
    {
          LastStatCode = YAFU_CC_INVALID_DATLEN;
          return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUVerifyFlashReq->VerifyFlashReq.Seqnum));
    }


/* wait until Erase Copy Flash to finish */
	while(ECFstart!=0)
	{
//		fprintf(stderr,"\nErase Copy Flash in Progress %d ",ECFstart);
	}
    MemOffset = (INT8U *)pAMIYAFUVerifyFlashReq->Memoffset;
	
	if(Vstart==0)
	{
		VWriteMemOff=MemOffset;
		Sizetoverify=pAMIYAFUVerifyFlashReq->Sizetoverify;
		VFlashoffset=pAMIYAFUVerifyFlashReq->Flashoffset;
		VFixoffset=pAMIYAFUVerifyFlashReq->Flashoffset;
		Vstart=1;
		VerifyFlashStatusCode = 0x00;
		if(Vthreadstate==0)
			{
	  			//OS_CREATE_THREAD (EraseCopyFlash, NULL, NULL);
	  			 pthread_create(&threadidv, NULL, VerifyFlashStatus, NULL);
			}
		else
			{
			pthread_mutex_lock(&mutexv);
			pthread_cond_broadcast(&condv);
			pthread_mutex_unlock(&mutexv);
			
			}
		pAMIYAFUVerfyFlash->CompletionCode = YAFU_CC_NORMAL;
	}
	else
	{
	pAMIYAFUVerfyFlash->CompletionCode = YAFU_VERIFY_PROGRESS;
    
	}
  	pAMIYAFUVerfyFlash->Offset = (gAMIYAFUSwitchFlashDevice.FlashInfo.FlashEraseBlkSize);
    LastStatCode = (INT16U)pAMIYAFUVerfyFlash->CompletionCode;
    pAMIYAFUVerfyFlash->VerifyFlashRes.Seqnum = pAMIYAFUVerifyFlashReq->VerifyFlashReq.Seqnum;
    pAMIYAFUVerfyFlash->VerifyFlashRes.YafuCmd= pAMIYAFUVerifyFlashReq->VerifyFlashReq.YafuCmd;
    pAMIYAFUVerfyFlash->VerifyFlashRes.CRC32chksum = CalculateChksum((char *)&(pAMIYAFUVerfyFlash->Offset),sizeof(INT32U));

    return (sizeof(AMIYAFUVerifyFlashRes_T));
}
else
{
    LastStatCode = YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUVerifyFlashReq->VerifyFlashReq.Seqnum));                       
}

}
int AMIYAFUGetVerifyStatus  ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
		AMIYAFUGetVerifyStatusReq_T *pAMIYAFUGetVerifyStatusReq = (AMIYAFUGetVerifyStatusReq_T *)pReq;
		AMIYAFUGetVerifyStatusRes_T* pAMIYAFUGetVerifyStatus = (AMIYAFUGetVerifyStatusRes_T*)pRes;

		if(LastStatCode!=YAFU_CC_NORMAL&&LastStatCode!=YAFU_VERIFY_SUCCESS)
			{
				return (AMIYAFUNotAcks(pRes,LastStatCode,pAMIYAFUGetVerifyStatusReq->GetVerifyStatusReq.Seqnum));    
			}
	
		pAMIYAFUGetVerifyStatus->CompletionCode=YAFU_CC_NORMAL;
		pAMIYAFUGetVerifyStatus->Status =(INT16U) VerifyFlashStatusCode;
		pAMIYAFUGetVerifyStatus->Offset=VOffset;
		pAMIYAFUGetVerifyStatus->Progress= ((VFlashoffset-VFixoffset)*100)/(Sizetoverify);
		pAMIYAFUGetVerifyStatus->GetVerifyStatusRes.Datalen=0x00;
		pAMIYAFUGetVerifyStatus->GetVerifyStatusRes.Seqnum = pAMIYAFUGetVerifyStatusReq->GetVerifyStatusReq.Seqnum;
		pAMIYAFUGetVerifyStatus->GetVerifyStatusRes.YafuCmd= pAMIYAFUGetVerifyStatusReq->GetVerifyStatusReq.YafuCmd;
		pAMIYAFUGetVerifyStatus->GetVerifyStatusRes.CRC32chksum = CalculateChksum((char *)&(pAMIYAFUGetVerifyStatus->Offset),sizeof(INT32U));
		return ( sizeof( AMIYAFUGetVerifyStatusRes_T ) );

}

int AMIYAFUMiscellaneousInfo ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
    int RetCode = 0;
    AMIYAFUMiscellaneousReq_T *pAMIYAFUMiscellaneousReq = (AMIYAFUMiscellaneousReq_T*)pReq;
    AMIYAFUMiscellaneousRes_T *pAMIYAFUMiscellaneousRes = (AMIYAFUMiscellaneousRes_T*)pRes;

    if(ActivateFlashStatus == 0x01)
    {
        PreserveFlag = pAMIYAFUMiscellaneousReq->PreserveFlag;
    }
    pAMIYAFUMiscellaneousRes->CompletionCode = RetCode;
    return sizeof(AMIYAFUMiscellaneousRes_T);

}

/*---------------------------------------
 * AMIYAFUReadMemory
 *---------------------------------------*/
int AMIYAFUReadMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
     AMIYAFUReadMemoryReq_T *pAMIYAFUReadMemoryReq =(AMIYAFUReadMemoryReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
     INT32U *offsetoread;
     INT8U	*Buf = 0;

    AMIYAFUReadMemoryRes_T* pAMIYAFUReadMemory = (AMIYAFUReadMemoryRes_T*)pRes;

   if(CalculateChksum((char *)&pAMIYAFUReadMemoryReq->Memoffset,pAMIYAFUReadMemoryReq->ReadMemReq.Datalen) != pAMIYAFUReadMemoryReq->ReadMemReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUReadMemoryReq->ReadMemReq.Seqnum));                       
   }

    if(pAMIYAFUReadMemoryReq->ReadMemReq.Datalen != 0x07)
    {
	  LastStatCode = YAFU_CC_INVALID_DATLEN;
         return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUReadMemoryReq->ReadMemReq.Seqnum));                       

    }

    offsetoread = (INT32U *)pAMIYAFUReadMemoryReq->Memoffset;

    Buf = malloc(pAMIYAFUReadMemoryReq->Sizetoread);

      if(Buf == NULL)
      {
	     LastStatCode = YAFU_CC_ALLOC_ERR;
           printf("Unable to allocate memory/n");
           return (AMIYAFUNotAcks(pRes,YAFU_CC_ALLOC_ERR,pAMIYAFUReadMemoryReq->ReadMemReq.Seqnum));                       
      }

    memcpy (( INT8U*) Buf,
              ( INT8U*)offsetoread,
              pAMIYAFUReadMemoryReq->Sizetoread );


    memcpy (( INT8U*) (pAMIYAFUReadMemory + 1),
              ( INT8U*)Buf,
              pAMIYAFUReadMemoryReq->Sizetoread );

    pAMIYAFUReadMemory->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U)pAMIYAFUReadMemory->CompletionCode;
    pAMIYAFUReadMemory->ReadMemRes.Seqnum = pAMIYAFUReadMemoryReq->ReadMemReq.Seqnum;
    pAMIYAFUReadMemory->ReadMemRes.Datalen= pAMIYAFUReadMemoryReq->Sizetoread;
    pAMIYAFUReadMemory->ReadMemRes.CRC32chksum = CalculateChksum((char *)Buf,pAMIYAFUReadMemoryReq->Sizetoread);

    free(Buf);
    return (sizeof(AMIYAFUReadMemoryRes_T) + pAMIYAFUReadMemoryReq->Sizetoread );
}
else
{
    LastStatCode = YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUReadMemoryReq->ReadMemReq.Seqnum));
}

}


/*---------------------------------------
 * AMIYAFUWriteMemory
 *---------------------------------------*/
int AMIYAFUWriteMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{

    AMIYAFUWriteMemoryReq_T *pAMIYAFUWriteMemoryReq = (AMIYAFUWriteMemoryReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
    INT8U *OffsetToWrite;


    AMIYAFUWriteMemoryRes_T* pAMIYAFUWriteMemory = (AMIYAFUWriteMemoryRes_T*)pRes;

    if(CalculateChksum((char *)&pAMIYAFUWriteMemoryReq->Memoffset,pAMIYAFUWriteMemoryReq->WriteMemReq.Datalen) != pAMIYAFUWriteMemoryReq->WriteMemReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
         printf("Error in checksum of write memory \n");
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUWriteMemoryReq->WriteMemReq.Seqnum));
  
   }

    OffsetToWrite = (INT8U *)pAMIYAFUWriteMemoryReq->Memoffset;

    memcpy (( INT8U*) OffsetToWrite,
	 	 (pReq + sizeof(AMIYAFUWriteMemoryReq_T)),
		 (pAMIYAFUWriteMemoryReq->WriteMemReq.Datalen - 5));


    pAMIYAFUWriteMemory->SizeWritten = (pAMIYAFUWriteMemoryReq->WriteMemReq.Datalen - 5);

    pAMIYAFUWriteMemory->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U) pAMIYAFUWriteMemory->CompletionCode;
    pAMIYAFUWriteMemory->WriteMemRes.Seqnum= pAMIYAFUWriteMemoryReq->WriteMemReq.Seqnum;
    pAMIYAFUWriteMemory->WriteMemRes.YafuCmd= pAMIYAFUWriteMemoryReq->WriteMemReq.YafuCmd;
    pAMIYAFUWriteMemory->WriteMemRes.Datalen=0x02;
    pAMIYAFUWriteMemory->WriteMemRes.CRC32chksum =  CalculateChksum((char *)&pAMIYAFUWriteMemory->SizeWritten,sizeof(INT16U));

    return (sizeof(AMIYAFUWriteMemoryRes_T));
}
else
{
   LastStatCode = YAFU_CC_IN_DEACTIVATE;
   return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUWriteMemoryReq->WriteMemReq.Seqnum));
}


}

/*---------------------------------------
 * AMIYAFUCopyMemory
 *---------------------------------------*/
int AMIYAFUCopyMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{

    AMIYAFUCopyMemoryReq_T *pAMIYAFUCopyMemoryReq = (AMIYAFUCopyMemoryReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
     INT8U *SrcOffset;
     INT8U *DestOffset;

     AMIYAFUCopyMemoryRes_T* pAMIYAFUCopyMemory = (AMIYAFUCopyMemoryRes_T*)pRes;

    if(CalculateChksum((char *)&pAMIYAFUCopyMemoryReq->MemoffsetSrc,pAMIYAFUCopyMemoryReq->CopyMemReq.Datalen) != pAMIYAFUCopyMemoryReq->CopyMemReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUCopyMemoryReq->CopyMemReq.Seqnum));                        

   }

   if(pAMIYAFUCopyMemoryReq->CopyMemReq.Datalen== 0x0c)
          pAMIYAFUCopyMemory->CopyMemRes.Datalen= 0x04;
    else
    {
	   LastStatCode = YAFU_CC_INVALID_DATLEN;
          return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUCopyMemoryReq->CopyMemReq.Seqnum));                        

    }

    SrcOffset = (INT8U *)pAMIYAFUCopyMemoryReq->MemoffsetSrc;

    DestOffset = (INT8U *)pAMIYAFUCopyMemoryReq->MemoffsetDest;

     memcpy(DestOffset,SrcOffset,pAMIYAFUCopyMemoryReq->Sizetocopy);

    pAMIYAFUCopyMemory->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U)pAMIYAFUCopyMemory->CompletionCode;
    pAMIYAFUCopyMemory->CopyMemRes.Seqnum = pAMIYAFUCopyMemoryReq->CopyMemReq.Seqnum;
    pAMIYAFUCopyMemory->CopyMemRes.YafuCmd= pAMIYAFUCopyMemoryReq->CopyMemReq.YafuCmd;
    pAMIYAFUCopyMemory->Sizecopied = pAMIYAFUCopyMemoryReq->Sizetocopy;
    pAMIYAFUCopyMemory->CopyMemRes.CRC32chksum= CalculateChksum((char *)&pAMIYAFUCopyMemory->Sizecopied,sizeof(INT32U));

    return (sizeof(AMIYAFUCopyMemoryRes_T));
}
else
{
    LastStatCode =  YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUCopyMemoryReq->CopyMemReq.Seqnum));                        
}

}

/*---------------------------------------
 * AMIYAFUCompareMemory
 *---------------------------------------*/
int AMIYAFUCompareMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst)
{

    AMIYAFUCompareMemoryReq_T *pAMIYAFUCompareMemoryReq = (AMIYAFUCompareMemoryReq_T *)pReq;

if( ActivateFlashStatus == 0x01)
{
   INT8U *FirstOffset;
   INT8U *SecondOffset;
   INT32U i=0;

    AMIYAFUCompareMemoryRes_T* pAMIYAFUCompareMemory = (	AMIYAFUCompareMemoryRes_T*)pRes;

    if(CalculateChksum((char *)&pAMIYAFUCompareMemoryReq->Memoffset1,pAMIYAFUCompareMemoryReq->CmpMemReq.Datalen) != pAMIYAFUCompareMemoryReq->CmpMemReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM,pAMIYAFUCompareMemoryReq->CmpMemReq.Seqnum));                           

   }

    if(pAMIYAFUCompareMemoryReq->CmpMemReq.Datalen== 0x0c)
          pAMIYAFUCompareMemory->CmpMemRes.Datalen= 0x04;
    else
    {
	   LastStatCode = YAFU_CC_INVALID_DATLEN;
          return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN,pAMIYAFUCompareMemoryReq->CmpMemReq.Seqnum));                              

    }

    FirstOffset = (INT8U *) pAMIYAFUCompareMemoryReq->Memoffset1;

    SecondOffset = (INT8U *) pAMIYAFUCompareMemoryReq->Memoffset2;

    for(i=0;i<pAMIYAFUCompareMemoryReq->SizetoCmp;i++)
    	{
    	   if(*FirstOffset == *SecondOffset)
    	   	{
    	   	   FirstOffset++;
                 SecondOffset++;
    	   	}
	    else
	    	{
	    	printf("The value of i = 0x%08X\n", i);
	    	printf("Inside else\n");
	    	break;
	    	}
    	}

    if( i == pAMIYAFUCompareMemoryReq->SizetoCmp)
		pAMIYAFUCompareMemory->Offset = 0x00;
    else
		pAMIYAFUCompareMemory->Offset = (INT32U )FirstOffset;

    pAMIYAFUCompareMemory->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U)pAMIYAFUCompareMemory->CompletionCode;
    pAMIYAFUCompareMemory->CmpMemRes.Seqnum = pAMIYAFUCompareMemoryReq->CmpMemReq.Seqnum;
    pAMIYAFUCompareMemory->CmpMemRes.YafuCmd= pAMIYAFUCompareMemoryReq->CmpMemReq.YafuCmd;
    pAMIYAFUCompareMemory->CmpMemRes.CRC32chksum = CalculateChksum((char *)&pAMIYAFUCompareMemory->Offset,sizeof(INT32U));

    return (sizeof(AMIYAFUCompareMemoryRes_T));
}
else
{
    LastStatCode =  YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE,pAMIYAFUCompareMemoryReq->CmpMemReq.Seqnum));                              
}

}

/*---------------------------------------
 * AMIYAFUClearMemory
 *---------------------------------------*/
int AMIYAFUClearMemory ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
     AMIYAFUClearMemoryReq_T *pAMIYAFUClearMemoryReq = (AMIYAFUClearMemoryReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
    INT8U *Offset;
    AMIYAFUClearMemoryRes_T* pAMIYAFUClearMemory = (AMIYAFUClearMemoryRes_T*)pRes;

    if(CalculateChksum((char *)&pAMIYAFUClearMemoryReq->MemofftoClear,pAMIYAFUClearMemoryReq->ClearMemReq.Datalen) != pAMIYAFUClearMemoryReq->ClearMemReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM, pAMIYAFUClearMemoryReq->ClearMemReq.Seqnum));                                 
   }

    if(pAMIYAFUClearMemoryReq->ClearMemReq.Datalen== 0x08)
          pAMIYAFUClearMemory->ClearMemRes.Datalen= 0x04;
    else
    {
	   LastStatCode = YAFU_CC_INVALID_DATLEN;
          return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN, pAMIYAFUClearMemoryReq->ClearMemReq.Seqnum));                                 
    }

    Offset = (INT8U *)pAMIYAFUClearMemoryReq->MemofftoClear;

    memset(Offset,0,pAMIYAFUClearMemoryReq->SizetoClear);

    pAMIYAFUClearMemory->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = pAMIYAFUClearMemory->CompletionCode;
    pAMIYAFUClearMemory->ClearMemRes.Seqnum = pAMIYAFUClearMemoryReq->ClearMemReq.Seqnum;
    pAMIYAFUClearMemory->ClearMemRes.YafuCmd=pAMIYAFUClearMemoryReq->ClearMemReq.YafuCmd;
    pAMIYAFUClearMemory->SizeCleared=pAMIYAFUClearMemoryReq->SizetoClear;
    pAMIYAFUClearMemory->ClearMemRes.CRC32chksum = CalculateChksum((char *)&pAMIYAFUClearMemory->SizeCleared,sizeof(INT32U));

    return (sizeof(AMIYAFUClearMemoryRes_T));
}
else
{
    LastStatCode =  YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE, pAMIYAFUClearMemoryReq->ClearMemReq.Seqnum));                                 
}

}

/*---------------------------------------
 * AMIYAFUGetBootConfig
 *---------------------------------------*/
int AMIYAFUGetBootConfig ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{

    AMIYAFUGetBootConfigReq_T *pAMIYAFUGetBootConfigReq = (AMIYAFUGetBootConfigReq_T *)pReq;
    char *Buffer = NULL;
    int len = 0;

if( ActivateFlashStatus == 0x01)
{
     int Retval;

    AMIYAFUGetBootConfigRes_T* pAMIYAFUGetBootConfig = (AMIYAFUGetBootConfigRes_T*)pRes;

    if(CalculateChksum((char *)&pAMIYAFUGetBootConfigReq->VarName[0],pAMIYAFUGetBootConfigReq->GetBootReq.Datalen) != pAMIYAFUGetBootConfigReq->GetBootReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM, pAMIYAFUGetBootConfigReq->GetBootReq.Seqnum));
   }

    if(pAMIYAFUGetBootConfigReq->GetBootReq.Datalen == 0x41)
          pAMIYAFUGetBootConfig->GetBootRes.Datalen= 0x42;
    else
    {
	   LastStatCode =  YAFU_CC_INVALID_DATLEN;
          return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN, pAMIYAFUGetBootConfigReq->GetBootReq.Seqnum));
    }
    Buffer = malloc (MAX_BOOTVAL_LENGTH);
    if(Buffer == NULL)
    {
        LastStatCode=YAFU_CC_ALLOC_ERR;
        return (AMIYAFUNotAcks(pRes,YAFU_CC_ALLOC_ERR,pAMIYAFUGetBootConfigReq->GetBootReq.Seqnum));

    }
    memset(Buffer,0,MAX_BOOTVAL_LENGTH);
    Retval= GetUBootParam((char *)&pAMIYAFUGetBootConfigReq->VarName,Buffer);

    len = strlen(Buffer);
    len++;

    if(Retval != 0)
         pAMIYAFUGetBootConfig->Status = 0x00;
   else
   	  pAMIYAFUGetBootConfig->Status = 0x01;

    pAMIYAFUGetBootConfig->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U)pAMIYAFUGetBootConfig->CompletionCode;
    pAMIYAFUGetBootConfig->GetBootRes.Seqnum = pAMIYAFUGetBootConfigReq->GetBootReq.Seqnum;
    pAMIYAFUGetBootConfig->GetBootRes.YafuCmd=pAMIYAFUGetBootConfigReq->GetBootReq.YafuCmd;

    memcpy (( INT8U*) (pAMIYAFUGetBootConfig + 1),
                   ( INT8U*)Buffer,len );
    pAMIYAFUGetBootConfig->GetBootRes.Datalen = len + 1;  //one for status
    pAMIYAFUGetBootConfig->GetBootRes.CRC32chksum= CalculateChksum((char *)&pAMIYAFUGetBootConfig->Status,pAMIYAFUGetBootConfig->GetBootRes.Datalen);

    free(Buffer);
    return (sizeof(AMIYAFUGetBootConfigRes_T) + len);
}
else
{
    LastStatCode =   YAFU_CC_IN_DEACTIVATE;
    return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE, pAMIYAFUGetBootConfigReq->GetBootReq.Seqnum));
}

}

/*---------------------------------------
 * AMIYAFUSetBootConfig
 *---------------------------------------*/
int AMIYAFUSetBootConfig ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
    AMIYAFUSetBootConfigReq_T *pAMIYAFUSetBootConfigReq = (AMIYAFUSetBootConfigReq_T *)pReq;

if( ActivateFlashStatus == 0x01)
{

    int RetVal=0;
    char *BootVal = NULL;

    AMIYAFUSetBootConfigRes_T* pAMIYAFUSetBootConfig = (AMIYAFUSetBootConfigRes_T*)pRes;

    if(CalculateChksum((char *)&pAMIYAFUSetBootConfigReq->VarName[0],pAMIYAFUSetBootConfigReq->SetBootReq.Datalen) != pAMIYAFUSetBootConfigReq->SetBootReq.CRC32chksum)
    {
	  LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM, pAMIYAFUSetBootConfigReq->SetBootReq.Seqnum));
    }


    BootVal = malloc (pAMIYAFUSetBootConfigReq->SetBootReq.Datalen - 65);
    if(BootVal  == NULL)
    {
        LastStatCode=YAFU_CC_ALLOC_ERR;
        return (AMIYAFUNotAcks(pRes,YAFU_CC_ALLOC_ERR,pAMIYAFUSetBootConfigReq->SetBootReq.Seqnum));

    }
    pReq += sizeof(AMIYAFUSetBootConfigReq_T);
    memcpy(BootVal,pReq,(pAMIYAFUSetBootConfigReq->SetBootReq.Datalen - 65));
    memset(pReq,0,(pAMIYAFUSetBootConfigReq->SetBootReq.Datalen - 65));
    pReq -= sizeof(AMIYAFUSetBootConfigReq_T);

    RetVal = SetUBootParam((char *)pAMIYAFUSetBootConfigReq->VarName,BootVal);
    if(RetVal != 0)
    {
        pAMIYAFUSetBootConfig->Status = 0x00;
    }
    else
    {
        pAMIYAFUSetBootConfig->Status = 0x01;
    }

    pAMIYAFUSetBootConfig->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U)pAMIYAFUSetBootConfig->CompletionCode;
    pAMIYAFUSetBootConfig->SetBootRes.Seqnum = pAMIYAFUSetBootConfigReq->SetBootReq.Seqnum;
    pAMIYAFUSetBootConfig->SetBootRes.YafuCmd = pAMIYAFUSetBootConfigReq->SetBootReq.YafuCmd;
    pAMIYAFUSetBootConfig->SetBootRes.Datalen = 0x01;
    pAMIYAFUSetBootConfig->SetBootRes.CRC32chksum= CalculateChksum((char *)&pAMIYAFUSetBootConfig->Status,pAMIYAFUSetBootConfig->SetBootRes.Datalen );

    free(BootVal);
    return (sizeof(AMIYAFUSetBootConfigRes_T));
}
else
{
   LastStatCode =  YAFU_CC_IN_DEACTIVATE;
   return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE, pAMIYAFUSetBootConfigReq->SetBootReq.Seqnum));
}

}

/*---------------------------------------
 * AMIYAFUGetBootVars
 *---------------------------------------*/
int AMIYAFUGetBootVars ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
    AMIYAFUGetBootVarsReq_T *pAMIYAFUGetBootVarsReq = (AMIYAFUGetBootVarsReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
    //int RetVal;
    char *Buffer;
    int BootVarlen =0;


    AMIYAFUGetBootVarsRes_T* pAMIYAFUGetBootVars = (AMIYAFUGetBootVarsRes_T*)pRes;
    pAMIYAFUGetBootVars->VarCount = 0X00;


    Buffer = malloc (MAX_BOOTVAR_LENGTH);
    if(Buffer == NULL)
    {
        LastStatCode=YAFU_CC_ALLOC_ERR;
        return (AMIYAFUNotAcks(pRes,YAFU_CC_ALLOC_ERR,pAMIYAFUGetBootVarsReq->GetBootReq.Seqnum));
    }

    GetAllUBootParam ((char *)&pAMIYAFUGetBootVars->VarCount,Buffer,&BootVarlen);

    memcpy (( INT8U*) (pAMIYAFUGetBootVars + 1),
                   ( INT8U*)Buffer,BootVarlen );

    pAMIYAFUGetBootVars->GetBootRes.Datalen = BootVarlen + 1; //One added for Variable count

    pAMIYAFUGetBootVars->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U)pAMIYAFUGetBootVars->CompletionCode;
    pAMIYAFUGetBootVars->GetBootRes.Seqnum = pAMIYAFUGetBootVarsReq->GetBootReq.Seqnum;
    pAMIYAFUGetBootVars->GetBootRes.YafuCmd = pAMIYAFUGetBootVarsReq->GetBootReq.YafuCmd;
    pAMIYAFUGetBootVars->GetBootRes.CRC32chksum= CalculateChksum((char *)&pAMIYAFUGetBootVars->VarCount,pAMIYAFUGetBootVars->GetBootRes.Datalen);

    free(Buffer);
    return (sizeof(AMIYAFUGetBootVarsRes_T)+ BootVarlen);
}
else
{
   LastStatCode = YAFU_CC_IN_DEACTIVATE;
   return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE, pAMIYAFUGetBootVarsReq->GetBootReq.Seqnum));
}

}



/*---------------------------------------
 * AMIYAFUDeactivateFlash
 *---------------------------------------*/
int AMIYAFUDeactivateFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
    char Cmd[128];
    AMIYAFUDeactivateFlashReq_T *pAMIYAFUDeactivateFlashReq = (AMIYAFUDeactivateFlashReq_T *)pReq;

if(ActivateFlashStatus == 0x01)
{
    AMIYAFUDeactivateFlashRes_T* pAMIYAFUDeactivateFlash = (AMIYAFUDeactivateFlashRes_T*)pRes;

    if(pAMIYAFUDeactivateFlashReq->DeactivateFlashReq.Datalen== 0x00)
          pAMIYAFUDeactivateFlash->DeactivateFlashRes.Datalen= 0x01;
    else
    {
          LastStatCode = YAFU_CC_INVALID_DATLEN;
          return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN, pAMIYAFUDeactivateFlashReq->DeactivateFlashReq.Seqnum));
    }

    ActivateFlashStatus = 0x00;

	  if(access("/var/yafu_cpld_update_selection", F_OK) == 0)
	  {
	  	StartInitFlashSetting();        
	  	sprintf(Cmd, "rm /var/yafu_cpld_update_selection");
	  	safe_system(Cmd);
	  }
	
    /*Need to terminate YafuTimer thread for every successfull flash complete */
    pthread_cancel(threadid_yafu); 
    threadid_yafu=0;
 
    if(access("/var/yafu_bios_update_selection", F_OK) == 0)
    {
        StartInitFlashSetting();
        sprintf(Cmd, "rm %s", FLASHER_COMPLETION_FILE_INIT7);
        safe_system(Cmd);
        sprintf(Cmd, "/sbin/rmmod host_spi_flash_hw");
        safe_system(Cmd);
        sprintf(Cmd, "/sbin/rmmod host_spi_flash");
        safe_system(Cmd);
        sprintf(Cmd, "rm /var/yafu_bios_update_selection");
        safe_system(Cmd);
    }

    if(g_corefeatures.online_flashing_support == ENABLED)
    {
        unlink(FLASHER_INIT_START);
        unlink(FLASHER_COMPLETION_FILE_INIT7);
    }
    pAMIYAFUDeactivateFlash->Status = 0x00;
    pAMIYAFUDeactivateFlash->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U) pAMIYAFUDeactivateFlash->CompletionCode;
    pAMIYAFUDeactivateFlash->DeactivateFlashRes.Seqnum = pAMIYAFUDeactivateFlashReq->DeactivateFlashReq.Seqnum;
    pAMIYAFUDeactivateFlash->DeactivateFlashRes.YafuCmd = pAMIYAFUDeactivateFlashReq->DeactivateFlashReq.YafuCmd;
    pAMIYAFUDeactivateFlash->DeactivateFlashRes.CRC32chksum= CalculateChksum((char *)&pAMIYAFUDeactivateFlash->Status,sizeof(INT8U));

    if(g_corefeatures.dual_image_support == ENABLED && access("/var/bios_update_selection", F_OK) != 0 && (gDeviceNode == 1 || gDeviceNode == 0x00))
    {
        SetMostRecentlyProgFW();
		if(g_corefeatures.fail_safe_booting == ENABLED)
			ClearFailsafeBootErrorCodes();
    }
    session_count = 0;

    return (sizeof(AMIYAFUDeactivateFlashRes_T));
}
else
{
    LastStatCode = YAFU_CC_IN_DEACTIVATE;
   return (AMIYAFUNotAcks(pRes,YAFU_CC_IN_DEACTIVATE, pAMIYAFUDeactivateFlashReq->DeactivateFlashReq.Seqnum));                                    
}

}

/*---------------------------------------
 * AMIYAFUResetDevice
 *---------------------------------------*/
int AMIYAFUResetDevice ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{

    AMIYAFUResetDeviceReq_T *pAMIYAFUResetDeviceReq = (AMIYAFUResetDeviceReq_T *)pReq;
    AMIYAFUResetDeviceRes_T* pAMIYAFUResetDevice = (AMIYAFUResetDeviceRes_T*)pRes;
    // YafuflashStatus is updated to terminate the YafuTimerThread
    YafuflashStatus = YAFU_FLASH_SUCCEEDED;

     if(CalculateChksum((char *)&pAMIYAFUResetDeviceReq->WaitSec,sizeof(INT16U)) != pAMIYAFUResetDeviceReq->ResetReq.CRC32chksum)
   {
         LastStatCode=YAFU_INVALID_CHKSUM;
         return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM, pAMIYAFUResetDeviceReq->ResetReq.Seqnum));                                    

   }

    if(pAMIYAFUResetDeviceReq->ResetReq.Datalen== 0x02)
          pAMIYAFUResetDevice->ResetRes.Datalen= 0x01;
    else
    {
	   LastStatCode =   YAFU_CC_INVALID_DATLEN;
          return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN, pAMIYAFUResetDeviceReq->ResetReq.Seqnum));                                      

    }

   if(access("/var/yafu_bios_update_selection", F_OK) != 0)
   {
        if(g_corefeatures.dual_image_support == ENABLED && g_corefeatures.common_conf == ENABLED && (gDeviceNode == 1 || gDeviceNode == 0x00))
        {
            if(DualImgPreserveConf == FALSE)
            {
                safe_system("rm -rf /conf/*");
            }
        }
   }

    RestartBMC_Flasher();
	
    pAMIYAFUResetDevice->Status = 0x01;
    pAMIYAFUResetDevice->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U) pAMIYAFUResetDevice->CompletionCode;
    pAMIYAFUResetDevice->ResetRes.Seqnum = pAMIYAFUResetDeviceReq->ResetReq.Seqnum;
    pAMIYAFUResetDevice->ResetRes.YafuCmd = pAMIYAFUResetDeviceReq->ResetReq.YafuCmd;
    pAMIYAFUResetDevice->ResetRes.CRC32chksum=CalculateChksum((char *)&pAMIYAFUResetDevice->Status,sizeof(INT8U));

    return (sizeof(AMIYAFUResetDeviceRes_T));
}

int AMIYAFUFWSelectFlash ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
    AMIYAFUFWSelectFlashModeReq_T *pAMIYAFUFWSelectFlashModeReq = (AMIYAFUFWSelectFlashModeReq_T *)pReq;
    AMIYAFUFWSelectFlashModeRes_T* pAMIYAFUFWSElectFlashMode = (AMIYAFUFWSelectFlashModeRes_T*)pRes;

    if(CalculateChksum((char *)&pAMIYAFUFWSelectFlashModeReq->fwselect,sizeof(INT8U)) != pAMIYAFUFWSelectFlashModeReq->fwselectflashReq.CRC32chksum)
    {
        LastStatCode=YAFU_INVALID_CHKSUM;
        return (AMIYAFUNotAcks(pRes,YAFU_INVALID_CHKSUM, pAMIYAFUFWSelectFlashModeReq->fwselectflashReq.Seqnum));
    }

    if(pAMIYAFUFWSelectFlashModeReq->fwselectflashReq.Datalen == 0x01)
        pAMIYAFUFWSElectFlashMode->fwselectflashRes.Datalen = 0x01;
    else
    {
        LastStatCode =   YAFU_CC_INVALID_DATLEN;
        return (AMIYAFUNotAcks(pRes,YAFU_CC_INVALID_DATLEN, pAMIYAFUFWSelectFlashModeReq->fwselectflashReq.Seqnum));
    }

    if (pAMIYAFUFWSelectFlashModeReq->fwselect == FW_BMC )
    {
        safe_system("rm -f /var/yafu_bios_update_selection");
        safe_system("rm -f /var/yafu_cpld_update_selection");
    }
    else if (pAMIYAFUFWSelectFlashModeReq->fwselect == FW_BIOS)
    {
        safe_system("touch /var/yafu_bios_update_selection");
    }

    pAMIYAFUFWSElectFlashMode->CompletionCode = YAFU_CC_NORMAL;
    LastStatCode = (INT16U) pAMIYAFUFWSElectFlashMode->CompletionCode;
    pAMIYAFUFWSElectFlashMode->fwselectflashRes.Seqnum = pAMIYAFUFWSelectFlashModeReq->fwselectflashReq.Seqnum;
    pAMIYAFUFWSElectFlashMode->fwselectflashRes.YafuCmd = pAMIYAFUFWSelectFlashModeReq->fwselectflashReq.YafuCmd;

    return (sizeof(AMIYAFUResetDeviceRes_T));
}
#endif


/**
 * @fn CheckForBootOption
 * @brief Check for valid boot selector option.
 * @param[in] bootoption - boot selector option.
 * @retval      0 - for valid boot selector option.
 *              1 - invalid boot option.
 */
int CheckForBootOption(char bootoption)
{
    //Checking for valid Boot selector option
    if(  (bootoption != AUTO_HIGH_VER_FW) && 
         (bootoption != LOWER_IMAGE_FW ) && 
         (bootoption != HIGHER_IMAGE_FW ) && 
         (bootoption != AUTO_LOW_VER_FW) &&
         (bootoption != MOST_RECENTLY_PROG_FW) && 
         (bootoption != LEAST_RECENTLY_PROG_FW) )
    {
        return -1;
    }

return 0;
}

/*
*@fn AMIGetNMChNum
*@brief This command helps in framing send message command if the Sensor Owner ID is
             other than BMC like Node Manager.This retrives the correct IPMB channel associated 
             with the device.
*@param pReq - Request for the command   
*@param ReqLen - Request length for the command
*@param pRes - Respose for the command
*@return Returns size of AMIGetChNumRes_T
*/
int AMIGetNMChNum ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst ) 
{
    AMIGetChNumRes_T* pAMIGetChNumRes = (AMIGetChNumRes_T*) pRes;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    if(g_corefeatures.node_manager != ENABLED)
    {
        *pRes = CC_DEVICE_NOT_SUPPORTED;
        return sizeof(*pRes);
    }
    
    if(NM_PRIMARY_IPMB_BUS == pBMCInfo->NMConfig.NM_IPMBBus)
    {
    	pAMIGetChNumRes->ChannelNum = pBMCInfo->PrimaryIPMBCh;
    }
    else if(NM_SECONDARY_IPMB_BUS == pBMCInfo->NMConfig.NM_IPMBBus)
    {
    	pAMIGetChNumRes->ChannelNum = pBMCInfo->SecondaryIPMBCh;
    }
    else if(NM_THIRD_IPMB_BUS == pBMCInfo->NMConfig.NM_IPMBBus)
    {
    	pAMIGetChNumRes->ChannelNum = pBMCInfo->ThirdIPMBCh;
    }
    pAMIGetChNumRes->CompletionCode = CC_NORMAL;

    return sizeof(AMIGetChNumRes_T);
}

/**
*@fn AMIGetEthIndex
*@brief This command helps in getting the Corresponding EthIndex Value
           for the requested Channel Number
*@param pReq - Request for the command   
*@param ReqLen - Request length for the command
*@param pRes - Respose for the command
*@return Returns size of AMIGetEthIndexReq_T
*/

int AMIGetEthIndex ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,_NEAR_ int BMCInst) 
{
    INT8U EthIndex = 0;
    AMIGetEthIndexReq_T *pAMIGetEthIndexReq = (AMIGetEthIndexReq_T *) pReq;
    AMIGetEthIndexRes_T *pAMIGetEthIndexRes = (AMIGetEthIndexRes_T *) pRes;

    pAMIGetEthIndexRes->CompletionCode = CC_NORMAL;

    EthIndex = GetEthIndex(pAMIGetEthIndexReq->ChannelNum, BMCInst);
    pAMIGetEthIndexRes->EthIndex=EthIndex;

    return sizeof(AMIGetEthIndexRes_T);
}


/*-----------------------------------
* Functions used by internal API
*-----------------------------------*/


/**
 * *@fn AMIGetFruDetails
 * *@brief This command can get the total fru and the repective fru ids
 *            *@param pReq - Request for the command
 *            *@param ReqLen - Request length for the command
 *            *@param pRes - Respose for the command
 *            */

int AMIGetFruDetails( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,int BMCInst )
{
    AMIGetFruDetailReq_T *pAMIFruDetailReq = (AMIGetFruDetailReq_T *) pReq;
    AMIGetFruDetailRes_T *pAMIFruDetailRes = (AMIGetFruDetailRes_T *) pRes;
     _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

     memset((char *)pRes,0,sizeof(AMIGetFruDetailRes_T));

    if(pAMIFruDetailReq->FruReq==0xFF)
    {                                                 	       
        pAMIFruDetailRes->TotalFru = pBMCInfo->FRUConfig.total_frus;
        pAMIFruDetailRes->CompletionCode = CC_NORMAL;
        return sizeof(pAMIFruDetailRes->TotalFru) + sizeof(pAMIFruDetailRes->CompletionCode);
    }
    else if(pAMIFruDetailReq->FruReq < pBMCInfo->FRUConfig.total_frus)
    {
        pAMIFruDetailRes->TotalFru = pBMCInfo->FRUConfig.total_frus;
        pAMIFruDetailRes->DeviceNo = pBMCInfo->FRUConfig.m_FRUInfo[pAMIFruDetailReq->FruReq]->DeviceID;
        strcpy((char *)pAMIFruDetailRes->FRUName,pBMCInfo->FRUConfig.m_FRUInfo[pAMIFruDetailReq->FruReq]->FRUName);
        pAMIFruDetailRes->CompletionCode = CC_NORMAL;
        return sizeof(AMIGetFruDetailRes_T);	
    }
    else
    {
        pAMIFruDetailRes->CompletionCode = CC_PARAM_OUT_OF_RANGE;
        return sizeof(AMIGetFruDetailRes_T);	
    }	
    return 0;	
}

/**
 * *@fn AMISetTriggerEvent
 * *@brief This command is used to enable/disable trigger event based on the parameter input
 * *@param pReq - Request for the command
 * *@param ReqLen - Request length for the command
 * *@param pRes - Response for the command
 * *@param BMCInst - BMC Instance Value
 **/
int AMISetTriggerEvent( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst )
{
    _NEAR_ AMISetTriggerEventReq_T *pAMISetTriggerEventReq = (AMISetTriggerEventReq_T *)pReq;
    _NEAR_ AMISetTriggerEventRes_T *pAMISetTriggerEventRes = (AMISetTriggerEventRes_T *)pRes;
    INT32U LocalTime,TriggerTime;
    INT32U TriggerLength = 0;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    TriggerLength = TriggerEventParamLength[pAMISetTriggerEventReq->TriggerParam-1];

    if(TriggerLength != 0)
    {
        if((ReqLen - 1) != TriggerLength)
        {
            *pRes = CC_REQ_INV_LEN;
            TDBG("Your entered Parameter %d is not a valid data \n",pAMISetTriggerEventReq->TriggerParam);
            return sizeof (INT8U);
        }
    }
    else
    {
         *pRes = CC_INV_DATA_FIELD;
           return sizeof(INT8U);
    }
    //In case of enabling/disabling pre crash/boot recording EnableDisableFlag may be >1 so the below check
    if( PRE_EVENT_RECORDING_FLAG == pAMISetTriggerEventReq->TriggerParam ) 
    {
        if( 2 < pAMISetTriggerEventReq->EnableDisableFlag)
        {
            *pRes = CC_INV_DATA_FIELD;
            printf("Your entered Parameter %d is not a valid data \n",pAMISetTriggerEventReq->TriggerParam);
            return sizeof (INT8U);
        }
    }
    else
    {
        if(1< pAMISetTriggerEventReq->EnableDisableFlag)
        {
            *pRes = CC_INV_DATA_FIELD;
            printf("Your entered Parameter %d is not a valid data \n",pAMISetTriggerEventReq->TriggerParam);
            return sizeof (INT8U);
        }
    }

    switch(pAMISetTriggerEventReq->TriggerParam)
    {
        case CRITICAL:
            pBMCInfo->TriggerEvent.CriticalFlag = pAMISetTriggerEventReq->EnableDisableFlag;
            break;
        case NON_CRITICAL:
            pBMCInfo->TriggerEvent.NONCriticalFlag = pAMISetTriggerEventReq->EnableDisableFlag;
              break;
          case NON_RECOVERABLE:
              pBMCInfo->TriggerEvent.NONRecoverableFlag= pAMISetTriggerEventReq->EnableDisableFlag;
              break;
          case FAN_TROUBLED:
              pBMCInfo->TriggerEvent.FanTroubled= pAMISetTriggerEventReq->EnableDisableFlag;
              break;
          case BMC_WDT_EXPIRE:
              pBMCInfo->TriggerEvent.WDTTimeExpire = pAMISetTriggerEventReq->EnableDisableFlag;
              break;
          case SYS_DC_ON:
              pBMCInfo->TriggerEvent.SysDConFlag= pAMISetTriggerEventReq->EnableDisableFlag;
              break;
          case SYS_DC_OFF:
              pBMCInfo->TriggerEvent.SysDCoffFlag= pAMISetTriggerEventReq->EnableDisableFlag;
              break;
          case SYS_RESET:
              pBMCInfo->TriggerEvent.SysResetFlag = pAMISetTriggerEventReq->EnableDisableFlag;
              break;
          case SPEC_DATE_TIME:
              if(pAMISetTriggerEventReq->EnableDisableFlag)
              {
                  if(UNSPECIFIED_UTC_OFFSET == ((INT16)pBMCInfo->GenConfig.SELTimeUTCOffset))
                  {
                      LocalTime = GET_SYSTEM_TIME_STAMP ();
                  }
                  else
                  {
                      LocalTime = GET_SYSTEM_TIME_STAMP() + (((INT16)pBMCInfo->GenConfig.SELTimeUTCOffset) * 60);
                  }

                  TriggerTime = ipmitoh_u32(pAMISetTriggerEventReq->TriggerData.Time);
                  if(TriggerTime<LocalTime)
                  {
                      TDBG("Invalid Time Specified\n");
                     *pRes = OEMCC_INV_DATE_TIME;
                          return sizeof(INT8U);
                  }
                  pBMCInfo->TriggerEvent.Time = pAMISetTriggerEventReq->TriggerData.Time;
              }
              pBMCInfo->TriggerEvent.SpecDateTime = pAMISetTriggerEventReq->EnableDisableFlag;
              break;
          case LPC_RESET_FLAG:
        	  pBMCInfo->TriggerEvent.LPCResetFlag = pAMISetTriggerEventReq->EnableDisableFlag;
        	  break;
          case PRE_EVENT_RECORDING_FLAG:
              if( 0 == pAMISetTriggerEventReq->EnableDisableFlag )
              {
                    pBMCInfo->TriggerEvent.PreBSODRcdFlag = pAMISetTriggerEventReq->EnableDisableFlag;
                    pBMCInfo->TriggerEvent.PreLPCRcdFlag = pAMISetTriggerEventReq->EnableDisableFlag;
                    //If event monitoring disabled need to stop continuous recording
                    if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE] != NULL)
                    {
                        if( 0 > ((int(*)(int , int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE])(STOP_CONT_RCD,0) )
                            IPMI_ERROR("Error in Writing data to Adviser\n");
                    }

              }
              else if( 1 == pAMISetTriggerEventReq->EnableDisableFlag )
              {
                    pBMCInfo->TriggerEvent.PreBSODRcdFlag = pAMISetTriggerEventReq->EnableDisableFlag; //Enable crash event monitoring
                    pBMCInfo->TriggerEvent.PreLPCRcdFlag = 0;//Disable reset event monitoring
              }
              else
              {
                    pBMCInfo->TriggerEvent.PreBSODRcdFlag = 0;//Disable crash event monitoring
                    pBMCInfo->TriggerEvent.PreLPCRcdFlag = 1;//Enable reset monitoring
              }
              //If any one of the flag enabled to monitor need to start continuous recording
              if( ( pBMCInfo->TriggerEvent.PreBSODRcdFlag ) || ( pBMCInfo->TriggerEvent.PreLPCRcdFlag ) )
              {
                    if(g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE] != NULL)
                    {
                        if ( 0 > ((int(*)(int , int))g_PDKRemoteKVMHandle[PDKREMOTEKVM_WRITECMDADVISERPIPE])(START_CONT_RCD,0) )
                            IPMI_ERROR("Error in Writing data to Adviser\n");
                    }

              }
              break;
           default:
                  pAMISetTriggerEventRes->CompletionCode = CC_PARAM_NOT_SUPPORTED;
                  return sizeof(INT8U);
       }

       pAMISetTriggerEventRes->CompletionCode = CC_NORMAL;
       FlushIPMI((INT8U*)&pBMCInfo->TriggerEvent,(INT8U*)&pBMCInfo->TriggerEvent,pBMCInfo->IPMIConfLoc.TriggerEventAddr,
                         sizeof(TriggerEventCfg_T),BMCInst);
       return sizeof(*pAMISetTriggerEventRes);
  }

/**
 * *@fn AMIGetTriggerEvent
 * *@brief This command is used to get the enable/disable trigger event based on the parameter input
 * *@param pReq - Request for the command
 * *@param ReqLen - Request length for the command
 * *@param pRes - Response for the command
 * *@param BMCInst - BMC Instance Value
 **/
int AMIGetTriggerEvent( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst )
{
    _NEAR_ AMIGetTriggerEventReq_T *pAMIGetTriggerEventReq = (AMIGetTriggerEventReq_T *)pReq;
    _NEAR_ AMIGetTriggerEventRes_T *pAMIGetTriggerEventRes = (AMIGetTriggerEventRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];

    pAMIGetTriggerEventRes->AMIGetTriggerEvent.CompletionCode = CC_NORMAL;
    switch(pAMIGetTriggerEventReq->TriggerParam)
    {
        case CRITICAL:
            pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = pBMCInfo->TriggerEvent.CriticalFlag;
            return sizeof(AMIGetTriggerEvent_T);
        case NON_CRITICAL:
           pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = pBMCInfo->TriggerEvent.NONCriticalFlag;
            return sizeof(AMIGetTriggerEvent_T);
        case NON_RECOVERABLE:
            pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = pBMCInfo->TriggerEvent.NONRecoverableFlag;
            return sizeof(AMIGetTriggerEvent_T);
        case FAN_TROUBLED:
            pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = pBMCInfo->TriggerEvent.FanTroubled;
            return sizeof(AMIGetTriggerEvent_T);
        case BMC_WDT_EXPIRE:
            pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = pBMCInfo->TriggerEvent.WDTTimeExpire;
            return sizeof(AMIGetTriggerEvent_T);
        case SYS_DC_ON:
            pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = pBMCInfo->TriggerEvent.SysDConFlag;
            return sizeof(AMIGetTriggerEvent_T);
        case SYS_DC_OFF:
            pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = pBMCInfo->TriggerEvent.SysDCoffFlag;
            return sizeof(AMIGetTriggerEvent_T);
        case SYS_RESET:
            pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = pBMCInfo->TriggerEvent.SysResetFlag;
            return sizeof(AMIGetTriggerEvent_T);
        case SPEC_DATE_TIME:
            pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag =pBMCInfo->TriggerEvent.SpecDateTime;
            pAMIGetTriggerEventRes->TriggerData.Time =pBMCInfo->TriggerEvent.Time;
            if(pBMCInfo->TriggerEvent.Time == 0)
               pAMIGetTriggerEventRes->TriggerData.Time = GET_SYSTEM_TIME_STAMP();
            return sizeof(AMIGetTriggerEvent_T)+sizeof(pAMIGetTriggerEventRes->TriggerData.Time);
        case LPC_RESET_FLAG:
        	pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag =pBMCInfo->TriggerEvent.LPCResetFlag;
        	return sizeof(AMIGetTriggerEvent_T);
        case PRE_EVENT_RECORDING_FLAG:
            //return which Flag is set
            if( ( !pBMCInfo->TriggerEvent.PreBSODRcdFlag ) && ( !pBMCInfo->TriggerEvent.PreLPCRcdFlag ) )
            {
                pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = 0;
            }
            else if( pBMCInfo->TriggerEvent.PreBSODRcdFlag )
            {
                 pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = 1;
            }
            else
            {
                 pAMIGetTriggerEventRes->AMIGetTriggerEvent.EnableDisableFlag = 2;
            }
            return sizeof(AMIGetTriggerEvent_T);
        default:
            pAMIGetTriggerEventRes->AMIGetTriggerEvent.CompletionCode = CC_PARAM_NOT_SUPPORTED;
            return sizeof(INT8U);
    }

    pAMIGetTriggerEventRes->AMIGetTriggerEvent.CompletionCode = CC_INV_DATA_FIELD;
   return sizeof(INT8U);
}

/**
 * *@fn AMIGetSolConf
 * *@brief This command is used to get the SOL configuration
 * *@param pReq - Request for the command
 * *@param ReqLen - Request length for the command
 * *@param pRes - Response for the command
 * *@param BMCInst - BMC Instance Value
 **/
int AMIGetSolConf( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst )
{
     _NEAR_ AMIGetSOLConfRes_T *pAMIGetSOLConfRes = (AMIGetSOLConfRes_T *)pRes;
     BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
     INTU strsize = 0;
     
     memset(pRes,0,sizeof(AMIGetSOLConfRes_T));
	 
     pAMIGetSOLConfRes->SolSessionTimeout = pBMCInfo->IpmiConfig.SOLSessionTimeOut;
     TDBG("SOL Session Timeout is %d \n",pAMIGetSOLConfRes->SolSessionTimeout);

     strsize = strlen(pBMCInfo->IpmiConfig.pSOLPort);
     strncpy(pAMIGetSOLConfRes->SolIfcPort,pBMCInfo->IpmiConfig.pSOLPort,strsize);
     pAMIGetSOLConfRes->SolIfcPort[strsize] = '\0';
     TDBG("SOL Port Configuration is %s \n",pAMIGetSOLConfRes->SolIfcPort);
     
     pAMIGetSOLConfRes->CompletionCode = CC_NORMAL;
     return sizeof(AMIGetSOLConfRes_T);
}

/**
 * *@fn AMISetLoginAuditConfig
 * *@brief This command is used to enable/disable login audit based on the parameter input
 * *@param pReq - Request for the command
 * *@param ReqLen - Request length for the command
 * *@param pRes - Response for the command
 * *@param BMCInst - BMC Instance Value
 **/
int AMISetLoginAuditConfig( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst )
{
    _NEAR_ AMISetLoginAuditCfgReq_T *pAMISetLoginAuditCfgReq = (AMISetLoginAuditCfgReq_T *)pReq;
    _NEAR_ AMISetLoginAuditCfgRes_T *pAMISetLoginAuditCfgRes = (AMISetLoginAuditCfgRes_T *)pRes;

    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    
    /* Chk request Length */
    if(ReqLen != sizeof(AMISetLoginAuditCfgReq_T) )
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof (INT8U);
    }

    /* Check Reserved bits */
    if( (pAMISetLoginAuditCfgReq->WebLogAuditCfg & 0xF0) || (pAMISetLoginAuditCfgReq->IPMILogAuditCfg & 0xF0)
        || (pAMISetLoginAuditCfgReq->TelnetLogAuditCfg & 0xF0) ||  (pAMISetLoginAuditCfgReq->SSHLogAuditCfg & 0xF0) 
        || (pAMISetLoginAuditCfgReq->KVMLogAuditCfg & 0xF0)) 
    {
        *pRes = CC_INV_DATA_FIELD;
         return sizeof(INT8U);    	
    }


    pBMCInfo->LoginAuditCfg.WebEventMask = pAMISetLoginAuditCfgReq->WebLogAuditCfg;
    pBMCInfo->LoginAuditCfg.IPMIEventMask = pAMISetLoginAuditCfgReq->IPMILogAuditCfg;
    pBMCInfo->LoginAuditCfg.TelnetEventMask = pAMISetLoginAuditCfgReq->TelnetLogAuditCfg;
    pBMCInfo->LoginAuditCfg.SSHEventMask = pAMISetLoginAuditCfgReq->SSHLogAuditCfg;
    pBMCInfo->LoginAuditCfg.KVMEventMask = pAMISetLoginAuditCfgReq->KVMLogAuditCfg;

    FlushIPMI((INT8U*)&pBMCInfo->LoginAuditCfg,(INT8U*)&pBMCInfo->LoginAuditCfg,pBMCInfo->IPMIConfLoc.LoginAuditCfgAddr,
                      sizeof(LoginAuditConfig_T),BMCInst);
    pAMISetLoginAuditCfgRes->CompletionCode = CC_NORMAL;   
    return sizeof(AMISetLoginAuditCfgRes_T);     

  }

/**
 * *@fn AMIGetLoginAuditConfig
 * *@brief This command is used to get the enable/disable login audit based on the parameter input
 * *@param pReq - Request for the command
 * *@param ReqLen - Request length for the command
 * *@param pRes - Response for the command
 * *@param BMCInst - BMC Instance Value
 **/
int AMIGetLoginAuditConfig( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst )
{

    _NEAR_ AMIGetLoginAuditCfgRes_T *pAMIGetLoginAuditCfgRes = (AMIGetLoginAuditCfgRes_T *)pRes;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    
    /* Chk request Length */
    if( ReqLen )
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof (INT8U);
    }    

    pAMIGetLoginAuditCfgRes->CompletionCode = CC_NORMAL;
    pAMIGetLoginAuditCfgRes->WebLogAuditCfg = pBMCInfo->LoginAuditCfg.WebEventMask;
    pAMIGetLoginAuditCfgRes->IPMILogAuditCfg = pBMCInfo->LoginAuditCfg.IPMIEventMask;
    pAMIGetLoginAuditCfgRes->TelnetLogAuditCfg = pBMCInfo->LoginAuditCfg.TelnetEventMask;
    pAMIGetLoginAuditCfgRes->SSHLogAuditCfg = pBMCInfo->LoginAuditCfg.SSHEventMask;
    pAMIGetLoginAuditCfgRes->KVMLogAuditCfg = pBMCInfo->LoginAuditCfg.KVMEventMask;
    return sizeof(AMIGetLoginAuditCfgRes_T);
}

/**
 **@fn AMIGetAllIPv6Address
 **@brief This command is used to get the ipv6 address 
 **@param pReq - Request for the command
 **@param ReqLen - Request length for the command
 **@param pRes - Response for the command
 **@param BMCInst - BMC Instance Value
 **/
int 
AMIGetAllIPv6Address ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes ,int BMCInst)
{
    INT8U EthIndex;
    INT8U netindex= 0xFF;
    INT8U i =0;
    char IfcName[16];
    NWCFG_STRUCT        NWConfig;
    NWCFG6_STRUCT      NWConfig6;
    AMIGetIPv6AddrReq_T *pAMIGetIPv6AddrReq = (AMIGetIPv6AddrReq_T *)pReq;

    if ((MAX_IPV6_COUNT < pAMIGetIPv6AddrReq->IPCnt) || \
          ((pAMIGetIPv6AddrReq->IPCnt + pAMIGetIPv6AddrReq->Index) > IP6_ADDR_LEN))
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    EthIndex= GetEthIndex(pAMIGetIPv6AddrReq->ChannelNum & 0x0F, BMCInst);
    if(0xff == EthIndex)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    memset(IfcName,0,sizeof(IfcName));
    /*Get the EthIndex*/
    if(GetIfcName(EthIndex,IfcName, BMCInst) == -1)
    {
        TCRIT("Error in Getting Ifc name\n");
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }

    for(i=0;i<sizeof(Ifcnametable)/sizeof(IfcName_T);i++)
    {
        if(strcmp(Ifcnametable[i].Ifcname,IfcName) == 0)
        {
            netindex= Ifcnametable[i].Index;
            break;
        }
    }

    if(netindex == 0xFF)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof (INT8U);
    }
    *pRes = CC_NORMAL;
    pRes++;
    nwReadNWCfg_v4_v6( &NWConfig, &NWConfig6, netindex,g_corefeatures.global_ipv6);
    memcpy (pRes, &NWConfig6.GlobalIPAddr[pAMIGetIPv6AddrReq->Index], (pAMIGetIPv6AddrReq->IPCnt *IP6_ADDR_LEN));
    memcpy (&pRes[pAMIGetIPv6AddrReq->IPCnt *IP6_ADDR_LEN], &NWConfig6.GlobalPrefix[pAMIGetIPv6AddrReq->Index], pAMIGetIPv6AddrReq->IPCnt);
    return (pAMIGetIPv6AddrReq->IPCnt *(IP6_ADDR_LEN + 1) + 1);
}

/*int GetPamCount()
{
    int cnt = 0,Index = 0;

    for (Index = 0;Index < (sizeof (g_DefaultOrder) /sizeof (PamOrder_T)); Index++)
    {
        if(strncmp(SRV_LDAP, (char *)g_DefaultOrder[Index].ServiceName,strlen(SRV_LDAP)) == 0)
        {
            g_DefaultOrder[Index].Enabled = g_corefeatures.auth_ldap_support;
        }
        else if(strncmp(SRV_AD, (char *)g_DefaultOrder[Index].ServiceName,strlen(SRV_AD)) == 0)
        {
            g_DefaultOrder[Index].Enabled = g_corefeatures.auth_ad_support;
        }
        else if(strncmp(SRV_RADIUS, (char *)g_DefaultOrder[Index].ServiceName,strlen(SRV_RADIUS)) == 0)
        {
            g_DefaultOrder[Index].Enabled = g_corefeatures.auth_radius_support;
        }

        if(g_DefaultOrder[Index].Enabled != ENABLED)
        {
            continue;
        }
        cnt++;
    }
    return cnt;
}
*/

/*BOOL isPAMModuleExist(INT8U Pammodule)
{
    INT8U Index = 0;

    for (Index = 0;Index < MODULES_TO_MODIFIED; Index++)
    {
        if(g_DefaultOrder[Index].SqnceNo == Pammodule)
        {
            if(g_DefaultOrder[Index].Enabled != ENABLED)
            {
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}
*/


/**
 * *@fn AMISetUBootMemtest
 * *@brief This command is used to Enable/Disable the U-Boot Memory Test 
 * *@param pReq - Request for the command
 * *@param ReqLen - Request length for the command
 * *@param pRes - Response for the command
 * *@param BMCInst - BMC Instance Value
 **/
int
AMISetUBootMemtest(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst)
{
    AMISetUBootMemtestReq_T *pAMISetUBootMemtestReq = (AMISetUBootMemtestReq_T *)pReq;
    AMISetUBootMemtestRes_T *pAMISetUBootMemtestRes = (AMISetUBootMemtestRes_T *)pRes;
    int RetVal;
    char BootVal[5];
    memset(BootVal,0,sizeof(BootVal));

    /*Check the Reserved value ( 0 - Disable; 1 - Enable memory Test)*/
    if(1 < pAMISetUBootMemtestReq->EnableMemoryTest)
    {
        *pRes = CC_INV_DATA_FIELD;
        return sizeof(INT8U);
    }

    /*Enable/Disable the U-Boot Memory Test*/
    sprintf(BootVal,"%d",pAMISetUBootMemtestReq->EnableMemoryTest);

    RetVal = SetUBootParam(UBOOT_MEMTEST_ENABLE_VAR,BootVal);
    if(0 != RetVal)
    {
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof(INT8U);
    }

    pAMISetUBootMemtestRes->CompletionCode = CC_NORMAL;
    return sizeof(INT8U);
}

/**
 * *@fn AMIGetUBootMemtestStatus
 * *@brief This command is used to get the status of U-Boot Memory Test
 * *@param pReq - Request for the command
 * *@param ReqLen - Request length for the command
 * *@param pRes - Response for the command
 * *@param BMCInst - BMC Instance Value
 **/
int
AMIGetUBootMemtestStatus(_NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes, int BMCInst)
{
    AMIGetUBootMemtestStatusRes_T *pAMIGetUBootMemtestStatusRes = (AMIGetUBootMemtestStatusRes_T *)pRes;
    char BootVal[10] = {0};
    char Memtest[10] = {0};
    int RetVal;

    /*Get the U-Boot Memory Test Result*/
    RetVal = GetUBootParam(UBOOT_MEMTEST_STATUS_VAR,BootVal);
    if(0 != RetVal)
    {
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof(INT8U);
    }

    /*Get the do_memtest U-Boot Env Value*/
    RetVal = GetUBootParam(UBOOT_MEMTEST_ENABLE_VAR,Memtest);
    if(0 != RetVal)
    {
        *pRes = CC_UNSPECIFIED_ERR;
        return sizeof(INT8U);
    }

    /*Memory Test idle (0)*/
    pAMIGetUBootMemtestStatusRes->MemtestStatus = 0;

    if(strcmp(BootVal,MEMTEST_PASS) == 0)
    {
        /*Memory Test Success (1)*/
        pAMIGetUBootMemtestStatusRes->MemtestStatus = 1;
    }
    else if(strcmp(BootVal,MEMTEST_FAIL) == 0)
    {
        /*Memory Test Fail (2)*/
        pAMIGetUBootMemtestStatusRes->MemtestStatus = 2;
    }

    /*Memory Test diable (0)*/
    pAMIGetUBootMemtestStatusRes->IsMemtestEnable = 0;

    if(strcmp(Memtest,MEMTEST_ENABLE) == 0)
    {
        /*Memory Test is Enabled (1)*/
        pAMIGetUBootMemtestStatusRes->IsMemtestEnable = 1;
    }

    pAMIGetUBootMemtestStatusRes->CompletionCode = CC_NORMAL;
    return sizeof(AMIGetUBootMemtestStatusRes_T);
}

/*-------------------------------------------------------------------------------------------
 * @fn CheckFirmwareChange
 * @brief : This Function is to detect Firmware Version Change 
 * @arguments : BMCInst
 * @Returns : void
---------------------------------------------------------------------------------------------*/
void CheckFirmwareChange(int BMCInst)
{
    FWINFO_STRUCT Curfwinfo;
    struct stat buf;
    FILE* fp = NULL;
    char LastBootedFWVersion[MAX_FW_STR_SIZE] = {0};
    memset(&Curfwinfo, 0, sizeof(FWINFO_STRUCT));
    if(GetFWInfo(&Curfwinfo) != 0)
    {
        TCRIT("Failed to Get Current Running Firmware Information\n");
        return;
    }
    if((stat(CONF_FWVERSION_FILE, &buf) == -1) && (errno == ENOENT))
    {
        fp = fopen(CONF_FWVERSION_FILE, "w+");
        fprintf(fp,"FW_VERSION=%s",Curfwinfo.FWVersion);
        fclose(fp);
    }
    else
    {
        fp = fopen(CONF_FWVERSION_FILE, "r");
        fscanf(fp,"FW_VERSION=%s",LastBootedFWVersion);
        fclose(fp);
        if(strncasecmp(Curfwinfo.FWVersion,LastBootedFWVersion,strlen(Curfwinfo.FWVersion)) != 0)
        {
            fp = fopen(CONF_FWVERSION_FILE, "w");
            fprintf(fp,"FW_VERSION=%s",Curfwinfo.FWVersion);
            fclose(fp);
            UINT8 EvntData1 = 0x01; /*Event Offset*/
            int EvntData2, EvntData3;
            sscanf(Curfwinfo.FWVersion,"%d.%d.",&EvntData2,&EvntData3);
                
            UINT8 reqData[EVENT_MSG_LENGTH];
            
            reqData[0] = g_BMCInfo[BMCInst].IpmiConfig.BMCSlaveAddr;
            reqData[1] = 0x0;
            reqData[2] = IPMI_EVM_REVISION;             // EvMRev 0x04
            reqData[3] = SENSOR_TYPE_VERSION_CHANGE;
            reqData[4] = 0x0;                          /*Default Sensor Number*/
            reqData[5] = SENSOR_SPECIFIC_READ_TYPE;
            reqData[6] = EvntData1;
            reqData[7] = EvntData2;
            reqData[8] = EvntData3;
            PostEventMessage(reqData, FALSE, sizeof(reqData), BMCInst);
        }
    }
    return;
}

int AMIGetFeatureStatus( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst)
{
    _NEAR_  AMIGetFeatureStatusRes_T*       pGetFeatureStatusRes = (AMIGetFeatureStatusRes_T *)pRes;
    char featureName[MAX_CORE_FEATURE_NAME_LEN+1] = "\0";

    pGetFeatureStatusRes->CompletionCode = CC_SUCCESS;
    pGetFeatureStatusRes->FeatureStatus = 0;
	
    if( (ReqLen == 0) || (ReqLen > MAX_CORE_FEATURE_NAME_LEN) )
    {
	pGetFeatureStatusRes->CompletionCode = CC_REQ_INV_LEN;
	return sizeof(INT8U);
    }

    memcpy(featureName,pReq,ReqLen);

    if ( IsFeatureEnabled(featureName) == ENABLED )
	pGetFeatureStatusRes->FeatureStatus = ENABLED;
		
return sizeof(AMIGetFeatureStatusRes_T);
}


int AMIYAFUReplaceSignedImageKey ( _NEAR_ INT8U *pReq, INT32U ReqLen, _NEAR_ INT8U *pRes,_NEAR_ int BMCInst )
{
    AMIYafuSignedImageKeyReq_T *pAMIYafuSignedImageReq = (AMIYafuSignedImageKeyReq_T *)pReq;
    AMIYafuSignedImageKeyRes_T *pAMIYafuSignedImageRes = (AMIYafuSignedImageKeyRes_T *)pRes;
    AMIPublicKeyUploadReq_T *pAMIPublicKeyUploadReq = (AMIPublicKeyUploadReq_T *)pReq;
    int SizeWritten = 0,i = 0, KeySize = 0, RetVal = -1;
    INT8U *pKeyBuf = NULL, FuncID = 0;
    INT8S KeyPath[MAX_PUB_KEY_PATH_SIZE] = {0};
    FILE *fp =NULL;

    memcpy(KeyPath,PubKeyPathInfo[0].KeyPath,MAX_PUB_KEY_PATH_SIZE);
    KeySize = ReqLen;
    pKeyBuf = pReq;
    FuncID = 0;

    if(ReqLen == 0)
    {
        *pRes = CC_REQ_INV_LEN;
        return sizeof(INT8U);
    }

    /* Check Requested Length */
    if( ReqLen > MAX_KEY_SIG_SIZE )
    {
        /* $PUB$ start from 2 byte*/
        if( 0 == memcmp(&pReq[1],PUB_KEY_SIG,MAX_KEY_SIG_SIZE))
        {
            if(ReqLen > sizeof(AMIPublicKeyUploadReq_T))
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }

            if (g_PDKHandle[PDK_GET_PUBKEY_PATH] != NULL)
            {
                RetVal = ((int(*)(INT8U, INT8S*, int))(g_PDKHandle[PDK_GET_PUBKEY_PATH]))(pAMIPublicKeyUploadReq->FuncID, KeyPath, BMCInst);
                if(RetVal > 0)
                {
                    /*If the Retval is positive, then Function ID  is invalid*/
                    *pRes = CC_INV_DATA_FIELD;
                    return sizeof(INT8U);
                }
                /*RetVal == -1, Proceed with core implementation*/
            }

            /*RetVal == 0, File path is overwrite by PDK*/
            if(RetVal == -1)
            {
                /*Check the Function ID*/
                for( i = 0; i < sizeof(PubKeyPathInfo)/sizeof(PubKeyPathInfo[0]); i++)
                {
                    if(pAMIPublicKeyUploadReq->FuncID == PubKeyPathInfo[i].FuncID)
                    {
                        FuncID = pAMIPublicKeyUploadReq->FuncID;
                        memcpy(KeyPath,PubKeyPathInfo[i].KeyPath,MAX_PUB_KEY_PATH_SIZE);
                        break;
                    }
                }
            }

            if(i == sizeof(PubKeyPathInfo)/sizeof(PubKeyPathInfo[0]))
            {
                *pRes = CC_INV_DATA_FIELD;
                return sizeof(INT8U);
            }

            KeySize = ReqLen - MAX_KEY_SIG_SIZE - sizeof(INT8U);
            if((KeySize == 0) || (KeySize > MAX_PERMITTED_KEY_SIZE))
            {
                *pRes = CC_REQ_INV_LEN;
                return sizeof(INT8U);
            }
            pKeyBuf = &pAMIPublicKeyUploadReq->PubKey[0];

            switch(FuncID)
            {
                case SINGED_IMAGE:
                    if(g_corefeatures.signed_hashed_image_support != ENABLED)
                    {
                        *pRes = CC_PUB_KEY_UPLOAD_FUNC_ID_NOT_ENABLED;
                        return sizeof(INT8U);
                    }
                    break;
                case BUNDLE_IMAGE:
                    if(g_corefeatures.unified_firmware_update != ENABLED)
                    {
                        *pRes = CC_PUB_KEY_UPLOAD_FUNC_ID_NOT_ENABLED;
                        return sizeof(INT8U);
                    }
                    break;
                default:
                    *pRes = CC_PARAM_NOT_SUPPORTED;
                    return sizeof(INT8U);
            }

        }
        else
        {
            if(g_corefeatures.signed_hashed_image_support != ENABLED)
            {
                pAMIYafuSignedImageRes->CompletionCode = CC_INV_CMD;
                return sizeof(AMIYafuSignedImageKeyRes_T);
            }

            if((ReqLen) > MAX_PERMITTED_KEY_SIZE)
            {
                IPMI_ERROR("Invalid Request Length\n");
               pAMIYafuSignedImageRes->CompletionCode = CC_REQ_INV_LEN;
               return sizeof(AMIYafuSignedImageKeyRes_T);
            }
            pKeyBuf = &pAMIYafuSignedImageReq->PubKey[0];
        }
    }

    fp = fopen(TEMP_PUB_KEY, "wb");
    if(fp == NULL)
    {
       IPMI_ERROR("Error in Opening File\n");
       pAMIYafuSignedImageRes->CompletionCode = CC_UNSPECIFIED_ERR;
       return sizeof(AMIYafuSignedImageKeyRes_T);
    }

    SizeWritten = fwrite(pKeyBuf, 1, KeySize, fp);
    if(SizeWritten != KeySize )
    {
       IPMI_ERROR("Error in Writing into the file\n");
       pAMIYafuSignedImageRes->CompletionCode = CC_UNSPECIFIED_ERR;
       fclose(fp);
       return sizeof(AMIYafuSignedImageKeyRes_T);
    }

    fclose(fp);

   /* Now Open the File in Read Mode */
    fp = fopen(TEMP_PUB_KEY, "rb");
    if(fp == NULL)
    {
       IPMI_ERROR("Error in Opening File\n");
       pAMIYafuSignedImageRes->CompletionCode = CC_UNSPECIFIED_ERR;
       unlink(TEMP_PUB_KEY);
       return sizeof(AMIYafuSignedImageKeyRes_T);
    }

    if(PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL) == NULL)
    {
       IPMI_ERROR("Error in loading RSA public key file %s\n", TEMP_PUB_KEY);
       fclose(fp);
       unlink(TEMP_PUB_KEY);
       pAMIYafuSignedImageRes->CompletionCode = CC_PUBLICKEY_VALIDATION_FAILED;
       return sizeof(AMIYafuSignedImageKeyRes_T);
    }

    fclose(fp);

    if(moveFile(TEMP_PUB_KEY,KeyPath) != 0)
    {
        IPMI_ERROR("Cannot Move File Named as %s\n",KeyPath);
        pAMIYafuSignedImageRes->CompletionCode = CC_UNSPECIFIED_ERR;
        return sizeof(AMIYafuSignedImageKeyRes_T);
    }

    pAMIYafuSignedImageRes->CompletionCode = CC_NORMAL;
    return sizeof(AMIYafuSignedImageKeyRes_T);

}

/**
 * @fn ClearFailsafeBootErrorCodes
 * @brief clears error codes generated by failsafe boot. 
 * @retval   0 - On success
 *           -1 - On failure 
 **/
int ClearFailsafeBootErrorCodes()
{
    char envval[5];

	if(GetUBootParam("fwimage1corrupted",envval)== 0)
    {
       if( g_FlashingImage == IMAGE_1 || g_FlashingImage == IMAGE_BOTH)
       {
           envval[0] = '\0'; //To delete env variable                
           if(SetUBootParam("fwimage1corrupted",envval) !=0 )
           {
               TCRIT("Unable to clear Failsafe boot error code\n");
               return -1;
            }
        }		

    }
    if(GetUBootParam("fwimage2corrupted",envval)== 0)
    {
       if( g_FlashingImage == IMAGE_2 || g_FlashingImage == IMAGE_BOTH)
       {
           envval[0] = '\0'; //To delete env variable                
           if(SetUBootParam("fwimage2corrupted",envval) != 0)
           {
               TCRIT("Unable to clear Failsafe boot error code\n");
               return -1;
           }
       }
        
    }

	return 0;
}

