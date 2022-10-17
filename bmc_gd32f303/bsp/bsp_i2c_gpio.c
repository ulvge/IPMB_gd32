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
#include "main.h"


/*
*********************************************************************************************************
local function
*********************************************************************************************************
*/
static void i2c1_CfgGpio(void);
static void i2c2_CfgGpio(void);

/* simulated i2c1 function declarations */
static void      i2c1_Start          (void);
static void      i2c1_Stop           (void);
static void      i2c1_SendByte       (uint8_t _ucByte);
static uint8_t   i2c1_ReadByte       (void);
static uint8_t   i2c1_WaitAck        (void);
static void      i2c1_Ack            (void);
static void      i2c1_NAck           (void);

/* simulated i2c2 function declarations */
static void      i2c2_Start          (void);
static void      i2c2_Stop           (void);
static void      i2c2_SendByte       (uint8_t _ucByte);
static uint8_t   i2c2_ReadByte       (void);
static uint8_t   i2c2_WaitAck        (void);
static void      i2c2_Ack            (void);
static void      i2c2_NAck           (void);

/*
*********************************************************************************************************
*********************************************************************************************************
*/
static void i2c_Delay(void)
{
    Delay_NoSchedue(15);
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c1_CfgGpio(void)
{
	/* enable the led clock */
	rcu_periph_clock_enable(macI2C1_CLK);
	/* configure led GPIO port */ 
	gpio_init(macI2C1_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, macI2C1_SCL_PIN | macI2C1_SDA_PIN);

	i2c1_Stop();
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c1_Start(void)
{

	macI2C1_SDA_1();
	macI2C1_SCL_1();
	i2c_Delay();
	macI2C1_SDA_0();
	i2c_Delay();
	macI2C1_SCL_0();
	i2c_Delay();
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c1_Stop(void)
{
	macI2C1_SDA_0();
	macI2C1_SCL_1();
	i2c_Delay();
	macI2C1_SDA_1();
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c1_SendByte(uint8_t _ucByte)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
	{		
		if (_ucByte & 0x80)
		{
			macI2C1_SDA_1();
		}
		else
		{
			macI2C1_SDA_0();
		}
		i2c_Delay();
		macI2C1_SCL_1();
		i2c_Delay();	
		macI2C1_SCL_0();
		if (i == 7)
		{
			 macI2C1_SDA_1(); 
		}
		_ucByte <<= 1;	
		i2c_Delay();
	}
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static uint8_t i2c1_ReadByte(void)
{
	uint8_t i;
	uint8_t value;

	value = 0;
	for (i = 0; i < 8; i++)
	{
		value <<= 1;
		macI2C1_SCL_1();
		i2c_Delay();
		if (macI2C1_SDA_READ())
		{
			value++;
		}
		macI2C1_SCL_0();
		i2c_Delay();
	}
	return value;
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static uint8_t i2c1_WaitAck(void)
{
	uint8_t re;

	macI2C1_SDA_1();	
	i2c_Delay();
	macI2C1_SCL_1();
	i2c_Delay();
	if (macI2C1_SDA_READ())	
	{
		re = 1;
	}
	else
	{
		re = 0;
	}
	macI2C1_SCL_0();
	i2c_Delay();
	return re;
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c1_Ack(void)
{
	macI2C1_SDA_0();	
	i2c_Delay();
	macI2C1_SCL_1();
	i2c_Delay();
	macI2C1_SCL_0();
	i2c_Delay();
	macI2C1_SDA_1();	
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c1_NAck(void)
{
	macI2C1_SDA_1();	
	i2c_Delay();
	macI2C1_SCL_1();
	i2c_Delay();
	macI2C1_SCL_0();
	i2c_Delay();	
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
uint8_t simulated_i2c1_CheckDevice(uint8_t _Address)
{
	uint8_t ucAck;

	i2c1_CfgGpio();		
	i2c1_Start();	
	i2c1_SendByte(_Address | macI2C_WR);
	ucAck = i2c1_WaitAck();	
	i2c1_Stop();		

	return ucAck;
}


/*
*****************************************************************************
I2C2 functions
*********************************************************************************************************
*/
static void i2c2_CfgGpio(void)
{
	/* enable the led clock */
	rcu_periph_clock_enable(macI2C2_CLK);
	/* configure led GPIO port */ 
	gpio_init(macI2C2_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, macI2C2_SCL_PIN | macI2C2_SDA_PIN);

	i2c2_Stop();
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c2_Start(void)
{

	macI2C2_SDA_1();
	macI2C2_SCL_1();
	i2c_Delay();
	macI2C2_SDA_0();
	i2c_Delay();
	macI2C2_SCL_0();
	i2c_Delay();
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c2_Stop(void)
{
	macI2C2_SDA_0();
	macI2C2_SCL_1();
	i2c_Delay();
	macI2C2_SDA_1();
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c2_SendByte(uint8_t _ucByte)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
	{		
		if (_ucByte & 0x80)
		{
			macI2C2_SDA_1();
		}
		else
		{
			macI2C2_SDA_0();
		}
		i2c_Delay();
		macI2C2_SCL_1();
		i2c_Delay();	
		macI2C2_SCL_0();
		if (i == 7)
		{
			 macI2C2_SDA_1(); 
		}
		_ucByte <<= 1;	
		i2c_Delay();
	}
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static uint8_t i2c2_ReadByte(void)
{
	uint8_t i;
	uint8_t value;

	value = 0;
	for (i = 0; i < 8; i++)
	{
		value <<= 1;
		macI2C2_SCL_1();
		i2c_Delay();
		if (macI2C2_SDA_READ())
		{
			value++;
		}
		macI2C2_SCL_0();
		i2c_Delay();
	}
	return value;
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static uint8_t i2c2_WaitAck(void)
{
	uint8_t re;

	macI2C2_SDA_1();	
	i2c_Delay();
	macI2C2_SCL_1();
	i2c_Delay();
	if (macI2C2_SDA_READ())	
	{
		re = 1;
	}
	else
	{
		re = 0;
	}
	macI2C2_SCL_0();
	i2c_Delay();
	return re;
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c2_Ack(void)
{
	macI2C2_SDA_0();	
	i2c_Delay();
	macI2C2_SCL_1();
	i2c_Delay();
	macI2C2_SCL_0();
	i2c_Delay();
	macI2C2_SDA_1();	
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
static void i2c2_NAck(void)
{
	macI2C2_SDA_1();	
	i2c_Delay();
	macI2C2_SCL_1();
	i2c_Delay();
	macI2C2_SCL_0();
	i2c_Delay();	
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
uint8_t simulated_i2c2_CheckDevice(uint8_t _Address)
{
	uint8_t ucAck;

	i2c2_CfgGpio();		
	i2c2_Start();	
	i2c2_SendByte(_Address | macI2C_WR);
	ucAck = i2c2_WaitAck();	
	i2c2_Stop();		

	return ucAck;
}



/*
*********************************************************************************************************

*********************************************************************************************************
*/
bool simulated_i2c1_read_bytes(uint8_t dev_addr, uint16_t _usAddress, uint8_t *_pReadBuf, uint16_t _usSize)
{
	uint16_t i;

	i2c1_Start();
	i2c1_SendByte(dev_addr | macI2C_WR);	
	
	if (i2c1_WaitAck() != 0)
	{
		goto cmd_fail;	
	}

	i2c1_SendByte((uint8_t)_usAddress);
	
	if (i2c1_WaitAck() != 0)
	{
		goto cmd_fail;	
	}
	

	i2c1_Start();
	i2c1_SendByte(dev_addr | macI2C_RD);	
	
	if (i2c1_WaitAck() != 0)
	{
		goto cmd_fail;
	}	
	
	for (i = 0; i < _usSize; i++)
	{
		_pReadBuf[i] = i2c1_ReadByte();
		
		if (i != _usSize - 1)
		{
			i2c1_Ack();	
		}
		else
		{
			i2c1_NAck();	
		}
	}

	i2c1_Stop();
	return true;	

cmd_fail: 
	i2c1_Stop();
	return false;
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
bool simulated_i2c1_write_bytes(uint8_t dev_addr, uint16_t _usAddress, uint8_t *_pWriteBuf, uint16_t _usSize)
{
	uint16_t i,m;
	uint16_t usAddr;

	usAddr = _usAddress;	
	for (i = 0; i < _usSize; i++)
	{
		if (i == 0)
		{
			i2c1_Stop();
			
			for (m = 0; m < 1000; m++)
			{				
				i2c1_Start();
				i2c1_SendByte(dev_addr | macI2C_WR);	

				if (i2c1_WaitAck() == 0)
				{
					break;
				}
			}
			if (m  == 1000)
			{
				goto cmd_fail;	
			}
		
			i2c1_SendByte((uint8_t)usAddr);

			if (i2c1_WaitAck() != 0)
			{
				goto cmd_fail;	
			}
		}
	
		i2c1_SendByte(_pWriteBuf[i]);

		if (i2c1_WaitAck() != 0)
		{
			goto cmd_fail;
		}

		usAddr++;			
	}
	
	i2c1_Stop();
	return true;

cmd_fail: 
	i2c1_Stop();
	return false;
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
bool simulated_i2c2_read_bytes(uint8_t dev_addr, uint16_t _usAddress, uint8_t *_pReadBuf, uint16_t _usSize)
{
	uint16_t i;

	i2c2_Start();
	i2c2_SendByte(dev_addr | macI2C_WR);	
	
	if (i2c2_WaitAck() != 0)
	{
		goto cmd_fail;	
	}

	i2c2_SendByte((uint8_t)_usAddress);
	
	if (i2c2_WaitAck() != 0)
	{
		goto cmd_fail;	
	}
	

	i2c2_Start();
	i2c2_SendByte(dev_addr | macI2C_RD);	
	
	if (i2c2_WaitAck() != 0)
	{
		goto cmd_fail;
	}	
	
	for (i = 0; i < _usSize; i++)
	{
		_pReadBuf[i] = i2c2_ReadByte();
		
		if (i != _usSize - 1)
		{
			i2c2_Ack();	
		}
		else
		{
			i2c2_NAck();	
		}
	}

	i2c2_Stop();
	return true;	

cmd_fail: 
	i2c2_Stop();
	return false;
}

/*
*********************************************************************************************************

*********************************************************************************************************
*/
bool simulated_i2c2_write_bytes(uint8_t dev_addr, uint16_t _usAddress, uint8_t *_pWriteBuf, uint16_t _usSize)
{
	uint16_t i,m;
	uint16_t usAddr;

	usAddr = _usAddress;	
	for (i = 0; i < _usSize; i++)
	{
		if (i == 0)
		{
			i2c2_Stop();
			
			for (m = 0; m < 1000; m++)
			{				
				i2c2_Start();
				i2c2_SendByte(dev_addr | macI2C_WR);	

				if (i2c2_WaitAck() == 0)
				{
					break;
				}
			}
			if (m  == 1000)
			{
				goto cmd_fail;	
			}
		
			i2c2_SendByte((uint8_t)usAddr);

			if (i2c2_WaitAck() != 0)
			{
				goto cmd_fail;	
			}
		}
	
		i2c2_SendByte(_pWriteBuf[i]);

		if (i2c2_WaitAck() != 0)
		{
			goto cmd_fail;
		}

		usAddr++;			
	}
	
	i2c2_Stop();
	return true;

cmd_fail: 
	i2c2_Stop();
	return false;
}
