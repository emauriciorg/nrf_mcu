/** 
******************************************************************************
* \file    main.c
* \brief    Entry point file.
******************************************************************************
*/
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_delay.h"


#include "timeslot.h"
#include "ws_uart.h"
#include "ws_ble.h"
#include "bbn_board.h"
#include "cafe.h"
#include "cli.h"
#include "ws_adc.h"
#include "ws_timer.h"
#include "accelerometer_i2c.h"
#include "app_twi.h"





void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name){
	app_error_handler(0xDEADBEEF, line_num, p_file_name); //0xDEADBEEF         /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
}

/*TO DO define a function to return the recieved packet*/


void print_recieved_radio_data(void)
{
	uint8_t len_t;
	char temp_buffer[32+10];

	len_t=cafe_get_rx_payload(temp_buffer);	
	WS_DBG("Received[ %s ][%d] /n",temp_buffer,len_t);
}

int main(void)
{	
	ws_timer_init();
	board_leds_init();
	ws_adc_setup();
	ws_uart_init ();
	
	ws_accelerometer_setup();
	//ws_timers_init();
	WS_ws_ble_init_modules();
	WS_TIMESLOT_INIT();
	WS_DBG("M.RIOS workshop start!\n");	

	for (;;)
	{	
		nrf_delay_ms(250);
		nrf_gpio_pin_toggle(LED_RED);
		cli_execute_debug_command();
		
#ifdef TRANSCEIVER_MODE
		if ( cafe_packet_recieved() ){
			print_recieved_radio_data();
		}
#endif
	}
}


/*
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	WS_DBG("Error at line %d ", line_num);
	while(1);
}
*/

