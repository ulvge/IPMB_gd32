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


#include "tmp/digital_temp_sensor.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "bsp_i2c_gpio.h"
#include "OSPort.h"


static void tmp431a_set_tmp_range(tmp431a_tmp_range_enum range);


bool tmp_init(void)
{
	uint8_t res;
	
	i2cs0_init();
    res = i2cs0_read_bytes(TMEP1_ADDR, 0, NULL, 0);
    if(res == false)
	{
		LOG_E("SD5075 init failed!");
	}
	
    res = i2cs0_read_bytes(TMEP2_ADDR, 0, NULL, 0);
	if(res == false)
	{
		LOG_E("STLM75M2F init failed!");
	}

	res = simulated_i2c2_CheckDevice(TMEP3_ADDR);
	if(res == false)
	{
		LOG_E("TMP431A init failed!");
	}
	tmp431a_set_tmp_range(TMP431A_TMP_RANGE_n64_191);
	
	return res;
}

bool get_tmp_raw_value(uint8_t channel, int16_t* tmp_value)
{
	uint8_t value[2];

	switch(channel)
	{
	case 0:
		i2cs0_read_bytes(TMEP1_ADDR, SD5075_TEMP_RESULT_REG, value, 2);
		break;
	case 1:
		i2cs0_read_bytes(TMEP2_ADDR, STLM75M2F_TEMP_RESULT_REG, value, 2);
		break;	
	case 2:
		simulated_i2c2_read_bytes(TMEP3_ADDR, TMP431A_LOCAL_TEMP_HIGH_READ_REG, &value[0], 1);
		simulated_i2c2_read_bytes(TMEP3_ADDR, TMP431A_LOCAL_TEMP_LOW_BYTE_REG, &value[1], 1);
		break;
	case 3:
		simulated_i2c2_read_bytes(TMEP3_ADDR, TMP431A_REMOTE_TEMP_HIGH_READ_REG, &value[0], 1);
		simulated_i2c2_read_bytes(TMEP3_ADDR, TMP431A_REMOTE_TEMP_LOW_BYTE_REG, &value[1], 1);
		break;
	default:
		return false;
	}

	*tmp_value = (int16_t) (value[0] | (value[1]<<8));
	
	return true;
}

float tmp_value_convert(uint8_t channel, int16_t tmp_raw)
{
	float tmp = 0.0f;

	switch(channel)
	{
	case 0:                     // SD5075
		tmp = (tmp_raw>>4)/16.0;
		break;
	case 1:                     // STLM75M2F
		tmp = tmp_raw/64.0;      
		break;
	case 2:                     // TMP431A
	case 3:
		tmp = ((tmp_raw>>8) & 0xFF) + (tmp_raw & 0xFF)*0.0625F;      
		break;
	default:
		break;
	}
	
	return tmp;
}


bool get_tmp_value(uint8_t channel, uint16_t* tmp)
{
	int16_t raw_value;
	bool res = false;
	
	res = get_tmp_raw_value(channel, &raw_value);
	if(!res){
		return false;
	}
	
	*tmp = tmp_value_convert(channel, raw_value)*100;

	return true;
}


/*****************  TMP431A Funcitons  ****************************/
uint8_t tmp431a_get_id()
{
	uint8_t id;

	simulated_i2c2_read_bytes(TMEP3_ADDR, TMP431A_DEVICE_ID_REG, &id, 1);
	
	return id;
}

uint8_t tmp431a_get_manufacturer()
{
	uint8_t manufacturer;

	simulated_i2c2_read_bytes(TMEP3_ADDR, TMP431A_MANUFACTURER_REG, &manufacturer, 1);
	
	return manufacturer;
}

void tmp431a_set_rate()
{

}

void tmp431a_software_reset()
{
	simulated_i2c2_write_bytes(TMEP3_ADDR, TMP431A_SOFTWARE_RESET_REG, 0, 1);
}

static void tmp431a_set_tmp_range(tmp431a_tmp_range_enum range)
{
	uint8_t config_reg_value; 

	simulated_i2c2_read_bytes(TMEP3_ADDR, TMP431A_CONFIG_READ_REG1, &config_reg_value, 1);
	switch(range)
	{
	case TMP431A_TMP_RANGE_0_127:
		config_reg_value &= TMP431A_CONFIG_TMP_RANGE_0_127;
		break;
	case TMP431A_TMP_RANGE_n64_191:
		config_reg_value |= TMP431A_CONFIG_TMP_RANGE_n64_191;
		break;
	default:
		break;
	}
	simulated_i2c2_write_bytes(TMEP3_ADDR, TMP431A_CONFIG_WRITE_REG1, &config_reg_value, 1);	
}
