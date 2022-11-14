/*!

    Copyright (c) 2020, ZK Inc.

    All rights reserved.
*/

#ifndef BSP_I2C_H
#define BSP_I2C_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

//#define I2C0_REMAP
//#define I2C1_REMAP
//#define I2C2_REMAP

#ifdef I2C0_REMAP
    #define I2C0_SCL_PIN                 GPIO_PIN_8
    #define I2C0_SDA_PIN                 GPIO_PIN_9
    #define I2C0_SCL_GPIO_PORT           GPIOB
    #define I2C0_SCL_GPIO_CLK            RCU_GPIOB
    #define I2C0_SDA_GPIO_PORT           GPIOB
    #define I2C0_SDA_GPIO_CLK            RCU_GPIOB
#else
    #define I2C0_SCL_PIN                 GPIO_PIN_6
    #define I2C0_SDA_PIN                 GPIO_PIN_7
    #define I2C0_SCL_GPIO_PORT           GPIOB
    #define I2C0_SCL_GPIO_CLK            RCU_GPIOB
    #define I2C0_SDA_GPIO_PORT           GPIOB
    #define I2C0_SDA_GPIO_CLK            RCU_GPIOB
#endif

#ifdef I2C1_REMAP
    #define I2C1_SCL_PIN                 GPIO_PIN_0
    #define I2C1_SDA_PIN                 GPIO_PIN_1
    #define I2C1_SCL_GPIO_PORT           GPIOF
    #define I2C1_SCL_GPIO_CLK            RCU_GPIOF
    #define I2C1_SDA_GPIO_PORT           GPIOF
    #define I2C1_SDA_GPIO_CLK            RCU_GPIOF
#else
    #define I2C1_SCL_PIN                 GPIO_PIN_10
    #define I2C1_SDA_PIN                 GPIO_PIN_11
    #define I2C1_SCL_GPIO_PORT           GPIOB
    #define I2C1_SCL_GPIO_CLK            RCU_GPIOB
    #define I2C1_SDA_GPIO_PORT           GPIOB
    #define I2C1_SDA_GPIO_CLK            RCU_GPIOB
#endif

#ifdef I2C2_REMAP
    #define I2C2_SCL_PIN                 GPIO_PIN_8
    #define I2C2_SDA_PIN                 GPIO_PIN_9
    #define I2C2_SCL_GPIO_PORT           GPIOA
    #define I2C2_SCL_GPIO_CLK            RCU_GPIOA
    #define I2C2_SDA_GPIO_PORT           GPIOC
    #define I2C2_SDA_GPIO_CLK            RCU_GPIOC
#else
    #define I2C2_SCL_PIN                 GPIO_PIN_7
    #define I2C2_SDA_PIN                 GPIO_PIN_8
    #define I2C2_SCL_GPIO_PORT           GPIOH
    #define I2C2_SCL_GPIO_CLK            RCU_GPIOH
    #define I2C2_SDA_GPIO_PORT           GPIOH
    #define I2C2_SDA_GPIO_CLK            RCU_GPIOH
#endif

typedef enum
{
    I2C_BUS_0 = 0,
    I2C_BUS_1,
    I2C_BUS_2,
    I2C_BUS_S0 = 3,
} I2C_BUS_NUM;
void i2c_channel_init(uint32_t i2cx);
void i2c_int(void);
bool i2c_write(uint32_t bus, const uint8_t *p_buffer, uint16_t len);
bool i2c_read(uint32_t bus, uint8_t devAddr, uint16_t regAddress, uint8_t *pReadBuf, uint16_t size);

void i2c_set_slave_addr(uint32_t bus, uint8_t device_addr);
void i2c_dualaddr_set(uint32_t i2c_periph, uint8_t dualaddr);
uint8_t i2c_dualaddr_get(uint32_t i2c_periph);

#ifdef __cplusplus
}
#endif

#endif /* BSP_I2C_H */

