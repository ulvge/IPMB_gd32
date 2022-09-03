/**
  ******************************************************************************
  * @author  
  * @version 
  * @date   
  * @brief   
  ******************************************************************************
  ******************************************************************************
  */
  
#include "bsp_gpio.h"   
#include "bsp_i2c.h"

void addr_gpio_init()
{
    /* enable the clock */
    rcu_periph_clock_enable(GA0_GPIO_CLK);
    gpio_init(GA0_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GA0_PIN);
	  rcu_periph_clock_enable(GA1_GPIO_CLK);
    gpio_init(GA1_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GA1_PIN);
	  rcu_periph_clock_enable(GA2_GPIO_CLK);
    gpio_init(GA2_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GA2_PIN);
	  rcu_periph_clock_enable(GA3_GPIO_CLK);
    gpio_init(GA3_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GA3_PIN);
	  rcu_periph_clock_enable(GA4_GPIO_CLK);
    gpio_init(GA4_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GA4_PIN);
	  rcu_periph_clock_enable(GAP_GPIO_CLK);
    gpio_init(GAP_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GAP_PIN);
}

uint8_t get_board_addr()
{
		uint8_t addr = 0;
	
		addr |= gpio_input_bit_get(GA0_GPIO_PORT, GA0_PIN) << 0;
		addr |= gpio_input_bit_get(GA1_GPIO_PORT, GA0_PIN) << 1;
		addr |= gpio_input_bit_get(GA2_GPIO_PORT, GA0_PIN) << 2;
		addr |= gpio_input_bit_get(GA3_GPIO_PORT, GA0_PIN) << 3;
		addr |= gpio_input_bit_get(GA4_GPIO_PORT, GA0_PIN) << 4;

    return I2C2_SLAVE_ADDRESS7;

	  // return addr;
}

void test_gpio_init()
{
    /* enable the clock */
    rcu_periph_clock_enable(TEST_GPIO_CLK);
    gpio_init(TEST_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, TEST_GPIO_PIN);
}

void test_gpio_set(bool status)
{
	if(status){
		GPIO_BC(TEST_GPIO_PORT) = TEST_GPIO_PIN;
	}else{
		GPIO_BOP(TEST_GPIO_PORT) = TEST_GPIO_PIN;
	}  
}

/*********************************************END OF FILE**********************/
