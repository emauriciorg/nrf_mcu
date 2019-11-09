/*
File	: main.c
Purpose : Generic application start

*/


/*----------------------------------standard headaers-------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
/*----------------------------------nRF headaers------------------------------*/
#include "app_uart.h"
#include "app_error.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

/*----------------------------------private headaers-------------------------*/
#include "ws_timer.h"
#include "ws_ble.h"
// #include "private_radio.h"
// #include "cli.h"
#include "ws_softdevice.h"

#define GATE_WAY_BOARD

//#define END_DEVICE_BOARD
#ifdef END_DEVICE_BOARD
#define TEST_POINT_1 7
#define TEST_POINT_2 8
#define TEST_POINT_3 9
#define TEST_POINT_4 10
#endif


#ifdef GATE_WAY_BOARD
#define TEST_POINT_1 12
#define TEST_POINT_2 13
#define TEST_POINT_3 14
#define TEST_POINT_4 15


#endif

/*---------------------------uart configuratio===-----------------------------*/

#define UART_STRING_MAX_SIZE  256
#define UART_RX_BUF_SIZE      256
#define UART_TX_BUF_SIZE      256

#define BBN_BOARD_PINS
#ifdef BBN_BOARD_PINS
	#define RX_PIN_NUMBER  16
	#define TX_PIN_NUMBER  15
	#define CTS_PIN_NUMBER 19
	#define RTS_PIN_NUMBER 5
#endif



char rx_buffer[5];
static void ws_uart_event_handle(app_uart_evt_t * p_event);


void ws_uart_event_handle(app_uart_evt_t * p_event){

	switch (p_event->evt_type){

	case APP_UART_DATA_READY:
		app_uart_get((uint8_t *)&rx_buffer[0]);

		//ws_uart_check_stream();
	break;

	case APP_UART_COMMUNICATION_ERROR:
		//APP_ERROR_HANDLER(p_event->data.error_communication);
	break;
	case APP_UART_FIFO_ERROR:
		//APP_ERROR_HANDLER(p_event->data.error_code);
	break;

	default:
	break;
	}
}
void uart_setup(void){

	uint32_t	 err_code;

	const app_uart_comm_params_t comm_params = {
		RX_PIN_NUMBER,
		TX_PIN_NUMBER,
		RTS_PIN_NUMBER,
		CTS_PIN_NUMBER,
		APP_UART_FLOW_CONTROL_DISABLED,
		false,
		UART_BAUDRATE_BAUDRATE_Baud115200
	};

	APP_UART_FIFO_INIT( &comm_params,
			UART_RX_BUF_SIZE,
			UART_TX_BUF_SIZE,
			ws_uart_event_handle,
			APP_IRQ_PRIORITY_LOW,
			err_code);
	APP_ERROR_CHECK(err_code);

}
/*------------------------------------------uart0 printf----------------------*/
/*
taken from
https://studio.segger.com/index.htm?https://studio.segger.com/libc_customizing_putchar.htm
*/
void uart0_printf(const char *fmt, ...)
{
	char buf[80]={0};
	char *p;
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	for (p = buf; *p; ++p)
		app_uart_put(*p);// null context
	va_end(ap);
}
/*------------------------------------------clock setup----------------------*/

void clock_setup(void){


	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

}


void main(void) {

#define BLE_ENABLE_IT
	NRF_POWER->DCDCEN = 0;

#ifndef BLE_ENABLE_IT
	clock_setup();
#endif

#ifdef BLE_ENABLE_IT

	ws_app_timer_init();
//        sd_ble_gap_tx_power_set(4);
	 sd_ble_gap_tx_power_set(4);
      
        ws_ble_stack_init();
	WS_BLE_INIT();
	 sd_ble_gap_tx_power_set(4);
      
#endif
        NRF_POWER->DCDCEN = 0;
	// uart_setup();
	nrf_gpio_cfg_output(TEST_POINT_1);
	nrf_gpio_cfg_output(TEST_POINT_2);
	nrf_gpio_cfg_output(TEST_POINT_3);
	nrf_gpio_cfg_output(TEST_POINT_4);


	
	//uart0_printf(" Program start!\r\n");

	while (1){

		nrf_delay_ms(100);

		nrf_gpio_pin_toggle(TEST_POINT_1);
		nrf_gpio_pin_toggle(TEST_POINT_2);
		nrf_gpio_pin_toggle(TEST_POINT_3);
		nrf_gpio_pin_toggle(TEST_POINT_4);
		//uart0_printf("looping \r\n");

	}
}

/*************************** End of file ****************************/
