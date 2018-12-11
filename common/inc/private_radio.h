
#ifndef _CAFE_PROTOCOL_H_
#define _CAFE_PROTOCOL_H_

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "radio_data_types.h"

#define NO_CYPHER
#define MAX_SLAVES_AVAILABLE 4
#define AES_BYTES_LENGHT 4 

#define RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS   48   // Smallest reliable value - 43
#define RX_WAIT_FOR_ACK_TIMEOUT_US_1MBPS   64   // Smallest reliable value - 59
#define RX_WAIT_FOR_ACK_TIMEOUT_US_250KBPS 250

#define DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)


#define RADIO_SHORTS_COMMON ( RADIO_SHORTS_READY_START_Msk         |\
				RADIO_SHORTS_END_DISABLE_Msk       |\
				RADIO_SHORTS_ADDRESS_RSSISTART_Msk |\
				RADIO_SHORTS_DISABLED_RSSISTOP_Msk )


char     radioget_rx_payload(char *out_buffer);
int8_t   radio_get_rssi(void);
bool     radio_is_idle(void);
void radio_start(void);
char radio_rx_packet_available(void);
void radio_update_mode(radio_mode_t radio_mode);
void radio_load_payload(uint8_t pipe_id, char *data,unsigned char len);

uint32_t radio_get_tx_attempts(uint32_t *attempts); //NO IMPLEMENTED!

void RADIO_IRQHandler(void);
#endif
