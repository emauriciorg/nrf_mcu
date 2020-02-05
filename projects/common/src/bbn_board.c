#include "bbn_board.h"
#include "nrf_gpio.h"

void ws_led_toggle(uint32_t led_id){
	nrf_gpio_pin_toggle(led_id);
}

void ws_led_off(uint32_t led_id){
	nrf_gpio_pin_set(led_id);
}
void ws_led_on(uint32_t led_id){
	nrf_gpio_pin_clear(led_id);
}

void ws_leds_init(void){
	nrf_gpio_cfg_output(LED_BLUE);
	nrf_gpio_cfg_output(LED_GREEN);
	nrf_gpio_cfg_output(LED_RED);
	nrf_gpio_pin_set(LED_BLUE);
	nrf_gpio_pin_set(LED_GREEN);
	nrf_gpio_pin_set(LED_RED);
}


void ws_hfclk_setup_init(void);
void ws_clock_setup(void){

#ifdef OLD_CLK
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
#else
	ws_hfclk_setup_init();
//	ws_lfclk_setup_init();
#endif
}

#ifdef LFCLK_CALIBRATION_ENABLE
static void lfclk_calibration_handle(void)
{
	uint8_t softdevice_enabled = 0;
	
	sd_softdevice_is_enabled(&softdevice_enabled); 

	/* Softdevice handles calibration.*/
	if (softdevice_enabled) return ;
	
	/* Handle calibration of LFCLK when in gzll mode*/
	static uint32_t lfclk_cal_timer = 0;
	
	if (lfclk_cal_timer >= LFCLK_CAL_INACTIVITY_TIMEOUT){
		lfclk_calibration_start();
	}
	
    	
}
static void lfclk_calibration_start(void){
	bool hfclk_started = false;
	NRF_CLOCK->INTENCLR             = CLOCK_INTENSET_DONE_Msk | CLOCK_INTENSET_CTTO_Msk;
	NRF_CLOCK->EVENTS_DONE          = 0;
	NRF_CLOCK->EVENTS_CTTO          = 0;        
	if ((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Msk) !=
	  (CLOCK_HFCLKSTAT_SRC_Xtal << CLOCK_HFCLKSTAT_SRC_Pos)){    
	    hfclk_crystal_oscillator_start(true);
	    hfclk_started = true;
	}

	/* Initial calibration for the LFCLK RCOSC */
	NRF_CLOCK->TASKS_CAL = 1;
	while (NRF_CLOCK->EVENTS_DONE == 0) ;
	NRF_CLOCK->EVENTS_DONE = 0;    
	if (hfclk_started){
	    hfclk_crystal_oscillator_stop();
	}    
	/* Start the calibration timer using specified interval */
	NRF_CLOCK->INTENSET = CLOCK_INTENSET_DONE_Msk | CLOCK_INTENSET_CTTO_Msk;
	NRF_CLOCK->CTIV = m_clk_mgmt_lfclk_cal_interval;
	nrf_delay_us(32); /* XLR PAN workaround */
	NRF_CLOCK->TASKS_CTSTOP = 1;
	nrf_delay_us(32); /* XLR PAN workaround */
	NRF_CLOCK->TASKS_CTSTART = 1;
	NVIC_SetPriority(POWER_CLOCK_IRQn, 3);
	NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);
	NVIC_EnableIRQ(POWER_CLOCK_IRQn);    
}
#endif

//HCLK provides PCLK1M: and PCLK16M (ONLY on mode)
void ws_lfclk_setup_init(void){
	
	if ((NRF_CLOCK->LFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) !=
	(CLOCK_HFCLKSTAT_STATE_Running << CLOCK_HFCLKSTAT_STATE_Pos)){
   
		NRF_CLOCK->LFCLKSRC = ((CLOCK_LFCLKSRC_SRC_RC << CLOCK_LFCLKSRC_SRC_Pos) & CLOCK_LFCLKSRC_SRC_Msk);    
		NRF_CLOCK->TASKS_LFCLKSTART = true;
		while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);
		NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
   
	}

#ifdef LFCLK_CALIBRATION_ENABLE
	if (calibrate){
		lfclk_calibration_start();
	}
#endif
}


void ws_hfclk_setup_init(void){
	
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
	return ;

}


