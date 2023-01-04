/****************************************Copyright (c)****************************************************

*********************************************************************************************************/
#define FIFO_GLOBALS 
#include "FIFO.h"   
#include "stdint.h" 
#include "FreeRTOS.h"
#include "task.h"

extern __asm uint32_t vPortGetIPSR(void);
static __inline uint32_t API_EnterCirtical(void) 
{
    if (vPortGetIPSR()) {
        return taskENTER_CRITICAL_FROM_ISR();
    }
    else {
        taskENTER_CRITICAL();
        return 0;
    }
}
static __inline void API_ExitCirtical(uint32_t x) 
{
    if (vPortGetIPSR()) {
        taskEXIT_CRITICAL_FROM_ISR(x);
    }
    else{
        taskEXIT_CRITICAL();
    }
}
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
    uint32_t x=API_EnterCirtical();
    *fifo->wp++ = data;
    if (fifo->wp >= fifo->limit) {
		fifo->wp = fifo->array;
	}
    fifo->occupy++;
    API_ExitCirtical(x);
    return TRUE;
}

BOOLEAN FIFO_Writes(FIFO *fifo, INT8U *data, INT16U dataSize)
{
    if (dataSize > (fifo->deepth - fifo->occupy)) {
		return FALSE;
	}
    uint32_t x=API_EnterCirtical();
    for (; dataSize > 0; dataSize--) { 
       *fifo->wp++ = *data++;
       if (fifo->wp >= fifo->limit){
		   fifo->wp = fifo->array;
	   }
       fifo->occupy++;
    }
    API_ExitCirtical(x);
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
    uint32_t x=API_EnterCirtical();
    *data = *fifo->rp++;
    if (fifo->rp >= fifo->limit) {
		fifo->rp = fifo->array;
	}
    fifo->occupy--;
    API_ExitCirtical(x);
    return true;
}

BOOLEAN FIFO_ReadN(FIFO *fifo, INT8U *data, INT16U dataSize, INT16U *readLen)
{
    if (fifo->occupy == 0) {
		return false;
	}
    uint32_t x=API_EnterCirtical();
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
    API_ExitCirtical(x);
    return true;
}

