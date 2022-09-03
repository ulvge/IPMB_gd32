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
 * HPMConfig.c
 * HPM configuration related functions
 *
 * Author: Joey Chen <joeychen@ami.com.tw>
 *
 *****************************************************************/
#include "IPMIDefs.h"
#include "HPMConfig.h"
#include "HPMCmds.h"
#include "IPMI_HPMCmds.h"
#include "HPMFuncs.h"
#include "HPM.h"
#include "fmh.h"
#include "iniparser.h"
#include <dbgout.h>
#include "unix.h"


/*----------------------------------------------------------*
 * Pre-defined or fixed configurations 
 *----------------------------------------------------------*/ 
 
/* Variable upgrade information about ID, size, and offsets */
#define OEM_HEADER_PRE_ALLOC_SIZE 12 
#define UBOOT_SIZE              (CONFIG_SPX_FEATURE_GLOBAL_UBOOT_ENV_START - CONFIG_SPX_FEATURE_GLOBAL_FLASH_START) 
#define BOOT_COMPONENT_SIZE     (UBOOT_SIZE + CONFIG_SPX_FEATURE_GLOBAL_UBOOT_ENV_SIZE)
#define APP_COMPONENT_OFFSET    BOOT_COMPONENT_SIZE
#define APP_COMPONENT_SIZE      (CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE - APP_COMPONENT_OFFSET)
#define ROOT_SECTION_NAME "root"
static ComponentFlashInfo_T ComponentFlashInfoTbl[MAX_COMPONENTS] = 
{
    /* Memoffset, Flashoffset, Sizetocpy, AllocateMemSize, DescString */
    { 0x0,     0x0,                  UBOOT_SIZE,         (BOOT_COMPONENT_SIZE + OEM_HEADER_PRE_ALLOC_SIZE), { 'B','O','O','T' } },
    { 0x0,     APP_COMPONENT_OFFSET, APP_COMPONENT_SIZE, (APP_COMPONENT_SIZE + OEM_HEADER_PRE_ALLOC_SIZE),  { 'A','P', 'P' }    },
    { 0x0,     0x0,                  0x0,                0x0,                                               { 0x0 }             }
};

static const INT8U ValidComponentIDTbl[MAX_COMPONENTS] = { 1, 1, 0, 0, 0, 0, 0, 0 };

/* static variables */
static HPMConfTargetCap_T HPMConfTargetCap;
static HPMConfCompProp_T HPMConfCompPropTbl[MAX_COMPONENTS];
static BOOL FwVersionCachedTbl[MAX_COMPONENTS][FWVER_TYPE_MAX];

/*-----------------------------------------------------
 *  Checking component related functions
 *-----------------------------------------------------*/ 
int IsValidComponentID(INT8U ComponentID)
{
    if(ComponentID >= MAX_COMPONENTS)
        return 0;
    else    
        return (ValidComponentIDTbl[ComponentID]);
}

static 
INT8U GetComponentBitMask(void)
{
    INT8U ComponentBitMask = 0x00;
    INT8U i;
    for(i = 0; i < MAX_COMPONENTS; i++)
       ComponentBitMask |= (ValidComponentIDTbl[i] << i);
    return ComponentBitMask;
}

BOOL IsValidComponents(INT8U Components)
{
    INT8U ComponentBitMask = 0, TempMask = 0;

    if(Components == 0)
        return 0;
        
    ComponentBitMask = GetComponentBitMask();
    TempMask = (ComponentBitMask ^ Components);
    
    return ((TempMask & ComponentBitMask) ==  TempMask);
}

BOOL IsOnlyOneComponent(INT8U Components)
{
    if((Components > 0) && (Components < 0xFF))   
        return ((Components & (Components - 1)) == 0);
    else
        return 0;
}

BOOL IsIPMCComponentID(INT8U ComponentID)
{
    return ((ComponentID == BOOT_COMPONENT_ID) || (ComponentID == APP_COMPONENT_ID));
}

