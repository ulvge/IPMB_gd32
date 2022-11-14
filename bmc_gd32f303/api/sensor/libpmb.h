/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2010, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
*
* libpmb.h
*
* Private prototypes and defines for libpmb.
*
* Author: Revanth A <revantha@amiindia.co.in>
*
******************************************************************/
#ifndef LIBPMB_H
#define LIBPMB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Types.h"

/**************************** I2C Definitions ********************************/

#define PMB_WORD    2
#define PMB_BYTE    1
#define PMB_BLOCK    3
#define PMB_RAW	    4
#define PMB_BLOCK_PROC_CALL    7

#define I2C_M_RD		0x0001	/* read data, from slave to master */


typedef enum
{
    /** \brief Perform an I2C master write for this operation */
    I2C_WRITE = 0,
    /** \brief Perform an I2C master read for this operation */
    I2C_READ = 1,
    I2C_READ_WRITE = 2,
    I2C_SEND_BYTE = 3,
}PMB_I2C_CMD;

typedef enum
{
    PMBUS_PAGE=0x0,
    PMBUS_OPERATION,
    PMBUS_ON_OFF_CONFIG,
    PMBUS_CLEAR_FAULTS,
    PMBUS_PHASE,
    PMBUS_PAGE_PLUS_WRITE=0x05,
    PMBUS_PAGE_PLUS_READ=0x06,
    PMBUS_WRITE_PROTECT=0x10,
    PMBUS_STORE_DEFAULT_ALL,
    PMBUS_RESTORE_DEFAULT_ALL,
    PMBUS_STORE_DEFAULT_CODE,
    PMBUS_RESTORE_DEFAULT_CODE,
    PMBUS_STORE_USER_ALL,
    PMBUS_RESTORE_USER_ALL,
    PMBUS_STORE_USER_CODE,
    PMBUS_RESTORE_USER_CODE,
    PMBUS_CAPABILITY,
    PMBUS_QUERY,
    PMBUS_SMBALERT_MASK =0x1B,
    PMBUS_VOUT_MODE=0x20,
    PMBUS_VOUT_COMMAND,
    PMBUS_VOUT_TRIM,
    PMBUS_VOUT_CAL_OFFSET,
    PMBUS_VOUT_MAX,
    PMBUS_VOUT_MARGIN_HIGH,
    PMBUS_VOUT_MARGIN_LOW,
    PMBUS_VOUT_TRANSITION_RATE,
    PMBUS_VOUT_DROOP,
    PMBUS_VOUT_SCALE_LOOP,
    PMBUS_VOUT_SCALE_MONITOR,
    PMBUS_COEFFICIENTS=0x30,
    PMBUS_POUT_MAX=0x31,
    PMBUS_MAX_DUTY,
    PMBUS_FREQUENCY_SWITCH,
    PMBUS_VIN_ON=0x35,
    PMBUS_VIN_OFF,
    PMBUS_INTERLEAVE,
    PMBUS_IOUT_CAL_GAIN,
    PMBUS_IOUT_CAL_OFFSET,
    PMBUS_FAN_CONFIG_1_2,
    PMBUS_FAN_COMMAND_1,
    PMBUS_FAN_COMMAND_2,
    PMBUS_FAN_CONFIG_3_4,
    PMBUS_FAN_COMMAND_3,
    PMBUS_FAN_COMMAND_4,
    PMBUS_VOUT_OV_FAULT_LIMIT,
    PMBUS_VOUT_OV_FAULT_RESPONSE,
    PMBUS_VOUT_OV_WARN_LIMIT,
    PMBUS_VOUT_UV_WARN_LIMT,
    PMBUS_VOUT_UV_FAULT_LIMIT,
    PMBUS_VOUT_UV_FAULT_RESPONSE,
    PMBUS_IOUT_OC_FAULT_LIMIT,
    PMBUS_IOUT_OC_FAULT_RESPONSE,
    PMBUS_IOUT_OC_LV_FAULT_LIMIT,
    PMBUS_IOUT_OC_LV_FAULT_RESPONSE,
    PMBUS_IOUT_OC_WARN_LIMIT,
    PMBUS_IOUT_UC_FAULT_LIMIT,
    PMBUS_IOUT_UC_FAULT_RESPONSE,
    PMBUS_OT_FAULT_LIMIT=0x4F,
    PMBUS_OT_FAULT_RESPONSE,
    PMBUS_OT_WARN_LIMIT,
    PMBUS_UT_WARN_LIMIT,
    PMBUS_UT_FAULT_LIMIT,
    PMBUS_UT_FAULT_RESPONSE,
    PMBUS_VIN_OV_FAULT_LIMIT,
    PMBUS_VIN_OV_FAULT_RESPONSE,
    PMBUS_VIN_OV_WARN_LIMIT,
    PMBUS_VIN_UV_WARN_LIMIT,
    PMBUS_VIN_UV_FAULT_LIMIT,
    PMBUS_VIN_UV_FAULT_RESPONSE,
    PMBUS_IIN_OC_FAULT_LIMIT,
    PMBUS_IIN_OC_FAULT_RESPONSE,
    PMBUS_IIN_OC_WARN_LIMIT,
    PMBUS_POWER_GOOD_ON,
    PMBUS_POWER_GOOD_OFF,
    PMBUS_TON_DELAY,
    PMBUS_TON_RISE,
    PMBUS_TON_MAX_FAULT_LIMIT,
    PMBUS_TON_MAX_FAULT_RESPONSE,
    PMBUS_TOFF_DELAY,
    PMBUS_TOFF_FALL,
    PMBUS_TOFF_MAX_WARN_LIMIT,
    PMBUS_POUT_OP_FAULT_LIMIT=0x68,
    PMBUS_POUT_OP_FAULT_RESPONSE,
    PMBUS_POUT_OF_WARN_LIMIT,
    PMBUS_PIN_OP_WARN_LIMIT,
    PMBUS_STATUS_BYTE=0x78,
    PMBUS_STATUS_WORD,
    PMBUS_STATUS_VOUT,
    PMBUS_STATUS_IOUT,
    PMBUS_STATUS_INPUT,
    PMBUS_STATUS_TEMPERATURE,
    PMBUS_STATUS_CML,
    PMBUS_STATUS_OTHER,
    PMBUS_STATUS_MFR_SPECIFIC,
    PMBUS_STATUS_FANS_1_2,
    PMBUS_STATUS_FANS_3_4,
    PMBUS_READ_EIN=0x86,
    PMBUS_READ_VIN=0x88,
    PMBUS_READ_IIN=0x89,
    PMBUS_READ_VCAP=0x8a,
    PMBUS_READ_VOUT=0x8b,
    PMBUS_READ_IOUT=0x8c,
    PMBUS_READ_TEMPERATURE_1,
    PMBUS_READ_TEMPERATURE_2,
    PMBUS_READ_TEMPERATURE_3,
    PMBUS_READ_FAN_SPEED_1,
    PMBUS_READ_FAN_SPEED_2,
    PMBUS_READ_FAN_SPEED_3,
    PMBUS_READ_FAN_SPEED_4,
    PMBUS_READ_DUTY_CYCLE,
    PMBUS_READ_FREQUENCY,
    PMBUS_READ_POUT,
    PMBUS_READ_PIN,
    PMBUS_PMBUS_REVISION,
    PMBUS_MFR_ID,
    PMBUS_MFR_MODEL,
    PMBUS_MFR_REVISION,
    PMBUS_MFR_LOCATION,
    PMBUS_MFR_DATE,
    PMBUS_MRF_SERIAL,
    PMBUS_MFR_VIN_MIN=0xA0,
    PMBUS_MFR_VIN_MAX,
    PMBUS_MFR_IIN_MAX,
    PMBUS_MFR_PIN_MAX,
    PMBUS_MFR_VOUT_MIN,
    PMBUS_MFR_VOUT_MAX,
    PMBUS_MFR_IOUT_MAX,
    PMBUS_MFR_POUT_MAX,
    PMBUS_MFR_TAMBIENT_MAX,
    PMBUS_MFR_TAMBIENT_MIN,
    PMBUS_MFR_EFFICIENCY_LL,
    PMBUS_MFR_EFFICIENCY_HL,
    PMBUS_USER_DATA_00,
    PMBUS_USER_DATA_01,
    PMBUS_USER_DATA_02,
    PMBUS_USER_DATA_03,
    PMBUS_USER_DATA_04,
    PMBUS_USER_DATA_05,
    PMBUS_USER_DATA_06,
    PMBUS_USER_DATA_07,
    PMBUS_USER_DATA_08,
    PMBUS_USER_DATA_09,
    PMBUS_USER_DATA_10,
    PMBUS_USER_DATA_11,
    PMBUS_USER_DATA_12,
    PMBUS_USER_DATA_13,
    PMBUS_USER_DATA_14,
    PMBUS_USER_DATA_15,
    PMBUS_MFR_MAX_TEMP_1=0xc0,
    PMBUS_MFR_MAX_TEMP_2,
}PMB_CMD;


