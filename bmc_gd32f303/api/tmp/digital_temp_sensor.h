#ifndef __API_TMP_H
#define __API_TMP_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/* address definitions */
#define TMEP1_ADDR  0x48    // SD5075     A2A1A0: 000
#define TMEP2_ADDR  0x4C    // STLM75M2F  1001A2A1A0   A2A1A0: 100
#define TMEP3_ADDR  0x4C    // TMP431A    1001100  

/* registers definitions */
#define SD5075_TEMP_RESULT_REG         0x00
#define SD5075_CONFIG_REG              0x01
#define SD5075_HYSTERESIS_REG          0x02
#define SD5075_OVER_TEMP_REG           0x03
#define SD5075_SINGLE_MEASURE_REG      0x04

/* registers definitions */
#define STLM75M2F_TEMP_RESULT_REG      0x00
#define STLM75M2F_CONFIG_REG           0x01
#define STLM75M2F_HYSTERESIS_REG       0x02
#define STLM75M2F_OVER_TEMP_REG        0x03

/* registers definitions */
#define TMP431A_LOCAL_TEMP_HIGH_READ_REG              0x00
#define TMP431A_REMOTE_TEMP_HIGH_READ_REG             0x01
#define TMP431A_STATUS_READ_REG                       0x02
#define TMP431A_CONFIG_READ_REG1                      0x03
#define TMP431A_CONFIG_WRITE_REG1                     0x09
#define TMP431A_CONVERSION_RATE_READ_REG              0x04
#define TMP431A_CONVERSION_RATE_WRITE_REG             0x0A
#define TMP431A_LOCAL_TEMP_HIGH_LIMIT_HIGH_READ_REG   0x05
#define TMP431A_LOCAL_TEMP_HIGH_LIMIT_HIGH_WRITE_REG  0x0B
#define TMP431A_LOCAL_TEMP_LOW_LIMIT_HIGH_READ_REG    0x06
#define TMP431A_LOCAL_TEMP_LOW_LIMIT_HIGH_WRITE_REG   0x0C
#define TMP431A_REMOTE_TEMP_HIGH_LIMIT_HIGH_READ_REG  0x07
#define TMP431A_REMOTE_TEMP_HIGH_LIMIT_HIGH_WRITE_REG 0x0D
#define TMP431A_REMOTE_TEMP_LOW_LIMIT_HIGH_READ_REG   0x08
#define TMP431A_REMOTE_TEMP_LOW_LIMIT_HIGH_WRITE_REG  0x0E

#define TMP431A_ONESHORT_START_REG                    0x0F

#define TMP431A_REMOTE_TEMP_LOW_BYTE_REG               0x10
#define TMP431A_REMOTE_TEMP_HIGH_LIMIT_LOW_BYTE_REG    0x13
#define TMP431A_REMOTE_TEMP_LOW_LIMIT_LOW_BYTE_REG     0x14
#define TMP431A_LOCAL_TEMP_LOW_BYTE_REG                0x15
#define TMP431A_LOCAL_TEMP_HIGH_LIMIT_LOW_BYTE_REG     0x16
#define TMP431A_LOCAL_TEMP_LOW_LIMIT_LOW_BYTE_REG      0x17
#define TMP431A_N_FACTOR_CORRECTION_REG                0x18
#define TMP431A_REMOTE_THERM_LIMIT_REG                 0x19
#define TMP431A_CONFIG_REG2                            0x1A
#define TMP431A_CHANNEL_MASK_REG                       0x1F
#define TMP431A_LOCAL_THERM_LIMIT_REG                  0x20
#define TMP431A_THERM_HYSTERESIS_REG                   0x21
#define TMP431A_CONSECUTIVE_ALERT_REG                  0x22
#define TMP431A_BETA_RANGE_REG                         0x25
#define TMP431A_SOFTWARE_RESET_REG                     0xFC
#define TMP431A_DEVICE_ID_REG                          0xFD
#define TMP431A_MANUFACTURER_REG                       0xFE


/* TMP431A temp range */
typedef enum
{
    /* flags in STAT0 register */
    TMP431A_TMP_RANGE_0_127 = 0,  
    TMP431A_TMP_RANGE_n64_191              
}tmp431a_tmp_range_enum;

/* bits definitions */
/* TMP431A Configuration 1 */
#define TMP431A_CONFIG_TMP_RANGE_0_127      ((uint8_t)0x00U)          /*!<  */
#define TMP431A_CONFIG_TMP_RANGE_n64_191    ((uint8_t)0x04U)          /*!<  */


/* function declarations */
// bool    tmp_init			    (void);
// bool    get_tmp_raw_value		(uint8_t channel, int16_t* tmp_value);
// float   tmp_value_convert       (uint8_t channel, int16_t tmp_raw);
// bool    get_tmp_value           (uint8_t channel, uint16_t* tmp);

#endif
