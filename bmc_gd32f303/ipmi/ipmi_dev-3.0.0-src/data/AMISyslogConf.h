/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2005-2006, American Megatrends Inc.        **
 ***                                                            **
 ***            All Rights Reserved.                            **
 ***                                                            **
 ***        6145-F, Northbelt Parkway, Norcross,                **
 ***                                                            **
 ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 ***                                                            **
 *****************************************************************
 ******************************************************************
 *
 * AmiSyslogConf.h
 * AMI Syslog configuration command Macros
 *
 * Author: Gokula Kannan. S	<gokulakannans@amiindia.co.in>
 *
 ******************************************************************/
#ifndef __AMISYSLOGCONF_H__
#define __AMISYSLOGCONF_H__

#include "Types.h"

/**
 * @fn AMIGetLogConf
 * @brief Get the log configuration file.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval  	CC_NORMAL, on success,
 * 				CC_UNSPECIFIED_ERR, if any unknown errors.
 */
extern int AMIGetLogConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);
/**
 * @fn AMIGetLogConf
 * @brief Get the log configuration file.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval  	CC_NORMAL, on success,
 * 				CC_UNSPECIFIED_ERR, if any unknown errors.
 */
extern int AMISetLogConf(INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);

/**
 * @fn AMIGetExtendedLogConf
 * @brief Get the Debug flag file.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval    CC_NORMAL, on success,
 *                CC_UNSPECIFIED_ERR, if any unknown errors.
 */

extern int AMIGetExtendedLogConf (INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);

/**
 * @fn AMISetExtendedLogConf
 * @brief Set the Debug flag file.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval      CC_NORMAL, on success,
 *                  CC_UNSPECIFIED_ERR, if any unknown errors.
 */

extern int AMISetExtendedLogConf (INT8U* pReq, INT32U ReqLen, INT8U* pRes,int BMCInst);

#endif // __AMISYSLOGCONF_H__


