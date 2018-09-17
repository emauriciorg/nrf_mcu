#include "bleAPP.h"
#include "sys_event.h"
#include "ble_conn_params.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble.h"
#include "device_manager.h"

#include "nrf_gpio.h"
#include "bbn_board.h"
#define IS_SRVC_CHANGED_CHARACT_PRESENT  1                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/
#define DEVICE_NAME                      "Nordic_T1"                               /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME                "NordicSemiconductor"                      /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                 300                                        /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS       180                                        /**< The advertising timeout in units of seconds. */


#define MIN_CONN_INTERVAL                MSEC_TO_UNITS(100, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL                MSEC_TO_UNITS(200, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                    0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                 MSEC_TO_UNITS(4000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */
 uint16_t                          m_conn_handle = BLE_CONN_HANDLE_INVALID;   /**< Handle of the current connection. */

static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}}; /**< Universally unique service identifiers. */
static dm_application_instance_t        m_app_handle;                               /**< Application identifier allocated by device manager */


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
 void ble_stack_init(void)
{
	uint32_t err_code;

    // Initialize the SoftDevice handler module.


	SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);
//	SOFTDEVICE_HANDLER_INIT(clock_lf_cfg, NULL);
//		nrf_gpio_pin_set(LED_GREEN);
#if defined(S110) || defined(S130) || defined(S132)

    // Enable BLE stack.
	ble_enable_params_t ble_enable_params;
	memset(&ble_enable_params, 0, sizeof(ble_enable_params));

#if (defined(S130) || defined(S132))
	ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
	//nrf_gpio_pin_set(LED_GREEN);
	
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
/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
 void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
	dm_ble_evt_handler(p_ble_evt);
	ble_conn_params_on_ble_evt(p_ble_evt);
	//bsp_btn_ble_on_ble_evt(p_ble_evt);
	on_ble_evt(p_ble_evt);
	ble_advertising_on_ble_evt(p_ble_evt);
    /*YOUR_JOB add calls to _on_ble_evt functions from each service your application is using
    ble_xxs_on_ble_evt(&m_xxs, p_ble_evt);
    ble_yys_on_ble_evt(&m_yys, p_ble_evt);
    */
}



/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
 void on_ble_evt(ble_evt_t * p_ble_evt)
{
	//uint32_t err_code;

	switch (p_ble_evt->header.evt_id)
	{
		case BLE_GAP_EVT_CONNECTED:
//		err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
	//	APP_ERROR_CHECK(err_code);
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
#if 0
void on_ble_evt(ble_evt_t * p_ble_evt)
{
	uint32_t                         err_code;
	switch (p_ble_evt->header.evt_id){

	case BLE_GAP_EVT_CONNECTED:
		//printf("ble connecting\n");

		err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
		APP_ERROR_CHECK(err_code);
		m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		rssi_indicator= sd_ble_gap_rssi_start(p_ble_evt->evt.gap_evt.conn_handle,1,4);
		printf("ble connected\n");
		ble_connection_state=1;

	break;
	case BLE_GAP_EVT_DISCONNECTED:
		err_code = bsp_indication_set(BSP_INDICATE_IDLE);
		APP_ERROR_CHECK(err_code);
		m_conn_handle = BLE_CONN_HANDLE_INVALID;
		printf("ble disconnected\n");
		ble_connection_state=0;

	break;
	case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
	    // Pairing not supported
		printf("params request\n");

		err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);

//	    err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
		APP_ERROR_CHECK(err_code);
	break;
	case BLE_GATTS_EVT_SYS_ATTR_MISSING:
		printf("sys attr missing\n");
	    // No system attributes have been stored.
		err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
		APP_ERROR_CHECK(err_code);
	break;
	case BLE_GAP_EVT_ADV_REPORT:

		printf("scaned \n");//,p_ble_evt->evt.gap_evt.params.adv_report.data);	
	break;
	default:
	    // No implementation needed.
	break;
	}
}
#endif
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
	uint32_t err_code;

	switch (ble_adv_evt)
	{
		case BLE_ADV_EVT_FAST:
	//	err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
		//APP_ERROR_CHECK(err_code);
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


/**@brief Function for handling the YYY Service events. 
 * YOUR_JOB implement a service handler function depending on the event the service you are using can generate
 *
 * @details This function will be called for all YY Service events which are passed to
 *          the application.
 *
 * @param[in]   p_yy_service   YY Service structure.
 * @param[in]   p_evt          Event received from the YY Service.
 *
 *
static void on_yys_evt(ble_yy_service_t     * p_yy_service, 
		       ble_yy_service_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
	case BLE_YY_NAME_EVT_WRITE:
	    APPL_LOG("[APPL]: charact written with value %s. \r\n", p_evt->params.char_xx.value.p_str);
	    break;
	
	default:
	    // No implementation needed.
	    break;
    }
}*/


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


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
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

/**@brief Function for handling the Device Manager events.
 *
 * @param[in] p_evt  Data associated to the device manager event.
 */
 uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
	dm_event_t const  * p_event,
	ret_code_t        event_result)
{
	APP_ERROR_CHECK(event_result);

#ifdef BLE_DFU_APP_SUPPORT
	if (p_event->event_id == DM_EVT_LINK_SECURED)
	{
		app_context_load(p_handle);
	}
#endif // BLE_DFU_APP_SUPPORT

	return NRF_SUCCESS;
}

/**@brief Function for the Device Manager initialization.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Device Manager.
 */
void device_manager_init(bool erase_bonds)
{
	uint32_t               err_code;
	dm_init_param_t        init_param = {.clear_persistent_data = erase_bonds};
	dm_application_param_t register_param;

    // Initialize persistent storage module.
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