BOOL IsContainIPMCComponents(INT8U Components)
{
    INT8U IPMCComponents = ((0x1 << BOOT_COMPONENT_ID) | (0x1 << APP_COMPONENT_ID));
    return (Components & IPMCComponents);
}

static 
BOOL IsFlashTwiceImgSize(void)
{
    if(CONFIG_SPX_FEATURE_GLOBAL_FLASH_SIZE*CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS >=
        (CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE * 2))
        return TRUE;
    else
        return FALSE;
}

BOOL IsBackupComponentSupport(INT8U Components)
{
    int i;

    if(Components == 0)
        return FALSE;

    for(i = 0; i < MAX_COMPONENTS; i++)
    {
        if(0x01 & (Components >> i))
        {
            /* This condition is only needed when single flash dual image */                            
            if(IsIPMCComponentID(i))
            {
                if(!IsFlashTwiceImgSize())
                    return FALSE;
            }   
            
            if (HPMConfCompPropTbl[i].GeneralCompProp.BitField.RollbackOrBackup == 0)
                return FALSE;
        }
    }
    
    return TRUE;
}

BOOL IsAutoRollbackSupport(INT8U ComponentID)
{
    if(ComponentID == INVALID_COMPONENT_ID)
    {
        return (HPMConfTargetCap.GlobalCap.BitField.ManualRollback == 1);
    }
    
    /* This condition is only needed when single flash dual image */     
    if(IsIPMCComponentID(ComponentID))
    {
        if(!IsFlashTwiceImgSize())
            return FALSE;
    }  
            
    if( (HPMConfCompPropTbl[ComponentID].GeneralCompProp.BitField.RollbackOrBackup != 0) &&
        (HPMConfTargetCap.GlobalCap.BitField.AutoRollbackOverridden == 0) &&
        (HPMConfTargetCap.GlobalCap.BitField.AutoRollback == 1) )
        return TRUE;
    else
        return FALSE;
}

BOOL IsManualRollbackSupport(INT8U ComponentID)
{
    if(ComponentID == INVALID_COMPONENT_ID)
    {
        return (HPMConfTargetCap.GlobalCap.BitField.ManualRollback == 1);
    }
    
    /* This condition is only needed when single flash dual image */    
    if(IsIPMCComponentID(ComponentID))
    {
        if(!IsFlashTwiceImgSize())
            return FALSE;
    }  
    
    if( (HPMConfCompPropTbl[ComponentID].GeneralCompProp.BitField.RollbackOrBackup != 0) &&
        (HPMConfTargetCap.GlobalCap.BitField.ManualRollback == 1) )
        return TRUE;
    else
        return FALSE;
}

BOOL IsDeferredActivationSupport(INT8U ComponentID)
{
    if(ComponentID == INVALID_COMPONENT_ID)
    {
        return (HPMConfTargetCap.GlobalCap.BitField.DeferredActivation == 1);
    }
    
        
    if( (HPMConfCompPropTbl[ComponentID].GeneralCompProp.BitField.DeferredActivation == 1) ||
        (HPMConfTargetCap.GlobalCap.BitField.DeferredActivation == 1) )
        return TRUE;
    else
        return FALSE;
}

int GetComponentFlashInfo(ComponentFlashInfo_T *ComponentFlashInfo, INT8U ComponentID)
{
    if((ComponentID < 0) || (ComponentID > MAX_COMPONENTS))
        return -1;
        
    memcpy(ComponentFlashInfo, &ComponentFlashInfoTbl[ComponentID], sizeof(ComponentFlashInfo_T));

  if(g_corefeatures.dual_image_support == ENABLED) 
  {                              
      /* if flashing image is 2 and component is not bootloader, we offset "total used flash size"*/ 
      if((g_FlashingImage==HPM_IMAGE_2) && (ComponentID !=BOOT_COMPONENT_ID)) 
           ComponentFlashInfo->Flashoffset+=CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE; 
   }    

    
    return 0;
}

