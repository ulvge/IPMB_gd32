/*!

    Copyright (c) 2020, ZK Inc.

    All rights reserved.
*/

#include "bsp_i2c.h"
#include "project_select.h"
#include <string.h>
#include "main.h"
#include "OSPort.h"
#include "Message.h"
#include "MsgHndlr.h"
#include "IPMDevice.h"

#define I2C0_INTERRUPT_ENALBE
static void i2c0_int(void);
static void i2c1_int(void);

static void i2c0_config(void);
#ifdef  I2C0_INTERRUPT_ENALBE
static void i2c0_nvic_config(void);
#endif
static void i2c0_gpio_config(void);
static void i2c0_rcu_config(void);

static void i2c1_config(void);
#ifdef  I2C1_INTERRUPT_ENALBE
static void i2c1_nvic_config(void);
#endif
static void i2c1_gpio_config(void);
static void i2c1_rcu_config(void);
#ifdef I2C2
static void i2c2_int(void);
static void i2c2_config(void);
static void i2c2_nvic_config(void);
static void i2c2_gpio_config(void);
static void i2c2_rcu_config(void);    
#endif

static bool i2c_bytes_write(uint32_t i2cx, uint8_t device_addr, const uint8_t *p_buffer, uint16_t len, int time_out);
static bool i2c_bytes_read(uint32_t i2cx, uint8_t device_addr, uint8_t read_addr, uint8_t *p_buffer, uint16_t len, int time_out);
static void i2c_set_as_slave_device_addr(uint32_t i2c_periph, uint8_t device_addr);
		
uint8_t g_device0_addr=0;
uint8_t g_device1_addr=0;
uint8_t g_device2_addr=0;
void i2c_int(void)
{
    i2c_channel_init(I2C0);
    i2c_channel_init(I2C1);
#ifdef I2C2
    i2c_channel_init(I2C2); 
#endif
}

void i2c_channel_init(uint32_t i2cx)
{
    switch(i2cx)
    {
    case I2C0:
        i2c0_int();
        break;
    case I2C1:
        i2c1_int();
        break;
#ifdef I2C2
    case I2C2:
        i2c2_int();
        LOG_I("i2c2_int");
        break;
#endif
    default:
        break;
    }
}

static void i2c0_int(void)
{
    i2c0_rcu_config();
    i2c0_gpio_config();
#ifdef  I2C0_INTERRUPT_ENALBE
    i2c0_nvic_config();
#endif
    i2c0_config();
}

static void i2c1_int(void)
{
    i2c1_rcu_config();
    i2c1_gpio_config();
#ifdef  I2C1_INTERRUPT_ENALBE
    i2c1_nvic_config();
#endif
    i2c1_config();
}

static void i2c0_rcu_config(void)
{
    /* enable I2C0 clock */
    rcu_periph_clock_enable(RCU_I2C0);

    /* enable GPIO clock */
    rcu_periph_clock_enable(I2C0_SCL_GPIO_CLK);
    rcu_periph_clock_enable(I2C0_SDA_GPIO_CLK);
}

static void i2c1_rcu_config(void)
{
    /* enable I2C1 clock */
    rcu_periph_clock_enable(RCU_I2C1);

    /* enable GPIO clock */
    rcu_periph_clock_enable(I2C1_SCL_GPIO_CLK);
    rcu_periph_clock_enable(I2C1_SDA_GPIO_CLK);
}


/*!
    \brief      cofigure the GPIO ports.
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void i2c0_gpio_config(void)
{
    /* I2C0 and I2C1 GPIO ports */
    /* connect I2C0_SCL I2C0_SDA*/
    gpio_init(I2C0_SCL_GPIO_PORT, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C0_SCL_PIN);
    gpio_init(I2C0_SDA_GPIO_PORT, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C0_SDA_PIN);
#ifdef I2C0_REMAP
    gpio_pin_remap_config(GPIO_I2C0_REMAP, ENABLE);
#endif
}

