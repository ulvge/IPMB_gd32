/*****************************************************************
******************************************************************
******************************************************************
******************************************************************
******************************************************************
*
* libpmbus.c
*
* PMBus read/write functions
*
*
******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <types.h>
#include <libpmb.h>
#include <debug_print.h>
#include <bsp_i2c.h>

static int PEC_Verify = 0;
int PEC_Disable = 0;

const PMBus_t PMBusStdCmds[] =
    {
        {PMBUS_PAGE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_OPERATION, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_ON_OFF_CONFIG, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_CLEAR_FAULTS, I2C_SEND_BYTE, 0},
        {PMBUS_PHASE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_PAGE_PLUS_WRITE, I2C_WRITE, PMB_BLOCK},
        {PMBUS_PAGE_PLUS_READ, I2C_READ, PMB_BLOCK},
        {PMBUS_WRITE_PROTECT, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_STORE_DEFAULT_ALL, I2C_SEND_BYTE, 0},
        {PMBUS_RESTORE_DEFAULT_ALL, I2C_SEND_BYTE, 0},
        {PMBUS_STORE_DEFAULT_CODE, I2C_WRITE, PMB_BYTE},
        {PMBUS_RESTORE_DEFAULT_CODE, I2C_WRITE, PMB_BYTE},
        {PMBUS_STORE_USER_ALL, I2C_SEND_BYTE, 0},
        {PMBUS_RESTORE_USER_ALL, I2C_SEND_BYTE, 0},
        {PMBUS_STORE_USER_CODE, I2C_WRITE, PMB_BYTE},
        {PMBUS_RESTORE_USER_CODE, I2C_WRITE, PMB_BYTE},
        {PMBUS_CAPABILITY, I2C_READ, PMB_BYTE},
        //{PMBUS_QUERY, I2C_READ_WRITE, PMB_BLOCK_PROC_CALL},
        {PMBUS_SMBALERT_MASK, I2C_WRITE, PMB_WORD},
        {PMBUS_VOUT_MODE, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_COMMAND, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_TRIM, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_CAL_OFFSET, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_MAX, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_MARGIN_HIGH, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_MARGIN_LOW, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_TRANSITION_RATE, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_DROOP, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_SCALE_LOOP, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_SCALE_MONITOR, I2C_READ_WRITE, PMB_WORD},
        //{PMBUS_COEFFICIENTS, I2C_READ_WRITE, PMB_BLOCK_PROC_CALL},
        {PMBUS_POUT_MAX, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_MAX_DUTY, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_FREQUENCY_SWITCH, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VIN_ON, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VIN_OFF, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_INTERLEAVE, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_IOUT_CAL_GAIN, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_IOUT_CAL_OFFSET, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_FAN_CONFIG_1_2, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_FAN_COMMAND_1, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_FAN_COMMAND_2, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_FAN_CONFIG_3_4, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_READ_EIN, I2C_READ, PMB_BLOCK},
        {PMBUS_FAN_COMMAND_3, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_FAN_COMMAND_4, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_OV_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_OV_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_VOUT_OV_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_UV_WARN_LIMT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_UV_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VOUT_UV_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_IOUT_OC_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_IOUT_OC_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_IOUT_OC_LV_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_IOUT_OC_LV_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_IOUT_OC_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_IOUT_UC_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_IOUT_UC_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_OT_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_OT_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_OT_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_UT_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_UT_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_UT_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_VIN_OV_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VIN_OV_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_VIN_OV_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VIN_UV_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VIN_UV_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_VIN_UV_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_IIN_OC_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_IIN_OC_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_IIN_OC_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_POWER_GOOD_ON, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_POWER_GOOD_OFF, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_TON_DELAY, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_TON_RISE, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_TON_MAX_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_TON_MAX_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_TOFF_DELAY, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_TOFF_FALL, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_TOFF_MAX_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_POUT_OP_FAULT_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_POUT_OP_FAULT_RESPONSE, I2C_READ_WRITE, PMB_BYTE},
        {PMBUS_POUT_OF_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_PIN_OP_WARN_LIMIT, I2C_READ_WRITE, PMB_WORD},
        {PMBUS_STATUS_BYTE, I2C_READ, PMB_BYTE},
        {PMBUS_STATUS_WORD, I2C_READ, PMB_WORD},
        {PMBUS_STATUS_VOUT, I2C_READ, PMB_BYTE},
        {PMBUS_STATUS_IOUT, I2C_READ, PMB_BYTE},
        {PMBUS_STATUS_INPUT, I2C_READ, PMB_BYTE},
        {PMBUS_STATUS_TEMPERATURE, I2C_READ, PMB_BYTE},
        {PMBUS_STATUS_CML, I2C_READ, PMB_BYTE},
        {PMBUS_STATUS_OTHER, I2C_READ, PMB_BYTE},
        {PMBUS_STATUS_MFR_SPECIFIC, I2C_READ, PMB_BYTE},
        {PMBUS_STATUS_FANS_1_2, I2C_READ, PMB_BYTE},
        {PMBUS_STATUS_FANS_3_4, I2C_READ, PMB_BYTE},
        {PMBUS_READ_VIN, I2C_READ, PMB_WORD},
        {PMBUS_READ_IIN, I2C_READ, PMB_WORD},
        {PMBUS_READ_VCAP, I2C_READ, PMB_WORD},
        {PMBUS_READ_VOUT, I2C_READ, PMB_WORD},
        {PMBUS_READ_IOUT, I2C_READ, PMB_WORD},
        {PMBUS_READ_TEMPERATURE_1, I2C_READ, PMB_WORD},
        {PMBUS_READ_TEMPERATURE_2, I2C_READ, PMB_WORD},
        {PMBUS_READ_TEMPERATURE_3, I2C_READ, PMB_WORD},
        {PMBUS_READ_FAN_SPEED_1, I2C_READ, PMB_WORD},
        {PMBUS_READ_FAN_SPEED_2, I2C_READ, PMB_WORD},
        {PMBUS_READ_FAN_SPEED_3, I2C_READ, PMB_WORD},
        {PMBUS_READ_FAN_SPEED_4, I2C_READ, PMB_WORD},
        {PMBUS_READ_DUTY_CYCLE, I2C_READ, PMB_WORD},
        {PMBUS_READ_FREQUENCY, I2C_READ, PMB_WORD},
        {PMBUS_READ_POUT, I2C_READ, PMB_WORD},
        {PMBUS_READ_PIN, I2C_READ, PMB_WORD},
        {PMBUS_PMBUS_REVISION, I2C_READ, PMB_BYTE},
        {PMBUS_MFR_ID, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_MFR_MODEL, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_MFR_REVISION, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_MFR_LOCATION, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_MFR_DATE, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_MRF_SERIAL, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_MFR_VIN_MIN, I2C_READ, PMB_WORD},
        {PMBUS_MFR_VIN_MAX, I2C_READ, PMB_WORD},
        {PMBUS_MFR_IIN_MAX, I2C_READ, PMB_WORD},
        {PMBUS_MFR_PIN_MAX, I2C_READ, PMB_WORD},
        {PMBUS_MFR_VOUT_MIN, I2C_READ, PMB_WORD},
        {PMBUS_MFR_VOUT_MAX, I2C_READ, PMB_WORD},
        {PMBUS_MFR_IOUT_MAX, I2C_READ, PMB_WORD},
        {PMBUS_MFR_POUT_MAX, I2C_READ, PMB_WORD},
        {PMBUS_MFR_TAMBIENT_MAX, I2C_READ, PMB_WORD},
        {PMBUS_MFR_TAMBIENT_MIN, I2C_READ, PMB_WORD},
        {PMBUS_MFR_EFFICIENCY_LL, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_MFR_EFFICIENCY_HL, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_00, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_01, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_02, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_03, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_04, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_05, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_06, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_07, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_08, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_09, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_10, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_11, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_12, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_13, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_14, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_USER_DATA_15, I2C_READ_WRITE, PMB_BLOCK},
        {PMBUS_MFR_MAX_TEMP_1, I2C_READ, PMB_WORD},
        {PMBUS_MFR_MAX_TEMP_2, I2C_READ, PMB_WORD}};

static const PMBus_t *PMBus_getCmds(u8 cmdCode)
{
    for (u8 i = 0; i < ARRARY_SIZE(PMBusStdCmds); i++)
    {
        if (PMBusStdCmds[i].cmdName == cmdCode)
        {
            // return (const PMBus_t *)&PMBusStdCmds[i];
            return (const PMBus_t *)&PMBusStdCmds[i];
        }
    }
    return NULL;
}
static int PMBUS_i2c_master_write(INT8U i2cBus, u8 slave, u8 *writebuf, int writelen)
{
    return i2c_write(i2cBus, writebuf, writelen);
}

static int PMBUS_i2c_writeread(INT8U i2cBus, u8 slave, u8 cmd, int cmdlen, u8 *readbuf, int readlen)
{
    if (cmdlen != 1)
    {
        LOG_E("\nInvalid Command length");
        return -1;
    }
    return i2c_read(i2cBus, slave, cmd, readbuf, readlen);
}

/**
 *@fn PMBUS_Crc8
 *@brief This function is invoked to calculate CRC value
 *@param data - Contains data to which the CRC calculated
 */
