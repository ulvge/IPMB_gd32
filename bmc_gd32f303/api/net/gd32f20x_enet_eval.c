/*!
    \file  gd32f20x_enet_eval.c
    \brief ethernet hardware configuration 
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, firmware for GD32F20x
    2017-06-05, V2.0.0, firmware for GD32F20x
*/

#include "gd32f20x_enet.h"
#include "gd32f20x_enet_eval.h"
#include "OSPort.h"
#include "bsp_net_gpio.h"

static __IO uint32_t enet_init_status = 0;

static void enet_gpio_config(void);
static bool enet_mac_dma_config(void);
static void nvic_configuration(void);

/*!
    \brief      setup ethernet system(GPIOs, clocks, MAC, DMA, systick)
    \param[in]  none
    \param[out] none
    \retval     none
*/
bool enet_hardware_init(void)
{	
    bool res;

//	set_phy_mode();
    nvic_configuration();
  
    /* configure the GPIO ports for ethernet pins */
    enet_gpio_config();
    /* configure the ethernet MAC/DMA */
    res = enet_mac_dma_config();
    if(!res){
        return false;
    }

    enet_interrupt_enable(ENET_DMA_INT_NIE);
    enet_interrupt_enable(ENET_DMA_INT_RIE); 

    return true;
}

/*!
    \brief      setup ethernet system(GPIOs, clocks, MAC, DMA, systick)
    \param[in]  none
    \param[out] none
    \retval     none
*/
bool enet_software_init(void)
{	
#ifdef CHECKSUM_BY_HARDWARE
    enet_init_status = enet_init(ENET_AUTO_NEGOTIATION, ENET_AUTOCHECKSUM_DROP_FAILFRAMES, ENET_BROADCAST_FRAMES_PASS);
#else  
    enet_init_status = enet_init(ENET_AUTO_NEGOTIATION, ENET_NO_AUTOCHECKSUM, ENET_BROADCAST_FRAMES_PASS);
#endif

    if (enet_init_status == 0){
        printf("enet_init failed!\r\n");
        return false;
    }

    return true;
}

/*!
    \brief      configures the ethernet interface
    \param[in]  none
    \param[out] none
    \retval     none
*/
static bool enet_mac_dma_config(void)
{
    ErrStatus reval_state = ERROR;
    
    /* enable ethernet clock  */
    rcu_periph_clock_enable(RCU_ENET);
    rcu_periph_clock_enable(RCU_ENETTX);
    rcu_periph_clock_enable(RCU_ENETRX);
    
    /* reset ethernet on AHB bus */
    enet_deinit();

    reval_state = enet_software_reset();
    if(reval_state == ERROR){
        printf("enet_software_reset failed!\r\n");
        return false;
    }

    return true;
}

/*!
    \brief      configures the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void nvic_configuration(void)
{
    nvic_irq_enable(ENET_IRQn, 2, 0);
}

/*!
    \brief      configures the different GPIO ports
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void enet_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
  
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
  
    /* enable SYSCFG clock */
    rcu_periph_clock_enable(RCU_AF);
  
#ifdef MII_MODE 
  
#ifdef PHY_CLOCK_MCO
    /* output HXTAL clock (25MHz) on CKOUT0 pin(PA8) to clock the PHY */
    rcu_ckout0_config(RCU_CKOUT0SRC_HXTAL,RCU_CKOUT0_DIV1);
#endif /* PHY_CLOCK_MCO */

    gpio_ethernet_phy_select(GPIO_ENET_PHY_MII);

#elif defined RMII_MODE
  
    rcu_pll2_config(RCU_PLL2_MUL10);
    rcu_osci_on(RCU_PLL2_CK);
    rcu_osci_stab_wait(RCU_PLL2_CK);
    /* get 50MHz from CK_PLL2 on CKOUT0 pin (PA8) to clock the PHY */
    rcu_ckout0_config(RCU_CKOUT0SRC_CKPLL2,RCU_CKOUT0_DIV1);
    gpio_ethernet_phy_select(GPIO_ENET_PHY_RMII);

#endif

#ifdef MII_MODE

    /* PA0: ETH_MII_CRS */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
    /* PA1: ETH_RX_CLK */    
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    /* PA2: ETH_MDIO */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    /* PA3: ETH_MII_COL */    
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_3);    
    /* PA7: ETH_MII_RX_DV */    
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_7);

    /* PC1: ETH_MDC */
    gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);    
    /* PC2: ETH_MII_TXD2 */    
    gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    /* PC3: ETH_MII_TX_CLK */    
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
    /* PC4: ETH_MII_RXD0 */    
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
    /* PC5: ETH_MII_RXD1 */    
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_5);

//    /* PB0: ETH_MII_RXD2 */
//    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
//    /* PB1: ETH_MII_RXD3 */    
//    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
		
		
		/* PB0: ETH_MII_RXD2 */
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
    /* PB1: ETH_MII_RXD3 */    
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
		
		
    /* PB8: ETH_MII_TXD3 */
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    /* PB10: ETH_MII_RX_ER */    
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
    /* PB11: ETH_MII_TX_EN */    
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
    /* PB12: ETH_MII_TXD0 */
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);
    /* PB13: ETH_MII_TXD1 */    
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);

      
#elif defined RMII_MODE

    /* PA1: ETH_RMII_REF_CLK */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    /* PA2: ETH_MDIO */    
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    /* PA7: ETH_RMII_CRS_DV */    
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_7);

    /* PC1: ETH_MDC */
    gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    /* PC4: ETH_RMII_RXD0 */    
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
    /* PC5: ETH_RMII_RXD1 */    
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_5);

    /* PB11: ETH_RMII_TX_EN */
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
    /* PB12: ETH_RMII_TXD0 */    
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);
    /* PB13: ETH_RMII_TXD1 */    
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);    
    
#endif /* MII_MODE */

}


uint8_t get_phy_addr(void)
{
    int i;
    uint16_t id1, id2;

    for(i=1; i<0x1f; i++)
    {
        enet_phy_write_read(ENET_PHY_READ, i, 0x02, &id1);
        enet_phy_write_read(ENET_PHY_READ, i, 0x03, &id2);
        if(id1 != 0xffff)
        {
            printf("id1:%02x, id2:%02x\r\n", id1, id2);
            printf("type:%d  revison:%d  phy_addr:%02x\r\n",(id2>>4) & 0x3f, id2 & 0x0f, i);
            return i;
        }
        vTaskDelay(5);
    }
    return 0;
}

void set_phy_mode(void)
{
	enet_en_set(1);
	enet_wake_set(1);
//	enet_inh_set(1);
	
	enet_reset_set(0);
	vTaskDelay(100);
	// phy set
//	enet_rxd3_set(0);
//  enet_rxdv_set(0);
	// mode set
//	enet_rxd2_set(0);
//	enet_rxd1_set(0);
//	enet_rxd0_set(0);
//	
	enet_reset_set(1);
	enet_rxer_set(0);
}

bool enet_get_link_status()
{
    uint16_t phy_value = 0U; 

    /* wait for PHY_LINKED_STATUS bit be set */
    enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_value);
    phy_value &= PHY_LINKED_STATUS;  

    return (bool)phy_value;
}

