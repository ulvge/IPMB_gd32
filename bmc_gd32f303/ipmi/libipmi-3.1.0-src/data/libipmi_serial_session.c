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
* Filename: libipmi_serial_session.c
*
* Description: Contains implementation of session establishment
*   functions to BMC thru serial medium
*
* Author: Rajasekhar
*
******************************************************************/
#include <stdlib.h>
#include <string.h>

#include "libipmi_session.h"
#include "libipmi_serial.h"
#include "libipmi_helper.h"

#include "mcro_def.h"




uint16 Send_RAW_IPMI2_0_Serial_Command(IPMI20_SESSION_T *pSession, 
										uint8 byNetFnLUN, uint8 byCommand,
										uint8 *pbyReqData, uint32 dwReqDataLen,
										uint8 *pbyResData, uint32 *pdwResDataLen,
										int	timeout);

uint8 WriteSerialPort(IPMI20_SESSION_T *pSession, uint8 *pbyData, 
						uint32 dwDataLen, int timeout);
uint8 ReadByte(IPMI20_SESSION_T *pSession, uint8 *pbyData, int timeout);
uint8 ReadSerialPort(IPMI20_SESSION_T *pSession, uint8 *pbyData, 
					 uint32 *pdwDataLen, int timeout);
uint8 OpenSerialPort(IPMI20_SESSION_T *pSession, char *pszDevice);
void  ByteEscapeEncode(uint8	*pbyInputData,	uint32 dwInputDataLen,
						 uint8	*pbyOutputData,	uint32 *pdwOutputDataLen);
uint8 ByteEscapeDecode(uint8	*pbyInputData,	uint32 dwInputDataLen,
						 uint8	*pbyOutputData,	uint32 *pdwOutputDataLen);
uint8 ValidateCheckSum(uint8	*pbyData, uint32 dwDataLen);
uint16 ChangeSerialPortSettings(IPMI20_SESSION_T *pSession);

extern uint8	CalculateCheckSum(uint8 *pbyStart, uint8 *pbyEnd);

/*---------------------------------------------------------------------
Create_IPMI20_Serial_Session

Establishes a connection with BMC using Serial medium.
----------------------------------------------------------------------*/
uint16 Create_IPMI20_Serial_Session(IPMI20_SESSION_T *pSession, char *pszDevice,
							char *pszUserName, char *pszPassword, 
							uint8 byAuthType,
							uint8 *byPrivLevel,
							uint8 byReqAddr, uint8 byResAddr,
							int timeout)
{

	/* Serial port opened successfully */
	return LIBIPMI_E_SUCCESS;
}

/*---------------------------------------------------------------------
Close_IPMI20_Serial_Session

Closes connection with BMC using Serial medium.
----------------------------------------------------------------------*/
uint16 Close_IPMI20_Serial_Session( IPMI20_SESSION_T *pSession )
{
	uint16	wRet;
	uint32	Req;
	uint8	byRes[MAX_RESPONSE_SIZE];
	uint32	dwResLen;

	Req = pSession->hSerialSession->dwSessionID;
	wRet = Send_RAW_IPMI2_0_Serial_Command(pSession, 
										DEFAULT_NET_FN_LUN, CMD_CLOSE_SESSION,
										(uint8 *)&Req, sizeof(uint32),
										byRes, &dwResLen,
										pSession->hSerialSession->byDefTimeout);
	return wRet;
}

