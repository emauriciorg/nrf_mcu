#include "../inc/bleAPP.h"

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

#define IS_SRVC_CHANGED_CHARACT_PRESENT  1                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/
#define DEVICE_NAME                      "Master_V1"                               /**< Name of device. Will be included in the advertising data. */
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

 void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
	dm_ble_evt_handler(p_ble_evt);
	ble_conn_params_on_ble_evt(p_ble_evt);
	
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
		nrf_gpio_pin_clear(LED_BLUE);
		m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		break;

		case BLE_GAP_EVT_DISCONNECTED:
		nrf_gpio_pin_set(LED_BLUE);
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

	err_code = sd_ble_gap_device_name_set(&sec_mode,
		(const uint8_t *)DEVICE_NAME,
		strlen(DEVICE_NAME));
	APP_ERROR_CHECK(err_code);

    /* YOUR_JOB: Use an appearance value matching the application's use case.
    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_);
    APP_ERROR_CHECK(err_code); */

	memset(&gap_conn_params, 0, sizeof(gap_conn_params));

	gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
	gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
	gap_conn_params.slave_latency     = SLAVE_LATENCY;
	gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

	err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing services that will be used by the application.
 */
 void services_init(void)
{
    /* YOUR_JOB: Add code to initialize the services used by the application.
    uint32_t                           err_code;
    ble_xxs_init_t                     xxs_init;
    ble_yys_init_t                     yys_init;

    // Initialize XXX Service.
    memset(&xxs_init, 0, sizeof(xxs_init));

    xxs_init.evt_handler                = NULL;
    xxs_init.is_xxx_notify_supported    = true;
    xxs_init.ble_xx_initial_value.level = 100; 
    
    err_code = ble_bas_init(&m_xxs, &xxs_init);
    APP_ERROR_CHECK(err_code);

    // Initialize YYY Service.
    memset(&yys_init, 0, sizeof(yys_init));
    yys_init.evt_handler                  = on_yys_evt;
    yys_init.ble_yy_initial_value.counter = 0;

    err_code = ble_yy_service_init(&yys_init, &yy_init);
    APP_ERROR_CHECK(err_code);
    */
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


/**@brief Function for initializing the Advertising functionality.
 */
 void advertising_init(void)
{
	uint32_t      err_code;
	ble_advdata_t advdata;

    // Build advertising data struct to pass into @ref ble_advertising_init.
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
