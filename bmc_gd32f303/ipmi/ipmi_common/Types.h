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
#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

/*----------------------------------------------
 * Include the platform specific types here.
 *----------------------------------------------*/
//#include "coreTypes.h"
//#include "icc_what.h"

/*----------------------------------------------
 * Bit fields
 *----------------------------------------------*/
#define BIT0    0x0001
#define BIT1	0x0002
#define BIT2	0x0004
#define BIT3	0x0008
#define BIT4	0x0010
#define BIT5	0x0020
#define BIT6	0x0040
#define BIT7	0x0080
#define BIT8	0x0100
#define BIT9	0x0200
#define BIT10   0x0400
#define BIT11   0x0800
#define BIT12   0x1000
#define BIT13   0x2000
#define BIT14   0x4000
#define BIT15   0x8000

/*----------------------------------------------
 * Processor specfic type definetion
 *----------------------------------------------*/
 typedef unsigned char  INT8U;
 typedef unsigned short INT16U;
 typedef unsigned int   INT32U;
 typedef unsigned long  INT64U;

 typedef char  INT8S;
 typedef short INT16S;
 typedef int  INT32S;
 typedef long INT64S;

/*-----------------------------------------------
 * Other Types
 *-----------------------------------------------*/
typedef unsigned int		INTU;
typedef bool   BOOL ;

					   
#ifndef FALSE
#define FALSE 		0
#endif           
#ifndef TRUE
#define TRUE		1
#endif

#ifndef false
#define false 		0
#endif
#ifndef true
#define true		1
#endif
		   
#define  KEY_SPACE                      0x20
#define  KEY_CR                        	0x0D
#define  KEY_LF                        	0x0A
#define  KEY_DEL                   		0x08      //Del!
typedef unsigned char  BOOLEAN;    
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#define uint8 INT8U
#define uint16 INT16U
#define uint32 INT32U
#define uint64 INT64U

//#define int8 INT8S
#define int16 INT16S
//#define int32 INT32S
//#define int64 INT64S

#define UINT8 INT8U
#define UINT16 INT16U
#define UINT32 INT32U

#define u8 INT8U
#define u16 INT16U
#define u32 INT32U


#define ARRARY_SIZE(str)    (sizeof(str) / sizeof(str[0]))
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif
		 
//#define PACKED __attribute__ ((packed))
#define PACKED


#endif	/* TYPES_H */