/*!
    \brief      cofigure the GPIO ports.
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void i2c1_gpio_config(void)
{
    /* connect I2C1_SCL I2C1_SDA*/
    gpio_init(I2C1_SCL_GPIO_PORT, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C1_SCL_PIN);
    gpio_init(I2C1_SDA_GPIO_PORT, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C1_SDA_PIN);
#ifdef I2C1_REMAP
	#ifdef GD32F2x
    gpio_pin_remap1_config(GPIO_PCF5, GPIO_PCF5_I2C1_REMAP1, ENABLE);
	#endif
#endif
}


/*!
    \brief      cofigure the I2C0 and I2C1 interfaces..
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void i2c0_config(void)
{
    // **************************** I2C0 *********************************************
    /* I2C clock configure */
    i2c_clock_config(I2C0, 100000, I2C_DTCY_2);
    /* I2C address configure */
    i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C0_SLAVE_ADDRESS7);
    /* enable I2C0 */
    i2c_enable(I2C0);
    /* enable acknowledge */
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);

#ifdef  I2C0_INTERRUPT_ENALBE
    /* enable the I2C0 interrupt */
    i2c_interrupt_enable(I2C0, I2C_INT_ERR);
    i2c_interrupt_enable(I2C0, I2C_INT_EV);
    i2c_interrupt_enable(I2C0, I2C_INT_BUF);
#endif
}

/*!
    \brief      cofigure the I2C0 and I2C1 interfaces..
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void i2c1_config(void)
{
    // **************************** I2C1 *********************************************
    i2c_clock_config(I2C1, 100000, I2C_DTCY_2);
    /* I2C address configure */
    i2c_mode_addr_config(I2C1, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C1_SLAVE_ADDRESS7);
    /* enable I2C1 */
    i2c_enable(I2C1);
    /* enable acknowledge */
    i2c_ack_config(I2C1, I2C_ACK_ENABLE);

#ifdef  I2C1_INTERRUPT_ENALBE
    /* enable the I2C0 interrupt */
    i2c_interrupt_enable(I2C1, I2C_INT_ERR);
    i2c_interrupt_enable(I2C1, I2C_INT_EV);
    i2c_interrupt_enable(I2C1, I2C_INT_BUF);
#endif
}


#ifdef  I2C0_INTERRUPT_ENALBE
/*!
    \brief      cofigure the NVIC peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void i2c0_nvic_config(void)
{
    nvic_irq_enable(I2C0_EV_IRQn, 3, 0);
    nvic_irq_enable(I2C0_ER_IRQn, 8, 0);
}
#endif

#ifdef  I2C1_INTERRUPT_ENALBE
/*!
    \brief      cofigure the NVIC peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void i2c1_nvic_config(void)
{
    nvic_irq_enable(I2C1_EV_IRQn, 3, 0);
    nvic_irq_enable(I2C1_ER_IRQn, 8, 0);
}
#endif


/**
  * @brief   
  * @param   
  *	@arg p_buffer: 
  *	@arg 
  * @arg len:
  * @retval  ?
  */
