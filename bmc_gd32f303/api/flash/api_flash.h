/*!
    \file  gd25qxx.h
    \brief the header file of SPI flash gd25qxx driver
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, demo for GD32F20x
    2017-06-05, V2.0.0, demo for GD32F20x
*/

#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include "bsp_gd25qxx.h"
#include "gd32f20x.h"

#include "diskio.h"
#include "integer.h"

#include "OSPort.h"


#ifdef __cplusplus
 extern "C" {
#endif
	 

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

#define SFLASH_ID                0xC84015


#define SPIT_FLAG_TIMEOUT         ((uint32_t)0x1000)
#define SPIT_LONG_TIMEOUT         ((uint32_t)(10 * SPIT_FLAG_TIMEOUT))


#define FLASH_DEBUG_ON         0
#define FLASH_DEBUG_FUNC_ON    0

#define FLASH_INFO(fmt,arg...)           LOG_I("<<-FLASH-INFO->> "fmt"\n",##arg)
#define FLASH_ERROR(fmt,arg...)          LOG_I("<<-FLASH-ERROR->> "fmt"\n",##arg)
#define FLASH_DEBUG(fmt,arg...)          do{\
                                          if(FLASH_DEBUG_ON)\
                                          LOG_I("<<-FLASH-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                          }while(0)

#define FLASH_DEBUG_FUNC()               do{\
                                         if(FLASH_DEBUG_FUNC_ON)\
                                         LOG_I("<<-FLASH-FUNC->> Func:%s@Line:%d\n",__func__,__LINE__);\
                                       }while(0)

																			 
#define  SPI_FLASH_ReadID                        spi_flash_read_id 
#define  SPI_FLASH_SectorErase(addr)             spi_flash_sector_erase(addr) 
#define  SPI_FLASH_BulkErase                     spi_flash_bulk_erase 
#define  SPI_FLASH_BufferRead(buf, addr, len)    spi_flash_buffer_read(buf, addr, len)
#define  SPI_FLASH_BufferWrite(buf, addr, len)   spi_flash_buffer_write(buf, addr, len)
#define  SPI_FLASH_PageWrite(buf, addr, len)     spi_flash_page_write(buf, addr, len)



DSTATUS TM_FATFS_FLASH_SPI_disk_initialize(void);
DSTATUS TM_FATFS_FLASH_SPI_disk_status(void) ;
DRESULT TM_FATFS_FLASH_SPI_disk_ioctl(BYTE cmd, char *buff) ;
DRESULT TM_FATFS_FLASH_SPI_disk_read(BYTE *buff, DWORD sector, UINT count) ;
DRESULT TM_FATFS_FLASH_SPI_disk_write(BYTE *buff, DWORD sector, UINT count) ;


u32 SPI_FLASH_ReadDeviceID(void);
void SPI_FLASH_StartReadSequence(u32 ReadAddr);
void SPI_Flash_PowerDown(void);
void SPI_Flash_WAKEUP(void);


//u8 SPI_FLASH_ReadByte(void);
//u8 SPI_FLASH_SendByte(u8 byte);
//u16 SPI_FLASH_SendHalfWord(u16 HalfWord);
void SPI_FLASH_WriteEnable(void);
void SPI_FLASH_WaitForWriteEnd(void);
int GetGBKCode_from_EXFlash(unsigned char* pBuffer,const unsigned char * c);

void fatfs_init(void);
u8 exf_getfree(char *drv, u32 *total, u32 *free);

#ifdef __cplusplus
}
#endif

#endif /* SPI_FLASH_H */
