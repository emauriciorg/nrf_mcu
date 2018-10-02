/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
*
* The information contained herein is property of Nordic Semiconductor ASA.
* Terms and conditions of usage are described in detail in NORDIC
* SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
*
* Licensees are granted free, non-transferable use of the information. NO
* WARRANTY of ANY KIND is provided. This heading must NOT be removed from
* the file.
*
*/

#include "../inc/RX_CAFE.h"
#include "nrf_gpio.h"
#include <string.h>

unsigned int led_state=0;


cafe_payload_t  rx_payload;

static cafe_event_handler_t     m_event_handler;

// RF parameters
static cafe_config_t            m_config_local;

// RX FIFO
static  uint8_t                 m_rx_payload_buffer[CAFE_CORE_MAX_PAYLOAD_LENGTH + 2];

// Run time variables
static volatile uint32_t        m_interrupt_flags       = 0;



// Constant parameters
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS   48   // Smallest reliable value - 43
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_1MBPS   64   // Smallest reliable value - 59
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_250KBPS 250

// Macros
#define                         DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define                         ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)

#define                         RADIO_SHORTS_COMMON ( RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk | \
RADIO_SHORTS_ADDRESS_RSSISTART_Msk | RADIO_SHORTS_DISABLED_RSSISTOP_Msk )

int8_t g_rssi  = 0; //PUT INTO AN STRUCTURE


int8_t get_rssi(void){
	return g_rssi;
}




//static cafe_load_radio_addr(nrf_st_address user_radio_addr;){
//	memcpy ( m_config_local.radio_addresses,user_radio_addr,sizeof(nrf_st_address));
//}


// The following functions are assigned to the function pointers above
static void on_radio_disabled(void);


static void update_rf_payload_format(uint32_t payload_length)
{
//#if (CAFE_CORE_MAX_PAYLOAD_LENGTH <= 32)
	NRF_RADIO->PCNF0 = (0 << RADIO_PCNF0_S0LEN_Pos) | (6 << RADIO_PCNF0_LFLEN_Pos) | (3 << RADIO_PCNF0_S1LEN_Pos);
	NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled        << RADIO_PCNF1_WHITEEN_Pos) |
	(RADIO_PCNF1_ENDIAN_Big              << RADIO_PCNF1_ENDIAN_Pos)  |
	((m_config_local.rf_addr_length - 1) << RADIO_PCNF1_BALEN_Pos)   |
	(0                                   << RADIO_PCNF1_STATLEN_Pos) |
	(CAFE_CORE_MAX_PAYLOAD_LENGTH      << RADIO_PCNF1_MAXLEN_Pos);
}

