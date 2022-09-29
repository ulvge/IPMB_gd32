#include "bsp_uart7.h"
#include <string.h>
#include "main.h"
#include "mcro_def.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"
#include "FIFO.h"
#include "bsp_uartcomm.h"

#define UART7_BUFF_SIZE 	(50)
static INT8U g_buffSend[UART7_BUFF_SIZE];
static INT8U g_buffRec[UART7_BUFF_SIZE];
static const UART_CONFIG_STRUCT g_uart7Config= {
    .baud = 115200U,
    .irqN = UART7_IRQn,
    .prePriority = 5,
    .subPriority = 0,
    
    .txPort = COM7_GPIO_PORT,
    .txPin = COM7_TX_PIN,
    .txPinMode = GPIO_MODE_AF_PP,
    .txPinSpeed = GPIO_OSPEED_50MHZ,

    .rxPort = COM7_GPIO_PORT,
    .rxPin = COM7_RX_PIN,
    .rxPinMode = GPIO_MODE_IN_FLOATING,
    .rxPinSpeed = GPIO_OSPEED_50MHZ,

    .rcuUart = COM7_CLK,
    .rcuGPIO = COM7_GPIO_CLK,
};
static UART_PARA_STRUCT g_UARTPara = {
    .usart_periph = COM7,  
    .config = &g_uart3Config,
};

void UART3_init(void)
{
	FIFO_Init(&g_UARTPara.fifo.sfifo, g_buffSend, sizeof(g_buffSend));	
	FIFO_Init(&g_UARTPara.fifo.rfifo, g_buffRec, sizeof(g_buffRec));
    COM_init(&g_UARTPara);
}

#ifdef USE_UART7_COM
static MsgPkt_T    g_uart_Req;
#endif

#ifdef USE_UART7_AS_IPMI
extern xQueueHandle RecvDatMsg_Queue;
#elif USE_UART7_COM
extern xQueueHandle FTUartRead_Queue;
#endif


void UART7_IRQHandler(void)
{
    uint8_t res;
#ifdef USE_UART7_COM

    BaseType_t err;
    static bool is_start = false;
    static BaseType_t xHigherPriorityTaskWoken;  // must set xHigherPriorityTaskWoken as a static variable, why?

    if (RESET != usart_interrupt_flag_get(COM7, USART_INT_FLAG_RBNE))
    {
        /* receive data */
        res = usart_data_receive(COM7);
        if (res == START_BYTE)
        { // start
            is_start = true;
            g_uart_Req.Size = 0;
        }
        else if (res == STOP_BYTE && is_start == true)
        { // stop
            is_start = false;
            // usart_data_transmit(USART1, HAND_SHAKE_BYTE); // BMC hand shake
#ifdef USE_UART7_AS_IPMI
            // usart_data_transmit(USART1, HAND_SHAKE_BYTE); // BMC hand shake
            if (RecvDatMsg_Queue != NULL)
            {
                g_uart_Req.Param = SERIAL_REQUEST;
                err = xQueueSendFromISR(RecvDatMsg_Queue, (char*)&g_uart_Req, &xHigherPriorityTaskWoken);
                if (err == pdFAIL)
                {
                    LOG_E("uart7 IPMI msg send failed!");
                }
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
#elif USE_UART7_AS_FT_COM
            if (FTUartRead_Queue != NULL)
            {
                g_uart_Req.Param = FTCPU_RESPONSE;
                err = xQueueSendFromISR(FTUartRead_Queue, (char*)&g_uart_Req, &xHigherPriorityTaskWoken);
                if (err == pdFAIL)
                {
                    LOG_E("uart7 FT_COM msg send failed!");
                }
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
#endif
        }
        else
        {
            g_uart_Req.Data[g_uart_Req.Size++] = usart_data_receive(COM7);
            if (g_uart_Req.Size > sizeof(g_uart_Req.Data) - 3)
            {
                is_start = false;
                g_uart_Req.Size = 0;
                //    LOG_E("uart recv overlap!");
            }
        }
    }
#else
    if (RESET != usart_interrupt_flag_get(COM7, USART_INT_FLAG_RBNE))
    {
        /* receive data */
        res = usart_data_receive(COM7);
        res = res;
    }

#endif
}
