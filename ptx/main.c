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
#include "nrf_delay.h"
#include "nrf_gpio.h"


#include "uesbAPP.h"
#define LED_BLUE   10
#define LED_GREEN  9

static uesb_payload_t tx_payload;

int main(void)
{

	nrf_gpio_range_cfg_output(10, 15);

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	uesb_setup(&tx_payload);
	
	uint32_t counter_ms=100;
	
	while (true)
	{   

		if(counter_ms)counter_ms--;
		if(!counter_ms){
			counter_ms=100;
			nrf_gpio_pin_toggle(LED_BLUE);
		}
	
		if(!uesb_write_tx_payload(&tx_payload))
		{
			tx_payload.data[1]++;
		}
		nrf_delay_us(10000);
	}
}
