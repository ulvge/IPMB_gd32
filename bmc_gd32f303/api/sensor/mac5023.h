#ifndef __API_MAC_5023_H
#define	__API_MAC_5023_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include "Types.h"
#include "pmbus.h"
#include "project_select.h"
#include "debug_print.h"

bool MAC5023_Sample(UINT8 devIndex, UINT8 cmd, float *humanVal, UINT8 *ipmbVal);

#ifdef __cplusplus
}
#endif

#endif /* __API_MAC_5023_H */

