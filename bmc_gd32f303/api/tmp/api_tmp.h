#ifndef __API_TMP_H
#define __API_TMP_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/* function declarations */
bool    tmp_init			(void);
bool    get_tmp_value       (uint8_t channel, uint8_t* tmp);
void    tmpSampleTask       (void *arg) ;

#ifdef __cplusplus
}
#endif

#endif
