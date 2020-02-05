#ifndef _MICRO_CLI_H__
#define _MICRO_CLI_H__


unsigned int cli_get_hash (char *string, unsigned int prime_number);

unsigned char cli_parse_debug_command(char *string);

void cli_execute_debug_command(void);


#endif
