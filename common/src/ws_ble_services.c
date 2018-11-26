#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "ble_srv_common.h"
#include "app_error.h"
#include "ble_types.h"

#include "ws_ble_services.h"
#include "cafe.h"

//#define DEBUG_BLE_SERVICE
#ifdef DEBUG_BLE_SERVICE
	#define BLE_DBG(...)  printf(__VA_ARGS__)
#else
	#define BLE_DBG(...)  
#endif

#define BLE_UUID_OUR_BASE_UUID {{0x55,0xb4,0x3a,0x03,0xbf,0x43,45,0x18,0xbd,0xbb,0xdd,0xaf,0x3e,0xa7,0xad,0xc6}}
#define BLE_UUID_OUR_SERVICE 0XAB
#define BLE_UUID_CS_TX_CHARACTERISTIC 0x0002                      /**< The UUID of the TX Characteristic. */
#define BLE_UUID_CS_RX_CHARACTERISTIC 0x0003                      /**< The UUID of the RX Characteristic. */
#define BLE_CUS_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */


void ws_ble_service_data_handler(ble_cs_t * p_custom_service, uint8_t * p_data, uint16_t length)
{

/*used for rssi
memset(nus_buffer_rx, 0,24);
length=sprintf((char *)nus_buffer_rx,"%d",rssi_indicator);

	for (uint32_t i = 0; i < length; i++)
	{
	while(	app_uart_put(nus_buffer_rx[i]) != NRF_SUCCESS);
	}    while(app_uart_put('\n') != NRF_SUCCESS);
*/
	uint8_t data[40];
	memset(data,0,40);
	memcpy(data,p_data, length);
	printf("Recieve [%s] \n", data);
 
//    printf("slave [%d] string [%s] len[%d] \n", command_id[0],argv, strlen(argv));
	cafe_load_payload((unsigned char)(data[0]-'0'),(char *) &data[2], strlen((const char*	)(data+2)) );

}



static void on_write(ble_cs_t * p_custom_service, ble_evt_t * p_ble_evt)
{
	ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

	if ((p_evt_write->handle == p_custom_service->rx_handles.cccd_handle)&&
		(p_evt_write->len == 2)){

		if (ble_srv_is_notification_enabled(p_evt_write->data)){
			p_custom_service->is_notification_enabled = true;
		}
		else{
			p_custom_service->is_notification_enabled = false;
		}
	}
	else if ( (p_evt_write->handle == p_custom_service->tx_handles.value_handle)&&
		(p_custom_service->data_handler != NULL)){
		p_custom_service->data_handler(p_custom_service, p_evt_write->data, p_evt_write->len);
	}
	else{
		// Do Nothing. This event is not relevant for this service.
	}
}



void ws_ble_service_on_evt(ble_cs_t * p_custom_service, ble_evt_t * p_ble_evt)
{
	if ((p_custom_service == NULL) || (p_ble_evt == NULL)){
		return;
	}

	switch (p_ble_evt->header.evt_id){
	
	case BLE_GAP_EVT_CONNECTED:
		//on_connect(p_custom_service, p_ble_evt);
		p_custom_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		break;
	case BLE_GAP_EVT_DISCONNECTED:
		//on_disconnect(p_custom_service, p_ble_evt);
		break;
	case BLE_GATTS_EVT_WRITE:
		on_write(p_custom_service, p_ble_evt);
		break;
	default:
		// No implementation needed.
		break;
	}
}
void ws_ble_service_init(ble_cs_t * p_custom_service)
{

	uint32_t          err_code;
	ble_uuid_t        service_uuid;
	ble_uuid128_t     base_uuid = BLE_UUID_OUR_BASE_UUID;
	
	p_custom_service->conn_handle             = BLE_CONN_HANDLE_INVALID;
	p_custom_service->data_handler            =ws_ble_service_data_handler;// p_ws_ble_service_init->data_handler;
	p_custom_service->is_notification_enabled = true;
	err_code = sd_ble_uuid_vs_add(&base_uuid, &p_custom_service->uuid_type);
	APP_ERROR_CHECK(err_code);
	
	service_uuid.type = p_custom_service->uuid_type;
	service_uuid.uuid = BLE_UUID_OUR_SERVICE;

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
										&service_uuid,
										&p_custom_service->service_handle);
	APP_ERROR_CHECK(err_code);
	
	err_code = ws_services_add_char_rx(p_custom_service);
	BLE_DBG("rx got error %x\n", err_code);
	APP_ERROR_CHECK(err_code);

	err_code = ws_services_add_char_tx(p_custom_service);
	BLE_DBG("tx got error %x\n", err_code);
	APP_ERROR_CHECK(err_code);


	BLE_DBG( "Executing our_service_init().\n");
	BLE_DBG( "Service UUID: %x\n", service_uuid.uuid);
	BLE_DBG( "Service UUID type: %x\n", service_uuid.type);
	BLE_DBG( "Service handle: %x\n", p_custom_service->service_handle);


}



 uint32_t ws_services_add_char_rx(ble_cs_t * p_custom_service)
{
	/**@snippet [Adding proprietary characteristic to S110 SoftDevice] */
	ble_gatts_char_md_t char_md;
	ble_gatts_attr_md_t cccd_md;
	ble_gatts_attr_t    attr_char_value;
	ble_uuid_t          ble_uuid;
	ble_gatts_attr_md_t attr_md;

	memset(&cccd_md, 0, sizeof(cccd_md));
 
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

	cccd_md.vloc = BLE_GATTS_VLOC_STACK;

	memset(&char_md, 0, sizeof(char_md));

	char_md.char_props.notify = 1;
	char_md.p_char_user_desc  = NULL;
	char_md.p_char_pf         = NULL;
	char_md.p_user_desc_md    = NULL;
	char_md.p_cccd_md         = &cccd_md;
	char_md.p_sccd_md         = NULL;

	ble_uuid.type = p_custom_service->uuid_type;
	ble_uuid.uuid = BLE_UUID_CS_RX_CHARACTERISTIC;

	memset(&attr_md, 0, sizeof(attr_md));

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

	attr_md.vloc    = BLE_GATTS_VLOC_STACK;
	attr_md.rd_auth = 0;
	attr_md.wr_auth = 0;
	attr_md.vlen    = 1;

	memset(&attr_char_value, 0, sizeof(attr_char_value));

	attr_char_value.p_uuid    = &ble_uuid;
	attr_char_value.p_attr_md = &attr_md;
	attr_char_value.init_len  = sizeof(uint8_t);
	attr_char_value.init_offs = 0;
	attr_char_value.max_len   = BLE_CUS_MAX_DATA_LEN;

	return sd_ble_gatts_characteristic_add(p_custom_service->service_handle,
										   &char_md,
										   &attr_char_value,
										   &p_custom_service->rx_handles);
}


