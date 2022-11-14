/**
  ******************************************************************************
  * @file    
  * @author 
  * @version
  * @date   
  * @brief  
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */ 


#include "bsp_i2c_gpio.h"
#include "tools.h"
#include "OSPort.h"

/********************* GPIO SIMULATED I2C1 MICRO DEF ***************************/
#define I2CS0_SCL_GPIO_PORT     GPIOB
#define I2CS0_SCL_CLK           RCU_GPIOB
#define I2CS0_SCL_PIN           GPIO_PIN_8

#define I2CS0_SDA_GPIO_PORT     GPIOB
#define I2CS0_SDA_CLK           RCU_GPIOB
#define I2CS0_SDA_PIN           GPIO_PIN_9

#define I2CS0_RETRY_TIMERS      3
#define I2CS0_SDA_READ() gpio_input_bit_get(I2CS0_SDA_GPIO_PORT, I2CS0_SDA_PIN) /* SDA read */
/********************* GPIO SIMULATED I2C1 MICRO DEF END***************************/

/*
*********************************************************************************************************
local function
*********************************************************************************************************
*/
static void i2cs0_CfgGpio(void);

/* simulated i2c1 function declarations */
static void      i2cs0_Start          (void);
static void      i2cs0_SendByte       (uint8_t _ucByte);
static uint8_t   i2cs0_ReadByte       (void);
static uint8_t   i2cs0_WaitAck        (void);
static void      i2cs0_Ack            (void);
static void      i2cs0_NAck           (void);

xSemaphoreHandle g_I2CS0_semaphore = NULL;
#define I2CS0_TAKE_SEMAPHORE_TIMEOUT      100

/*
*********************************************************************************************************
*********************************************************************************************************
*/
static void i2cs0_Delay(uint32_t clk)
{
    Delay_NoSchedue(clk);
}

static void I2CS0_SCL_1(void)
{
    gpio_bit_set(I2CS0_SCL_GPIO_PORT, I2CS0_SCL_PIN); /* SCL = 1 */
    i2cs0_Delay(25);
}
static void I2CS0_SCL_0(void)
{
    gpio_bit_reset(I2CS0_SCL_GPIO_PORT, I2CS0_SCL_PIN); /* SCL = 0 */
    i2cs0_Delay(15);
}

