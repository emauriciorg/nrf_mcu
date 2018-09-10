/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#ifndef __cafe_ESB_H
#define __cafe_ESB_H

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"
#include "uesb_error_codes.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

#define S1

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
#define     CAFE_CORE_MAX_PAYLOAD_LENGTH    32
#define     cafe_CORE_TX_FIFO_SIZE          8
#define     cafe_CORE_RX_FIFO_SIZE          8

#define     cafe_SYS_TIMER                  NRF_TIMER2
#define     cafe_SYS_TIMER_IRQ_Handler      TIMER2_IRQHandler

#define     cafe_PPI_TIMER_START            4
#define     cafe_PPI_TIMER_STOP             5
#define     cafe_PPI_RX_TIMEOUT             6
#define     cafe_PPI_TX_START               7

// Interrupt flags
#define     cafe_INT_TX_SUCCESS_MSK         0x01
#define     cafe_INT_TX_FAILED_MSK          0x02
#define     cafe_INT_RX_DR_MSK              0x04

#define     cafe_PID_RESET_VALUE            0xFF


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

// Default radio parameters, roughly equal to nRF24L default parameters (except CRC which is set to 16-bit, and protocol set to DPL)
#define cafe_DEFAULT_CONFIG {.mode                  = I_AM_TRANSMITTER,                    \
                             .event_handler         = 0,                                \
                             .rf_channel            = 2,                                \
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
    cafe_payload_t *payload_ptr[cafe_CORE_TX_FIFO_SIZE];
    uint32_t        count;
}cafe_payload_tx_fifo_t;

typedef struct
{
    cafe_payload_t *payload_ptr[cafe_CORE_RX_FIFO_SIZE];
    uint32_t        count;
}cafe_payload_rx_fifo_t;

uint32_t cafe_init(cafe_config_t *parameters);


bool     cafe_is_idle(void);

uint32_t cafe_write_tx_payload(cafe_payload_t *payload);

uint32_t cafe_write_ack_payload(cafe_payload_t *payload);

uint32_t cafe_read_rx_payload(cafe_payload_t *payload);

uint32_t cafe_start_tx(void);

uint32_t cafe_start_rx(void);

uint32_t cafe_stop_rx(void);

uint32_t cafe_get_tx_attempts(uint32_t *attempts);

uint32_t cafe_flush_tx(void);

uint32_t cafe_flush_rx(void);

uint32_t cafe_get_clear_interrupts(uint32_t *interrupts);

uint32_t cafe_set_address(nrf_st_address address, const uint8_t *data_ptr);

void get_rf_packet(cafe_payload_t *dst_buffer);

void cafe_setup_rx(void);
void get_rx_payload(uint8_t *out_buffer);

int8_t get_rssi(void);


void update_nrf_radio_address(nrf_st_address radio_addr);

#endif
