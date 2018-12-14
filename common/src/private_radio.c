#include "private_radio.h"
#include "nrf_gpio.h"
#include <string.h>
#include <stdio.h>
#include "ws_aes.h"
#include "bbn_board.h"

#define CAFE_DBG(...)	printf(__VA_ARGS__);

#define MAX_SLAVES_AVAILABLE 4
#define AES_BYTES_LENGHT 4 

#define RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS   48   // Smallest reliable value - 43
#define RX_WAIT_FOR_ACK_TIMEOUT_US_1MBPS   64   // Smallest reliable value - 59
#define RX_WAIT_FOR_ACK_TIMEOUT_US_250KBPS 250

#define DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)


#define RADIO_SHORTS_COMMON ( RADIO_SHORTS_READY_START_Msk         |\
				RADIO_SHORTS_END_DISABLE_Msk       |\
				RADIO_SHORTS_ADDRESS_RSSISTART_Msk |\
				RADIO_SHORTS_DISABLED_RSSISTOP_Msk )


static radioconfig_t            m_config_local = CAFE_DEFAULT_CONFIG;
static radio_packet_t		rx_payload;
static radio_packet_t		tx_payload;
static char radio_irq_state = 0 ;
uint32_t radio_status=0;


static bool rx_fifo_push_rfbuf(uint8_t pipe);
static void radio_update_payload_format(uint32_t payload_length);
static void radio_init_addresses(void);
static void radio_rx_setup(void);
static void radio_start_rx_transaction(void);
static void radio_update_nrf_radio_address(nrf_st_address radio_addr);
static void radio_start_tx_transaction(void);
static void radio_tx_setup(void);

static uint8_t radio_check_pipe_limits(uint8_t pipe_id);
static uint32_t radio_disable(void);
static uint32_t radio_get_clear_interrupts(uint32_t *interrupts);



enum {
	S_IRQ_HANDLER  =0X01,
	S_EVENTS_END   =0X02,
	S_EVENTS_READY =0X04,
	S_EVENTS_DISABLED =0X08,
}RADIO_IRQ_STATES;

/****************************************************


           CORE FUNCTION            


****************************************************/

static uint32_t radio_get_clear_interrupts(uint32_t *interrupts)
{
	DISABLE_RF_IRQ;
//	*interrupts         = m_interrupt_flags;
	ENABLE_RF_IRQ;
	return true;
}


static uint32_t bytewise_bit_swap(uint32_t inp)
{
	inp = (inp & 0xF0F0F0F0)  >> 4 | (inp & 0x0F0F0F0F) << 4;
	inp = (inp & 0xCCCCCCCC)  >> 2 | (inp & 0x33333333) << 2;
	return (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
}

static void radio_update_payload_format(uint32_t payload_length)
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
			(RADIO_PACKET_LEN        << RADIO_PCNF1_MAXLEN_Pos);
}

static void radio_update_core_parameters(void)
{
	NRF_RADIO->TXPOWER = m_config_local.tx_output_power << RADIO_TXPOWER_TXPOWER_Pos;
	NRF_RADIO->MODE    = m_config_local.bitrate         << RADIO_MODE_MODE_Pos;
	NRF_RADIO->CRCCNF  = m_config_local.crc             << RADIO_CRCCNF_LEN_Pos;
	NRF_RADIO->CRCINIT = 0xFFFFUL;
	NRF_RADIO->CRCPOLY = 0x11021UL;/* CRC poly: x^16+x^12^x^5+1*/
	radio_update_payload_format(m_config_local.payload_length);
}

/*THIS associate pipes some channels such as timers  with the radio events*/
static void radio_ppi_init(void)
{
	NRF_PPI->CH[CAFE_PPI_TIMER_START].EEP = (uint32_t) &NRF_RADIO->EVENTS_READY;
	NRF_PPI->CH[CAFE_PPI_TIMER_START].TEP = (uint32_t) &CAFE_SYS_TIMER->TASKS_START;
	
	NRF_PPI->CH[CAFE_PPI_TIMER_STOP].EEP  = (uint32_t) &NRF_RADIO->EVENTS_ADDRESS;
	NRF_PPI->CH[CAFE_PPI_TIMER_STOP].TEP  = (uint32_t) &CAFE_SYS_TIMER->TASKS_STOP;
	
	NRF_PPI->CH[CAFE_PPI_RX_TIMEOUT].EEP  = (uint32_t) &CAFE_SYS_TIMER->EVENTS_COMPARE[0];
	NRF_PPI->CH[CAFE_PPI_RX_TIMEOUT].TEP  = (uint32_t) &NRF_RADIO->TASKS_DISABLE;
	
	//NRF_PPI->CH[radioPPI_TX_START].EEP    = (uint32_t) &radioSYS_TIMER->EVENTS_COMPARE[1];
	//NRF_PPI->CH[radioPPI_TX_START].TEP    = (uint32_t) &NRF_RADIO->TASKS_TXEN;
}


