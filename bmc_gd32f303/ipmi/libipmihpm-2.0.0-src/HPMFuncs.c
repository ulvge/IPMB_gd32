/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2002-2012, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200, Norcross,        **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * HPMFuns.c
 * HPM Functions
 *
 * Author: Joey Chen <joeychen@ami.com.tw>
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS     0

#include "Types.h"
#include "Support.h"
#include "IPMIDefs.h"
#include "Debug.h"
#include "OSPort.h"
#include "HPMFuncs.h"
#include "IPMI_HPM.h"
#include "IPMI_HPMCmds.h"
#include "IPMI_IPM.h"
#include "HPMConfig.h"
#include "IPMDevice.h"
#include "IPMIConf.h"
#include "Ethaddr.h"
#include "flashlib.h"
#include "flshfiles.h"
#include <sys/sysinfo.h>
#include "ubenv.h"
#include "safesystem.h"

#define NON_BACKUP_FLASH    0
#define BACKUP_FLASH        1
#define NO_FLASH            0xFF
#define PROTECT_FLASH_ON    1
#define PROTECT_FLASH_OFF   0

#define MEMERASE                _IOW('M', 2, struct erase_info_user)
#define MEMLOCK                 _IOW('M', 5, struct erase_info_user)
#define MEMUNLOCK               _IOW('M', 6, struct erase_info_user)

struct erase_info_user {
        uint32_t start;
        uint32_t length;
};

typedef enum {
    INVALID_FLASH_MODE = 0x00,
	BACKUP_FLASH_MODE, 
	ROLLBACK_FLASH_MODE,
	UPG_FLASH_MODE
} PrepareFlashMode_E;

/* static variables */
static BOOL InitUpFwThreadState = FALSE;
static BOOL UpgFwThreadState = FALSE;

static INT8U *allocmem = NULL;
static INT32U Addofallocmem;
static INT32U TotalDataLen = 0;

static BOOL ActivateFlashStatus = FALSE;
static BOOL skip = FALSE;
static BOOL DeferredAct_Support = FALSE;

static _FAR_ INT8U      CurrCompOnUpg = INVALID_COMPONENT_ID; /* component to be upgraded */
static _FAR_ INT8U      CurrRollbackComponets = 0x00;

static HPMCmdStatus_T	HPMCmdStatus;
static FWUpgState_T FWUpgState[MAX_COMPONENTS];

/* HPM Timer relating variables*/
unsigned long HPMTimerCnt = 0;
int HPMflashStatus = 0;

/* static functions */
static void InitUpgVar(void);

/*-----------------------------------------------------
 * Checking functions 
 *-----------------------------------------------------*/ 
BOOL IsCachedComponentID(INT8U ComponentID)
{
    return (CurrCompOnUpg == ComponentID);
}

static 
INT8U ConvertComponentToID(INT8U Component)
{
    INT8U i;

    for(i = 0; i < MAX_COMPONENTS; i++)
        if(0x01 & (Component >> i))
            return i;
    return INVALID_COMPONENT_ID;
}
 
BOOL IsFwUpSupportIfc(int BMCInst)
{   
    INT8U CurCh;
    
    OS_THREAD_TLS_GET(g_tls.CurChannel,CurCh); 
	if ( (SYS_IFC_CHANNEL != CurCh)    &&
	     (USB_CHANNEL != CurCh)        &&
	     (!IsLANChannel(CurCh,BMCInst)) )
	{
            return FALSE;
	}
   
    return TRUE;
}

INT8U GetSelfTestResultByte(INT8U ByteNum, int BMCInst)
{
    if (ByteNum == 1)
    {
        return ( 0 == g_BMCInfo[BMCInst].Msghndlr.SelfTestByte ) ?
                GST_NO_ERROR: GST_CORRUPTED_DEVICES;
    }                                 
    else if (ByteNum == 2)
    {
        return (g_BMCInfo[BMCInst].Msghndlr.SelfTestByte);
    }
    
    return 0xFF;
}

int VerifyImageLength(INT32U ImageLength)
{
    return (ImageLength == TotalDataLen);
}

static 
void RestartIPMC(int secs)
{    
    int retval;
  
    sleep(secs);
    printf("RestartDevice...\n");
    retval = RestartDeviceWithNewFirmware();
    if(retval != EXIT_SUCCESS)
    {
        printf("Using linux system reboot...\n");
        safe_system("reboot");
    }  
}

/*---------------------------------------
 * HPMTimerAction
 * @brief   This function is invoked when timer is count to 0
 *---------------------------------------*/
int HPMTimerAction()
{

// Resetting the BMC
    /*  Reboot system here  */   
    RestartIPMC(5);
    return 0;
}


/*--------------------------------------------------------------------
 * HPMTimerTask
 * @brief   This function is invoked when client is start to upload image every one second
 *--------------------------------------------------------------------*/
