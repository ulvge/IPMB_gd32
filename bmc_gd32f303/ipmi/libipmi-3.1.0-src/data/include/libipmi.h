/*****************************-*- andrew-c -*-********************************/
/* Filename:    libipmi.h                                                    */
/* Author:      Andrew McCallum (andrewm@ami.com)                            */
/* Created:     10/06/2003                                                   */
/* Modified:    03/24/2004                                                   */
/* Description: The public interface to the IPMI library.  All IPMI related  */
/*              functions are accessible through this interface.  Inherited  */
/*              from libhhm, which has gone on to a better place.            */
/*****************************************************************************/

#ifndef LIBIPMI_H
#define LIBIPMI_H

/** \file libipmi.h
 *  \brief Public interface for all IPMI related functions
 *  
 *  This library contains all functionality needed to send IPMI commands and
 *  receive responses via the I2C bus.  Common useful IPMI commands like
 *  GetDeviceID, GetSensorReading, and others are built into this library.
 *  If a needed command is not present, it can still be sent using the
 *  \ref ipmi_generic_command function.
 *
 *  This library handles all the details of sequence numbers, waiting for
 *  responses, verifying checksums, retrying commands in the event of
 *  timeouts and failures, etc.

*/

/** \mainpage IPMI Command Library
 *
 *  \section Overview
 *  
 *  This library contains all functionality needed to send IPMI commands and
 *  receive responses via the I2C bus.  Common useful IPMI commands like
 *  GetDeviceID, GetSensorReading, and others are built into this library.
 *  If a needed command is not present, it can still be sent using the
 *  \ref ipmi_generic_command function.
 *
 *  This library handles all the details of sequence numbers, waiting for
 *  responses, verifying checksums, retrying commands in the event of
 *  timeouts and failures, etc.

*/


#include "Types.h"
//#include "ipmi_structs.h"
#define s8 int8
#define s32 int32

/*! Return value that indicates a specified sensor is disabled, and cannot be read */
#define IPMI_SENSOR_DISABLED        ( -3 )

/* IPMI threshold state definitions for monitoring */
#define THRESH_UNINITIALIZED        ( (u16)0x00 ) /*!< Threshold state on first run */
#define THRESH_NORMAL               ( (u16)0x01 ) /*!< Sensor is normal (unused in IPMI ) */
#define THRESH_UP_NONCRIT           ( (u16)0x02 ) /*!< IPMI Upper Non-Critical Threshold */
#define THRESH_UP_CRITICAL          ( (u16)0x04 ) /*!< IPMI Upper Critical Threshold */
#define THRESH_LOW_NONCRIT          ( (u16)0x08 ) /*!< IPMI Lower Non-Critical Threshold */
#define THRESH_LOW_CRITICAL         ( (u16)0x10 ) /*!< IPMI Lower Critical Threshold */
#define THRESH_ACCESS_FAILED        ( (u16)0x20 ) /*!< Access failed sensor state */
#define THRESH_UP_NON_RECOV         ( (u16)0x40 ) /*!< IPMI Upper Non-Recoverable Threshold */
#define THRESH_LOW_NON_RECOV        ( (u16)0x80 ) /*!< IPMI Lower Non-Recoverable Threshold */


/*****                NetFn Definitions and Macros                       *****/
/* Adapted/stolen from MegaNASFreeBSD/Development/ipmid/ipmb.h originally    */

/* Macros for converting netfn/lun combos */
#define NETFN( netfnlun )       ( ( netfnlun & (u8)0xFC ) >> 2 )
#define NETLUN( netfnlun )      ( ( netfnlun & (u8)0x03 ) )
#define NETFNLUN( netfn,lun )   ( ( netfn << 2 ) | ( lun ) )

/* Macros for converting seq/lun combos */
#define SEQ( seqlun )           ( ( seqlun & (u8)0xFC ) >> 2 )
#define SLUN( seqlun )          ( ( seqlun & (u8)0x03 ) )
#define SEQLUN( seq, lun )      ( ( seq << 2 ) | ( lun ) )


/*****                 IPMI Constant Definitions                         *****/

/* Completion Codes */
#define IPMI_SUCCESS            ( (u8)0x00 )

/* Sensor Data Record types */
#define SDR_FULL                ( (u8)0x01 ) /**< SDR RecordType for Full Sensor Record */
#define SDR_COMPACT             ( (u8)0x02 ) /**< SDR RecordType for Compact Sensor Record */

/* Raw sensor reading numeric format codes */
#define SDR_READING_UNSIGNED    ( (u8)0 )
#define SDR_READING_1SCOMP      ( (u8)1 )
#define SDR_READING_2SCOMP      ( (u8)2 )
#define SDR_READING_NONANALOG   ( (u8)3 )

/* Max size of an SDR of type SDR_FULL.  SDR_COMPACT records are smaller. */
#define SDR_MAX_SIZE            ( 64 )

/* Chassis control codes */
typedef enum
{
    CHASSIS_POWER_OFF               = 0x00,
    CHASSIS_POWER_ON                = 0x01,
    CHASSIS_POWER_CYCLE             = 0x02,
    CHASSIS_POWER_RESET             = 0x03,
    CHASSIS_DIAGNOSTIC_INTERRUPT    = 0x04,
    CHASSIS_SOFT_OFF                = 0x05,
    CHASSIS_SMI_INTERRUPT           = 0x06,
}CHASSIS_CMD_CTRL;

