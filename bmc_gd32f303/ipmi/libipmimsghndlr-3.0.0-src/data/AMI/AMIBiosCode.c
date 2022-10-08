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
 * AMIBiosCode.c
 * 
 *
 * Author: Gokulakannan. S <gokulakannans@amiindia.co.in>
 *****************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>

#include "Support.h"
#include "dbgout.h"
#include "PMConfig.h"
#include "NVRData.h"
#include "NVRAccess.h"
#include "IPMIDefs.h"
#include "SharedMem.h"
#include "IPMI_AMIBiosCode.h"
#include "AMIBiosCode.h"
#include "hal_hw.h"
#include "PDKCmdsAccess.h"
#include "iniparser.h"
#include "dictionary.h"
#include "parse-ex.h"
#include "IPMIConf.h"
#if AMI_GET_BIOS_CODE != UNIMPLEMENTED

static INT32U BIOS_Bitmap(INT32U Bitmap, INT32U BIOS_flag ,INT32U BIOS_State)
{
	INT32U BitMap_output=0;
	INT32U State=1;
	int i=0;
	for(i=0; i < BIOS_FLAG_BIT ; i++)
	{
		if(  (Bitmap & (State<<i)) > 0)
		{
			if( (BIOS_flag & (State<<i)) > 0 )
				BitMap_output +=(BIOS_flag & (State<<i));
		}
		else
			BitMap_output=BitMap_output+( BIOS_State & (State<<i) );
	}
	return BitMap_output;
}
static INT8U
ReadBiosCodes(int nCode, unsigned char *pCode, int *pLen)
{
	void *pHandle = NULL;
	int (*pGetBiosCode) ( unsigned char *, int );
	
	pHandle = dlopen("/usr/local/lib/libsnoop.so", RTLD_NOW);
	if (pHandle == NULL)
	{
		return CC_INV_CMD;
	}
	
	if (nCode == CURRENT)
	{
		pGetBiosCode = dlsym(pHandle, "ReadCurrentBiosCode");
	}
	else if (nCode == PREVIOUS)
	{
		pGetBiosCode = dlsym(pHandle, "ReadPreviousBiosCode");
	}
	else
	{
		dlclose (pHandle);
		return CC_INV_DATA_FIELD;
	}
	
	if (pGetBiosCode == NULL)
	{
		dlclose (pHandle);
		return CC_INV_CMD;
	}
	
	(*pLen) = pGetBiosCode(pCode, MAX_SIZE);

	/* Check whether the driver returned a INT number*/
	if ((*pLen) == -1)
	{
		TDBG("No Bios Codes Found \n");
		(*pLen) = 0;
		dlclose (pHandle);
		return OEMCC_FILE_ERR;
	}
	
#if 0
    printf("Bios Codes :\n");

    for(i=0;i<(*pLen);i++)
    {
        printf("%02X ", pCode[i]);
        if ((i>0) && (((i+1)%16)==0))
            printf("\n");
    }
    printf("\n");
#endif

    dlclose (pHandle);
    return CC_NORMAL;
}


/**
 * @fn AMIGetBiosCode
 * @brief Set the SOL Status.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval      CC_NORMAL, on success,
 *              CC_INV_DATA_FIELD, if invalid data in the request.
 */

int
AMIGetBiosCode (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
    AMIGetBiosCodeReq_T* pAMIGetBiosCodeReq  =  ( AMIGetBiosCodeReq_T* ) pReq;
    AMIGetBiosCodeRes_T* pAMIGetBiosCodeRes  =  ( AMIGetBiosCodeRes_T* ) pRes;
    int nLen = 0;

    memset (pAMIGetBiosCodeRes, 0x00, sizeof (AMIGetBiosCodeRes_T));
    pAMIGetBiosCodeRes->CompletionCode = ReadBiosCodes(pAMIGetBiosCodeReq->Command, pAMIGetBiosCodeRes->BiosCode, &nLen);

    // Length of the bioscode including the completion code.
    return (nLen + 1);
}

#endif

/**
 * @fn AMISendToBios
 * @brief to triger the flag to BIOS.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval      CC_NORMAL, on success,
 *              CC_INV_DATA_FIELD, if invalid data in the request.
 */

