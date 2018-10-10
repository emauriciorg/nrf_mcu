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

#include "RX_CAFE.h"

#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "bbn_board.h"
#include "simple_uart.h"
#include <string.h>
#include <stdio.h>
#include "aes_app.h"


//static uesb_payload_t rx_payload;

extern uint8_t led_state;
static uint8_t  uncripted_data[40];

void print_received_data(void)
{
	uint8_t temp_buffer[CAFE_CORE_MAX_PAYLOAD_LENGTH+5];
	get_rx_payload(temp_buffer);
	
	
#ifdef SHOW_AES_PACKET
	uint8_t temp_buffer2[CAFE_CORE_MAX_PAYLOAD_LENGTH+20];
	
	simple_uart_put('[');

	for (int i=0;i <32; i++){
		sprintf((char *)temp_buffer2,"%d ",temp_buffer[i] );
 	   //simple_uart_put(temp_buffer[i]);
	   simple_uart_putstring(temp_buffer2); 
			memset(temp_buffer2,0,sizeof(CAFE_CORE_MAX_PAYLOAD_LENGTH+20));
	}

	simple_uart_put(']');
#endif

	memset(uncripted_data,0,sizeof(uncripted_data));
	aes_decrypt_data( &temp_buffer[2],temp_buffer[2],uncripted_data);
	
	simple_uart_put('[');
	simple_uart_putstring(&uncripted_data[3]);
	//simple_uart_putstring( temp_buffer2); 
	simple_uart_putstring("]\n");
}

int main(void)
{

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	nrf_gpio_range_cfg_output(8, 31);

	cafe_setup_rx();
	simple_uart_config(  RTS_PIN,
		RX_PIN, 
		CTS_PIN,
		TX_PIN, 
		false);
	simple_uart_putstring("nrf init\n");
	nrf_gpio_pin_set(LED_BLUE);

	uint8_t rssi_buffer[40];
	extern  cafe_payload_t  rx_payload;
	memset(rssi_buffer,0,sizeof(rssi_buffer));
	

	uint8_t connection_status=0;
	extern uint8_t recieved_counter;	
	while (true)
	{   

		if( led_state){

	
			connection_status=1;
		
			sprintf((char *)rssi_buffer,"Parameters[rssi -%d] [ state: %d]\n",get_rssi(), NRF_RADIO->STATE);
			simple_uart_putstring(rssi_buffer);;
			print_received_data();
			led_state=0;
	
			nrf_gpio_pin_toggle(LED_GREEN);
			nrf_gpio_pin_set(LED_RED);
			
		}else{
			nrf_gpio_pin_set(LED_GREEN);
			if(connection_status){
				connection_status=0;
				simple_uart_putstring("Connection loss\n");  
			}
			
		}
		nrf_delay_ms(500);

	}
}
