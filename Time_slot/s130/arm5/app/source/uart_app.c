

#include <string.h>

#include "nrf6310.h"/* depends on board, use mainly to address the macros for the pins */

#include "uart_app.h"
#include "micro_cli.h"
#include "app_uart.h"
#include "common_structs.h"

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
		app_uart_get(&local_uart_stream->stream[ (local_uart_stream->index)++ ]);
		uart_check_stream();
	break;

	case APP_UART_COMMUNICATION_ERROR:
		APP_ERROR_HANDLER(p_event->data.error_communication);
    	break;
	case APP_UART_FIFO_ERROR:
		APP_ERROR_HANDLER(p_event->data.error_code);
	break;

	default:
	    break;
    }
}

void uart_check_stream(void){

	char *pch= (char *)memchr(local_uart_stream->stream,'\n',strlen(local_uart_stream->stream));
	

	if(pch){
   	      
		local_uart_stream->pending_parse =true;
		local_uart_stream->index       =0; 	
	}else{
		local_uart_stream->pending_parse =false;
	}


	if( local_uart_stream->index > UART_STRING_MAX_SIZE)
		local_uart_stream->index       =0; 
	
}

void uart_init(void)
{
    uint32_t                     err_code;
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


void uart_msg_dbg(const char * message,uint32_t length){
	for (uint32_t i = 0; i < length; i++)
	{
		while(app_uart_put(message[i]) != NRF_SUCCESS);
	}while(app_uart_put('\n') != NRF_SUCCESS);
}

