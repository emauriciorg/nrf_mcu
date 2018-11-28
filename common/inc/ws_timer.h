#ifndef __TIMER_APP__H__
#define __TIMER_APP__H__

#include "nrf_drv_timer.h"
#include <stdint.h>
#define APP_TIMER_PRESCALER              0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          4                                          /**< Size of timer operation queues. */

void ws_timer1_setup(void);
void ws_timer_event_handler(nrf_timer_event_t event_type, void* p_context);
void ws_app_timer_init(void);
uint32_t ws_get_timer1_ticks(void);
#endif 	
