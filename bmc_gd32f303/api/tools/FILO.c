/****************************************Copyright (c)****************************************************

*********************************************************************************************************/
#define FILO_GLOBALS 
#include "FILO.h"  
#include "stdint.h"
#include "FreeRTOS.h"
#include "task.h"
	
#define OS_ENTER_CRITICAL() 		 taskENTER_CRITICAL()
#define OS_EXIT_CRITICAL 		 taskEXIT_CRITICAL
/******************************************************************
   FILO
******************************************************************/
void  FILO_Init(FILO *filo, INT8U *array, INT16U deepth)
{
    filo->deepth    = deepth;
    filo->array     = array;
    filo->wp = filo->array;
}
    
void  FILO_Reset(FILO *filo)
{
    filo->wp = filo->array;
}

INT8U*  FILO_StartPos(FILO *filo)
{
    return filo->array;
}    

BOOLEAN FILO_IsEmpty(FILO *filo)
{
    if (filo->wp == filo->array) return true;
    else return false;
}    
BOOLEAN FILO_IsFull(FILO *filo)
{
    if (filo->wp >= filo->array + filo->deepth) return true;
    else return false;
}    
    
BOOLEAN FILO_Write(FILO *filo, INT8U unit)
{
    if (FILO_IsFull(filo)) return false;
    else {
       *filo->wp = unit;
       filo->wp++;
       return true;
    }
}
    
INT8U  FILO_Read(FILO *filo)
{
    if (FILO_IsEmpty(filo)) return 0xff;
    filo->wp--;
    return *filo->wp;
}

INT16U  FILO_Occupy(FILO *filo)
{
    return filo->wp - filo->array;
}
