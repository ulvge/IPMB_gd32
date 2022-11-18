#ifndef __EEPROM_H
#define	__EEPROM_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include "Types.h"

#define    EEP_ADDR_SAVE_MODE     0x30


BOOL EEP_WriteData(INT16U writeAddr, INT8U *pWriteData, INT16U wirteLen);
BOOL EEP_ReadData(INT32U readAddr, INT8U *pReadData, INT16U readLen);
BOOL EEP_WriteDataCheckFirst(INT16U writeAddr, INT8U *pWriteData, INT16U wirteLen);

#ifdef __cplusplus
}
#endif

#endif /* __EEPROM_H */

