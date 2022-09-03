/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2008, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
* 
* Filename: libipmi_IPM.c
*
* Descriptions: Contains implementation of Device specific
*   high level functions
*
* Author: Rajasekhar
*
******************************************************************/
#include "libipmi_IPM.h"


#include <string.h>

/**
\breif	Executes GetDeviceID IPMI command.
 @param	pSession		[in]Session handle
 @param	pGetDeviceID	[out]Device ID structure according to IPMI Spec
 @param	timeout			[in]timeout value in seconds.

 @retval Returns LIBIPMI_STATUS_SUCCESS on success and error codes on failure
*/
uint16	IPMICMD_GetDeviceID( IPMI20_SESSION_T *pSession, GetDevIDRes_T	*pGetDeviceID, int timeout )
{
	uint8		Req [20];
	uint32		dwResLen;
	uint16		wRet;

	dwResLen = sizeof(GetDevIDRes_T);
	wRet = LIBIPMI_Send_RAW_IPMI2_0_Command(pSession, PAYLOAD_TYPE_IPMI,
											DEFAULT_NET_FN_LUN, CMD_GET_DEV_ID,
											Req, 0,
											(uint8*)pGetDeviceID, &dwResLen,
											timeout);
	return wRet;
}

/**
\breif	Executes GetDeviceGUID IPMI command.
 @param	pSession		[in]Session handle
 @param	pGetDeviceID	[out]Device GUID structure according to IPMI Spec
 @param	timeout			[in]timeout value in seconds.

 @retval Returns LIBIPMI_STATUS_SUCCESS on success and error codes on failure
*/
uint16	IPMICMD_GetDeviceGUID( IPMI20_SESSION_T *pSession, GetDevGUIDRes_T *pGetDeviceGUID, int timeout )
{
	uint8		Req [20];
	uint32		dwResLen;
	uint16		wRet;

	dwResLen = sizeof(GetDevGUIDRes_T);
	wRet = LIBIPMI_Send_RAW_IPMI2_0_Command(pSession, PAYLOAD_TYPE_IPMI,
											DEFAULT_NET_FN_LUN, CMD_GET_DEV_GUID,
											Req, 0,
											(uint8*)pGetDeviceGUID, &dwResLen,
											timeout);
	return wRet;
}


/**
\breif	Higher level function for retrieving DeviceID.
		Calls IPMICMD_GetDeviceID internally.
 @param	pSession		[in]Session handle
 @param	pszDeviceID		[out]Device ID in the form of chars
 @param	timeout			[in]timeout value in seconds.

 @retval Returns LIBIPMI_STATUS_SUCCESS on success and error codes on failure
*/
uint16 LIBIPMI_HL_GetDeviceID( IPMI20_SESSION_T *pSession, char *pszDeviceID, int timeout )
{
	uint8	i;
	uint16	wRet;
	char	szTemp[10];
	GetDevIDRes_T	DeviceID;

	pszDeviceID[0] = (char)0;
	
	wRet = IPMICMD_GetDeviceID( pSession, &DeviceID, timeout);

	if (wRet == LIBIPMI_E_SUCCESS)
	{
		uint8	*pbyRes;
		pbyRes = (uint8 *) &DeviceID;
		for (i = 0; i < (uint8)sizeof(GetDevIDRes_T); i++ )
		{
			sprintf (szTemp, "%x ", pbyRes[i]);
			strcat(pszDeviceID, szTemp);
		}
	}
	
	return wRet;
}

/**
\breif	Higher level function for retrieving Device GUID.
		Calls IPMICMD_GetDeviceGUID internally.
 @param	pSession		[in]Session handle
 @param	pszDeviceID		[out]Device GUID in the form of chars
 @param	timeout			[in]timeout value in seconds.

 @retval Returns LIBIPMI_STATUS_SUCCESS on success and error codes on failure
*/
uint16 LIBIPMI_HL_GetDeviceGUID( IPMI20_SESSION_T *pSession, char *pszDeviceGUID, int timeout )
{
	uint8	i;
	uint16	wRet;
	char	szTemp[10];
	GetDevGUIDRes_T DeviceGUID;

	pszDeviceGUID[0] = (char)0;
	
	wRet = IPMICMD_GetDeviceGUID( pSession, &DeviceGUID, timeout);

	if (wRet == LIBIPMI_E_SUCCESS)
	{
		uint8	*pbyRes;
		pbyRes = (uint8 *) &DeviceGUID;
		for (i = 0; i < (uint8)sizeof(GetDevGUIDRes_T); i++ )
		{
			sprintf (szTemp, "%x ", pbyRes[i]);
			strcat(pszDeviceGUID, szTemp);
		}
	}
	
	return wRet;
}

