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
* Filename: ipmb_utils.h
*
* Author: Andrew McCallum 
*
******************************************************************/
#ifndef IPMI_COMMON_H
#define IPMI_COMMON_H

#include "Types.h"
#include "IPMIDefs.h"

/**
*@fn CalculateChecksum2
*@brief Calculates the checksum
*@param Pkt Pointer to the data for the checksum to be calculated
*@param Len Size of data for checksum calculation
*@return Returns the checksum value
*/
extern INT32U CalculateCheckSum2(INT8U *Pkt, INT32U Len);
extern INT8U CalculateCheckSum(const INT8U *Pkt, INT32U Len);

/**
*@fn CheckReqMsgValidation
*@brief check the request msg validation
*@param Pkt Pointer to the data for the checksum to be calculated
*@param Len Size of data for checksum calculation
*@return Returns the checksum value
*/
extern bool CheckMsgValidation(const INT8U *pReq, INT32U Len);
#endif /*IPMI_COMMON_H*/
