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
 * @file	SensorOB.h
 * @author	Hari Lakshmanan <haril@ami.com>
 * @brief	sensor specific routines
 ****************************************************************/

#ifndef SENSOROB_H
#define SENSOROB_H
#include "Types.h"

#define CMM_BLADES_PRESENT_SENSOR_EVENT_TYPE    0x15
#define CMM_BLADES_PRESENT_SENSOR               0x60
#define CMM_PS_SENSOR_TYPE                      0x03

#define CMM_THERM1_SENSOR                       0x0A
#define CMM_THERM2_SENSOR                       0x0B
#define CMM_CPU_TEMP_SENSOR                     0x0F
#define SW_1_TEMP_SENSOR                        0x50
#define SW_2_TEMP_SENSOR                        0x51
#define SW_3_TEMP_SENSOR                        0x52
#define SW_4_TEMP_SENSOR                        0x53

#define BLADE_STATE_FRU_SENSOR                  0x70
#define CMM_AGGR_THERM_SENSOR                   0x77

/* Sensor Numbers */
#define PS1_STATUS_SENSOR_NUM                   0x80
#define PS2_STATUS_SENSOR_NUM                   0x81

#define PS1_OP_STATE_SENSOR_NUM                 0x90
#define PS2_OP_STATE_SENSOR_NUM                 0x91

#define FAN1_STATUS_SENSOR_NUM                  0xA0
#define FAN2_STATUS_SENSOR_NUM                  0xA1
#define FAN3_STATUS_SENSOR_NUM                  0xA2
#define FAN4_STATUS_SENSOR_NUM                  0xA3
#define FAN5_STATUS_SENSOR_NUM                  0xA4
#define FAN6_STATUS_SENSOR_NUM                  0xA5
#define FAN7_STATUS_SENSOR_NUM                  0xA6
#define FAN8_STATUS_SENSOR_NUM                  0xA7
#define FAN9_STATUS_SENSOR_NUM                  0xA8
#define FAN10_STATUS_SENSOR_NUM                 0xA9
#define FAN11_STATUS_SENSOR_NUM                 0xAA
#define FAN12_STATUS_SENSOR_NUM                 0xAB

#define FAN1_OP_STATE_SENSOR_NUM                0xB0
#define FAN2_OP_STATE_SENSOR_NUM                0xB1
#define FAN3_OP_STATE_SENSOR_NUM                0xB2
#define FAN4_OP_STATE_SENSOR_NUM                0xB3
#define FAN5_OP_STATE_SENSOR_NUM                0xB4
#define FAN6_OP_STATE_SENSOR_NUM                0xB5
#define FAN7_OP_STATE_SENSOR_NUM                0xB6
#define FAN8_OP_STATE_SENSOR_NUM                0xB7
#define FAN9_OP_STATE_SENSOR_NUM                0xB8
#define FAN10_OP_STATE_SENSOR_NUM               0xB9
#define FAN11_OP_STATE_SENSOR_NUM               0xBA
#define FAN12_OP_STATE_SENSOR_NUM               0xBB

#define FAN1_TACH_SENSOR_NUM                    0x40
#define FAN2_TACH_SENSOR_NUM                    0x41
#define FAN3_TACH_SENSOR_NUM                    0x42
#define FAN4_TACH_SENSOR_NUM                    0x43
#define FAN5_TACH_SENSOR_NUM                    0x44
#define FAN6_TACH_SENSOR_NUM                    0x45
#define FAN7_TACH_SENSOR_NUM                    0x46
#define FAN8_TACH_SENSOR_NUM                    0x47
#define FAN9_TACH_SENSOR_NUM                    0x48
#define FAN10_TACH_SENSOR_NUM                   0x49
#define FAN11_TACH_SENSOR_NUM                   0x4A
#define FAN12_TACH_SENSOR_NUM                   0x4B

/**
 * @brief Calculate the chassis aggregated sensor state
 * @param pstate is the chassis aggregated sensor state
 * @return 0 on success, -1 on error.
**/
extern UINT32 PDK_CalcChassisAggregSensorState (INT8U* pstate);

#endif /*SENSOROB_H*/
