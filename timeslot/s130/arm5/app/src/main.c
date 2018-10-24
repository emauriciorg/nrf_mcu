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
#include "ble_advertising.h"
#include "nrf_delay.h"


#include "../inc/timeslot.h"
#include "../inc/uart_app.h"
#include "../inc/ble_app.h"
#include "../inc/sys_event.h"
#include "../inc/bbn_board.h"
#include "../inc/cafe.h"
#include "../inc/cli.h"

#include "timer_app.h"

#define SOFT_DEVICE_ENABLED


extern uint16_t                          m_conn_handle;   /**< Handle of the current connection. */

void sys_evt_dispatch(uint32_t sys_evt)
{
	pstorage_sys_event_handler(sys_evt);
	ble_advertising_on_sys_evt(sys_evt);
	nrf_evt_signal_handler(sys_evt);
}


void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name){
	app_error_handler(0xDEADBEEF, line_num, p_file_name); //0xDEADBEEF         /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
}

static void timers_init(void){
	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
}




extern uint8_t packet_recieved;
unsigned char sample_text;

void print_received_data(void)
{
	uint8_t len_t;
	uint8_t temp_buffer[32+10];

	len_t=cafe_get_rx_payload(temp_buffer);
	
	uart_msg_dbg("Received[",9);
	uart_msg_dbg((char *)temp_buffer,len_t );
	uart_msg_dbg("]\n",2);
}

int main(void)
{
	uint32_t err_code;
	
	st_uart_string uart_stream;

	board_leds_init();
	
	uart_set_structe (&uart_stream);
	uart_init();
	uart_msg_dbg("Start!\n",strlen("Start!\n") );

	timers_init();

#ifdef SOFT_DEVICE_ENABLED
	ble_stack_init();

	device_manager_init(false);

	gap_params_init(); 
	services_init();
	advertising_init();
	

	conn_params_init();
	err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(err_code);

#endif

	timeslot_sd_init();

	for (;;)
	{	
				
	        cli_parse_command(&uart_stream);
		
#ifdef TRANSCEIVER_MODE
		if ( packet_recieved ){
			packet_recieved= 0;
			print_received_data();
		}
#endif

	}
}



void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	uart_msg_dbg("Error at line %d ", line_num);
	while(1);
}