void radio_event_handler(void)
{
    static uint32_t rf_interrupts;    
    radio_get_clear_interrupts(&rf_interrupts);
    
    if (rf_interrupts & CAFE_INT_TX_SUCCESS_MSK){   
    }
    
    if (rf_interrupts & CAFE_INT_TX_FAILED_MSK){
    }
    
    if (rf_interrupts & CAFE_INT_RX_DR_MSK){    
    }
}

static void radio_reset(void)
{
	NRF_RADIO->INTENCLR = 0xFFFFFFFF;
    	NRF_RADIO->EVENTS_DISABLED = 0;
	return;    
}


static uint32_t radio_disable(void)
{
	NRF_PPI->CHENCLR =	(1 << CAFE_PPI_TIMER_START)|
				(1 << CAFE_PPI_TIMER_STOP) |
				(1 << CAFE_PPI_RX_TIMEOUT) |
				(1 << CAFE_PPI_TX_START);
	return true;
}



static void radio_update_nrf_radio_address(nrf_st_address radio_addr){	
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

static void radio_init_addresses(void)
{
	nrf_st_address user_radio_addr;

	const char pipe_addr[8]  = { 0x60, CUSTOM_PIPE, 0x12,
				   0xAA,0x12, 0x0F,0x10,0x11};
	const char base_addr0[6] = { 0x34, 0x56, 0x78, 0x23};
	const char base_addr1[6] = { 0x34, 0x56, 0x78, 0x9A};
	
	memcpy (user_radio_addr.logic_pipe , pipe_addr  , 8);
	memcpy( user_radio_addr.base_addr0 , base_addr0 , 5);
	memcpy( user_radio_addr.base_addr1 , base_addr1 , 5);
	
	radio_update_nrf_radio_address(user_radio_addr);	
}

/****************************************************
 *
 *		USER FUNCTIONS
 *
 * 
****************************************************/
/**
 * Call this function to set the initial configuration of the
 * radio, by default radio would start in RADIO_RECEIVER_MODE/RADIO_TRASMITTER MODE
 */
void radio_start(void)
{
	radio_init_addresses();
	 radio_update_mode(RADIO_RECEIVER_MODE);
	//radio_update_mode(RADIO_TRANSMITTER_MODE);

	radio_update_core_parameters();
		
	NVIC_SetPriority( RADIO_IRQn,  m_config_local.radio_irq_priority & 0x03);
}


/**
 @brief Call this function to switch between the FULL TRANMISTTER to FULL RECEIVER
 */
void radio_update_mode(radio_mode_t radio_mode)
{
	radio_reset();
	CAFE_DBG("*****RADIO UPDATE MODE *****\n\r")
	
	switch (radio_mode ){
	case RADIO_RECEIVER_MODE:

		CAFE_DBG("RECEIVER MODE\n\r");
		m_config_local.event_handler = radio_start_rx_transaction,
		m_config_local.mode          = RADIO_RECEIVER_MODE;	
		radio_rx_setup();  		

		break;
	case RADIO_TRANSMITTER_MODE:
	
		CAFE_DBG("TRASMITTER MODE\n\r");
		m_config_local.event_handler = radio_start_tx_transaction,
		m_config_local.mode          = RADIO_TRANSMITTER_MODE;
		radio_tx_setup();
		
		break;	
	default :
		CAFE_DBG("UNDEFINED RADIO MODE!\n\r");	
		
		break;
	}
}

char radioget_rx_payload(char *out_buffer)
{
	uint32_t recieved_addr = NRF_RADIO->RXMATCH;
	memcpy(  out_buffer,   
		rx_payload.data.formated.payload,
		rx_payload.data.formated.length - USER_PACKET_OVERHEAD);
	
	CAFE_DBG ("\n****RX PAYLOAD****\n")
	CAFE_DBG("[%s%d]\n\r"  ,"Pipe:     ",rx_payload.pipe);
	CAFE_DBG("[%s%d]\n\r"  ,"Addr:     ",rx_payload.data.formated.address);
	CAFE_DBG("[%s%d]\n\r"  ,"Ack:      ",rx_payload.data.formated.ack);
	CAFE_DBG("[%s%s]\n\r"  ,"Payload:  ",rx_payload.data.formated.payload);
	CAFE_DBG("[%s%d]\n\r"  ,"Len:      ",rx_payload.data.formated.length);
	CAFE_DBG("[%s%d]\n\r"  ,"Crc:      ",rx_payload.data.formated.crc);
	CAFE_DBG("[%s%d]\n\n\r","RXMatch:  ",recieved_addr );
	nrf_gpio_pin_toggle(LED_RED);	
	return (rx_payload.data.formated.length - USER_PACKET_OVERHEAD);
}

void radio_load_payload(uint8_t pipe_id, char *data,unsigned char len)
{
	if (tx_payload.state.pending) return;
	tx_payload.data.formated.length  = len+USER_PACKET_OVERHEAD;
        tx_payload.data.formated.S1      = 'C';
        tx_payload.data.formated.ack     = 'D';
        tx_payload.data.formated.crc     = 'B';
	tx_payload.data.formated.address = 'A'; 
	tx_payload.pipe = radio_check_pipe_limits( pipe_id );	

	memcpy(tx_payload.data.formated.payload, data, len);

	tx_payload.state.pending =  1;
	
	CAFE_DBG("\n****TX PAYLOAD****\n");
	CAFE_DBG("[%s%d]\n\r"  , "Pipe   : ",tx_payload.pipe);
	CAFE_DBG("[%s%d]\n\r"  , "Addr   : ",tx_payload.data.formated.address);
	CAFE_DBG("[%s%d]\n\r"  , "Ack    : ",tx_payload.data.formated.ack);
	CAFE_DBG("[%s%s]\n\r"  , "Payload: ",tx_payload.data.formated.payload);
	CAFE_DBG("[%s%d]\n\r"  , "Len    : ",tx_payload.data.formated.length);
	CAFE_DBG("[%s%d]\n\n\r", "Crc    : ",tx_payload.data.formated.crc );

	nrf_gpio_pin_toggle(LED_GREEN);
	radio_start_tx_transaction();	
}

void radio_print_current_state(void)
{
	if ( radio_status != NRF_RADIO->STATE )	  CAFE_DBG ("*****STATE: %X *****\n\r",NRF_RADIO->STATE );
	
	radio_status = NRF_RADIO->STATE;

	if ( radio_irq_state )                    CAFE_DBG ("\n\r***RADIO STATUS***\n\r");			
	if ( radio_irq_state & S_IRQ_HANDLER)     CAFE_DBG("IRQ : IRQ_HANDLER\n\r");
	if ( radio_irq_state & S_EVENTS_READY )	  CAFE_DBG("IRQ : EVENTS_READY\n\r");
	if ( radio_irq_state & S_EVENTS_END)      CAFE_DBG("IRQ : EVENTS_END\n\r");
	if ( radio_irq_state & S_EVENTS_DISABLED )CAFE_DBG("IRQ : EVENTS_DISABLED\n\r");
	
	radio_irq_state = 0;
}


void radio_start_task(void){
	NRF_RADIO->TASKS_START= 1;
}

char radio_rx_packet_available(void)
{	
	if(rx_payload.state.available){
		rx_payload.state.available = false;
		return true;
	}
	return false;	
}


int8_t radio_get_rssi(void)
{
	return rx_payload.rssi;
}


/**************************************************** 
 	
                  RX ROUTINES
 
****************************************************/

static bool rx_fifo_push_rfbuf(uint8_t pipe)
{
	rx_payload.rssi  =  NRF_RADIO->RSSISAMPLE;
	return true;
}


static void radio_rx_setup(void)
{	
	NRF_RADIO->TASKS_TXEN      = 0;

	NRF_RADIO->INTENCLR        = 0xFFFFFFFF;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->SHORTS          = RADIO_SHORTS_COMMON | RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->INTENSET        = RADIO_INTENSET_DISABLED_Msk;
	NRF_RADIO->RXADDRESSES     = CUSTOM_PIPE_INDEX; //<--set to dynamic or protocol based address
	NRF_RADIO->FREQUENCY       = m_config_local.rf_channel;
	NRF_RADIO->PACKETPTR       = (uint32_t) rx_payload.data.raw;

	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS  = 0;
	NRF_RADIO->EVENTS_PAYLOAD  = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_RXEN      = 1;
}

static void radio_start_rx_transaction(void)
{
	static uint32_t rf_interrupts;
	NRF_RADIO->SHORTS                = RADIO_SHORTS_COMMON;
	radio_update_payload_format(m_config_local.payload_length);
	
	NRF_RADIO->PACKETPTR             = (uint32_t) &rx_payload.data.raw[0];
	NRF_RADIO->EVENTS_DISABLED       = 0;
	NRF_RADIO->TASKS_DISABLE         = 1;
	while(0 == NRF_RADIO->EVENTS_DISABLED);
	
	NRF_RADIO->EVENTS_DISABLED       = 0;
	NRF_RADIO->SHORTS                = RADIO_SHORTS_COMMON | 
	RADIO_SHORTS_DISABLED_RXEN_Msk;
	NRF_RADIO->TASKS_RXEN            = 1;	
	rx_fifo_push_rfbuf( NRF_RADIO->RXMATCH );
	//m_interrupt_flags   |= CAFE_INT_RX_DR_MSK;
	radio_get_clear_interrupts(&rf_interrupts);
		rx_payload.state.available= true;
	
#if 0
	if (rf_interrupts & CAFE_INT_TX_SUCCESS_MSK)
	{   
	}
	
	if (rf_interrupts & CAFE_INT_TX_FAILED_MSK)
	{
	}
#endif
	if (rf_interrupts & CAFE_INT_RX_DR_MSK){
		rx_payload.state.available= true;
	}
}

/**************************************************** 
 	
                  TX ROUTINES
 
****************************************************/


static void radio_tx_setup(void)
{
	NRF_RADIO->TASKS_RXEN        = 0;
	
	NRF_RADIO->INTENCLR          = 0xFFFFFFFF;	
	NRF_RADIO->EVENTS_DISABLED   = 0;
	NRF_RADIO->SHORTS            = RADIO_SHORTS_COMMON;
	NRF_RADIO->INTENSET          = RADIO_INTENSET_DISABLED_Msk;
	NRF_RADIO->TXADDRESS         = tx_payload.pipe;
	NRF_RADIO->RXADDRESSES       = CUSTOM_PIPE_INDEX;                         
	NRF_RADIO->FREQUENCY         = m_config_local.rf_channel; 
	NRF_RADIO->PACKETPTR         = (uint32_t)tx_payload.data.raw;
  	
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS  = 0;
	NRF_RADIO->EVENTS_PAYLOAD  = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;	
	NRF_RADIO->TASKS_TXEN      = 1;
}



static uint8_t radio_check_pipe_limits(uint8_t pipe_id)
{
	if (pipe_id < MAX_SLAVES_AVAILABLE)
		return	 pipe_id;
	return 0;
}

static void radio_start_tx_transaction(void)
{
	if(!tx_payload.state.pending) return;
	tx_payload.state.pending   = 0;
// TO DO: Add function instead
#ifndef NO_CYPHER 
	aes_encrypt_data( tx_payload.data,
			tx_payload.data.formated.length ,
			&m_tx_payload_buffer[2]);
	tx_payload.data.formated.length += AES_BYTES_LENGHT;
#endif     


	tx_payload.data.formated.S1 = 1;

	NRF_RADIO->SHORTS      = RADIO_SHORTS_COMMON;
	NRF_RADIO->INTENSET    = RADIO_INTENSET_DISABLED_Msk;
	NRF_RADIO->TXADDRESS   = tx_payload.pipe++;
	NRF_RADIO->RXADDRESSES = CUSTOM_PIPE_INDEX;
	NRF_RADIO->FREQUENCY   = m_config_local.rf_channel;
	NRF_RADIO->PACKETPTR   = (uint32_t)tx_payload.data.raw;
  
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS  = 0;
	NRF_RADIO->EVENTS_PAYLOAD  = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;	
	NRF_RADIO->TASKS_TXEN      = 1;
}



/**************************************************** 
 	
                  RADIO ISR
 
****************************************************/


void RADIO_IRQHandler(){

	radio_irq_state                                 |=S_IRQ_HANDLER ;
	if (NRF_RADIO->EVENTS_END     ) radio_irq_state |=S_EVENTS_END;
	if (NRF_RADIO->EVENTS_DISABLED) radio_irq_state |=S_EVENTS_DISABLED;
	if (NRF_RADIO->EVENTS_READY   ) radio_irq_state |=S_EVENTS_READY;
	
	if (NRF_RADIO->EVENTS_READY &&
	   (NRF_RADIO->INTENSET     & RADIO_INTENSET_READY_Msk)){
	
		NRF_RADIO->EVENTS_READY    = 0;
	}
	
	if (NRF_RADIO->EVENTS_END &&
	   (NRF_RADIO->INTENSET   & RADIO_INTENSET_END_Msk)){		
		
		NRF_RADIO->EVENTS_END      = 0;
	}
	
	if (NRF_RADIO->EVENTS_DISABLED &&
	   (NRF_RADIO->INTENSET        & RADIO_INTENSET_DISABLED_Msk)){
		
		NRF_RADIO->EVENTS_DISABLED = 0; 
		
		if (m_config_local.event_handler) m_config_local.event_handler();			
	
	}
}
