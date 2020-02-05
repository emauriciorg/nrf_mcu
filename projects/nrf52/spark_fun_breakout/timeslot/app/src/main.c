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
#include "ws_uart.h"
#include "ws_ble.h"
#include "sys_event.h"
#include "bbn_board.h"
#include "cafe.h"
#include "cli.h"
#include "io_expander.h" 
#include "ws_uart.h"
#include "ws_timer.h"
#include <stdio.h>

//#include "adxl345.h"

#define SOFT_DEVICE_ENABLED


unsigned char sample_text;
extern uint16_t                          m_conn_handle;   /**< Handle of the current connection. */
//extern st_str  uart_tx;
/*
void sys_evt_dispatch(uint32_t sys_evt)
{
	pstorage_sys_event_handler(sys_evt);
	ble_advertising_on_sys_evt(sys_evt);
	nrf_evt_signal_handler(sys_evt);
}
*/

void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name){
	app_error_handler(0xDEADBEEF, line_num, p_file_name); //0xDEADBEEF         /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
}

extern char i2c_state;


extern const nrf_drv_twi_t m_twi_global_accelerometer;
extern volatile bool m_xfer_done;


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
	//ws_adc_setup();
	ws_uart_init ();
	
	//ws_accelerometer_setup();
	//ws_timers_init();
	WS_BLE_INIT();
//	WS_TIMESLOT_INIT();
	WS_DBG("M.RIOS \n[BLE MASTER]]\nworkshop start!\n");	
//	APP_ERROR_CHECK(4);
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



#ifdef OWN_ERROR_HANDLER
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	printf("Error at line %d ", line_num);
	while(1);
}
#endif

