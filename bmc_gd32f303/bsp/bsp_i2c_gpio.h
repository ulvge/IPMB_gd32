#ifndef _BSP_I2C_GPIO_H
#define _BSP_I2C_GPIO_H

#include "project_select.h"
#include <inttypes.h>
#include <stdbool.h>

#define I2CS_WR 0 /* write ctrl bit */
#define I2CS_RD 1 /* read ctrl bit */

/********************* GPIO SIMULATED I2C1 MICRO DEF ***************************/
#define I2CS0_SCL_GPIO_PORT     GPIOB
#define I2CS0_SCL_CLK           RCU_GPIOB
#define I2CS0_SCL_PIN           GPIO_PIN_8

#define I2CS0_SDA_GPIO_PORT     GPIOB
#define I2CS0_SDA_CLK           RCU_GPIOB
#define I2CS0_SDA_PIN           GPIO_PIN_9

#define I2CS0_RETRY_TIMERS      3
/********************* GPIO SIMULATED I2C1 MICRO DEF END***************************/


/********************* GPIO SIMULATED I2C2 MICRO DEF END***************************/

/* simulated i2c1 function declarations */
void i2cs0_init(void);
bool i2cs0_read_bytes(uint8_t devAddr, uint32_t regAddress, uint8_t *_pReadBuf, uint16_t readSize);
bool i2cs0_write_bytes(uint8_t devAddr, const uint8_t *pWriteBuf, uint16_t writeSize);
void i2cs0_set_address(uint8_t _Address);
void i2cs0_Stop(void);

#endif
