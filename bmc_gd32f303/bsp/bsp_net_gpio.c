/**
  ******************************************************************************
  * @author  
  * @version 
  * @date   
  * @brief   
  ******************************************************************************
  ******************************************************************************
  */
  
#include "bsp_net_gpio.h"   

void enet_ctrl_gpio_init(void)
{
	
	rcu_periph_clock_enable(ENET_EN_GPIO_CLK);
	gpio_init(ENET_EN_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_EN_PIN);
	rcu_periph_clock_enable(ENET_INH_GPIO_CLK);
	gpio_init(ENET_INH_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_INH_PIN);
	rcu_periph_clock_enable(ENET_WAKE_GPIO_CLK);
	gpio_init(ENET_WAKE_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_WAKE_PIN);
	rcu_periph_clock_enable(ENET_RESET_GPIO_CLK);
	gpio_init(ENET_RESET_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_RESET_PIN);
	
	
//	rcu_periph_clock_enable(ENET_RXD0_GPIO_CLK);
//	gpio_init(ENET_RXD0_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_RXD0_PIN);
//	rcu_periph_clock_enable(ENET_RXD1_GPIO_CLK);
//	gpio_init(ENET_RXD1_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_RXD1_PIN);
//	rcu_periph_clock_enable(ENET_RXD2_GPIO_CLK);
//	gpio_init(ENET_RXD2_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_RXD2_PIN);
//	rcu_periph_clock_enable(ENET_RXD3_GPIO_CLK);
//	gpio_init(ENET_RXD3_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_RXD3_PIN);
	
//	
//	rcu_periph_clock_enable(ENET_RXDV_GPIO_CLK);
//	gpio_init(ENET_RXDV_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_RXDV_PIN);
		rcu_periph_clock_enable(ENET_RXER_GPIO_CLK);
   	gpio_init(ENET_RXER_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, ENET_RXER_PIN);
}

void enet_en_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_EN_GPIO_PORT, ENET_EN_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_EN_GPIO_PORT, ENET_EN_PIN);  
	}
}

void enet_inh_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_INH_GPIO_PORT, ENET_INH_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_INH_GPIO_PORT, ENET_INH_PIN);  
	}
}


void enet_wake_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_WAKE_GPIO_PORT, ENET_WAKE_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_WAKE_GPIO_PORT, ENET_WAKE_PIN);  
	}
}

void enet_reset_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_RESET_GPIO_PORT, ENET_RESET_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_RESET_GPIO_PORT, ENET_RESET_PIN);  
	}
}

void enet_rxd0_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_RXD0_GPIO_PORT, ENET_RXD0_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_RXD0_GPIO_PORT, ENET_RXD0_PIN);  
	}
}

void enet_rxd1_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_RXD1_GPIO_PORT, ENET_RXD1_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_RXD1_GPIO_PORT, ENET_RXD1_PIN);  
	}
}

void enet_rxd2_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_RXD2_GPIO_PORT, ENET_RXD2_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_RXD2_GPIO_PORT, ENET_RXD2_PIN);  
	}
}

void enet_rxd3_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_RXD3_GPIO_PORT, ENET_RXD3_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_RXD3_GPIO_PORT, ENET_RXD3_PIN);  
	}
}

void enet_rxdv_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_RXDV_GPIO_PORT, ENET_RXDV_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_RXDV_GPIO_PORT, ENET_RXDV_PIN);  
	}
}

void enet_rxer_set(bool flag)
{
	if(flag)
	{
		gpio_bit_set(ENET_RXER_GPIO_PORT, ENET_RXER_PIN);   
	}
	else
	{
		gpio_bit_reset(ENET_RXER_GPIO_PORT, ENET_RXER_PIN);  
	}
}


/*********************************************END OF FILE**********************/