uint32_t ws_services_add_char_tx(ble_cs_t * p_custom_service){

	ble_gatts_char_md_t char_md;
	ble_gatts_attr_t    attr_char_value;
	ble_uuid_t          ble_uuid;
	ble_gatts_attr_md_t attr_md;

	memset(&char_md, 0, sizeof(char_md));

	char_md.char_props.write         = 1;
	char_md.char_props.write_wo_resp = 1;
	char_md.p_char_user_desc         = NULL;
	char_md.p_char_pf                = NULL;
	char_md.p_user_desc_md           = NULL;
	char_md.p_cccd_md                = NULL;
	char_md.p_sccd_md                = NULL;

	ble_uuid.type = p_custom_service->uuid_type;
	ble_uuid.uuid = BLE_UUID_CS_TX_CHARACTERISTIC;

	memset(&attr_md, 0, sizeof(attr_md));

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

	attr_md.vloc    = BLE_GATTS_VLOC_STACK;
	attr_md.rd_auth = 0;
	attr_md.wr_auth = 0;
	attr_md.vlen    = 1;

	memset(&attr_char_value, 0, sizeof(attr_char_value));

	attr_char_value.p_uuid    = &ble_uuid;
	attr_char_value.p_attr_md = &attr_md;
	attr_char_value.init_len  = 1;
	attr_char_value.init_offs = 0;
	attr_char_value.max_len   = BLE_CUS_MAX_DATA_LEN;

	return sd_ble_gatts_characteristic_add(p_custom_service->service_handle,
						&char_md,
						&attr_char_value,
						&p_custom_service->tx_handles);


}



uint32_t ws_ble_nus_string_send(ble_cs_t * p_custom_service, uint8_t * p_string, uint16_t length)
{
	ble_gatts_hvx_params_t hvx_params;

	//VERIFY_PARAM_NOT_NULL(p_custom_service);

	if ((p_custom_service->conn_handle == BLE_CONN_HANDLE_INVALID) ||
	    (!p_custom_service->is_notification_enabled)){
		BLE_DBG("INVALID CONN HANDLE/NOTIFICIATION DISABLED\n");
		return NRF_ERROR_INVALID_STATE;
	}

	if (length > BLE_CUS_MAX_DATA_LEN){
		BLE_DBG("max lent reached/ data not send\n");
		return NRF_ERROR_INVALID_PARAM;
	}
	memset(&hvx_params, 0, sizeof(hvx_params));

	hvx_params.handle = p_custom_service->rx_handles.value_handle;
	hvx_params.p_data = p_string;
	hvx_params.p_len  = &length;
	hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

	return sd_ble_gatts_hvx(p_custom_service->conn_handle, &hvx_params);
}




void ble_nus_ws_on_ble_evt(ble_cs_t * p_custom_service, ble_evt_t * p_ble_evt)
{
	
	if ((p_custom_service == NULL) || (p_ble_evt == NULL)){
		return;
	}

	switch (p_ble_evt->header.evt_id){
	
	case BLE_GAP_EVT_CONNECTED:
	//on_connect(p_custom_service, p_ble_evt);
		break;
	case BLE_GAP_EVT_DISCONNECTED:
	//on_disconnect(p_custom_service, p_ble_evt);
		break;
	case BLE_GATTS_EVT_WRITE:
		on_write(p_custom_service, p_ble_evt);
		break;
	default:
		// No implementation needed.
	break;
	}
}
