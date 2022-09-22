#include "bsp_usart1.h"
#include <string.h>
#include "main.h"
#include "mcro_def.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"    
#include "FIFO.h"  
#include "stdio.h"

#define USART1_DATA_ADDRESS    ((uint32_t)&USART_DATA(USART1))

static void uart1_dma_init(void);

static FIFO_Buf_STRUCT		g_FifoUART1;

#define SendCmdBuf_size 	(50)
static INT8U g_buffSend[600];	 
static INT8U g_buffRec[SendCmdBuf_size];

void com1_init()
{
	FIFO_Init(&g_FifoUART1.sfifo, g_buffSend, sizeof(g_buffSend));	
	FIFO_Init(&g_FifoUART1.rfifo, g_buffRec, sizeof(g_buffRec));
    /* enable GPIO clock */
    rcu_periph_clock_enable(COM1_GPIO_CLK);

    /* enable USART clock */
    rcu_periph_clock_enable(COM1_CLK);
    rcu_periph_clock_enable(RCU_AF);

    /* connect port to USARTx_Tx */
    gpio_init(COM1_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, COM1_TX_PIN);
    /* connect port to USARTx_Rx */
    gpio_init(COM1_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, COM1_RX_PIN);
	
#ifdef UART1_REMAP
    gpio_pin_remap_config(GPIO_USART1_REMAP, ENABLE);
#endif

    /* USART configure */
    usart_deinit(COM1);
    usart_baudrate_set(COM1, 115200U);
    usart_word_length_set(COM1, USART_WL_8BIT);
    usart_stop_bit_set(COM1, USART_STB_1BIT);
    usart_parity_config(COM1, USART_PM_NONE);
    usart_hardware_flow_rts_config(COM1, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(COM1, USART_CTS_DISABLE);
    usart_receive_config(COM1, USART_RECEIVE_ENABLE);
    usart_transmit_config(COM1, USART_TRANSMIT_ENABLE);
    usart_enable(COM1);

    //uart1_dma_init();
    /* USART interrupt configuration */
    nvic_irq_enable(USART1_IRQn, 3, 0); // USART1_IRQHandler
    /* enable USART TBE interrupt */
    usart_interrupt_enable(COM1, USART_INT_RBNE);   
    usart_interrupt_enable(COM1, USART_INT_TBE);
    usart_interrupt_enable(COM1, USART_INT_TC);

}

#if USE_UART1_AS_IPMI
static MsgPkt_T    g_uart_Req;
extern xQueueHandle RecvDatMsg_Queue;
#endif

extern xQueueHandle FTUartRead_Queue;


INT8U USART1_SendFinally(uint32_t usart_periph, FIFO_Buf_STRUCT *fifoUart)
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
			//usart_transmit_config(usart_periph, USART_TRANSMIT_ENABLE);
		}
		if(FIFO_Read(&(fifoUart->sfifo), &data) == true) {	//sending data   
		    usart_data_transmit(usart_periph, data);
        }
		return true;
	}
}
void USART1_IRQHandler(void)
{
    uint8_t res;
	static BaseType_t xHigherPriorityTaskWoken;  // must set xHigherPriorityTaskWoken as a static variable, why?
    BaseType_t err;
    static bool is_start = false;

    if (RESET != usart_interrupt_flag_get(COM1, USART_INT_FLAG_RBNE))
    {                        
        res = usart_data_receive(COM1);
#if USE_UART1_AS_IPMI
        usart_interrupt_flag_clear(COM1, USART_INT_FLAG_RBNE);
        /* receive data */
        if (res == START_BYTE)
        { // start
            is_start = true;
            g_uart_Req.Size = 0;
        }
        else if (res == STOP_BYTE && is_start == true)
        { // stop
            is_start = false;
            // usart_data_transmit(USART1, HAND_SHAKE_BYTE); // BMC hand shake
            if (RecvDatMsg_Queue != NULL)
            {
                g_uart_Req.Param = SERIAL_REQUEST;
                err = xQueueSendFromISR(RecvDatMsg_Queue, (char*)&g_uart_Req, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                if (err == pdFAIL)
                {
                    LOG_E("uart queue msg send failed!");
                }
            }
        }
        else
        {
            g_uart_Req.Data[g_uart_Req.Size++] = res;
            if (g_uart_Req.Size > sizeof(g_uart_Req.Data) - 3)
            {
                is_start = false;
                g_uart_Req.Size = 0;
                //    LOG_E("uart recv overlap!");
            }
        }

	// use FIFO store all  
		if (is_start == false)
		{
			/* receive data */
			FIFO_Write(&g_FifoUART1.rfifo, (INT8U)res);
			//fputc(res, NULL);     //loopback
		}
#endif
	}

	if (RESET != usart_interrupt_flag_get(COM1, USART_INT_FLAG_TBE))
    {
        /* send data continue */
		USART1_SendFinally(COM1, &g_FifoUART1);
    }
    if (RESET != usart_interrupt_flag_get(COM1, USART_INT_FLAG_TC))
    {
        /* send data continue */
		USART1_SendFinally(COM1, &g_FifoUART1);
    }
}

#if 1  //use FIFO
int fputc(int ch, FILE *f)
{
    FIFO_Write(&g_FifoUART1.sfifo, (INT8U)ch); 
	if(g_FifoUART1.status != UART_SENDING) {
		USART1_SendFinally(COM1, &g_FifoUART1);
	}
	return ch;
}
#else
int fputc(int ch, FILE *f)
{
	usart_data_transmit(USART1, (uint8_t)ch);
	while (RESET == usart_flag_get(USART1, USART_FLAG_TBE))
	;

	return ch;
}
#endif

void uart1_send_byte(char dat)
{
	FILE *f = NULL;
    fputc((int)dat, f);
}

void uart1_send_dat(uint8_t *str, uint16_t len)
{
    FIFO_Writes(&g_FifoUART1.sfifo, str , len);
	
	if(g_FifoUART1.status != UART_SENDING) {
		USART1_SendFinally(COM1, &g_FifoUART1);
	}
}

void uart1_send_string(char *str)
{
	uart1_send_dat((uint8_t *)str, strlen(str)+1);
}

bool uart1_get_dataN(uint8_t *p_buffer, uint32_t buffSize, uint32_t *retLen)
{
    return FIFO_ReadN(&g_FifoUART1.rfifo, p_buffer, buffSize, (INT16U *)retLen);
}
bool uart1_get_data(uint8_t *p_buffer)
{
    return FIFO_Read(&g_FifoUART1.rfifo, p_buffer);
}

static void uart1_dma_init(void)
{
    dma_parameter_struct dma_init_struct;
    /* enable DMA1 */
    rcu_periph_clock_enable(RCU_DMA0);
    /* deinitialize DMA channel2 (USART0 rx) */
    dma_deinit(DMA0, DMA_CH5);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)g_uart_Req.Data;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = 10;
    dma_init_struct.periph_addr = USART1_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(DMA0, DMA_CH5, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH5);
    /* enable DMA channel2 */
    dma_channel_enable(DMA0, DMA_CH5);
    
    usart_dma_receive_config(COM1, USART_DENR_ENABLE);

}

