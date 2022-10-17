/*
 ****************************************************************
 **                                                            **
 **    bsp_capture.c
 **                                                            **
 ****************************************************************
******************************************************************/

#include "bsp_capture.h"
#include "OSPort.h"
#include "main.h"

static const CaptureConfig g_captureConfig[] = {
    {FAN_CHANNEL_1, RCU_TIMER0, TIMER0,   TIMER_CH_1,  TIMER_INT_CH1, RCU_GPIOE, GPIOE, GPIO_PIN_11, GPIO_TIMER0_FULL_REMAP},
    {FAN_CHANNEL_2, RCU_TIMER0, TIMER0,   TIMER_CH_2,  TIMER_INT_CH2, RCU_GPIOE, GPIOE, GPIO_PIN_13, GPIO_TIMER0_FULL_REMAP},
};

#define SIZE_CAPTURE_CONFIG     sizeof(g_captureConfig)/sizeof(g_captureConfig[0])
CaptureStruct g_Cap[SIZE_CAPTURE_CONFIG];

static void capture_gpio_config(const CaptureConfig *config)
{
    rcu_periph_clock_enable(config->gpioRcu);
	gpio_init(config->gpioPort, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, config->pin);

    if (config->remap != NULL) {
        rcu_periph_clock_enable(RCU_AF);
	    gpio_pin_remap_config(config->remap, ENABLE);
    }
}

/**
    \brief      configure the TIMER peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void capture_timer_config(const CaptureConfig *config)
{
    /* TIMER2 configuration: input capture mode -------------------
    the external signal is connected to timer0
    the rising edge is used as active edge
    the timer0 CH0CV is used to compute the frequency value
    ------------------------------------------------------------ */
    timer_ic_parameter_struct timer_icinitpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(config->timerRcu);

    //timer_deinit(config->timerPeriph);

    /* TIMERx configuration */
    timer_initpara.prescaler         = (SystemCoreClock / _1M) - 1;  //
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = UINT16_MAX;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(config->timerPeriph, &timer_initpara);

    /* TIMERx  configuration */
    /* TIMERx  input capture configuration */
    timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;
    timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
    timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
    timer_icinitpara.icfilter    = 0x0;
    
    timer_input_capture_config(config->timerPeriph, config->timerCh, &timer_icinitpara);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(config->timerPeriph);
    /* clear channel 0-3 interrupt bit */
	//timer_interrupt_flag_clear(config->timerPeriph, config->timerIntCh | TIMER_INT_CH3);
	/* channel 0-3 interrupt enable */
    timer_interrupt_enable(config->timerPeriph, config->timerIntCh);

    /* TIMER0 counter enable */
    timer_enable(config->timerPeriph);
}
/**
    \brief      configure the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void capture_nvic_configuration(uint32_t periph, uint8_t nvic_irq_pre_priority, uint8_t nvic_irq_sub_priority)
{
 //   nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2); // NVIC_PRIGROUP_PRE1_SUB3
    switch (periph)
    {
    case TIMER0:
        nvic_irq_enable(TIMER0_Channel_IRQn, nvic_irq_pre_priority, nvic_irq_pre_priority);
        nvic_irq_enable(TIMER0_UP_IRQn, nvic_irq_pre_priority, nvic_irq_pre_priority);
        break;  
    case TIMER1:   
        nvic_irq_enable(TIMER1_IRQn, nvic_irq_pre_priority, nvic_irq_pre_priority);
    case TIMER2:   
        nvic_irq_enable(TIMER2_IRQn, nvic_irq_pre_priority, nvic_irq_pre_priority);
        break;  
    default:
        break;
    }
}

static CaptureStruct *capture_getHandlerByName(int num)
{
    for(int32_t i = 0; i < SIZE_CAPTURE_CONFIG; i++)
    {
        CaptureStruct *pCap = &g_Cap[i]; 
        if (pCap->config->fanSensorNum == num){
            return pCap;
        }
    }
    return NULL;
}

CaptureStruct *capture_getHandler(int idx)
{
    for(int32_t i = 0; i < SIZE_CAPTURE_CONFIG; i++)
    {
        CaptureStruct *pCap = &g_Cap[i]; 
        if (pCap->idx == idx){
            return pCap;
        }
    }
    return NULL;
}
int32_t capture_getTotalNum(void)
{
    return SIZE_CAPTURE_CONFIG;
}
void capture_init(void)
{
    for(int32_t i = 0; i < SIZE_CAPTURE_CONFIG; i++)
    {
        CaptureStruct *pCap = &g_Cap[i]; 

        pCap->idx = i;
        pCap->is_start = false;
        pCap->period_cnt = 0;
        pCap->cap_no_update_cnt = 0;
        pCap->total_value = 0;
        pCap->is_valid = false;

        pCap->config = &g_captureConfig[i];   
        capture_gpio_config(pCap->config);
        capture_timer_config(pCap->config);
        capture_nvic_configuration(pCap->config->timerPeriph, 6, 0);
    }
}

void TIMER0_UP_IRQHandler(void)
{
    const static int32_t timerX = TIMER0;
    if(timer_interrupt_flag_get(timerX, TIMER_INT_UP))
	{
		timer_interrupt_flag_clear(timerX, TIMER_INT_UP);
        
        for(int32_t i = 0; i < SIZE_CAPTURE_CONFIG; i++)
        {
            CaptureStruct *pCap = &g_Cap[i];
            if (pCap->config->timerPeriph == timerX) {
                pCap->period_cnt++;
            }
        }
	}
}

void TIMER0_Channel_IRQHandler(void)
{
    const static int32_t timerX = TIMER0;
    for(int32_t i = 0; i < SIZE_CAPTURE_CONFIG; i++)
    {
        CaptureStruct *pCap = &g_Cap[i];
		if(SET == timer_interrupt_flag_get(timerX, pCap->config->timerIntCh)) {
			/* clear channel 0 interrupt bit */
			timer_interrupt_flag_clear(timerX, pCap->config->timerIntCh);

            if(false == pCap->is_start){
                pCap->cap_value_first = timer_channel_capture_value_register_read(timerX, pCap->config->timerCh);
                pCap->is_start = true;
                pCap->period_cnt = 0;
			}else{
                pCap->cap_value_second = timer_channel_capture_value_register_read(timerX, pCap->config->timerCh);
                pCap->is_start = false;
                pCap->cap_no_update_cnt = 0;
                pCap->is_valid = true;
			}
        }
    }
}
bool capture_get_value(unsigned char fanSensorNum, uint32_t* cap_value)
{
    CaptureStruct *pCap = capture_getHandlerByName(fanSensorNum);

    if (pCap == NULL)
    {
        *cap_value = 0xFFFF;
        return false;
    }

    *cap_value = pCap->total_value;
    return true;
}
bool capture_getIsValid(unsigned char fanSensorNum)
{
    CaptureStruct *pCap = capture_getHandlerByName(fanSensorNum);
    if (pCap == NULL)
    {
        return false;
    }

    return pCap->is_valid;
}
