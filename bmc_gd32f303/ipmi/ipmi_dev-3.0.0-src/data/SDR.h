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
 * SDR.h
 * SDR functions.
 *
 *  Author: Rama Bisa <ramab@ami.com>
 *
 ******************************************************************/
#ifndef SDR_H
#define SDR_H

#include "Types.h"
#include "IPMI_SDR.h"
#include "SDRRecord.h"

typedef enum{
  UNSPECIFIED = 0,
  INPUT,
  OUTPUT,
  RESERVED
} SensorDir_E;

#pragma pack( 1 )

/**
 * @struct E2ROMHdr_T
 * @brief EEROM Header.
**/
typedef struct
{
    INT8U Valid;
    INT8U Len;

} PACKED  E2ROMHdr_T;

#pragma pack( )

/**
 * @var g_SDRRAM
 * @brief SDR Repository.
**/
extern SDRRepository_T*     g_SDRRAM;


/**
 * @defgroup src SDR Repository Command handlers
 * @ingroup storage
 * IPMI Sensor Data Records Repository interface commands.
 * These commands provide read/write access to BMC's SDR repository.
 * @{
**/
extern INT8U GetSDRRepositoryNum(void);
extern int GetSDRRepositoryInfo(INT8U *pReq, INT8U ReqLen, INT8U *pRes, int BMCInst);
extern int GetSDRRepositoryAllocInfo (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int ReserveSDRRepository      (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSDR                    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int AddSDR                    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int PartialAddSDR             (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int DeleteSDR                 (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int ClearSDRRepository        (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int GetSDRRepositoryTime      (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int SetSDRRepositoryTime      (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int EnterSDRUpdateMode        (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int ExitSDRUpdateMode         (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
extern int RunInitializationAgent    (INT8U* pReq, INT8U ReqLen, INT8U* pRes,int BMCInst);
/** @} */

/**
 * @brief Get the next SDR record ID.
 * @param RecID - Current SDR record ID.
 * @return the next SDR record ID.
**/
extern INT16U   SDR_GetNextSDRId     (INT16U RecID,int BMCInst);

/**
 * @brief Reads SDR Repository contents.
 * @param pSDRRec - Current SDR Record header.
 * @return the next SDR Record header.
**/
extern SDRRecHdr_T*   ReadSDRRepository (SDRRecHdr_T* pSDRRec,int BMCInst);
extern FullSensorRec_T *ReadSensorRecByID(INT8U id, int BMCInst);
extern FullSensorRec_T *ReadSensorRecByName(INT8U sensorName, int BMCInst);
/**
 * @brief Write into SDR Repository.
 * @param pSDRRec - Record to be written.
 * @param Offset  - Write offset.
 * @param Size    - Size of write.
 * @return the SDR Record header.
**/
extern void WriteSDRRepository (SDRRecHdr_T* pSDRRec, INT8U Offset, INT8U Size,INT8U SdrSize,int BMCInst);

/**
 * @brief Get the SDR Record.
 * @param RecID - SDR Record ID.
 * @return the SDR Record.
**/
extern SDRRecHdr_T* GetSDRRec  (INT16U RecID,int BMCInst);

/**
 * @brief Initialize SDR Repository.
 * @return 0 if success, -1 if error
**/
extern  int  InitSDR (int BMCInst);


#endif /* SDR_H */
