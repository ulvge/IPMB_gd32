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
#include "api_subdevices.h"
#include "debug_print.h" 

const static GPIOConfig g_gpioConfigComm[] = {
    {GPIO_IN_GAP0, GPIOE, GPIO_PIN_2, RCU_GPIOE, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, 0},
    {GPIO_IN_GAP1, GPIOE, GPIO_PIN_3, RCU_GPIOE, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, 0},
    {GPIO_IN_GAP2, GPIOE, GPIO_PIN_4, RCU_GPIOE, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, 0},
    {GPIO_IN_DEBUG,GPIOA, GPIO_PIN_11, RCU_GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_10MHZ, 0},
};

const static GPIOConfig_Handler *g_pGpioConfig_Handler = NULL;
const static GPIOConfig_Handler *g_gpioAllDevices[] = {
    &g_gpioConfigHandler_main,
    // &g_gpioConfigHandler_net,
    // &g_gpioConfigHandler_switch,
    // &g_gpioConfigHandler_power,
    // &g_gpioConfigHandler_storage0,
};


static void GPIO_InitGPIOs(const GPIOConfig *config, UINT8 size)
{
    const GPIOConfig *p_gpioCfg;
    for (UINT8 i = 0; i < size; i++)
    {
        p_gpioCfg = &config[i];

        /* enable the clock */
        rcu_periph_clock_enable(p_gpioCfg->gpioClk);
        gpio_init(p_gpioCfg->gpioPort, p_gpioCfg->pinMode, p_gpioCfg->pinSpeed, p_gpioCfg->pin);
        GPIO_setPinStatus(p_gpioCfg->alias, DISABLE);
    }
}
static bool GPIO_CheckMode(void)
{
    if (!SubDevice_CheckAndPrintMode())
    {
        LOG_E("Check mode failed, system will reset line=%d", __LINE__);
        return false;
    }
    SUB_DEVICE_MODE myMode = SubDevice_GetMyMode();

    for (size_t i = 0; i < ARRARY_SIZE(g_gpioAllDevices); i++)
    {
        const GPIOConfig_Handler **phandler = (g_gpioAllDevices + i);
        if ((*phandler)->mode == myMode)
        {
            g_pGpioConfig_Handler = *phandler;
            GPIO_InitGPIOs(g_pGpioConfig_Handler->gpioCfg, g_pGpioConfig_Handler->gpioCfgSize);
            return true;
        }
    }

    LOG_E("not find gpio config mode, system will reset line=%d", __LINE__);
    return false;
}
void GPIO_bspInit(void)
{
    GPIO_InitGPIOs(&g_gpioConfigComm[0], ARRARY_SIZE(g_gpioConfigComm));

    while (1)
    {
        if (GPIO_CheckMode()){
            break;
        }
    }
}
static const GPIOConfig *GPIO_findGpio(BMC_GPIO_enum alias)
{
    if (g_pGpioConfig_Handler == NULL)
    {
        return NULL;
    }
    UINT8 num = g_pGpioConfig_Handler->gpioCfgSize;
    const GPIOConfig *p_gpioCfg;
    for (UINT8 i = 0; i < num; i++)
    {
        p_gpioCfg = (g_pGpioConfig_Handler->gpioCfg) + i;
        if (p_gpioCfg->alias == alias)
        {
            return p_gpioCfg;
        }
    }
    return NULL;
}
FlagStatus GPIO_getPinStatus(BMC_GPIO_enum alias)
{
    const GPIOConfig *p_gpioCfg = GPIO_findGpio(alias);

    if (p_gpioCfg == NULL)
    {
        return RESET;
    }
    return gpio_input_bit_get(p_gpioCfg->gpioPort, p_gpioCfg->pin);
}
bool GPIO_isPinActive(BMC_GPIO_enum alias)
{
    const GPIOConfig *p_gpioCfg = GPIO_findGpio(alias);

    if (p_gpioCfg == NULL)
    {
        return false;
    }
    FlagStatus staus = gpio_input_bit_get(p_gpioCfg->gpioPort, p_gpioCfg->pin);
    if (staus == p_gpioCfg->pinMode) {
        return true;
    }
    return false;
}

bool GPIO_setPinStatus(BMC_GPIO_enum alias, ControlStatus isActive)
{
    const GPIOConfig *p_gpioCfg = GPIO_findGpio(alias);

    if (p_gpioCfg == NULL)
    {
        return false;
    }

    if (!((p_gpioCfg->pinMode == GPIO_MODE_OUT_OD) || (p_gpioCfg->pinMode == GPIO_MODE_OUT_PP) ||
          (p_gpioCfg->pinMode == GPIO_MODE_AF_OD) || (p_gpioCfg->pinMode == GPIO_MODE_AF_PP)))
    {
        return false;
    }

    if (p_gpioCfg->activeMode)
    {
        if (isActive)
        {
            GPIO_BOP(p_gpioCfg->gpioPort) = p_gpioCfg->pin;
        }
        else
        {
            GPIO_BC(p_gpioCfg->gpioPort) = p_gpioCfg->pin;
        }
    }
    else
    {
        if (isActive)
        {
            GPIO_BC(p_gpioCfg->gpioPort) = p_gpioCfg->pin;
        }
        else
        {
            GPIO_BOP(p_gpioCfg->gpioPort) = p_gpioCfg->pin;
        }
    }
    return true;
}

uint8_t get_board_addr()
{
    uint8_t addr = 0;
    addr |= GPIO_getPinStatus(GPIO_IN_GAP0) << 0;
    addr |= GPIO_getPinStatus(GPIO_IN_GAP1) << 1;
    addr |= GPIO_getPinStatus(GPIO_IN_GAP2) << 2;

    return 0;
}


/*********************************************END OF FILE**********************/
