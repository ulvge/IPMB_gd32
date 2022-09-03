
#ifndef NET_PRINT_H
#define NET_PRINT_H



void netPrintTask(void *arg);

int net_raw_print (const char* format, ...);
int net_hdr_end_print (const char* hdr, const char* end, const char* format, ...);

#endif /* NET_PRINT_H */
