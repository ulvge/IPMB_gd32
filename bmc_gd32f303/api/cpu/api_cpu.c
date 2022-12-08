#include "cpu/api_cpu.h"
#include "Message.h"
#include "OSPort.h"
#include "ipmi_common.h"
#include "utc/api_utc.h"

uint16_t GetBmcFirmwareVersion(char *str)
{
    char buff[20] = {0};
    uint16_t version = 0xffff;
    uint8_t val;
    int str_len = strlen(str);

    if (str_len > sizeof(buff)) {
        return version;
    }

    memcpy(buff, str, str_len);
    version = atoi(buff);
    while (*str != NULL) {
        val = *str;
        if (val < '0' || val > '9') {
            *str++;
            version = (version << 8) | atoi(str);
            break;
        }
        *str++;
    }
    return version;
}
extern uint32_t g_time_run;
uint32_t GetBmcRunTime(void)
{
    return g_time_run;
}