static void I2CS0_SDA_1(void)
{
    gpio_bit_set(I2CS0_SCL_GPIO_PORT, I2CS0_SDA_PIN); /* SDA = 1 */
    i2cs0_Delay(15);
}
static void I2CS0_SDA_0(void)
{
    gpio_bit_reset(I2CS0_SCL_GPIO_PORT, I2CS0_SDA_PIN); /* SDA = 0 */
    i2cs0_Delay(15);
}
/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2cs0_CfgGpio(void)
{
	if (g_I2CS0_semaphore == NULL) {
		g_I2CS0_semaphore = xSemaphoreCreateBinary();
	}
	/* enable the led clock */
	rcu_periph_clock_enable(I2CS0_SCL_CLK);
	rcu_periph_clock_enable(I2CS0_SDA_CLK);
	/* configure led GPIO port */ 
	gpio_init(I2CS0_SCL_GPIO_PORT, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, I2CS0_SCL_PIN);
	gpio_init(I2CS0_SDA_GPIO_PORT, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, I2CS0_SDA_PIN);

	i2cs0_Stop();
}
/*
*********************************************************************************************************

*********************************************************************************************************
*/
void i2cs0_set_address(uint8_t _Address)
{

}
/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2cs0_Start(void)
{
	I2CS0_SDA_1();
	I2CS0_SCL_1();
	I2CS0_SDA_0();
	I2CS0_SCL_0();
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
void i2cs0_Stop(void)
{
	I2CS0_SDA_0();
	I2CS0_SCL_1();
	I2CS0_SDA_1();
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2cs0_SendByte(uint8_t _ucByte)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
	{		
		if (_ucByte & 0x80)
		{
			I2CS0_SDA_1();
		}
		else
		{
			I2CS0_SDA_0();
		}
		I2CS0_SCL_1();	
		I2CS0_SCL_0();
		if (i == 7)
		{
			 I2CS0_SDA_1(); 
		}
		_ucByte <<= 1;	
	}
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static uint8_t i2cs0_ReadByte(void)
{
	uint8_t i;
	uint8_t value;

	value = 0;
	for (i = 0; i < 8; i++)
	{
		value <<= 1;
		I2CS0_SCL_1();
		if (I2CS0_SDA_READ())
		{
			value++;
		}
		I2CS0_SCL_0();
	}
	return value;
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static uint8_t i2cs0_WaitAck(void)
{
	uint8_t re;

	I2CS0_SDA_1();	
	I2CS0_SCL_1();
	if (I2CS0_SDA_READ())	
	{
		re = 1;
	}
	else
	{
		re = 0;
	}
	I2CS0_SCL_0();
	return re;
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2cs0_Ack(void)
{
	I2CS0_SDA_0();	
	I2CS0_SCL_1();
	I2CS0_SCL_0();
	I2CS0_SDA_1();	
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2cs0_NAck(void)
{
	I2CS0_SDA_1();	
	I2CS0_SCL_1();
	I2CS0_SCL_0();	
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
void i2cs0_init(void)
{
	i2cs0_CfgGpio();		
	i2cs0_Stop();
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
bool i2cs0_read_bytes(uint8_t dev_addr, uint16_t _usAddress, uint8_t *_pReadBuf, uint16_t _usSize)
{
	uint16_t i;

	i2cs0_Start();
	i2cs0_SendByte(dev_addr | I2CS_WR);	
	
	if (i2cs0_WaitAck() != 0)
	{
		goto cmd_fail;	
	}
    if ((_usSize == 0) || (_pReadBuf == NULL)) {
        i2cs0_Stop();
        return true;	
    }
	i2cs0_SendByte((uint8_t)_usAddress);
	
	if (i2cs0_WaitAck() != 0)
	{
		goto cmd_fail;	
	}
	

	i2cs0_Start();
	i2cs0_SendByte(dev_addr | I2CS_RD);	
	
	if (i2cs0_WaitAck() != 0)
	{
		goto cmd_fail;
	}	
	
	for (i = 0; i < _usSize; i++)
	{
		_pReadBuf[i] = i2cs0_ReadByte();
		
		if (i != _usSize - 1)
		{
			i2cs0_Ack();	
		}
		else
		{
			i2cs0_NAck();	
		}
	}

	i2cs0_Stop();
	return true;	

cmd_fail: 
	i2cs0_Stop();
	return false;
}
/*
*********************************************************************************************************

*********************************************************************************************************
*/
bool i2cs0_write_bytes(uint8_t dev_addr, uint16_t _usAddress, const uint8_t *_pWriteBuf, uint16_t _usSize)
{
	uint16_t i,m;
	uint16_t usAddr;

    if (xSemaphoreTake(g_I2CS0_semaphore, I2CS0_TAKE_SEMAPHORE_TIMEOUT)) {
	    return false;
    }
	usAddr = _usAddress;	
	for (i = 0; i < _usSize; i++)
	{
		if (i == 0)
		{
			i2cs0_Stop();
			
			for (m = 0; m < I2CS0_RETRY_TIMERS; m++)
			{				
				i2cs0_Start();
				i2cs0_SendByte(dev_addr | I2CS_WR);	

				if (i2cs0_WaitAck() == 0)
				{
					break;
				}else{
			        i2cs0_Stop();
                }
			}
			if (m  == I2CS0_RETRY_TIMERS)
			{
				goto cmd_fail;	
			}
		
			i2cs0_SendByte((uint8_t)usAddr);

			if (i2cs0_WaitAck() != 0)
			{
				goto cmd_fail;	
			}
		}
	
		i2cs0_SendByte(_pWriteBuf[i]);

		if (i2cs0_WaitAck() != 0)
		{
			goto cmd_fail;
		}

		usAddr++;			
	}
	
	i2cs0_Stop();
    xSemaphoreGive(g_I2CS0_semaphore);
	return true;

cmd_fail:
	i2cs0_Stop();
    xSemaphoreGive(g_I2CS0_semaphore);
    return false;
}
