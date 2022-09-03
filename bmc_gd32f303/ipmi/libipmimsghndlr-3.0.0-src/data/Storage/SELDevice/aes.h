/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2012, American Megatrends Inc.         ***
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
* Filename: aes.h
*
* Descriptions: Declaration of functions that connect to AES engine 
*   
*
* Author: Manish Tomar (manisht@ami.com)
*
******************************************************************/

#ifndef _AES_H
#define _AES_H

#include "IPMI_SEL.h"


int AES_SendEvent(SELEventRecord_T *event);

#endif
