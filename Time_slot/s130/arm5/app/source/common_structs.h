#ifndef COMMON_STRUCTS_H

#define COMMON_STRUCTS_H

#define UART_STRING_MAX_SIZE  32
typedef struct {
	char *ptr_stream;
	char  stream[UART_STRING_MAX_SIZE];
	char  pending_parse;
	char  index;
}st_uart_string; 


#endif
