

#include "uart_app.h"

#include "nrf6310.h"/* depends on board, use mainly to address the macros for the pins */
#include <string.h>
/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to 
 *          a string. The string will be be sent over BLE when the last character received was a 
 *          'new line' i.e '\n' (hex 0x0D) or if the string has reached a length of 
 *          @ref NUS_MAX_DATA_LENGTH.
 */
/**@snippet [Handling the data received over UART] */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

void uart_event_handle(app_uart_evt_t * p_event)
{
#define BLE_NUS_MAX_DATA_LEN 20
	static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;

    switch (p_event->evt_type)
    {
	case APP_UART_DATA_READY:
	 //   UNUSED_VARIABLE(app_uart_get(&data_array[index]));
	    index++;

	    if ((data_array[index - 1] == '\n') || (index >= (BLE_NUS_MAX_DATA_LEN)))
	    {
	//	err_code = ble_nus_string_send(&m_nus, data_array, index);
		index=0;
		//printf("%s\n",data_array);
		memset(data_array,0,sizeof(data_array));
		if (err_code != NRF_ERROR_INVALID_STATE)
		{
		 //   APP_ERROR_CHECK(err_code);
		}
		
		index = 0;
	    }
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

/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
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
	UART_BAUDRATE_BAUDRATE_Baud9600
    };

    APP_UART_FIFO_INIT( &comm_params,
		       UART_RX_BUF_SIZE,
		       UART_TX_BUF_SIZE,
		       uart_event_handle,
		       APP_IRQ_PRIORITY_LOW,
		       err_code);
    APP_ERROR_CHECK(err_code);
}


void msg_dbg(const char * message,uint32_t length){


	for (uint32_t i = 0; i < length; i++)
	{
		while(app_uart_put(message[i]) != NRF_SUCCESS);
	}while(app_uart_put('\n') != NRF_SUCCESS);
}