static bool i2c_bytes_write(uint32_t i2cx, uint8_t device_addr, const uint8_t *p_buffer, uint16_t len, int time_out)
{
    int i = 0;
    int timeout = time_out;
    int err_code = 0;

    /* disable I2C0 interrupt */
    i2c_interrupt_disable(i2cx, I2C_INT_BUF);
    i2c_interrupt_disable(i2cx, I2C_INT_EV);

    taskENTER_CRITICAL();
    /* wait until I2C bus is idle */
    while (i2c_flag_get(i2cx, I2C_FLAG_I2CBSY))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 1;
            goto __exit;  
        }
    }

    timeout = time_out;
    /* send a start condition to I2C bus */
    i2c_start_on_bus(i2cx);
    /* wait until SBSEND bit is set */
    while (!i2c_flag_get(i2cx, I2C_FLAG_SBSEND))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 2;
            goto __exit; 
        }
    }

    timeout = time_out;
    /* send slave address to I2C bus */
    i2c_master_addressing(i2cx, device_addr, I2C_TRANSMITTER);
    /* wait until ADDSEND bit is set */
    while (!i2c_flag_get(i2cx, I2C_FLAG_ADDSEND))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 3;
            goto __exit; 
        }
    }
    /* clear ADDSEND bit */
    i2c_flag_clear(i2cx, I2C_FLAG_ADDSEND);
    timeout = time_out;
    /* wait until the transmit data buffer is empty */
    while (!i2c_flag_get(i2cx, I2C_FLAG_TBE))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 4;
            goto __exit; 
        }
    }

    for (i = 0; i < len; i++)
    {
        timeout = time_out;
        /* data transmission */
        i2c_data_transmit(i2cx, p_buffer[i]);
        /* wait until the TBE bit is set */
        while (!i2c_flag_get(i2cx, I2C_FLAG_TBE))
        {
            timeout--;
            if (timeout == 0)
            {
                err_code = 5;
                goto __exit; 
            }
        }
    }
    /* send a stop condition to I2C bus */
    timeout = time_out;
    i2c_stop_on_bus(i2cx);
    while (I2C_CTL0(i2cx) & 0x0200)
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 6;
            goto __exit;  
        }
    }

    /* enable I2C0 interrupt */
    i2c_interrupt_enable(i2cx, I2C_INT_BUF);
    i2c_interrupt_enable(i2cx, I2C_INT_EV);

    taskEXIT_CRITICAL();

    return true;

__exit:
    i2c_deinit(i2cx);
    i2c_channel_init(i2cx);
    taskEXIT_CRITICAL();
    LOG_E("I2C write err code: %d.", err_code);
    return false;
}

/**
  * @brief   
  * @param   
  *	@arg p_buffer: 
  *	@arg 
  * @arg len:
  * @retval  ?
  */
static bool i2c_bytes_read(uint32_t i2cx, uint8_t device_addr, uint8_t read_addr, uint8_t *p_buffer, uint16_t len, int time_out)
{ 
    int timeout = time_out; 
    int err_code = 0;

    /* wait until I2C bus is idle */
    while (i2c_flag_get(i2cx, I2C_FLAG_I2CBSY))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 1;
            goto __exit;
        }
    }

    if(2 == len){
        i2c_ackpos_config(i2cx,I2C_ACKPOS_NEXT);
    }
    
    timeout = time_out;
    /* send a start condition to I2C bus */
    i2c_start_on_bus(i2cx);
    /* wait until SBSEND bit is set */
    while (!i2c_flag_get(i2cx, I2C_FLAG_SBSEND))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 2;
            goto __exit;
        }
    }
    
    timeout = time_out;
    /* send slave address to I2C bus */
    i2c_master_addressing(i2cx, device_addr, I2C_TRANSMITTER);
    /* wait until ADDSEND bit is set */
    while (!i2c_flag_get(i2cx, I2C_FLAG_ADDSEND))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 3;
            goto __exit;
        }
    }
    
    /* clear ADDSEND bit */
    i2c_flag_clear(i2cx, I2C_FLAG_ADDSEND);
    timeout = time_out;
    /* wait until the transmit data buffer is empty */
    while (!i2c_flag_get(i2cx, I2C_FLAG_TBE))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 4;
            goto __exit;
        }
    }

    /* enable I2C0*/
    i2c_enable(i2cx);

    timeout = time_out;
    /* send the internal address to write to */
    i2c_data_transmit(i2cx, read_addr);
    /* wait until the BTC bit is set */
    while (!i2c_flag_get(i2cx, I2C_FLAG_BTC))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 5;
            goto __exit;
        }
    }

    timeout = time_out;
    /* send a start condition to I2C bus */
    i2c_start_on_bus(i2cx);
    /* wait until SBSEND bit is set */
    while (!i2c_flag_get(i2cx, I2C_FLAG_SBSEND))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 6;
            goto __exit;
        }
    }
    
    /* send slave address to I2C bus */
    i2c_master_addressing(i2cx, device_addr, I2C_RECEIVER);

    if(len < 3){
        /* disable acknowledge */
        i2c_ack_config(i2cx,I2C_ACK_DISABLE);
    }
    
    timeout = time_out;
    /* wait until ADDSEND bit is set */
    while (!i2c_flag_get(i2cx, I2C_FLAG_ADDSEND))
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 7;
            goto __exit;
        }
    }
    /* clear the ADDSEND bit */
    i2c_flag_clear(i2cx,I2C_FLAG_ADDSEND);
    
    if(1 == len){
        /* send a stop condition to I2C bus */
        i2c_stop_on_bus(i2cx);
    }
    
    /* while there is data to be read */
    while(len){
        if(3 == len){
            timeout = time_out;
            /* wait until the BTC bit is set */
            while (!i2c_flag_get(i2cx, I2C_FLAG_BTC))
            {
                timeout--;
                if (timeout == 0)
                {
                    err_code = 8;
                    goto __exit;
                }
            }
            /* disable acknowledge */
            i2c_ack_config(i2cx,I2C_ACK_DISABLE);
        }
        else if(2 == len){
            timeout = time_out;
            /* wait until the BTC bit is set */
            while (!i2c_flag_get(i2cx, I2C_FLAG_BTC))
            {
                timeout--;
                if (timeout == 0)
                {
                    err_code = 9;
                    goto __exit;
                }
            }
            /* send a stop condition to I2C bus */
            i2c_stop_on_bus(i2cx);
        }
        
        /* wait until the RBNE bit is set and clear it */
        if(i2c_flag_get(i2cx, I2C_FLAG_RBNE)){
            /* read a byte from the EEPROM */
            *p_buffer = i2c_data_receive(i2cx);
            /* point to the next location where the byte read will be saved */
            p_buffer++; 
            /* decrement the read bytes counter */
            len--;
        } 
    }
    
    timeout = time_out;
    /* wait until the stop condition is finished */
    while (I2C_CTL0(i2cx) & 0x0200)
    {
        timeout--;
        if (timeout == 0)
        {
            err_code = 10;
            goto __exit;
        }
    }
    /* enable acknowledge */
    i2c_ack_config(i2cx,I2C_ACK_ENABLE);
    i2c_ackpos_config(i2cx,I2C_ACKPOS_CURRENT);
		
    return true;

