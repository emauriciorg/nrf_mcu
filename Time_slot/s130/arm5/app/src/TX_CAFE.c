#include "../inc/TX_CAFE.h"
#include "nrf_gpio.h"
#include <string.h>
#include <stdio.h>



// Constant parameters
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS   48   // Smallest reliable value - 43
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_1MBPS   64   // Smallest reliable value - 59
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_250KBPS 250

// Macros
#define                         DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define                         ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)

// Constant parameters
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS   48   // Smallest reliable value - 43
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_1MBPS   64   // Smallest reliable value - 59
#define                         RX_WAIT_FOR_ACK_TIMEOUT_US_250KBPS 250

// Macros
#define                         DISABLE_RF_IRQ      NVIC_DisableIRQ(RADIO_IRQn)
#define                         ENABLE_RF_IRQ       NVIC_EnableIRQ(RADIO_IRQn)

#define                         RADIO_SHORTS_COMMON ( RADIO_SHORTS_READY_START_Msk\
						    | RADIO_SHORTS_END_DISABLE_Msk  \
						    | RADIO_SHORTS_ADDRESS_RSSISTART_Msk \
						    | RADIO_SHORTS_DISABLED_RSSISTOP_Msk )

int8_t g_rssi  		  =0; //PUT INTO AN STRUCTURE
uint8_t        radio_sent =0;
unsigned int   led_state  =0;

cafe_payload_t  		rx_payload;
static cafe_event_handler_t     m_event_handler;
static cafe_config_t            m_config_local;
static cafe_payload_t           m_tx_fifo_payload[UESB_CORE_TX_FIFO_SIZE];
static cafe_payload_tx_fifo_t   m_tx_fifo;
static  uint8_t                 m_rx_payload_buffer[CAFE_CORE_MAX_PAYLOAD_LENGTH + 2];
static  uint8_t                 m_tx_payload_buffer[CAFE_CORE_MAX_PAYLOAD_LENGTH + 2];
static volatile uint32_t        m_interrupt_flags       = 0;



// These function pointers are changed dynamically, depending on protocol configuration and state

static void (*update_rf_payload_format)(uint32_t payload_length) = 0;

uint32_t cafe_read_rx_payload(cafe_payload_t *payload);

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
		led_state=1;
	}
	
}




