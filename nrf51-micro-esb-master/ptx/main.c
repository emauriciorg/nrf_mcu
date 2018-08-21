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

//#define UESB_APP
#ifdef UESB_APP
	#include "uesbAPP.h"
	#include "micro_esb.h"
	static uesb_payload_t tx_payload;

#else	
	#include "tinyRF.h"
	tiny_payload_t  tx_payload;
#endif


int main(void)
{

	nrf_gpio_range_cfg_output(8, 15);

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
	#ifdef UESB_APP
		uesb_setup_tx(&tx_payload);
	#else
		tiny_init(&tx_payload);

	#endif
	uint32_t counter_ms=100;
	
	nrf_gpio_pin_set(8);
	nrf_gpio_pin_set(10);

	while (true)
	{ 
		if(counter_ms)counter_ms--;
		if(!counter_ms){
			counter_ms=100;
			nrf_gpio_pin_toggle(LED_GREEN);
		}
		#ifdef UESB_APP
			if (!uesb_write_tx_payload(&tx_payload))
			tx_payload.data[1]++;
		#else
			tiny_tx_transaction(&tx_payload);
		#endif
		

		nrf_delay_us(10000);

	}
}
