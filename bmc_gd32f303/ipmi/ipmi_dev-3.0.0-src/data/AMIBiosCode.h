/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2010-2011, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy, Building 200, Norcross,         **
 **                                                            **
 **        Georgia 30093, USA. Phone-(770)-246-8600.           **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * AMIBiosCode.h
 * 
 *
 * Author: Gokulakannan. S <gokulakannans@amiindia.co.in>
 *****************************************************************/


#ifndef AMIBIOSCODE_H_
#define AMIBIOSCODE_H_

#include "Types.h"

#define BIOS_FLAG_BIT 32

extern int AMIGetBiosCode (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);

extern int AMISendToBios (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);
extern int AMIGetBiosCommand (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);
extern int AMISetBiosResponse (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);
extern int AMIGetBiosResponse (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);
extern int AMISetBiosFlag (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);
extern int AMIGetBiosFlag (_NEAR_ INT8U* pReq, INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);
extern int AMIPLDMBIOSMsg(_NEAR_ INT8U *pReq,INT32U ReqLen, _NEAR_ INT8U* pRes, int BMCInst);




#endif /* AMIBIOSCODE_H_ */