// Function that swaps the bits within each byte in a uint32. Used to convert from nRF24L type addressing to nRF51 type addressing
static uint32_t bytewise_bit_swap(uint32_t inp)
{
	inp = (inp & 0xF0F0F0F0)  >> 4 | (inp & 0x0F0F0F0F) << 4;
	inp = (inp & 0xCCCCCCCC)  >> 2 | (inp & 0x33333333) << 2;
	return (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
}


static void update_radio_parameters()
{
	

	NRF_RADIO->TXPOWER       = m_config_local.tx_output_power   << RADIO_TXPOWER_TXPOWER_Pos;

	NRF_RADIO->MODE          = m_config_local.bitrate           << RADIO_MODE_MODE_Pos;

	NRF_RADIO->CRCCNF        = m_config_local.crc               << RADIO_CRCCNF_LEN_Pos;
	NRF_RADIO->CRCINIT       = 0xFFFFUL;      // Initial value
	NRF_RADIO->CRCPOLY       = 0x11021UL;     // CRC poly: x^16+x^12^x^5+1

// Packet format
	update_rf_payload_format(m_config_local.payload_length);
// Radio address config
//	update_nrf_radio_address (m_config_local.radio_addresses);
}

void update_nrf_radio_address(nrf_st_address radio_addr){
	
	NRF_RADIO->PREFIX0 = bytewise_bit_swap(   radio_addr.logic_pipe[3]  <<  24  |   radio_addr.logic_pipe[2]  <<  16 |   radio_addr.logic_pipe[1]  << 8   |  radio_addr.logic_pipe[0]);
	NRF_RADIO->PREFIX1 = bytewise_bit_swap(   radio_addr.logic_pipe[7]  <<  24  |   radio_addr.logic_pipe[6]  <<  16 |   radio_addr.logic_pipe[5]  << 8   |  radio_addr.logic_pipe[4]);
	NRF_RADIO->BASE0   = bytewise_bit_swap(   radio_addr.base_addr0[0] << 24 |  radio_addr.base_addr0[1] << 16  |  radio_addr.base_addr0[2] << 8 | radio_addr.base_addr0[3]);
	NRF_RADIO->BASE1   = bytewise_bit_swap(   radio_addr.base_addr1[0] << 24 |  radio_addr.base_addr1[1] << 16  |  radio_addr.base_addr1[2] << 8 | radio_addr.base_addr1[3]);
}


/*CUSTOM PROTOCOL HANDLING*/
static bool rx_fifo_push_rfbuf(uint8_t pipe)
{
	g_rssi  =  NRF_RADIO->RSSISAMPLE;
	return true;
}


void get_rx_payload(uint8_t *out_buffer){
	memcpy(  out_buffer,   m_rx_payload_buffer,  CAFE_CORE_MAX_PAYLOAD_LENGTH);
}

static void ppi_init()
{
	NRF_PPI->CH[cafe_PPI_TIMER_START].EEP = (uint32_t) &NRF_RADIO->EVENTS_READY;
	NRF_PPI->CH[cafe_PPI_TIMER_START].TEP = (uint32_t) &cafe_SYS_TIMER->TASKS_START;
	NRF_PPI->CH[cafe_PPI_TIMER_STOP].EEP  = (uint32_t) &NRF_RADIO->EVENTS_ADDRESS;
	NRF_PPI->CH[cafe_PPI_TIMER_STOP].TEP  = (uint32_t) &cafe_SYS_TIMER->TASKS_STOP;
	NRF_PPI->CH[cafe_PPI_RX_TIMEOUT].EEP  = (uint32_t) &cafe_SYS_TIMER->EVENTS_COMPARE[0];
	NRF_PPI->CH[cafe_PPI_RX_TIMEOUT].TEP  = (uint32_t) &NRF_RADIO->TASKS_DISABLE;
	//NRF_PPI->CH[cafe_PPI_TX_START].EEP    = (uint32_t) &cafe_SYS_TIMER->EVENTS_COMPARE[1];
	//NRF_PPI->CH[cafe_PPI_TX_START].TEP    = (uint32_t) &NRF_RADIO->TASKS_TXEN;
}

uint32_t cafe_init(cafe_config_t *parameters)
{
	m_event_handler      = parameters->event_handler;
	m_interrupt_flags    = 0;

	memcpy(&m_config_local, parameters, sizeof(cafe_config_t));
	
	update_radio_parameters();

	ppi_init();

	NVIC_SetPriority( RADIO_IRQn,  m_config_local.radio_irq_priority & 0x03);

	return true;
}
#if 0
uint32_t cafe_disable(void)
{
	NRF_PPI->CHENCLR = (1 << cafe_PPI_TIMER_START) | (1 << cafe_PPI_TIMER_STOP) | (1 << cafe_PPI_RX_TIMEOUT) | (1 << cafe_PPI_TX_START);
	return true;
}
#endif


//CUSTOM PROTOCOL ASSIGNATION : cpy recieved data to user structure
	
uint32_t cafe_read_rx_payload(cafe_payload_t *payload)
{
	DISABLE_RF_IRQ;
	ENABLE_RF_IRQ;
	return true;
}

uint32_t cafe_start_rx(void)
{
	NRF_RADIO->INTENCLR        = 0xFFFFFFFF;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->SHORTS          = RADIO_SHORTS_COMMON | RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->INTENSET        = RADIO_INTENSET_DISABLED_Msk;

	NRF_RADIO->RXADDRESSES     =  0X02; //<--set to dynamic or protocol based address
	NRF_RADIO->FREQUENCY       =  m_config_local.rf_channel;
	NRF_RADIO->PACKETPTR       =  (uint32_t) m_rx_payload_buffer;

	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS  = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_RXEN      = 1;
	return true;
}


uint32_t cafe_get_clear_interrupts(uint32_t *interrupts)
{
	DISABLE_RF_IRQ;
	*interrupts         = m_interrupt_flags;
	m_interrupt_flags   = 0;
	ENABLE_RF_IRQ;
	return true;
}


uint32_t cafe_set_rf_channel(uint32_t channel)
{
	if (channel > 125) return false;
	m_config_local.rf_channel = channel;
	return true;
}



void RADIO_IRQHandler()
{
	if (NRF_RADIO->EVENTS_READY && (NRF_RADIO->INTENSET & RADIO_INTENSET_READY_Msk)){
		NRF_RADIO->EVENTS_READY = 0;
	}

	if (NRF_RADIO->EVENTS_END && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)){
		NRF_RADIO->EVENTS_END = 0;
	}

	if (NRF_RADIO->EVENTS_DISABLED && (NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk)){
		NRF_RADIO->EVENTS_DISABLED = 0;
		//ADD OPTION FOR BOTH TX AND RX
		on_radio_disabled();
	}
}