__exit:
    i2c_deinit(i2cx);
    i2c_channel_init(i2cx);
    LOG_E("I2C write & read err code: %d.", err_code);
    return false;
}


bool i2c0_bytes_write(const uint8_t *p_buffer, uint16_t len)
{
    return i2c_bytes_write(I2C0, p_buffer[0], &p_buffer[1], len-1, 20000);
}

bool i2c1_bytes_write(const uint8_t *p_buffer, uint16_t len)
{
    return i2c_bytes_write(I2C1, p_buffer[0], &p_buffer[1], len-1, 20000);
}

bool i2c0_bytes_read(const uint8_t device_addr, const uint8_t read_addr, uint8_t* p_buffer, uint16_t len)
{
    return i2c_bytes_read(I2C0, device_addr, read_addr, p_buffer, len, 1000);
}

bool i2c1_bytes_read(const uint8_t device_addr, const uint8_t read_addr, uint8_t* p_buffer, uint16_t len)
{
    return i2c_bytes_read(I2C1, device_addr, read_addr, p_buffer, len, 10000);
}

extern xQueueHandle RecvDatMsg_Queue;
extern xQueueHandle RecvForwardI2CDatMsg_Queue;
static MsgPkt_T    g_i2c_Req;

#ifdef  I2C0_INTERRUPT_ENALBE
/*!
    \brief      handle I2C0 event interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/

void I2C0_EV_IRQHandler(void)
{
#ifdef USE_I2C0_AS_IPMB
    BaseType_t err;
    static BaseType_t xHigherPriorityTaskWoken;

    if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_ADDSEND))
    {
        /* clear the ADDSEND bit */
        i2c_interrupt_flag_clear(I2C0, I2C_INT_FLAG_ADDSEND);
        g_i2c_Req.Size = 0;; // start, clear recv count
        g_i2c_Req.Data[g_i2c_Req.Size++] = GetDevAddr();
    }
    else if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_RBNE))
    {
        /* if reception data register is not empty ,I2C0 will read a data from I2C_DATA */
        g_i2c_Req.Data[g_i2c_Req.Size++] = i2c_data_receive(I2C0);
    }
    else if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_STPDET))
    {
        /* clear the STPDET bit */
        i2c_enable(I2C0);
        g_i2c_Req.Param = IPMI_REQUEST;
        if(RecvForwardI2CDatMsg_Queue != NULL && RecvDatMsg_Queue != NULL)
        {
            if((g_i2c_Req.Data[1] & 0x04) == 0) // netFn even
            {
                err = xQueueSendFromISR(RecvDatMsg_Queue, (char*)&g_i2c_Req, &xHigherPriorityTaskWoken);
            }
            else  // odd
            {
                err = xQueueSendFromISR(RecvForwardI2CDatMsg_Queue, (char*)&g_i2c_Req, &xHigherPriorityTaskWoken);
            }

            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            if (err == pdFAIL)
            {
                LOG_E("iic xQueueSendFromISR failed!");
            }
        }
    }

