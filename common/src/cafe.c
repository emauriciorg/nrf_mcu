#include "cafe.h"
#include "nrf_gpio.h"
#include <string.h>
#include <stdio.h>
#include "ws_aes.h"
#include "bbn_board.h"

#define CAFE_DBG(...)	printf(__VA_ARGS__);

#define NO_CYPHER
#define MAX_SLAVES_AVAILABLE 4
#define AES_BYTES_LENGHT 4 

// Constant parameters
#define RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS   48   // Smallest reliable value - 43
#define RX_WAIT_FOR_ACK_TIMEOUT_US_1MBPS   64   // Smallest reliable value - 59
#define RX_WAIT_FOR_ACK_TIMEOUT_US_250KBPS 250

#define DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)

#define RADIO_SHORTS_COMMON ( RADIO_SHORTS_READY_START_Msk         |\
				RADIO_SHORTS_END_DISABLE_Msk       |\
				RADIO_SHORTS_ADDRESS_RSSISTART_Msk |\
				RADIO_SHORTS_DISABLED_RSSISTOP_Msk )

static cafe_config_t            m_config_local=CAFE_DEFAULT_CONFIG;
static cafe_payload_t		rx_payload;
static cafe_payload_t		tx_payload;
static volatile uint32_t        m_interrupt_flags       = 0;


static void update_payload_format(uint32_t payload_length);
static bool rx_fifo_push_rfbuf(uint8_t pipe);
static void cafe_init_radio_address(void);
static uint32_t cafe_init(cafe_config_t *parameters);

char cafe_pending(void){	
	if(rx_payload.pending){
		rx_payload.pending = false;
		return true;
	}
	return false;	
}

void cafe_load_payload(unsigned char slave_id, char *data,unsigned char len){
	if (tx_payload.pending) return;
	tx_payload.data.formated.length  = len+USER_PACKET_OVERHEAD;
        tx_payload.data.formated.S1      = 'C';
        tx_payload.data.formated.ack     = 'D';
        tx_payload.data.formated.crc     = 'B';
	tx_payload.data.formated.address = 'A'; 
	
	if (slave_id>MAX_SLAVES_AVAILABLE)
		tx_payload.pipe =0;
	else
		tx_payload.pipe = slave_id;
	memcpy(tx_payload.data.formated.payload,data,len);

	tx_payload.pending =  1;
	CAFE_DBG("\n[S%d][Addr %d][ack %d][%s][%d][%d] \n",tx_payload.pipe,
						tx_payload.data.formated.address,
						tx_payload.data.formated.ack,
						tx_payload.data.formated.payload,
						tx_payload.data.formated.length,
						tx_payload.data.formated.crc );
	cafe_start_tx_transaction();	
}


uint32_t cafe_get_clear_interrupts(uint32_t *interrupts){
	DISABLE_RF_IRQ;
	*interrupts         = m_interrupt_flags;
	m_interrupt_flags   = 0;
	ENABLE_RF_IRQ;
	return true;
}



void cafe_start_rx_transaction(void){
	static uint32_t rf_interrupts;
	NRF_RADIO->SHORTS                = RADIO_SHORTS_COMMON;
	update_payload_format(m_config_local.payload_length);
	
	NRF_RADIO->PACKETPTR             = (uint32_t) &rx_payload.data.raw[0];
	NRF_RADIO->EVENTS_DISABLED       = 0;
	NRF_RADIO->TASKS_DISABLE         = 1;
	while(NRF_RADIO->EVENTS_DISABLED == 0);
	
	NRF_RADIO->EVENTS_DISABLED       = 0;
	NRF_RADIO->SHORTS                = RADIO_SHORTS_COMMON | 
	RADIO_SHORTS_DISABLED_RXEN_Msk;
	NRF_RADIO->TASKS_RXEN            = 1;	
	rx_fifo_push_rfbuf( NRF_RADIO->RXMATCH );
	m_interrupt_flags   |= cafe_INT_RX_DR_MSK;
	cafe_get_clear_interrupts(&rf_interrupts);
	
#if 0
	if (rf_interrupts & cafe_INT_TX_SUCCESS_MSK)
	{   
	}
	
	if (rf_interrupts & cafe_INT_TX_FAILED_MSK)
	{
	}
#endif
	if (rf_interrupts & cafe_INT_RX_DR_MSK){
		rx_payload.pending= true;
	}
}

