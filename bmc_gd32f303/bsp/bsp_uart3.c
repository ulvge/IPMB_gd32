#include "bsp_uart3.h"
#include <string.h>
#include "main.h"
#include "mcro_def.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"

void com3_init()
{
    /* enable GPIO clock */
    rcu_periph_clock_enable(COM3_GPIO_CLK);

    /* enable USART clock */
    rcu_periph_clock_enable(COM3_CLK);
    rcu_periph_clock_enable(RCU_AF);

    /* connect port to USARTx_Tx */
    gpio_init(COM3_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, COM3_TX_PIN);
    /* connect port to USARTx_Rx */
    gpio_init(COM3_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, COM3_RX_PIN);

    /* USART configure */
    usart_deinit(COM3);
    usart_baudrate_set(COM3, 115200U);
    usart_word_length_set(COM3, USART_WL_8BIT);
    usart_stop_bit_set(COM3, USART_STB_1BIT);
    usart_parity_config(COM3, USART_PM_NONE);
    usart_hardware_flow_rts_config(COM3, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(COM3, USART_CTS_DISABLE);
    usart_receive_config(COM3, USART_RECEIVE_ENABLE);
    usart_transmit_config(COM3, USART_TRANSMIT_ENABLE);
    usart_enable(COM3);

    /* USART interrupt configuration */
    nvic_irq_enable(UART3_IRQn, 5, 0);
    /* enable USART TBE interrupt */
    usart_interrupt_enable(COM3, USART_INT_RBNE);
}

#ifdef USE_UART3_AS_IPMI
extern xQueueHandle RecvDatMsg_Queue;
static MsgPkt_T    g_uart_Req;
#endif

void UART3_IRQHandler(void)
{
    uint8_t res;

#ifdef USE_UART3_AS_IPMI
    BaseType_t err;
    static bool is_start = false;
    static BaseType_t xHigherPriorityTaskWoken;  // must set xHigherPriorityTaskWoken as a static variable, why?

    if (RESET != usart_interrupt_flag_get(COM3, USART_INT_FLAG_RBNE))
    {
        /* receive data */
        res = usart_data_receive(COM3);
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
            g_uart_Req.Data[g_uart_Req.Size++] = usart_data_receive(COM3);
            if (g_uart_Req.Size > sizeof(g_uart_Req.Data) - 3)
            {
                is_start = false;
                g_uart_Req.Size = 0;
                //    LOG_E("uart recv overlap!");
            }
        }
    }
#else
    if (RESET != usart_interrupt_flag_get(COM3, USART_INT_FLAG_RBNE))
    {
        /* receive data */
        res = usart_data_receive(COM3);
        res = res;
    }

#endif
}

void uart3_send_dat(uint8_t *str, uint16_t len)
{
    int i = 0;
    for (i = 0; i < len; i++)
    {
        usart_data_transmit(COM3, str[i]);
        while (RESET == usart_flag_get(COM3, USART_FLAG_TBE))
            ;
    }
}

void uart3_send_string(char *str)
{
    unsigned int k = 0;
    do
    {
        usart_data_transmit(COM3, *(str + k));
        while (RESET == usart_flag_get(COM3, USART_FLAG_TBE))
            ;
        k++;
    } while (*(str + k) != '\0');
}

bool uart3_get_data(uint8_t *p_buffer, uint32_t *len)
{
    // if (!g_uart_rx_is_updated)
    // {
    //     return false;
    // }
    // memcpy(p_buffer, rxbuffer, g_rxcount);
    // *len = g_rxcount;
    // g_uart_rx_is_updated = false; // has read

    return false;
}
