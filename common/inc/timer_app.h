#ifndef __TIMER_APP__H__
#define __TIMER_APP__H__

#include "nrf_drv_timer.h"

#define APP_TIMER_PRESCALER              0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          4                                          /**< Size of timer operation queues. */

void timer1_app_setup(uint16_t *external_counter_holder);
void timer_event_handler(nrf_timer_event_t event_type, void* p_context);
void timer_app_init(void);

#endif 
