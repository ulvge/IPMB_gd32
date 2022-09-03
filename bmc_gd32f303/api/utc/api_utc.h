#ifndef __API_UTC_H
#define	__API_UTC_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include <stdint.h>

enum {
    SECS_PER_DAY = 86400,
    MSECS_PER_DAY = 86400000,
    SECS_PER_HOUR = 3600,
    MSECS_PER_HOUR = 3600000,
    SECS_PER_MIN = 60,
    MSECS_PER_MIN = 60000,
    TIME_T_MAX = 2145916799,  // int maximum 2037-12-31T23:59:59 UTC
    JULIAN_DAY_FOR_EPOCH = 2440588 // result of julianDayFromDate(1970, 1, 1)
};



typedef struct {
	
	uint8_t  wHour;
	uint8_t  wMinute;
	uint8_t  wSecond;
	
	uint16_t wYear;
	uint8_t  wMonth;
	uint8_t  wDay;

} SYSTEMTIME_T;

int split(char* src, const char* sp, char** dest);
uint64_t currentSecsSinceEpoch(char* date, char* time);

#ifdef __cplusplus
}
#endif

#endif /* __API_UTC_H */