void* HPMTimerTask(void *pArg)
{
  
     /* set HPM timer depend on different state*/  
   if(HPMflashStatus == HPM_UPLOAD_BLK_INPROGRESS)	
        HPMTimerCnt = HPM_UPLOAD_BLK_TIMER_COUNT; 
    else if(HPMflashStatus == HPM_FLASH_FW_INPROGRESS)	
        HPMTimerCnt = HPM_FLASH_FW_TIMER_COUNT; 	
	
    TDBG("HPMTimer has started with Timeout = %ld\n",HPMTimerCnt);
    while (1)
    {
    	if(HPMflashStatus == HPM_FLASH_SUCCEEDED)
		break;	
	
	HPMTimerCnt--;
        
	 TDBG("HPMTimer counting down = %ld\n",HPMTimerCnt);	
        usleep (1000 * HPM_MSECS_PER_TICK);
	if( !HPMTimerCnt)
 	{
 		printf("Flashing process has Timed OUT, so resetting the BMC\n");
		HPMTimerAction();
		break;
 	}
    }
    return NULL;

}



int ActivateFlashMode(int BMCInst)
{
    int RetVal = 0;
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst]; 
    _FAR_ SensorSharedMem_T*    pSenSharedMem;

    if((skip == FALSE) && (ActivateFlashStatus == FALSE))
    {        
        RetVal = PrepareFlashArea (FLSH_CMD_PREP_YAFU_FLASH_AREA, 0);
	   
        if((RetVal == 0) || (RetVal == 4))
            ActivateFlashStatus = TRUE;
        else   
            ActivateFlashStatus = FALSE;
            
        pSenSharedMem = (_FAR_ SensorSharedMem_T*)&pBMCInfo->SensorSharedMem; //m_hSensorSharedMem;   /* Create mutex for Sensor shared memory */
        OS_THREAD_MUTEX_ACQUIRE(&pBMCInfo->SensorSharedMemMutex,SHARED_MEM_TIMEOUT);
        pSenSharedMem->GlobalSensorScanningEnable = FALSE;
        /* Release mutex for Sensor shared memory */

        OS_THREAD_MUTEX_RELEASE(&pBMCInfo->SensorSharedMemMutex);

        skip = TRUE;
    }
    else
        ActivateFlashStatus = TRUE;

	

    return 0;
}

static 
int DeactivateFlashMode(void)
{
    if(ActivateFlashStatus == TRUE)
    {
        ActivateFlashStatus = FALSE;
    }
    return 0;
}

/*-----------------------------------------------------
 * HPM Status related functions 
 *-----------------------------------------------------*/ 
HPMStates_E GetFwUpgState(INT8U ComponentID)
{
    return FWUpgState[ComponentID].State;
}

int GetHPMStatus (HPMCmdStatus_T *HPMStatus)
{
	memcpy(HPMStatus, &HPMCmdStatus, sizeof(HPMCmdStatus_T));
    return 0;
}
       
void UpdateHPMStatus (INT8U CompCode, INT8U Cmd, INT8U CmdDuration)
{
	HPMCmdStatus.CmdCC 			= CompCode;
	HPMCmdStatus.CmdInProgress 	= Cmd;
	HPMCmdStatus.CmdDuration 	= CmdDuration;
}

BOOL IsLastHPMCmdCorrect(INT8U LastCmd)
{
    HPMCmdStatus_T HPMStatus;
    
    GetHPMStatus(&HPMStatus);
    
    return ((HPMStatus.CmdInProgress == LastCmd) && (HPMStatus.CmdCC == CC_NORMAL));
}

void InitFWUpgState(INT8U ComponentID)
{
    memset(&FWUpgState[ComponentID], 0, sizeof(FWUpgState_T)); 
}

void InitAllFWUpgState(void)
{
    memset(&FWUpgState[0], 0, (sizeof(FWUpgState_T) * MAX_COMPONENTS));             
}

 



/*-----------------------------------------------
 *  Abort firmware functions
 *----------------------------------------------*/ 
void* AbortIPMCThread(void *pArg)
{   
    INT8U CompID = CurrCompOnUpg;
    INT8U UpAction = FWUpgState[CompID].UpAction;

    const int secs = 5;

    if(UpAction == INIT_UPLOAD_FOR_UPGRADE)
    {
        RestartIPMC(secs);
    }
    else if(UpAction == INIT_UPLOAD_FOR_COMPARE)
    {
        RevertToInitState(CompID);
    }          
    OS_DELETE_THREAD();
}

INT8U HandleAbortFirmwareUpgrade(void)
{   
    INT8U CompID = CurrCompOnUpg;
    
    if(ActivateFlashStatus == TRUE)
    {
        if(VERIFYING == FWUpgState[CompID].State)
            return CC_UPG_NOT_ABORTED_AT_THIS_MOMENT;
            
        if(FLASHING == FWUpgState[CompID].State)
        {
            if(CompID == BOOT_COMPONENT_ID )
                return CC_UPG_NOT_ABORTED_AT_THIS_MOMENT;
            
            if(CompID == APP_COMPONENT_ID)
            {
                FWUpgState[CompID].Abort = TRUE;
                return CC_UPG_ABORTED_FW_NOT_RESUMABLE;
            }
        }   
        else
            FWUpgState[CompID].Abort = TRUE;
        
        if(IsIPMCComponentID(CompID))
        {
            OS_CREATE_THREAD(AbortIPMCThread, NULL, NULL);
        }
    }

    return CC_NORMAL;    
}

