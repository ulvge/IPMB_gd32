/**
 * @file tools.h
 * @author
 * @brief
 * 
 * @copyright 
 * 
 */

#ifndef     __TOOLS_H__
#define     __TOOLS_H__

#include "shell_cfg.h"

//#define READ_MODE       ( 0 )
//#define WRITE_MODE      ( 1 )
//#define RW_MODE         ( 2 )
#define RESET_MODE      ( 3 )
#define SCAN_MODE       ( 4 )
#define SETH_MODE       ( 5 )
#define GETH_MODE       ( 6 )
//#define MM_MODE			( 7 )
//#define GET_REC_INFO	( 8 )
//#define SET_REC_INFO	( 9 )

#define  ETH_MAX_BYTES     (16*3)
#define  Max_SALVES        (127)

static int parameterChecked(int para1, int para2, int para3, int para4);
#endif

