/****************************************Copyright (c)****************************************************
**
*********************************************************************************************************/

#ifndef _FIFO_H_
#define _FIFO_H_


#ifndef FIFO_GLOBALS
#define   EXT_FIFO     extern
#else 
#define   EXT_FIFO
#endif
					 
#include "Types.h"
#include "stdint.h"	

typedef enum{//FIFO_SendData
	FIFO_Chan_USART = 0,
	FIFO_Chan_BEEP = 1,
	FIFO_Chan_ESP = 2
}FIFO_CHAN_ENUM;

typedef struct {
    INT16U      deepth;
    INT16U      occupy;
    INT8U       *array;  //buff address start 
    INT8U       *limit; //buff address end = array + deepth
    INT8U       *wp;
    INT8U       *rp;
} FIFO; 	     

#define UART_SENDING                0x01
typedef struct {
    INT32U          status;
    FIFO            sfifo;                    
    FIFO            rfifo;                    
}FIFO_Buf_STRUCT;	


#define   DBG_UART           FIFO_Chan_USART	

/******************************************************************
	FIFO
******************************************************************/
			
EXT_FIFO	BOOLEAN		USART_main(INT8U chan);  
EXT_FIFO	void		FIFO_Init(FIFO *fifo, INT8U *array, INT16U deepth);
EXT_FIFO	void		FIFO_Reset(FIFO *fifo);
EXT_FIFO	BOOLEAN		FIFO_Write(FIFO *fifo, INT8U unit); 
EXT_FIFO	BOOLEAN		FIFO_Writes(FIFO *fifo, INT8U *data, INT16U dataSize);
EXT_FIFO	BOOLEAN		FIFO_Empty(FIFO *fifo);
EXT_FIFO    BOOLEAN     FIFO_Read(FIFO *fifo, INT8U *data);
EXT_FIFO    BOOLEAN     FIFO_ReadN(FIFO *fifo, INT8U *data, INT16U dataSize, INT16U *len);

#endif