// Function that swaps the bits within each byte in a uint32. Used to convert from nRF24L type addressing to nRF51 type addressing
static uint32_t bytewise_bit_swap(uint32_t inp)
{
	inp = (inp & 0xF0F0F0F0)  >> 4 | (inp & 0x0F0F0F0F) << 4;
	inp = (inp & 0xCCCCCCCC)  >> 2 | (inp & 0x33333333) << 2;
	return (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
}




/*CUSTOM PROTOCOL HANDLING*/
static bool rx_fifo_push_rfbuf(uint8_t pipe)
{
	g_rssi  =  NRF_RADIO->RSSISAMPLE;
	return true;
}

uint8_t recieved_counter=0;
 void on_radio_disabled(void)
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
	
	recieved_counter++;
	rx_fifo_push_rfbuf( NRF_RADIO->RXMATCH );
	m_interrupt_flags   |= cafe_INT_RX_DR_MSK;
	if (m_event_handler != 0) m_event_handler();
	 
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


static void update_radio_parameters()
{
	// Protocol
	update_rf_payload_format = update_payload_format;
	// TX power
	NRF_RADIO->TXPOWER       = m_config_local.tx_output_power   << RADIO_TXPOWER_TXPOWER_Pos;

	// RF bitrate
	NRF_RADIO->MODE          = m_config_local.bitrate           << RADIO_MODE_MODE_Pos;
	// CRC configuration    
	NRF_RADIO->CRCCNF        = m_config_local.crc               << RADIO_CRCCNF_LEN_Pos;
	NRF_RADIO->CRCINIT       = 0xFFFFUL;      // Initial value
	NRF_RADIO->CRCPOLY       = 0x11021UL;     // CRC poly: x^16+x^12^x^5+1
	

	// Packet format
	update_rf_payload_format(m_config_local.payload_length);

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
	//initialize_fifos();	
	ppi_init();	
	NVIC_SetPriority( RADIO_IRQn,  m_config_local.radio_irq_priority & 0x03);

	return true;
}

uint32_t uesb_disable(void)
{
	NRF_PPI->CHENCLR = (1 << UESB_PPI_TIMER_START) | (1 << UESB_PPI_TIMER_STOP) | (1 << UESB_PPI_RX_TIMEOUT) | (1 << UESB_PPI_TX_START);
	return true;
}

 void start_tx_transaction(void)
{
	static int packet_counter=0;
	static int ifg_change_addr=0x01;

	NRF_RADIO->SHORTS   = RADIO_SHORTS_COMMON;
	NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
	 
	if (ifg_change_addr==4)
	 	ifg_change_addr=1;
	else	 
	 	ifg_change_addr++;
	 
	NRF_RADIO->TXADDRESS   = ifg_change_addr;
	NRF_RADIO->RXADDRESSES = 1 << 0;//current_payload->pipe;
	NRF_RADIO->FREQUENCY   = m_config_local.rf_channel;
	
	uint16_t len;
	len=sprintf((char *)&m_tx_payload_buffer[2],"nrf_%0d, %02d ",ifg_change_addr,packet_counter++);
	memset(&m_tx_payload_buffer[len+2],'O',32-(len+3));
	m_tx_payload_buffer[0]=32;
	m_tx_payload_buffer[1]=1;
	NRF_RADIO->PACKETPTR = (uint32_t)&m_tx_payload_buffer[0];

	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;
	
	NRF_RADIO->TASKS_TXEN  = 1;
	
}

static uint32_t write_tx_payload(cafe_payload_t *payload) // ~50us @ 61 bytes SB
{
	DISABLE_RF_IRQ;	
	memcpy(m_tx_fifo.payload_ptr[0], payload, sizeof(cafe_payload_t));
	ENABLE_RF_IRQ;
	start_tx_transaction();
	return true;
}

uint32_t uesb_write_tx_payload(cafe_payload_t *payload)
{
	return write_tx_payload(payload);
}


uint32_t uesb_flush_tx(void)
{
	DISABLE_RF_IRQ;
	m_tx_fifo.count = 0;
	ENABLE_RF_IRQ;
	return true;
}




void uesb_event_handler()
{
    static uint32_t rf_interrupts;    
    cafe_get_clear_interrupts(&rf_interrupts);
    
    if(rf_interrupts & UESB_INT_TX_SUCCESS_MSK){   
    }
    
    if(rf_interrupts & UESB_INT_TX_FAILED_MSK){
    }
    
    if(rf_interrupts & UESB_INT_RX_DR_MSK){
    
    }
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

void self_TX_CAFE_configuration(uint8_t transeciever_mode){


	//SLAVE_addr - 0XAA;
   	nrf_st_address user_radio_addr;
	const char pipe_addr[8]         = {0x60, SLAVE_addr, 0xF4, 0x12, 0x0E,0x0F,0x10,0x11};
//	const char pipe_addr[8]         = {0x66, SLAVE_addr, 0x23, 0x66, 0x0E,0x0F,0x10,0x11};

	const char base_addr0[6]={ 0x34, 0x56, 0x78, 0x23};
	const char base_addr1[6]={ 0x34, 0x56, 0x78, 0x9A};
	
	memcpy (user_radio_addr.logic_pipe , pipe_addr  , 8);
	memcpy( user_radio_addr.base_addr0 , base_addr0 , 5);
	memcpy( user_radio_addr.base_addr1 , base_addr1 , 5);
	
	if (transeciever_mode){

		cafe_config_t cafe_config       = cafe_DEFAULT_CONFIG_TX;
		cafe_config.event_handler         = uesb_event_handler;
	    	cafe_init(&cafe_config);
	    	update_nrf_radio_address(user_radio_addr);
	
	    		if ( radio_sent){
		//		start_tx_transaction();
			radio_sent=0;
		}

	}else{
		/*USER DEFINED ADDR*/
   		cafe_config_t cafe_config       = cafe_DEFAULT_CONFIG_RX;
		memcpy (user_radio_addr.logic_pipe,pipe_addr,8);
		memcpy( user_radio_addr.base_addr0 , base_addr0, 5);
		memcpy( user_radio_addr.base_addr1 , base_addr1 , 5);
		update_nrf_radio_address(user_radio_addr);	
		cafe_init( &cafe_config );
		cafe_start_rx();
    	}
  
    
  }


int8_t get_rssi(void){
	return g_rssi;
}

void update_nrf_radio_address(nrf_st_address radio_addr){
	
	NRF_RADIO->PREFIX0 = bytewise_bit_swap(   radio_addr.logic_pipe[3]  <<  24  |   radio_addr.logic_pipe[2]  <<  16 |   radio_addr.logic_pipe[1]  << 8   |  radio_addr.logic_pipe[0]);
	NRF_RADIO->PREFIX1 = bytewise_bit_swap(   radio_addr.logic_pipe[7]  <<  24  |   radio_addr.logic_pipe[6]  <<  16 |   radio_addr.logic_pipe[5]  << 8   |  radio_addr.logic_pipe[4]);
	NRF_RADIO->BASE0   = bytewise_bit_swap(   radio_addr.base_addr0[0] << 24 |  radio_addr.base_addr0[1] << 16  |  radio_addr.base_addr0[2] << 8 | radio_addr.base_addr0[3]);
	NRF_RADIO->BASE1   = bytewise_bit_swap(   radio_addr.base_addr1[0] << 24 |  radio_addr.base_addr1[1] << 16  |  radio_addr.base_addr1[2] << 8 | radio_addr.base_addr1[3]);
}

void get_rx_payload(uint8_t *out_buffer){
	memcpy(  out_buffer,   m_rx_payload_buffer,  CAFE_CORE_MAX_PAYLOAD_LENGTH);
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
		radio_sent=1;
		// Call the correct on_radio_end function, depending on the current protocol state
		//	on_radio_end();
		
	}

	if (NRF_RADIO->EVENTS_DISABLED && (NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk)){
		NRF_RADIO->EVENTS_DISABLED = 0; 

		if (m_config_local.mode==I_AM_TRANSMITTER){
			m_interrupt_flags |= 0X01;
			if (m_event_handler != 0) m_event_handler();
			start_tx_transaction();
		}
		else{
			on_radio_disabled();
			nrf_gpio_pin_toggle(10);
		}
	}


}







