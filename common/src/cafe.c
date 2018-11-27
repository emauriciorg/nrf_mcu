#include "cafe.h"
#include "nrf_gpio.h"
#include <string.h>
#include <stdio.h>
#include "ws_aes.h"
#include "bbn_board.h"

// Constant parameters
#define RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS   48   // Smallest reliable value - 43
#define RX_WAIT_FOR_ACK_TIMEOUT_US_1MBPS   64   // Smallest reliable value - 59
#define RX_WAIT_FOR_ACK_TIMEOUT_US_250KBPS 250

                                                                        // Macros
#define DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)

                                                                        // Constant parameters
#define RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS   48   // Smallest reliable value - 43
#define RX_WAIT_FOR_ACK_TIMEOUT_US_1MBPS   64   // Smallest reliable value - 59
#define RX_WAIT_FOR_ACK_TIMEOUT_US_250KBPS 250

                                                                        // Macros
#define DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)

#define RADIO_SHORTS_COMMON ( RADIO_SHORTS_READY_START_Msk\
						    | RADIO_SHORTS_END_DISABLE_Msk  \
						    | RADIO_SHORTS_ADDRESS_RSSISTART_Msk \
						    | RADIO_SHORTS_DISABLED_RSSISTOP_Msk )

#define MAX_SLAVES_AVAILABLE 4


int8_t g_rssi  		  =0;
char   packet_recieved  =0;
unsigned int   slave_RGB_value[5];

static cafe_payload_t		rx_payload;
static cafe_payload_t		tx_payload;

static cafe_event_handler_t     m_event_handler;
static cafe_config_t            m_config_local;

static  uint8_t                 m_rx_payload_buffer[CAFE_CORE_MAX_PAYLOAD_LENGTH + 2];
static  uint8_t                 m_tx_payload_buffer[CAFE_CORE_MAX_PAYLOAD_LENGTH + 10];
static volatile uint32_t        m_interrupt_flags       = 0;


char cafe_packet_recieved(void){
	
	if(packet_recieved){
		packet_recieved = false;
		return true	;
	}
	return false	;	
}

void cafe_load_payload(unsigned char slave_id, char *data,unsigned char len){

	if (tx_payload.pending) return;
	tx_payload.length  = len;
	

	if (slave_id>MAX_SLAVES_AVAILABLE)
		tx_payload.slave_id=0;
	else
		tx_payload.slave_id = slave_id;
	memcpy(tx_payload.data,data,len);

	printf("\nloaded\n" );
	tx_payload.pending =  1;

}


uint32_t cafe_get_clear_interrupts(uint32_t *interrupts)
{
	DISABLE_RF_IRQ;
	*interrupts         = m_interrupt_flags;
	m_interrupt_flags   = 0;
	ENABLE_RF_IRQ;
	return true;
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
		packet_recieved=1;
	}
	
}

static uint32_t bytewise_bit_swap(uint32_t inp)
{
	inp = (inp & 0xF0F0F0F0)  >> 4 | (inp & 0x0F0F0F0F) << 4;
	inp = (inp & 0xCCCCCCCC)  >> 2 | (inp & 0x33333333) << 2;
	return (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
}


static bool rx_fifo_push_rfbuf(uint8_t pipe)
{
	g_rssi  =  NRF_RADIO->RSSISAMPLE;
	return true;
}


int8_t cafe_get_rssi(void){
	return g_rssi;
}


static void update_payload_format(uint32_t payload_length)
{
	//#if (CAFE_CORE_MAX_PAYLOAD_LENGTH <= 32)
	NRF_RADIO->PCNF0 = (0 << RADIO_PCNF0_S0LEN_Pos) | (6 << RADIO_PCNF0_LFLEN_Pos) | (3 << RADIO_PCNF0_S1LEN_Pos);
	NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled        << RADIO_PCNF1_WHITEEN_Pos) |
	(RADIO_PCNF1_ENDIAN_Big              << RADIO_PCNF1_ENDIAN_Pos)  |
	((m_config_local.rf_addr_length - 1) << RADIO_PCNF1_BALEN_Pos)   |
	(0                                   << RADIO_PCNF1_STATLEN_Pos) |
	(CAFE_CORE_MAX_PAYLOAD_LENGTH      << RADIO_PCNF1_MAXLEN_Pos);
}




uint8_t recieved_counter=0;

