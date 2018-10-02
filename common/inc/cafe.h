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

#ifndef __MICRO_ESB_H
#define __MICRO_ESB_H

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"

#include "cafe_data_types.h"


void get_rx_payload(uint8_t *out_buffer);

uint32_t uesb_disable(void);

bool     uesb_is_idle(void);

uint32_t uesb_write_tx_payload(cafe_payload_t *payload);




uint32_t uesb_start_tx(void);


uint32_t uesb_get_tx_attempts(uint32_t *attempts);


uint32_t uesb_get_clear_interrupts(uint32_t *interrupts);


uint32_t uesb_set_rf_channel(uint32_t channel);

uint32_t uesb_set_tx_power(uint8_t tx_output_power);


void update_nrf_radio_address(nrf_st_address radio_addr);
void self_cafe_configuration(uint8_t transeciever_mode);

 void start_tx_transaction(void);

#endif
