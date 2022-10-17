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

static const UART_CONFIG_STRUCT g_uart0Config= {
    .baud = 115200U,
    .irqN = USART0_IRQn,
    .prePriority = 10,
    .subPriority = 0,
    
    .txPort = GPIOD,
    .txPin = COM0_TX_PIN,
    .txPinMode = GPIO_MODE_AF_PP,
    .txPinSpeed = GPIO_OSPEED_50MHZ,

    .rxPort = GPIOD,
    .rxPin = COM0_RX_PIN,
    .rxPinMode = GPIO_MODE_IN_FLOATING,
    .rxPinSpeed = GPIO_OSPEED_50MHZ,

    .rcuUart = COM0_CLK,
    .rcuGPIO = COM0_GPIO_CLK,
    .remap = GPIO_USART0_REMAP,
};
static UART_PARA_STRUCT g_UARTPara = {
    .usart_periph = COM0,  
    .config = &g_uart0Config,
};

#define UART0_BUFF_SIZE 	(50)
static INT8U g_buffSend[UART0_BUFF_SIZE];	 
static INT8U g_buffRec[UART0_BUFF_SIZE];

void UART0_init(void)
{
	FIFO_Init(&g_UARTPara.fifo.sfifo, g_buffSend, sizeof(g_buffSend));	
	FIFO_Init(&g_UARTPara.fifo.rfifo, g_buffRec, sizeof(g_buffRec));
    COM_init(&g_UARTPara);
}
#ifdef USE_UART0_AS_IPMI
extern xQueueHandle RecvDatMsg_Queue;
static MsgPkt_T    g_uart_Req;
#endif

void USART0_IRQHandler(void)
{
    uint8_t res;
    static BaseType_t xHigherPriorityTaskWoken;  // must set xHigherPriorityTaskWoken as a static variable, why?

#ifdef USE_UART0_AS_IPMI

    BaseType_t err;
    static bool is_start = false;

    if (RESET != usart_interrupt_flag_get(COM0, USART_INT_FLAG_RBNE))
    {
        /* receive data */
        res = usart_data_receive(COM0);
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
            g_uart_Req.Data[g_uart_Req.Size++] = usart_data_receive(COM0);
            if (g_uart_Req.Size > sizeof(g_uart_Req.Data) - 3)
            {
                is_start = false;
                g_uart_Req.Size = 0;
                //    LOG_E("uart recv overlap!");
            }
        }
    }
#else
    if (RESET != usart_interrupt_flag_get(COM0, USART_INT_FLAG_RBNE))
    {
        /* receive data */
    }
#endif
}
