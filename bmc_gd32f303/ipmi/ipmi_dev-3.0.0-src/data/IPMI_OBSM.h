/****************************************************************
 **                                                            **
 **    (C)Copyright 2007-2008, American Megatrends Inc.        **
 **                                                            **
 **                   All Rights Reserved.                     **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200, Norcross,        **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************/

 /****************************************************************
 * @file	IPMI_OBSM.h
 * @author	Velu	<velmuruganv@amiindia.co.in>
 * @brief
 ****************************************************************/

#ifndef IPMI_OBSM_H
#define IPMI_OBSM_H

/*** Definitions and Macros ***/
#define CMD_OBSM_GET_OPEN_BLADE_PROPS		0x00
#define CMD_OBSM_GET_ADDR_INFO				0x01
#define CMD_OBSM_PLATFORM_EVT_MSG			0x02
#define CMD_OBSM_MGD_MOD_BMI_CTRL			0x03
#define CMD_OBSM_MGD_MOD_PAYLD_CTRL			0x04
#define CMD_OBSM_SET_SYS_EVNT_LOG_POLICY	0x05
#define CMD_OBSM_SET_MOD_ACTVN_POLICY		0x0A
#define CMD_OBSM_GET_MOD_ACTVN_POLICY		0x0B
#define CMD_OBSM_SET_MOD_ACTVN				0x0C
#define CMD_OBSM_SET_POWER_LEVEL			0x11
#define CMD_OBSM_GET_POWER_LEVEL			0x12
#define CMD_OBSM_RENOG_POWER				0x13

#define CMD_OBSM_GET_SERVICE_INFO			0x16
#define CMD_OBSM_GET_APPLET_PACKAGE_URI		0x17
#define CMD_OBSM_GET_SERVICE_ENABLE_STATE	0x18
#define CMD_OBSM_SET_SERVICE_ENABLE_STATE	0x19
#define CMD_OBSM_SET_SERVICE_TICKET			0x20
#define CMD_OBSM_STOP_SERVICE_SESSION		0x21

#endif  /* IPMI_OBSM_H */

