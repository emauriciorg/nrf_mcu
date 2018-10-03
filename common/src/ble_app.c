#include "../inc/ble_app.h"

#include "../inc/sys_event.h"

#include "ble_conn_params.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble.h"

#include "device_manager.h"

#include "nrf_gpio.h"
#include "../inc/bbn_board.h"
#include <stdio.h>

#include "custom_ble_services.h"


#define IS_SRVC_CHANGED_CHARACT_PRESENT  1                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/
#define DEVICE_NAME                      "BLE_TS_CS4"                               /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME                "NordicSemiconductor"                      /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                 300                                        /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS       180                                        /**< The advertising timeout in units of seconds. */


#define MIN_CONN_INTERVAL                MSEC_TO_UNITS(100, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL                MSEC_TO_UNITS(200, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                    0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                 MSEC_TO_UNITS(4000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */
 uint16_t                          m_conn_handle = BLE_CONN_HANDLE_INVALID;   /**< Handle of the current connection. */

static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}}; /**< Universally unique service identifiers. */
//static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE, BLE_UUID_NUS_SERVICE}}; /**< Universally unique service identifiers. */


ble_cs_t m_custom_service;

uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
	dm_event_t const  * p_event,
	ret_code_t        event_result);

 void ble_stack_init(void)
 {
	uint32_t err_code;

	SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

#if defined(S110) || defined(S130) || defined(S132)
    // Enable BLE stack.
	ble_enable_params_t ble_enable_params;
	memset(&ble_enable_params, 0, sizeof(ble_enable_params));

#if (defined(S130) || defined(S132))
	ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
	
	ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
	err_code = sd_ble_enable(&ble_enable_params);	
	APP_ERROR_CHECK(err_code);	
#endif

    // Register with the SoftDevice handler module for BLE events.
	err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
	APP_ERROR_CHECK(err_code);
	
    // Register with the SoftDevice handler module for BLE events.
	err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
	APP_ERROR_CHECK(err_code);
}	

 void ble_evt_dispatch( ble_evt_t * p_ble_evt)
{
	dm_ble_evt_handler(p_ble_evt);
	ble_conn_params_on_ble_evt(p_ble_evt);

	ble_cs_on_ble_evt(&m_custom_service, p_ble_evt);

	on_ble_evt(p_ble_evt);
	ble_advertising_on_ble_evt(p_ble_evt);

}



/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
 void on_ble_evt(ble_evt_t * p_ble_evt)
{

	switch (p_ble_evt->header.evt_id)
	{
		case BLE_GAP_EVT_CONNECTED:
		nrf_gpio_pin_set(LED_RED);
		nrf_gpio_pin_clear(LED_GREEN);
		m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		break;

		case BLE_GAP_EVT_DISCONNECTED:
		nrf_gpio_pin_set(LED_GREEN);
		nrf_gpio_pin_clear(LED_RED);

		m_conn_handle = BLE_CONN_HANDLE_INVALID;
		break;

		default:
	    // No implementation needed.
		break;
	}
}

void conn_params_error_handler(uint32_t nrf_error)
{
	APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{

	switch (ble_adv_evt)
	{
		case BLE_ADV_EVT_FAST:

		break;
		case BLE_ADV_EVT_IDLE:
		//sleep_mode_enter();
		break;
		default:
		break;
	}
}
/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
void gap_params_init(void)
{
	uint32_t                err_code;
	ble_gap_conn_params_t   gap_conn_params;
	ble_gap_conn_sec_mode_t sec_mode;

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

	err_code = sd_ble_gap_device_name_set(
		    &sec_mode
		   ,(const uint8_t *) DEVICE_NAME
		   ,strlen(DEVICE_NAME));
	
	APP_ERROR_CHECK(err_code);

	err_code = sd_ble_gap_appearance_set( 	BLE_APPEARANCE_GENERIC_CLOCK   );
	APP_ERROR_CHECK(err_code); 


	memset(&gap_conn_params, 0, sizeof(gap_conn_params));

	gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
	gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
	gap_conn_params.slave_latency     = SLAVE_LATENCY;
	gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;
	printf("max_conn_interval %x\n",MAX_CONN_INTERVAL);
	printf("min_conn_interval %x\n",MIN_CONN_INTERVAL);
	printf("min_conn_interval %x\n",SLAVE_LATENCY);
	printf("min_conn_interval %x\n",CONN_SUP_TIMEOUT);
	err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(err_code);
		
}
  
/**@brief Function for initializing services that will be used by the application.
 */




void services_init(void)
{
	custom_service_init (&m_custom_service);
}


 void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
	uint32_t err_code;

	if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
	{
		err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
		APP_ERROR_CHECK(err_code);
	}
}



#define SEC_PARAM_BOND                   1                                          /**< Perform bonding. */
#define SEC_PARAM_MITM                   0                                          /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES        BLE_GAP_IO_CAPS_NONE                       /**< No I/O capabilities. */
#define SEC_PARAM_OOB                    0                                          /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE           7                                          /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE           16                                         /**< Maximum encryption key size. */


#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                          /**< Number of attempts before giving up the connection parameter negotiation. */

static dm_application_instance_t        m_app_handle;                               /**< Application identifier allocated by device manager */

/**@brief GAP setting and init.
 */

 void advertising_init(void)
{
	uint32_t      err_code;
	ble_advdata_t advdata;

        
	memset(&advdata, 0, sizeof(advdata));

	advdata.name_type               = BLE_ADVDATA_FULL_NAME;
	advdata.include_appearance      = true;
	advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
	advdata.uuids_complete.p_uuids  = m_adv_uuids;

	ble_adv_modes_config_t options = {0};
	options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
	options.ble_adv_fast_interval = APP_ADV_INTERVAL;
	options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

	err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt, NULL);
	APP_ERROR_CHECK(err_code);
}

void conn_params_init(void)
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




uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
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


 void device_manager_init(bool erase_bonds)
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



void ble_send(uint8_t *p_string, uint8_t length){

	ble_nus_string_send(&m_custom_service, p_string, length);
}
