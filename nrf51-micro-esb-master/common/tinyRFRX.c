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

#include "tinyRFRX.h"
#include "uesb_error_codes.h"
#include "nrf_gpio.h"
#include <string.h>


static tinyrx_event_handler_t     m_event_handler;
unsigned int led_state=0;
// RF parameters
static tinyrx_config_t            m_config_local;

// RX FIFO
static tinyrx_payload_t           m_rx_fifo_payload[tinyrx_CORE_RX_FIFO_SIZE];
static tinyrx_payload_rx_fifo_t   m_rx_fifo;
static  uint8_t                 m_rx_payload_buffer[tinyrx_CORE_MAX_PAYLOAD_LENGTH + 2];

// Run time variables
static volatile uint32_t        m_interrupt_flags       = 0;
static volatile uint32_t        m_last_rx_packet_crc = 0xFFFFFFFF;


// Constant parameters
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS   48   // Smallest reliable value - 43
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_1MBPS   64   // Smallest reliable value - 59
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_250KBPS 250

// Macros
#define                         DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define                         ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)

#define                         RADIO_SHORTS_COMMON ( RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk | \
RADIO_SHORTS_ADDRESS_RSSISTART_Msk | RADIO_SHORTS_DISABLED_RSSISTOP_Msk )


int8_t get_rssi(void){
	return m_rx_fifo.payload_ptr[m_rx_fifo.entry_point]->rssi;
}
// These function pointers are changed dynamically, depending on protocol configuration and state
static void (*on_radio_disabled)(void) = 0;
static void (*update_rf_payload_format)(uint32_t payload_length) = 0;

// The following functions are assigned to the function pointers above
static void on_radio_disabled_esb_dpl_rx(void);


static void update_rf_payload_format_esb_dpl(uint32_t payload_length)
{
//#if(tinyrx_CORE_MAX_PAYLOAD_LENGTH <= 32)
	NRF_RADIO->PCNF0 = (0 << RADIO_PCNF0_S0LEN_Pos) | (6 << RADIO_PCNF0_LFLEN_Pos) | (3 << RADIO_PCNF0_S1LEN_Pos);
	NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled        << RADIO_PCNF1_WHITEEN_Pos) |
	(RADIO_PCNF1_ENDIAN_Big              << RADIO_PCNF1_ENDIAN_Pos)  |
	((m_config_local.rf_addr_length - 1) << RADIO_PCNF1_BALEN_Pos)   |
	(0                                   << RADIO_PCNF1_STATLEN_Pos) |
	(tinyrx_CORE_MAX_PAYLOAD_LENGTH        << RADIO_PCNF1_MAXLEN_Pos);
}