static 
void GetEraseInfoForProtectFlashOnOff(INT8U ComponentID, INT8U FlashPart, struct erase_info_user *erase_info)
{
    int EraseBlk = 0;
    ComponentFlashInfo_T ComponentFlashInfo;
    
    memset(&ComponentFlashInfo, 0, sizeof(ComponentFlashInfo_T));
    GetComponentFlashInfo(&ComponentFlashInfo, ComponentID);
    
    /* Calc erase_info for Protect Flash On/Off, fill the erase_info by multiplied erase block size */
    EraseBlk = (ComponentFlashInfo.AllocateMemSize / CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE);
    if((ComponentFlashInfo.AllocateMemSize % CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE) != 0)
        EraseBlk += 1;
    erase_info->start = ComponentFlashInfo.Flashoffset;
    erase_info->length = (EraseBlk * CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE);

    if(FlashPart == BACKUP_FLASH)
        erase_info->start += CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE;
}

int ProtectFlashOnOff(INT8U ComponentID, INT8U FlashPart, INT8U Protect)
{
    int Status = 0;

    if(ActivateFlashStatus == TRUE)
    {
        int fd;
        struct erase_info_user	erase_info;
        GetEraseInfoForProtectFlashOnOff(ComponentID, FlashPart, &erase_info);
        
        fd = open(DEFAULT_MTD_DEVICE, O_RDWR);
        if (fd == -1) 
        {
            printf("ERROR: open failed (%s)\n", strerror(errno));
        }

        if(Protect ==  0x01)
        {
            if((ioctl (fd, MEMLOCK, &erase_info)) != -1)
                Status = 0x00;
            else
                Status = 0x01;
        }
        else if(Protect == 0x00)
        {
            if((ioctl (fd, MEMUNLOCK, &erase_info)) != -1)
                Status = 0x00;
            else
                Status = 0x01;
        }
    }

    return Status;
}

int AllocateMemory(INT32U Sizeofmemtoalloc)
{   
	struct sysinfo s_info;
	int error;
	INT32U  free_mem = 0;

    if ((ActivateFlashStatus == TRUE) && (allocmem == NULL))
    {
        if (Sizeofmemtoalloc > CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE)
        {
            error = sysinfo(&s_info);
            if(error != 0)
            {
               fprintf(stderr,"\nError in getting free RAM Memory using Sysinfo system call\n Error Code:%d\n",error);
               return -1;
            }
            free_mem = s_info.freeram;
            if(Sizeofmemtoalloc > free_mem)
            {
                fprintf(stderr,"\nFlash Image Size(%u)  is greater than Free Memory(%u) in RAM \n\n",Sizeofmemtoalloc,free_mem);
                Addofallocmem = (INT32U)0xfffffffe;
            }
            else
            {
                allocmem = (INT8U *) malloc (Sizeofmemtoalloc);
                
                if(allocmem == NULL)
                    Addofallocmem = (INT32U)0xffffffff;
                else
                    Addofallocmem = (INT32U)allocmem ;  
            }
        }
    }
    return 0;
}

void WriteMemory (INT32U MemOffsets, INT16U DataLen, INT8U *Buf)
{
    INT8U *offs;
    uint32 Addofoffs;
    
    Addofoffs = Addofallocmem + MemOffsets;
 
    offs = (INT8U *) Addofoffs;

    if(allocmem != NULL)
        memcpy (offs, Buf, DataLen);
}

void FreeMemory(void)
{
    INT32U *memtofree;
    
    memtofree = (INT32U *)Addofallocmem;
    if(allocmem != NULL)
    {
        free(memtofree);
        allocmem = NULL;
    }
}

static 
int InitEraseCopyFlashInfo(INT8U ComponentID, INT32U *WriteMemOff, INT32U *Flashoffset, INT32U *Sizetocpy)
{
    ComponentFlashInfo_T ComponentFlashInfo;
    
    memset(&ComponentFlashInfo, 0, sizeof(ComponentFlashInfo_T));
    GetComponentFlashInfo(&ComponentFlashInfo, ComponentID);
    
    *WriteMemOff = ComponentFlashInfo.Memoffset;
    *Sizetocpy = ComponentFlashInfo.Sizetocpy;
    *Flashoffset = ComponentFlashInfo.Flashoffset;
  
    return 0;    
}

