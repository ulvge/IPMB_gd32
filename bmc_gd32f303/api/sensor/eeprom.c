/*!
    \file    eeprom.c.c
    \brief

    \version
*/
#include "eeprom.h"
#include "FreeRTOS.h"
#include "bsp_i2c.h"
#include "bsp_i2c_gpio.h"
#include "string.h"
#include "task.h"
#include "tools.h"

#define EEPROM_I2C_BUS              I2C_BUS_S0
#define EEPROM_I2C_ADDR             0xA0
#define EEPROM_PAGE_BYTES           8
#define EEPROM_24C02_ADDRESS_LEN    1

#define EEPROM_RW_RETRY_TIMES       2
#define EEPROM_WRITE_PAGE_DLEAY_XMS 10

static void EEP_Delay(void)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskDelay(EEPROM_WRITE_PAGE_DLEAY_XMS);
    } else {
        Delay_NoSchedue(2500 * EEPROM_WRITE_PAGE_DLEAY_XMS); // 250:0.1ms
    }
}
static bool EEP_ReadPage(INT32U readAddr, uint8_t *pReadData, uint8_t readLen)
{
    /*
    low 16bit, The meaning of the name
    hi 16bit: regAddressLenth,default=0,if val=0|1,means len=1;if val=2, means len=2
    */
    uint32_t regAddress = (EEPROM_24C02_ADDRESS_LEN << 16) | readAddr;
    uint8_t maxCount = EEPROM_RW_RETRY_TIMES;
    do {
        if (i2c_read(EEPROM_I2C_BUS, EEPROM_I2C_ADDR, regAddress, pReadData, readLen)) {
            return true;
        }
    } while (maxCount--);
    return false;
}
static bool EEP_WritePage(INT8U devAddr, INT16U writeAddr, const uint8_t *pWriteBuf, uint8_t writeSize)
{
    uint8_t sendbuff[EEPROM_PAGE_BYTES + 3];
    uint8_t idx = 0;
    uint8_t maxCount = EEPROM_RW_RETRY_TIMES;

    if (writeSize > EEPROM_PAGE_BYTES) {
        return false;
    }

    switch (EEPROM_24C02_ADDRESS_LEN) {
    case 1:
        sendbuff[idx++] = devAddr;
        sendbuff[idx++] = (INT8U)writeAddr;
        memcpy(sendbuff + idx, pWriteBuf, writeSize);
        break;
    case 2:
        sendbuff[idx++] = devAddr;
        sendbuff[idx++] = (INT8U)(writeAddr >> 8);
        sendbuff[idx++] = (INT8U)writeAddr;
        memcpy(sendbuff + idx, pWriteBuf, writeSize);
        break;
    default:
        return false;
    }

    do {
        if (i2c_write(EEPROM_I2C_BUS, sendbuff, writeSize + idx)) {
            return true;
        }
    } while (maxCount--);
    return false;
}
BOOL EEP_ReadData(INT32U readAddr, INT8U *pReadData, INT16U readLen)
{
    INT16U i, offset = 0;
    for (i = 0; i <= (readLen - EEPROM_PAGE_BYTES); i += EEPROM_PAGE_BYTES) {
        if (EEP_ReadPage(readAddr + offset, pReadData + offset, EEPROM_PAGE_BYTES) == false) {
            return false;
        }
        offset += EEPROM_PAGE_BYTES;
        EEP_Delay();
    }
    if (readLen % EEPROM_PAGE_BYTES) {
        if (EEP_ReadPage(readAddr + offset, pReadData + offset, readLen % EEPROM_PAGE_BYTES) == false) {
            return false;
        }
        EEP_Delay();
    }
    return TRUE;
}
BOOL EEP_WriteData(INT16U writeAddr, INT8U *pWriteData, INT16U wirteLen)
{
    INT16U i, offSet, firstPageWirteLen;
    // 1、start ->page start
    if (wirteLen == 0)
        return TRUE;

    firstPageWirteLen = EEPROM_PAGE_BYTES - (writeAddr % EEPROM_PAGE_BYTES);
    if (firstPageWirteLen) {
        if (wirteLen <= firstPageWirteLen) {
            return EEP_WritePage(EEPROM_I2C_ADDR, writeAddr, pWriteData, wirteLen);
        } else {
            if (EEP_WritePage(EEPROM_I2C_ADDR, writeAddr, pWriteData, firstPageWirteLen) == false) {
                return false;
            }
            wirteLen -= firstPageWirteLen;
            EEP_Delay();
        }
    }

    // 2、page * N
    offSet = firstPageWirteLen;
    for (i = 0; i < (wirteLen / EEPROM_PAGE_BYTES); i++) {
        if (EEP_WritePage(EEPROM_I2C_ADDR, writeAddr + offSet, pWriteData + offSet, EEPROM_PAGE_BYTES) == false) {
            return false;
        }
        offSet += EEPROM_PAGE_BYTES;
        EEP_Delay();
    }
    // 3、The rest of page
    if (wirteLen % EEPROM_PAGE_BYTES) {
        if (EEP_WritePage(EEPROM_I2C_ADDR, writeAddr + offSet, pWriteData + offSet, wirteLen % EEPROM_PAGE_BYTES) == false) {
            return false;
        }
    }
    return TRUE;
}

/// @brief Read first, if not equal, then write
/// @param writeAddr
/// @param pWriteData
/// @param wirteLen
/// @return
BOOL EEP_WriteDataCheckFirst(INT16U writeAddr, INT8U *pWriteData, INT16U wirteLen)
{
    INT8U *pReadBuff = (INT8U *)pvPortMalloc(wirteLen);
    if (pReadBuff == NULL) {
        return false;
    }
    if (EEP_ReadData(writeAddr, pReadBuff, wirteLen) == false) {
        vPortFree(pReadBuff);
        return false;
    }
    if (memcmp(pWriteData, pReadBuff, wirteLen) == 0) {
        vPortFree(pReadBuff);
        return true;
    }
    vPortFree(pReadBuff);
    return EEP_WriteData(writeAddr, pWriteData, wirteLen);
}
