/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2020-2020,Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **                   **
 **                                                            **
 **               **
 **                                                            **
 ****************************************************************
 */

/******************************************************************
*
*
*
*
******************************************************************/


#include "tmp/hwd1668.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "bsp_i2c.h"
#include "OSPort.h"

static uint8_t hwd1668_get_device_id(void);


bool hwd1668_init(void)
{
	if(hwd1668_get_device_id() != 0x03)
	{
		return false;
	}
	
	return true;
}

bool hwd1668_get_tmp_value(uint8_t channel, int8_t* tmp)
{
	bool res = false;
	uint8_t tmp_value; 
	uint32_t read_len;
	if(channel > 4)
	{
		LOG_E("temp channel %d out of range! max up to 4...", channel);
		return false;
	}
	//res = tmp_i2c_read(HWD1668_ADDR, channel, &tmp_value, &read_len);
	*tmp = (int8_t)tmp_value;

	return res;
}

static uint8_t hwd1668_get_device_id()
{
	uint8_t id; 
	uint32_t read_len;

	//tmp_i2c_read(HWD1668_ADDR, HWD1668_DEVICE_ID_REG, &id, &read_len);

	return id;
}

