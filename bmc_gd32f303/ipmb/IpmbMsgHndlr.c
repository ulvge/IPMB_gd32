/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2020-2020,Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **                   **
 **                                                            **
 **               **
 **                                                            **
 ****************************************************************
 */

/******************************************************************
*
*
*
*
******************************************************************/


#include "IpmbMsgHndlr.h"
#include "MsgHndlr.h"
#include "IPMI_App.h"
#include "AMI.h"
#include "App.h"
#include "Bridge.h"
#include "Chassis.h"
#include "Storage.h"
#include "SensorEvent.h"
#include "DeviceConfig.h"
#include "IPMIConf.h"


bool ProcessIpmbMsg(uint8_t *p_buffer, uint32_t len)
{
	if(len < sizeof(IPMIMsgHdr_T) + 1){
        return false;
    }
   return false; 
}

