#include "bsp_uart3.h"
#include <string.h>
#include "main.h"
#include "mcro_def.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"
#include "bsp_uartcomm.h"

																
#define UART_NUM_TOTAL 3

static UART_PARA_STRUCT *g_pUARTSHandler[UART_NUM_TOTAL] = {NULL};	

//use FIFO
int fputc(int ch, FILE *f)
{
    return UART_sendByte(DEBUG_UART_PERIPH, ch);
}
bool com_registHandler(UART_PARA_STRUCT *uartPara)
{
    uint32_t i;
    for (i = 0; i < UART_NUM_TOTAL; i++)
    {
        if (g_pUARTSHandler[i] == NULL){
            break;
        }
        if (g_pUARTSHandler[i]->usart_periph == uartPara->usart_periph) {
            return true; //alread exist
        }
    }
    if (i < UART_NUM_TOTAL) {
        g_pUARTSHandler[i] = uartPara;
        return true; //success
    } else{
        return false;
    }
}
UART_PARA_STRUCT *com_getHandler(uint32_t usart_periph)
{
    for (uint32_t i = 0; i < UART_NUM_TOTAL; i++)
    {
        if (g_pUARTSHandler[i] == NULL){
            continue;
        }
        if (g_pUARTSHandler[i]->usart_periph == usart_periph) {
            return g_pUARTSHandler[i]; //alread exist
        }
    }
    return NULL; //alread exist
}

bool UART_getByte(uint32_t usart_periph, uint8_t *p_buffer)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
    return FIFO_Read(&uartPara->fifo.rfifo, p_buffer);
}

bool UART_getData(uint32_t usart_periph, uint8_t *p_buffer, uint32_t buffSize, INT16U *retLen)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
    return FIFO_ReadN(&uartPara->fifo.rfifo, p_buffer, buffSize, (INT16U *)retLen);
}


bool UART_sendByte(uint32_t usart_periph, uint8_t dat)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
	FILE *f = NULL;
    
    FIFO_Write(&uartPara->fifo.sfifo, (INT8U)dat); 
	if(uartPara->fifo.status != UART_SENDING) {
		UART_sendFinally(usart_periph, &uartPara->fifo);
	} 
	return true;
}

bool UART_sendData(uint32_t usart_periph, uint8_t *str, uint16_t len)
{
    UART_PARA_STRUCT *uartPara = com_getHandler(usart_periph);
    if (uartPara == NULL) {
        return false;
    }
    FIFO_Writes(&uartPara->fifo.sfifo, str, len);
	
	if(uartPara->fifo.status != UART_SENDING) {
		UART_sendFinally(usart_periph, &uartPara->fifo);
	}            
	return true;
}

/// @brief NO fifo,can be called in HardFault_Handler
/// @param usart_periph 
/// @param str 
/// @param len 
void UART_sendDataBlock(uint32_t usart_periph, const uint8_t *str, uint16_t len)
{
    int i = 0;
    for (i = 0; i < len; i++)
    {
        usart_data_transmit(usart_periph, str[i]);
        while (RESET == usart_flag_get(usart_periph, USART_FLAG_TBE))
            ;
    }
}

/// @brief read one byte from fifo,and start transmit
/// @param usart_periph 
/// @param fifoUart 
/// @return 
INT8U UART_sendFinally(uint32_t usart_periph, FIFO_Buf_STRUCT *fifoUart)
{
    INT8U data;
    if(FIFO_Empty(&(fifoUart->sfifo)))
	{				
		fifoUart->status &= ~UART_SENDING;                            
		usart_interrupt_disable(usart_periph, USART_INT_TBE); 
		usart_interrupt_disable(usart_periph, USART_INT_TC); 
		return false ;	
	} else {  
		if(fifoUart->status != UART_SENDING) {	                            
			usart_interrupt_enable(usart_periph, USART_INT_TBE);  
			usart_interrupt_enable(usart_periph, USART_INT_TC);   
			fifoUart->status |= UART_SENDING;                      
		}
		if(FIFO_Read(&(fifoUart->sfifo), &data) == true) {	//sending data   
		    usart_data_transmit(usart_periph, data);
        }
		return true;
	}
}

void COM_init(UART_PARA_STRUCT *uartPara)
{
    com_registHandler(uartPara);
    /* enable GPIO clock */
    rcu_periph_clock_enable(uartPara->config->rcuGPIO);

    /* enable USART clock */
    rcu_periph_clock_enable(uartPara->config->rcuUart);
    rcu_periph_clock_enable(RCU_AF);

    /* connect port to USARTx_Tx */
    gpio_init(uartPara->config->txPort, uartPara->config->txPinMode, uartPara->config->txPinSpeed, uartPara->config->txPin);

    /* connect port to USARTx_Rx */
    gpio_init(uartPara->config->rxPort, uartPara->config->rxPinMode, uartPara->config->rxPinSpeed, uartPara->config->rxPin);

#ifdef UART0_REMAP
    gpio_pin_remap_config(GPIO_USART0_REMAP, ENABLE);
#endif
#ifdef UART1_REMAP
    gpio_pin_remap_config(GPIO_USART1_REMAP, ENABLE);
#endif
    /* USART configure */
    usart_deinit(uartPara->usart_periph);
    usart_baudrate_set(uartPara->usart_periph, uartPara->config->baud);
    usart_word_length_set(uartPara->usart_periph, USART_WL_8BIT);
    usart_stop_bit_set(uartPara->usart_periph, USART_STB_1BIT);
    usart_parity_config(uartPara->usart_periph, USART_PM_NONE);
    usart_hardware_flow_rts_config(uartPara->usart_periph, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(uartPara->usart_periph, USART_CTS_DISABLE);
    usart_receive_config(uartPara->usart_periph, USART_RECEIVE_ENABLE);
    usart_transmit_config(uartPara->usart_periph, USART_TRANSMIT_ENABLE);
    usart_enable(uartPara->usart_periph);

    /* USART interrupt configuration */
    nvic_irq_enable(uartPara->config->irqN, uartPara->config->prePriority, uartPara->config->subPriority);
    /* enable USART TBE interrupt */
    usart_interrupt_enable(uartPara->usart_periph, USART_INT_RBNE);
    usart_interrupt_enable(uartPara->usart_periph, USART_INT_TBE);
    usart_interrupt_enable(uartPara->usart_periph, USART_INT_TC);
}

