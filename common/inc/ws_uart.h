#ifndef  UART_APP_
#define UART_APP_

#include <stdint.h>
#include  "common_structs.h"
#define WS_DBG(...)	printf(__VA_ARGS__);

void ws_uart_init(void);
void ws_uart_check_stream(void);
char ws_uart_pending_debug_packet(void);
void ws_uart_flush_debug_packet(void);
char *ws_uart_get_debug_packet(void);

#endif
