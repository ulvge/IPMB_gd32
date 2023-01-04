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


static INT8U g_UART1BuffSend[UART1_BUFF_SIZE];
static INT8U g_UART1BuffRec[UART1_BUFF_SIZE];

static const UART_CONFIG_STRUCT g_uart1Config = {
    .baud = 115200U,
    .irqN = USART1_IRQn,
    .prePriority = 10,
    .subPriority = 0,

    .txPort = COM1_GPIO_PORT,
    .txPin = COM1_TX_PIN,
    .txPinMode = GPIO_MODE_AF_PP,
    .txPinSpeed = GPIO_OSPEED_50MHZ,

    .rxPort = COM1_GPIO_PORT,
    .rxPin = COM1_RX_PIN,
    .rxPinMode = GPIO_MODE_IPU,
    .rxPinSpeed = GPIO_OSPEED_50MHZ,

    .rcuUart = COM1_CLK,
    .rcuGPIO = COM1_GPIO_CLK,
    .remap = GPIO_USART1_REMAP,
};
static UART_PARA_STRUCT g_UART1Para = {
    .usart_periph = COM1,
    .config = &g_uart1Config,
};

void UART1_init()
{
    FIFO_Init(&g_UART1Para.fifo.sfifo, g_UART1BuffSend, sizeof(g_UART1BuffSend));
    FIFO_Init(&g_UART1Para.fifo.rfifo, g_UART1BuffRec, sizeof(g_UART1BuffRec));
    COM_init(&g_UART1Para);
}

#ifndef BOOTLOADER
static SamllMsgPkt_T g_uart_Req;
extern xQueueHandle CPU_recvDatMsg_Queue;
#endif

void USART1_IRQHandler(void)
{
#define COM_NUM    COM1
    uint8_t res;
#ifdef BOOTLOADER
    if (RESET != usart_interrupt_flag_get(COM_NUM, USART_INT_FLAG_RBNE)) {
        res = usart_data_receive(COM_NUM);
        usart_interrupt_flag_clear(COM_NUM, USART_INT_FLAG_RBNE);
        UNUSED(res);
    }
#else
    static BaseType_t xHigherPriorityTaskWoken; // must set xHigherPriorityTaskWoken as a static variable, why?
    static bool is_start = false;
    if (RESET != usart_interrupt_flag_get(COM_NUM, USART_INT_FLAG_RBNE)) {
        res = usart_data_receive(COM_NUM);
        usart_interrupt_flag_clear(COM_NUM, USART_INT_FLAG_RBNE);

        /* receive data */
        if (res == START_BYTE) { // start
            is_start = true;
            g_uart_Req.Size = 0;
        } else if (res == STOP_BYTE && is_start == true) { // stop
            is_start = false;
            // usart_data_transmit(USART1, HAND_SHAKE_BYTE); // BMC hand shake
            if (CPU_recvDatMsg_Queue != NULL) {
                g_uart_Req.Param = SERIAL_REQUEST;
                BaseType_t err = xQueueSendFromISR(CPU_recvDatMsg_Queue, (char *)&g_uart_Req, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                if (err == pdFAIL) {
                    // LOG_E("uart queue msg send failed!");
                }
            }
        } else {
            g_uart_Req.Data[g_uart_Req.Size++] = res;
            if (g_uart_Req.Size > (sizeof(g_uart_Req.Data) - 3)) {
                is_start = false;
                g_uart_Req.Size = 0;
                // LOG_E("uart recv overlap!");
            }
        }

        // use FIFO store all
        if (is_start == false) {
            /* receive data */
            // UART_sendByte(COM_NUM, res);  //loopback
            // FIFO_Write(&g_UART1Para.fifo.rfifo, (INT8U)res); // only save
        }
    }
#endif

    if (RESET != usart_interrupt_flag_get(COM_NUM, USART_INT_FLAG_TC)) {
        usart_interrupt_flag_clear(COM_NUM, USART_INT_FLAG_TC);
        /* send data continue */
        UART_sendFinally(COM_NUM, &g_UART1Para.fifo);
    }
}