#define POLYNOMIAL (0x1070U << 3)

unsigned char PMBUS_Crc8(unsigned char inCrc, unsigned char inData)
{
    int i;
    unsigned short data;

    data = inCrc ^ inData;
    data <<= 8;

    for (i = 0; i < 8; i++)
    {
        if ((data & 0x8000) != 0)
        {
            data = data ^ POLYNOMIAL;
        }
        data = data << 1;
    }

    return (unsigned char)(data >> 8);

} // PMBUS_Crc8

/**
 *@fn PMBUS_Add_PEC
 *@brief This function is invoked to Append PEC value to the data
 *@param i2c_dev - Name of i2c device to perform Write
 *@param slave - Slave Address
 *@param Writebuf - Calculated PEC value is Appended to this pointer Variable
 *@param Writelen - Length of Writebuf
 */
int PMBUS_Add_PEC(u8 slave, u8 *writebuf, int writelen)
{
    int i;
    unsigned char crc = 0;

    slave = slave << 1;           // convert 7-Bit address format to 8-Bit format
    crc = PMBUS_Crc8(crc, slave); // first byte is slave address

    for (i = 0; i < writelen; i++)
        crc = PMBUS_Crc8(crc, writebuf[i]);
    writebuf[writelen] = crc; // Add PEC behind write data
    return 0;
}

