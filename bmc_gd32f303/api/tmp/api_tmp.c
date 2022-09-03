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


#include "tmp/api_tmp.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "tmp/hwd1668.h"
#include "OSPort.h"


int8_t g_temperature_raw[4];


void tmpSampleTask(void *arg)
{
	int i = 0;
	int8_t tmp = 0;

	sleep(5);
	if(!tmp_init())
	{
		LOG_E("tmp chip init failed, delete task tmpSampleTask!");
		vTaskDelete(NULL);
		return;
	}

	while(1)
	{
		for(i=0; i<sizeof(g_temperature_raw)/sizeof(int8_t); i++)
		{
			if(!hwd1668_get_tmp_value(i, &tmp))
			{
				LOG_E("channel %d get tmp failed", i);
			}
			else
			{
				g_temperature_raw[i] = tmp;
			}
		}
		msleep(1000);
	}
}

bool tmp_init(void)
{
	if(!hwd1668_init())
	{
		LOG_E("temp chip hwd1668 init failed!");
		return false;
	}
	return true;
}


bool get_tmp_value(uint8_t channel, uint8_t* tmp)
{
	if(channel<4)
	{
		*tmp = g_temperature_raw[channel] + 70;  // -70 --  130 
		return true;
	}

	return false;
}

