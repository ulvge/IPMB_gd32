#include "bsp_usart1.h"
#include <string.h>
#include "main.h"
#include "mcro_def.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"

#define USART1_DATA_ADDRESS    ((uint32_t)&USART_DATA(USART1))

static void uart1_dma_init(void);

void com1_init()
{
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

#if 1
    uart1_dma_init();
#else
    /* USART interrupt configuration */
    nvic_irq_enable(USART1_IRQn, 3, 0);
    /* enable USART TBE interrupt */
    usart_interrupt_enable(COM1, USART_INT_RBNE);
#endif
}

#ifdef USE_UART1_COM
static MsgPkt_T    g_uart_Req;
#endif

#if USE_UART1_AS_IPMI
extern xQueueHandle RecvDatMsg_Queue;
#elif USE_UART1_COM
extern xQueueHandle FTUartRead_Queue;
#endif

#if 0
void USART1_IRQHandler(void)
{
    uint8_t res;
    
#ifdef USE_UART1_COM
	
    BaseType_t err;
    static bool is_start = false;
    static BaseType_t xHigherPriorityTaskWoken;  // must set xHigherPriorityTaskWoken as a static variable, why?

    if (RESET != usart_interrupt_flag_get(COM1, USART_INT_FLAG_RBNE))
    {
        /* receive data */
        res = usart_data_receive(COM1);
        if (res == START_BYTE)
        { // start
            is_start = true;
            g_uart_Req.Size = 0;
        }
        else if (res == STOP_BYTE && is_start == true)
        { // stop
            is_start = false;
#ifdef USE_UART1_AS_IPMI
            // usart_data_transmit(USART1, HAND_SHAKE_BYTE); // BMC hand shake
            if (RecvDatMsg_Queue != NULL)
            {
                g_uart_Req.Param = SERIAL_REQUEST;
                err = xQueueSendFromISR(RecvDatMsg_Queue, (char*)&g_uart_Req, &xHigherPriorityTaskWoken);
                if (err == pdFAIL)
                {
                    LOG_E("uart1 IPMI msg send failed!");
                }
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
#elif USE_UART1_AS_FT_COM
            if (FTUartRead_Queue != NULL)
            {
                g_uart_Req.Param = FTCPU_RESPONSE;
                err = xQueueSendFromISR(FTUartRead_Queue, (char*)&g_uart_Req, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                if (err == pdFAIL)
                {
                    LOG_E("uart1 FT_COM msg send failed!");
                }
            }
#endif
        }
        else
        {
            g_uart_Req.Data[g_uart_Req.Size++] = usart_data_receive(COM1);
            if (g_uart_Req.Size > sizeof(g_uart_Req.Data) - 3)
            {
                is_start = false;
                g_uart_Req.Size = 0;
                LOG_E("uart recv overlap!");
            }
        }
    }
#else
    if (RESET != usart_interrupt_flag_get(COM1, USART_INT_FLAG_RBNE))
    {
        /* receive data */
        res = usart_data_receive(COM1);
        res = res;
    }

#endif
}

#endif

void uart1_send_dat(uint8_t *str, uint16_t len)
{
    int i = 0;
    for (i = 0; i < len; i++)
    {
        usart_data_transmit(USART1, str[i]);
        while (RESET == usart_flag_get(USART1, USART_FLAG_TBE))
            ;
    }
}

void uart1_send_string(char *str)
{
    unsigned int k = 0;
    do
    {
        usart_data_transmit(USART1, *(str + k));
        while (RESET == usart_flag_get(USART1, USART_FLAG_TBE))
            ;
        k++;
    } while (*(str + k) != '\0');
}

bool uart1_get_data(uint8_t *p_buffer, uint32_t *len)
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
