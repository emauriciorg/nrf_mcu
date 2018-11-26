#ifndef __TIMER_APP__H__
#define __TIMER_APP__H__

#include "nrf_drv_timer.h"

#define APP_TIMER_PRESCALER              0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          4                                          /**< Size of timer operation queues. */

void ws_timer1_setup(uint16_t *external_counter_holder);
void ws_timer_event_handler(nrf_timer_event_t event_type, void* p_context);
void ws_timer_init(void);

#endif 