#else
    if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_ADDSEND))
    {
        /* clear the ADDSEND bit */
        i2c_interrupt_flag_clear(I2C0, I2C_INT_FLAG_ADDSEND);
    }
    else if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_RBNE))
    {
        /* if reception data register is not empty ,I2C0 will read a data from I2C_DATA */
        //*i2c_rxbuffer++ = i2c_data_receive(I2C0);
    }
    else if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_STPDET))
    {
        /* clear the STPDET bit */
        i2c_enable(I2C0);
    }

#endif
}

/*!
    \brief      handle I2C0 error interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void I2C0_ER_IRQHandler(void)
{
    /* no acknowledge received */
    if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_AERR))
    {
        i2c_interrupt_flag_clear(I2C0, I2C_INT_FLAG_AERR);
    }

    /* SMBus alert */
    if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_SMBALT))
    {
        i2c_interrupt_flag_clear(I2C0, I2C_INT_FLAG_SMBALT);
    }

    /* bus timeout in SMBus mode */
    if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_SMBTO))
    {
        i2c_interrupt_flag_clear(I2C0, I2C_INT_FLAG_SMBTO);
    }

    /* over-run or under-run when SCL stretch is disabled */
    if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_OUERR))
    {
        i2c_interrupt_flag_clear(I2C0, I2C_INT_FLAG_OUERR);
    }

    /* arbitration lost */
    if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_LOSTARB))
    {
        i2c_interrupt_flag_clear(I2C0, I2C_INT_FLAG_LOSTARB);
    }

    /* bus error */
    if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_BERR))
    {
        i2c_interrupt_flag_clear(I2C0, I2C_INT_FLAG_BERR);
    }

    /* CRC value doesn't match */
    if (i2c_interrupt_flag_get(I2C0, I2C_INT_FLAG_PECERR))
    {
        i2c_interrupt_flag_clear(I2C0, I2C_INT_FLAG_PECERR);
    }

    /* disable the error interrupt */
    // i2c_interrupt_disable(I2C0, I2C_INT_ERR);
    // i2c_interrupt_disable(I2C0, I2C_INT_BUF);
    // i2c_interrupt_disable(I2C0, I2C_INT_EV);

    i2c_deinit(I2C0);
    i2c_channel_init(I2C0);
}

bool i2c0_get_slave_device_data(uint8_t *p_buffer, uint32_t *len)
{
    // if (!g_i2c0_recv_is_updated)
    // {
    //     return false;
    // }
    // memcpy(p_buffer, g_i2c0_buff_rx, g_i2c0_rx_count);
    // *len = g_i2c0_rx_count;
    // g_i2c0_recv_is_updated = false; // has read

    return false;
}

#endif

