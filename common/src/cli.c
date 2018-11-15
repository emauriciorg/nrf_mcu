/** 
******************************************************************************
* \file    cli.c
* \brief    Entry point file.
******************************************************************************
*/
#include <string.h>
#include <stdio.h>
#include "../inc/cli.h"
#include "app_uart.h"
#include "../inc/common_structs.h"
#include "../inc/command_list.h"
#include "nrf_gpio.h"
#include "../inc/bbn_board.h"
#include "../inc/aes_app.h"
#include "../inc/ble_app.h"
#include "../inc/cafe.h"

#define TWI_DEBUG_CLI
#ifdef TWI_DEBUG_CLI
	#include "../inc/accelerometer_i2c.h"
	#include "app_twi.h"
	extern const nrf_drv_twi_t m_twi_global_accelerometer;
	extern volatile bool m_xfer_done;
	extern uint8_t regW,regR;
	extern uint8_t waiting_for_ack_response,waiting_for_reading_response;    
	extern uint8_t i2c_mode;

	uint8_t err_code;
#endif
/* 
	implementation of a cli like parser 
	mainly used for debuggin purposes, to tranfers data a simple
	raw string can be applied
*/

#define DEBUG_CLI_AES
#ifdef DEBUG_CLI_AES
	#define CLI_OUT(...)  printf(__VA_ARGS__)
#else
	#define CLI_OUT(...)
#endif

#define PRIME_NUMBER 1009//2069
#define PRIME_NUMBER_SUB 167
#define MAX_COMMAND_SIZE 20//

// to do get hash number of each argv
//to decide: hash of the whole string or single argv, find a better hash algorithm 



unsigned int cli_get_hash (char *string, unsigned int prime_number){
	char  len=strlen(string);
	char  i;
	unsigned int hash=0;
	for (i=0;i<len;i++ ){
		hash+= (string[i]*(i+1)); 	
	}
	hash=hash%prime_number;
	
	CLI_OUT("[%s][%x]  ",string,hash );
	return hash;
}

unsigned char cli_find_char(char *string)
{
	
	char *pch;

	pch=(char *)memchr(string,' ',strlen(string));
	
	if (pch)  return pch-string+1;
	
	pch =(char *)memchr(string,'\r',strlen(string));
	
	if (pch)	return pch-string+1;
	

	return 0;
}




