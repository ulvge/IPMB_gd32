#include "bsp_usart0.h"
#include <string.h>   
#include <stdarg.h>  
#include "main.h"
#include "mcro_def.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"  
#include "FIFO.h"
#include "bsp_uartcomm.h"
#include "boot_update.h"

static const UART_CONFIG_STRUCT g_uart0Config= {
    .baud = 115200U,
    .irqN = USART0_IRQn,
    .prePriority = 10,
    .subPriority = 0,
    
    .txPort = GPIOA,
    .txPin = GPIO_PIN_9,
    .txPinMode = GPIO_MODE_AF_PP,
    .txPinSpeed = GPIO_OSPEED_50MHZ,

    .rxPort = GPIOA,
    .rxPin = GPIO_PIN_10,
    .rxPinMode = GPIO_MODE_IN_FLOATING,
    .rxPinSpeed = GPIO_OSPEED_50MHZ,

    .rcuUart = RCU_USART0,
    .rcuGPIO = RCU_GPIOA,
    .remap = NULL,
};
static UART_PARA_STRUCT g_UARTPara = {
    .usart_periph = COM0,  
    .config = &g_uart0Config,
};

#define UART0_BUFF_SIZE 	(50)
static INT8U g_buffSend[500];
static INT8U g_buffRec[UART0_BUFF_SIZE];

void UART0_init(void)
{
	FIFO_Init(&g_UARTPara.fifo.sfifo, g_buffSend, sizeof(g_buffSend));	
	FIFO_Init(&g_UARTPara.fifo.rfifo, g_buffRec, sizeof(g_buffRec));
    COM_init(&g_UARTPara);
}
static BootPkt_T    g_uart_Req;

extern void Delay_NoSchedue(uint32_t clk);
void USART0_IRQHandler(void)
{
#define COM_NUM    COM0
    uint8_t res;
	static BaseType_t xHigherPriorityTaskWoken;  // must set xHigherPriorityTaskWoken as a static variable, why?
    BaseType_t err;

    if (RESET != usart_interrupt_flag_get(COM_NUM, USART_INT_FLAG_RBNE))
    {
        res = usart_data_receive(COM_NUM);
		usart_interrupt_enable(g_UARTPara.usart_periph, USART_INT_IDLE);
        usart_interrupt_flag_clear(COM_NUM, USART_INT_FLAG_RBNE);
        /* receive data */
        g_uart_Req.Data[g_uart_Req.Size++] = res;
        if (g_uart_Req.Size > sizeof(g_uart_Req.Data) - 3)
        {
            g_uart_Req.Size = 0;
            //LOG_E("uart recv overlap!\n");
        }              
	}
    if (RESET != usart_interrupt_flag_get(COM_NUM, USART_INT_FLAG_IDLE))
    {
		usart_interrupt_disable(g_UARTPara.usart_periph, USART_INT_IDLE);
		if (g_uart_Req.Size != 0) {
			if (updateDatMsg_Queue != NULL)
			{                          
				err = xQueueSendFromISR(updateDatMsg_Queue, (char*)&g_uart_Req.Size, &xHigherPriorityTaskWoken);
				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
				if (err == pdFAIL)
				{
					//LOG_E("uart queue msg send failed!\n");
				}
			}
			g_uart_Req.Size = 0;
			//memset((void *)&g_uart_Req, 0, sizeof(BootPkt_T));
		}
    }
    if (RESET != usart_interrupt_flag_get(COM_NUM, USART_INT_FLAG_TC))
    {
        usart_interrupt_flag_clear(COM_NUM, USART_INT_FLAG_TC);
        /* send data continue */
		UART_sendFinally(COM_NUM, &g_UARTPara.fifo);
    } 
}
