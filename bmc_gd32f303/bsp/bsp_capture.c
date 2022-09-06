#include "bsp_capture.h"
#include "OSPort.h"
#include "main.h"

CapPreiodCapInfo_T g_timer_cap_info[4] = {0};

const uint32_t timer_int_chx[] = {TIMER_INT_CH0, TIMER_INT_CH1, TIMER_INT_CH2, TIMER_INT_CH3};
const uint32_t timer_ch_chx[] = {TIMER_CH_0, TIMER_CH_1, TIMER_CH_2, TIMER_CH_3};

static void capture_timer1_gpio_config(void);
static void capture_timer1_config(void);
static void capture_timer11_gpio_config(void);
static void capture_timer11_config(void);
#if 0
static void capture_timer3_gpio_config(void);
static void capture_timer3_config(void);
#endif
static void capture_nvic_configuration(void);


void capture_init(void)
{
	capture_timer1_gpio_config();
	capture_timer1_config();
  capture_timer11_gpio_config();
	capture_timer11_config();
  // capture_timer3_gpio_config();
	// capture_timer3_config();
  capture_nvic_configuration();
}

bool capture_get_value(unsigned char channel, uint32_t* cap_value)
{
	if(channel >= sizeof(g_timer_cap_info)/sizeof(g_timer_cap_info[0]))
  {
		*cap_value = 0xFFFF; 
    return false;
  }
	
	*cap_value = g_timer_cap_info[channel].total_value;
	
	return true;
}


/**
    \brief      configure the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void capture_nvic_configuration(void)
{
 //   nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2); // NVIC_PRIGROUP_PRE1_SUB3
    nvic_irq_enable(TIMER1_IRQn, 6, 0);
    //nvic_irq_enable(TIMER7_BRK_TIMER11_IRQn, 6, 0);
    // nvic_irq_enable(TIMER3_IRQn, 6, 0);
}


/**
    \brief      configure the GPIO ports
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void capture_timer1_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);

    /*configure PA0-3 (timer1 CH0 CH3) as alternate function*/
	gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
    //gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
}

/**
    \brief      configure the TIMER peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void capture_timer1_config(void)
{
    /* TIMER2 configuration: input capture mode -------------------
    the external signal is connected to timer0
    the rising edge is used as active edge
    the timer0 CH0CV is used to compute the frequency value
    ------------------------------------------------------------ */
    timer_ic_parameter_struct timer_icinitpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);

    timer_deinit(TIMER1);

    /* TIMER0 configuration */
    timer_initpara.prescaler         = 119;  // 119 ->120M
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 65535;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1,&timer_initpara);

    /* TIMER1  configuration */
    /* TIMER1 CH0 CH3 input capture configuration */
    timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;
    timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
    timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
    timer_icinitpara.icfilter    = 0x0;
    
		timer_input_capture_config(TIMER1,TIMER_CH_0,&timer_icinitpara);
    timer_input_capture_config(TIMER1,TIMER_CH_3,&timer_icinitpara);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER1);
    /* clear channel 0-3 interrupt bit */
		timer_interrupt_flag_clear(TIMER1,TIMER_INT_CH0 | TIMER_INT_CH3);
		/* channel 0-3 interrupt enable */
    timer_interrupt_enable(TIMER1,TIMER_INT_CH0 | TIMER_INT_CH3);

    /* TIMER0 counter enable */
    timer_enable(TIMER1);
}


/**
    \brief      configure the GPIO ports
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void capture_timer11_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);

    /*configure PB14-15 (timer11 CH0 CH1) as alternate function*/
		gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_14);
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_15);	
}

