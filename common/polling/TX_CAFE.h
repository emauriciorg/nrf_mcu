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

#ifndef __MICRO_CAFE_H
#define __MICRO_CAFE_H

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"

#define DEBUGPIN1   12
#define DEBUGPIN2   13
#define DEBUGPIN3   14
#define DEBUGPIN4   15



// Hard coded parameters - change if necessary
#define     CAFE_CORE_MAX_PAYLOAD_LENGTH    32
#define     CAFE_CORE_TX_FIFO_SIZE          8
#define     CAFE_CORE_RX_FIFO_SIZE          8

#define     CAFE_SYS_TIMER                  NRF_TIMER2
#define     CAFE_SYS_TIMER_IRQ_Handler      TIMER2_IRQHandler

#define     CAFE_PPI_TIMER_START            4
#define     CAFE_PPI_TIMER_STOP             5
#define     CAFE_PPI_RX_TIMEOUT             6
#define     CAFE_PPI_TX_START               7

// Interrupt flags
#define     CAFE_INT_TX_SUCCESS_MSK         0x01
#define     CAFE_INT_TX_FAILED_MSK          0x02
#define     CAFE_INT_RX_DR_MSK              0x04

#define     CAFE_PID_RESET_VALUE            0xFF

// Configuration parameter definitions
typedef struct  {
                               uint8_t   base_addr0_tx[5];
                               uint8_t   base_addr0[5];
                               uint8_t   base_addr1[5];
                               uint8_t   logic_pipe[8];
}nrf_st_address;

typedef enum {
    CAFE_MODE_PTX,          // Primary transmitter
    CAFE_MODE_PRX           // Primary receiver
} cafe_mode_t;

typedef enum {
    CAFE_BITRATE_2MBPS = RADIO_MODE_MODE_Nrf_2Mbit,
    CAFE_BITRATE_1MBPS = RADIO_MODE_MODE_Nrf_1Mbit,
    CAFE_BITRATE_250KBPS = RADIO_MODE_MODE_Nrf_250Kbit
} cafe_bitrate_t;

typedef enum {
    CAFE_CRC_16BIT = RADIO_CRCCNF_LEN_Two,
    CAFE_CRC_8BIT  = RADIO_CRCCNF_LEN_One,
    CAFE_CRC_OFF   = RADIO_CRCCNF_LEN_Disabled
} cafe_crc_t;


typedef enum {
    CAFE_TXMODE_AUTO,        // Automatic TX mode - When the TX fifo is non-empty and the radio is idle packets will be sent automatically.
    CAFE_TXMODE_MANUAL,      // Manual TX mode - Packets will not be sent until cafe_start_tx() is called. Can be used to ensure consistent packet timing.
    CAFE_TXMODE_MANUAL_START // Manual start TX mode - Packets will not be sent until cafe_start_tx() is called, but transmission will continue automatically until the TX FIFO is empty.
} cafe_tx_mode_t;

// Internal state definition
typedef enum {
    CAFE_STATE_UNINITIALIZED,
    CAFE_STATE_IDLE,
    CAFE_STATE_PTX_TX,
    CAFE_STATE_PTX_TX_ACK,
    CAFE_STATE_PTX_RX_ACK,
    CAFE_STATE_PRX,
    CAFE_STATE_PRX_SEND_ACK,
    CAFE_STATE_PRX_SEND_ACK_PAYLOAD
} cafe_mainstate_t;

typedef void (*cafe_event_handler_t)(void);

// Main CAFE configuration struct, contains all radio parameters
typedef struct
{
  //  cafe_protocol_t         protocol;
   
    cafe_event_handler_t    event_handler;

    // General RF parameters
    cafe_bitrate_t          bitrate;
    cafe_crc_t              crc;
    uint8_t                 rf_channel;
    uint8_t                 payload_length;
    uint8_t                 rf_addr_length;

    uint8_t       	    tx_output_power;
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

    // CAFE specific features
    uint8_t                 dynamic_payload_length_enabled;
    uint8_t                 dynamic_ack_enabled;
    uint16_t                retransmit_delay;
    uint16_t                retransmit_count;

    // Control settings
    cafe_tx_mode_t          tx_mode;

    uint8_t                 radio_irq_priority;
}cafe_config_t;

// Default radio parameters, roughly equal to nRF24L default parameters (except CRC which is set to 16-bit, and protocol set to DPL)
#define CAFE_DEFAULT_CONFIG {.event_handler         = 0,                                \
                             .rf_channel            = 2,                                \
                             .payload_length        = CAFE_CORE_MAX_PAYLOAD_LENGTH,     \
                             .rf_addr_length        = 5,                                \
                             .bitrate               = CAFE_BITRATE_2MBPS,               \
                             .crc                   = CAFE_CRC_16BIT,                   \
                             .tx_output_power       = RADIO_TXPOWER_TXPOWER_0dBm,               \
                             .rx_address_p0         = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7},   \
                             .rx_address_p1         = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2},   \
                             .rx_address_p2         = 0xC3,                             \
                             .rx_address_p3         = 0xC4,                             \
                             .rx_address_p4         = 0xC5,                             \
                             .rx_address_p5         = 0xC6,                             \
                             .rx_address_p6         = 0xC7,                             \
                             .rx_address_p7         = 0xC8,                             \
                             .dynamic_payload_length_enabled = 1,                       \
                             .dynamic_ack_enabled   = 0,                                \
                             .retransmit_delay      = 250,                              \
                             .retransmit_count      = 3,                                \
                             .tx_mode               = CAFE_TXMODE_AUTO,                 \
                             .radio_irq_priority    = 1}

enum cafe_event_type_t  {CAFE_EVENT_TX_SUCCESS, CAFE_EVENT_TX_FAILED, CAFE_EVENT_RX_RECEIVED};

typedef enum {CAFE_ADDRESS_PIPE0, CAFE_ADDRESS_PIPE1, CAFE_ADDRESS_PIPE2, CAFE_ADDRESS_PIPE3, CAFE_ADDRESS_PIPE4, CAFE_ADDRESS_PIPE5, CAFE_ADDRESS_PIPE6, CAFE_ADDRESS_PIPE7} cafe_address_type_t;

typedef struct
{
    enum cafe_event_type_t  type;
}cafe_event_t;

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
    cafe_payload_t *payload_ptr[CAFE_CORE_TX_FIFO_SIZE];
    uint32_t        entry_point;
    uint32_t        exit_point;
    uint32_t        count;
}cafe_payload_tx_fifo_t;

typedef struct
{
    cafe_payload_t *payload_ptr[CAFE_CORE_RX_FIFO_SIZE];
    uint32_t        entry_point;
    uint32_t        exit_point;
    uint32_t        count;
}cafe_payload_rx_fifo_t;

uint32_t cafe_init(cafe_config_t *parameters);

uint32_t cafe_disable(void);

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

uint32_t cafe_set_address(cafe_address_type_t address, const uint8_t *data_ptr);

uint32_t cafe_set_rf_channel(uint32_t channel);

uint32_t cafe_set_tx_power(uint8_t tx_output_power);


void update_nrf_radio_address(nrf_st_address radio_addr);
void on_radio_disabled_esb_dpl_tx_noack();
#endif