/*---------------------------------------------------------------------
Send_RAW_IPMI2_0_Serial_Command

Sends RAW IPMI command to BMC using serial medium
----------------------------------------------------------------------*/
uint16 Send_RAW_IPMI2_0_Serial_Command(IPMI20_SESSION_T *pSession,
										uint8 byNetFnLUN, uint8 byCommand,
										uint8 *pbyReqData, uint32 dwReqDataLen,
										uint8 *pbyResData, uint32 *pdwResDataLen,
										int	timeout)
{
	uint8			pbyRequestData[MAX_REQUEST_SIZE];
	uint8			pbyResponseData[MAX_RESPONSE_SIZE];
	uint8			EncryptBuf[MAX_REQUEST_SIZE];
	uint32			dwReqLen;
	uint32			dwResLen;
	uint32			dwEncryptBuffLen;
	IPMIMsgHdr_T	*pbyIPMIHdr;
	uint8			*pbyIPMIData;
	uint8			byRet;
	uint8			byCompletionCode;
	
	/* Find the IPMI Header and IPMI Data Part */
	pbyIPMIHdr = (IPMIMsgHdr_T	*)&pbyRequestData[ 0 ];
	pbyIPMIData = &pbyRequestData[ sizeof(IPMIMsgHdr_T) ];
	

	/* Fill the packet */
	pbyIPMIHdr->ResAddr 	= pSession->hSerialSession->byResAddr;
	pbyIPMIHdr->ReqAddr 	= pSession->hSerialSession->byReqAddr;
	pbyIPMIHdr->NetFnLUN 	= byNetFnLUN;
	pbyIPMIHdr->RqSeqLUN	= pSession->hSerialSession->byIPMBSeqNum << 2;
	pbyIPMIHdr->Cmd		= byCommand;
	pbyIPMIHdr->ChkSum	= LIBIPMI_CalculateCheckSum( (uint8 *)pbyIPMIHdr, (uint8 *)&(pbyIPMIHdr->ChkSum) ); /* cheksum_1 */

	//printf("libipmi_raw_serial: req len : %ld\n",dwReqDataLen);
	/* Increment the sequence number */
	pSession->hSerialSession->byIPMBSeqNum = (pSession->hSerialSession->byIPMBSeqNum+1) % 0x40;

	/* Fill the Data part  */
	memcpy(pbyIPMIData, pbyReqData, dwReqDataLen);

	/* Calculate CheckSum-2*/
	pbyIPMIData[dwReqDataLen] = LIBIPMI_CalculateCheckSum( (&(pbyIPMIHdr->ChkSum))+1, &pbyIPMIData[dwReqDataLen] );

	/* Update the request data length */
	dwReqLen = sizeof(IPMIMsgHdr_T) + dwReqDataLen + 1;
	//printf("libipmi_raw_serial: sizeof(IPMIMsgHdr_T) : %d\n",sizeof(IPMIMsgHdr_T));

	//printf("libipmi_raw_serial: before bytes escape encode : %ld\n",dwReqLen);
	/* Byte Escape Encoding */
	ByteEscapeEncode(&pbyRequestData[0], dwReqLen, &EncryptBuf[0], &dwEncryptBuffLen);

	//printf("libipmi_raw_serial: total bytes written: %ld\n",dwEncryptBuffLen);
	/* Send the data to BMC */
	byRet = WriteSerialPort(pSession, &EncryptBuf[0], dwEncryptBuffLen, timeout);

	/* Get response */
	if(byRet == LIBIPMI_STATUS_SUCCESS)
		byRet = ReadSerialPort(pSession, &pbyResponseData[0], &dwResLen, timeout);
	
	/* Return on error */
	if(byRet != LIBIPMI_STATUS_SUCCESS)
		return STATUS_CODE(MEDIUM_ERROR_FLAG, byRet);

	/* Byte Escape Decoding */
	ByteEscapeDecode(&pbyResponseData[0], dwResLen, &EncryptBuf[0], &dwEncryptBuffLen);

	/* Validate CheckSum */
	if(!LIBIPMI_ValidateCheckSum(&EncryptBuf[0], dwEncryptBuffLen))
		return	STATUS_CODE(MEDIUM_ERROR_FLAG, LIBIPMI_E_CHKSUM_MISMATCH);

	/* Completion Code */
	byCompletionCode = EncryptBuf[ sizeof(IPMIMsgHdr_T) ];
	if( byCompletionCode != CC_NORMAL )
		return	STATUS_CODE(IPMI_ERROR_FLAG, byCompletionCode );
	
	/* Calculate response data len */
	*pdwResDataLen = dwEncryptBuffLen - sizeof(IPMIMsgHdr_T) - 1;
		
	memcpy(pbyResData, &EncryptBuf[ sizeof(IPMIMsgHdr_T) ], *pdwResDataLen);

	return LIBIPMI_E_SUCCESS;
}

/*---------------------------------------------------------------------
LIBIPMI_SetSerialPortSettings

Sets serial port settings 
----------------------------------------------------------------------*/
uint16 ChangeSerialPortSettings(IPMI20_SESSION_T *pSession)
{
	return LIBIPMI_E_SUCCESS;
}

