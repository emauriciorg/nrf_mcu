/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"

#include "nrf.h"
#include "app_error.h"


#include "ble_advertising.h"
#include "device_manager.h"
//#include "boards.h"



//#include "app_trace.h"

#include "app_timer.h"


//#include "bsp.h"
//#include "bsp_btn_ble.h"

#include "timeslot.h"
//#include "uart_app.h"
#include "nrf_delay.h"
#include "simple_uart.h"

#include "bleAPP.h"
#include "sys_event.h"


#include "bbn_board.h"
#define APP_TIMER_PRESCALER              0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          4                                          /**< Size of timer operation queues. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                          /**< Number of attempts before giving up the connection parameter negotiation. */


#define DEAD_BEEF                        0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */



void sys_evt_dispatch(uint32_t sys_evt)
{

	pstorage_sys_event_handler(sys_evt);
	ble_advertising_on_sys_evt(sys_evt);
	nrf_evt_signal_handler(sys_evt);
}

void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
	app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void timers_init(void)
{

    // Initialize timer module.
	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

}


 
static void conn_params_error_handler(uint32_t nrf_error)
{
	APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
	uint32_t               err_code;
	ble_conn_params_init_t cp_init;

	memset(&cp_init, 0, sizeof(cp_init));

	cp_init.p_conn_params                  = NULL;
	cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
	cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
	cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
	cp_init.disconnect_on_fail             = false;
	cp_init.evt_handler                    = on_conn_params_evt;
	cp_init.error_handler                  = conn_params_error_handler;

	err_code = ble_conn_params_init(&cp_init);
	APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting timers.
*/
static void application_timers_start(void)
{
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{

    // Prepare wakeup buttons.
	 uint32_t err_code;
//	= bsp_btn_ble_sleep_mode_prepare();
	// APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
	err_code = sd_power_system_off();
	APP_ERROR_CHECK(err_code);
}





extern uint16_t                          m_conn_handle;   /**< Handle of the current connection. */

typedef enum
{
    BSP_EVENT_NOTHING = 0,                  /**< Assign this event to an action to prevent the action from generating an event (disable the action). */
    BSP_EVENT_DEFAULT,                      /**< Assign this event to an action to assign the default event to the action. */
    BSP_EVENT_CLEAR_BONDING_DATA,           /**< Persistent bonding data should be erased. */
    BSP_EVENT_CLEAR_ALERT,                  /**< An alert should be cleared. */
    BSP_EVENT_DISCONNECT,                   /**< A link should be disconnected. */
    BSP_EVENT_ADVERTISING_START,            /**< The device should start advertising. */
    BSP_EVENT_ADVERTISING_STOP,             /**< The device should stop advertising. */
    BSP_EVENT_WHITELIST_OFF,                /**< The device should remove its advertising whitelist. */
    BSP_EVENT_BOND,                         /**< The device should bond to the currently connected peer. */
    BSP_EVENT_RESET,                        /**< The device should reset. */
    BSP_EVENT_SLEEP,                        /**< The device should enter sleep mode. */
    BSP_EVENT_WAKEUP,                       /**< The device should wake up from sleep mode. */
    BSP_EVENT_DFU,                          /**< The device should enter DFU mode. */
    BSP_EVENT_KEY_0,                        /**< Default event of the push action of BSP_BUTTON_0 (only if this button is present). */
    BSP_EVENT_KEY_1,                        /**< Default event of the push action of BSP_BUTTON_1 (only if this button is present). */
    BSP_EVENT_KEY_2,                        /**< Default event of the push action of BSP_BUTTON_2 (only if this button is present). */
    BSP_EVENT_KEY_3,                        /**< Default event of the push action of BSP_BUTTON_3 (only if this button is present). */
    BSP_EVENT_KEY_4,                        /**< Default event of the push action of BSP_BUTTON_4 (only if this button is present). */
    BSP_EVENT_KEY_5,                        /**< Default event of the push action of BSP_BUTTON_5 (only if this button is present). */
    BSP_EVENT_KEY_6,                        /**< Default event of the push action of BSP_BUTTON_6 (only if this button is present). */
    BSP_EVENT_KEY_7,                        /**< Default event of the push action of BSP_BUTTON_7 (only if this button is present). */
    BSP_EVENT_KEY_LAST = BSP_EVENT_KEY_7,
} bsp_event_t;



/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
	uint32_t err_code;
	switch (event){
		
	case BSP_EVENT_SLEEP:
					sleep_mode_enter();
	break;
	case BSP_EVENT_DISCONNECT:
					err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
					if (err_code != NRF_ERROR_INVALID_STATE){
						APP_ERROR_CHECK(err_code);
					}
	break;

	case BSP_EVENT_WHITELIST_OFF:
					err_code = ble_advertising_restart_without_whitelist();
					if (err_code != NRF_ERROR_INVALID_STATE){
							APP_ERROR_CHECK(err_code);
					}
	break;

	default:
	break;
	}
}







/*static void power_manage(void)
{
	uint32_t err_code = sd_app_evt_wait();
	APP_ERROR_CHECK(err_code);
}*/


/**@brief Function for application main entry.
 */
int main(void)
{
	uint32_t err_code;
	bool erase_bonds;

   	//Enter main loop.
  	nrf_gpio_range_cfg_output(8, 10);
	nrf_gpio_pin_clear(LED_RED);
	nrf_gpio_pin_set(LED_BLUE);
	nrf_gpio_pin_set(LED_GREEN);

    	//Initialize.
	timers_init();


	simple_uart_config(  RTS_PIN,
                         RX_PIN, 
                         CTS_PIN,
                         TX_PIN, 
                        false);

	 simple_uart_putstring("nrf init\n");

	
	ble_stack_init();
	
	device_manager_init(erase_bonds);
	
	
	gap_params_init();
	advertising_init();
	services_init();
	conn_params_init();
	
	timeslot_sd_init();
	
    	//Start execution.
	application_timers_start();

	err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(err_code);
	
	
	for (;;)
	{		
		nrf_delay_ms(1000);
		//nrf_gpio_pin_toggle(LED_GREEN); //Toggle LED4
	}
}



void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	
	printf("Error code is %x \n", error_code );
	while(1);
}
