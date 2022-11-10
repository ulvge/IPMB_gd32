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

/* DEBUG level */
#define DBG_ERROR           0
#define DBG_WARNING         1
#define DBG_INFO            2
#define DBG_LOG             3

#ifndef DBG_SECTION_NAME
#define DBG_SECTION_NAME    "DBG"
#endif

#define DBG_LEVEL         g_debugLevel

#define _DBG_LOG_HDR(lvl_name)                    \
    rt_kprintf("["lvl_name"/"DBG_SECTION_NAME"] ")


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


#define LOG_E(fmt, ...)      dbg_log(DBG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...)      dbg_log(DBG_WARNING, fmt, ##__VA_ARGS__)
#define LOG_I(fmt, ...)      dbg_log(DBG_INFO, fmt, ##__VA_ARGS__)
#define LOG_D(fmt, ...)      dbg_log(DBG_LOG, fmt, ##__VA_ARGS__)


#define LOG_RAW(...)         rt_kprintf(__VA_ARGS__)

#endif	/* DEBUG_PRINT_H */
