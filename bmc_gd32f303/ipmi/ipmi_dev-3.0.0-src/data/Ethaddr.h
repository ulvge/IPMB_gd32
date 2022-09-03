/*****************************************************************
**                                                              **
**    (C)Copyright 2006-2009,American Megatrends Inc.           **
**                                                              **
**            All Rights Reserved.                              **
**                                                              **
**        5555 Oakbrook Pkwy Suite 200, Norcross,               **
**                                                              **
**        Georgia - 30093, USA. Phone-(770)-246-8600.           **
**                                                              **
*****************************************************************/

/************************************************************/
/*  GetEthIndex   function which return the Eth depends on given channel*/
/************************************************************/

extern int SplitEthIfcnames(char *Ifcnames, int BMCInst,int *count);
extern INT8U GetEthIndex(INT8U Channel, int BMCInst);
extern  	INT8U GetLANChannel(INT8U EthIndex,int BMCInst);
extern INT8U GetIfcName(INT8U EthIndex,INT8S *IfcName,int BMCInst);
extern int GetChannelByAddr(char * IPAddr,int BMCInst);
extern int GetChannelByIPv6Addr(char * IPAddr,int BMCInst);
extern INT8U IsLANChannel(INT8U Channel, int BMCInst)  ;
extern int InitDNSConfiguration(int BMCInst);
extern int GetLinkStatus(INT8U Channel, int BMCInst);
extern int GetChEthInfo(INT8U ChannelNum, INT8U *EthIndex,char *IfcName,INT8U *netindex,int BMCInst);
extern int IsBondingActive ( int BMCInst );
extern int GetActiveIfcnameAndNoofIfc(INT8U InterfaceName[][16]);