// Function that swaps the bits within each byte in a uint32. Used to convert from nRF24L type addressing to nRF51 type addressing
static uint32_t bytewise_bit_swap(uint32_t inp)
{
	inp = (inp & 0xF0F0F0F0) >> 4 | (inp & 0x0F0F0F0F) << 4;
	inp = (inp & 0xCCCCCCCC) >> 2 | (inp & 0x33333333) << 2;
	return (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
}

static void update_radio_parameters()
{
	update_rf_payload_format = update_rf_payload_format_esb_dpl;

	NRF_RADIO->TXPOWER   = m_config_local.tx_output_power   << RADIO_TXPOWER_TXPOWER_Pos;

// RF bitrate
	NRF_RADIO->MODE      = m_config_local.bitrate           << RADIO_MODE_MODE_Pos;

// CRC configuration
	NRF_RADIO->CRCCNF    = m_config_local.crc               << RADIO_CRCCNF_LEN_Pos;
	NRF_RADIO->CRCINIT = 0xFFFFUL;      // Initial value
	NRF_RADIO->CRCPOLY = 0x11021UL;     // CRC poly: x^16+x^12^x^5+1

// Packet format
	update_rf_payload_format(m_config_local.payload_length);
// Radio address config
	NRF_RADIO->PREFIX0 = bytewise_bit_swap(m_config_local.rx_address_p3 << 24 | m_config_local.rx_address_p2 << 16 | m_config_local.rx_address_p1[0] << 8 | m_config_local.rx_address_p0[0]);
	NRF_RADIO->PREFIX1 = bytewise_bit_swap(m_config_local.rx_address_p7 << 24 | m_config_local.rx_address_p6 << 16 | m_config_local.rx_address_p5 << 8 | m_config_local.rx_address_p4);
	NRF_RADIO->BASE0   = bytewise_bit_swap(m_config_local.rx_address_p0[1] << 24 | m_config_local.rx_address_p0[2] << 16 | m_config_local.rx_address_p0[3] << 8 | m_config_local.rx_address_p0[4]);
	NRF_RADIO->BASE1   = bytewise_bit_swap(m_config_local.rx_address_p1[1] << 24 | m_config_local.rx_address_p1[2] << 16 | m_config_local.rx_address_p1[3] << 8 | m_config_local.rx_address_p1[4]);
}

static void initialize_fifos()
{
	m_rx_fifo.entry_point = 0;
	m_rx_fifo.exit_point  = 0;
	m_rx_fifo.count       = 0;
	for(int i = 0; i < tinyrx_CORE_RX_FIFO_SIZE; i++)
	{
		m_rx_fifo.payload_ptr[i] = &m_rx_fifo_payload[i];
	}
}


static bool rx_fifo_push_rfbuf(uint8_t pipe)
{
	if(m_rx_fifo.count < tinyrx_CORE_RX_FIFO_SIZE)
	{
		if(m_rx_payload_buffer[0] > tinyrx_CORE_MAX_PAYLOAD_LENGTH) return false;
		
		m_rx_fifo.payload_ptr[m_rx_fifo.entry_point]->length = m_rx_payload_buffer[0];
		
		memcpy(m_rx_fifo.payload_ptr[m_rx_fifo.entry_point]->data, &m_rx_payload_buffer[2], m_rx_fifo.payload_ptr[m_rx_fifo.entry_point]->length);

		m_rx_fifo.payload_ptr[m_rx_fifo.entry_point]->pipe = pipe;
		m_rx_fifo.payload_ptr[m_rx_fifo.entry_point]->rssi = NRF_RADIO->RSSISAMPLE;
		if(++m_rx_fifo.entry_point >= tinyrx_CORE_RX_FIFO_SIZE) m_rx_fifo.entry_point = 0;
		m_rx_fifo.count++;
		return true;
	}
	return false;
}


static void ppi_init()
{
	NRF_PPI->CH[tinyrx_PPI_TIMER_START].EEP = (uint32_t)&NRF_RADIO->EVENTS_READY;
	NRF_PPI->CH[tinyrx_PPI_TIMER_START].TEP = (uint32_t)&tinyrx_SYS_TIMER->TASKS_START;
	NRF_PPI->CH[tinyrx_PPI_TIMER_STOP].EEP  = (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
	NRF_PPI->CH[tinyrx_PPI_TIMER_STOP].TEP  = (uint32_t)&tinyrx_SYS_TIMER->TASKS_STOP;
	NRF_PPI->CH[tinyrx_PPI_RX_TIMEOUT].EEP  = (uint32_t)&tinyrx_SYS_TIMER->EVENTS_COMPARE[0];
	NRF_PPI->CH[tinyrx_PPI_RX_TIMEOUT].TEP  = (uint32_t)&NRF_RADIO->TASKS_DISABLE;
	NRF_PPI->CH[tinyrx_PPI_TX_START].EEP    = (uint32_t)&tinyrx_SYS_TIMER->EVENTS_COMPARE[1];
	NRF_PPI->CH[tinyrx_PPI_TX_START].TEP    = (uint32_t)&NRF_RADIO->TASKS_TXEN;
}

uint32_t tinyrx_init(tinyrx_config_t *parameters)
{
	m_event_handler = parameters->event_handler;
	memcpy(&m_config_local, parameters, sizeof(tinyrx_config_t));

	m_interrupt_flags    = 0;
	m_last_rx_packet_crc = 0xFFFFFFFF;

	update_radio_parameters();

	initialize_fifos();

	ppi_init();

	NVIC_SetPriority(RADIO_IRQn, m_config_local.radio_irq_priority & 0x03);


	return UESB_SUCCESS;
}

uint32_t tinyrx_disable(void)
{
	NRF_PPI->CHENCLR = (1 << tinyrx_PPI_TIMER_START) | (1 << tinyrx_PPI_TIMER_STOP) | (1 << tinyrx_PPI_RX_TIMEOUT) | (1 << tinyrx_PPI_TX_START);
	return UESB_SUCCESS;
}



uint32_t tinyrx_read_rx_payload(tinyrx_payload_t *payload)
{
	if(m_rx_fifo.count == 0) return UESB_ERROR_RX_FIFO_EMPTY;

	DISABLE_RF_IRQ;
	
	payload->length = m_rx_fifo.payload_ptr[m_rx_fifo.exit_point]->length;
	payload->pipe   = m_rx_fifo.payload_ptr[m_rx_fifo.exit_point]->pipe;
	payload->rssi   = m_rx_fifo.payload_ptr[m_rx_fifo.exit_point]->rssi;
	
	memcpy(payload->data, m_rx_fifo.payload_ptr[m_rx_fifo.exit_point]->data, payload->length);
	
	if(++m_rx_fifo.exit_point >= tinyrx_CORE_RX_FIFO_SIZE) m_rx_fifo.exit_point = 0;
	m_rx_fifo.count--;
	
	ENABLE_RF_IRQ;

	return UESB_SUCCESS;
}

uint32_t tinyrx_start_rx(void)
{

	NRF_RADIO->INTENCLR = 0xFFFFFFFF;
	NRF_RADIO->EVENTS_DISABLED = 0;
	on_radio_disabled = on_radio_disabled_esb_dpl_rx;
	NRF_RADIO->SHORTS      = RADIO_SHORTS_COMMON | RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->INTENSET    = RADIO_INTENSET_DISABLED_Msk;


	NRF_RADIO->RXADDRESSES = m_config_local.rx_pipes_enabled;

	NRF_RADIO->FREQUENCY = m_config_local.rf_channel;

	NRF_RADIO->PACKETPTR = (uint32_t)m_rx_payload_buffer;

	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_RXEN  = 1;
	return UESB_SUCCESS;
}


uint32_t tinyrx_flush_rx(void)
{
	DISABLE_RF_IRQ;
	m_rx_fifo.count = 0;
	m_rx_fifo.entry_point = m_rx_fifo.exit_point = 0;
	ENABLE_RF_IRQ;
	return UESB_SUCCESS;
}

uint32_t tinyrx_get_clear_interrupts(uint32_t *interrupts)
{
	DISABLE_RF_IRQ;
	*interrupts = m_interrupt_flags;
	m_interrupt_flags = 0;
	ENABLE_RF_IRQ;
	return UESB_SUCCESS;
}

uint32_t tinyrx_set_address(tinyrx_address_type_t address, const uint8_t *data_ptr)
{
	switch(address)
	{
		case tinyrx_ADDRESS_PIPE0:
		memcpy(m_config_local.rx_address_p0, data_ptr, m_config_local.rf_addr_length);
		break;
		case tinyrx_ADDRESS_PIPE1:
		memcpy(m_config_local.rx_address_p1, data_ptr, m_config_local.rf_addr_length);
		break;
		case tinyrx_ADDRESS_PIPE2:
		m_config_local.rx_address_p2 = *data_ptr;
		break;
		case tinyrx_ADDRESS_PIPE3:
		m_config_local.rx_address_p3 = *data_ptr;
		break;
		case tinyrx_ADDRESS_PIPE4:
		m_config_local.rx_address_p4 = *data_ptr;
		break;
		case tinyrx_ADDRESS_PIPE5:
		m_config_local.rx_address_p5 = *data_ptr;
		break;
		case tinyrx_ADDRESS_PIPE6:
		m_config_local.rx_address_p6 = *data_ptr;
		break;
		case tinyrx_ADDRESS_PIPE7:
		m_config_local.rx_address_p7 = *data_ptr;
		break;
		default:
		return UESB_ERROR_INVALID_PARAMETERS;
	}
	update_radio_parameters();
	return UESB_SUCCESS;
}

uint32_t tinyrx_set_rf_channel(uint32_t channel)
{
	if(channel > 125) return UESB_ERROR_INVALID_PARAMETERS;
	m_config_local.rf_channel = channel;
	return UESB_SUCCESS;
}



void RADIO_IRQHandler()
{
	if(NRF_RADIO->EVENTS_READY && (NRF_RADIO->INTENSET & RADIO_INTENSET_READY_Msk)){
		NRF_RADIO->EVENTS_READY = 0;
	}

	if(NRF_RADIO->EVENTS_END && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)){
		NRF_RADIO->EVENTS_END = 0;
	}

	if(NRF_RADIO->EVENTS_DISABLED && (NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk)){
		NRF_RADIO->EVENTS_DISABLED = 0;
		// Call the correct on_radio_disable function, depending on the current protocol state
		if(on_radio_disabled){
			on_radio_disabled();
		}
	}
}


static void on_radio_disabled_esb_dpl_rx(void)
{

	NRF_RADIO->SHORTS = RADIO_SHORTS_COMMON;
	update_rf_payload_format(m_config_local.payload_length);
	NRF_RADIO->PACKETPTR = (uint32_t)m_rx_payload_buffer;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_DISABLE = 1;
	while(NRF_RADIO->EVENTS_DISABLED == 0);
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->SHORTS = RADIO_SHORTS_COMMON | RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->TASKS_RXEN = 1;
	
	rx_fifo_push_rfbuf(NRF_RADIO->RXMATCH);
	m_interrupt_flags |= tinyrx_INT_RX_DR_MSK;
	if(m_event_handler != 0) m_event_handler();
	 
}

static tinyrx_payload_t  rx_payload;
void tinyrx_event_handler_rx(void)
{
	static uint32_t rf_interrupts;
	
	tinyrx_get_clear_interrupts(&rf_interrupts);
	
	if(rf_interrupts & tinyrx_INT_TX_SUCCESS_MSK)
	{   
	}
	
	if(rf_interrupts & tinyrx_INT_TX_FAILED_MSK)
	{
	}
	
	if(rf_interrupts & tinyrx_INT_RX_DR_MSK)
	{
		tinyrx_read_rx_payload(&rx_payload);
		led_state=1;
	}
	
}



void tinyrx_setup_rx(void){


	uint8_t rx_addr_p0[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
	uint8_t rx_addr_p1[] = {0xBA, 0xDE, 0xF0, 0x12, 0x2C};
	uint8_t rx_addr_p2   = 0x66;
	
	tinyrx_config_t tinyrx_config       = tinyrx_DEFAULT_CONFIG;
	tinyrx_config.rf_channel          = 5;
	tinyrx_config.crc                 = tinyrx_CRC_16BIT;
	tinyrx_config.payload_length      = 8;
	//tinyrx_config.protocol            = tinyrx_PROTOCOL_ESB_DPL;
	tinyrx_config.bitrate             = tinyrx_BITRATE_2MBPS;
	tinyrx_config.mode                = tinyrx_MODE_PRX;
	tinyrx_config.event_handler       = tinyrx_event_handler_rx;
	
	tinyrx_init(&tinyrx_config);

	tinyrx_set_address(tinyrx_ADDRESS_PIPE0, rx_addr_p0);
	tinyrx_set_address(tinyrx_ADDRESS_PIPE1, rx_addr_p1);
	tinyrx_set_address(tinyrx_ADDRESS_PIPE2, &rx_addr_p2);
  
	tinyrx_start_rx();
}