/*!
    \brief      handle I2C1 event interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void I2C1_EV_IRQHandler(void)
{
#ifdef USE_I2C1_AS_IPMB
    BaseType_t err;
    static BaseType_t xHigherPriorityTaskWoken;

    if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_ADDSEND))
    {
        /* clear the ADDSEND bit */
        i2c_interrupt_flag_clear(I2C1, I2C_INT_FLAG_ADDSEND);
        g_i2c_Req.Size = 0;; // start, clear recv count
        g_i2c_Req.Data[g_i2c_Req.Size++] = GetDevAddr();
    }
    else if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_RBNE))
    {
        /* if reception data register is not empty ,I2C0 will read a data from I2C_DATA */
        g_i2c_Req.Data[g_i2c_Req.Size++] = i2c_data_receive(I2C1);
    }
    else if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_STPDET))
    {
        /* clear the STPDET bit */
        i2c_enable(I2C1);
        g_i2c_Req.Param = IPMI_REQUEST;
        if(RecvForwardI2CDatMsg_Queue != NULL && RecvDatMsg_Queue != NULL)
        {
            if((g_i2c_Req.Data[1] & 0x04) == 0) // netFn even
            {
                err = xQueueSendFromISR(RecvDatMsg_Queue, (char*)&g_i2c_Req, &xHigherPriorityTaskWoken);
            }
            else  // odd
            {
                err = xQueueSendFromISR(RecvForwardI2CDatMsg_Queue, (char*)&g_i2c_Req, &xHigherPriorityTaskWoken);
            }

            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            if (err == pdFAIL)
            {
                LOG_E("iic xQueueSendFromISR failed!");
            }
        }
    }

#else
    if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_ADDSEND))
    {
        /* clear the ADDSEND bit */
        i2c_interrupt_flag_clear(I2C1, I2C_INT_FLAG_ADDSEND);
    }
    else if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_RBNE))
    {
        /* if reception data register is not empty ,I2C0 will read a data from I2C_DATA */
        //*i2c_rxbuffer++ = i2c_data_receive(I2C1);
    }
    else if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_STPDET))
    {
        /* clear the STPDET bit */
        i2c_enable(I2C1);
    }

#endif
}

/*!
    \brief      handle I2C1 error interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void I2C1_ER_IRQHandler(void)
{
    /* no acknowledge received */
    if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_AERR))
    {
        i2c_interrupt_flag_clear(I2C1, I2C_INT_FLAG_AERR);
    }

    /* SMBus alert */
    if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_SMBALT))
    {
        i2c_interrupt_flag_clear(I2C1, I2C_INT_FLAG_SMBALT);
    }

    /* bus timeout in SMBus mode */
    if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_SMBTO))
    {
        i2c_interrupt_flag_clear(I2C1, I2C_INT_FLAG_SMBTO);
    }

    /* over-run or under-run when SCL stretch is disabled */
    if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_OUERR))
    {
        i2c_interrupt_flag_clear(I2C1, I2C_INT_FLAG_OUERR);
    }

    /* arbitration lost */
    if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_LOSTARB))
    {
        i2c_interrupt_flag_clear(I2C1, I2C_INT_FLAG_LOSTARB);
    }

    /* bus error */
    if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_BERR))
    {
        i2c_interrupt_flag_clear(I2C1, I2C_INT_FLAG_BERR);
    }

    /* CRC value doesn't match */
    if (i2c_interrupt_flag_get(I2C1, I2C_INT_FLAG_PECERR))
    {
        i2c_interrupt_flag_clear(I2C1, I2C_INT_FLAG_PECERR);
    }

    /* disable the error interrupt */
    // i2c_interrupt_disable(I2C1, I2C_INT_ERR);
    // i2c_interrupt_disable(I2C1, I2C_INT_BUF);
    // i2c_interrupt_disable(I2C1, I2C_INT_EV);

    i2c_deinit(I2C1);
    i2c_channel_init(I2C1);
}
bool i2c1_get_slave_device_data(uint8_t *p_buffer, uint32_t *len)
{
    // if (!g_i2c1_recv_is_updated)
    // {
    //     return false;
    // }
    // memcpy(p_buffer, g_i2c1_buff_rx, g_i2c1_rx_count);
    // *len = g_i2c1_rx_count;
    // g_i2c1_recv_is_updated = false; // has read

    return false;
}

#ifdef I2C2
 
static void i2c2_int(void)
{
    i2c2_rcu_config();
    i2c2_gpio_config();
    i2c2_nvic_config();
    i2c2_config();
}

