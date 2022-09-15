/*

    project_select.h


*/
#ifndef __PROJECT_SELECT_H
#define	__PROJECT_SELECT_H

/*
cl：互联型产品，stm32f105/107 系列 
vl：超值型产品，stm32f100 系列 
xl：超高密度产品，stm32f101/103 系列
ld：低密度产品，FLASH 小于 64K 
md：中等密度产品，FLASH=64 or 128 
hd：高密度产品，FLASH 大于 128
*/

#ifdef GD32F1x
/*gd32f103VBT6
flash:  64K     0x1 0000
ram:    20k     0x5000=20,480
*/
#include "gd32f10x_it.h"
#include "system_gd32f10x.h"
#endif


#ifdef GD32F2x
#include "gd32f20x_it.h"
#include "system_gd32f20x.h"
//#include "flash/api_flash.h"
#endif




 #ifdef GD32F3x
//#elif GD32F3x
/*gd32f303VGT6
flash:  1024K   0x10 0000
ram:    96k     0x1 8000
*/
#include "gd32f30x_it.h"
#include "system_gd32f30x.h"
//#include "flash/api_flash.h"
#endif


#endif /* __PROJECT_SELECT_H */



