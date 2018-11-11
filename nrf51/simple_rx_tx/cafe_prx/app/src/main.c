/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
//#include "RX_CAFE.h"
#include "cafe.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "bbn_board.h"
#include "simple_uart.h"
#include <string.h>
#include <stdio.h>
#include "aes_app.h"
#include "timer_app.h"
#include "cli.h"

#include "fsm_slave.h"
#include "uart_app.h"
extern uint8_t packet_recieved;
static uint8_t  uncripted_data[40];


void print_received_data(void)
{
	uint8_t temp_buffer[CAFE_CORE_MAX_PAYLOAD_LENGTH+5];
	cafe_get_rx_payload(temp_buffer);
	
	
#ifdef SHOW_AES_PACKET
	uint8_t temp_buffer2[CAFE_CORE_MAX_PAYLOAD_LENGTH+20];
	
	printf("[");

	for (int i=0;i <32; i++){
		sprintf((char *)temp_buffer2,"%d ",temp_buffer[i] );
 		printf("%s",temp_buffer2); 
		memset(temp_buffer2,0,sizeof(CAFE_CORE_MAX_PAYLOAD_LENGTH+20));
	}

	printf("]\n");
#endif

	memset(uncripted_data,0,sizeof(uncripted_data));
	aes_decrypt_data( &temp_buffer[2],temp_buffer[2],uncripted_data);
	
	printf("[%s]\n",(&uncripted_data[3]));

	if (!memcmp(&uncripted_data[3],"REDOFF",strlen("REDOFF")))nrf_gpio_pin_set(LED_RED);
	if (!memcmp(&uncripted_data[3],"REDOON",strlen("REDOON")))nrf_gpio_pin_clear(LED_RED);
	if (!memcmp(&uncripted_data[3],"BLUEOFF",strlen("BLUEOFF")))nrf_gpio_pin_set(LED_BLUE);
	if (!memcmp(&uncripted_data[3],"BLUEON",strlen("BLUEON")))nrf_gpio_pin_clear(LED_BLUE);

}



int main(void)
{
	static uint16_t external_counter; 

	extern uint8_t recieved_counter;
	

	st_uart_string uart_stream;
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	nrf_gpio_range_cfg_output(8, 31);


	cafe_self_configuration(I_AM_RECIEVER);

	uart_set_structe (&uart_stream);
	uart_init();
	SDBG("Start!\n" );

	//timer1_app_setup(&external_counter);
	
	nrf_gpio_pin_set(LED_BLUE);
	nrf_gpio_pin_set(LED_RED);

	nrf_gpio_pin_set(LED_GREEN);
	
	uint8_t rssi_buffer[40];
	extern  cafe_payload_t  rx_payload;
	memset(rssi_buffer,0,sizeof(rssi_buffer));
	
	nrf_gpio_pin_clear(LED_RED);
	external_counter=1000;
	while (true)
	{   


		cli_parse_command(&uart_stream);
		

		if (!packet_recieved){
			//if (single_notification &timeout){
				//printf("Connection loss\n");  
				nrf_gpio_pin_set(LED_GREEN);
			//}
		}

		if ( packet_recieved){
	
			sprintf((char *)rssi_buffer,"Parameters[rssi -%d] [ state: %d], [timer %d]\n",cafe_get_rssi(), NRF_RADIO->STATE, external_counter);
			printf("%s", rssi_buffer);
			print_received_data();
			packet_recieved=0;
			//trigger_timeout
			continue;	
			
		}
		
		
	
	}
}
