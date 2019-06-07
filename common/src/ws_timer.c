#include <stdio.h>
#include "app_timer.h"
#include "app_error.h"
#include "ws_timer.h"
#include "bbn_board.h"

#define TIMER_DBG(...)
//printf(__VA_ARGS__);

#define TIMER_INTERVAL_IN_MS 100u

//TO DO: define structures for timers
const nrf_drv_timer_t TIMER_1_APP = NRF_DRV_TIMER_INSTANCE(1);
static uint16_t i;
static uint32_t tick_counter = 0;


uint32_t ws_get_timer1_ticks(void){

	return tick_counter;
}

void ws_clear_timer1_ticks(uint32_t tick_trigger){
	tick_counter = tick_trigger;
}

void ws_app_timer_init(void){
	//#warning "APP_TIMER_ININ IS disabled"
	APP_TIMER_INIT(APP_TIMER_PRESCALER, 5, APP_TIMER_OP_QUEUE_SIZE, NULL);
}


void ws_timer_event_handler(nrf_timer_event_t event_type, void* p_context){

	ws_led_toggle(LED_BLUE);
	if (tick_counter)(tick_counter)--;
	if(!tick_counter){
		tick_counter=10;

	}
	if (i>500){
		i=0;
	}
	i++;
}



static void timer_a_handler(void * p_context){
	ws_led_toggle(LED_BLUE);
}
static void create_timers()
{
    uint32_t err_code;

    // Create timers
   //  APP_TIMER_DEF(m_led_a_timer_id);
   /* err_code = app_timer_create(&m_led_a_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                timer_a_handler);
    */
    #warning "timer create is not enabled"
    APP_ERROR_CHECK(err_code);
}

void ws_timer1_setup(void){



#ifdef OLD_TIMER
	err_code = nrf_drv_timer_init(  &TIMER_1_APP,NULL, ws_timer_event_handler);
	APP_ERROR_CHECK( err_code );
	time_ticks   = nrf_drv_timer_ms_to_ticks(&TIMER_1_APP, TIMER_INTERVAL_IN_MS);
	TIMER_DBG("TIMER : [%d ticks ], [%d ms]\n",time_ticks, TIMER_INTERVAL_IN_MS);
	//time_ticks=1;
	nrf_drv_timer_extended_compare(&TIMER_1_APP,
					NRF_TIMER_CC_CHANNEL0,
					time_ticks,
					NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
					true);
	nrf_drv_timer_enable(&TIMER_1_APP);
#endif

	create_timers();

}
