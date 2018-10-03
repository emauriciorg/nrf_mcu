
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble_advertising.h"

#include "nrf_delay.h"


#include "../inc/timeslot.h"
#include "uart_app.h"
#include "../inc/ble_app.h"
#include "../inc/sys_event.h"
#include "bbn_board.h"
#include "cafe.h"
#include "cli.h"

#define DEAD_BEEF                        0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define SOFT_DEVICE_ENABLED


extern uint8_t radio_sent;
extern uint16_t                          m_conn_handle;   /**< Handle of the current connection. */

void sys_evt_dispatch(uint32_t sys_evt)
{
	pstorage_sys_event_handler(sys_evt);
	
	ble_advertising_on_sys_evt(sys_evt);
	nrf_evt_signal_handler(sys_evt);
}

void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name){
	app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void timers_init(void){
	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
}




extern uint8_t led_state;



void print_received_data(void)
{
	uint8_t len_t;
	uint8_t temp_buffer[32+10];
	uint8_t temp_buffer2[32+30];
	get_rx_payload(temp_buffer);
	len_t=sprintf((char *)temp_buffer2,"Received [%s]\n",temp_buffer );
	
	uart_msg_dbg((char *)temp_buffer2,len_t );
	
}

int main(void)
{
	uint32_t err_code;
	bool erase_bonds;
	st_uart_string uart_stream;

  	nrf_gpio_range_cfg_output(8, 10);	
	nrf_gpio_pin_set(LED_BLUE);
	nrf_gpio_pin_set(LED_GREEN);
	
	uart_set_structe (&uart_stream);
	uart_init();
	uart_msg_dbg("Start!\n",strlen("Start!\n") );

	
	timers_init();
#ifdef SOFT_DEVICE_ENABLED
	ble_stack_init();

	device_manager_init(erase_bonds);

 	radio_sent=1;//flag to enable TS messages


	gap_params_init(); //gap connection paramaters (timeouts)
	services_init();
	advertising_init();
	

	
	
	conn_params_init();
#ifdef TS_ENABLED
	timeslot_sd_init();
#endif		
	err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(err_code);

#endif
	
	for (;;)
	{	
				
	        cli_parse_command(&uart_stream);
		
		if( led_state){
			led_state= 0;
			
			print_received_data();
		}else{
		//	nrf_gpio_pin_toggle(LED_RED);
		}

	}
}



void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	uart_msg_dbg("Error code is", 10);
	while(1);
}
