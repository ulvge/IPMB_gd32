/*!
    \file  gd25qxx.c
    \brief SPI flash gd25qxx driver
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, demo for GD32F20x
    2017-06-05, V2.0.0, demo for GD32F20x
*/

#include "api_flash.h"
#include "bsp_gd25qxx.h"
#include "ff.h"


static __IO uint32_t SPITimeout = SPIT_LONG_TIMEOUT;
static volatile DSTATUS TM_FATFS_FLASH_SPI_Stat = STA_NOINIT; /* Physical drive status */

/*******************************************************************************
* Function Name  : SPI_FLASH_Init
* Description    : Initializes the peripherals used by the SPI FLASH driver.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
DSTATUS TM_FATFS_FLASH_SPI_disk_initialize(void)
{
	// spi_flash_init();
	if (SFLASH_ID == SPI_FLASH_ReadID()) /*���FLASH�Ƿ���������*/
	{
		return TM_FATFS_FLASH_SPI_Stat &= ~STA_NOINIT; /* Clear STA_NOINIT flag */
	}
	else
	{
		return TM_FATFS_FLASH_SPI_Stat |= STA_NOINIT;
	}
}

DSTATUS TM_FATFS_FLASH_SPI_disk_status(void)
{
	FLASH_DEBUG_FUNC();
	if (SFLASH_ID == SPI_FLASH_ReadID()) /*���FLASH�Ƿ���������*/
	{
		return TM_FATFS_FLASH_SPI_Stat &= ~STA_NOINIT; /* Clear STA_NOINIT flag */
	}
	else
	{
		return TM_FATFS_FLASH_SPI_Stat |= STA_NOINIT;
	}
}

DRESULT TM_FATFS_FLASH_SPI_disk_ioctl(BYTE cmd, char *buff)
{
	FLASH_DEBUG_FUNC();
	switch (cmd)
	{
	case GET_SECTOR_SIZE:	  // Get R/W sector size (WORD)
		*(WORD *)buff = 4096; //flash��Сд��ԪΪҳ��256�ֽڣ��˴�ȡ2ҳΪһ����д��λ
		break;
	case GET_BLOCK_SIZE:	// Get erase block size in unit of sector (DWORD)
		*(DWORD *)buff = 1; //flash��4kΪ��С������λ
		break;
	case GET_SECTOR_COUNT:
		*(DWORD *)buff = 1536; //sector����
		break;
	case CTRL_SYNC:
		break;
	default:
		break;
	}

	return RES_OK;
}

DRESULT TM_FATFS_FLASH_SPI_disk_read(BYTE *buff, DWORD sector, UINT count)
{
	FLASH_DEBUG_FUNC();
	if ((TM_FATFS_FLASH_SPI_Stat & STA_NOINIT))
	{
		return RES_NOTRDY;
	}
	sector += 512; //����ƫ�ƣ��ⲿFlash�ļ�ϵͳ�ռ�����ⲿFlash����6M�ռ�
	SPI_FLASH_BufferRead(buff, sector << 12, count << 12);
	return RES_OK;
}

DRESULT TM_FATFS_FLASH_SPI_disk_write(BYTE *buff, DWORD sector, UINT count)
{
	uint32_t write_addr;
	FLASH_DEBUG_FUNC();
	sector += 512; //����ƫ�ƣ��ⲿFlash�ļ�ϵͳ�ռ�����ⲿFlash����6M�ռ�
	write_addr = sector << 12;
	SPI_FLASH_SectorErase(write_addr);
	SPI_FLASH_BufferWrite(buff, write_addr, 4096);
	return RES_OK;
}


FATFS g_fs; /* Work area (file system object) for logical drives */
FIL g_fnew; /* file objects */

void fatfs_init(void)
{
	uint32_t flash_id = 0;
	u32 total, free;
	FRESULT res_flash;

	LOG_I("fatfs start\r\n");
	/* configure SPI0 GPIO and parameter */
	spi_flash_init();

	/* GD32207i-EVAL-V1.0 start up */
	LOG_I("fatfs init.");
	LOG_I("GD32207 Flash:%dK", *(__IO uint16_t *)(0x1FFFF7E0));
	/* get flash id */
	flash_id = spi_flash_read_id();
	LOG_I("The Flash_ID:0x%X", flash_id);

	res_flash = f_mount(&g_fs, "0:", 1);
	LOG_I("f_mount res_flash=%d", res_flash);

	if (res_flash == FR_NO_FILESYSTEM)
	{
		res_flash = f_mkfs("0:", 0, 4096);
		LOG_I("mkfs res_flash=%d", res_flash);
		res_flash = f_mount(&g_fs, "0:", 0);
		res_flash = f_mount(&g_fs, "0:", 1);
		f_setlabel((const TCHAR *)"0:FLASH"); // set disk name
	}

	exf_getfree("0:", &total, &free);
	LOG_I("SPI Flash total size: %d KB", total);
	LOG_I("SPI Flash free size: %d KB", free);

	// res_flash = f_open(&fnew, "0:Log.txt", FA_CREATE_ALWAYS | FA_WRITE);

	// if (res_flash == FR_OK)
	// {
	// 	LOG_I("write \"hello world\" to flash");
	// 	res_flash = f_write(&fnew, "hello world", sizeof("hello world"), &bw);
	// 	f_close(&fnew);
	// }

	// LOG_I("read from flash: ");
	// res_flash = f_open(&fnew, "0:Log.txt", FA_OPEN_EXISTING | FA_READ);
	// res_flash = f_read(&fnew, buffer, sizeof(buffer), &br);
	// f_close(&fnew);
	// LOG_I("%s ", buffer);
}

//drv:("0:"/"1:")
u8 exf_getfree(char *drv, u32 *total, u32 *free)
{
	FATFS *fs1;
	u8 res;
	u32 fre_clust = 0, fre_sect = 0, tot_sect = 0;

	res = (u32)f_getfree((const TCHAR *)drv, (DWORD *)&fre_clust, &fs1);
	if (res == 0)
	{
		tot_sect = (fs1->n_fatent - 2) * fs1->csize; 
		fre_sect = fre_clust * fs1->csize;			 
#if _MAX_SS != 512						
		tot_sect *= fs1->ssize / 512;
		fre_sect *= fs1->ssize / 512;
#endif
		*total = tot_sect >> 1; // KB
		*free = fre_sect >> 1;	// KB
	}
	return res;
}