static uint32_t bytewise_bit_swap(uint32_t inp){
	inp = (inp & 0xF0F0F0F0)  >> 4 | (inp & 0x0F0F0F0F) << 4;
	inp = (inp & 0xCCCCCCCC)  >> 2 | (inp & 0x33333333) << 2;
	return (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
}

static bool rx_fifo_push_rfbuf(uint8_t pipe){
	rx_payload.rssi  =  NRF_RADIO->RSSISAMPLE;
	return true;
}

int8_t cafe_get_rssi(void){
	return rx_payload.rssi;
}

static void update_payload_format(uint32_t payload_length)
{
	NRF_RADIO->PCNF0 = 
			(0 << RADIO_PCNF0_S0LEN_Pos) |
			(6 << RADIO_PCNF0_LFLEN_Pos) |
			(3 << RADIO_PCNF0_S1LEN_Pos);
	NRF_RADIO->PCNF1 = 
			(RADIO_PCNF1_WHITEEN_Disabled        << RADIO_PCNF1_WHITEEN_Pos) |
			(RADIO_PCNF1_ENDIAN_Big              << RADIO_PCNF1_ENDIAN_Pos)  |
			((m_config_local.rf_addr_length - 1) << RADIO_PCNF1_BALEN_Pos)   |
			(0                                   << RADIO_PCNF1_STATLEN_Pos) |
			(CAFE_CORE_MAX_PAYLOAD_LENGTH        << RADIO_PCNF1_MAXLEN_Pos);
}

static void update_radio_parameters(){
	NRF_RADIO->TXPOWER = m_config_local.tx_output_power << RADIO_TXPOWER_TXPOWER_Pos;
	NRF_RADIO->MODE    = m_config_local.bitrate         << RADIO_MODE_MODE_Pos;
	NRF_RADIO->CRCCNF  = m_config_local.crc             << RADIO_CRCCNF_LEN_Pos;
	NRF_RADIO->CRCINIT = 0xFFFFUL;
	NRF_RADIO->CRCPOLY = 0x11021UL;/* CRC poly: x^16+x^12^x^5+1*/
	update_payload_format(m_config_local.payload_length);
}

 void cafe_ppi_init(void){
	NRF_PPI->CH[CAFE_PPI_TIMER_START].EEP = (uint32_t) &NRF_RADIO->EVENTS_READY;
	NRF_PPI->CH[CAFE_PPI_TIMER_START].TEP = (uint32_t) &cafe_SYS_TIMER->TASKS_START;
	NRF_PPI->CH[CAFE_PPI_TIMER_STOP].EEP  = (uint32_t) &NRF_RADIO->EVENTS_ADDRESS;
	NRF_PPI->CH[CAFE_PPI_TIMER_STOP].TEP  = (uint32_t) &cafe_SYS_TIMER->TASKS_STOP;
	NRF_PPI->CH[CAFE_PPI_RX_TIMEOUT].EEP  = (uint32_t) &cafe_SYS_TIMER->EVENTS_COMPARE[0];
	NRF_PPI->CH[CAFE_PPI_RX_TIMEOUT].TEP  = (uint32_t) &NRF_RADIO->TASKS_DISABLE;
	//NRF_PPI->CH[cafe_PPI_TX_START].EEP    = (uint32_t) &cafe_SYS_TIMER->EVENTS_COMPARE[1];
	//NRF_PPI->CH[cafe_PPI_TX_START].TEP    = (uint32_t) &NRF_RADIO->TASKS_TXEN;
}


void cafe_start_tx_transaction(void)
{
	if(!tx_payload.pending) return;
	tx_payload.pending   = 0;
	
#ifdef NO_CYPHER
//	memcpy( &tx_payload.data.payload, tx_payload.data, tx_payload.length -USER_PACKET_OVERHEAD );	
#else
	aes_encrypt_data( tx_payload.data, tx_payload.length, &m_tx_payload_buffer[2]);
#endif     
//	tx_payload.data.formated.length += AES_BYTES_LENGHT;
	tx_payload.data.formated.S1 = 1;

	NRF_RADIO->SHORTS      = RADIO_SHORTS_COMMON;
	NRF_RADIO->INTENSET    = RADIO_INTENSET_DISABLED_Msk;
	NRF_RADIO->TXADDRESS   = tx_payload.pipe++;
	NRF_RADIO->RXADDRESSES = SLAVE_ADDR_INDEX;
	NRF_RADIO->FREQUENCY   = m_config_local.rf_channel;
	NRF_RADIO->PACKETPTR   = (uint32_t)tx_payload.data.raw;
  
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;	
	NRF_RADIO->TASKS_TXEN     = 1;
}

void cafe_event_handler(){
    static uint32_t rf_interrupts;    
    cafe_get_clear_interrupts(&rf_interrupts);
    
    if (rf_interrupts & CAFE_INT_TX_SUCCESS_MSK){   
    }
    
    if (rf_interrupts & CAFE_INT_TX_FAILED_MSK){
    }
    
    if (rf_interrupts & CAFE_INT_RX_DR_MSK){    
    }
}



uint32_t cafe_start_rx(void)
{
	NRF_RADIO->TASKS_TXEN      = 0;
	NRF_RADIO->INTENCLR        = 0xFFFFFFFF;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->SHORTS          = RADIO_SHORTS_COMMON | RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->INTENSET        = RADIO_INTENSET_DISABLED_Msk;
	NRF_RADIO->RXADDRESSES     = SLAVE_ADDR_INDEX; //<--set to dynamic or protocol based address
	NRF_RADIO->FREQUENCY       = m_config_local.rf_channel;
	NRF_RADIO->PACKETPTR       = (uint32_t) rx_payload.data.raw;

	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS  = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_RXEN      = 1;
	return true;
}

void cafe_radio_radio(void){
	DISABLE_RF_IRQ;
        NRF_RADIO->SHORTS = 0;
        NRF_RADIO->INTENCLR = 0xFFFFFFFF;
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE = 1;
        while(NRF_RADIO->EVENTS_DISABLED == 0);
	NRF_RADIO->TASKS_RXEN      = 0;
	NRF_RADIO->TASKS_TXEN      = 0;
}
void cafe_radio_update_mode(char radio_mode){
	cafe_radio_radio();
	cafe_init_radio_address();
	switch (radio_mode ){
	case I_AM_RECIEVER:
			m_config_local.event_handler = cafe_start_rx_transaction,
			m_config_local.mode          = I_AM_RECIEVER;
			cafe_init( &m_config_local );
			cafe_start_rx();  		
			if(tx_payload.pending) {
				cafe_start_tx_transaction();	
			}	
			break;
	case I_AM_TRANSMITTER:
			m_config_local.event_handler = cafe_start_tx_transaction,
			m_config_local.mode          = I_AM_TRANSMITTER;
			cafe_init( &m_config_local );
			break;	
	default :break;
	}
}

void cafe_update_nrf_radio_address(nrf_st_address radio_addr){	
	NRF_RADIO->PREFIX0 = bytewise_bit_swap( radio_addr.logic_pipe[3]  << 24 |   
						radio_addr.logic_pipe[2]  << 16 |
						radio_addr.logic_pipe[1]  <<  8 |
						radio_addr.logic_pipe[0]);

	NRF_RADIO->PREFIX1 = bytewise_bit_swap( radio_addr.logic_pipe[7]  << 24 |
						radio_addr.logic_pipe[6]  << 16 |
						radio_addr.logic_pipe[5]  <<  8 |  
						radio_addr.logic_pipe[4]);
	NRF_RADIO->BASE0   = bytewise_bit_swap( radio_addr.base_addr0[0] << 24 |
						radio_addr.base_addr0[1] << 16 |
						radio_addr.base_addr0[2] << 8  |
						radio_addr.base_addr0[3]);
	NRF_RADIO->BASE1   = bytewise_bit_swap( radio_addr.base_addr1[0] << 24 |
						radio_addr.base_addr1[1] << 16 |
						radio_addr.base_addr1[2] << 8  |
						radio_addr.base_addr1[3]);
}


char cafe_get_rx_payload(char *out_buffer){
	memcpy(  out_buffer,   rx_payload.data.formated.payload,  rx_payload.data.formated.length - USER_PACKET_OVERHEAD);
	CAFE_DBG("\n[S%d][Addr %d][ack %d][%s][%d][%d] \n",rx_payload.pipe,
					rx_payload.data.formated.address,
					rx_payload.data.formated.ack,
					rx_payload.data.formated.payload,
					rx_payload.data.formated.length,
					rx_payload.data.formated.crc );

	return (rx_payload.data.formated.length - USER_PACKET_OVERHEAD);
}

uint32_t cafe_disable(void){
	NRF_PPI->CHENCLR =	(1 << CAFE_PPI_TIMER_START)|
				(1 << CAFE_PPI_TIMER_STOP) |
				(1 << CAFE_PPI_RX_TIMEOUT) |
				(1 << CAFE_PPI_TX_START);
	return true;
}

uint32_t cafe_read_rx_payload(cafe_payload_t *payload){	
	DISABLE_RF_IRQ;
	ENABLE_RF_IRQ;
	return true;
}


uint32_t cafe_set_rf_channel(uint32_t channel){

	if (channel > 125) return false;
	m_config_local.rf_channel = channel;
	return true;
}

void RADIO_IRQHandler(){
	if (NRF_RADIO->EVENTS_READY && (NRF_RADIO->INTENSET & RADIO_INTENSET_READY_Msk)){
		NRF_RADIO->EVENTS_READY = 0;
	}
	if (NRF_RADIO->EVENTS_END && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)){		
		NRF_RADIO->EVENTS_END = 0;
	}
	if (NRF_RADIO->EVENTS_DISABLED &&(NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk)){
		NRF_RADIO->EVENTS_DISABLED = 0; 
		m_interrupt_flags         |= (0X01);
		if (m_config_local.event_handler){
			m_config_local.event_handler();			
		}
		nrf_gpio_pin_toggle(LED_GREEN);
	}
}