typedef struct
{
    u8 cmdName;
    u8 transType;
    u8 dataBytes;
}PMBus_t;


#define MAX_PMBUS_CMD_IDX	0xFF


/************************* Function Prototypes *******************************/
extern int PMBus_I2CRead( INT8U i2cBus, u8 slave, u8 cmdCode,u8 *read_buf);

extern int PMBus_I2CRead_NoPEC( INT8U i2cBus, u8 slave, u8 cmdCode,u8 *buf);

extern int PMBus_I2CWrite( INT8U i2cBus, u8 slave, u8 cmdCode,u8 *Write_buf);

extern int PMBus_I2CWrite_NoPEC( INT8U i2cBus, u8 slave, u8 cmdCode,u8 *Write_buf);

extern inline int PMBus_SendByte(INT8U i2cBus, u8 slave, u8 cmdCode);

extern int PMBUS_Add_PEC(u8 slave,u8 *writebuf,int writelen);

extern void PMBUS_PECEnable(INT8U i2cBus, u8 slave);

extern unsigned char PMBUS_Crc8( unsigned char inCrc, unsigned char inData );

extern int PMBUS_Verify_PEC(u8 slave,u8 *writebuf,u8* readbuf, int readlen);

extern inline int PMBus_ReadVIN(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadIIN(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadPIN(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadRevision(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadStatusWord(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadStatusCML(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadStatusOther(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadStatusIOUT(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadStatusINPUT(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadStatusTemp(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadStatusFan1_2(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadStatusFan3_4(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_SendClearFaults(INT8U i2cBus, u8 slave);

extern inline int PMBus_ReadVOUTMODE(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadVOUT(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadMFR_IOUT_MAX(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadIOUT(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadPOUT(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadCapability(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_ReadPIN_OP_WARN_LIMIT(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_WritePIN_OP_WARN_LIMIT(INT8U i2cBus, u8 slave,u8 *write_buf);

extern inline int PMBus_ReadON_OFF_CONFIG(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_WriteON_OFF_CONFIG(INT8U i2cBus, u8 slave,u8 *write_buf);

extern inline int PMBus_ReadOperation(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_WriteOperation(INT8U i2cBus, u8 slave,u8 *write_buf);

extern inline int PMBus_ReadPOUT_MAX(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_WritePOUT_MAX(INT8U i2cBus, u8 slave,u8 *write_buf);

extern inline int PMBus_ReadPOUT_OP_FAULT_LIMIT(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_WritePOUT_OP_FAULT_LIMIT(INT8U i2cBus, u8 slave,u8 
*write_buf);

extern inline int PMBus_ReadPOUT_OP_FAULT_RESPONSE(INT8U i2cBus, u8 slave,u8 
*read_buf);

extern inline int PMBus_WritePOUT_OP_FAULT_RESPONSE(INT8U i2cBus, u8 slave,u8 
*write_buf);

extern inline int PMBus_ReadPAGE(INT8U i2cBus, u8 slave,u8 *read_buf);

extern inline int PMBus_WritePAGE(INT8U i2cBus, u8 slave,u8 *write_buf);

extern inline int PMBus_ReadQUERY(INT8U i2cBus, u8 slave,u8 *buf);

extern inline int PMBus_ReadCOEFFICIENTS(INT8U i2cBus, u8 slave,u8 *buf);

extern inline int PMBus_ReadTemp(INT8U i2cBus, u8 slave,int Temp_num,u8 *read_buf);

extern inline int PMBus_ReadFanSpeed(INT8U i2cBus, u8 slave,int Fan_num,u8 *read_buf);

extern inline int PMBus_WriteSMBALERT_MASK(INT8U i2cBus, u8 slave,u8 *buf);

extern inline int PMBus_ReadREAD_EIN(INT8U i2cBus, u8 slave,u8 *buf);

extern inline int PMBus_ReadMFR_MAX_TEMP_1(INT8U i2cBus, u8 slave,u8 *buf);

extern inline int PMBus_ReadMFR_MAX_TEMP_2(INT8U i2cBus, u8 slave,u8 *buf);

extern inline int PMBus_WritePAGE_PLUS_WRITE(INT8U i2cBus, u8 slave,u8 *buf);

extern inline int PMBus_ReadPAGE_PLUS_READ(INT8U i2cBus, u8 slave,u8 *buf);

#ifdef __cplusplus
}
#endif

#endif    //LIBPMB_H
