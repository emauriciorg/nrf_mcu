
#ifndef _CAFE_PROTOCOL_H_
#define _CAFE_PROTOCOL_H_

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
//#include "nrf51.h"
//#include "nrf51_bitfields.h"

#include "cafe_data_types.h"


bool     cafe_is_idle(void);
char     cafe_get_rx_payload(char *out_buffer);

int8_t   cafe_get_rssi(void);

uint32_t cafe_read_rx_payload(cafe_payload_t *payload);
uint32_t cafe_disable(void);
uint32_t cafe_write_tx_payload(cafe_payload_t *payload);
uint32_t cafe_start_tx(void);
uint32_t cafe_get_tx_attempts(uint32_t *attempts);
uint32_t cafe_get_clear_interrupts(uint32_t *interrupts);
uint32_t cafe_set_rf_channel(uint32_t channel);
uint32_t cafe_set_tx_power(uint8_t tx_output_power);


void cafe_update_nrf_radio_address(nrf_st_address radio_addr);
void cafe_self_configuration(uint8_t transeciever_mode);
void cafe_start_tx_transcation(void);
void cafe_load_payload(unsigned char slave_id, char *data,unsigned char len);
char cafe_packet_recieved(void);
void RADIO_IRQHandler(void);
#endif