int
AMISendToBios (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
	AMISendToBiosReq_T* pAMISendToBiosReq  =  ( AMISendToBiosReq_T* ) pReq;
	AMISendToBiosRes_T* pAMISendToBiosRes  =  ( AMISendToBiosRes_T* ) pRes;
	int Data_size=0;

	if( (ReqLen < 3 || ReqLen > (MAX_BUFFER_SIZE+3)) )
	{
		pAMISendToBiosRes->CompletionCode = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	}
	Data_size = ReqLen -3;
	if(Data_size == pAMISendToBiosReq->Request_Data_Length)
	{
		pAMISendToBiosRes->Sequence_ID=pAMISendToBiosReq->Sequence_ID;
	}
	else 
	{
		pAMISendToBiosRes->CompletionCode = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	}
	LOCK_BMC_SHARED_MEM(BMCInst);
	memset(&(BMC_GET_SHARED_MEM(BMCInst)->dataBuffer), 0, sizeof(BIOS_Control_buffer_T));
	BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Sequence_ID = pAMISendToBiosReq->Sequence_ID;
	BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Action = pAMISendToBiosReq->Action;
	BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length = Data_size;
	if (BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length > 0)
		memcpy(BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Variable_data, pAMISendToBiosReq->Variable_data, BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length);
	UNLOCK_BMC_SHARED_MEM(BMCInst) 
	if(g_PDKHandle[PDK_ENABLE_SMI] != NULL)
    {
		if( 0xFF == ((int(*)(int))g_PDKHandle[PDK_ENABLE_SMI]) (BMCInst))
        {
			pAMISendToBiosRes->CompletionCode = CC_UNSPECIFIED_ERR;
			return sizeof (INT8U);
        }
    }
    return sizeof(AMISendToBiosRes_T);
}
/**
 * @fn AMIGetBiosCommand
 * @brief For BIOS Get the reason to trigger SMI.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval      CC_NORMAL, on success,
 *              CC_INV_DATA_FIELD, if invalid data in the request.
 */
int
AMIGetBiosCommand (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
	AMIGetBiosCommandRes_T* pAMIGetBiosCommandRes  =  ( AMIGetBiosCommandRes_T* ) pRes;
	LOCK_BMC_SHARED_MEM(BMCInst);
	pAMIGetBiosCommandRes->Action=BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Action;
	pAMIGetBiosCommandRes->Sequence_ID = BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Sequence_ID;
	pAMIGetBiosCommandRes->Request_Data_Length =  BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length;
	memset(pAMIGetBiosCommandRes->Variable_data, 0, MAX_BUFFER_SIZE);
	if (BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length > 0)
		memcpy(pAMIGetBiosCommandRes->Variable_data, BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Variable_data,BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length);
	pAMIGetBiosCommandRes->CompletionCode = CC_NORMAL;
	UNLOCK_BMC_SHARED_MEM(BMCInst);
		return pAMIGetBiosCommandRes->Request_Data_Length+sizeof(pAMIGetBiosCommandRes->Request_Data_Length)+sizeof(pAMIGetBiosCommandRes->Sequence_ID)+sizeof(pAMIGetBiosCommandRes->Action)+1;
	
    return sizeof(AMIGetBiosCommandRes_T);
}
/**
 * @fn AMIGetBiosCommand
 * @brief For BIOS set the response data to BIOS.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval      CC_NORMAL, on success,
 *              CC_INV_DATA_FIELD, if invalid data in the request.
 */
int
AMISetBiosResponse (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
	AMISetBiosReponseReq_T* pAMISetBiosReponseReq  =  ( AMISetBiosReponseReq_T* ) pReq;
	AMISetBiosReponseRes_T* pAMISetBiosReponseRes  =  ( AMISetBiosReponseRes_T* ) pRes;
	int Data_size =0;
	LOCK_BMC_SHARED_MEM(BMCInst);
	if (BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Sequence_ID == pAMISetBiosReponseReq->Sequence_ID)
	{
		Data_size=ReqLen-2;
		if(Data_size != pAMISetBiosReponseReq->Response_Data_Length)
		{
			UNLOCK_BMC_SHARED_MEM(BMCInst);
			pAMISetBiosReponseRes->CompletionCode = CC_REQ_INV_LEN;
			return  CC_REQ_INV_LEN;
		}
		
		BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length = Data_size;
		memset(&(BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Variable_data), 0, MAX_BUFFER_SIZE);
		
		if (BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length > 0)
			memcpy(BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Variable_data, pAMISetBiosReponseReq->Response_data, BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length);
		UNLOCK_BMC_SHARED_MEM(BMCInst);
		return sizeof(AMISetBiosReponseRes_T);
	}
	else
	{
		UNLOCK_BMC_SHARED_MEM(BMCInst);
		pAMISetBiosReponseRes->CompletionCode = CC_ACTIVATE_SESS_INVALID_SESSION_ID;
		return sizeof (INT8U);
	}
}
/**
 * @fn AMIGetBiosResponse
 * @brief For BIOS Read the response data to BIOS.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval      CC_NORMAL, on success,
 *              CC_INV_DATA_FIELD, if invalid data in the request.
 */
