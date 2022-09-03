/*****************************************************************
 *****************************************************************
 ***                                                            **
 ***    (C)Copyright 2005-2006, American Megatrends Inc.        **
 ***                                                            **
 ***            All Rights Reserved.                            **
 ***                                                            **
 ***        6145-F, Northbelt Parkway, Norcross,                **
 ***                                                            **
 ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 ***                                                            **
 *****************************************************************
 *****************************************************************
 ******************************************************************
 *
 * Iptables.h
 * .
 *
 *  Author: AMI MegaRAC PM Team
 ******************************************************************/


#define IP_IPV4                             0x00 
#define IPS_IPV4                            0x01
#define PORT                                0x02
#define PORT_RANGE                          0x03
#define IP_IPV4_UNLOCK                      0x04
#define IPS_IPV4_UNLOCK                     0x05
#define PORT_RELEASE                        0x06
#define PORT_RANGE_RELEASE                  0x07
#define FLUSH                               0x08
#define DISABLE_ALL                         0x09
#define REMOVE_DISABLE_ALL                  0x0a

//Time out support
#define IP_IPV4_TIMEOUT                     0x0b
#define IPS_IPV4_TIMEOUT                    0x0c
#define PORT_TIMEOUT                        0x0d
#define PORT_RANGE_TIMEOUT                  0x0e

#define IP_IPV4_TIMEOUT_UNLOCK              0x0f
#define IPS_IPV4_TIMEOUT_UNLOCK             0x10
#define PORT_TIMEOUT_RELEASE                0x11
#define PORT_RANGE_TIMEOUT_RELEASE          0x12

#define DISABLE_ALL_TIMEOUT                 0x13
#define REMOVE_DISABLE_ALL_TIMEOUT          0x14



//Get
#define GET_IPTABLE_COUNT                   0x00
#define GET_ENTRY_INFO                      0x01
#define IS_BLOCK_ALL                        0x02

typedef struct
{
    INT16U yy;
    INT8U  mm;
    INT8U  dd;
}PACKED Date_T;

typedef struct
{
    INT8U hh;
    INT8U mm;
}PACKED Time_T;


typedef struct
{
    Date_T start_date; 
    Time_T start_time;
    Date_T stop_date;
    Time_T stop_time;
} PACKED Timeout_T;

typedef struct
{
    INT8U protocol;
    INT16U Port_NO;
    Timeout_T timeout; 
} PACKED Port;

typedef struct
{
    INT8U protocol;
    INT16U starting_port;
    INT16U closing_port;
    Timeout_T timeout; 
} PACKED RANGE_Prt;

typedef struct
{
    INT8U starting_ip[4];
    INT8U closing_ip[4];
    Timeout_T timeout; 
} PACKED RANGE_Ipv4;

typedef struct 
{
    INT8U ipaddr_ipv4[4];
    Timeout_T timeout; 
} PACKED Ipv4;

typedef struct
{
    INT8U block;		/*1 - IPv4 only ; 2 - IPv6 only ; 3 - Both*/
    Timeout_T timeout; 
}PACKED Block_T;

typedef struct 
{
    INT8U unblock;		  /*1 - IPv4 only ; 2 - IPv6 only ; 3 - Both*/
    Timeout_T timeout; 
}PACKED Unblock_T;

typedef union 
{
    Ipv4 IPAddr_ipv4; 
    RANGE_Ipv4 IPRange_ipv4;
    Port  Port_Data;
    RANGE_Prt Port_Range;
    Block_T Block;
    Unblock_T Unblock;

} FirewallConfUn_T; 

typedef struct
{
    INT8U TYPE;
    INT8U State;
    FirewallConfUn_T Entry_info;
    
} PACKED GetFirewallConf_T;



/* Prototype Declarations */

void *block_ip_ipv4 ( void *, void *);
void *block_ip_range_ipv4 ( void *, void * );
void *block_port ( void *, void * );
void *block_range_ports ( void *, void *);
void *release_range_port ( void *port_range, void * );
void *release_port ( void *port_in, void *);
void *release_range_ip_ipv4 ( void *range_in, void * );
void *release_ip_ipv4 ( void *ip_in, void * );


void *flush_iptables ( void );
void *enable_all ( void *in);
void *block_all ( void *in);

void iptables_save();
void ip6tables_save();


//time out support
void *block_ip_ipv4_timeout ( void *, void *);
void *block_ip_range_ipv4_timeout ( void *, void * );
void *block_port_timeout ( void *, void *);
void *block_range_ports_timeout ( void *, void *);
void *release_range_port_timeout ( void *, void * );
void *release_port_timeout( void *, void *);
void *release_range_ip_ipv4_timeout ( void *, void * );
void *release_ip_ipv4_timeout ( void *, void * );

int validateSingleTimeout(void *,void *);
int validateTimeout(void *);
int validateStartTimeout(void *);



void convert_IPv4_string (INT8U *IP_array, INT8U *IPv4 );
void convert_Time_string (void *Time_array,INT8U *Time);
void convert_Date_string (void *Date_struct,INT8U *Date);


int get_iptable_count ();
int get_iptable_entry ( INT8U *pos, GetFirewallConf_T *pFWc);
int IsBlockAllEnabled();
void *remove_block_all(void *in );
void *remove_block_all_timeout(void *in );

/***********************************/