void cafe_init_radio_address(void){
	nrf_st_address user_radio_addr;
	DISABLE_RF_IRQ;
	const char pipe_addr[8]  = { 0x60, SLAVE_addr, 0x12, 0xAA, 0x12,0x0F,0x10,0x11};
	const char base_addr0[6] = { 0x34, 0x56, 0x78, 0x23};
	const char base_addr1[6] = { 0x34, 0x56, 0x78, 0x9A};
	
	memcpy (user_radio_addr.logic_pipe , pipe_addr  , 8);
	memcpy( user_radio_addr.base_addr0 , base_addr0 , 5);
	memcpy( user_radio_addr.base_addr1 , base_addr1 , 5);
	cafe_update_nrf_radio_address(user_radio_addr);	
}

uint32_t cafe_init(cafe_config_t *parameters){
	memcpy(&m_config_local, parameters, sizeof(cafe_config_t));
	m_interrupt_flags    = 0;
	update_radio_parameters();
	cafe_ppi_init();	
	NVIC_SetPriority( RADIO_IRQn,  m_config_local.radio_irq_priority & 0x03);
	return true;
}

void cafe_tx_setup(void){
	NRF_RADIO->SHORTS      = RADIO_SHORTS_COMMON;
	NRF_RADIO->INTENSET    = RADIO_INTENSET_DISABLED_Msk;
	NRF_RADIO->TXADDRESS   = tx_payload.pipe;
	NRF_RADIO->RXADDRESSES = SLAVE_ADDR_INDEX;                         
	NRF_RADIO->FREQUENCY   = m_config_local.rf_channel; 
	NRF_RADIO->PACKETPTR   = (uint32_t)tx_payload.data.raw;
  	
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;
	
	NRF_RADIO->TASKS_TXEN  = 1;
}
	
#define RADIO_ONLY_TRANSMITTER	

void cafe_start_radio(void){
#ifdef RADIO_ONLY_TRANSMITTER	
	cafe_radio_update_mode(I_AM_TRANSMITTER);
#else
	cafe_radio_update_mode(I_AM_RECIEVER );
#endif	
}