/*-----------------------------------------------------
 *  Get firmware version related functions
 *-----------------------------------------------------*/   
BOOL IsFwVersionCached(INT8U ComponentID, FwVersionType_E FwVerType)
{
    return FwVersionCachedTbl[ComponentID][FwVerType];
}

void ClearAllCachedFwVersion(void)
{
    memset(FwVersionCachedTbl, 0, sizeof(FwVersionCachedTbl));
    return;
}

void CacheFwVersion(INT8U ComponentID, FwVersionType_E FwVerType)
{
    FwVersionCachedTbl[ComponentID][FwVerType] = TRUE;
    return;
}

#define FW_INFO_FILE "/proc/ractrends/Helper/FwInfo"
static 
void GetFirmwareVersion(unsigned int* Major,unsigned int* Minor,unsigned int* Aux)
{
    char aline[82];
    int Rev;
    int i = 0, count = 0;

    FILE* fp = fopen(FW_INFO_FILE,"rb");
    if(fp == NULL)
    {
        printf("Unable to find firmware version info!!!\n");
        *Major = 0;
        *Minor = 0;
        *Aux = 0;
        return;
    }

    fgets(aline,79,fp);

    for(i = 0; i < 79; i++)
    {
        if (aline[i] == '\0')
            break;
        if (aline[i] == '.')
            ++count;
    }

    *Aux = 0;
    if(count == 3)
        sscanf(aline,"FW_VERSION=%d.%d.%d.%d",Major,Minor,Aux,&Rev);
    else
        sscanf(aline,"FW_VERSION=%d.%d.%d",Major,Minor,Aux);

    fclose(fp);

    return;
}

/*
 * Took from flashlib for reading FMH
 */
static
int ReadOneBlock (int fd, void *Buffer, unsigned int BlockSize)
{
    int Count = 0;
    int nRead = 0;

    if(BlockSize == 0)
    {
 	TCRIT("ReadOneBlock called with request for 0 bytes!!Maybe unwise\n");
    }

    while (1)
    {
        nRead = read (fd, Buffer + Count, BlockSize - Count);
	//nRead can be 0 or -1 .0 is EOF and -1 is error
	if((nRead != 0) && (nRead != -1))
	{
		Count += nRead;
	}
	else
	{
		if(nRead == -1)
		{
			TCRIT("read returned -1\n");
			return -1;
		}
		else if(nRead == 0)
		{
			TCRIT("End of file encountered\n");
			return -1;
			//logic for returning error here is that caller of ReadOneBlock expects
			//a complete read of one block.if we cannot do it then we indicate error
			//since this is a abstraction that makes it easy to read one block
		}
	}

        if ((Count == BlockSize))
            return Count;
    }
    /* Control should never reach here */
    return Count;
}

static
int ReadBlock(unsigned long BlkAddress,char *DataBuffer)
{
    char * mtdDevice = DEFAULT_MTD_DEVICE;
    int fd = 0;

    fd = open (mtdDevice, O_RDWR);
    if (fd < 0)
    {
        TCRIT ("Cannot open mtd raw device %s. Exiting...\n", mtdDevice);
        return errno;
    }

    /*  Position fd pointer */
    lseek (fd, BlkAddress, SEEK_SET);
    
    INT32U EraseBlkSize = CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;
    /*  Read EraseBlockSize bytes at a time */
    if (-1 == ReadOneBlock (fd, DataBuffer, EraseBlkSize))
    {
    	TCRIT ("CRITICAL ERROR: Error reading device %s\n",strerror(errno));
		close(fd);
		return errno;
    }
    close(fd);
    return 0;
}

