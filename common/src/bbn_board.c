#include "../inc/bbn_board.h"

#include "nrf_gpio.h"
void board_leds_init(void){
  	nrf_gpio_range_cfg_output(LED_RED, LED_BLUE);	
	nrf_gpio_pin_set(LED_BLUE);
	nrf_gpio_pin_set(LED_GREEN);
	nrf_gpio_pin_set(LED_RED);
}