void on_radio_disabled(void)
{
        //	if (!NRF_RADIO->CRCSTATUS) return;
	NRF_RADIO->SHORTS                = RADIO_SHORTS_COMMON;
	
	update_payload_format(m_config_local.payload_length);
	
	NRF_RADIO->PACKETPTR             = (uint32_t) m_rx_payload_buffer;
	NRF_RADIO->EVENTS_DISABLED       = 0;
	NRF_RADIO->TASKS_DISABLE         = 1;

	while(NRF_RADIO->EVENTS_DISABLED == 0);
	
	NRF_RADIO->EVENTS_DISABLED       = 0;
	NRF_RADIO->SHORTS                = RADIO_SHORTS_COMMON | RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->TASKS_RXEN            = 1;
	
	recieved_counter++;
	rx_fifo_push_rfbuf( NRF_RADIO->RXMATCH );
	m_interrupt_flags   |= cafe_INT_RX_DR_MSK;
	if (m_event_handler != 0) m_event_handler();
	 
}





static void update_radio_parameters()
{

	                                                                // TX power
	NRF_RADIO->TXPOWER       = m_config_local.tx_output_power   << RADIO_TXPOWER_TXPOWER_Pos;

	                                                                // RF bitrate
	NRF_RADIO->MODE          = m_config_local.bitrate           << RADIO_MODE_MODE_Pos;
	                                                                // CRC configuration    
	NRF_RADIO->CRCCNF        = m_config_local.crc               << RADIO_CRCCNF_LEN_Pos;
	NRF_RADIO->CRCINIT       = 0xFFFFUL;                            // Initial value
	NRF_RADIO->CRCPOLY       = 0x11021UL;                           // CRC poly: x^16+x^12^x^5+1
	
	                                                                // Packet format
	update_payload_format(m_config_local.payload_length);
}






 void ppi_init()
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
	m_event_handler = parameters->event_handler;
	memcpy(&m_config_local, parameters, sizeof(cafe_config_t));

	m_interrupt_flags    = 0;
	update_radio_parameters();
	ppi_init();	
	NVIC_SetPriority( RADIO_IRQn,  m_config_local.radio_irq_priority & 0x03);

	return true;
}



uint32_t cafe_disable(void)
{
	NRF_PPI->CHENCLR = (1 << CAFE_PPI_TIMER_START) | (1 << CAFE_PPI_TIMER_STOP) | (1 << CAFE_PPI_RX_TIMEOUT) | (1 << CAFE_PPI_TX_START);
	return true;
}



void cafe_start_tx_transaction(void)
{
#ifdef NO_CYPHER
	memcpy( &m_tx_payload_buffer[2], tx_payload.data, tx_payload.length );	
	
#else
	aes_encrypt_data( tx_payload.data, tx_payload.length, &m_tx_payload_buffer[2]);
#endif     
	m_tx_payload_buffer[0]=tx_payload.length+4;
	m_tx_payload_buffer[1]=1;

	NRF_RADIO->SHORTS   = RADIO_SHORTS_COMMON;
	NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
	
	
	if (tx_payload.slave_id==MAX_SLAVES_AVAILABLE) 	tx_payload.slave_id=1;
	

	NRF_RADIO->TXADDRESS   = tx_payload.slave_id++;
	NRF_RADIO->RXADDRESSES = 1 << 0;                                //current_payload->pipe;
	NRF_RADIO->FREQUENCY   = m_config_local.rf_channel;             //can be addressed individually
	
	NRF_RADIO->PACKETPTR = (uint32_t)&m_tx_payload_buffer[0];//dinamyc index instead
  
	tx_payload.pending	=0;
	
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;
	
	NRF_RADIO->TASKS_TXEN  = 1;
}


uint32_t cafe_flush_tx(void)
{
	DISABLE_RF_IRQ;
	ENABLE_RF_IRQ;
	return true;
}




