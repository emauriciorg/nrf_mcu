#ifndef _BBN_GPIO_H_
#define _BBN_GPIO_H_

#include <stdint.h>

#define UART_TX
#define UART_RX
#define BBN_BOARD_PINS
#ifdef BBN_BOARD_PINS
	#define MOTOR_PIN       0
	#define LED_RED		8
	#define LED_GREEN	9
	#define LED_BLUE	10
#endif

#ifdef SPARKFUN_BOARD
	#define LED_RED		8
	#define LED_GREEN	10
	#define LED_BLUE	7
#endif

#ifdef DEVKIT_BOARD
	#define LED_RED		17
	#define LED_GREEN	18
	#define LED_BLUE	19
#endif

void ws_leds_init(void);
void ws_clock_setup(void);
void ws_led_off(uint32_t led_id);
void ws_led_on(uint32_t led_id);
void ws_led_toggle(uint32_t led_id);
void ws_lfclk_setup_init(void);	

#endif
