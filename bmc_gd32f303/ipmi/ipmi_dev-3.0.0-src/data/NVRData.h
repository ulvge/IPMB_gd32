/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2005-2006, American Megatrends Inc.        **
 ***                                                            **
 ***            All Rights Reserved.                            **
 ***                                                            **
 ***        6145-F, Northbelt Parkway, Norcross,                **
 ***                                                            **
 ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 ***                                                            **
 *****************************************************************
 ******************************************************************
 * 
 * nvrdata.h
 * NVRAM Data
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 *
 ******************************************************************/
#ifndef NVR_DATA_H
#define NVR_DATA_H

#include "PMConfig.h"

/**
 * NVRAM Handles
**/
#define NVRH_USERCONFIG 0
#define NVRH_CHCONFIG     0
#define NVRH_SDR		0


#if 0
/*** Extern Definitions ***/
extern const ChcfgInfo_T g_IPMBChcfg;
extern const ChcfgInfo_T g_SysChcfg;
extern const ChcfgInfo_T g_LanChcfg1;
extern const ChcfgInfo_T g_LanChcfg2;
extern const ChcfgInfo_T g_LanChcfg3;
extern const ChcfgInfo_T g_SerialChcfg;
extern const ChcfgInfo_T g_IcmbChcfg;
extern const ChcfgInfo_T g_SMBChcfg;
extern const ChcfgInfo_T g_SMMChcfg;
extern const ChcfgInfo_T g_SmlinkipmbChcfg;
#endif

#endif	/* NVR_DATA_H */


