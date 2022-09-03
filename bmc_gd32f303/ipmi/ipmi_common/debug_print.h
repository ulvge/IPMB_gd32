/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2005-2006, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************
 ****************************************************************
 *
 * types.h
 * Standard Type definitions
 *
 * Author: Govind Kothandapani <govindk@ami.com>
 *
 *****************************************************************/
#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include "bsp_usart1.h"
#include "net_print/net_print.h"
#include "main.h"

#define IPMI_INFO(STR, Args...)                      LOG_I(STR, ##Args)  
#define IPMI_WARNING(STR, Args...)                   LOG_W(STR, ##Args)  
#define IPMI_ERROR(STR, Args... )                    LOG_E(STR, ##Args)   


#define IPMI_DBG_PRINT(Args...)    LOG_I(Args)
#define IPMI_DBG_PRINT_1(Args...)  LOG_I(Args)
#define IPMI_DBG_PRINT_2(Args...)  LOG_I(Args)

#define TDBG(Args...)  printf(Args)

#define TINFO        TDBG
#define TWARN        TDBG
#define TCRIT        TDBG
#define TEMERG       TDBG
#define TEXTLOG      TDBG

#ifdef NET_LOG_ENABLE
    #define rt_kprintf             net_raw_print
    #define rt_hdr_end_kprintf     net_hdr_end_print
#else
    #define rt_kprintf(Args...)  printf(Args)
#endif

#define DBG_ENABLE
/* DEBUG level */
#define DBG_ERROR           0
#define DBG_WARNING         1
#define DBG_INFO            2
#define DBG_LOG             3

#ifndef DBG_SECTION_NAME
#define DBG_SECTION_NAME    "DBG"
#endif

#ifdef DBG_ENABLE

#ifndef DBG_LEVEL
#define DBG_LEVEL         DBG_INFO
#endif

#define _DBG_COLOR(n)
#define _DBG_LOG_HDR(lvl_name)                    \
    rt_kprintf("["lvl_name"/"DBG_SECTION_NAME"] ")
#define _DBG_LOG_X_END                                     \
    rt_kprintf("\r\n")


/*
 * static debug routine
 * NOTE: This is a NOT RECOMMENDED API. Please using LOG_X API.
 *       It will be DISCARDED later. Because it will take up more resources.
 */
#define dbg_log(level, fmt, ...)                            \
    if ((level) <= DBG_LEVEL)                               \
    {                                                       \
        switch(level)                                       \
        {                                                   \
            case DBG_ERROR:   _DBG_LOG_HDR("E"); break; \
            case DBG_WARNING: _DBG_LOG_HDR("W"); break; \
            case DBG_INFO:    _DBG_LOG_HDR("I"); break; \
            case DBG_LOG:     _DBG_LOG_HDR("D"); break;  \
            default: break;                                 \
        }                                                   \
        rt_kprintf(fmt, ##__VA_ARGS__);                     \
    }

#define dbg_here                                            \
    if ((DBG_LEVEL) <= DBG_LOG){                            \
        rt_kprintf(DBG_SECTION_NAME " Here %s:%d\n",        \
            __FUNCTION__, __LINE__);                        \
    }

#define dbg_enter                                           \
    if ((DBG_LEVEL) <= DBG_LOG){                            \
        rt_kprintf(DBG_SECTION_NAME " Enter %s\n",          \
            __FUNCTION__);                                  \
    }

#define dbg_exit                                            \
    if ((DBG_LEVEL) <= DBG_LOG){                            \
        rt_kprintf(DBG_SECTION_NAME " Exit  %s:%d\n",       \
            __FUNCTION__);                                  \
    }

#ifdef NET_LOG_ENABLE
#define dbg_log_line(lvl, fmt, ...)                \
    do                                                      \
    {                                                       \
        rt_hdr_end_kprintf("["lvl"/"DBG_SECTION_NAME"] ", "\r\n", fmt, ##__VA_ARGS__);    \
    }                                                       \
    while (0)
#else
#define dbg_log_line(lvl, fmt, ...)                \
    do                                                      \
    {                                                       \
        _DBG_LOG_HDR(lvl);                         \
        rt_kprintf(fmt, ##__VA_ARGS__);                     \
        _DBG_LOG_X_END;                                     \
    }                                                       \
    while (0)
#endif


#define dbg_raw(...)         rt_kprintf(__VA_ARGS__);

#else
#define dbg_log(level, fmt, ...)
#define dbg_here
#define dbg_enter
#define dbg_exit
#define dbg_log_line(lvl, fmt, ...)
#define dbg_raw(...)
#endif /* DBG_ENABLE */

#if (DBG_LEVEL >= DBG_LOG)
#define LOG_D(fmt, ...)      dbg_log_line("D", fmt, ##__VA_ARGS__)
#else
#define LOG_D(...)
#endif

#if (DBG_LEVEL >= DBG_INFO)
#define LOG_I(fmt, ...)      dbg_log_line("I", fmt, ##__VA_ARGS__)
#else
#define LOG_I(...)
#endif

#if (DBG_LEVEL >= DBG_WARNING)
#define LOG_W(fmt, ...)      dbg_log_line("W", fmt, ##__VA_ARGS__)
#else
#define LOG_W(...)
#endif

#if (DBG_LEVEL >= DBG_ERROR)
#define LOG_E(fmt, ...)      dbg_log_line("E", fmt, ##__VA_ARGS__)
#else
#define LOG_E(...)
#endif

#define LOG_RAW(...)         dbg_raw(__VA_ARGS__)

#endif	/* DEBUG_PRINT_H */
