
#ifndef _CAFE_PROTOCOL_H_
#define _CAFE_PROTOCOL_H_

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "radio_data_types.h"


#define NO_CYPHER

/*TODO: add descriptors*/
int8_t   radio_get_rssi(void);


char radioget_rx_payload(char *out_buffer);
char radio_rx_packet_available(void);
void radio_update_mode(radio_mode_t radio_mode);
void radio_load_payload(uint8_t pipe_id, char *data,unsigned char len);
void radio_start(void);

void radio_print_current_state(void);
void radio_start_task(void);
void RADIO_IRQHandler(void);

#endif
