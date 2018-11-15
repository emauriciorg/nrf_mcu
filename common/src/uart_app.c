

#include <string.h>

//#include "nrf6310.h"/* depends on board, use mainly to address the macros for the pins */
#include "nrf.h"
#include "../inc/uart_app.h"
#include "../inc/cli.h"
#include "app_uart.h"
#include "../inc/common_structs.h"
#include "app_error.h"
extern void uart_event_handle(app_uart_evt_t * p_event);


#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

st_uart_string *local_uart_stream;

void uart_set_structe(st_uart_string *external_uart_stream){
	local_uart_stream= external_uart_stream;

}

void uart_event_handle(app_uart_evt_t * p_event)
{
	
	switch (p_event->evt_type){
	
	case APP_UART_DATA_READY:
		app_uart_get((uint8_t *)&local_uart_stream->stream[ (local_uart_stream->index)++ ]);
		uart_check_stream();
	
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

void uart_check_stream(void){

	char len=strlen(local_uart_stream->stream);
	char *pch= (char *)memchr(local_uart_stream->stream,'\n',len);
	

	if (pch){
   	     	 local_uart_stream->stream[len-1]=0;
		local_uart_stream->pending_parse =true;
		local_uart_stream->index       =0; 	
	}else{
		local_uart_stream->pending_parse =false;
	}


	if ( local_uart_stream->index > UART_STRING_MAX_SIZE)
		local_uart_stream->index       =0; 
	
}

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
void uart_init(st_uart_string *external_uart_stream)
{
    uint32_t                     err_code;

    uart_set_structe (external_uart_stream);

    const app_uart_comm_params_t comm_params =
    {
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
		       uart_event_handle,
		       APP_IRQ_PRIORITY_LOW,
		       err_code);
    APP_ERROR_CHECK(err_code);
}
