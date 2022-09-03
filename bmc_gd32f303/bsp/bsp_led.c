/**
  ******************************************************************************
  * @author  
  * @version 
  * @date   
  * @brief   
  ******************************************************************************
  ******************************************************************************
  */
  
#include "bsp_led.h"   

void led_init()
{
    /* enable the led clock */
    rcu_periph_clock_enable(LED1_GPIO_CLK);
    /* configure led GPIO port */ 
    gpio_init(LED1_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED1_PIN);
    GPIO_BC(LED1_GPIO_PORT) = LED1_PIN;
}


void led1_set(bool status)
{
	if(status){
		GPIO_BC(LED1_GPIO_PORT) = LED1_PIN;
	}else{
		GPIO_BOP(LED1_GPIO_PORT) = LED1_PIN;
	}
}


/*********************************************END OF FILE**********************/
