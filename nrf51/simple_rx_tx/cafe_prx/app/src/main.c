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
#include <string.h>
#include <stdio.h>

#include "nrf.h"
#include "cafe.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "bbn_board.h"

#include "cli.h"

#include "fsm_slave.h"
#include "ws_uart.h"
#include "ws_aes.h"
#include "ws_timer.h"

extern uint8_t packet_recieved;
static uint8_t  uncripted_data[40];



#define SHOW_AES_PACKET	
void print_received_data(void){

	char encripted_message[CAFE_CORE_MAX_PAYLOAD_LENGTH+5];
	memset(uncripted_data, 0, sizeof(uncripted_data));

	cafe_get_rx_payload(encripted_message);

	WS_DBG("Parameters[rssi -%d\n ",cafe_get_rssi());
	WS_DBG("[Timer %d]\n",ws_get_timer1_ticks());	

#ifdef SHOW_AES_PACKET
	WS_DBG("[");
	for (int i=0;i <32; i++){
 		WS_DBG(" %d",encripted_message[i]); 
	}
	WS_DBG("]\n");
#endif

	aes_decrypt_data( (uint8_t *)&encripted_message[2],encripted_message[2],uncripted_data);
	WS_DBG("[%s]\n",(&uncripted_data[3]));
	/*simple parser to reflect packet reception on the board*/
	if (!memcmp(&uncripted_data[3],"REDOFF",strlen("REDOFF")))nrf_gpio_pin_set(LED_RED);
	if (!memcmp(&uncripted_data[3],"REDOON",strlen("REDOON")))nrf_gpio_pin_clear(LED_RED);
	if (!memcmp(&uncripted_data[3],"BLUEOFF",strlen("BLUEOFF")))nrf_gpio_pin_set(LED_BLUE);
	if (!memcmp(&uncripted_data[3],"BLUEON",strlen("BLUEON")))nrf_gpio_pin_clear(LED_BLUE);
}

int main(void)
{	
	ws_clock_setup();
	ws_uart_init();
	ws_leds_init();

	//ws_timer1_setup();

	cafe_start_radio();

	WS_DBG("M.RIOS \n[BLE SLAVE]\nWorkshop start!\n");	
	while (true){
		cli_execute_debug_command();
		if (!packet_recieved){
			//if (single_notification &timeout){
			//printf("Connection loss\n");  
			//nrf_gpio_pin_set(LED_GREEN);
			//}
		}
		if ( packet_recieved){
			packet_recieved=0;
			print_received_data();
			continue;		
		}
	}
}
