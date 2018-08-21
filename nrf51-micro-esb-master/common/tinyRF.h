#ifndef _TINY_RF_H
#define _TINY_RF_H
#include "nrf51.h"
#include "nrf51_bitfields.h"
#include "stdint.h"
typedef void (*tinyRF_event_handler_t)(void);


typedef enum {tinytx_ADDRESS_PIPE0, tinytx_ADDRESS_PIPE1, tinytx_ADDRESS_PIPE2, tinytx_ADDRESS_PIPE3, tinytx_ADDRESS_PIPE4, tinytx_ADDRESS_PIPE5, tinytx_ADDRESS_PIPE6, tinytx_ADDRESS_PIPE7} tinytx_address_type_t;

typedef struct
{
    uint8_t length;
    uint8_t pipe;
    int8_t  rssi;
    uint8_t noack;
    uint8_t data[32];
}tiny_payload_t;
typedef struct
{
   
    tinyRF_event_handler_t    event_handler;

    // General RF parameters
    uint8_t          bitrate;
    uint8_t            crc;
    uint8_t                 rf_channel;
    uint8_t                 payload_length;
    uint8_t                 rf_addr_length;

    uint8_t		    tx_output_power;
    uint8_t                 tx_address[5];
    uint8_t                 rx_address_p0[5];
    uint8_t                 rx_address_p1[5];
    uint8_t                 rx_address_p2;
    uint8_t                 rx_address_p3;
    uint8_t                 rx_address_p4;
    uint8_t                 rx_address_p5;
    uint8_t                 rx_address_p6;
    uint8_t                 rx_address_p7;
    uint8_t                 rx_pipes_enabled;

    
    uint8_t                 radio_irq_priority;
}tiny_rf_config_t;

  
   
 
#define TYNY_RF_DEFAULT_CONFIG { .event_handler         = 0,                                \
                             .rf_channel            = 5,                                \
                             .payload_length        = 32,     \
                             .rf_addr_length        = 5,                                \
                             .bitrate               = RADIO_MODE_MODE_Nrf_2Mbit,               \
                             .crc                   = RADIO_CRCCNF_LEN_Two,                   \
                             .tx_output_power       = RADIO_TXPOWER_TXPOWER_0dBm,               \
                             .rx_address_p0         = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7},   \
                             .rx_address_p1         = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2},   \
                             .rx_address_p2         = 0xC3,                             \
                             .rx_address_p3         = 0xC4,                             \
                             .rx_address_p4         = 0xC5,                             \
                             .rx_address_p5         = 0xC6,                             \
                             .rx_address_p6         = 0xC7,                             \
                             .rx_address_p7         = 0xC8,                             \
                             .rx_pipes_enabled      = 0x3F,                             \
                             .radio_irq_priority    = 1}

		
void load_tiny_radio_parameters(void);			 
uint32_t tiny_init(tiny_payload_t*  tx_payload);
void tiny_tx_transaction(tiny_payload_t*  tx_payload);

uint32_t tiny_set_address(tinytx_address_type_t address, const uint8_t *data_ptr);		
#endif
