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
#include "TX_CAFE.h"
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
//        uesb_flush_tx();
    }
    
    if(rf_interrupts & UESB_INT_RX_DR_MSK)
    {
    
    }
    
    
}


void blink_led(uint16_t *count_ms);

extern uint8_t   sw_radio_flag;
extern unsigned char ready_to_send;

int main(void)
{


    nrf_st_address user_radio_addr;
    const char pipe_addr[8]         = {0x60, 0xF4, 0xAA, 0x12, 0x0E,0x0F,0x10,0x11};
    const char base_addr0[6]        ={ 0x34, 0x56, 0x78, 0x23};
    const char base_addr1[6]        ={ 0x34, 0x56, 0x78, 0x9A};

    memcpy (user_radio_addr.logic_pipe , pipe_addr  , 8);
    memcpy( user_radio_addr.base_addr0 , base_addr0 , 5);
    memcpy( user_radio_addr.base_addr1 , base_addr1 , 5);
    
    nrf_gpio_range_cfg_output(8, 15);
    nrf_gpio_pin_set(LED_RED);
    nrf_gpio_pin_set(LED_GREEN);
    
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;
		
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
    update_nrf_radio_address(user_radio_addr);

    tx_payload.length  = 8;
    tx_payload.pipe    = 0;
    tx_payload.data[0] = 'A';//0x01;
    tx_payload.data[1] = 0x00;
    tx_payload.data[2] = 0x00;
    tx_payload.data[3] = 0x00;
    tx_payload.data[4] = 0x11;
    
    memcpy((char *)tx_payload.data,"Pigeon  \n\0",10);
		
	
    simple_uart_putstring("nrf init\n");
    
    uint8_t package_id=0;
    ready_to_send=1;
    //uesb_write_tx_payload(&tx_payload);
    while (true)
    {   
    	blink_led(&counter_ms);
			if(ready_to_send){
	          start_tx_transaction();
              // uesb_write_tx_payload(&tx_payload);
            ready_to_send=0;
            }
           //     check_radio_flags(); //sending by polling            
        nrf_delay_ms(100);
			
    }
}

void blink_led(uint16_t *count_ms){
    
            nrf_gpio_pin_toggle(LED_BLUE);
return;
/*   
	if(*count_ms)(*count_ms)--;
       
       if(!(*count_ms)){
            *count_ms=100;
            nrf_gpio_pin_toggle(LED_BLUE);
    }
*/
}
