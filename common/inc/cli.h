#ifndef _MICRO_CLI_H__
#define _MICRO_CLI_H__

#include "common_structs.h"
unsigned int cli_get_hash (char *string, unsigned int prime_number);

unsigned char cli_parse(char *string);

void cli_parse_command(st_uart_string *stream);


#endif
