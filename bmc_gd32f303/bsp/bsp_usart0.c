#include "bsp_usart0.h"
#include <string.h>
#include "main.h"
#include "mcro_def.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"

#ifdef USE_UART0_DEBUG
	/* retarget the C library printf function to the USART */
	int fputc(int ch, FILE *f)
	{
			usart_data_transmit(USART0, (uint8_t)ch);
			while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
					;

			return ch;
	}
#elif USE_UART1_DEBUG
//	int fputc(int ch, FILE *f)
//	{
//			usart_data_transmit(USART1, (uint8_t)ch);
//			while (RESET == usart_flag_get(USART1, USART_FLAG_TBE))
//					;

//			return ch;
//	}
#elif USE_UART3_DEBUG
	int fputc(int ch, FILE *f)
	{
			usart_data_transmit(UART3, (uint8_t)ch);
			while (RESET == usart_flag_get(UART3, USART_FLAG_TBE))
					;

			return ch;
	}
#elif USE_UART7_DEBUG
	int fputc(int ch, FILE *f)
	{
			usart_data_transmit(UART7, (uint8_t)ch);
			while (RESET == usart_flag_get(UART7, USART_FLAG_TBE))
					;

			return ch;
	}
#endif

void com0_init()
{
    /* enable GPIO clock */
    rcu_periph_clock_enable(COM0_GPIO_CLK);

    /* enable USART clock */
    rcu_periph_clock_enable(COM0_CLK);

    /* connect port to USARTx_Tx */
    gpio_init(COM0_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, COM0_TX_PIN);

    /* connect port to USARTx_Rx */
    gpio_init(COM0_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, COM0_RX_PIN);

    /* USART configure */
    usart_deinit(COM0);
    usart_baudrate_set(COM0, 115200U);
    usart_word_length_set(COM0, USART_WL_8BIT);
    usart_stop_bit_set(COM0, USART_STB_1BIT);
    usart_parity_config(COM0, USART_PM_NONE);
    usart_hardware_flow_rts_config(COM0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(COM0, USART_CTS_DISABLE);
    usart_receive_config(COM0, USART_RECEIVE_ENABLE);
    usart_transmit_config(COM0, USART_TRANSMIT_ENABLE);
    usart_enable(COM0);

    /* USART interrupt configuration */
    nvic_irq_enable(USART0_IRQn, 10, 0);
    /* enable USART TBE interrupt */
    usart_interrupt_enable(COM0, USART_INT_RBNE);
}

#ifdef USE_UART0_AS_IPMI
extern xQueueHandle RecvDatMsg_Queue;
static MsgPkt_T    g_uart_Req;
#else 
extern QueueHandle_t uart_queue;
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
        res = usart_data_receive(COM0);
				if(uart_queue)
				{
					xQueueSendFromISR(uart_queue, &res, &xHigherPriorityTaskWoken);
					portYIELD_FROM_ISR(xHigherPriorityTaskWoken);				
				}
    }
#endif
}

void uart0_send_byte(char dat)
{
		usart_data_transmit(COM0, dat);
		while (RESET == usart_flag_get(COM0, USART_FLAG_TBE));
}

void uart0_send_dat(uint8_t *str, uint16_t len)
{
    int i = 0;
    for (i = 0; i < len; i++)
    {
        usart_data_transmit(COM0, str[i]);
        while (RESET == usart_flag_get(COM0, USART_FLAG_TBE))
            ;
    }
}

void uart0_send_string(uint8_t *str)
{
    unsigned int k = 0;
    do
    {
        usart_data_transmit(USART0, *(str + k));
        while (RESET == usart_flag_get(COM0, USART_FLAG_TBE))
            ;
        k++;
    } while (*(str + k) != '\0');
}

bool uart0_get_data(uint8_t *p_buffer, uint32_t *len)
{
    // //   usart_interrupt_enable(com0, USART_INT_RBNE);
    // if (!g_uart_rx_is_updated)
    // {
    //     return false;
    // }
    // memcpy(p_buffer, g_uart_rxbuffer, g_uart_rxcount);
    // *len = g_uart_rxcount;
    // g_uart_rxcount = 0;
    // g_uart_rx_is_updated = false;
    // //  usart_interrupt_disable(com0, USART_INT_RBNE);

    return false;
}
