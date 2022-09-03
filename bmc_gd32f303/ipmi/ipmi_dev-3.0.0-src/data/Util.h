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
 ****************************************************************/
/*****************************************************************
 *
 * util.h
 * Utility functions.
 *
 * Author: Govind Kothandapani <govindk@ami.com> 
 * 
 *****************************************************************/
#ifndef UTIL_H
#define UTIL_H
#include "Types.h"
#include "Platform.h"

/**
 * Returns the maximum of two values
**/
#define UTIL_MAX(val1, val2) \
    ((val1 > val2) ? val1 : val2)

/**
 * Returns the minimum of two values
**/
#define UTIL_MIN(val1, val2) \
    ((val1 < val2) ? val1 : val2)


    #define UNIX_SUCCESS    0

    #define UNIX_FAILURE    (-1)

/**
 * @brief Extracts the contents of the bits corresponding to the mask.
**/
extern INT8U GetBits (INT8U Val, INT8U Mask);

/**
 * @brief Sets the bits corresponding to the mask according to the value.
**/
extern INT8U SetBits (INT8U Mask, INT8U Val);

/**
 * @brief Find the checksum
**/
extern INT8U CalculateCheckSum (INT8U* Data, INT16U Len);

/**
*@ brief Retrieves the value from sysctl
*/
int GetJiffySysCtlvalue (const char *TagName, long long *SysVal);

/**
*@fn RestartDaemon
*@brief This function restarts the Daemon
*@param DaemonName -Daemon to be restarted
*@param ExtUnlinkFile - Anyother file to be removed
*@param SigNum - Signal used to kill process
*@return Returns 0 on success
*        Returns -1 on failure
*/
int RestartDaemonByForce(char *DaemonName,char *GenName,char *ExtUnlinkFile,int SigNum);

int GetINT32UBitsNum (INT32U value, INT8U *bitnum);


/*
*@fn IsValidUserName
 *@brief This function will check for the valid user Name
*@param value - UserName string 
*@param UserNameLen - UserName length
*@return Returns -1 on failure
*        Returns 0 on success
*/
int IsValidUserName (INT8U *pUserName, INT8U UserNameLen); 


#endif	/* UTIL_H */