void uart1_dma_enable(uint8_t len)
{
    int tick = 100;
    dma_parameter_struct dma_init_struct;
    BaseType_t err;

    /* deinitialize DMA channel2 (USART0 rx) */
    dma_deinit(DMA0, DMA_CH5);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)g_uart_Req.Data;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = len;
    dma_init_struct.periph_addr = USART1_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(DMA0, DMA_CH5, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH5);
    /* enable DMA channel2 */
    dma_channel_enable(DMA0, DMA_CH5);

    /* wait DMA channel transfer complete */
    while(RESET == dma_flag_get(DMA0, DMA_CH5, DMA_FLAG_G) && tick)
    {
        msleep(2);
        tick--;
    }
    msleep(20);
    if(tick)
    {
        g_uart_Req.Param = FTCPU_RESPONSE;
        g_uart_Req.Size = len - dma_transfer_number_get(DMA0, DMA_CH5);
        if(g_uart_Req.Size > 2)
        {
            if(g_uart_Req.Data[0] == START_BYTE && g_uart_Req.Data[g_uart_Req.Size-1] == STOP_BYTE)
            {
                err = xQueueSend(FTUartRead_Queue, (char*)&g_uart_Req, 10);
                if (err == pdFAIL)
                {
                    LOG_E("uart1 FT_COM msg send failed!");
                }
            }
            else
            {
                LOG_E("FT_COM redv msg header or tail error!");
            }
        }
        else
        {
            LOG_E("FT_COM recv msg length less than 2!");
        }
    }
}
