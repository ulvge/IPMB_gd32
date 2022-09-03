#include "bsp_pwm.h"
#include <stdbool.h>
#include "main.h"

static void pwm_timer0_gpio_config(void);
static void pwm_timer0_config(void);
static void pwm_timer7_gpio_config(void);
static void pwm_timer7_config(void);
static void pwm_timer8_gpio_config(void);
static void pwm_timer8_config(void);

void pwm_init (void)
{
	pwm_timer0_gpio_config();
	pwm_timer0_config();

  // pwm_timer7_gpio_config();
	// pwm_timer7_config();
  // pwm_timer8_gpio_config();
	// pwm_timer8_config();
}

/**
    \brief      configure the GPIO ports
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void pwm_timer0_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_AF);

    /*Configure (timer0 CH0 CH1 CH2 CH3) as alternate function*/
		gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_14);
	
	  gpio_pin_remap_config(GPIO_TIMER0_FULL_REMAP, ENABLE);
}

/**
    \brief      configure the TIMER peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void pwm_timer0_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER0);

    timer_deinit(TIMER0);

    /* TIMER3 configuration */
    timer_initpara.prescaler         = 0; // 120MHz
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = FAN_PWM_MAX_DUTY_VALUE; // 40ms = 25Khz 40000 
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER0,&timer_initpara);

    /* CH1,CH2 and CH3 configuration in PWM mode1 */
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

		timer_channel_output_config(TIMER0,TIMER_CH_0,&timer_ocintpara);
    timer_channel_output_config(TIMER0,TIMER_CH_1,&timer_ocintpara);
    timer_channel_output_config(TIMER0,TIMER_CH_2,&timer_ocintpara);
    timer_channel_output_config(TIMER0,TIMER_CH_3,&timer_ocintpara);
		
	  /* CH0 configuration in PWM mode1,duty cycle 75% */
    timer_channel_output_pulse_value_config(TIMER0,TIMER_CH_0,0);
    timer_channel_output_mode_config(TIMER0,TIMER_CH_0,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER0,TIMER_CH_0,TIMER_OC_SHADOW_DISABLE);

    /* CH1 configuration in PWM mode1,duty cycle 25% */
    timer_channel_output_pulse_value_config(TIMER0,TIMER_CH_1,0);
    timer_channel_output_mode_config(TIMER0,TIMER_CH_1,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER0,TIMER_CH_1,TIMER_OC_SHADOW_DISABLE);

    /* CH2 configuration in PWM mode1,duty cycle 50% */
    timer_channel_output_pulse_value_config(TIMER0,TIMER_CH_2,0);
    timer_channel_output_mode_config(TIMER0,TIMER_CH_2,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER0,TIMER_CH_2,TIMER_OC_SHADOW_DISABLE);

    /* CH3 configuration in PWM mode1,duty cycle 75% */
    timer_channel_output_pulse_value_config(TIMER0,TIMER_CH_3,0);
    timer_channel_output_mode_config(TIMER0,TIMER_CH_3,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER0,TIMER_CH_3,TIMER_OC_SHADOW_DISABLE);

    timer_primary_output_config(TIMER0, ENABLE);  // special for primary timer
		
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER0);
    /* auto-reload preload enable */
    timer_enable(TIMER0);
}

/**
    \brief      configure the GPIO ports
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void pwm_timer7_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_AF);

    /*Configure PC7 PC8 (TIMER7 CH1-PWM4 CH2-PWM5) as alternate function*/
    gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
}

