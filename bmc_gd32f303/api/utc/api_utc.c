#include "utc/api_utc.h"
#include <string.h>
#include "debug_print.h"

// #define Q_INT64_C(c) c ## i64

static void GetSystemTime(SYSTEMTIME_T *st, char *date, char *time);
static __inline uint64_t julianDayFromDate(int year, int month, int day);
static __inline int floordiv(int a, int b);
static uint8_t get_month(char *month);

uint64_t currentSecsSinceEpoch(char *date, char *time)
{
	SYSTEMTIME_T st;
	memset(&st, 0, sizeof(SYSTEMTIME_T));
	GetSystemTime(&st, date, time);

	return st.wHour * SECS_PER_HOUR + st.wMinute * SECS_PER_MIN + st.wSecond +
		   (julianDayFromDate(st.wYear, st.wMonth, st.wDay) - julianDayFromDate(1970, 1, 1)) * 86400 - 8 * SECS_PER_HOUR; // Q_INT64_C(86400);
}

void GetSystemTime(SYSTEMTIME_T *st, char *date, char *time)
{
	char *p[10];
	int num = 0;
	char buff[50];

	int date_str_len = strlen(date);
	int time_str_len = strlen(time);

	if (date_str_len > sizeof(buff) || time_str_len > sizeof(buff))
	{
		return;
	}

	memcpy(buff, date, date_str_len);
	num = split(buff, " ", p);
	if (num == 3)
	{
		st->wYear = atoi(p[2]);
		st->wMonth = get_month(p[0]);
		st->wDay = atoi(p[1]);
	}
	else
	{
		LOG_E("date str error!");
		return;
	}

	memcpy(buff, time, time_str_len);
	num = split(buff, ":", p);
	if (num == 3)
	{
		st->wHour = atoi(p[0]);
		st->wMinute = atoi(p[1]);
		st->wSecond = atoi(p[2]);
	}
	else
	{
		LOG_E("time str error!");
		return;
	}
}

static __inline int floordiv(int a, int b)
{
	return (a - (a < 0 ? b - 1 : 0)) / b;
}

static __inline uint64_t julianDayFromDate(int year, int month, int day)
{
	int a;
	int m;
	uint64_t y;

	// Adjust for no year 0
	if (year < 0)
		++year;

	/*
 * Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
 * This formula is correct for all julian days, when using mathematical integer
 * division (round to negative infinity), not c++11 integer division (round to zero)
 */
	a = floordiv(14 - month, 12);
	y = (uint64_t)year + 4800 - a;
	m = month + 12 * a - 3;
	return day + floordiv(153 * m + 2, 5) + 365 * y + floordiv(y, 4) - floordiv(y, 100) + floordiv(y, 400) - 32045;
}

int split(char *src, const char *sp, char **dest)
{
	char *p_next = NULL;
	int count = 0;

	if (src == NULL || strlen(src) == 0)
	{
		return 0;
	}

	if (sp == NULL || strlen(sp) == 0)
	{
		return 0;
	}

	p_next = strtok(src, sp);
	while (p_next != NULL)
	{
		*dest++ = p_next;
		++count;
		p_next = strtok(NULL, sp);
	}

	return count;
}

static uint8_t get_month(char *month)
{
	int i = 0;
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	for (i = 0; i < sizeof(months)/sizeof(months[0]); i++)
	{
		if(strcmp(month, months[i]) == 0)
		{
			return i+1;
		}
	}
	LOG_E("month string error!");
	return 0;
}