static 
int EraseCopyFlashState(INT32U WriteMemOff, INT32U StartOffset, INT32U Flashoffset, INT32U Sizetocpy)
{
    INT8U CompID = CurrCompOnUpg;
    int ECFerror = 0;

	int fd;
	INT32U i = 0, j;
	INT8U *Buf = NULL;
	struct erase_info_user erase_info;
    
    INT8U *memoffs;
    memoffs = (INT8U *)Addofallocmem;
    memoffs += WriteMemOff;

    fd = open(DEFAULT_MTD_DEVICE, O_RDWR);		
    if (fd == -1) 
    {
        printf("ERROR: open failed (%s)\n", strerror(errno));
        return -1;
    }
    
    for(j=0; j < (Sizetocpy / CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE); j++)
    {
        if(FWUpgState[CompID].Abort)
        {
            RevertToInitState(CompID);
            ECFerror = 1;
            break;
        }
        
        Buf = malloc(CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE);
        if(Buf == NULL)
        {
            ECFerror=1;
            break;
        }

        if ( lseek (fd,StartOffset + Flashoffset, SEEK_SET) == -1)
        {
            fprintf (stderr,
            "seek error on %s: %s\n",
            DEFAULT_MTD_DEVICE, strerror (errno));
            ECFerror=1;
            break;
        }

        if( (read(fd, Buf, CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE)) != CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE)
        {
            fprintf (stderr,
            "Bytes read error %s: %s\n",
            DEFAULT_MTD_DEVICE, strerror (errno));
            ECFerror=1;
            break;
        }

        for(i = 0; i < CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE; i++) 
        {
            if(*Buf == *memoffs)
            {
                Buf++;
                memoffs++;
            }
            else
            {
                memoffs = memoffs - i;
                break;
            }
        }

        if (i == CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE)
        {
            Buf = Buf - i;
            free(Buf);
            printf("Skipping the erase block %d\n",(Flashoffset / CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE));
            Flashoffset += CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;
            continue;
        }
        else
        {
            Buf = Buf - i;
            free(Buf);
        }

        erase_info.start = StartOffset + Flashoffset;
        printf("Upgrading the block =  %d\n",(Flashoffset / CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE));
        erase_info.length = CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;

        if((ioctl (fd, MEMERASE, &erase_info)) == -1)
        {
            ECFerror=1;
            break;
        }

        if ( lseek (fd, StartOffset + Flashoffset, SEEK_SET) == -1)
        {
            fprintf (stderr,
            "seek error on %s: %s\n",
            DEFAULT_MTD_DEVICE, strerror (errno));
            ECFerror=1;
            break;
        }

        if(write(fd, memoffs, CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE) != CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE)
        {
            ECFerror=1;
            break;
        }
        Flashoffset += CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;
        memoffs += CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;
    }
    close(fd);

    return ECFerror;
}

static 
int HPMEraseCopyFlash(INT8U ComponentID, INT8U FlashPart)
{
    INT32U WriteMemOff = 0;
    INT32U Sizetocpy = 0;
    INT32U Flashoffset = 0;
    INT32U StartOffset = 0;
    
    if(FlashPart == BACKUP_FLASH)
        StartOffset = CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE;
        
    if(ActivateFlashStatus == TRUE)
    {
        InitEraseCopyFlashInfo(ComponentID, &WriteMemOff, &Flashoffset, &Sizetocpy);

        TDBG("ComponentID=0x%02X, WriteMemOff=0x%X, Flashoffset=0x%X Sizetocpy=0x%X\n", 
                ComponentID, WriteMemOff, Flashoffset, Sizetocpy);
        EraseCopyFlashState(WriteMemOff, StartOffset, Flashoffset, Sizetocpy);
    }
       
    return 0;
}

static 
int HPMVerifyFlashStatus(INT32U VWriteMemOff, INT32U StartOffset, INT32U VFlashoffset, INT32U Sizetoverify)
{
    int VFerror = 0;

    //INT32U VOffset = 0; 	
    int fd;
    INT32U i,j;
    INT8U *FlashBuf=0;

    INT8U *memoffs;
    memoffs = (INT8U *)Addofallocmem;
    memoffs += VWriteMemOff;

    fd = open(DEFAULT_MTD_DEVICE, O_RDWR);
    if (fd == -1) 
    {
        printf("ERROR: open failed (%s)\n", strerror(errno));
        return -1;
    }

    for(j = 0; j < (Sizetoverify / CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE); j++)
    {
        FlashBuf = malloc (CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE);
        if(FlashBuf == NULL)
        {
            VFerror = 1;
            break;						
        }
        if ( lseek (fd, (StartOffset + VFlashoffset), SEEK_SET) == -1)
        {
            fprintf (stderr,
            "seek error on %s: %s\n",
            DEFAULT_MTD_DEVICE, strerror (errno));
            VFerror = 1;				
            break;
        }

        if( (read(fd, FlashBuf, CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE)) != CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE)
        {
            fprintf (stderr,
            "Bytes read error %s: %s\n",
            DEFAULT_MTD_DEVICE, strerror (errno));
            VFerror = 1;					
            break;
        }

        for(i = 0; i< CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE; i++) 
        {
            if(*FlashBuf == *memoffs)
            {
                FlashBuf++;
                memoffs++;
            }
            else
            {
                printf("The value of i = 0x%08X\n", i);
                printf("Inside else\n");
                VFerror = 1;
                break;
            }
        }

        if( i == CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE)
        {
            FlashBuf = FlashBuf - CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;
            //VOffset = CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE; 
        }
        else
        {
            FlashBuf = FlashBuf - i;
            printf("FlashBuf after verify2 = 0x%08X\n", (INT32U)memoffs);
            printf("Verify Failure \n");
            //VOffset = (INT32U )memoffs;
            VFerror = 1;
            break;
        }
        free(FlashBuf);
        VFlashoffset += CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;
    }
    close(fd);

    return VFerror;
}

