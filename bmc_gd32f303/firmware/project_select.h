/*

    project_select.h


*/
#ifndef __PROJECT_SELECT_H
#define	__PROJECT_SELECT_H


#ifdef GD32F1x
/*gd32f103VBT6
flash:  64K     0x1 0000
ram:    20k     0x5000
*/
#include "gd32f10x_it.h"
#include "system_gd32f10x.h"
#endif





#ifdef GD32F2x
#include "gd32f20x_it.h"
#include "system_gd32f20x.h"
#endif




 #ifdef GD32F3x
/*gd32f303VGT6
flash:  1024K   0x10 0000
ram:    96k     0x1 8000
*/
#include "gd32f30x_it.h"
#include "system_gd32f30x.h"
#endif


#endif /* __PROJECT_SELECT_H */



