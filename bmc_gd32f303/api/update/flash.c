/*!
    \file  gd32f1x0_it.c
    \brief interrupt service routines
*/

/*
    Copyright (C) 2017 GigaDevice

    2014-12-26, V1.0.0, platform GD32F1x0(x=3,5)
    2016-01-15, V2.0.0, platform GD32F1x0(x=3,5,7,9)
    2016-04-30, V3.0.0, firmware update for GD32F1x0(x=3,5,7,9)
    2017-06-19, V3.1.0, firmware update for GD32F1x0(x=3,5,7,9)
*/

#include "gd32f20x.h" 
#include "gd32f20x_gpio.h"
#include "flash.h"


/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/

TestStatus FLASH_Program(uint32_t WRITE_START_ADDR, uint16_t Size, uint32_t * data)
{
    uint32_t Address;
    TestStatus TransferStatus = FAILED;
    uint32_t i;
    TransferStatus = PASSED;
    /* Unlock the Flash Bank1 Program Erase controller */
    fmc_unlock();
    
    /* Clear All pending flags */
    fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    
    
    /* Program Flash Bank1 */
    Address = WRITE_START_ADDR;
    i = 0;
    while(Address < (WRITE_START_ADDR + Size))
    {
        fmc_word_program(Address, data[i]);
        i++;
        Address = Address + 4; 
        fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    }
    
    fmc_lock();
    
    return TransferStatus;
}

TestStatus CRC_FLASH(uint32_t START_ADDR,uint32_t Size,uint16_t CRC_Value) 
{
    uint8_t *p = (uint8_t*)START_ADDR;
    uint32_t len = Size/4 -1;
    uint8_t temp[4] = {0};
    rcu_periph_clock_enable(RCU_PMU);
    rcu_periph_clock_enable(RCU_CRC);
    CRC_CTL = CRC_CTL_RST;
    do
    {
        temp[0] = *(p+3);
        temp[1] = *(p+2);
        temp[2] = *(p+1);
        temp[3] = *p;
        CRC_DATA = * (uint32_t *)temp;
        p = p + 4;
    }
    while (len--);
    if((CRC_DATA & 0xFFFF) != (uint32_t)CRC_Value)
    {
        return FAILED;
    }
    else
    {
        return PASSED;
    }

}

void read_memory(uint32_t address, uint8_t *data, uint8_t data_num)
{
    uint8_t i;
    uint8_t *p = (uint8_t *)address;
    for(i = 0; i < data_num; i++){
        data[i] = *p;
        p++;
    }
}

void read_fmc_pid(uint8_t *data,uint8_t len)
{
    if(len < 4 )
       return;
    else{

         uint32_t temp = FMC_PID;
         data[0] = (uint8_t)(temp & 0XFF);
         data[1] = (uint8_t)((temp >> 8) & 0XFF);
         data[2] = (uint8_t)((temp >> 16) & 0XFF);
         data[3] = (uint8_t)((temp >> 24) & 0XFF);
    }
}

void erase_page(uint16_t num_pages, uint16_t page_num)
{
    uint16_t i;

    uint32_t base_address = 0x08000000;
    uint32_t page_address = base_address + (FMC_PAGE_SIZE * page_num);
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    for(i = 0; i < num_pages; i++){
        fmc_page_erase(page_address);
        page_address = page_address + FMC_PAGE_SIZE;
        fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    }
}
