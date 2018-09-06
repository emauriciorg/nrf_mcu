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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf.h"

#include "uesb_error_codes.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "bbn_board.h"
#include "simple_uart.h"
#include "tinyRF.h"
static uesb_payload_t tx_payload;

uint16_t counter_ms=0;

void uesb_event_handler()
{
    static uint32_t rf_interrupts;

    
    uesb_get_clear_interrupts(&rf_interrupts);
    
    if(rf_interrupts & UESB_INT_TX_SUCCESS_MSK)
    {   
    }
    
    if(rf_interrupts & UESB_INT_TX_FAILED_MSK)
    {
        uesb_flush_tx();
    }
    
    if(rf_interrupts & UESB_INT_RX_DR_MSK)
    {
    
    }
    
    
}


void blink_led(uint16_t *count_ms);

int main(void)
{
    uint8_t rx_addr_p0[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    uint8_t rx_addr_p1[] = {0xBC, 0xDE, 0xF0, 0x12, 0x23};
    uint8_t rx_addr_p2   = 0x66;
    
    nrf_gpio_range_cfg_output(8, 15);
    
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
		
    while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

		simple_uart_config(  RTS_PIN,
                         RX_PIN, 
                         CTS_PIN,
                         TX_PIN, 
                        false);
		
    uesb_config_t uesb_config       = UESB_DEFAULT_CONFIG;
    uesb_config.rf_channel          = 5;
    uesb_config.crc                 = UESB_CRC_16BIT;
    uesb_config.retransmit_count    = 6;
    uesb_config.retransmit_delay    = 500;
    uesb_config.dynamic_ack_enabled = 0;
    uesb_config.bitrate             = UESB_BITRATE_2MBPS;
    uesb_config.event_handler       = uesb_event_handler;
    
    uesb_init(&uesb_config);

    uesb_set_address(UESB_ADDRESS_PIPE0, rx_addr_p0);
    uesb_set_address(UESB_ADDRESS_PIPE1, rx_addr_p1);
    uesb_set_address(UESB_ADDRESS_PIPE2, &rx_addr_p2);

    tx_payload.length  = 8;
    tx_payload.pipe    = 0;
    tx_payload.data[0] = 'A';//0x01;
    tx_payload.data[1] = 0x00;
    tx_payload.data[2] = 0x00;
    tx_payload.data[3] = 0x00;
    tx_payload.data[4] = 0x11;
    memcpy((char *)tx_payload.data,"Pigeon\0\0",8);
		
		uint8_t cli_buffer[20];
		
	nrf_gpio_pin_set(8);
	nrf_gpio_pin_set(10);
	simple_uart_putstring("nrf init\n");
    uint8_t package_id=0;
    while (true)
    {   
    	blink_led(&counter_ms);
			

        if(uesb_write_tx_payload(&tx_payload) == UESB_SUCCESS)
        {
         //   tx_payload.data[1]++;
					memcpy((char *)cli_buffer,"Pigeon  \n\0\0",strlen("Pigeon  \n\0\0"));
                    cli_buffer[7]=package_id+'0';
                    if ((package_id++)>8)package_id=0;
					simple_uart_putstring(cli_buffer);
        }
        nrf_delay_ms(1000);
			
    }
}

void blink_led(uint16_t *count_ms){
    
            nrf_gpio_pin_toggle(LED_GREEN);
return;
    if(*count_ms)(*count_ms)--;
       
       if(!(*count_ms)){
            *count_ms=100;
            nrf_gpio_pin_toggle(LED_GREEN);
    }

}
