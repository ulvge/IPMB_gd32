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
#include "Types.h"

const static GPIOConfig g_gpioConfig[] = {
    {GPIO_OUT_LED_RED,                  GA0_GPIO_PORT, GA0_PIN, GA0_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
    {GPIO_OUT_LED_GREEN,                GA0_GPIO_PORT, GA0_PIN, GA0_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
    {GPIO_OUT_CPU_POWER_ON,             GA0_GPIO_PORT, GA0_PIN, GA0_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
    {GPIO_OUT_CPU_POWER_OFF,            GA0_GPIO_PORT, GA0_PIN, GA0_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
    {GPIO_OUT_CPU_RESET,                GA0_GPIO_PORT, GA0_PIN, GA0_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
    {GPIO_OUT_BMC_POWER_ON_FINISHED,    GA0_GPIO_PORT, GA0_PIN, GA0_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},

    {GPIO_IN_GAP0,                      GA0_GPIO_PORT, GA0_PIN, GA0_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
    {GPIO_IN_GAP1,                      GA1_GPIO_PORT, GA1_PIN, GA1_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
    {GPIO_IN_GAP2,                      GA2_GPIO_PORT, GA2_PIN, GA2_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
    {GPIO_IN_GAP3,                      GA3_GPIO_PORT, GA3_PIN, GA3_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
    {GPIO_IN_GAP4,                      GA4_GPIO_PORT, GA4_PIN, GA4_GPIO_CLK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ},
};

void GPIO_bspInit(void)
{
    UINT8 num = sizeof(g_gpioConfig) / sizeof(g_gpioConfig[0]);
    const GPIOConfig  *p_gpioCfg;
    for (UINT8 i=0; i< num;i++){
		p_gpioCfg = &g_gpioConfig[i];
        
        /* enable the clock */
        rcu_periph_clock_enable(p_gpioCfg->gpioClk);
        gpio_init(p_gpioCfg->gpioPort, p_gpioCfg->pinMode, p_gpioCfg->pinSpeed, p_gpioCfg->pin);
	}
}
static const GPIOConfig  *GPIO_findGpio(BMC_GPIO_enum alias)
{
    UINT8 num = sizeof(g_gpioConfig) / sizeof(g_gpioConfig[0]);
	const GPIOConfig  *p_gpioCfg;
    for (UINT8 i=0; i< num;i++){
        p_gpioCfg = &g_gpioConfig[i];
        if (p_gpioCfg->alias == alias) {
            return p_gpioCfg;
        }
    }
    return NULL;
}
FlagStatus GPIO_getPinStatus(BMC_GPIO_enum alias)
{
    UINT8 num = sizeof(g_gpioConfig) / sizeof(g_gpioConfig[0]);
	const GPIOConfig  *p_gpioCfg = GPIO_findGpio(alias);
    
    if (p_gpioCfg == NULL) {
        return RESET;
    }
    return gpio_input_bit_get(p_gpioCfg->gpioPort, p_gpioCfg->pin);
}

bool GPIO_setPinStatus(BMC_GPIO_enum alias, bool status)
{
    UINT8 num = sizeof(g_gpioConfig) / sizeof(g_gpioConfig[0]);
	const GPIOConfig  *p_gpioCfg = GPIO_findGpio(alias);
    
    if (p_gpioCfg == NULL) {
        return false;
    }
    if(status){
        GPIO_BC(p_gpioCfg->gpioPort) = p_gpioCfg->pin;
    }else{
        GPIO_BOP(p_gpioCfg->gpioPort) = p_gpioCfg->pin;
    }
    return true;
}

uint8_t get_board_addr()
{
	uint8_t addr = 0;

	addr |= GPIO_getPinStatus(GPIO_IN_GAP0) << 0;
	addr |= GPIO_getPinStatus(GPIO_IN_GAP1) << 1;
	addr |= GPIO_getPinStatus(GPIO_IN_GAP2) << 2;
	addr |= GPIO_getPinStatus(GPIO_IN_GAP3) << 3;
	addr |= GPIO_getPinStatus(GPIO_IN_GAP4) << 4;

    return I2C2_SLAVE_ADDRESS7;
}


/*********************************************END OF FILE**********************/
