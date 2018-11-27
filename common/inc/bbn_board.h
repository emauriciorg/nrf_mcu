#ifndef _BBN_GPIO_H_
#define _BBN_GPIO_H_

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

#define UART_TX
#define UART_RX

void board_leds_init(void);

#endif
