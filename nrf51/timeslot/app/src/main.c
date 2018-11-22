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


#include "timeslot.h"
#include "uart_app.h"
#include "ble_app.h"
#include "sys_event.h"
#include "bbn_board.h"
#include "cafe.h"
#include "cli.h"

#include "timer_app.h"
#include "accelerometer_i2c.h"
#include "app_twi.h"


//extern uint16_t                          m_conn_handle;   /**< Handle of the current connection. */

void sys_evt_dispatch(uint32_t sys_evt)
{
	pstorage_sys_event_handler(sys_evt);
	ble_advertising_on_sys_evt(sys_evt);
	nrf_evt_signal_handler(sys_evt);
}


void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name){
	app_error_handler(0xDEADBEEF, line_num, p_file_name); //0xDEADBEEF         /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
}


extern uint8_t packet_recieved;


void print_recieved_radio_data(void)
{
	uint8_t len_t;
	uint8_t temp_buffer[32+10];

	len_t=cafe_get_rx_payload(temp_buffer);
	
	SDBG("Received[");
	SDBG((char *)temp_buffer,len_t );
	SDBG("]\n");
}



int main(void)
{	
	st_uart_string uart_stream;
	timer_app_init();
	board_leds_init();
	uart_init (&uart_stream);
	SDBG("Start!\n");
	
	accelerometer_setup();
	//timers_init();

	BLE_INIT_INI_MODULES();

	adc_app_init();

	TIMESLOT_INIT();
	
	APP_ERROR_CHECK(4);
	for (;;)
	{	
		

		nrf_delay_ms(250);
		nrf_gpio_pin_toggle(LED_RED);
		cli_parse_command(&uart_stream);
		
#ifdef TRANSCEIVER_MODE
		if ( packet_recieved ){
			packet_recieved= 0;
			print_recieved_radio_data();
		}
#endif

	}
}


/*
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	SDBG("Error at line %d ", line_num);
	while(1);
}
*/

