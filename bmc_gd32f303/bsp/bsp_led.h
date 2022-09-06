#ifndef __BSP_LED_H_
#define	__BSP_LED_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "project_select.h"
#include <stdbool.h>
#include "main.h"

#define LED1_PIN                         GPIO_PIN_0
#define LED1_GPIO_PORT                   GPIOC
#define LED1_GPIO_CLK                    RCU_GPIOC

  
void led_init(void);
void led1_set(bool status);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_LED_H_ */