static
int GetFwVersionFromFMH(INT32U ReadOffset, INT32U ScanSize, FirmwareVersion_T *FwVersion)
{
    unsigned char *OneEBlock = NULL;
    FMH *FMHPtr = NULL;
    MODULE_INFO *ModulePtr = NULL;
	INT32U EraseBlkSize = CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;
    int i;
    
    OneEBlock = malloc(EraseBlkSize);
    if(OneEBlock == NULL)
    {
        TCRIT("Error in allocating memory \n");
        return -1;
    }
        
    
    /* 
     * Normal case should be forward scan.
     * The reason why it scans backward here is because I want to 
     * use the version of END fmh for APP Component as default.
     */
    for(i = ((ScanSize/EraseBlkSize) - 1); i >= 0; i--) //for(i = 0; i < (ScanSize / EraseBlkSize); i++)
	{
        memset(OneEBlock, 0xFF, EraseBlkSize);

        INT32U BlkAddress = ReadOffset + (i * EraseBlkSize);
		
        /*  Read EraseBlockSize bytes at a time */
        if (0 != ReadBlock(BlkAddress, (char *)OneEBlock))
        {
            TCRIT ("Error reading device %s\n",strerror(errno));
            free(OneEBlock);
            return -1;
        }

        /* Scan for FMH in the block just read */
        FMHPtr = ScanforFMH (OneEBlock, EraseBlkSize);
        if (FMHPtr == NULL)
        {
            TCRIT ("Can not find FMH\n");
            //free(OneEBlock);
            continue;
        }
        /* FMH module found!!    */
        /* Assuming that first FMH found is the product FMH */
        ModulePtr = &(FMHPtr->Module_Info);
        FwVersion->FWRev1 = ModulePtr->Module_Ver_Major;
        
        INT8U Minor = ModulePtr->Module_Ver_Minor;
        INT8U MinorBCD = ((Minor/10)<<4)+(Minor%10);
        
        FwVersion->FWRev2 = MinorBCD;
        FwVersion->AuxillaryFWRevInfo = atol((char*)ModulePtr->Module_Ver_Aux);
        break;

    }
    free(OneEBlock);
    
    return 0;
}

int 
GetCurrFirmwareVersion(INT8U ComponentID, FirmwareVersion_T *FwVersion)
{
    unsigned int Major, Minor, Aux;
    INT8U MinorBCD;
    INT32U StartOffset = 0;
    INT32U Flashoffset = ComponentFlashInfoTbl[ComponentID].Flashoffset;
    INT32U ReadOffset = StartOffset + Flashoffset;
    INT32U ScanSize = ComponentFlashInfoTbl[ComponentID].Sizetocpy;
  
    switch(ComponentID)
    {
       /* Get BOOT version from FMH*/
       case BOOT_COMPONENT_ID:	   
            GetFwVersionFromFMH(ReadOffset,ScanSize,FwVersion);
	break;		

	case APP_COMPONENT_ID:	
          GetFirmwareVersion(&Major,&Minor,&Aux);
          MinorBCD = ((Minor/10)<<4)+(Minor%10);
          FwVersion->FWRev1          		= (INT8U)Major;
          FwVersion->FWRev2          		= (INT8U)MinorBCD;
          FwVersion->AuxillaryFWRevInfo	= (INT32U)Aux;
	break;	  
    }
    return 0;
}

int CheckRollBackFirmwareExist(char *RunningImage)
{
    unsigned char *OneEBlock = NULL;
    FMH *FMHPtr = NULL; 	
    INT32U ReadOffset = 0;
    INT32U EraseBlkSize = CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;
    int i;
   
	
   if((g_corefeatures.dual_image_support == ENABLED) && ( g_corefeatures.hpm_rollback_support == ENABLED) )
   {
	  /* Here,beside dual-image support, we need to have backup part to store backup components for image1 and image2.
	  It requires at least 65Mb flash size to support it if rom size is 16Mb, now we don't have this test case.*/
   }
   else
   {
   	*RunningImage = HPM_IMAGE_1;
	  ReadOffset = CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE*2;
   }
    		 
    OneEBlock = malloc(EraseBlkSize);
    if(OneEBlock == NULL)
    {
        TCRIT("Error in allocating memory \n");
        return -1;
    }
        
    
     /* scan last 5 blocks to check version of END fmh, make sure backup image is existing
     */
   	for(i = 1;i<6;i++)
    {
        memset(OneEBlock, 0xFF, EraseBlkSize);

        INT32U BlkAddress = ReadOffset - (i * EraseBlkSize);
		
        /*  Read EraseBlockSize bytes at a time */
        if (0 != ReadBlock(BlkAddress, (char *)OneEBlock))
        {
            TCRIT ("Error reading device %s\n",strerror(errno));
            free(OneEBlock);
            return -1;
        }

        /* Scan for FMH in the block just read */
        FMHPtr = ScanforFMH (OneEBlock, EraseBlkSize);
	if (FMHPtr != NULL)
	        /* FMH module found!!    */	
               break;
					  
    }
	
    free(OneEBlock);
	 
    if (FMHPtr == NULL)
	return -1;
    else	
       return 0;
}

