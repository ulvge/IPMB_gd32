#include "bsp_timer.h"
#include "debug_print.h"
					
#define _CONSTR(a, b) (a##b)
#define CONSTR(a, b) (_CONSTR(a, b))
								   
#define _CONSTRABC(a, b, c) (a##b##c)
#define CONSTRABC(a, b, c) (_CONSTRABC(a, b, c))
 
#define TIMER_NUM 2

uint32_t g_time_run= 0;
static void timer_config(uint32_t timerx, rcu_periph_enum rcu_timer);
static void nvic_config(uint32_t irqN);

void timer_config_init(void)
{                         
	timer_config(CONSTR(TIMER, TIMER_NUM), CONSTR(RCU_TIMER, TIMER_NUM));
	nvic_config(CONSTRABC(TIMER, TIMER_NUM, _IRQn));//  TIMER4_IRQn
}


/**
    \brief      configure the TIMER peripheral interrupt: 1/1s
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void timer_config(uint32_t timerx, rcu_periph_enum rcu_timer)
{
      /* ----------------------------------------------------------------------------
    TIMER1 Configuration: 
    TIMER1CLK = SystemCoreClock/10000 = 12KHz.
    TIMER1 configuration is timing mode, and the timing is 0.2s(12000/12000 = 1s).
    CH0 update rate = TIMER1 counter clock/CH0CV = 20000/4000 = 5Hz.
    ---------------------------------------------------------------------------- */
    timer_oc_parameter_struct timer_ocinitpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(rcu_timer);
  
    timer_deinit(timerx);
    /* initialize TIMER init parameter struct */
    timer_struct_para_init(&timer_initpara);
    /* TIMER1 configuration */
    timer_initpara.prescaler         = SystemCoreClock/12000 - 1;	
    timer_initpara.alignedmode       = TIMER_COUNTER_CENTER_UP;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 12000;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_init(timerx, &timer_initpara);

    /* initialize TIMER channel output parameter struct */
    timer_channel_output_struct_para_init(&timer_ocinitpara);
    /* CH0, CH1 and CH2 configuration in OC timing mode */
    timer_ocinitpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocinitpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocinitpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_channel_output_config(timerx, TIMER_CH_0, &timer_ocinitpara);

    /* CH0 configuration in OC timing mode */
    timer_channel_output_pulse_value_config(timerx, TIMER_CH_0, 2000);
    timer_channel_output_mode_config(timerx, TIMER_CH_0, TIMER_OC_MODE_TIMING);
    timer_channel_output_shadow_config(timerx, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);

    timer_interrupt_enable(timerx, TIMER_INT_UP);
    timer_enable(timerx);
}


/**
    \brief      configure the TIMER1 interrupt
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void nvic_config(uint32_t irqN)
{
    nvic_irq_enable(irqN, 15, 0);
}


/**
  * @brief  This function handles TIMER4_IRQHandler interrupt request.
  * @param  None
  * @retval None
  */
void CONSTRABC(TIMER, TIMER_NUM, _IRQHandler)(void)
{
    if (SET == timer_interrupt_flag_get(CONSTR(TIMER, TIMER_NUM), TIMER_INT_UP))
    {
        g_time_run++;
        timer_interrupt_flag_clear(CONSTR(TIMER, TIMER_NUM), TIMER_INT_UP);
		//printf("g_time_run=%d\n", g_time_run);
    }
}
