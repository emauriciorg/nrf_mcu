#ifndef __CUSTOM_BLE_SERVICE_H__
#define __CUSTOM_BLE_SERVICE_H__
#include <stdint.h>
#include <stdbool.h>
#include <ble_gatts.h>
#include <ble.h>

typedef struct ble_cs_s ble_cs_t;


typedef void (*cs_data_handler) (ble_cs_t * p_custom_service, uint8_t * p_data, uint16_t length);


 struct ble_cs_s{  
    uint8_t                  uuid_type;               /**< UUID type for Nordic UART Service Base UUID. */
    uint16_t                 service_handle;          /**< Handle of Nordic UART Service (as provided by the SoftDevice). */
    ble_gatts_char_handles_t tx_handles;              /**< Handles related to the TX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t rx_handles;              /**< Handles related to the RX characteristic (as provided by the SoftDevice). */
    uint16_t                 conn_handle;             /**< Handle of the current connection (as provided by the SoftDevice). BLE_CONN_HANDLE_INVALID if not in a connection. */
    bool                     is_notification_enabled; /**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
    cs_data_handler   data_handler;            /**< Event handler to be called for handling received data. */

};



typedef struct
{
    cs_data_handler data_handler; /**< Event handler to be called for handling received data. */
} ble_cs_inint_t;


void custom_service_init(ble_cs_t * p_custom_service  );
uint32_t custom_services_add_char_rx(ble_cs_t * p_custom_service);
uint32_t custom_services_add_char_tx(ble_cs_t * p_custom_service);

void ble_cs_on_ble_evt(ble_cs_t * p_custom_service, ble_evt_t * p_ble_evt);
uint32_t ble_nus_string_send(ble_cs_t * p_custom_service, uint8_t * p_string, uint16_t length);

#endif