void cafe_event_handler()
{
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
	NRF_RADIO->INTENCLR        = 0xFFFFFFFF;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->SHORTS          = RADIO_SHORTS_COMMON | RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->INTENSET        = RADIO_INTENSET_DISABLED_Msk;

	NRF_RADIO->RXADDRESSES     =  0X02;                             //<--set to dynamic or protocol based address
	NRF_RADIO->FREQUENCY       =  m_config_local.rf_channel;
	NRF_RADIO->PACKETPTR       =  (uint32_t) m_rx_payload_buffer;

	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS  = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_RXEN      = 1;
	return true;
}



	
void cafe_start_radio(void){

	nrf_st_address user_radio_addr;
	const char pipe_addr[8]         ={ 0x60, SLAVE_addr, 0xF4, 0xAA, 0x0E,0x0F,0x10,0x11};
	const char base_addr0[6]        ={ 0x34, 0x56, 0x78, 0x23};
	const char base_addr1[6]        ={ 0x34, 0x56, 0x78, 0x9A};
	
	memcpy (user_radio_addr.logic_pipe , pipe_addr  , 8);
	memcpy( user_radio_addr.base_addr0 , base_addr0 , 5);
	memcpy( user_radio_addr.base_addr1 , base_addr1 , 5);

#ifdef RADIO_ONLY_TRANSMITTER
	
		//tx_payload.pending=0;
		cafe_config_t cafe_config  = cafe_DEFAULT_CONFIG_TX;
		cafe_config.event_handler  = cafe_event_handler;
	    	cafe_init(&cafe_config);
	    	cafe_update_nrf_radio_address(user_radio_addr);
	
	    	if ( tx_payload.pending){
			cafe_start_tx_transaction();	
		}
          return;
	
#else
   	
   	cafe_config_t cafe_config       = cafe_DEFAULT_CONFIG_RX;
	memcpy (user_radio_addr.logic_pipe , pipe_addr  , 8);
	memcpy( user_radio_addr.base_addr0 , base_addr0 , 5);
	memcpy( user_radio_addr.base_addr1 , base_addr1 , 5);
	cafe_update_nrf_radio_address(user_radio_addr);	
	cafe_init( &cafe_config );
	cafe_start_rx();
#endif    	
 }


void cafe_start_radio_state_handle(cafe_payload_t *current_payload){
	if(current_payload->noack){
		current_payload->pending = true;
		//update reception address!
		cafe_start_rx();
	}

}

void cafe_update_nrf_radio_address(nrf_st_address radio_addr){
	
	NRF_RADIO->PREFIX0 = bytewise_bit_swap(   radio_addr.logic_pipe[3]  <<  24  |   radio_addr.logic_pipe[2]  <<  16 |   radio_addr.logic_pipe[1]  << 8   |  radio_addr.logic_pipe[0]);
	NRF_RADIO->PREFIX1 = bytewise_bit_swap(   radio_addr.logic_pipe[7]  <<  24  |   radio_addr.logic_pipe[6]  <<  16 |   radio_addr.logic_pipe[5]  << 8   |  radio_addr.logic_pipe[4]);
	NRF_RADIO->BASE0   = bytewise_bit_swap(   radio_addr.base_addr0[0] << 24 |  radio_addr.base_addr0[1] << 16  |  radio_addr.base_addr0[2] << 8 | radio_addr.base_addr0[3]);
	NRF_RADIO->BASE1   = bytewise_bit_swap(   radio_addr.base_addr1[0] << 24 |  radio_addr.base_addr1[1] << 16  |  radio_addr.base_addr1[2] << 8 | radio_addr.base_addr1[3]);
}





char cafe_get_rx_payload(char *out_buffer){
	memcpy(  out_buffer,   m_rx_payload_buffer,  CAFE_CORE_MAX_PAYLOAD_LENGTH);
	return CAFE_CORE_MAX_PAYLOAD_LENGTH;
}



#if 0
uint32_t cafe_disable(void)
{
	NRF_PPI->CHENCLR = (1 << cafe_PPI_TIMER_START) 
			 | (1 << cafe_PPI_TIMER_STOP) 
			 | (1 << cafe_PPI_RX_TIMEOUT) 
			 | (1 << cafe_PPI_TX_START);
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
		
	      // Call the correct on_radio_end function, depending on the current protocol state
	      //	on_radio_end();
		
	}

	if (NRF_RADIO->EVENTS_DISABLED && (NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk)){
		NRF_RADIO->EVENTS_DISABLED = 0; 

		if (m_config_local.mode == I_AM_TRANSMITTER){
			m_interrupt_flags |= 0X01;
			if (m_event_handler != 0) m_event_handler();
			
			if ((tx_payload.pending))
	   			cafe_start_tx_transaction();//not used by timeslot
			return;
		}
		
		on_radio_disabled();
		nrf_gpio_pin_toggle(LED_GREEN);
		
	}


}