int
AMIGetBiosResponse (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
	AMIGetBiosReponseSReq_T* pAMIGetBiosReponseSReq  =  ( AMIGetBiosReponseSReq_T* ) pReq;
	AMIGetBiosReponseSRes_T* pAMIGetBiosReponseSRes  =  ( AMIGetBiosReponseSRes_T* ) pRes;

	if(ReqLen > MAX_BUFFER_SIZE)
	{
		pAMIGetBiosReponseSRes->CompletionCode = CC_REQ_INV_LEN;
		return sizeof (INT8U);
	}
	UNLOCK_BMC_SHARED_MEM(BMCInst);
	if (BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Sequence_ID == pAMIGetBiosReponseSReq->Sequence_ID)
	{
		pAMIGetBiosReponseSRes->Response_Data_Length = BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length;
		
		memset(pAMIGetBiosReponseSRes->Response_data, 0, MAX_BUFFER_SIZE);
		if (BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length > 0)
			memcpy(pAMIGetBiosReponseSRes->Response_data, BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Variable_data,BMC_GET_SHARED_MEM(BMCInst)->dataBuffer.Data_Length);
		UNLOCK_BMC_SHARED_MEM(BMCInst);
		return pAMIGetBiosReponseSRes->Response_Data_Length+sizeof(pAMIGetBiosReponseSRes->Response_Data_Length)+1;
	}
	else
	{
		UNLOCK_BMC_SHARED_MEM(BMCInst);
		pAMIGetBiosReponseSRes->CompletionCode = CC_ACTIVATE_SESS_INVALID_SESSION_ID;
		return sizeof (INT8U);
	}
}
/**
 * @fn AMISetBiosFlag
 * @brief For BIOS set 32bit flag data to BIOS.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval      CC_NORMAL, on success,
 *              CC_INV_DATA_FIELD, if invalid data in the request.
 */
int
AMISetBiosFlag (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
	AMISetBiosFlagReq_T* pAMISetBiosFlagReq  =  ( AMISetBiosFlagReq_T* ) pReq;
	AMISetBiosFlagRes_T* pAMISetBiosFlagRes  =  ( AMISetBiosFlagRes_T* ) pRes;
	char Filename[MAX_FILE_SIZE];
	FILE *fp=NULL;
	int err_value = 0xFFFFFF;
	dictionary *d = NULL;
	INT32U BIOS_State=0;
	INT32U BIOS_flag=0;
	sprintf(Filename,"/conf/BMC%d/BIOS_FLAG.ini",BMCInst);
	
	d = iniparser_load(Filename);
	if( d != NULL )
	{
		BIOS_State=IniGetUInt(d,(char*)BIOS,BIOS_FLAG,err_value);
		BIOS_flag=BIOS_Bitmap((pAMISetBiosFlagReq->MASK),(pAMISetBiosFlagReq->BIOS_Flag),BIOS_State);	
	}
	iniparser_freedict(d);
	fp = fopen(Filename,"w");
	if(fp == NULL)
	{
		pAMISetBiosFlagRes->CompletionCode=CC_UNSPECIFIED_ERR;
		return sizeof(INT8U);
	}
	fprintf(fp, "[%s]\n", BIOS);
	fprintf(fp, "%s=%d\n",BIOS_FLAG ,BIOS_flag);
	fclose(fp);
		pAMISetBiosFlagRes->CompletionCode=CC_SUCCESS;
    
		return sizeof(INT8U);
}
int
/**
 * @fn AMIGetBiosFlag
 * @brief For BIOS read 32 bit flag data to BIOS.
 * @param[in] pReq - pointer to the request.
 * @param[in] ReqLen - Length of the request.
 * @param[out] pRes - pointer to the result.
 * @retval      CC_NORMAL, on success,
 *              CC_INV_DATA_FIELD, if invalid data in the request.
 */
AMIGetBiosFlag (INT8U* pReq, INT32U ReqLen, INT8U* pRes, int BMCInst)
{
	AMIGetBiosFlagRes_T* pAMIGetBiosFlagRes  =  ( AMIGetBiosFlagRes_T* ) pRes;
	char Filename[MAX_FILE_SIZE];
	dictionary *d = NULL;
	sprintf(Filename,"/conf/BMC%d/BIOS_FLAG.ini",BMCInst);
	int err_value = 0xFFFFFF;
	d = iniparser_load(Filename);
	if( d == NULL )
	{
		 pAMIGetBiosFlagRes->CompletionCode=CC_UNSPECIFIED_ERR;
		 return sizeof(INT8U);
	}
	pAMIGetBiosFlagRes->BIOS_Flag= IniGetUInt(d,(char*)BIOS,BIOS_FLAG,err_value);
	if (pAMIGetBiosFlagRes->BIOS_Flag == err_value)
	{
		 iniparser_freedict(d);
		 pAMIGetBiosFlagRes->CompletionCode=CC_UNSPECIFIED_ERR;
		 return sizeof(INT8U);
	}
	iniparser_freedict(d);
	pAMIGetBiosFlagRes->CompletionCode=CC_SUCCESS;
	return sizeof(AMIGetBiosFlagRes_T);
}
