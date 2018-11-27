/** 
******************************************************************************
* \file    ws_aes.h
* \brief   ws_aes.h Entry point file.  AES registers on the 51822 cant be access while softdevice is running
******************************************************************************
*/

#ifndef aes_app_H
#define aes_app_H

#include <stdint.h>

#define AES_SAMPLE_TEXT  "The quick brown fox jumps over the lazy dog"

uint8_t aes_decrypt_data(uint8_t *src_data, uint8_t  length, uint8_t *out_data);

uint8_t aes_encrypt_data(uint8_t *input_data, uint8_t packet_length, uint8_t *out_data);

#endif
