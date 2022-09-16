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
* Filename: ipmi_common.h
*
* Author: Andrew McCallum 
*
******************************************************************/
#include "ipmi_common.h"
#include "Types.h"




/**
*@fn CalculateChecksum2
*@brief Calculates the checksum
*@param Pkt Pointer to the data for the checksum to be calculated
*@param Len Size of data for checksum calculation
*@return Returns the checksum value
*/
extern INT32U
CalculateCheckSum2(_FAR_ INT8U *Pkt, INT32U Len)
{
    INT8U Sum;
    INT32U i;

    /* Get Checksum 2 */
    Sum = 0;
    for (i = 3; i < Len; i++)
    {
        Sum += Pkt[i];
    }
    return (INT8U)(0xFF & (0x100 - Sum));
}

extern INT8U
CalculateCheckSum(const INT8U *Pkt, INT32U Len)
{
    INT8U Sum;
    INT32U i;

    /* Get Checksum 2 */
    Sum = 0;
    for (i = 0; i < Len; i++)
    {
        Sum += Pkt[i];
    }
    return (INT8U)(0xFF & (-Sum));
}

/**
*@fn CheckReqMsgValidation
*@brief check the request msg validation
*@param Pkt Pointer to the data for the checksum to be calculated
*@param Len Size of data for checksum calculation
*@return Returns the checksum value
*/
bool CheckMsgValidation(const INT8U *pReq, INT32U Len)
{
    if (Len < sizeof(IPMIMsgHdr_T))
    {
        return false;
    }

    if (CalculateCheckSum(pReq, 2) != pReq[2])
    {
        return false;
    }

    if (CalculateCheckSum(pReq, Len - 1) != pReq[Len - 1])
    {
        return false;
    }

    return true;
}
