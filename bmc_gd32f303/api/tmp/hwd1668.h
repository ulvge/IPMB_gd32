#ifndef __HWD1688_H
#define __HWD1688_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "main.h"

/* address definitions */
#define HWD1668_ADDR  0x30

/* registers definitions */
#define HWD1668_LOCAL_TEMP_REG         0x00
#define HWD1668_REMOTE_TEMP1_REG       0x01
#define HWD1668_REMOTE_TEMP2_REG       0x02
#define HWD1668_REMOTE_TEMP3_REG       0x03
#define HWD1668_REMOTE_TEMP4_REG       0x04

#define HWD1668_DEVICE_ID_REG          0xFF

/* registers definitions */
#ifdef USE_BMC_BOARD
    #define  tmp_i2c_read(dev, addr, dat, len)    i2c0_bytes_read(dev, addr, dat, len)  
#else
    #define  tmp_i2c_read(dev, addr, dat, len)    i2c1_bytes_read(dev, addr, dat, len) 
#endif

/* funcitons declare */
bool hwd1668_init(void);
bool hwd1668_get_tmp_value(uint8_t channel, int8_t* tmp);

#endif
