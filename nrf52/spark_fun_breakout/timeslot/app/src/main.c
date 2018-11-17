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
#include "timer_app.h"
#include <stdio.h>

#include "adxl345.h"

#define SOFT_DEVICE_ENABLED

extern uint8_t packet_recieved;
unsigned char sample_text;
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

void print_recieved_radio_data(void)
{
	uint8_t temp_buffer[32+10];
	cafe_get_rx_payload(temp_buffer);
	SDBG("Received[ %s]\n",(char *)temp_buffer );
}
extern char i2c_state;


extern const nrf_drv_twi_t m_twi_global_accelerometer;
extern volatile bool m_xfer_done;

int main(void)
{

	st_uart_string uart_stream;
	timer_app_init();
	board_leds_init();
	nrf_gpio_pin_clear(LED_BLUE);
	uart_init (&uart_stream);
	SDBG("start!\n");
	adxl345_init();

#ifdef SOFT_DEVICE_ENABLED
	ble_init_modules();
#endif

    uint8_t reg = 0;
    ret_code_t err_code;

	//timeslot_sd_init();
	for (;;){

		nrf_delay_ms(500);
		nrf_gpio_pin_toggle(LED_RED);
	
//		SDBG("loop\n");
		cli_parse_command(&uart_stream);
		adxl345_read();
 		do{
	            __WFE();
       		 }while(m_xfer_done == false);
     	 	err_code= nrf_drv_twi_tx(&m_twi_global_accelerometer, ADXL_ADDRESS, &reg, sizeof(reg),true);  

       	 	APP_ERROR_CHECK(err_code);
    	    	m_xfer_done = false;
    
		
#ifdef TRANSCEIVER_MODE
		if ( packet_recieved ){
			packet_recieved= 0;
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