/**
    \brief      configure the TIMER peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void pwm_timer7_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER7);

    timer_deinit(TIMER7);

    /* TIMER1 configuration */
    timer_initpara.prescaler         = 0; // 120MHz
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = FAN_PWM_MAX_DUTY_VALUE; // 40ms = 25Khz
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER7,&timer_initpara);

    /* CH1,CH2 and CH3 configuration in PWM mode1 */
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

    timer_channel_output_config(TIMER7,TIMER_CH_1,&timer_ocintpara);
    timer_channel_output_config(TIMER7,TIMER_CH_2,&timer_ocintpara);

    /* CH1 configuration in PWM mode1,duty cycle 0% */
    timer_channel_output_pulse_value_config(TIMER7,TIMER_CH_1,0);
    timer_channel_output_mode_config(TIMER7,TIMER_CH_1,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER7,TIMER_CH_1,TIMER_OC_SHADOW_DISABLE);

    /* CH2 configuration in PWM mode1,duty cycle 0% */
    timer_channel_output_pulse_value_config(TIMER7,TIMER_CH_2,0);
    timer_channel_output_mode_config(TIMER7,TIMER_CH_2,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER7,TIMER_CH_2,TIMER_OC_SHADOW_DISABLE);

		
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER7);
    /* auto-reload preload enable */
    timer_enable(TIMER7);
}

/**
    \brief      configure the GPIO ports
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void pwm_timer8_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_AF);

    /*Configure PE5 PE6 (TIMER8 CH0-PWM6 CH1-PWM7) as alternate function*/
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
}

/**
    \brief      configure the TIMER peripheral
    \param[in]  none
    \param[out] none
    \retval     none
  */
static void pwm_timer8_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER8);

    timer_deinit(TIMER8);

    /* TIMER1 configuration */
    timer_initpara.prescaler         = 0; // 120MHz
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = FAN_PWM_MAX_DUTY_VALUE; // 40ms = 25Khz
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER8,&timer_initpara);

    /* CH1,CH2 and CH3 configuration in PWM mode1 */
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

    timer_channel_output_config(TIMER8,TIMER_CH_0,&timer_ocintpara);
    timer_channel_output_config(TIMER8,TIMER_CH_1,&timer_ocintpara);

    /* CH1 configuration in PWM mode1,duty cycle 0% */
    timer_channel_output_pulse_value_config(TIMER8,TIMER_CH_0,0);
    timer_channel_output_mode_config(TIMER8,TIMER_CH_0,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER8,TIMER_CH_0,TIMER_OC_SHADOW_DISABLE);

    /* CH2 configuration in PWM mode1,duty cycle 0% */
    timer_channel_output_pulse_value_config(TIMER8,TIMER_CH_1,0);
    timer_channel_output_mode_config(TIMER8,TIMER_CH_1,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER8,TIMER_CH_1,TIMER_OC_SHADOW_DISABLE);

		
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER8);
    /* auto-reload preload enable */
    timer_enable(TIMER8);
}

void pwm_set_duty_raw(unsigned char fannum, uint32_t value)
{
	switch(fannum)
	{
		case 0:
			timer_channel_output_pulse_value_config(TIMER0, 0, value);
			break;
		case 1:
			timer_channel_output_pulse_value_config(TIMER0, 1, value);
			break;
		case 2:
			timer_channel_output_pulse_value_config(TIMER0, 2, value);
			break;
		case 3:
			timer_channel_output_pulse_value_config(TIMER0, 3, value);
			break;
    // case 4:
		// 	timer_channel_output_pulse_value_config(TIMER7, TIMER_CH_1, value);
		// 	break;
		// case 5:
		// 	timer_channel_output_pulse_value_config(TIMER7, TIMER_CH_2, value);
		// 	break;
		// case 6:
		// 	timer_channel_output_pulse_value_config(TIMER8, TIMER_CH_0, value);
		// 	break;
		// case 7:
		// 	timer_channel_output_pulse_value_config(TIMER8, TIMER_CH_1, value);
		// 	break;
		default:
			break;
	}
}

void pwm_set_duty_percent(unsigned char fannum, unsigned char percent)
{
  uint32_t duty_value = 0;

  duty_value = percent*FAN_PWM_MAX_DUTY_VALUE/100;
  pwm_set_duty_raw(fannum, duty_value);
}