/**
 *@fn PMBUS_Verify_PEC
 *@brief This function is invoked to Verify the PEC
 *@param i2c_dev - Name of i2c device to perform Write
 *@param slave - Slave Address
 *@param writebuf - pointer variable which contains data to write on pmbus
 *@param readbuf - pointer variable which contains read data from pmbus
 *@param readlen - Length of the readbuf
 *@return Returns 0 on success
 *        Returns -1 on failure
 */
int PMBUS_Verify_PEC(u8 slave, u8 *writebuf, u8 *readbuf, int readlen)
{
    int count = 0;
    unsigned char crc = 0;
    unsigned char Recvdbyte[32];
    Recvdbyte[0] = (slave << 1) | !!(I2C_WRITE & I2C_M_RD);
    Recvdbyte[1] = writebuf[0];
    Recvdbyte[2] = (slave << 1) | !!(I2C_READ & I2C_M_RD);

    memcpy(&Recvdbyte[3], &readbuf[0], readlen);

    for (count = 0; count < (readlen + 3); count++)
        crc = PMBUS_Crc8(crc, Recvdbyte[count]);

    if (crc != readbuf[readlen])
    {
        LOG_E("Bad PEC 0x%02x vs. 0x%02x\n",
              crc, readbuf[readlen]);
        return -1;
    }
    return 0;
}

