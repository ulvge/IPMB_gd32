#ifndef _BSP_I2C_GPIO_H
#define _BSP_I2C_GPIO_H

#include "project_select.h"
#include <inttypes.h>
#include <stdbool.h>


#define macI2C_WR	0		/* write ctrl bit */
#define macI2C_RD	1		/* read ctrl bit */

/********************* GPIO SIMULATED I2C1 MICRO DEF ***************************/
#define macI2C1_GPIO_PORT	GPIOC			
#define macI2C1_CLK      	RCU_GPIOC	
#define macI2C1_SCL_PIN		GPIO_PIN_7			
#define macI2C1_SDA_PIN		GPIO_PIN_8		

#define macI2C1_SCL_1()  gpio_bit_set(macI2C1_GPIO_PORT, macI2C1_SCL_PIN)		    /* SCL = 1 */
#define macI2C1_SCL_0()  gpio_bit_reset(macI2C1_GPIO_PORT, macI2C1_SCL_PIN)		    /* SCL = 0 */

#define macI2C1_SDA_1()  gpio_bit_set(macI2C1_GPIO_PORT, macI2C1_SDA_PIN)		    /* SDA = 1 */
#define macI2C1_SDA_0()  gpio_bit_reset(macI2C1_GPIO_PORT, macI2C1_SDA_PIN)		    /* SDA = 0 */

#define macI2C1_SDA_READ()  gpio_input_bit_get(macI2C1_GPIO_PORT, macI2C1_SDA_PIN)	/* SDA read */
/********************* GPIO SIMULATED I2C1 MICRO DEF END***************************/


/********************* GPIO SIMULATED I2C2 MICRO DEF ***************************/
#define macI2C2_GPIO_PORT	GPIOE			
#define macI2C2_CLK      	RCU_GPIOE	
#define macI2C2_SCL_PIN		GPIO_PIN_1			
#define macI2C2_SDA_PIN		GPIO_PIN_0		

#define macI2C2_SCL_1()  gpio_bit_set(macI2C2_GPIO_PORT, macI2C2_SCL_PIN)		    /* SCL = 1 */
#define macI2C2_SCL_0()  gpio_bit_reset(macI2C2_GPIO_PORT, macI2C2_SCL_PIN)		    /* SCL = 0 */

#define macI2C2_SDA_1()  gpio_bit_set(macI2C2_GPIO_PORT, macI2C2_SDA_PIN)		    /* SDA = 1 */
#define macI2C2_SDA_0()  gpio_bit_reset(macI2C2_GPIO_PORT, macI2C2_SDA_PIN)		    /* SDA = 0 */

#define macI2C2_SDA_READ()  gpio_input_bit_get(macI2C2_GPIO_PORT, macI2C2_SDA_PIN)	/* SDA read */
/********************* GPIO SIMULATED I2C2 MICRO DEF END***************************/


/* simulated i2c1 function declarations */
uint8_t   simulated_i2c1_CheckDevice    (uint8_t _Address);
bool      simulated_i2c1_read_bytes     (uint8_t dev_addr, uint16_t _usAddress, uint8_t *_pReadBuf, uint16_t _usSize);
bool      simulated_i2c1_write_bytes    (uint8_t dev_addr, uint16_t _usAddress, uint8_t *_pWriteBuf, uint16_t _usSize);

/* simulated i2c2 function declarations */
uint8_t   simulated_i2c2_CheckDevice    (uint8_t _Address);
bool      simulated_i2c2_read_bytes     (uint8_t dev_addr, uint16_t _usAddress, uint8_t *_pReadBuf, uint16_t _usSize);
bool      simulated_i2c2_write_bytes    (uint8_t dev_addr, uint16_t _usAddress, uint8_t *_pWriteBuf, uint16_t _usSize);


#endif

