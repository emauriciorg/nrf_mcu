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

#ifndef __TINYRX_ESB_H
#define __TINYRX_ESB_H

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"
#include "uesb_error_codes.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"


// Hard coded parameters - change if necessary
#define     tinyrx_CORE_MAX_PAYLOAD_LENGTH    32
#define     tinyrx_CORE_TX_FIFO_SIZE          8
#define     tinyrx_CORE_RX_FIFO_SIZE          8

#define     tinyrx_SYS_TIMER                  NRF_TIMER2
#define     tinyrx_SYS_TIMER_IRQ_Handler      TIMER2_IRQHandler

#define     tinyrx_PPI_TIMER_START            4
#define     tinyrx_PPI_TIMER_STOP             5
#define     tinyrx_PPI_RX_TIMEOUT             6
#define     tinyrx_PPI_TX_START               7

// Interrupt flags
#define     tinyrx_INT_TX_SUCCESS_MSK         0x01
#define     tinyrx_INT_TX_FAILED_MSK          0x02
#define     tinyrx_INT_RX_DR_MSK              0x04

#define     tinyrx_PID_RESET_VALUE            0xFF



typedef enum {
    tinyrx_MODE_PTX,          // Primary transmitter
    tinyrx_MODE_PRX           // Primary receiver
} tinyrx_mode_t;

typedef enum {
    tinyrx_BITRATE_2MBPS = RADIO_MODE_MODE_Nrf_2Mbit,
    tinyrx_BITRATE_1MBPS = RADIO_MODE_MODE_Nrf_1Mbit,
    tinyrx_BITRATE_250KBPS = RADIO_MODE_MODE_Nrf_250Kbit
} tinyrx_bitrate_t;

typedef enum {
    tinyrx_CRC_16BIT = RADIO_CRCCNF_LEN_Two,
    tinyrx_CRC_8BIT  = RADIO_CRCCNF_LEN_One,
    tinyrx_CRC_OFF   = RADIO_CRCCNF_LEN_Disabled
} tinyrx_crc_t;

typedef enum {
    tinyrx_TX_POWER_4DBM     = RADIO_TXPOWER_TXPOWER_Pos4dBm,
    tinyrx_TX_POWER_0DBM     = RADIO_TXPOWER_TXPOWER_0dBm,
    tinyrx_TX_POWER_NEG4DBM  = RADIO_TXPOWER_TXPOWER_Neg4dBm,
    tinyrx_TX_POWER_NEG8DBM  = RADIO_TXPOWER_TXPOWER_Neg8dBm,
    tinyrx_TX_POWER_NEG12DBM = RADIO_TXPOWER_TXPOWER_Neg12dBm,
    tinyrx_TX_POWER_NEG16DBM = RADIO_TXPOWER_TXPOWER_Neg16dBm,
    tinyrx_TX_POWER_NEG20DBM = RADIO_TXPOWER_TXPOWER_Neg20dBm,
    tinyrx_TX_POWER_NEG30DBM = RADIO_TXPOWER_TXPOWER_Neg30dBm
} tinyrx_tx_power_t;

typedef enum {
    tinyrx_TXMODE_AUTO,        // Automatic TX mode - When the TX fifo is non-empty and the radio is idle packets will be sent automatically.
    tinyrx_TXMODE_MANUAL,      // Manual TX mode - Packets will not be sent until tinyrx_start_tx() is called. Can be used to ensure consistent packet timing.
    tinyrx_TXMODE_MANUAL_START // Manual start TX mode - Packets will not be sent until tinyrx_start_tx() is called, but transmission will continue automatically until the TX FIFO is empty.
} tinyrx_tx_mode_t;

typedef void (*tinyrx_event_handler_t)(void);

