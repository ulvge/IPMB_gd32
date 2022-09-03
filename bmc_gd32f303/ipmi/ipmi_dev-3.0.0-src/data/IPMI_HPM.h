/* ***************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2012, American Megatrends Inc.             **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555, Oakbrook Parkway, Suite 200,  Norcross,       **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************
 ****************************************************************
 *
 * IPMI_HPM.h
 * IPMI HPM Command Definitions.
 *
 * Author: Joey Chen <JoeyChen@ami.com.tw>
 *
 *****************************************************************/
#ifndef IPMI_HPM_H
#define IPMI_HPM_H


/* HPM.1 Commands */
#define CMD_GET_TARGET_UPLD_CAPABLITIES     0x2E
#define CMD_GET_COMPONENT_PROPERTIES        0x2F

#define CMD_ABORT_FIRMWARE_UPGRADE          0x30
#define CMD_INITIATE_UPG_ACTION             0x31
#define CMD_UPLOAD_FIRMWARE_BLOCK           0x32
#define CMD_FINISH_FIRMWARE_UPLOAD          0x33
#define CMD_GET_UPGRADE_STATUS              0x34
#define CMD_ACTIVATE_FIRMWARE               0x35
#define CMD_QUERY_SELF_TEST_RESULTS         0x36
#define CMD_QUERY_ROLLBACK_STATUS           0x37
#define CMD_INITIATE_MANUAL_ROLLBACK        0x38


#endif  /* IPMI_HPM_H */