/**
 *@fn PMBUS_PECEnable
 *@brief This function is invoked to check Packet error checking is enabled or not
 *@param i2c_dev - Name of i2c device to perform Write
 *@param slave - Slave Address
 */
void PMBUS_PECEnable(INT8U i2cBus, u8 slave)
{
    int ret;
    u8 read_buf;
    u8 RSbyte;
    PEC_Verify = -1;

    if (PEC_Disable == 1)
    {
        PEC_Verify = 0;
        return;
    }

    ret = PMBus_I2CRead(i2cBus, slave, PMBUS_CAPABILITY, &read_buf);
    if (ret < 0)
    {
        PEC_Verify = 0;
        return;
    }
    RSbyte = read_buf;
    if (!(RSbyte >> 7))
    {
        PEC_Verify = 0;
    }
    else
    {
        PEC_Verify = 1;
    }
}

/**
 *@fn PMBus_I2CRead
 *@brief This function is invoked to Read Byte/Word on i2c deive using i2c_rw protocal
 *@param i2c_dev - Name of i2c device to perform Read
 *@param slave - Slave Address
 *@param cmdCode - command code of the pmbus device
 *@param read_buf - contains readed data from i2c device
 *@return Returns no of bytes readed on success
 *        Returns -1 on failure
 */
int PMBus_I2CRead(INT8U i2cBus, u8 slave, u8 cmdCode, u8 *readbuf)
{
    int i, ret, j;
    int readlen = 0;
    int writelen = 1;
    u8 writebuf;

    if (PEC_Verify == 0)
    {
        PMBUS_PECEnable(i2cBus, slave);
    }

    const PMBus_t *pPMBusCmd = PMBus_getCmds(cmdCode);
    if (pPMBusCmd == NULL)
    {
        return -1;
    }
    if ((pPMBusCmd->transType != I2C_READ) && (pPMBusCmd->transType != I2C_READ_WRITE))
    {
        LOG_E("\nInvalid Request for this Command 0x%02x", pPMBusCmd->cmdName);
        return -1;
    }
    writelen = 1;
    writebuf = cmdCode;
    if ((pPMBusCmd->dataBytes == PMB_BLOCK) || (pPMBusCmd->dataBytes == PMB_RAW))
    {
        readlen = readbuf[0];
    }
    else
    {
        readlen = pPMBusCmd->dataBytes;
    }

    if (PEC_Verify == 1)
    {
        readlen++;
    }
    ret = PMBUS_i2c_writeread(i2cBus, slave, writebuf, writelen, readbuf, readlen);
    if (ret < 0)
    {
        return -1;
    }
    if (PEC_Verify == 1)
    {
        if (PMBUS_Verify_PEC(slave, &writebuf, readbuf, readlen - 1) < 0)
        {
            LOG_E("\nVerify Failed for Packet Error Checking\n");
            return -1;
        }
        readlen--; // FTSCHG_MC
    }
    return readlen;
}

/**
 *@fn PMBus_I2CRead_NoPEC
 *@brief This function is invoked to Read Byte/Word on i2c deive using i2c_rw protocal without PEC byte
 *@param i2c_dev - Name of i2c device to perform Read
 *@param slave - Slave Address
 *@param cmdCode - command code of the pmbus device
 *@param read_buf - contains readed data from i2c device
 *@return Returns no of bytes readed on success
 *        Returns -1 on failure
 */
