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

#define I2C_SLAVE_ADDRESS7  0x86

#define I2C0_SLAVE_ADDRESS7 I2C_SLAVE_ADDRESS7   // i2c0 default slave address
#define I2C1_SLAVE_ADDRESS7 I2C_SLAVE_ADDRESS7   // i2c1 default slave address
#define I2C2_SLAVE_ADDRESS7 I2C_SLAVE_ADDRESS7


#ifdef USE_I2C0_AS_IPMB
    #define ipmb_write(pbuf, len)                i2c0_bytes_write(pbuf, len)
    #define ipmb_set_addr(addr)                  i2c0_set_as_slave_device_addr(addr)
    #define ipmb_read(pbuf, len)                 i2c0_get_slave_device_data(pbuf, len)
#elif USE_I2C1_AS_IPMB
    #define ipmb_write(pbuf, len)                i2c1_bytes_write(pbuf, len)
    #define ipmb_set_addr(addr)                  i2c1_set_as_slave_device_addr(addr)
    #define ipmb_read(pbuf, len)                 i2c1_get_slave_device_data(pbuf, len)
#elif USE_I2C2_AS_IPMB
    #define ipmb_write(pbuf, len)                i2c2_bytes_write(pbuf, len)
    #define ipmb_set_addr(addr)                  i2c2_set_as_slave_device_addr(addr)
    #define ipmb_read(pbuf, len)                 i2c2_get_slave_device_data(pbuf, len)
#endif

//#define I2C0_REMAP

//#define I2C1_INTERRUPT_ENALBE

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

void i2c_channel_init(uint32_t i2cx);
void i2c_int(void);

bool i2c0_bytes_write(const uint8_t* p_buffer, uint16_t len);
bool i2c0_bytes_read(const uint8_t device_addr, const uint8_t read_addr, uint8_t* p_buffer, uint16_t len);
bool i2c0_get_slave_device_data(uint8_t* p_buffer, uint32_t* len);
void i2c0_set_as_slave_device_addr(uint8_t device_addr);


bool i2c1_bytes_write(const uint8_t* p_buffer, uint16_t len);
bool i2c1_bytes_read(const uint8_t device_addr, const uint8_t read_addr, uint8_t* p_buffer, uint16_t len);
bool i2c1_get_slave_device_data(uint8_t* p_buffer, uint32_t* len);
void i2c1_set_as_slave_device_addr(uint8_t device_addr);

#ifdef I2C2
bool i2c2_bytes_write(const uint8_t* p_buffer, uint16_t len);
bool i2c2_bytes_read(const uint8_t device_addr, const uint8_t read_addr, uint8_t* p_buffer, uint16_t len);
bool i2c2_get_slave_device_data(uint8_t* p_buffer, uint32_t* len);
void i2c2_set_as_slave_device_addr(uint8_t device_addr);
#endif

uint8_t get_device_addr(uint8_t bus); 

#ifdef __cplusplus
}
#endif

#endif /* BSP_I2C_H */

