#ifndef  UART_APP_
#define UART_APP_

#include <stdint.h>
#include  "common_structs.h"
extern void uart_init(void);
#define SDBG(...)	printf(__VA_ARGS__);
			//SDBG(uart_stream.out_stream,uart_stream.out_len);


//#define NRF_DEBUG_LOG(__ARGS_)
void uart_set_structe(st_uart_string *external_uart_stream);
void uart_check_stream(void);
#endif
