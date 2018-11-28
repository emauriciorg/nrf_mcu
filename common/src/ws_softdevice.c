#include "ws_softdevice.h"
#include "pstorage.h"
#include "softdevice_handler.h"

#ifdef TIMESLOT_ENABLE
#include "ws_timeslot.h"
#endif
#include "ws_ble.h"

#define DEBUG_BLE_CONNECTION
#ifdef DEBUG_BLE_CONNECTION
	#define BLE_DBG(...)  printf(__VA_ARGS__)
#else
	#define BLE_DBG(...)  
#endif

#define IS_SRVC_CHANGED_CHARACT_PRESENT  1                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define CENTRAL_LINK_COUNT              0 /**<number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT           1 /**<number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

void sys_evt_dispatch(uint32_t sys_evt)
{
	pstorage_sys_event_handler(sys_evt);
	ble_advertising_on_sys_evt(sys_evt);
	
	#ifdef TIMESLOT_ENABLE
	nrf_evt_signal_handler(sys_evt); // check this funtion!
	#endif
}

 void ws_ble_stack_init(void)
 {
	uint32_t err_code;

	SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

	
#if defined(S110) || defined(S130) || defined(S132)
    // Enable BLE stack.
	ble_enable_params_t ble_enable_params;
	memset(&ble_enable_params, 0, sizeof(ble_enable_params));

	 err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
						    PERIPHERAL_LINK_COUNT,
						    &ble_enable_params);
    BLE_DBG ("%s, %d\n",__FUNCTION__, err_code);
	
    APP_ERROR_CHECK(err_code);
 
  //CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);

#if (defined(S130) || defined(S132))
	ble_enable_params.gatts_enable_params.attr_tab_size   = 0x500 ;//BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
	
	ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;


//#define OLD_SDK_VERSION
#ifndef OLD_SDK_VERSION

	#ifdef S130 
	#warning "RAM_APP_START_AT 0x20002800"
		#define RAM_APP_START_AT  0x20002800
	#else
		#warning "RAM_APP_START_AT 0x20001F00"
		#define RAM_APP_START_AT  0x20001F00
	
	#endif	 
	uint32_t   app_ram_base = RAM_APP_START_AT;
	
	 err_code = sd_ble_enable(&ble_enable_params,&app_ram_base);	
#else
	 err_code = sd_ble_enable(&ble_enable_params);	
#endif
	BLE_DBG ("%s, %d\n",__FUNCTION__, err_code);
	APP_ERROR_CHECK(err_code);	
#endif

    // Register with the SoftDevice handler module for BLE events.
	err_code = softdevice_ble_evt_handler_set(ws_ble_evt_dispatch);
	BLE_DBG ("%s, %d\n",__FUNCTION__, err_code);
	APP_ERROR_CHECK(err_code);
	
    // Register with the SoftDevice handler module for BLE events.
	err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
	BLE_DBG ("%s, %d\n",__FUNCTION__, err_code);
	APP_ERROR_CHECK(err_code);
}	
