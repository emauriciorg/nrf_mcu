#ifndef _BLE_APP_
#define _BLE_APP_

#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"

#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"


#define SEC_PARAM_BOND                   1                                          /**< Perform bonding. */
#define SEC_PARAM_MITM                   0                                          /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES        BLE_GAP_IO_CAPS_NONE                       /**< No I/O capabilities. */
#define SEC_PARAM_OOB                    0                                          /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE           7                                          /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE           16                                         /**< Maximum encryption key size. */


void ble_stack_init(void);
void ble_evt_dispatch(ble_evt_t * p_ble_evt);
void on_ble_evt(ble_evt_t * p_ble_evt);

void advertising_init(void);
 void on_conn_params_evt(ble_conn_params_evt_t * p_evt);
 void services_init(void);;

void on_adv_evt(ble_adv_evt_t  ble_adv_evt);
void gap_params_init(void);


void device_manager_init(bool erase_bonds);

#endif