int 
GetRollbackFirmwareVersion(INT8U ComponentID, FirmwareVersion_T *FwVersion)
{
    INT32U StartOffset = CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE;
    INT32U Flashoffset = ComponentFlashInfoTbl[ComponentID].Flashoffset;
    INT32U ReadOffset = StartOffset + Flashoffset;
    INT32U ScanSize = ComponentFlashInfoTbl[ComponentID].Sizetocpy;
    int ret =0;
    char RunningImage = HPM_IMAGE_1;

   /*to avoid ipmitool timout, need to check APP backup firmware before get its version*/ 
    if(ComponentID==BOOT_COMPONENT_ID)
		ret=1;
    else
              ret = CheckRollBackFirmwareExist(&RunningImage);

   /*if backup image is not exist,  set fwRev=0; */   
    if(ret == -1)
    {
         FwVersion->FWRev1          		= 0;
         FwVersion->FWRev2          		= 0;
         FwVersion->AuxillaryFWRevInfo	= 0;	
		 
	  return 0;	 
    }
   else
   {
	 if((g_corefeatures.dual_image_support == ENABLED) && ( g_corefeatures.hpm_rollback_support == ENABLED ) )
	 {
	  /* Here,beside dual-image support, we need to have backup part to store backup components for image1 and image2. 
	  It requires at least 65Mb flash size to support it if rom size is 16Mb, now we don't have this test case.*/
	 }
	 
   }
    return ( GetFwVersionFromFMH(ReadOffset, ScanSize, FwVersion) );
}

int
GetDeferUpgFirmwareVersion(INT8U ComponentID, FirmwareVersion_T *FwVersion)
{
    INT32U StartOffset = 0;
    INT32U Flashoffset = ComponentFlashInfoTbl[ComponentID].Flashoffset;
    INT32U ReadOffset = 0;
    INT32U ScanSize = ComponentFlashInfoTbl[ComponentID].Sizetocpy;

   if((g_corefeatures.dual_image_support == ENABLED) && ( g_corefeatures.hpm_rollback_support == ENABLED) )
   {
	 /* Here,beside dual-image support, we need to have backup part to store backup components for image1 and image2.
	 It requires at least 65Mb flash size to support it if rom size is 16Mb, now we don't have this test case.*/
   }
   
    ReadOffset = StartOffset + Flashoffset;	 
    
    if(ACTIVATION_PENDING != GetFwUpgState(ComponentID))
        return -1;
    
    return ( GetFwVersionFromFMH(ReadOffset, ScanSize, FwVersion) );
}

int GetCompDescString(INT8U ComponentID, char *DescString, int StrSize)
{
    int DescStringLen = strlen((char *)ComponentFlashInfoTbl[ComponentID].DescString);
    
    memset(DescString, '\0', StrSize);
    if(StrSize >= DescStringLen)
        strncpy(DescString, (char *)ComponentFlashInfoTbl[ComponentID].DescString, DescStringLen);
    return 0;
}


/*-----------------------------------------
 * Read Configuration from HPM_CONF_FILE
 *-----------------------------------------*/ 
