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
 * @file	OBSMPort.h
 * @author	Velu	<velmuruganv@amiindia.co.in>
 * @brief
 ****************************************************************/

#ifndef OBSMPORT_H
#define OBSMPORT_H

/*** Definitions and Macros ***/
#define MAX_CHASSIS_MNGR_SUPPORTED              1
#define MAX_SLOT_NUM_SUPPORTED                  32
#define MAX_SLOT_ID_SUPPORTED                   32
#define MAX_BLADE_SUPPORTED                     10
#define MAX_SWITCH_SUPPORTED                    4
#define MAX_COOLING_CTLR_SUPPORTED              1
#define MAX_COOLING_DOMAIN_SUPPORTED            1
#define MAX_FAN_SUPPORTED_PER_DOMAIN            12
#define MAX_COOLING_ZONE_SUPPORTED_PER_DOMAIN   4
#define MAX_PWR_DOMAIN_SUPPORTED                1
#define MAX_PWR_SUPPLY_SUPPORTED_PER_DOMAIN     4
#define MAX_CHAN_NUM_SUPPORTED                  4

/* Slot Numbers (syc with Open blade slot map record) */
#define CHASSIS_MNGR_SLOT_NUM   1
#define SWITCH_SLOT_NUM         2
#define PWR_CTLR_SLOT_NUM       3
#define COOLING_CTLR_SLOT_NUM   4
#define BLADE1_SLOT_NUM         5
#define BLADE2_SLOT_NUM         6
#define BLADE3_SLOT_NUM         7
#define BLADE4_SLOT_NUM         8
#define BLADE5_SLOT_NUM         9
#define BLADE6_SLOT_NUM         10
#define BLADE7_SLOT_NUM         11
#define BLADE8_SLOT_NUM         12
#define BLADE9_SLOT_NUM         13
#define BLADE10_SLOT_NUM        14

#define PS1_SLOT_NUM            15
#define PS2_SLOT_NUM            16

#define FAN1_SLOT_NUM           17
#define FAN2_SLOT_NUM           18
#define FAN3_SLOT_NUM           19
#define FAN4_SLOT_NUM           20
#define FAN5_SLOT_NUM           21
#define FAN6_SLOT_NUM           22
#define FAN7_SLOT_NUM           23
#define FAN8_SLOT_NUM           24
#define FAN9_SLOT_NUM           25
#define FAN10_SLOT_NUM          26
#define FAN11_SLOT_NUM          27
#define FAN12_SLOT_NUM          28

#define IPMB_BLADE_BASE_ADDR       0x30
#define DEFAULT_PS_MAX_PWR_OUTPUT  1000 /* in watts */
#define DEFAULT_FAN_MAX_PWR_INPUT  10   /* in watts */

#endif
