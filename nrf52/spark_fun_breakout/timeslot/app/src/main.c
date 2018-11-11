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
#include "io_expander.h" 
#include "uart_app.h"
#include <stdio.h>

#define SOFT_DEVICE_ENABLED


extern uint16_t                          m_conn_handle;   /**< Handle of the current connection. */
//extern st_str  uart_tx;

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
	uint8_t temp_buffer[32+10];

	cafe_get_rx_payload(temp_buffer);
	
	SDBG("Received[");
	SDBG((char *)temp_buffer );
	SDBG("]\n");
}
st_uart_string uart_stream;

int main(void)
{
	uint32_t err_code;

	board_leds_init();
	nrf_gpio_pin_clear(LED_BLUE);
	uart_set_structe (&uart_stream);
	uart_init();
	SDBG("start!\n");
	timers_init();
	io_init_io_expander();
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
	for (;;){

		nrf_delay_ms(1000);
		nrf_gpio_pin_toggle(LED_RED);
		io_read_port();
		
		cli_parse_command(&uart_stream);
		
#ifdef TRANSCEIVER_MODE
		if ( packet_recieved ){
			packet_recieved= 0;
			print_received_data();
		}
#endif

	}
}


#ifdef OWN_ERROR_HANDLER
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	printf("Error at line %d ", line_num);
	while(1);
}
#endif