/*---------------------------------------------------------------------
WriteSerialPort

Writes data to the serial port between A0 and A5
----------------------------------------------------------------------*/
uint8 WriteSerialPort(IPMI20_SESSION_T *pSession, uint8 *pbyData, 
					  uint32 dwDataLen, int timeout)
{
	uint8	byTempData=0;
	uint8	byRet;


#if 0 //LIBIPMI_IS_OS_LINUX()
	fd_set				wdfs;
	struct timeval		wait_time;
	int					nRet;

	FD_ZERO(&wdfs);
	FD_SET(pSession->hSerialSession->hSerialPort, &wdfs);
	
	/* set the timeout */
	wait_time.tv_sec=timeout;
	wait_time.tv_usec=0;
	nRet = select(pSession->hSerialSession->hSerialPort+1, NULL, 
						&wdfs, NULL, &wait_time);
	
	/* error */
	if(nRet == -1)
	{
		DEBUG_PRINTF("Select failed while waiting for write pipe \n");
		return LIBIPMI_MEDIUM_E_INVALID_SOCKET;
	}
	/* timeout happened */
	else if(nRet == 0)
	{
		DEBUG_PRINTF("Connection timedout: Write Pipe is full \n");
		return LIBIPMI_MEDIUM_E_TIMED_OUT;
	}


	//printf("LIBIPMI: datalen = %ld\n",dwDataLen);
#if 1
	/* write pipe is free. write the data */
	nRet = write(pSession->hSerialSession->hSerialPort, pbyData, dwDataLen);
	if(nRet != (int)dwDataLen)
	{
		printf("Error writing to serial: bytes written : %d != %d\n",nRet,dwDataLen);
		return LIBIPMI_MEDIUM_E_SEND_DATA_FAILURE;
	}
#endif


#else

	/* For any other OS, return error */
	return LIBIPMI_MEDIUM_E_OS_UNSUPPORTED;

#endif

	/* Receive Hand Shake */
	if( (byRet = ReadByte(pSession, &byTempData, timeout)) != LIBIPMI_STATUS_SUCCESS )
	{
		return byRet;
	}

	if(byTempData != HAND_SHAKE_BYTE)
	{
		return LIBIPMI_SESSION_E_HANDSHAKE_NOT_RECVD;
	}
	//printf("LIBIPMI_SESSION_E_HANDSHAKE_RECVD\n");
	return LIBIPMI_STATUS_SUCCESS;
}

/*---------------------------------------------------------------------
ReadByte

Reads a byte from Serial Port
----------------------------------------------------------------------*/
uint8 ReadByte(IPMI20_SESSION_T *pSession, uint8 *pbyData, int timeout)
{
#if 0// LIBIPMI_IS_OS_LINUX()
	fd_set				rdfs;
	struct timeval		wait_time;
	int					nRet;


	FD_ZERO(&rdfs);
	FD_SET(pSession->hSerialSession->hSerialPort, &rdfs);
	
	/* set the timeout */
	//wait_time.tv_sec=timeout;
	wait_time.tv_sec=30;
	wait_time.tv_usec=0;
	nRet = select(pSession->hSerialSession->hSerialPort+1, &rdfs, 
						NULL, NULL, &wait_time);
	
	/* error */
	if(nRet == -1)
	{
		DEBUG_PRINTF("Select failed while waiting for read pipe \n");
		printf("Select failed while waiting for read pipe \n");
		return LIBIPMI_MEDIUM_E_INVALID_SOCKET;
	}
	/* timeout happened */
	else if(nRet == 0)
	{
		DEBUG_PRINTF("Connection timedout: Read Pipe is empty \n");
		printf("Connection timedout: Read Pipe is empty \n");
		return LIBIPMI_MEDIUM_E_TIMED_OUT;
	}

	/* read pipe has some data. read the data */
	nRet = read(pSession->hSerialSession->hSerialPort, pbyData, 1);

	if(nRet != 1)
		return LIBIPMI_MEDIUM_E_SEND_DATA_FAILURE;
#else

	/* For any other OS return error */
	return LIBIPMI_MEDIUM_E_OS_UNSUPPORTED;

#endif
	
	return LIBIPMI_STATUS_SUCCESS;
}

/*---------------------------------------------------------------------
ReadSerialPort

Reads data from Serial Port
----------------------------------------------------------------------*/
uint8 ReadSerialPort(IPMI20_SESSION_T *pSession, uint8 *pbyData, 
					 uint32 *pdwDataLen, int timeout)
{
	uint8	byTempData;
	uint8	byRet;

	byTempData = 0x00;

	/* Receive A0 from serial port */
	if( (byRet = ReadByte(pSession, &byTempData, timeout)) != LIBIPMI_STATUS_SUCCESS )
		return byRet;
		
	if(byTempData != START_BYTE)
		return LIBIPMI_MEDIUM_E_INVALID_DATA;
	
	*pdwDataLen = 0;
	
	/* Now Receive data till A5 */
	while( (byRet = ReadByte(pSession, &byTempData, timeout)) == LIBIPMI_STATUS_SUCCESS )
	{
		if(byTempData != STOP_BYTE)
			pbyData[(*pdwDataLen)++] = byTempData;
		else
			break;
	}

	/* Data received. Check if any error occured in between */
	if( byRet != LIBIPMI_STATUS_SUCCESS)
		return byRet;

		
	return LIBIPMI_STATUS_SUCCESS;
}

