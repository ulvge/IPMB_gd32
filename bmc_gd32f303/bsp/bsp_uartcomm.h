#ifndef __BSP_UARTCOMM_H
#define	__BSP_UARTCOMM_H


#include "project_select.h"
#include <stdio.h>
#include <stdbool.h>
#include <FIFO.h>

typedef struct {
    uint32_t    baud;
    IRQn_Type   irqN;
    uint8_t     prePriority;
    uint8_t     subPriority;

    uint32_t    txPort;
    uint32_t    txPin;
    uint8_t     txPinMode;
    uint8_t     txPinSpeed;
	
    uint32_t    rxPort;
    uint32_t    rxPin;
    uint8_t     rxPinMode;
    uint8_t     rxPinSpeed;
    rcu_periph_enum    rcuUart;
    rcu_periph_enum    rcuGPIO;
    uint32_t    remap;
}UART_CONFIG_STRUCT;

typedef struct {
    uint32_t                    usart_periph;
    FIFO_Buf_STRUCT             fifo;
    const UART_CONFIG_STRUCT    *config;
}UART_PARA_STRUCT;

void UART_sendDataBlock(uint32_t usart_periph, const uint8_t *str, uint16_t len);
//int fputc(int ch, FILE *f);
void COM_init(UART_PARA_STRUCT *uartPara);

bool UART_getByte(uint32_t usart_periph, uint8_t *p_buffer);
bool UART_getData(uint32_t usart_periph, uint8_t *p_buffer, uint32_t buffSize, INT16U *retLen);
bool UART_sendByte(uint32_t usart_periph, uint8_t dat);
bool UART_sendData(uint32_t usart_periph, uint8_t *str, uint16_t len);
INT8U UART_sendFinally(uint32_t usart_periph, FIFO_Buf_STRUCT *fifoUart);

#endif /* __BSP_UARTCOMM_H */
