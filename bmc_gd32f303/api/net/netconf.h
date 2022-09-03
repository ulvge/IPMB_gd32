/*!
    \file  netconf.h
    \brief the header file of netconf 
*/

/*
    Copyright (C) 2017 GigaDevice

    2015-07-15, V1.0.0, demo for GD32F20x
    2017-06-05, V2.0.0, demo for GD32F20x
*/

#ifndef NETCONF_H
#define NETCONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* function declarations */
/* initializes the LwIP stack */
void lwip_stack_init(void);
/*initializes the LwIP ip and gateway*/
void lwip_ip_init(void);
/* dhcp_task */
void dhcp_task(void * pvParameters);
	 
#ifdef __cplusplus
}
#endif

#endif /* NETCONF_H */