/**
    \brief      configure the TIMER peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void capture_timer11_config(void)
{
    /* TIMER2 configuration: input capture mode -------------------
    the external signal is connected to timer0
    the rising edge is used as active edge
    the timer0 CH0CV is used to compute the frequency value
    ------------------------------------------------------------ */
    timer_ic_parameter_struct timer_icinitpara;
    timer_parameter_struct timer_initpara;

    //rcu_periph_clock_enable(RCU_TIMER11);

    timer_deinit(TIMER11);

    /* TIMER0 configuration */
    timer_initpara.prescaler         = 119;  // 119 ->120M
    timer_initpara.alignedmode       = TIMER_COUNTER_CENTER_UP;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 65535;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER11,&timer_initpara);

    /* TIMER1  configuration */
    /* TIMER1 CH0 CH3 input capture configuration */
    timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;
    timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
    timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
    timer_icinitpara.icfilter    = 0x0;
    
		timer_input_capture_config(TIMER11,TIMER_CH_0,&timer_icinitpara);
    timer_input_capture_config(TIMER11,TIMER_CH_1,&timer_icinitpara);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER11);
    /* clear channel 0-3 interrupt bit */
		timer_interrupt_flag_clear(TIMER11,TIMER_INT_CH0 | TIMER_INT_CH1);
		/* channel 0-3 interrupt enable */
    timer_interrupt_enable(TIMER11,TIMER_INT_CH0 | TIMER_INT_CH1);

    /* TIMER0 counter enable */
    timer_enable(TIMER11);
}

#if 0
/**
    \brief      configure the GPIO ports
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void capture_timer3_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_AF);

    /*configure PD12-13-14-15 (timer3 CH0 CH1 CH2 CH3) as alternate function*/
    gpio_init(GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_12);
    gpio_init(GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_13);	
		gpio_init(GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_14);
    gpio_init(GPIOD, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_15);	

    gpio_pin_remap_config(GPIO_TIMER3_REMAP, ENABLE);
}

/**
    \brief      configure the TIMER peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void capture_timer3_config(void)
{
    /* TIMER2 configuration: input capture mode -------------------
    the external signal is connected to timer0
    the rising edge is used as active edge
    the timer0 CH0CV is used to compute the frequency value
    ------------------------------------------------------------ */
    timer_ic_parameter_struct timer_icinitpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER3);

    timer_deinit(TIMER3);

    /* TIMER0 configuration */
    timer_initpara.prescaler         = 119;  // 119 ->120M
    timer_initpara.alignedmode       = TIMER_COUNTER_CENTER_UP;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 65535;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER3,&timer_initpara);

    /* TIMER1  configuration */
    /* TIMER1 CH0 CH3 input capture configuration */
    timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;
    timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
    timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
    timer_icinitpara.icfilter    = 0x0;
    
		timer_input_capture_config(TIMER3,TIMER_CH_0,&timer_icinitpara);
    timer_input_capture_config(TIMER3,TIMER_CH_1,&timer_icinitpara);
    timer_input_capture_config(TIMER3,TIMER_CH_2,&timer_icinitpara);
    timer_input_capture_config(TIMER3,TIMER_CH_3,&timer_icinitpara);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER3);
    /* clear channel 0-3 interrupt bit */
		timer_interrupt_flag_clear(TIMER3,TIMER_INT_CH0 | TIMER_INT_CH1 | TIMER_INT_CH2 | TIMER_INT_CH3);
		/* channel 0-3 interrupt enable */
    timer_interrupt_enable(TIMER3,TIMER_INT_CH0 | TIMER_INT_CH1 | TIMER_INT_CH2 | TIMER_INT_CH3);

    /* TIMER0 counter enable */
    timer_enable(TIMER3);
}
#endif

void TIMER1_IRQHandler(void)
{
	if(timer_interrupt_flag_get(TIMER1, TIMER_INT_UP))
	{
		timer_interrupt_flag_clear(TIMER1, TIMER_INT_UP);
    g_timer_cap_info[0].period_cnt++;
    g_timer_cap_info[1].period_cnt++;
	}	                             

  if(SET == timer_interrupt_flag_get(TIMER1,TIMER_INT_CH0)){
    /* clear channel 0 interrupt bit */
    timer_interrupt_flag_clear(TIMER1,TIMER_INT_CH0);

    if(false == g_timer_cap_info[0].is_start){
        g_timer_cap_info[0].cap_value_first = timer_channel_capture_value_register_read(TIMER1,TIMER_INT_CH0);
        g_timer_cap_info[0].is_start = true;
        g_timer_cap_info[0].period_cnt = 0;
    }else{
        g_timer_cap_info[0].cap_value_second = timer_channel_capture_value_register_read(TIMER1,TIMER_INT_CH0);
        g_timer_cap_info[0].is_start = false;
        g_timer_cap_info[0].cap_no_update_cnt = 0;
        g_timer_cap_info[0].is_valid = true;
    }
  }

  if(SET == timer_interrupt_flag_get(TIMER1,TIMER_INT_CH1)){
    /* clear channel 0 interrupt bit */
    timer_interrupt_flag_clear(TIMER1,TIMER_INT_CH1);

    if(false == g_timer_cap_info[1].is_start){
        g_timer_cap_info[1].cap_value_first = timer_channel_capture_value_register_read(TIMER1,TIMER_INT_CH1);
        g_timer_cap_info[1].is_start = true;
        g_timer_cap_info[1].period_cnt = 0;
    }else{
        g_timer_cap_info[1].cap_value_second = timer_channel_capture_value_register_read(TIMER1,TIMER_INT_CH1);
        g_timer_cap_info[1].is_start = false;
        g_timer_cap_info[1].cap_no_update_cnt = 0;
        g_timer_cap_info[1].is_valid = true;
    }
  }
}