static 
int ReadIntConf(dictionary *d, char *SectionName, const char *KeyName, int *Value)
{
    INTU    err_value = 0xFFFFFFFF;
    char temp[MAX_TEMP_ARRAY_SIZE];
    INTU tempval;
    
    err_value = 0xFFFFFFFF;
    memset(temp, 0, sizeof(temp));
    sprintf(temp,"%s:%s",SectionName, KeyName);
    tempval = iniparser_getint (d, temp, err_value);
    if(tempval == err_value)
    {
        TINFO("Configuration %s is not found\n", KeyName);
        *Value = 0;
    }
    else
    {
        *Value = tempval;
        TDBG("Configured %s value is %x \n", KeyName, tempval);
    }
    
    return 0;   
}

static 
void ReadTargetCapConf(dictionary *d)
{
    int Value;

    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_UPGRADE_UNDERSIRABLE, &Value);
    HPMConfTargetCap.GlobalCap.BitField.FWUpgDesirable = (INT8U)Value;
    
    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_ROLLBACK_OVERRIDE, &Value);
    HPMConfTargetCap.GlobalCap.BitField.AutoRollbackOverridden = (INT8U)Value;
    
    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_IPMC_DEGRADED, &Value);
    HPMConfTargetCap.GlobalCap.BitField.IPMCDegradedInUpg = (INT8U)Value;
    
    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_DEFERRED_ACTIVATION, &Value);
    HPMConfTargetCap.GlobalCap.BitField.DeferredActivation = (INT8U)Value;

    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_SERVICE_AFFECTED, &Value);
    HPMConfTargetCap.GlobalCap.BitField.ServiceAffected = (INT8U)Value;

    /*ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_MANUAL_ROLLBACK, &Value);
    HPMConfTargetCap.GlobalCap.BitField.ManualRollback = (INT8U)Value;*/

    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_AUTO_ROLLBACK, &Value);
    HPMConfTargetCap.GlobalCap.BitField.AutoRollback = (INT8U)Value;
    
    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_SELFTEST, &Value);
    HPMConfTargetCap.GlobalCap.BitField.SelfTest = (INT8U)Value;
    
    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_UPGRADE_TIMEOUT, &Value);
    HPMConfTargetCap.UpgradeTimeout = (INT8U)Value;
    
    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_SELFTEST_TIMEOUT, &Value);
    HPMConfTargetCap.SelfTestTimeout = (INT8U)Value;
    
    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_ROLLBACK_TIMEOUT, &Value);
    HPMConfTargetCap.Rollbackimeout = (INT8U)Value;
    
    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_INACCESS_TIMEOUT, &Value);
    HPMConfTargetCap.InaccessiblityTimeout = (INT8U)Value;

    ReadIntConf(d, STR_SECTION_NAME_TARGETCAP, STR_COMP_PRESENCE, &Value);
    HPMConfTargetCap.ComponentsPresent = (INT8U)Value;

    if( g_corefeatures.hpm_rollback_support == ENABLED)    
	HPMConfTargetCap.GlobalCap.BitField.ManualRollback = 1;    
    else    
       HPMConfTargetCap.GlobalCap.BitField.ManualRollback = 0;
    	
                                    
    return;
}

static 
void ReadCompPropConf(dictionary *d, int CompID, char *SectionName)
{
    int Value;
   
    ReadIntConf(d, SectionName, STR_COLD_RESET_REQ, &Value);
    HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.PayloadColdResetRequired = (INT8U)Value;

    ReadIntConf(d, SectionName, STR_DEFERRED_ACTIVATION, &Value);
    HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.DeferredActivation = (INT8U)Value;
    
    ReadIntConf(d, SectionName, STR_COMPARISON, &Value);
    HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.Comparison = (INT8U)Value;
    
    ReadIntConf(d, SectionName, STR_PREPARATION, &Value);
    HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.Preparation = (INT8U)Value;
 
    ReadIntConf(d, SectionName, STR_ROLLBACK_BACKUP, &Value);
    HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.RollbackOrBackup = (INT8U)Value;

    if( g_corefeatures.hpm_rollback_support != ENABLED)	
    HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.RollbackOrBackup =0;	
                    
    return;
}
                
