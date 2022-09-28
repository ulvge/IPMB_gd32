#include "bsp_usart1.h"
#include <string.h>
#include "main.h"
#include "mcro_def.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"    
#include "FIFO.h"  
#include "stdio.h"
#include "bsp_uartcomm.h"

#define SendCmdBuf_size 	(50)
static INT8U g_buffSend[600];	 
static INT8U g_buffRec[SendCmdBuf_size];

static const UART_CONFIG_STRUCT g_uart0Config= {
    .baud = 115200U,
    .irqN = USART1_IRQn,
    .prePriority = 3,
    .subPriority = 0,
    
    .txPort = COM1_GPIO_PORT,
    .txPin = COM1_TX_PIN,
    .txPinMode = GPIO_MODE_AF_PP,
    .txPinSpeed = GPIO_OSPEED_50MHZ,

    .rxPort = COM1_GPIO_PORT,
    .rxPin = COM1_RX_PIN,
    .rxPinMode = GPIO_MODE_IN_FLOATING,
    .rxPinSpeed = GPIO_OSPEED_50MHZ,

    .rcuUart = COM1_CLK,
    .rcuGPIO = COM1_GPIO_CLK,
};
static UART_PARA_STRUCT g_UARTPara = {
    .usart_periph = COM1,  
    .config = &g_uart0Config,
};

void com1_init()
{
	FIFO_Init(&g_UARTPara.fifo.sfifo, g_buffSend, sizeof(g_buffSend));	
	FIFO_Init(&g_UARTPara.fifo.rfifo, g_buffRec, sizeof(g_buffRec));
    com_init(&g_UARTPara);
	
}

#if USE_UART1_AS_IPMI
static MsgPkt_T    g_uart_Req;
extern xQueueHandle RecvDatMsg_Queue;
#endif

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
#endif

	// use FIFO store all  
		if (is_start == false)
		{
			/* receive data */
            //uart_sendByte(COM1, res);  //loopback
			FIFO_Write(&g_UARTPara.fifo.rfifo, (INT8U)res); // only save
		}
	}

	if (RESET != usart_interrupt_flag_get(COM1, USART_INT_FLAG_TBE))
    {
        /* send data continue */
		uart_SendFinally(COM1, &g_UARTPara.fifo);
    }
    if (RESET != usart_interrupt_flag_get(COM1, USART_INT_FLAG_TC))
    {
        /* send data continue */
		uart_SendFinally(COM1, &g_UARTPara.fifo);
    }
}
