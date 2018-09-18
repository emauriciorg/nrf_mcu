#include <stdint.h>
#include <stdbool.h>
#include "nrf_soc.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "app_util.h"
#include "softdevice_handler.h"
#include "boards.h"
#include "bbn_board.h"
#include "TX_CAFE.h"
#include "uart_app.h"
#include <stdio.h>
#define APP_ROUTINE_IRQ  

#define TIMESLOT_BEGIN_IRQn         LPCOMP_IRQn             /**< Re-used LPCOMP interrupt for processing the beginning of timeslot. */
#define TIMESLOT_BEGIN_IRQHandler   LPCOMP_IRQHandler       /**< The IRQ handler of LPCOMP interrupt */
#define TIMESLOT_BEGIN_IRQPriority  1                       /**< Interrupt priority of @ref TIMESLOT_BEGIN_IRQn. */

#define TIMESLOT_END_IRQn           QDEC_IRQn               /**< Re-used QDEC interrupt for processing the end of timeslot. */
#define TIMESLOT_END_IRQHandler     QDEC_IRQHandler         /**< The IRQ handler of QDEC interrupt */
#define TIMESLOT_END_IRQPriority    1                       /**< Interrupt priority of @ref TIMESLOT_END_IRQn. */

#define UESB_RX_HANDLE_IRQn         WDT_IRQn                /**< Re-used WDT interrupt for processing the RX data from UESB. */
#define UESB_RX_HANDLE_IRQHandler   WDT_IRQHandler          /**< The IRQ handler of WDT interrupt */
#define UESB_RX_HANDLE_IRQPriority  3                       /**< Interrupt priority of @ref UESB_RX_HANDLE_IRQn. */

uint8_t radio_counter=0;
uint8_t temp_buff[32];
uint8_t radio_len=0;
uint8_t counter_sec=0;
void TIMESLOT_END_IRQHandler(void)
{



}

#define TIME_TO_BLINK 10
uint32_t blink_counter=TIME_TO_BLINK;
extern uint8_t radion_sent;

void TIMESLOT_BEGIN_IRQHandler(void)
{

	

	nrf_gpio_pin_toggle(LED_BLUE); //Toggle LED4
	self_TX_CAFE_configuration();	
		if( radion_sent){
			start_tx_transaction();
			radion_sent=0;
 			counter_sec++;
		}
}

/**Constants for timeslot API
*/
static nrf_radio_request_t  m_timeslot_request;
static uint32_t             m_slot_length;

static nrf_radio_signal_callback_return_param_t signal_callback_return_param;

/**@brief Request next timeslot event in earliest configuration
*/
uint32_t request_next_event_earliest(void)
{
	m_slot_length                                  = 15000;
	m_timeslot_request.request_type                = NRF_RADIO_REQ_TYPE_EARLIEST;
	m_timeslot_request.params.earliest.hfclk       = NRF_RADIO_HFCLK_CFG_DEFAULT;
	m_timeslot_request.params.earliest.priority    = NRF_RADIO_PRIORITY_NORMAL;
	m_timeslot_request.params.earliest.length_us   = m_slot_length;
	m_timeslot_request.params.earliest.timeout_us  = 1000000;
	return sd_radio_request(&m_timeslot_request);
}


/**@brief Configure next timeslot event in earliest configuration
*/
void configure_next_event_earliest(void)
{
	m_slot_length                                  = 15000;
	m_timeslot_request.request_type                = NRF_RADIO_REQ_TYPE_EARLIEST;  
	m_timeslot_request.params.earliest.hfclk       = NRF_RADIO_HFCLK_CFG_DEFAULT;
	m_timeslot_request.params.earliest.priority    = NRF_RADIO_PRIORITY_NORMAL;
	m_timeslot_request.params.earliest.length_us   = m_slot_length;
	m_timeslot_request.params.earliest.timeout_us  = 1000000;
}


/**@brief Configure next timeslot event in normal configuration
*/
void configure_next_event_normal(void)
{
	m_slot_length                                 = 15000;
	m_timeslot_request.request_type               = NRF_RADIO_REQ_TYPE_NORMAL;
	m_timeslot_request.params.normal.hfclk        = NRF_RADIO_HFCLK_CFG_DEFAULT;
	m_timeslot_request.params.normal.priority     = NRF_RADIO_PRIORITY_HIGH;

	m_timeslot_request.params.normal.distance_us  = 100000;
	m_timeslot_request.params.normal.length_us    = m_slot_length;
}


