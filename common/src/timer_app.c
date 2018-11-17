#include "app_timer.h"
#include "timer_app.h"

#include <stdio.h>
#include "app_error.h"


#define TIMER_INTERVAL 100

const nrf_drv_timer_t TIMER_1_APP = NRF_DRV_TIMER_INSTANCE(1);
 static uint16_t i;
 static uint16_t *tick_counter;


void timer_app_init(void){
	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
}



void timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{   
	if (*tick_counter)(*tick_counter)--;	

	if (i>500){
		//printf("%x\n", i);
		i=0;
//		sample_text=1;	
	}
	
	i++;
}

void timer1_app_setup(uint16_t *external_counter_holder){

	uint32_t time_ms = TIMER_INTERVAL;
	uint32_t time_ticks;
	uint32_t err_code;

	tick_counter= external_counter_holder;

	err_code = nrf_drv_timer_init(  &TIMER_1_APP,NULL, timer_event_handler);
	
	APP_ERROR_CHECK( err_code );

	
	time_ticks = nrf_drv_timer_ms_to_ticks(&TIMER_1_APP, time_ms);
	
	nrf_drv_timer_extended_compare(&TIMER_1_APP,  NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true); 
	nrf_drv_timer_enable(&TIMER_1_APP);
}
