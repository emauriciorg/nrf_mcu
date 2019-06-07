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
#include "private_radio.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "bbn_board.h"

#include "cli.h"

#include "fsm_slave.h"
#include "ws_uart.h"
#include "ws_aes.h"
#include "ws_timer.h"

static uint8_t  uncripted_data[40];



void print_received_data(void)
{
	char encripted_message[RADIO_PACKET_LEN+5];
	memset(uncripted_data, 0, sizeof(uncripted_data));
	radioget_rx_payload(encripted_message);

#ifndef NO_CYPHER

#ifdef SHOW_AES_PACKET
	WS_DBG("[");
	for (int i = 0; i < 32; i++){
 		WS_DBG(" %d",encripted_message[i]); 
	}
	WS_DBG("]\n");
#endif
	
	aes_decrypt_data( (uint8_t *)&encripted_message[2],
					encripted_message[2],
					uncripted_data);

	WS_DBG("[%s]\n",(&uncripted_data[3]));
	/*simple parser to reflect packet reception on the board*/
	if (!memcmp(&uncripted_data[3],"REDOFF",6)) nrf_gpio_pin_set(LED_RED);
	if (!memcmp(&uncripted_data[3],"REDOON",6)) nrf_gpio_pin_clear(LED_RED);
	if (!memcmp(&uncripted_data[3],"BLUEOF",6)) nrf_gpio_pin_set(LED_BLUE);
	if (!memcmp(&uncripted_data[3],"BLUEON",6)) nrf_gpio_pin_clear(LED_BLUE);
	if (!memcmp(&uncripted_data[3],"BLUEON",6)) nrf_gpio_pin_clear(LED_BLUE);
	
	if (!memcmp(&uncripted_data[3],"TX",2)){
		WS_DBG("[Trasmitter mode set]\n");
		radio_update_mode(RADIO_TRANSMITTER_MODE);
	}
	if (!memcmp(&uncripted_data[3],"RX",2)){
		WS_DBG("[reciever mode set]\n");
		radio_update_mode(RADIO_RECEIVER_MODE);
	}
#endif	
}

int main(void)
{	
	ws_clock_setup();
	ws_uart_init();
	WS_DBG("M.RIOS \n[BLE SLAVE]\nWorkshop start!\n");	

	ws_leds_init();
	radio_start();

	
	while (true){

		cli_execute_debug_command();
		radio_print_current_state();

		if ( radio_rx_packet_available()){		
			print_received_data();
		}
	}
}