/*---------------------------------------------------------------------
OpenSerialPort

Opens the serial port
----------------------------------------------------------------------*/
uint8	OpenSerialPort(IPMI20_SESSION_T *pSession, char *pszDevice)
{
	
	return LIBIPMI_STATUS_SUCCESS;
}


/*---------------------------------------------------------------------
ByteEscapeEncode

Encodes the data using byte Escape
IPMI Spec Page-204 Table 14-5 & 14-6
----------------------------------------------------------------------*/
void ByteEscapeEncode(uint8	*pbyInputData,	uint32 dwInputDataLen,
						 uint8	*pbyOutputData,	uint32 *pdwOutputDataLen)
{
	uint32 i;

	*pdwOutputDataLen = 0;
	pbyOutputData[(*pdwOutputDataLen)++] = START_BYTE;
	for(i = 0; i < dwInputDataLen; i++)
	{
		switch(pbyInputData[i])
		{
			case START_BYTE:
				pbyOutputData[(*pdwOutputDataLen)++] = DATA_ESCAPE;
				pbyOutputData[(*pdwOutputDataLen)++] = ENCODED_START_BYTE;
				break;
			case STOP_BYTE:
				pbyOutputData[(*pdwOutputDataLen)++] = DATA_ESCAPE;
				pbyOutputData[(*pdwOutputDataLen)++] = ENCODED_STOP_BYTE;
				break;
			case HAND_SHAKE_BYTE:
				pbyOutputData[(*pdwOutputDataLen)++] = DATA_ESCAPE;
				pbyOutputData[(*pdwOutputDataLen)++] = ENCODED_HAND_SHAKE_BYTE;
				break;
			case DATA_ESCAPE:
				pbyOutputData[(*pdwOutputDataLen)++] = DATA_ESCAPE;
				pbyOutputData[(*pdwOutputDataLen)++] = ENCODED_DATA_ESCAPE;
				break;
			case BYTE_ESCAPE:
				pbyOutputData[(*pdwOutputDataLen)++] = DATA_ESCAPE;
				pbyOutputData[(*pdwOutputDataLen)++] = ENCODED_BYTE_ESCAPE;
				break;
			default:
				pbyOutputData[(*pdwOutputDataLen)++] = pbyInputData[i];
				break;
		}
	}
	pbyOutputData[(*pdwOutputDataLen)++] = STOP_BYTE;

	return;
}

/*---------------------------------------------------------------------
ByteEscapeDecode

Decodes the data using byte Escape
IPMI Spec Page-204 Table 14-5 & 14-6
----------------------------------------------------------------------*/
uint8 ByteEscapeDecode(uint8	*pbyInputData,	uint32 dwInputDataLen,
						 uint8	*pbyOutputData,	uint32 *pdwOutputDataLen)
{
	uint32	i;
	uint8	byESCByteRecvd=0;

	*pdwOutputDataLen = 0;
	for(i = 0; i < dwInputDataLen; i++)
	{
		/* ESC byte already received so decode the next byte */
		if( byESCByteRecvd )
		{
			switch(pbyInputData[i])
			{
				case ENCODED_START_BYTE:
					pbyOutputData[(*pdwOutputDataLen)++] = START_BYTE;
					break;
				case ENCODED_STOP_BYTE:
					pbyOutputData[(*pdwOutputDataLen)++] = STOP_BYTE;
					break;
				case ENCODED_HAND_SHAKE_BYTE:
					pbyOutputData[(*pdwOutputDataLen)++] = HAND_SHAKE_BYTE;
					break;
				case ENCODED_DATA_ESCAPE:
					pbyOutputData[(*pdwOutputDataLen)++] = DATA_ESCAPE;
					break;
				case ENCODED_BYTE_ESCAPE:
					pbyOutputData[(*pdwOutputDataLen)++] = BYTE_ESCAPE;
					break;
				default:
					return LIBIPMI_MEDIUM_E_INVALID_DATA;					
			}
			
			byESCByteRecvd = 0;
		}
		else if( pbyInputData[i] == DATA_ESCAPE ) /* ESC received just now */
			byESCByteRecvd = 1;
		else /* Normal data */
			pbyOutputData[(*pdwOutputDataLen)++] = pbyInputData[i];
	}

	return LIBIPMI_STATUS_SUCCESS;
}
						
/*---------------------------------------------------------------------
CloseSerialPort

Closes the Serial Port
----------------------------------------------------------------------*/
uint8 CloseSerialPort(IPMI20_SESSION_T *pSession)
{

	return LIBIPMI_STATUS_SUCCESS;
}





