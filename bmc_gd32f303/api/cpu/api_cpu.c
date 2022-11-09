#include "cpu/api_cpu.h"
#include "OSPort.h"
#include "Message.h"
#include "ipmi_common.h"
#include "utc/api_utc.h"


uint16_t GetBmcFirmwareVersion(char* str)
{
	char buff[20] = {0};
	uint16_t version = 0xffff;

	int str_len = strlen(str);

	if (str_len > sizeof(buff))
	{
		return version;
	}
	
	memcpy(buff, str, str_len);
	version = atoi(buff);

	return version;	
}
extern uint32_t g_time_run;
uint32_t  GetBmcRunTime(void)
{
  	return g_time_run;	
}

