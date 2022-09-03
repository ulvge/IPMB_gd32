#ifndef __BSP_CAPTURE_H
#define __BSP_CAPTURE_H

#include "gd32f20x.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct{
	bool     is_start;
	uint16_t cap_value_first;
	uint16_t cap_value_second;
	uint16_t period_cnt;
	uint16_t cap_no_update_cnt;
	bool     is_valid;

	uint32_t total_value;

} CapPreiodCapInfo_T;


// Cap1  PC

void       capture_init        (void);
bool       capture_get_value   (unsigned char channel, uint32_t* cap_value);

#endif