static void i2c2_rcu_config(void)
{
    /* enable I2C2 clock */
    rcu_periph_clock_enable(RCU_I2C2);

    /* enable GPIO clock */
    rcu_periph_clock_enable(I2C2_SCL_GPIO_CLK);
    rcu_periph_clock_enable(I2C2_SDA_GPIO_CLK);
}
/*!
    \brief      cofigure the GPIO ports.
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void i2c2_gpio_config(void)
{
    /* connect I2C2_SCL I2C2_SDA*/
    gpio_init(I2C2_SCL_GPIO_PORT, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C2_SCL_PIN);
    gpio_init(I2C2_SDA_GPIO_PORT, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C2_SDA_PIN);
#ifdef I2C2_REMAP
    gpio_pin_remap1_config(GPIO_PCF5, GPIO_PCF5_I2C2_REMAP0, ENABLE);
#endif
}
bool i2c2_bytes_write(const uint8_t *p_buffer, uint16_t len)
{
    return i2c_bytes_write(I2C2, p_buffer[0], &p_buffer[1], len-1, 20000);
}

/*!
    \brief      cofigure the I2C0 and I2C1 interfaces..
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void i2c2_config(void)
{
    // **************************** I2C2 *********************************************
    i2c_clock_config(I2C2, 100000, I2C_DTCY_2);
    /* I2C address configure */
    i2c_mode_addr_config(I2C2, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C2_SLAVE_ADDRESS7);
    /* enable I2C1 */
    i2c_enable(I2C2);
    /* enable acknowledge */
    i2c_ack_config(I2C2, I2C_ACK_ENABLE);
        /* enable the I2C0 interrupt */
    i2c_interrupt_enable(I2C2, I2C_INT_ERR);
    i2c_interrupt_enable(I2C2, I2C_INT_EV);
    i2c_interrupt_enable(I2C2, I2C_INT_BUF);
}

bool i2c2_bytes_read(const uint8_t device_addr, const uint8_t read_addr, uint8_t* p_buffer, uint16_t len)
{
    return i2c_bytes_read(I2C2, device_addr, read_addr, p_buffer, len, 10000);
}

/*!
    \brief      handle I2C1 event interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void I2C2_EV_IRQHandler(void)
{
#ifdef USE_I2C2_AS_IPMB
    BaseType_t err;
    static BaseType_t xHigherPriorityTaskWoken;

    if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_ADDSEND))
    {
        /* clear the ADDSEND bit */
        i2c_interrupt_flag_clear(I2C2, I2C_INT_FLAG_ADDSEND);
        g_i2c_Req.Size = 0;; // start, clear recv count
        g_i2c_Req.Data[g_i2c_Req.Size++] = GetDevAddr();
    }
    else if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_RBNE))
    {
        /* if reception data register is not empty ,I2C0 will read a data from I2C_DATA */
        //*i2c_rxbuffer++ = i2c_data_receive(I2C0);
        g_i2c_Req.Data[g_i2c_Req.Size++] = i2c_data_receive(I2C2);
    }
    else if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_STPDET))
    {
        /* clear the STPDET bit */
        i2c_enable(I2C2);
        g_i2c_Req.Param = IPMI_REQUEST;
        if(RecvForwardI2CDatMsg_Queue != NULL && RecvDatMsg_Queue != NULL)
        {
            if((g_i2c_Req.Data[1] & 0x04) == 0) // netFn even
            {
                err = xQueueSendFromISR(RecvDatMsg_Queue, (char*)&g_i2c_Req, &xHigherPriorityTaskWoken);
            }
            else  // odd
            {
                err = xQueueSendFromISR(RecvForwardI2CDatMsg_Queue, (char*)&g_i2c_Req, &xHigherPriorityTaskWoken);
            }

            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            if (err == pdFAIL)
            {
                LOG_E("iic xQueueSendFromISR failed!");
            }
        }
    }
