/** 
******************************************************************************
* \file   main.c
* \brief  timeslot workshop
* \author mauricio.rios@titoma.com
******************************************************************************
*/
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_delay.h"


#include "ws_timeslot.h"
#include "ws_uart.h"
#include "ws_ble.h"
#include "bbn_board.h"
#include "cafe.h"
#include "cli.h"
#include "ws_adc.h"
#include "ws_timer.h"
#include "accelerometer_i2c.h"
#include "app_twi.h"
#include "ws_softdevice.h"



/**< 0xDEADBEEF Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name){
	app_error_handler(0xDEADBEEF, line_num, p_file_name); 
}

/*TO DO define a function to return the recieved packet*/
void print_recieved_radio_data(void){
	
	uint8_t len_t;
	char temp_buffer[32+10];
	len_t = cafe_get_rx_payload(temp_buffer);	
	WS_DBG("Received[%s][%d] /n",temp_buffer,len_t);
}

int main(void){	
	
	ws_app_timer_init();
	ws_leds_init();
	ws_uart_init ();
	//ws_adc_setup();
	//ws_accelerometer_setup();

	ws_ble_stack_init();

	WS_BLE_INIT();
	WS_TIMESLOT_INIT();
	WS_DBG("M.RIOS\n[BLE MASTER]\nWorkshop start!\n");	

	while (true){	
		cli_execute_debug_command();
		nrf_delay_ms(250);
		nrf_gpio_pin_toggle(LED_RED);	
#ifdef TRANSCEIVER_MODE
		if ( cafe_packet_recieved() ){
			print_recieved_radio_data();
		}
#endif
	}
}



