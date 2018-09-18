#ifndef  UART_APP_
#define UART_APP_
#include "app_uart.h"
extern void uart_event_handle(app_uart_evt_t * p_event);
extern void uart_init(void);

void msg_dbg(const char * message,uint32_t length);


#endif
