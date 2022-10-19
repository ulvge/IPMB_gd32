/****************************************Copyright (c)****************************************************

*********************************************************************************************************/
#define FIFO_GLOBALS 
#include "FIFO.h"   
#include "stdint.h" 
#include "FreeRTOS.h"
#include "task.h"

		
/************************************************************************************************************
   FIFO  底层接口层
******************************************************************/
void FIFO_Init(FIFO *fifo, INT8U *array, INT16U deepth)
{
    fifo->deepth    = deepth;
    fifo->occupy    = 0;
    fifo->array     = array;
    fifo->limit     = array + deepth;
    fifo->wp = fifo->array;
    fifo->rp = fifo->array;
}

void FIFO_Reset(FIFO *fifo)
{
    fifo->occupy = 0;
    fifo->rp = fifo->array;
    fifo->wp = fifo->array;
}

BOOLEAN FIFO_Write(FIFO *fifo, INT8U data)
{
    if (fifo->occupy >= fifo->deepth) {
		return FALSE;
    }
    uint32_t x=taskENTER_CRITICAL_FROM_ISR();
    *fifo->wp++ = data;
    if (fifo->wp >= fifo->limit) {
		fifo->wp = fifo->array;
	}
    fifo->occupy++;
    taskEXIT_CRITICAL_FROM_ISR(x);
    return TRUE;
}

BOOLEAN FIFO_Writes(FIFO *fifo, INT8U *data, INT16U dataSize)
{
    if (dataSize > (fifo->deepth - fifo->occupy)) {
		return FALSE;
	}
    uint32_t x=taskENTER_CRITICAL_FROM_ISR();
    for (; dataSize > 0; dataSize--) { 
       *fifo->wp++ = *data++;
       if (fifo->wp >= fifo->limit){
		   fifo->wp = fifo->array;
	   }
       fifo->occupy++;
    }
    taskEXIT_CRITICAL_FROM_ISR(x);   
    return TRUE;
}

__INLINE  BOOLEAN FIFO_Empty(FIFO *fifo)
{
    if (fifo->occupy == 0){
		return true;
	} else {
		return false;
	}
}
 
BOOLEAN FIFO_Read(FIFO *fifo, INT8U *data)
{
    if (fifo->occupy == 0) {
		return false;
	}
    uint32_t x=taskENTER_CRITICAL_FROM_ISR();
    *data = *fifo->rp++;
    if (fifo->rp >= fifo->limit) {
		fifo->rp = fifo->array;
	}
    fifo->occupy--;
    taskEXIT_CRITICAL_FROM_ISR(x);
    return true;
}

BOOLEAN FIFO_ReadN(FIFO *fifo, INT8U *data, INT16U dataSize, INT16U *readLen)
{
    if (fifo->occupy == 0) {
		return false;
	}
    uint32_t x=taskENTER_CRITICAL_FROM_ISR();
    if (dataSize < fifo->occupy) {
        *readLen = dataSize;
    } else {
        *readLen = fifo->occupy;
    }
    INT16U tmpLen = *readLen;
    while (tmpLen--) {
        *data++ = *fifo->rp++;
        if (fifo->rp >= fifo->limit) {
            fifo->rp = fifo->array;
        }
    }
    fifo->occupy -= *readLen;
    taskEXIT_CRITICAL_FROM_ISR(x);
    return true;
}