static
int HPMVerifyFlash (INT8U ComponentID, INT8U FlashPart)
{
    int VFerror = 0;
    INT32U VWriteMemOff = 0;
    INT32U Sizetoverify = 0;
    INT32U VFlashoffset = 0;
    INT32U StartOffset = 0;

    if(FlashPart == BACKUP_FLASH)
        StartOffset = CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE;
            
    if(ActivateFlashStatus == TRUE)
    {
        InitEraseCopyFlashInfo(ComponentID, &VWriteMemOff, &VFlashoffset, &Sizetoverify);
        VFerror = HPMVerifyFlashStatus(VWriteMemOff, StartOffset, VFlashoffset, Sizetoverify);   
    }
  
    return VFerror;
}

static
int ReadFlashToMem(INT32U Offset, INT32U Sizetocpy)
{ 
	int fd;
    INT32U j;
    INT32U EraseBlockSize = CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;

    fd = open(DEFAULT_MTD_DEVICE, O_RDWR);		
    if (fd == -1) 
    {
        printf("ERROR: open failed (%s)\n", strerror(errno));
        return -1;
    }

    for(j = 0; j < (Sizetocpy / EraseBlockSize); j++)
    {
        if ( lseek (fd, Offset, SEEK_SET) == -1)
        {
            fprintf (stderr,
            "seek error on %s: %s\n",
            DEFAULT_MTD_DEVICE, strerror (errno));
            close(fd);
            return 1;
        }

        if( (read(fd, (allocmem + (j * EraseBlockSize)), EraseBlockSize)) != EraseBlockSize)
        {
            fprintf (stderr,
            "Bytes read error %s: %s\n",
            DEFAULT_MTD_DEVICE, strerror (errno));
            close(fd);
            return 1;
        }
        Offset += EraseBlockSize;
    }
    close(fd);   
               
    return 0;
}

/*-----------------------------------------------
 *  Prepare/Backup firmware functions
 *----------------------------------------------*/ 
static 
int HPMPrepareFlash(INT8U ComponentID, INT8U PreFlashMode)
{
    INT8U FlashPart = NO_FLASH;     
    INT32U Sizeofmemtoalloc; 
    
    ComponentFlashInfo_T ComponentFlashInfo;
    
    memset(&ComponentFlashInfo, 0, sizeof(ComponentFlashInfo_T));
    GetComponentFlashInfo(&ComponentFlashInfo, ComponentID);

    Sizeofmemtoalloc = ComponentFlashInfo.AllocateMemSize;

    g_MBMCInfo.FlashType= HPM_FLASH;
    ActivateFlashMode(0);
    
    
    if(PreFlashMode == BACKUP_FLASH_MODE) 
        FlashPart = BACKUP_FLASH;
    if(PreFlashMode == ROLLBACK_FLASH_MODE) 
        FlashPart = NON_BACKUP_FLASH;
    if(PreFlashMode == UPG_FLASH_MODE) 
        FlashPart = NON_BACKUP_FLASH;
        
    if(PreFlashMode != INVALID_FLASH_MODE)
        ProtectFlashOnOff(ComponentID, FlashPart, PROTECT_FLASH_OFF); 
    
    
    AllocateMemory(Sizeofmemtoalloc);
    
    return 0;
}

static
int BackupComponentID(INT8U ComponentID)
{
    ComponentFlashInfo_T ComponentFlashInfo;
    
    memset(&ComponentFlashInfo, 0, sizeof(ComponentFlashInfo_T));
    GetComponentFlashInfo(&ComponentFlashInfo, ComponentID);
        
    INT32U ReadOffset = ComponentFlashInfo.Flashoffset; 
    INT32U Sizetocpy = ComponentFlashInfo.Sizetocpy; 

    ReadFlashToMem(ReadOffset, Sizetocpy);
    
    HPMEraseCopyFlash(ComponentID, BACKUP_FLASH);
    HPMVerifyFlash(ComponentID, BACKUP_FLASH);
 
    return 0;
}

void* BackupComponentsThread(void *pArg)
{
    int BackComps = (int) pArg;
    INT8U Components = (INT8U) BackComps; 
         
    int CompID;
    InitUpFwThreadState = TRUE;

    for(CompID = 0; CompID < MAX_COMPONENTS; CompID++)
    {
        if(0x01 & (Components >> CompID))
        {
            if(IsIPMCComponentID(CompID))
            {       			
                HPMPrepareFlash(CompID, BACKUP_FLASH_MODE);
                BackupComponentID(CompID);
                InitUpgVar();
            }
        }
    }

    UpdateHPMStatus(CC_NORMAL, CMD_INITIATE_UPG_ACTION, HPM_LONG_DURATION_CMD);
    
    InitUpFwThreadState = FALSE;
    OS_DELETE_THREAD(); 
      
    return (void*)0;    
}

int InitBackupComponents(INT8U Components)
{
    int BackComps = (int) Components;

    if(!IsOnlyOneComponent(Components))
        InitAllFWUpgState();
    else
        InitFWUpgState(ConvertComponentToID(Components));
    
    OS_CREATE_THREAD(BackupComponentsThread, (void *) (BackComps), NULL);

    return 0;
}

int InitPreComponents(INT8U Components)
{
    if(!IsOnlyOneComponent(Components))
        InitAllFWUpgState();
    else
        InitFWUpgState(ConvertComponentToID(Components));
        
    InitUpgVar();

    return 0;
}

/*-----------------------------------------------
 *  Upload firmware functions
 *----------------------------------------------*/ 
 