void TIMER7_BRK_TIMER11_IRQHandler(void)
{
	if(timer_interrupt_flag_get(TIMER11, TIMER_INT_UP))
	{
		timer_interrupt_flag_clear(TIMER11, TIMER_INT_UP);
    g_timer_cap_info[2].period_cnt++;
    g_timer_cap_info[3].period_cnt++;
	}	  

  if(SET == timer_interrupt_flag_get(TIMER11,TIMER_INT_CH0)){
    /* clear channel 0 interrupt bit */
    timer_interrupt_flag_clear(TIMER11,TIMER_INT_CH0);

    if(false == g_timer_cap_info[3].is_start){
        g_timer_cap_info[3].cap_value_first = timer_channel_capture_value_register_read(TIMER11,TIMER_INT_CH0);
        g_timer_cap_info[3].is_start = true;
        g_timer_cap_info[3].period_cnt = 0;
    }else{
        g_timer_cap_info[3].cap_value_second = timer_channel_capture_value_register_read(TIMER11,TIMER_INT_CH0);
        g_timer_cap_info[3].is_start = false;
        g_timer_cap_info[3].cap_no_update_cnt = 0;
        g_timer_cap_info[3].is_valid = true;
    }
  }

  if(SET == timer_interrupt_flag_get(TIMER11,TIMER_INT_CH1)){
    /* clear channel 0 interrupt bit */
    timer_interrupt_flag_clear(TIMER11,TIMER_INT_CH1);

    if(false == g_timer_cap_info[2].is_start){
        g_timer_cap_info[2].cap_value_first = timer_channel_capture_value_register_read(TIMER11,TIMER_INT_CH1);
        g_timer_cap_info[2].is_start = true;
        g_timer_cap_info[2].period_cnt = 0;
    }else{
        g_timer_cap_info[2].cap_value_second = timer_channel_capture_value_register_read(TIMER11,TIMER_INT_CH1);
        g_timer_cap_info[2].is_start = false;
        g_timer_cap_info[2].cap_no_update_cnt = 0;
        g_timer_cap_info[2].is_valid = true;
    }
  }
}

#if 0
void TIMER3_IRQHandler(void)
{
	int i = 0;

	if(timer_interrupt_flag_get(TIMER3, TIMER_INT_UP))
	{
		timer_interrupt_flag_clear(TIMER3, TIMER_INT_UP);
    g_timer_cap_info[4].period_cnt++;
    g_timer_cap_info[5].period_cnt++;
    g_timer_cap_info[6].period_cnt++;
    g_timer_cap_info[7].period_cnt++;
	}	                             

	for(i=0; i<4; i++)
	{
		if(SET == timer_interrupt_flag_get(TIMER3,timer_int_chx[i])){
			/* clear channel 0 interrupt bit */
			timer_interrupt_flag_clear(TIMER3,timer_int_chx[i]);

			if(false == g_timer_cap_info[4+i].is_start){
					g_timer_cap_info[4+i].cap_value_first = timer_channel_capture_value_register_read(TIMER3,timer_ch_chx[i]);
					g_timer_cap_info[4+i].is_start = true;
					g_timer_cap_info[4+i].period_cnt = 0;
			}else{
					g_timer_cap_info[4+i].cap_value_second = timer_channel_capture_value_register_read(TIMER3,timer_ch_chx[i]);
          g_timer_cap_info[4+i].is_start = false;
          g_timer_cap_info[4+i].cap_no_update_cnt = 0;
          g_timer_cap_info[4+i].is_valid = true;
			}
		}
	}
}
#endif


