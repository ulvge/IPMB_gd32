#ifndef __BSP_GPIO_H_
#define	__BSP_GPIO_H_


#include "project_select.h"
#include <stdbool.h>
#include "main.h"


#define ENET_EN_PIN                           GPIO_PIN_1
#define ENET_EN_GPIO_PORT                     GPIOD
#define ENET_EN_GPIO_CLK                      RCU_GPIOD

#define ENET_INH_PIN                          GPIO_PIN_2
#define ENET_INH_GPIO_PORT                    GPIOD
#define ENET_INH_GPIO_CLK                     RCU_GPIOD

#define ENET_WAKE_PIN                         GPIO_PIN_3
#define ENET_WAKE_GPIO_PORT                   GPIOD
#define ENET_WAKE_GPIO_CLK                    RCU_GPIOD

#define ENET_RESET_PIN                        GPIO_PIN_0
#define ENET_RESET_GPIO_PORT                  GPIOD
#define ENET_RESET_GPIO_CLK                   RCU_GPIOD

#define ENET_RXD0_PIN                         GPIO_PIN_4
#define ENET_RXD0_GPIO_PORT                   GPIOC
#define ENET_RXD0_GPIO_CLK                    RCU_GPIOC

#define ENET_RXD1_PIN                         GPIO_PIN_5
#define ENET_RXD1_GPIO_PORT                   GPIOC
#define ENET_RXD1_GPIO_CLK                    RCU_GPIOC

#define ENET_RXD2_PIN                         GPIO_PIN_0
#define ENET_RXD2_GPIO_PORT                   GPIOB
#define ENET_RXD2_GPIO_CLK                    RCU_GPIOB

#define ENET_RXD3_PIN                         GPIO_PIN_1
#define ENET_RXD3_GPIO_PORT                   GPIOB
#define ENET_RXD3_GPIO_CLK                    RCU_GPIOB

#define ENET_RXDV_PIN                         GPIO_PIN_7
#define ENET_RXDV_GPIO_PORT                   GPIOA
#define ENET_RXDV_GPIO_CLK                    RCU_GPIOA

//-------------------
#define ENET_RXER_PIN                         GPIO_PIN_10
#define ENET_RXER_GPIO_PORT                   GPIOB
#define ENET_RXER_GPIO_CLK                    RCU_GPIOB

  
void      enet_ctrl_gpio_init     (void);

void enet_en_set(bool flag);
void enet_inh_set(bool flag);
void enet_wake_set(bool flag);
void enet_reset_set(bool flag);
void enet_rxd0_set(bool flag);
void enet_rxd1_set(bool flag);
void enet_rxd2_set(bool flag);
void enet_rxd3_set(bool flag);
void enet_rxdv_set(bool flag);

void enet_rxer_set(bool flag);

#endif /* __BSP_GPIO_H_ */