void* InitUploadIPMCThread(void *pArg)
{
    INT8U CompID = CurrCompOnUpg;
    INT8U UpAction = FWUpgState[CompID].UpAction;

    InitUpFwThreadState = TRUE;
    
    if(UpAction == INIT_UPLOAD_FOR_UPGRADE)
        HPMPrepareFlash(CompID, UPG_FLASH_MODE);
    else
        HPMPrepareFlash(CompID, INVALID_FLASH_MODE);
        
    TotalDataLen = 0;

    UpdateHPMStatus(CC_NORMAL, CMD_INITIATE_UPG_ACTION, HPM_LONG_DURATION_CMD);
    
    InitUpFwThreadState = FALSE;
    OS_DELETE_THREAD(); 
      
    return (void*)0;    
}

static 
int InitUploadIPMC(void) 
{
    INT8U CompID = CurrCompOnUpg;

    if(ActivateFlashStatus == FALSE)
    {
        if(InitUpFwThreadState == FALSE)
        {
            if(!FWUpgState[CompID].Abort)
                OS_CREATE_THREAD(InitUploadIPMCThread, NULL, NULL);
            else
            {   
                RevertToInitState(CompID);
                return -1;      
            }
        }
        else
            return -1;
    }
    else
        return -1;

    return 0;
}

int InitUpload(INT8U UpgradeAction, INT8U Component)
{
    INT8U ComponentID;

    /* cach the ID of the single component */
    CurrCompOnUpg = ComponentID = ConvertComponentToID(Component);

    if(ComponentID == INVALID_COMPONENT_ID)
        return -1;

    FWUpgState[ComponentID].UpAction = UpgradeAction;
    FWUpgState[ComponentID].State = INIT_UPLOAD;
                
    if(IsIPMCComponentID(ComponentID))
    {
        InitUploadIPMC();
    }
    else
    {
        printf("Not IPMCComponent(CompID) == %d\n", IsIPMCComponentID(ComponentID));
    }    
    return 0;
}

int HandleFirmwareBlock(INT8U BlkNum, INT8U *Data, INT16U Len)
{
    INT8U CompID = CurrCompOnUpg;
    /* WriteMemOffset will be the same as current received TotalDataLen */
    INT32U WriteMemOffsets = TotalDataLen; 
    
    ComponentFlashInfo_T ComponentFlashInfo;
    
    memset(&ComponentFlashInfo, 0, sizeof(ComponentFlashInfo_T));
    GetComponentFlashInfo(&ComponentFlashInfo, CurrCompOnUpg);
	
   
    if(ActivateFlashStatus == TRUE)
    {
        if(TotalDataLen == 0)
            FWUpgState[CompID].State = UPLOAD_FW_BLK;
            
        if((WriteMemOffsets + Len) > ComponentFlashInfo.AllocateMemSize)
        {
            printf("write memory exceed the allocated size\n");
            return -1;
        }
        
        WriteMemory(WriteMemOffsets, Len, Data);

        TotalDataLen += Len;
        TDBG("TotalDataLen = %lu, Len = %d\n", TotalDataLen, Len);
    }
    
    return 0;  
}


void InitUpgVar(void)
{

    TotalDataLen = 0;

    FreeMemory();

    DeactivateFlashMode();
  
    CurrCompOnUpg = INVALID_COMPONENT_ID; 
}

void RevertToInitState(INT8U ComponentID)
{
    InitUpgVar();
    if(IsValidComponentID(ComponentID))
        InitFWUpgState(ComponentID);    
}
   
void* UpgradeFirmwareThread(void *pArg)
{    
    INT8U CompID = CurrCompOnUpg;
    	
    UpgFwThreadState = TRUE;
	
    /*set timer*/
    HPMflashStatus = 	HPM_FLASH_FW_INPROGRESS;   
    HPMTimerCnt = HPM_FLASH_FW_TIMER_COUNT;
	
    if(IsDeferredActivationSupport(CurrCompOnUpg))
		DeferredAct_Support = TRUE;

    if(!FWUpgState[CompID].Abort)
    {
        FWUpgState[CompID].State = FLASHING;
        HPMEraseCopyFlash(CompID, NON_BACKUP_FLASH);
    }
    else
    {
        RevertToInitState(CompID);
        UpgFwThreadState = FALSE;
        OS_DELETE_THREAD(); 
    }
    if(!FWUpgState[CompID].Abort)
    {
        FWUpgState[CompID].State = VERIFYING;    
        HPMVerifyFlash(CompID, NON_BACKUP_FLASH);
    }
    else
    {
        RevertToInitState(CompID);
        UpgFwThreadState = FALSE;
        OS_DELETE_THREAD(); 
    }
    FWUpgState[CompID].State = UPG_DONE;
    FWUpgState[CompID].State = ACTIVATION_PENDING;      
    InitUpgVar();
    UpdateHPMStatus(CC_NORMAL, CMD_FINISH_FIRMWARE_UPLOAD, HPM_LONG_DURATION_CMD);

    UpgFwThreadState = FALSE;

    if(DeferredAct_Support == FALSE)
    {
    	UpdateHPMStatus(CC_CMD_INPROGRESS, CMD_ACTIVATE_FIRMWARE, HPM_LONG_DURATION_CMD);
	HandleActivateFirmware();

    }

     HPMflashStatus = HPM_FLASH_SUCCEEDED;	
	
    OS_DELETE_THREAD();      
    
    return (void*)0;    
}

