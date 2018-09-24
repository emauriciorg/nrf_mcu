#ifndef _CAFE_DATATYPE_H_
#define _CAFE_DATATYPE_H_


#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"
#include "uesb_error_codes.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

#define S2

#ifdef S1
	#define SLAVE_addr 0xF4 
#endif

#ifdef S2
	#define SLAVE_addr 0xAA
#endif
#ifdef S3
	#define SLAVE_addr 0x12
#endif


// Hard coded parameters - change if necessary
#define     CAFE_CORE_MAX_PAYLOAD_LENGTH    8//32
#define     cafe_CORE_RX_FIFO_SIZE          8

#define     cafe_SYS_TIMER                  NRF_TIMER2
#define     cafe_SYS_TIMER_IRQ_Handler      TIMER2_IRQHandler

#define     cafe_PPI_TIMER_START            4
#define     cafe_PPI_TIMER_STOP             5
#define     cafe_PPI_RX_TIMEOUT             6

// Interrupt flags
#define     cafe_INT_RX_DR_MSK              0x04




typedef struct  {
                 uint8_t   base_addr0_tx[5];
                 uint8_t   base_addr0 [5]  ;
                 uint8_t   base_addr1 [5]  ;
                 uint8_t   logic_pipe [8]  ;
}nrf_st_address;

typedef enum {
    I_AM_TRANSMITTER,          // Primary transmitter
    I_AM_RECIEVER           // Primary receiver
} cafe_mode;



//nrf51_bitfields
typedef enum {
    cafe_2MBPS   = RADIO_MODE_MODE_Nrf_2Mbit,
    cafe_1MBPS   = RADIO_MODE_MODE_Nrf_1Mbit,
    cafe_250KBPS = RADIO_MODE_MODE_Nrf_250Kbit
} cafe_bitrate_t;

typedef enum {
    cafe_CRC_16BIT = RADIO_CRCCNF_LEN_Two,
    cafe_CRC_8BIT  = RADIO_CRCCNF_LEN_One,
    cafe_CRC_OFF   = RADIO_CRCCNF_LEN_Disabled
} cafe_crc_t;

typedef enum {
    cafe_TX_POWER_4DBM     = RADIO_TXPOWER_TXPOWER_Pos4dBm,
    cafe_TX_POWER_0DBM     = RADIO_TXPOWER_TXPOWER_0dBm,
    cafe_TX_POWER_NEG4DBM  = RADIO_TXPOWER_TXPOWER_Neg4dBm,
    cafe_TX_POWER_NEG8DBM  = RADIO_TXPOWER_TXPOWER_Neg8dBm,
    cafe_TX_POWER_NEG12DBM = RADIO_TXPOWER_TXPOWER_Neg12dBm,
    cafe_TX_POWER_NEG16DBM = RADIO_TXPOWER_TXPOWER_Neg16dBm,
    cafe_TX_POWER_NEG20DBM = RADIO_TXPOWER_TXPOWER_Neg20dBm,
    cafe_TX_POWER_NEG30DBM = RADIO_TXPOWER_TXPOWER_Neg30dBm
} cafe_tx_power_t;

typedef enum {
    cafe_TXMODE_AUTO,        // Automatic TX mode - When the TX fifo is non-empty and the radio is idle packets will be sent automatically.
    cafe_TXMODE_MANUAL,      // Manual TX mode - Packets will not be sent until cafe_start_tx() is called. Can be used to ensure consistent packet timing.
    cafe_TXMODE_MANUAL_START // Manual start TX mode - Packets will not be sent until cafe_start_tx() is called, but transmission will continue automatically until the TX FIFO is empty.
} cafe_tx_mode_t;

typedef void (*cafe_event_handler_t)(void);

// Main UESB configuration struct, contains all radio parameters
typedef struct
{
   
    cafe_mode             mode;
    cafe_event_handler_t    event_handler;

    // General RF parameters
    cafe_bitrate_t          bitrate;
    cafe_crc_t              crc;
    uint8_t                 rf_channel;
    uint8_t                 payload_length;
    uint8_t                 rf_addr_length;

    cafe_tx_power_t         tx_output_power;
    nrf_st_address          radio_addresses;

    uint8_t                 rx_pipes_enabled;

    // Control settings
    cafe_tx_mode_t          tx_mode;

    uint8_t                 radio_irq_priority;
}cafe_config_t;

#define cafe_DEFAULT_CONFIG {.mode                  = I_AM_RECIEVER,                    \
                             .event_handler         = cafe_event_handler_rx,                                \
                             .rf_channel            = 5,                                \
                             .payload_length        = CAFE_CORE_MAX_PAYLOAD_LENGTH,     \
                             .rf_addr_length        = 5,                                \
                             .bitrate               = cafe_2MBPS,               \
                             .crc                   = cafe_CRC_16BIT,                   \
                             .tx_output_power       = cafe_TX_POWER_0DBM,               \
                             .tx_mode               = cafe_TXMODE_AUTO,                 \
                             .radio_irq_priority    = 1}


	


typedef struct
{
    uint8_t length;
    uint8_t pipe;
    int8_t  rssi;
    uint8_t noack;
    uint8_t data[CAFE_CORE_MAX_PAYLOAD_LENGTH];
}cafe_payload_t;



typedef struct
{
    cafe_payload_t *payload_ptr[cafe_CORE_RX_FIFO_SIZE];
    uint32_t        count;
}cafe_payload_rx_fifo_t;

#endif
