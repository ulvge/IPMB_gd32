#include "bsp_pwm.h"
#include <stdbool.h>
#include "main.h"

void pwm_timer_gpio_config(const PwmChannleConfig *config)
{
    rcu_periph_clock_enable(config->gpioRcu);
	gpio_init(config->gpioPort, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, config->pin);

    if (config->remap != NULL) {
        rcu_periph_clock_enable(RCU_AF);
	    gpio_pin_remap_config(config->remap, ENABLE);
    }
}

void pwm_timer_config(const PwmChannleConfig *config)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(config->timerRcu);

    timer_deinit(config->timerPeriph);

    /* TIMER1 configuration */
    timer_initpara.prescaler         = 0; // 120MHz
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = FAN_PWM_MAX_DUTY_VALUE;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(config->timerPeriph, &timer_initpara);

    /* CH1,CH2 and CH3 configuration in PWM mode1 */
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

    timer_channel_output_config(config->timerPeriph, config->timerCh ,&timer_ocintpara);

    /* CH1 configuration in PWM mode1,duty cycle 0% */
    timer_channel_output_pulse_value_config(config->timerPeriph, config->timerCh,0);
    timer_channel_output_mode_config(config->timerPeriph, config->timerCh, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(config->timerPeriph, config->timerCh, TIMER_OC_SHADOW_DISABLE);
		
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(config->timerPeriph);
    /* auto-reload preload enable */
    timer_enable(config->timerPeriph);
}






