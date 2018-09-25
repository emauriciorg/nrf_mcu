#include <string.h>
#include <stdio.h>
#include "micro_cli.h"
#include "app_uart.h"
#include "common_structs.h"
#include "command_list.h"
#include "nrf_gpio.h"
#include "../bbn_board.h"
/* 
	implementation of a cli like parser 
	mainly used for debuggin purposes, to tranfers data a simple
	raw string can be applied
*/


#define PRIME_NUMBER 1009//2069
#define PRIME_NUMBER_SUB 167
#define MAX_COMMAND_SIZE 20//

// to do get hash number of each argv
//to decide: hash of the whole string or single argv



unsigned char get_command_indexes(char *index){
return 0;
}



unsigned char cli_gen_params_argc(){
	return 0;
}
unsigned char cli_get_params_argv(){
	return 0;
}


unsigned int cli_get_hash (char *string, unsigned int prime_number){
	char  len=strlen(string);
	char  i;
	unsigned int hash=0;
	for (i=0;i<len;i++ ){
		hash+= (string[i]*(i+1)); 	
	}
	hash=hash%prime_number;
	printf("hash  [%s] %x\n",string,hash );
	return hash;
}

unsigned char cli_find_char(char *string)
{
	
	char *pch;

	pch=(char *)memchr(string,' ',strlen(string));
	
	if(pch)  return pch-string+1;
	
	pch =(char *)memchr(string,'\r',strlen(string));
	
	if(pch)	return pch-string+1;
	

	return 0;
}




unsigned int cli_get_command_id(char * string, unsigned char *index, unsigned int prime_number){
	
	char tmp_command[20];
	memset(tmp_command,0,20);
        
	*index= cli_find_char(string);
	
	if(!(*index)){
		printf("not found\n");
		return 0;
	}

	memcpy(tmp_command, string, (*index)-1 );
 	
 	return  cli_get_hash(tmp_command, prime_number);

}



void cli_gpio_handle(unsigned int *command_id){
	if (command_id[0]==cmd_gpio_hi){
		nrf_gpio_pin_clear(command_id[1]);		

	}
	if (command_id[0]==cmd_gpio_lo){
		nrf_gpio_pin_set(command_id[1]);

	}
}
void cli_blink(unsigned int *command_id){
	if (command_id[0]  == cmd_sub_off){

		if(command_id[1]==cmd_sub_red) 	nrf_gpio_pin_set(LED_RED);
		if(command_id[1]==cmd_sub_blue) 	nrf_gpio_pin_set(LED_BLUE);
		if(command_id[1]==cmd_sub_green) nrf_gpio_pin_set(LED_GREEN);
	}

	if (command_id[0]==cmd_sub_on){
		if(command_id[1]== cmd_sub_red)   nrf_gpio_pin_clear(LED_RED);
		if(command_id[1]== cmd_sub_blue)  nrf_gpio_pin_clear(LED_BLUE);
		if(command_id[1]== cmd_sub_green) nrf_gpio_pin_clear(LED_GREEN);


	}

}

#define COMMAND_LEVELS  4

unsigned char cli_parse(char *string){
	
	unsigned char index=0;
	unsigned int command_id[ COMMAND_LEVELS ];
	//FUNCTION TO EXTRACT ALL THE ARGV!!!!
	command_id[0]= cli_get_command_id(string, &index, PRIME_NUMBER);
	string+=index;
	switch (command_id[0]){
	
	case cmd_turn:	
			
			command_id[1]=cli_get_command_id(string,&index,PRIME_NUMBER_SUB);
 			string+=index;
			command_id[2]=cli_get_command_id(string,&index,PRIME_NUMBER_SUB);
			// a look up table could be applied here since the functions are based on ids
			cli_blink(&command_id[1]);
	break;		
	
	case cmd_gpio : 
			command_id[1]=cli_get_command_id(string,&index,PRIME_NUMBER_SUB);
 			string+=index;
 			command_id[2] =(*string)-'0';
 			if((!command_id[2]) || (command_id[2]>20)) break;
 			cli_gpio_handle(&command_id[1]);
	break;
	case cmd_sample:
	break;	
	
	default:	
			printf("unknow command\n");
	break;


	}



	return 0;
}

void cli_parse_command(st_uart_string *uart_stream){
	
	if(!(uart_stream->pending_parse)) return;
	
	uart_stream->pending_parse=0;
	
	cli_parse(uart_stream->stream);
	memset(uart_stream->stream,0,sizeof(st_uart_string));
}