uint8_t recieved_counter=0;
static void on_radio_disabled(void)
{
//	if (!NRF_RADIO->CRCSTATUS) return;
	NRF_RADIO->SHORTS                = RADIO_SHORTS_COMMON;
	
	update_rf_payload_format(m_config_local.payload_length);
	
	NRF_RADIO->PACKETPTR             = (uint32_t) m_rx_payload_buffer;
	NRF_RADIO->EVENTS_DISABLED       = 0;
	NRF_RADIO->TASKS_DISABLE         = 1;

	while(NRF_RADIO->EVENTS_DISABLED == 0);
	
	NRF_RADIO->EVENTS_DISABLED       = 0;
	NRF_RADIO->SHORTS                = RADIO_SHORTS_COMMON | RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->TASKS_RXEN            = 1;
	
	rx_fifo_push_rfbuf( NRF_RADIO->RXMATCH );
	m_interrupt_flags   |= cafe_INT_RX_DR_MSK;
	if (m_event_handler != 0) m_event_handler();
	 
}


void cafe_event_handler_rx(void)
{
	static uint32_t rf_interrupts;
	
	cafe_get_clear_interrupts(&rf_interrupts);
	
#if 0
	if (rf_interrupts & cafe_INT_TX_SUCCESS_MSK)
	{   
	}
	
	if (rf_interrupts & cafe_INT_TX_FAILED_MSK)
	{
	}
	#endif
	if (rf_interrupts & cafe_INT_RX_DR_MSK)
	{
		cafe_read_rx_payload(&rx_payload);
		led_state=1;
	}
	
}

void cafe_setup_rx(void){
   	

 /*USER DEFINED ADDR*/

    nrf_st_address user_radio_addr;
    const char pipe_addr[8]         = {0x66, SLAVE_addr, 0x23, 0x66, 0x0E,0x0F,0x10,0x11};
    const char base_addr0[6]={ 0x34, 0x56, 0x78, 0x23};
    const char base_addr1[6]={ 0x34, 0x56, 0x78, 0x9A};
    memcpy (user_radio_addr.logic_pipe,pipe_addr,8);
    memcpy( user_radio_addr.base_addr0 , base_addr0, 5);
    memcpy( user_radio_addr.base_addr1 , base_addr1 , 5);
    
    update_nrf_radio_address(user_radio_addr);
	cafe_config_t cafe_config       = cafe_DEFAULT_CONFIG_RX;

 	cafe_init( &cafe_config );
	cafe_start_rx();
}


