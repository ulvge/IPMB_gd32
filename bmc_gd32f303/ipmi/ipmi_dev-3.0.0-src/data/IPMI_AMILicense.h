/****************************************************************
 **                                                            **
 **       (C)Copyright 2012, American Megatrends Inc.          **
 **                                                            **
 **                  All Rights Reserved.                      **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************
 *
 * Filename: IPMI_AMILicense.h
 * 
 *  Author: Poulabi Patra   <poulabipatra@amiindia.co.in>
 ****************************************************************/

#ifndef _IPMI_AMILICENSE_H    
#define _IPMI_AMILICENSE_H 1

#include "Types.h"

#define BUFFER_LENGTH            50
#define MAC_ADDR_LEN             6
#define LIC_SIGNATURE_LEN        2
#define LIC_SIGNATURE            "SP"
#define MAC_OUI_LEN              3
#define APPCODE_LEN              8
#define MAX_LIC_APP              32
#define LIC_KEY_LEN              26
#define MAX_LIC_KEY_LEN          60
#define ETH0_INDEX               0
#define LICENSE_CONF             "/etc/license/license.conf"
#define LICENSE_STATUS_FILE      "/conf/license/status"
#define LICENSE_TEMP_FILE        "/tmp/temp"
#define LICENSE_CNT_DAYS_FILE    "/conf/license/count_days"
#define LICENSE_DIR             "/conf/license/"
#define LICENSE_KEY_FILE        "/conf/license/keys"


#pragma pack(1)
typedef struct
{
    INT8U Signature[LIC_SIGNATURE_LEN];
    INT8U Lic_Validity;
    INT32U FtrList;
    INT8U MACAddr[MAC_ADDR_LEN];
    
}PACKED LicenseKeyInfo_T;
#pragma pack()
extern INT16S GetLicenseStatus(const char *AppCode, BOOL *Status);
extern BOOL  IsValidMACAddr(INT8U *LicMAC, INT8U *SysMAC);
#endif
