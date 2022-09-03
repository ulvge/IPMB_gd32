#ifndef __BSP_TIMER_H
#define	__BSP_TIMER_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "project_select.h"
#include <stdio.h>
#include <stdbool.h>

void timer_config_init(void);
void timer_config(void);
void nvic_config(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TIMER_H */

