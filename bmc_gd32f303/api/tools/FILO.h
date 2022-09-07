/****************************************Copyright (c)****************************************************
**
*********************************************************************************************************/

#ifndef _FILO_H_
#define _FILO_H_


#ifndef FILO_GLOBALS
#define   EXT_FILO     extern
#else 
#define   EXT_FILO
#endif
 
#include "Types.h"   
#include "stdint.h"	
/******************************************************************
	FILO
******************************************************************/
typedef struct {
    INT16U      deepth;
    INT8U       *array;
    INT8U       *wp;    
} FILO;

EXT_FILO	void	FILO_Init(FILO *filo, INT8U *array, INT16U deepth);
EXT_FILO	void	FILO_Reset(FILO *filo);
EXT_FILO	INT8U*  FILO_StartPos(FILO *filo);
EXT_FILO	BOOLEAN FILO_IsEmpty(FILO *filo);
EXT_FILO	BOOLEAN FILO_IsFull(FILO *filo);
EXT_FILO	INT8U	FILO_Read(FILO *filo);
EXT_FILO	BOOLEAN FILO_Write(FILO *filo, INT8U unit);
EXT_FILO	INT16U  FILO_Occupy(FILO *filo);

#endif