int PMBus_I2CRead_NoPEC(INT8U i2cBus, u8 slave, u8 cmdCode, u8 *buf)
{
    int RetVal, PEC_Enable = 0;

    /* Disable the PEC byte forcefully*/
    PEC_Disable = 1;

    /*Disable the PEC_Verify */
    if (PEC_Verify == 1)
    {
        PEC_Enable = 1;
        PEC_Verify = 0;
    }

    RetVal = PMBus_I2CRead(i2cBus, slave, cmdCode, buf);

    PEC_Disable = 0;

    /*Enable the PEC if it is already enabled*/
    if (PEC_Enable == 1)
    {
        PEC_Verify = 1;
    }

    return RetVal;
}

/**
 *@fn PMBus_I2CWrite
 *@brief This function is invoked to Read Byte/Word on i2c device using i2c_rw protocol
 *@param i2c_dev - Name of i2c device to perform Read
 *@param slave - Slave Address
 *@param cmdCode - command code of the pmbus device
 *@param Write_buf - contains data to write on i2c device
 *@return Returns 0 on success
 *        Returns -1 on failure
 */
int PMBus_I2CWrite(INT8U i2cBus, u8 slave, u8 cmdCode, u8 *Write_buf)
{
    int j, ret;
    int writelen;
    u8 writebuf[50];
    if (PEC_Verify == 0)
    {
        PMBUS_PECEnable(i2cBus, slave);
    }

    const PMBus_t *pPMBusCmd = PMBus_getCmds(cmdCode);
    if (pPMBusCmd == NULL)
    {
        return -1;
    }
    if (((pPMBusCmd->transType != I2C_WRITE) && (pPMBusCmd->transType != I2C_READ_WRITE) && (pPMBusCmd->transType != I2C_SEND_BYTE)) ||
        ((pPMBusCmd->transType == I2C_SEND_BYTE) && (pPMBusCmd->dataBytes != 0)) ||
        ((pPMBusCmd->transType != I2C_SEND_BYTE) && (Write_buf == NULL)))
    {
        LOG_E("\nInvalid Request for this Command 0x%02x\n", pPMBusCmd->cmdName);
        return -1;
    }
    writebuf[0] = pPMBusCmd->cmdName;
    switch (pPMBusCmd->dataBytes)
    {
    case 0:
        writelen = 1;
        break;
    case PMB_BYTE:
        writelen = 2;
        writebuf[1] = Write_buf[0];
        break;
    case PMB_WORD:
        writelen = 3;
        writebuf[1] = Write_buf[0];
        writebuf[2] = Write_buf[1];
        break;
    case PMB_BLOCK:
        writelen = Write_buf[0] + 2; // 2 byte extra - 1 for cmd code and 1 for data length
        if (writelen > sizeof(writebuf))
        {
            return -1;
        }
        for (j = 1; j < writelen; j++)
        {
            writebuf[j] = Write_buf[j - 1];
        }
        break;
    case PMB_RAW:
        writelen = Write_buf[0];
        if (writelen > sizeof(writebuf))
        {
            return -1;
        }
        for (j = 1; j <= writelen; j++)
        {
            writebuf[j] = Write_buf[j];
        }
        writelen += 1; /*Added for Command Code*/
        break;
    default:
        return -1;
    }

    if (PEC_Verify == 1)
    {
        PMBUS_Add_PEC(slave, writebuf, writelen);
        writelen++;
    }
    ret = PMBUS_i2c_master_write(i2cBus, slave, writebuf, writelen);
    return ret;
}

/**
 *@fn PMBus_I2CWrite_NoPEC
 *@brief This function is invoked to Read Byte/Word on i2c device using i2c_rw protocol without PEC byte
 *@param i2c_dev - Name of i2c device to perform Read
 *@param slave - Slave Address
 *@param cmdCode - command code of the pmbus device
 *@param Write_buf - contains data to write on i2c device
 *@return Returns 0 on success
 *        Returns -1 on failure
 */
