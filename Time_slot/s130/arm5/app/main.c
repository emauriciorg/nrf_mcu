
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble_advertising.h"
#include "device_manager.h"
#include "app_timer.h"
#include "timeslot.h"
#include "uart_app.h"
#include "nrf_delay.h"
#include "bleAPP.h"
#include "sys_event.h"
#include "bbn_board.h"


#define APP_TIMER_PRESCALER              0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          4                                          /**< Size of timer operation queues. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                          /**< Number of attempts before giving up the connection parameter negotiation. */


#define SEC_PARAM_BOND                   1                                          /**< Perform bonding. */
#define SEC_PARAM_MITM                   0                                          /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES        BLE_GAP_IO_CAPS_NONE                       /**< No I/O capabilities. */
#define SEC_PARAM_OOB                    0                                          /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE           7                                          /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE           16                                         /**< Maximum encryption key size. */

#define DEAD_BEEF                        0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */


static dm_application_instance_t        m_app_handle;                               /**< Application identifier allocated by device manager */

extern uint8_t counter_sec;
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


 
static void conn_params_error_handler(uint32_t nrf_error){
	APP_ERROR_HANDLER(nrf_error);
}

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




static uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
	dm_event_t const  * p_event,
	ret_code_t        event_result)
{
	APP_ERROR_CHECK(event_result);

#ifdef BLE_DFU_APP_SUPPORT
	if (p_event->event_id == DM_EVT_LINK_SECURED){
		app_context_load(p_handle);
	}
#endif // BLE_DFU_APP_SUPPORT

	return NRF_SUCCESS;
}


static void device_manager_init(bool erase_bonds)
{
	uint32_t               err_code;

	dm_init_param_t        init_param = {.clear_persistent_data = erase_bonds};
	dm_application_param_t register_param;

	err_code = pstorage_init();
	APP_ERROR_CHECK(err_code);

	err_code = dm_init(&init_param);
	APP_ERROR_CHECK(err_code);

	memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

	register_param.sec_param.bond         = SEC_PARAM_BOND;
	register_param.sec_param.mitm         = SEC_PARAM_MITM;
	register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
	register_param.sec_param.oob          = SEC_PARAM_OOB;
	register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
	register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
	register_param.evt_handler            = device_manager_evt_handler;
	register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

	err_code = dm_register(&m_app_handle, &register_param);
	APP_ERROR_CHECK(err_code);
}



int main(void)
{
	uint32_t err_code;
	bool erase_bonds;

  	nrf_gpio_range_cfg_output(8, 10);	
	nrf_gpio_pin_set(LED_BLUE);
	nrf_gpio_pin_set(LED_GREEN);
	
	timers_init();
	uart_init();

	msg_dbg("Start!\n",strlen("Start!\n") );
	ble_stack_init();
	device_manager_init(erase_bonds);

 	radio_sent=1;//flag to enable TS messages


	gap_params_init();
	advertising_init();
	services_init();
	conn_params_init();
	timeslot_sd_init();
		
	err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(err_code);
	
	
	uint8_t buff_counter[70];
	uint8_t len_t, main_tick=0;

	for (;;)
	{	
		nrf_delay_ms(1000);
		len_t=sprintf((char *)buff_counter,"Count :[%d],[%d]\n", counter_sec, main_tick++);
		msg_dbg((char *)buff_counter,len_t );
		
	}
}



void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	msg_dbg("Error code is", 10);
	while(1);
}