/**@brief Timeslot signal handler
*/
void nrf_evt_signal_handler(uint32_t evt_id)
{
	uint32_t err_code;

	switch (evt_id)
	{
		case NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN:
//No implementation needed
		break;
		case NRF_EVT_RADIO_SESSION_IDLE:
//No implementation needed
		break;
		case NRF_EVT_RADIO_SESSION_CLOSED:
//No implementation needed, session ended
		break;
		case NRF_EVT_RADIO_BLOCKED:
//Fall through
		case NRF_EVT_RADIO_CANCELED:
		err_code = request_next_event_earliest();
		APP_ERROR_CHECK(err_code);
		break;
		default:
		break;
	}
}

void trigger_time_slot_timer(){
#if 0
	NRF_TIMER0->TASKS_STOP          = 1;
	NRF_TIMER0->TASKS_CLEAR         = 1;
	NRF_TIMER0->MODE                = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
	NRF_TIMER0->EVENTS_COMPARE[0]   = 0;
	NRF_TIMER0->EVENTS_COMPARE[1]   = 0;
	NRF_TIMER0->INTENSET            = TIMER_INTENSET_COMPARE0_Msk | TIMER_INTENSET_COMPARE1_Msk ;
	NRF_TIMER0->CC[0]               = TS_LEN_US - TS_SAFETY_MARGIN_US;
	NRF_TIMER0->CC[1]               = (TS_LEN_US - TS_EXTEND_MARGIN_US);
	NRF_TIMER0->BITMODE             = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
	NRF_TIMER0->TASKS_START         = 1;
	NRF_RADIO->POWER                = (RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos);
#else

	NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;
	NRF_TIMER0->CC[0] = m_slot_length - 1000;

#endif
}

/**@brief Timeslot event handler
*/

nrf_radio_signal_callback_return_param_t * radio_callback(uint8_t signal_type)
{
	switch(signal_type)
	{
	case NRF_RADIO_CALLBACK_SIGNAL_TYPE_START:
//Start of the timeslot - set up timer interrupt
		signal_callback_return_param.params.request.p_next = NULL;
		signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;

		//trigger_time_slot_timer();
		NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;
		NRF_TIMER0->CC[0] = m_slot_length - 1000;
		NVIC_EnableIRQ(TIMER0_IRQn);   
		//nrf_gpio_pin_toggle(LED_GREEN); //Toggle LED4
  NRF_RADIO->POWER                = (RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos);
		NVIC_SetPendingIRQ(TIMESLOT_BEGIN_IRQn);
	break;

	case NRF_RADIO_CALLBACK_SIGNAL_TYPE_RADIO:
		//radio_len=sprintf((char *)temp_buff,"Radio [%d]",radio_counter++);
		//msg_dbg((char *)temp_buff,radio_len);
	       
		signal_callback_return_param.params.request.p_next = NULL;
		signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
		 RADIO_IRQHandler();
	break;

	case NRF_RADIO_CALLBACK_SIGNAL_TYPE_TIMER0:
//Timer interrupt - do graceful shutdown - schedule next timeslot
		configure_next_event_normal();
		signal_callback_return_param.params.request.p_next = &m_timeslot_request;
		signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END;
	break;
	case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_SUCCEEDED:
//No implementation needed
	break;
	case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_FAILED:
//Try scheduling a new timeslot
		configure_next_event_earliest();
		signal_callback_return_param.params.request.p_next = &m_timeslot_request;
		signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END;
	break;
	default:
//No implementation needed
	break;
	}
	return (&signal_callback_return_param);
}


/**@brief Function for initializing the timeslot API.
*/
uint32_t timeslot_sd_init(void)
{
	uint32_t err_code;

	err_code = sd_radio_session_open(radio_callback);
	if (err_code != NRF_SUCCESS)
	{
		return err_code;
	}

	err_code = request_next_event_earliest();
	if (err_code != NRF_SUCCESS)
	{
		(void)sd_radio_session_close();
		return err_code;
	}
	NVIC_ClearPendingIRQ(TIMESLOT_BEGIN_IRQn);
	NVIC_SetPriority(TIMESLOT_BEGIN_IRQn, 1);
    	NVIC_EnableIRQ(TIMESLOT_BEGIN_IRQn);
	return NRF_SUCCESS;
}