int PMBus_I2CWrite_NoPEC(INT8U i2cBus, u8 slave, u8 cmdCode, u8 *Write_buf)
{
    int RetVal, PEC_Enable = 0;

    /* Disable the PEC byte forcefully*/
    PEC_Disable = 1;

    /*Disable the PEC_Verify */
    if (PEC_Verify == 1)
    {
        PEC_Enable = 1;
        PEC_Verify = 0;
    }

    RetVal = PMBus_I2CWrite(i2cBus, slave, cmdCode, Write_buf);

    PEC_Disable = 0;

    /*Enable the PEC if it is already enabled*/
    if (PEC_Enable == 1)
    {
        PEC_Verify = 1;
    }

    return RetVal;
}

/**
 *@fn PMBus_SendByte
 *@brief This function is invoked to send a single command byte on i2c device using i2c_rw protocol
 *@param i2c_dev - Name of i2c device to perform Read
 *@param slave - Slave Address
 *@param cmdCode - command code of the pmbus device
 *@return Returns 0 on success
 *        Returns -1 on failure
 */
inline int PMBus_SendByte(INT8U i2cBus, u8 slave, u8 cmdCode)
{
    return PMBus_I2CWrite(i2cBus, slave, cmdCode, NULL);
}

int PMBus_ReadFanSpeed(INT8U i2cBus, u8 slave, int Fan_num, u8 *read_buf)
{
    switch (Fan_num)
    {
    case 1:
        return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_FAN_SPEED_1, read_buf);
    case 2:
        return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_FAN_SPEED_2, read_buf);
    case 3:
        return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_FAN_SPEED_3, read_buf);
    case 4:
        return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_FAN_SPEED_4, read_buf);
    default:
        LOG_E("\n Invalid Fan Number\n");
        return -1;
    }
}
int PMBus_ReadTemp(INT8U i2cBus, u8 slave, int Temp_num, u8 *read_buf)
{
    switch (Temp_num)
    {
    case 1:
        return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_TEMPERATURE_1, read_buf);
    case 2:
        return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_TEMPERATURE_2, read_buf);
    case 3:
        return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_TEMPERATURE_3, read_buf);
    default:
        LOG_E("\n Invalid Temperature Number\n");
        return -1;
    }
}

inline int PMBus_ReadVIN(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_VIN, read_buf);
}

inline int PMBus_ReadIIN(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_IIN, read_buf);
}

inline int PMBus_ReadPIN(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_PIN, read_buf);
}

inline int PMBus_ReadRevision(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_PMBUS_REVISION, read_buf);
}

inline int PMBus_ReadStatusWord(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_STATUS_WORD, read_buf);
}

inline int PMBus_ReadStatusCML(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_STATUS_CML, read_buf);
}

inline int PMBus_ReadStatusOther(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_STATUS_OTHER, read_buf);
}

inline int PMBus_ReadStatusIOUT(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_STATUS_IOUT, read_buf);
}

inline int PMBus_ReadStatusINPUT(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_STATUS_INPUT, read_buf);
}

inline int PMBus_ReadStatusTemp(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_STATUS_TEMPERATURE, read_buf);
}

inline int PMBus_ReadStatusFan1_2(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_STATUS_FANS_1_2, read_buf);
}

inline int PMBus_ReadStatusFan3_4(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_STATUS_FANS_3_4, read_buf);
}

inline int PMBus_SendClearFaults(INT8U i2cBus, u8 slave)
{
    return PMBus_SendByte(i2cBus, slave, PMBUS_CLEAR_FAULTS);
}

inline int PMBus_ReadVOUTMODE(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_VOUT_MODE, read_buf);
}

inline int PMBus_ReadVOUT(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_VOUT, read_buf);
}

inline int PMBus_ReadMFR_IOUT_MAX(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_MFR_IOUT_MAX, read_buf);
}

inline int PMBus_ReadIOUT(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_IOUT, read_buf);
}

inline int PMBus_ReadPOUT(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_POUT, read_buf);
}

inline int PMBus_ReadCapability(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_CAPABILITY, read_buf);
}

inline int PMBus_ReadPIN_OP_WARN_LIMIT(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_PIN_OP_WARN_LIMIT, read_buf);
}

