#ifndef __API_MAC_5023_H
#define	__API_MAC_5023_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include <stdbool.h>
#include "Types.h"
#include "libpmb.h"
#include "project_select.h"
#include "debug_print.h"

bool MAC5023_init(void);
void MAC5023_Sample(void);

#ifdef __cplusplus
}
#endif

#endif /* __API_MAC_5023_H */

