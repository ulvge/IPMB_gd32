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
 ****************************************************************
 *
 * std_macros.h
 *
 *  Author: Anurag Bhatia
 *  Description: Taken from Platform.h inside IPMI stack.
 ******************************************************************/
#ifndef STD_MACROS_H
#define STD_MACROS_H

#include "Types.h"
#include "platform.h"



#if 0
//Anurag Written
/**
 * @def htoipmi_u32
 * @brief Converts unsigned 32 bit data from Host format to IPMI format.
 * @param Val 32bit value to be converted.
 **/
#define htoipmi_u32(Val)   (Val)
/* ((INT32U)(((Val & 0xff) << 24) +    \
                            ((Val & 0xff00) << 8) +             \
                            ((Val & 0xff0000) >> 8) +           \
                            ((Val & 0xff000000) >> 24)))*/


/**
 * @def ipmitoh_u32
 * @brief Converts unsigned 32 bit data from IPMI format to Host format.
 * @param Val 32bit value to be converted.
 **/
#define ipmitoh_u32(Val)    (Val)
/*((INT32U)(((Val & 0xff) << 24) +    \
                            ((Val & 0xff00) << 8) +             \
                            ((Val & 0xff0000) >> 8) +           \
                            ((Val & 0xff000000) >> 24)))
*/
/**
 * @def htoipmi_u16
 * @brief Converts unsigned 16 bit data from Host format to IPMI format.
 * @param Val 16bit value to be converted.
 **/
#define htoipmi_u16(Val)    (Val)
/*((INT16U)(((Val & 0xff) << 8) + (Val >> 8)))*/

/**
 * @def htoipmi_u16
 * @brief Converts unsigned 16 bit data from IPMI format to Host format.
 * @param Val 16bit value to be converted.
 **/
#define ipmitoh_u16(Val)    (Val)
/*((INT16U)(((Val & 0xff) << 8) + (Val >> 8)))*/

#endif

#if 0
/**
 * @def htons
 * @brief Converts unsigned 16 bit data from Host format to Network format.
 * @param Val 16bit value to be converted.
 **/
#define htons(Val)          Val

/**
 * @def htonl
 * @brief Converts unsigned 32 bit data from Host format to Network format.
 * @param Val 132bit value to be converted.
 **/
#define htonl(Val)          Val

/**
 * @def htons
 * @brief Converts unsigned 16 bit data from Network format to Host format.
 * @param Val 16bit value to be converted.
 **/
#define ntohs(Val)          Val

/**
 * @def ntohl
 * @brief Converts unsigned 32 bit data from Network format to Host format.
 * @param Val 32bit value to be converted.
 **/
#define ntohl(Val)          Val
#endif
#endif  /* STD_MACROS_H */