#else
    if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_ADDSEND))
    {
        /* clear the ADDSEND bit */
        i2c_interrupt_flag_clear(I2C2, I2C_INT_FLAG_ADDSEND);
    }
    else if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_RBNE))
    {
        /* if reception data register is not empty ,I2C0 will read a data from I2C_DATA */
        //*i2c_rxbuffer++ = i2c_data_receive(I2C0);
    }
    else if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_STPDET))
    {
        /* clear the STPDET bit */
        i2c_enable(I2C2);
    }
#endif
}

/*!
    \brief      handle I2C1 error interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void I2C2_ER_IRQHandler(void)
{
    /* no acknowledge received */
    if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_AERR))
    {
        i2c_interrupt_flag_clear(I2C2, I2C_INT_FLAG_AERR);
    }

    /* SMBus alert */
    if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_SMBALT))
    {
        i2c_interrupt_flag_clear(I2C2, I2C_INT_FLAG_SMBALT);
    }

    /* bus timeout in SMBus mode */
    if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_SMBTO))
    {
        i2c_interrupt_flag_clear(I2C2, I2C_INT_FLAG_SMBTO);
    }

    /* over-run or under-run when SCL stretch is disabled */
    if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_OUERR))
    {
        i2c_interrupt_flag_clear(I2C2, I2C_INT_FLAG_OUERR);
    }

    /* arbitration lost */
    if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_LOSTARB))
    {
        i2c_interrupt_flag_clear(I2C2, I2C_INT_FLAG_LOSTARB);
    }

    /* bus error */
    if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_BERR))
    {
        i2c_interrupt_flag_clear(I2C2, I2C_INT_FLAG_BERR);
    }

    /* CRC value doesn't match */
    if (i2c_interrupt_flag_get(I2C2, I2C_INT_FLAG_PECERR))
    {
        i2c_interrupt_flag_clear(I2C2, I2C_INT_FLAG_PECERR);
    }

    /* disable the error interrupt */
    i2c_interrupt_disable(I2C2, I2C_INT_ERR);
    i2c_interrupt_disable(I2C2, I2C_INT_BUF);
    i2c_interrupt_disable(I2C2, I2C_INT_EV);

    i2c_deinit(I2C2);
    i2c_channel_init(I2C2);
}

bool i2c2_get_slave_device_data(uint8_t *p_buffer, uint32_t *len)
{
    // if (!g_i2c2_recv_is_updated)
    // {
    //     return false;
    // }
    // memcpy(p_buffer, g_i2c2_buff_rx, g_i2c2_rx_count);
    // *len = g_i2c2_rx_count;
    // g_i2c2_recv_is_updated = false; // has read

    return false;
}
   
void i2c2_set_as_slave_device_addr(uint8_t device_addr)
{
      g_device2_addr = device_addr;
    i2c_set_as_slave_device_addr(I2C2, device_addr);
}

/*!
    \brief      cofigure the NVIC peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void i2c2_nvic_config(void)
{
    nvic_irq_enable(I2C2_EV_IRQn, 3, 0);
    nvic_irq_enable(I2C2_ER_IRQn, 8, 0);
}
#endif
static void i2c_set_as_slave_device_addr(uint32_t i2c_periph, uint8_t device_addr)
{
    /* I2C address configure */
    i2c_mode_addr_config(i2c_periph, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, device_addr);
    /* enable I2C0 */
    i2c_enable(i2c_periph);
    /* enable acknowledge */
    i2c_ack_config(i2c_periph, I2C_ACK_ENABLE);
}


void i2c0_set_as_slave_device_addr(uint8_t device_addr)
{
    g_device0_addr = device_addr;
    i2c_set_as_slave_device_addr(I2C0, device_addr);
}

void i2c1_set_as_slave_device_addr(uint8_t device_addr)
{
	g_device1_addr = device_addr;
    i2c_set_as_slave_device_addr(I2C1, device_addr);
}



uint8_t get_device_addr(uint8_t bus) 
{
   if(0 == bus)
   {
        return  g_device0_addr;
   }else if(1 == bus)
   {
        return  g_device1_addr;
   }else if(2 == bus)
   {
       return  g_device2_addr;
   }else
   {
       return  0;
   }
   
}


