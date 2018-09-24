
#ifndef __CAFE_RX_H__
#define __CAFE_RX_H__

#include "cafe_data_types.h"


/*Common rx, tx functions*/
uint32_t cafe_init(cafe_config_t *parameters);
bool     cafe_is_idle(void);
bool     cafe_get_transciever_state(void);




/*Reception related functions*/
int8_t get_rssi(void);

uint32_t cafe_start_rx(void);

uint32_t cafe_stop_rx(void);

uint32_t cafe_get_clear_interrupts(uint32_t *interrupts);

uint32_t cafe_read_rx_payload(cafe_payload_t *payload);

void get_rx_payload(uint8_t *out_buffer);

void get_rf_packet(cafe_payload_t *dst_buffer);
void cafe_setup_rx(void);
void update_nrf_radio_address(nrf_st_address radio_addr);








/*Emition related functions*/

uint32_t cafe_start_tx(void);


#endif