inline int PMBus_WritePIN_OP_WARN_LIMIT(INT8U i2cBus, u8 slave, u8 *write_buf)
{
    return PMBus_I2CWrite(i2cBus, slave, PMBUS_PIN_OP_WARN_LIMIT, write_buf);
}

inline int PMBus_ReadON_OFF_CONFIG(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_ON_OFF_CONFIG, read_buf);
}

inline int PMBus_WriteON_OFF_CONFIG(INT8U i2cBus, u8 slave, u8 *write_buf)
{
    return PMBus_I2CWrite(i2cBus, slave, PMBUS_ON_OFF_CONFIG, write_buf);
}

inline int PMBus_ReadOperation(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_OPERATION, read_buf);
}

inline int PMBus_WriteOperation(INT8U i2cBus, u8 slave, u8 *write_buf)
{
    return PMBus_I2CWrite(i2cBus, slave, PMBUS_OPERATION, write_buf);
}

inline int PMBus_ReadPOUT_MAX(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_POUT_MAX, read_buf);
}

inline int PMBus_WritePOUT_MAX(INT8U i2cBus, u8 slave, u8 *write_buf)
{
    return PMBus_I2CWrite(i2cBus, slave, PMBUS_POUT_MAX, write_buf);
}

inline int PMBus_ReadPOUT_OP_FAULT_LIMIT(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_POUT_OP_FAULT_LIMIT, read_buf);
}

inline int PMBus_WritePOUT_OP_FAULT_LIMIT(INT8U i2cBus, u8 slave, u8 *write_buf)
{
    return PMBus_I2CWrite(i2cBus, slave, PMBUS_POUT_OP_FAULT_LIMIT, write_buf);
}

inline int PMBus_ReadPOUT_OP_FAULT_RESPONSE(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_POUT_OP_FAULT_RESPONSE, read_buf);
}

inline int PMBus_WritePOUT_OP_FAULT_RESPONSE(INT8U i2cBus, u8 slave, u8 *write_buf)
{
    return PMBus_I2CWrite(i2cBus, slave, PMBUS_POUT_OP_FAULT_RESPONSE, write_buf);
}

inline int PMBus_ReadPAGE(INT8U i2cBus, u8 slave, u8 *read_buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_PAGE, read_buf);
}

inline int PMBus_WritePAGE(INT8U i2cBus, u8 slave, u8 *write_buf)
{
    return PMBus_I2CWrite(i2cBus, slave, PMBUS_PAGE, write_buf);
}

inline int PMBus_ReadQUERY(INT8U i2cBus, u8 slave, u8 *buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_QUERY, buf);
}

inline int PMBus_ReadCOEFFICIENTS(INT8U i2cBus, u8 slave, u8 *buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_COEFFICIENTS, buf);
}
inline int PMBus_WriteSMBALERT_MASK(INT8U i2cBus, u8 slave, u8 *buf)
{
    return PMBus_I2CWrite(i2cBus, slave, PMBUS_SMBALERT_MASK, buf);
}

inline int PMBus_ReadREAD_EIN(INT8U i2cBus, u8 slave, u8 *buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_READ_EIN, buf);
}

inline int PMBus_ReadMFR_MAX_TEMP_1(INT8U i2cBus, u8 slave, u8 *buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_MFR_MAX_TEMP_1, buf);
}

inline int PMBus_ReadMFR_MAX_TEMP_2(INT8U i2cBus, u8 slave, u8 *buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_MFR_MAX_TEMP_2, buf);
}

inline int PMBus_WritePAGE_PLUS_WRITE(INT8U i2cBus, u8 slave, u8 *buf)
{
    return PMBus_I2CWrite(i2cBus, slave, PMBUS_PAGE_PLUS_WRITE, buf);
}

inline int PMBus_ReadPAGE_PLUS_READ(INT8U i2cBus, u8 slave, u8 *buf)
{
    return PMBus_I2CRead(i2cBus, slave, PMBUS_PAGE_PLUS_READ, buf);
}