// Main UESB configuration struct, contains all radio parameters
typedef struct
{
   
    tinyrx_mode_t             mode;
    tinyrx_event_handler_t    event_handler;

    // General RF parameters
    tinyrx_bitrate_t          bitrate;
    tinyrx_crc_t              crc;
    uint8_t                 rf_channel;
    uint8_t                 payload_length;
    uint8_t                 rf_addr_length;

    tinyrx_tx_power_t         tx_output_power;
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

    // ESB specific features

    // Control settings
    tinyrx_tx_mode_t          tx_mode;

    uint8_t                 radio_irq_priority;
}tinyrx_config_t;

// Default radio parameters, roughly equal to nRF24L default parameters (except CRC which is set to 16-bit, and protocol set to DPL)
#define tinyrx_DEFAULT_CONFIG {.mode                  = tinyrx_MODE_PTX,                    \
                             .event_handler         = 0,                                \
                             .rf_channel            = 2,                                \
                             .payload_length        = tinyrx_CORE_MAX_PAYLOAD_LENGTH,     \
                             .rf_addr_length        = 5,                                \
                             .bitrate               = tinyrx_BITRATE_2MBPS,               \
                             .crc                   = tinyrx_CRC_16BIT,                   \
                             .tx_output_power       = tinyrx_TX_POWER_0DBM,               \
                             .rx_address_p0         = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7},   \
                             .rx_address_p1         = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2},   \
                             .rx_address_p2         = 0xC3,                             \
                             .rx_address_p3         = 0xC4,                             \
                             .rx_address_p4         = 0xC5,                             \
                             .rx_address_p5         = 0xC6,                             \
                             .rx_address_p6         = 0xC7,                             \
                             .rx_address_p7         = 0xC8,                             \
                             .rx_pipes_enabled      = 0x3F,                             \
                             .tx_mode               = tinyrx_TXMODE_AUTO,                 \
                             .radio_irq_priority    = 1}


typedef enum {tinyrx_ADDRESS_PIPE0, tinyrx_ADDRESS_PIPE1, tinyrx_ADDRESS_PIPE2, tinyrx_ADDRESS_PIPE3, tinyrx_ADDRESS_PIPE4, tinyrx_ADDRESS_PIPE5, tinyrx_ADDRESS_PIPE6, tinyrx_ADDRESS_PIPE7} tinyrx_address_type_t;


typedef struct
{
    uint8_t length;
    uint8_t pipe;
    int8_t  rssi;
    uint8_t noack;
    uint8_t data[tinyrx_CORE_MAX_PAYLOAD_LENGTH];
}tinyrx_payload_t;

typedef struct
{
    tinyrx_payload_t *payload_ptr[tinyrx_CORE_TX_FIFO_SIZE];
    uint32_t        entry_point;
    uint32_t        exit_point;
    uint32_t        count;
}tinyrx_payload_tx_fifo_t;

typedef struct
{
    tinyrx_payload_t *payload_ptr[tinyrx_CORE_RX_FIFO_SIZE];
    uint32_t        entry_point;
    uint32_t        exit_point;
    uint32_t        count;
}tinyrx_payload_rx_fifo_t;

uint32_t tinyrx_init(tinyrx_config_t *parameters);


bool     tinyrx_is_idle(void);

uint32_t tinyrx_write_tx_payload(tinyrx_payload_t *payload);

uint32_t tinyrx_write_ack_payload(tinyrx_payload_t *payload);

uint32_t tinyrx_read_rx_payload(tinyrx_payload_t *payload);

uint32_t tinyrx_start_tx(void);

uint32_t tinyrx_start_rx(void);

uint32_t tinyrx_stop_rx(void);

uint32_t tinyrx_get_tx_attempts(uint32_t *attempts);

uint32_t tinyrx_flush_tx(void);

uint32_t tinyrx_flush_rx(void);

uint32_t tinyrx_get_clear_interrupts(uint32_t *interrupts);

uint32_t tinyrx_set_address(tinyrx_address_type_t address, const uint8_t *data_ptr);



int8_t get_rssi(void);
#endif
