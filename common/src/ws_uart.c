

#include <string.h>

#include "nrf.h"
#include "ws_uart.h"
#include "cli.h"
#include "app_uart.h"
#include "common_structs.h"
#include "app_error.h"

#define UART_STRING_MAX_SIZE  256
#define UART_RX_BUF_SIZE      256
#define UART_TX_BUF_SIZE      256

typedef struct {
	char *ptr_stream;
	char  input_stream[UART_STRING_MAX_SIZE];
	//char  output_stream[UART_STRING_MAX_SIZE+32];
	char  out_len;
	char  pending_parse;
	char  tail;
}uart_driver_t; 


#ifdef DEVKIT_BOARD
	#define RX_PIN_NUMBER  8
	#define TX_PIN_NUMBER  6
	#define CTS_PIN_NUMBER 7
	#define RTS_PIN_NUMBER 5
#endif

#ifdef BBN_BOARD_PINS
	#define RX_PIN_NUMBER  16
	#define TX_PIN_NUMBER  15
	#define CTS_PIN_NUMBER 19
	#define RTS_PIN_NUMBER 5
#endif

#ifdef SPARKFUN_BOARD
	#define RX_PIN_NUMBER  26
	#define TX_PIN_NUMBER  27
	#define CTS_PIN_NUMBER 7
	#define RTS_PIN_NUMBER 5
#endif


static void ws_uart_event_handle(app_uart_evt_t * p_event);
static uart_driver_t local_uart;


char ws_uart_pending_debug_packet(void){

	if(!local_uart.pending_parse){ 	 
		return false;
	}
	local_uart.pending_parse = false;
	return true;	
}

char   *ws_uart_get_debug_packet(void){

	return 	local_uart.input_stream;
}

void ws_uart_flush_debug_packet(void){

	memset(&local_uart.input_stream[0],0,sizeof(uart_driver_t));
}

void ws_uart_check_stream(void){

	char len = strlen(local_uart.input_stream);
	char *pch = (char *)memchr(local_uart.input_stream,'\n',len);

	if (pch){
		local_uart.input_stream[len-1] = 0;
		local_uart.pending_parse = true;
		local_uart.tail         = 0; 	
	}else{
		local_uart.pending_parse = false;
	}

	if ( local_uart.tail > UART_STRING_MAX_SIZE)
		local_uart.tail       =0; 
}


void ws_uart_init(void){

	uint32_t                     err_code;

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


void ws_uart_event_handle(app_uart_evt_t * p_event){

	switch (p_event->evt_type){
	
	case APP_UART_DATA_READY:
		app_uart_get((uint8_t *)&local_uart.input_stream[ local_uart.tail ]);
		local_uart.tail++;
		ws_uart_check_stream();	
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
