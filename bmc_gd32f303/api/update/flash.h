#ifndef __FLASH_H
#define __FLASH_H

#include "project_select.h"

typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;

#define FMC_PAGE_SIZE           ((uint16_t)0x800)

TestStatus FLASH_Program(uint32_t WRITE_START_ADDR,uint16_t Size,uint32_t * data);
TestStatus CRC_FLASH(uint32_t START_ADDR,uint32_t Size,uint16_t CRC_Value);
void read_memory(uint32_t address, uint8_t *data, uint8_t data_num);
void read_fmc_pid(uint8_t *data,uint8_t len);
void erase_page(uint16_t num_pages, uint16_t page_num);

#endif
