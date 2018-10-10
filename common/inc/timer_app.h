#ifndef __TIMER_APP__H__
#define __TIMER_APP__H__
#include "nrf_drv_timer.h"

void timer1_app_setup(void);
void timer_event_handler(nrf_timer_event_t event_type, void* p_context);


#endif 