void* CompareFirmwareThread(void *pArg)
{    
    int VFerror = 0;
    INT8U CompID = CurrCompOnUpg;

    UpgFwThreadState = TRUE;

    if(!FWUpgState[CompID].Abort)
    {
        FWUpgState[CompID].State = VERIFYING;
        VFerror = HPMVerifyFlash(CompID, NON_BACKUP_FLASH);
    }
    else
    {
        RevertToInitState(CompID);
        UpgFwThreadState = FALSE;
        OS_DELETE_THREAD();   
    }
    FWUpgState[CompID].State = UPG_DONE;

    InitUpgVar();
    if(!VFerror)
        UpdateHPMStatus(CC_NORMAL, CMD_FINISH_FIRMWARE_UPLOAD, HPM_LONG_DURATION_CMD);
    else
        UpdateHPMStatus(CC_IMAGE_NOT_MATCH, CMD_FINISH_FIRMWARE_UPLOAD, HPM_LONG_DURATION_CMD);
        
    UpgFwThreadState = FALSE;
    OS_DELETE_THREAD();  

    return (void*)0;    
}

static 
int HandleIPMCFirmware(void)
{
    INT8U ComponentID = CurrCompOnUpg;
    INT8U UpgradeAction = FWUpgState[ComponentID].UpAction;

    if(UpgFwThreadState == FALSE)
    {
        if (UpgradeAction == INIT_UPLOAD_FOR_UPGRADE)
            OS_CREATE_THREAD(UpgradeFirmwareThread, NULL, NULL);
        else if(UpgradeAction == INIT_UPLOAD_FOR_COMPARE)
            OS_CREATE_THREAD(CompareFirmwareThread, NULL, NULL);
    }
    else
        return -1;

    return 0;
}

int HandleUploadedFirmware(void)
{

    INT8U ComponentID = CurrCompOnUpg;
    INT8U *HeaderStart;
    int RetHeaderSize = 0;

    ComponentFlashInfo_T ComponentFlashInfo;
    
    memset(&ComponentFlashInfo, 0, sizeof(ComponentFlashInfo_T));
    GetComponentFlashInfo(&ComponentFlashInfo, ComponentID);
        
    HeaderStart = (INT8U *)Addofallocmem;
    HeaderStart += ComponentFlashInfo.Memoffset;
    
    /*
     * The reason why get header size here is because some customer may define a dynamic size of OEM area,
     * You have to read header to make sure the size. 
     * If I put this in upload firmware command will increase the complexity of that command
     */
    if(g_PDKHandle[PDK_VERIFYOEMHPMHEADER] != NULL)
    {
        int Ret;
        Ret = ( (int(*)(INT8U *, int *)) g_PDKHandle[PDK_VERIFYOEMHPMHEADER] ) (HeaderStart, &RetHeaderSize);
        if(Ret == -1)
        {
            printf("PDK check OEM header error\n");
            return -1;
        }
        if(RetHeaderSize != 0)
            ComponentFlashInfo.Memoffset += RetHeaderSize;        
    }
     
    if(IsIPMCComponentID(ComponentID))
    {
        HandleIPMCFirmware();
    }

    return 0;
}


/*-----------------------------------------------
 *  Activate firmware functions
 *----------------------------------------------*/ 
void* ActivateIPMCThread(void *pArg)
{
    const int secs = 5;
    int BackComps = (int) pArg;
    INT8U Components = (INT8U) BackComps;
    INT8U CompID;	

    for(CompID = 0; CompID < MAX_COMPONENTS; CompID++)
    {
        if(0x01 & (Components >> CompID))
        {
            if(IsIPMCComponentID(CompID))
            {
                 /*set bootselector only when activate APP component*/
            	   if((g_corefeatures.dual_image_support == ENABLED)&&(CompID==APP_COMPONENT_ID))
            	   {
                     /*select flashing image to be booted*/
			SetBootSelectorToBootParam(g_FlashingImage);
			break; 		 
            	   }
            }
         }
     }	
	
    RestartIPMC(secs);
    UpdateHPMStatus(CC_NORMAL, CMD_ACTIVATE_FIRMWARE, HPM_LONG_DURATION_CMD);
    OS_DELETE_THREAD();
  
}

INT8U GetActivatePendingComponents(void)
{
    INT8U ActivatedComponents = 0;
    int i = 0;
    for(i = 0; i < MAX_COMPONENTS; i++)
    {
        if(FWUpgState[i].State == ACTIVATION_PENDING)
        {
            ActivatedComponents += (0x1<<i);
        }
    }
    return ActivatedComponents;
}

int SetHPMActCompsToBootParam(INT8U Components)
{
    char *ParamName = "hpmactcomps";
    char ParamVar[5];
    
    memset(ParamVar, '\0', sizeof(ParamVar));
    sprintf(ParamVar, "%02X", Components);
    
    if(SetUBootParam(ParamName, ParamVar) != 0)
    {
        TCRIT("Unable to set hpm env variable\n");
        return -1;        
    }
    
    return 0;
}