static 
void ReadHPMSectionNameConf(dictionary *d, char *SectionName)
{
    int i = 0;
    char temp[MAX_TEMP_ARRAY_SIZE];
    
    /* target upgrade capabilities section */
    if( 0 == strncmp(SectionName, STR_SECTION_NAME_TARGETCAP, strlen(STR_SECTION_NAME_TARGETCAP)) )
    {
        ReadTargetCapConf(d);
        return;
    }
    
    /* general properties of component properties section */
    for(i = 0; i < MAX_COMPONENTS; i++)
    {
        snprintf(temp, MAX_TEMP_ARRAY_SIZE, "%s%d", STR_SECTION_NAME_COMPPROP_PREFIX, i);
        if( 0 == strncmp(SectionName, temp, strlen(temp)) )
        {
            ReadCompPropConf(d, i, SectionName); 
            return;   
        }
    }
    return;
}
  
/*
 * @fn ReadAllHPMConf
 * @brief This function reads the HPM config file at once
 * @params 
 * @return Returns 0 on success, -1 on failure
 */
int ReadAllHPMConf(void)
{
    dictionary *d = NULL;

    char *sectionname=NULL;
    int nsec = 0, i = 0;

    d = iniparser_load(HPM_CONF_FILE);
    if( d == NULL )
    {
        TDBG("Unable to find/load/parse Configuration file : %s", HPM_CONF_FILE);
        return -1;
    }

    nsec = iniparser_getnsec(d);
    if(0 == nsec)
    {
        TDBG("Unable to locate section in ini file\n");
        iniparser_freedict(d);
        return -1;
    }

    for(i=0;i<nsec;i++)
    {
        sectionname = iniparser_getsecname (d, i);
        if(NULL == sectionname)
        {
            TINFO("Unable to get setion name of configuration file : %s", HPM_CONF_FILE);
            iniparser_freedict(d);
            return -1;
        }
        
        ReadHPMSectionNameConf(d, sectionname);
    }
    
    iniparser_freedict(d);
    return 0;
}

/*
 * @fn GetTargetCapConf
 * @brief This function get the target upgrade capabilities from saved static variable
 * @params TargetCap - target upgrade capabilities
 * @return Returns 0 on success, -1 on failure
 */
int GetTargetCapConf(HPMConfTargetCap_T *TargetCap)
{
    /* The values that are fixed and cannot be changed for now */
    HPMConfTargetCap.GlobalCap.BitField.AutoRollbackOverridden = 0;
    HPMConfTargetCap.GlobalCap.BitField.IPMCDegradedInUpg = 1;
    HPMConfTargetCap.GlobalCap.BitField.ServiceAffected = 1;
    HPMConfTargetCap.GlobalCap.BitField.AutoRollback=0;	 

    memcpy(TargetCap, &HPMConfTargetCap, sizeof(HPMConfTargetCap_T));
    
    return 0;
}

/*
 * @fn GetCompPropConf
 * @brief This function get the component properties from saved static variable
 * @params CompProp - general properties of component properties
 * @params CompID - component id
 * @return Returns 0 on success, -1 on failure
 */
int GetCompPropConf(HPMConfCompProp_T *CompProp, INT8U CompID)
{
    /* The values that are fixed and cannot be changed for now */
    HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.PayloadColdResetRequired = 1;
    HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.DeferredActivation = 1;
    HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.Preparation = 1;
    
   /* Only Rollback with backup component command or no rollback */
    if( ((HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.RollbackOrBackup != 0) &&
        (HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.RollbackOrBackup != 1) ) )
    {
        HPMConfCompPropTbl[CompID].GeneralCompProp.BitField.RollbackOrBackup = 0;
    }

    
    memcpy(CompProp, &HPMConfCompPropTbl[CompID], sizeof(HPMConfCompProp_T));
    
    return 0;    
}
