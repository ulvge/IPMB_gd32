/*!
    \file  gd32f20x_enet_eval.h
    \brief the header file of gd32f20x_enet_eval 
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, firmware for GD32F20x
    2017-06-05, V2.0.0, firmware for GD32F20x
*/

#ifndef GD32F20x_ENET_EVAL_H
#define GD32F20x_ENET_EVAL_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "lwip/netif.h"
#include <stdbool.h>

/* function declarations */
/* setup ethernet system(GPIOs, clocks, MAC, DMA, systick) */
bool        enet_hardware_init     (void);
bool        enet_software_init     (void);
uint8_t     get_phy_addr           (void);
void        set_phy_mode           (void);
bool        enet_get_link_status   (void);
	 
	 
#ifdef __cplusplus
}
#endif

#endif /* GD32F20x_ENET_EVAL_H */