/* Sensor Type Codes */
#define IPMI_SENSOR_TEMPERATURE     ( (u8)0x01 )
#define IPMI_SENSOR_VOLTAGE         ( (u8)0x02 )
#define IPMI_SENSOR_CURRENT         ( (u8)0x03 )
#define IPMI_SENSOR_FAN             ( (u8)0x04 )
#define IPMI_SENSOR_POWER_SUPPLY    ( (u8)0x08 )
#define IPMI_SENSOR_POWER_UNIT      ( (u8)0x09 )
#define IPMI_SENSOR_COOLING_DEVICE  ( (u8)0x0A )
#define OTHER_UNITS_SENSOR_TYPE     ( (u8)0x0B )
#define IPMI_SENSOR_MEMORY          ( (u8)0x0C )

/* Sensor Unit Type Codes from the IPMI spec */
#define IPMI_UNIT_UNSPECIFIED   0
#define IPMI_UNIT_DEGREES_C     1
#define IPMI_UNIT_DEGREES_F     2
#define IPMI_UNIT_DEGREES_K     3
#define IPMI_UNIT_VOLTS         4
#define IPMI_UNIT_AMPS          5
#define IPMI_UNIT_WATTS         6
#define IPMI_UNIT_JOULES        7
#define IPMI_UNIT_COULOMBS      8
#define IPMI_UNIT_VA            9
#define IPMI_UNIT_NITS          10
#define IPMI_UNIT_LUMEN         11
#define IPMI_UNIT_LUX           12
#define IPMI_UNIT_CANDELA       13
#define IPMI_UNIT_KPA           14
#define IPMI_UNIT_PSI           15
#define IPMI_UNIT_NEWTON        16
#define IPMI_UNIT_CFM           17
#define IPMI_UNIT_RPM           18
#define IPMI_UNIT_HZ            19
#define IPMI_UNIT_MICROSECOND   20
#define IPMI_UNIT_MILLISECOND   21
#define IPMI_UNIT_SECOND        22
#define IPMI_UNIT_MINUTE        23
#define IPMI_UNIT_HOUR          24
#define IPMI_UNIT_DAY           25
#define IPMI_UNIT_WEEK          26
#define IPMI_UNIT_MIL           27
#define IPMI_UNIT_INCHES        28
#define IPMI_UNIT_FEET          29
#define IPMI_UNIT_CUIN          30
#define IPMI_UNIT_CUFEET        31
#define IPMI_UNIT_MM            32
#define IPMI_UNIT_CM            33
#define IPMI_UNIT_M             34
#define IPMI_UNIT_CUCM          35
#define IPMI_UNIT_CUM           36
#define IPMI_UNIT_LITERS        37
#define IPMI_UNIT_FLUIDOUNCE    38
#define IPMI_UNIT_RADIANS       39
#define IPMI_UNIT_STERADIANS    40
#define IPMI_UNIT_REVOLUTIONS   41
#define IPMI_UNIT_CYCLES        42
#define IPMI_UNIT_GRAVITIES     43
#define IPMI_UNIT_OUNCE         44
#define IPMI_UNIT_POUND         45
#define IPMI_UNIT_FTLB          46
#define IPMI_UNIT_OZIN          47
#define IPMI_UNIT_GAUSS         48
#define IPMI_UNIT_GILBERTS      49
#define IPMI_UNIT_HENRY         50
#define IPMI_UNIT_MILLIHENRY    51
#define IPMI_UNIT_FARAD         52
#define IPMI_UNIT_MICROFARAD    53
#define IPMI_UNIT_OHMS          54
#define IPMI_UNIT_SIEMENS       55
#define IPMI_UNIT_MOLE          56
#define IPMI_UNIT_BECQUEREL     57
#define IPMI_UNIT_PPM           58
#define IPMI_UNIT_RESERVED      59
#define IPMI_UNIT_DECIBELS      60
#define IPMI_UNIT_DBA           61
#define IPMI_UNIT_DBC           62
#define IPMI_UNIT_GRAY          63
#define IPMI_UNIT_SIEVERT       64
#define IPMI_UNIT_COLORTEMPDK   65
#define IPMI_UNIT_BIT           66
#define IPMI_UNIT_KILOBIT       67
#define IPMI_UNIT_MEGABIT       68
#define IPMI_UNIT_GIGABIT       69
#define IPMI_UNIT_BYTE          70
#define IPMI_UNIT_KILOBYTE      71
#define IPMI_UNIT_MEGABYTE      72
#define IPMI_UNIT_GIGABYTE      73
#define IPMI_UNIT_WORD          74
#define IPMI_UNIT_DWORD         75
#define IPMI_UNIT_QWORD         76
#define IPMI_UNIT_LINE          77
#define IPMI_UNIT_HIT           78
#define IPMI_UNIT_MISS          79
#define IPMI_UNIT_RETRY         80
#define IPMI_UNIT_RESET         81
#define IPMI_UNIT_OVERRUNFLOW   82
#define IPMI_UNIT_UNDERRUN      83
#define IPMI_UNIT_COLLISION     84
#define IPMI_UNIT_PACKETS       85
#define IPMI_UNIT_MESSAGES      86
#define IPMI_UNIT_CHARACTERS    87
#define IPMI_UNIT_ERROR         88
#define IPMI_UNIT_CORRERROR     89
#define IPMI_UNIT_UNCORRERROR   90


#endif