unsigned int cli_get_command_id(char * string, unsigned char *index, unsigned int prime_number){
	
	char tmp_command[20];
	memset(tmp_command,0,20);
        
	*index= cli_find_char(string);
	
	if (!(*index)){
		CLI_OUT("not found\n");
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


void cli_blink(unsigned int *command_id)
{
	
	if (command_id[0]  == cmd_sub_off){

		if (command_id[1]==cmd_sub_red) 	 nrf_gpio_pin_set(LED_RED);
		if (command_id[1]==cmd_sub_blue)  nrf_gpio_pin_set(LED_BLUE);
		if (command_id[1]==cmd_sub_green) nrf_gpio_pin_set(LED_GREEN);
	}

	if (command_id[0]==cmd_sub_on){
		if (command_id[1]== cmd_sub_red)   nrf_gpio_pin_clear(LED_RED);
		if (command_id[1]== cmd_sub_blue)  nrf_gpio_pin_clear(LED_BLUE);
		if (command_id[1]== cmd_sub_green) nrf_gpio_pin_clear(LED_GREEN);
	}

}



//010   0A0    ,  16
uint8_t cli_ascii_charhex_to_hex( char  hex_character){
	
	if ( hex_character <= 'F' && hex_character >= 'A') return (hex_character -'A')+10;

	if ( hex_character <= 'f' && hex_character >= 'a') return (hex_character -'f')+10;

	if ( hex_character <= '9' && hex_character >= '0') return (hex_character -'0');

	return 0;
}

uint16_t cli_ascii_streamhex_to_hex(char *stream_pointer, uint8_t stream_length){
	uint16_t hex_result=0;

	while(stream_length &&  ( (stream_pointer[stream_length]) != 0) ){
//		printf("[%x][%x]->[%x]\n\n",stream_pointer+stream_length,stream_length,cli_ascii_charhex_to_hex( *(stream_pointer) ) );	
		hex_result= ( hex_result * 0x10	) + ( cli_ascii_charhex_to_hex(*stream_pointer));
		//printf("*%x*",hex_result);
		stream_pointer++;
		stream_length--;
	}
	return hex_result;
}



#define COMMAND_LEVELS  4

static uint8_t  uncripted_data[40];
static uint8_t  cripted_data  [40];

/*command structure command argv1 arg2 argv3 \n*/
unsigned char cli_parse(char *argv)
{
	
	unsigned char index=0;
	unsigned int command_id[ COMMAND_LEVELS ];
	memset(command_id,0,sizeof(command_id));
	
	command_id[0]= cli_get_command_id(argv, &index, PRIME_NUMBER);
	argv+=index;
	
	switch (command_id[0]){
	
	case cmd_turn:	
			
			command_id[1]=cli_get_command_id(argv,&index,PRIME_NUMBER_SUB);
 			argv+=index;
			command_id[2]=cli_get_command_id(argv,&index,PRIME_NUMBER_SUB);
			// a look up table could be applied here since the functions are based on ids
			cli_blink(&command_id[1]);
			break;		
	
	case cmd_gpio : 
			command_id[1]=cli_get_command_id(argv,&index,PRIME_NUMBER_SUB);
 			argv+=index;
 			command_id[2] =(*argv)-'0';
 			if ((!command_id[2]) || (command_id[2]>20)) break;
 			cli_gpio_handle(&command_id[1]);
			break;
	
	case cmd_cip:  
			aes_encrypt_data( AES_SAMPLE_TEXT,
					 27, cripted_data);
			break;
	case cmd_dcip: 	aes_decrypt_data(cripted_data,27,uncripted_data);


			break;
	case cmd_help:
			
			CLI_OUT("\n____________command list____________\n\n ");
			CLI_OUT("turn\t\tgpio\t\tsend\t\tcip\n");
			break;
	case cmd_clear:
			
			for(index=0;index<28;index++)CLI_OUT("\n\n");
			break;
	case cmd_ble:   CLI_OUT("Sending to ble..'");
			#ifndef SLAVE_MODE
			ble_send ((uint8_t *)argv, strlen(argv));
			#endif
			
			break; 	
        case cmd_rgb:	command_id[0]=((*argv)-'0');
        		argv+=2;
			CLI_OUT("\nloading paramter\n");
      		        CLI_OUT("slave [%d] string [%s] len[%d] \n", command_id[0],argv, strlen(argv));
        		cafe_load_payload(command_id[0], argv, strlen(argv) );

        		break;

		case cmd_write_twi:
				err_code= nrf_drv_twi_tx(&m_twi_global_accelerometer, 0x1DU, (uint8_t*)&regR, sizeof(regR),true);
        			printf("[Error %x]\n",err_code);
        			break;
        case cmd_read_twi:
       				err_code= nrf_drv_twi_rx(&m_twi_global_accelerometer, 0x0D, (uint8_t*)&regR, sizeof(regR));
        			printf(" [Error %x ]\n",err_code);

        			break;
        case cmd_twi_read_saved_buffer: 
        			printf("twi [%x]", regR);

        			break;
        case cmd_twiapp:
         			accelerometer_load_addr(cli_ascii_streamhex_to_hex(argv, strlen(argv)-1));

        			accelerometer_read_reg();

        			break;
	case cmd_cmdtest:		printf("argv [%c] argv [%c] argv [%c]\n",argv [0],argv [1],argv [2]);
				break;
	case cmd_hex2dec:	printf("result [%x] , \n", cli_ascii_streamhex_to_hex(argv, strlen(argv)-1) );
				
				break;
	case cmd_accinit:	
				//accelerometer_on_start_configuration();
				break;
	default:	
				CLI_OUT("unknow command\n");
	break;


	}



	return 0;
}
void cli_parse_command(st_uart_string *uart_stream){
	
	if (!(uart_stream->pending_parse)) return;
	
	uart_stream->pending_parse=0;
	
	cli_parse(uart_stream->stream);

	memset(uart_stream->stream,0,sizeof(st_uart_string));
}