int SetBootSelectorToBootParam(char image)
{
    char *ParamName1 = "bootselector";
    char *ParamName2 = "recentlyprogfw";
    char ParamVar[5];
    
    memset(ParamVar, '\0', sizeof(ParamVar));
    sprintf(ParamVar, "%d", image);
    
    if(SetUBootParam(ParamName1, ParamVar) != 0)
    {
        TCRIT("Unable to set hpm env variable\n");
        return -1;        
    }

   if(SetUBootParam(ParamName2, ParamVar) != 0)
    {
        TCRIT("Unable to set hpm env variable\n");
        return -1;        
    }	
    
    return 0;
}

INT8U GetHPMActCompsFromBootParam(void)
{
    INT8U Components = 0;
    unsigned long Var = 0;
    char ParamVar[5];

    memset(ParamVar, '\0', sizeof(ParamVar));
    if(GetUBootParam("hpmactcomps", ParamVar) != 0)
    {
        TCRIT("Unable to get hpm env variable\n");
        return 0;
    }
    sscanf(ParamVar, "%lx", &Var);
    Components = (INT8U) Var;
    
    return Components;
}

BOOL HandleActivateFirmware(void)
{
    BOOL ActFwStatus = FALSE;

    int ActivatedComponents =(int) GetActivatePendingComponents();
       
    if(ActivatedComponents != 0)
        SetHPMActCompsToBootParam(ActivatedComponents); 
    
    if(IsContainIPMCComponents(ActivatedComponents))
    {
        OS_CREATE_THREAD(ActivateIPMCThread,  (void *) (ActivatedComponents), NULL);
        ActFwStatus = TRUE;
    }

    return ActFwStatus;
}

/*-----------------------------------------------
 *  Rollback functions
 *----------------------------------------------*/ 
INT8U GetRollbackComponents(void)
{
    return CurrRollbackComponets;
}

static
int HPMRollbackFlash(INT8U ComponentID)
{
    ComponentFlashInfo_T ComponentFlashInfo;
    
    memset(&ComponentFlashInfo, 0, sizeof(ComponentFlashInfo_T));
    GetComponentFlashInfo(&ComponentFlashInfo, ComponentID);
        
    INT32U StartOffset = CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE;
    INT32U Flashoffset = ComponentFlashInfo.Flashoffset; 
    INT32U ReadOffset = StartOffset + Flashoffset;
    INT32U Sizetocpy = ComponentFlashInfo.Sizetocpy;

    ReadFlashToMem(ReadOffset, Sizetocpy);
    
    HPMEraseCopyFlash(ComponentID, NON_BACKUP_FLASH);
    HPMVerifyFlash(ComponentID, NON_BACKUP_FLASH);   
   
    return 0;
}

void* ManualRollbackIPMCThread(void *pArg)
{
    INT8U Components = CurrRollbackComponets;
    INT8U CompID;
    const int secs = 3;
 
    UpgFwThreadState = TRUE;

    for(CompID = 0; CompID < MAX_COMPONENTS; CompID++)
    {
        if(0x01 & (Components >> CompID))
        {
            if(IsIPMCComponentID(CompID))
            {
            	  if((g_corefeatures.dual_image_support) && ( g_corefeatures.hpm_rollback_support) )
   		  {
	  		/* Here,beside dual-image support, we need to have backup part to store backup APP component.
	  		It requires at least 65Mb flash size to support it if rom size is 16Mb, now we don't have this test case.*/
   		  }
		  else
		  {
			HPMPrepareFlash(CompID, ROLLBACK_FLASH_MODE);
                	HPMRollbackFlash(CompID);
                	InitUpgVar();
		  }
				  	
            }
        }
    }
    
    UpdateHPMStatus(CC_NORMAL, CMD_INITIATE_MANUAL_ROLLBACK, HPM_LONG_DURATION_CMD);
   	
    RestartIPMC(secs);

    HPMflashStatus = HPM_FLASH_SUCCEEDED;	     
    UpgFwThreadState = FALSE;
    OS_DELETE_THREAD();      
 
    return (void*)0;    
}

int HandleManualRollback(void)
{
    INT8U ActivatedComponents = GetHPMActCompsFromBootParam();
    CurrRollbackComponets = ActivatedComponents;
  
    if(ActivatedComponents != 0)
    {
    	 UpdateHPMStatus(CC_CMD_INPROGRESS, CMD_INITIATE_MANUAL_ROLLBACK, HPM_LONG_DURATION_CMD);
        if(IsContainIPMCComponents(ActivatedComponents))
        {
            OS_CREATE_THREAD(ManualRollbackIPMCThread, NULL, NULL);
        }
    }    
   
    return 0;
}

/*--------------------------------------------------------------------
 * SetHPMFlashStatus
 * @brief  This function is to set current flash status for HPMTimerTask.
 *--------------------------------------------------------------------*/
void SetHPMFlashStatus(INT8U curstatus)
{
   HPMflashStatus = curstatus ;
}

/*--------------------------------------------------------------------
 * SetHPMTimerCnt
 * @brief  This function is to set counter for HPMTimerTask.
 *--------------------------------------------------------------------*/
void SetHPMTimerCnt(INT8U secs)
{
   HPMTimerCnt = secs;
}
