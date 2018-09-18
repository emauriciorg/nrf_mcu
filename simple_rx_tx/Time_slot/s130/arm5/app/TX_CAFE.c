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

#include "TX_CAFE.h"
#include "uesb_error_codes.h"
#include "nrf_gpio.h"
#include <string.h>
#include <stdio.h>

static uesb_payload_t tx_payload;



static uesb_event_handler_t     m_event_handler;

// RF parameters
static uesb_config_t            m_config_local;

// TX FIFO
static uesb_payload_t           m_tx_fifo_payload[UESB_CORE_TX_FIFO_SIZE];
static uesb_payload_tx_fifo_t   m_tx_fifo;


static  uint8_t                 m_tx_payload_buffer[UESB_CORE_MAX_PAYLOAD_LENGTH + 2];

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

// These function pointers are changed dynamically, depending on protocol configuration and state

static void (*update_rf_payload_format)(uint32_t payload_length) = 0;

// The following functions are assigned to the function pointers above
static void on_radio_disabled_esb_dpl_tx_noack(void);

static void update_rf_payload_format_esb_dpl(uint32_t payload_length)
{
	NRF_RADIO->PCNF0 = (0 << RADIO_PCNF0_S0LEN_Pos) | (6 << RADIO_PCNF0_LFLEN_Pos) | (3 << RADIO_PCNF0_S1LEN_Pos);

	NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled        << RADIO_PCNF1_WHITEEN_Pos) |
					   (RADIO_PCNF1_ENDIAN_Big              << RADIO_PCNF1_ENDIAN_Pos)  |
					   ((m_config_local.rf_addr_length - 1) << RADIO_PCNF1_BALEN_Pos)   |
					   (0                                   << RADIO_PCNF1_STATLEN_Pos) |
					   (UESB_CORE_MAX_PAYLOAD_LENGTH        << RADIO_PCNF1_MAXLEN_Pos);
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
	// Protocol
	update_rf_payload_format = update_rf_payload_format_esb_dpl;
	// TX power
	NRF_RADIO->TXPOWER   = m_config_local.tx_output_power   << RADIO_TXPOWER_TXPOWER_Pos;

	// RF bitrate
	NRF_RADIO->MODE      = m_config_local.bitrate           << RADIO_MODE_MODE_Pos;
	
	// CRC configuration
	NRF_RADIO->CRCCNF    = m_config_local.crc               << RADIO_CRCCNF_LEN_Pos;
	NRF_RADIO->CRCINIT   = 0xFFFFUL;      // Initial value
	NRF_RADIO->CRCPOLY   = 0x11021UL;     // CRC poly: x^16+x^12^x^5+1
	

	// Packet format
	update_rf_payload_format(m_config_local.payload_length);

}

void update_nrf_radio_address(nrf_st_address radio_addr){
	
	NRF_RADIO->PREFIX0 = bytewise_bit_swap(   radio_addr.logic_pipe[3]  <<  24  |   radio_addr.logic_pipe[2]  <<  16 |   radio_addr.logic_pipe[1]  << 8   |  radio_addr.logic_pipe[0]);
	NRF_RADIO->PREFIX1 = bytewise_bit_swap(   radio_addr.logic_pipe[7]  <<  24  |   radio_addr.logic_pipe[6]  <<  16 |   radio_addr.logic_pipe[5]  << 8   |  radio_addr.logic_pipe[4]);
	NRF_RADIO->BASE0   = bytewise_bit_swap(   radio_addr.base_addr0[0] << 24 |  radio_addr.base_addr0[1] << 16  |  radio_addr.base_addr0[2] << 8 | radio_addr.base_addr0[3]);
	NRF_RADIO->BASE1   = bytewise_bit_swap(   radio_addr.base_addr1[0] << 24 |  radio_addr.base_addr1[1] << 16  |  radio_addr.base_addr1[2] << 8 | radio_addr.base_addr1[3]);
}


static void initialize_fifos()
{
	m_tx_fifo.count       = 0;
	for(int i = 0; i < UESB_CORE_TX_FIFO_SIZE; i++){
		m_tx_fifo.payload_ptr[i] = &m_tx_fifo_payload[i];
	}

}

static void tx_fifo_remove_last()
{
	if (m_tx_fifo.count > 0){
	//	DISABLE_RF_IRQ;
		m_tx_fifo.count--;
	//	ENABLE_RF_IRQ;
	}
}


static void ppi_init()
{
	NRF_PPI->CH[  UESB_PPI_TIMER_START ].EEP   =  (uint32_t)&NRF_RADIO->EVENTS_READY;
	NRF_PPI->CH[  UESB_PPI_TIMER_START ].TEP   =  (uint32_t)&UESB_SYS_TIMER->TASKS_START;
	NRF_PPI->CH[  UESB_PPI_TIMER_STOP  ].EEP   =  (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
	NRF_PPI->CH[  UESB_PPI_TIMER_STOP  ].TEP   =  (uint32_t)&UESB_SYS_TIMER->TASKS_STOP;
	NRF_PPI->CH[  UESB_PPI_RX_TIMEOUT  ].EEP   =  (uint32_t)&UESB_SYS_TIMER->EVENTS_COMPARE[0];
	NRF_PPI->CH[  UESB_PPI_RX_TIMEOUT  ].TEP   =  (uint32_t)&NRF_RADIO->TASKS_DISABLE;
	NRF_PPI->CH[  UESB_PPI_TX_START    ].EEP   =  (uint32_t)&UESB_SYS_TIMER->EVENTS_COMPARE[1];
	NRF_PPI->CH[  UESB_PPI_TX_START    ].TEP   =  (uint32_t)&NRF_RADIO->TASKS_TXEN;
}

uint32_t uesb_init(uesb_config_t *parameters)
{
	m_event_handler = parameters->event_handler;
	memcpy(&m_config_local, parameters, sizeof(uesb_config_t));

	m_interrupt_flags    = 0;
	m_last_rx_packet_crc = 0xFFFFFFFF;

	update_radio_parameters();
	initialize_fifos();	
	//ppi_init();

//	NVIC_SetPriority(RADIO_IRQn, m_config_local.radio_irq_priority & 0x03);


	return true;
}

uint32_t uesb_disable(void)
{
	NRF_PPI->CHENCLR = (1 << UESB_PPI_TIMER_START) | (1 << UESB_PPI_TIMER_STOP) | (1 << UESB_PPI_RX_TIMEOUT) | (1 << UESB_PPI_TX_START);
	return true;
}
 int packet_counter  =0;
static void start_tx_transaction()
{
	
	static int ifg_change_addr =0x01;

	NRF_RADIO->SHORTS   = RADIO_SHORTS_COMMON;
	NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Msk;
	
	 
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
	m_tx_payload_buffer[0]  = 32;
	m_tx_payload_buffer[1]  =  1;
	simple_uart_putstring (m_tx_payload_buffer);
	simple_uart_putstring("\n");
	NRF_RADIO->PACKETPTR    =  (uint32_t)&m_tx_payload_buffer[0];

//	NVIC_ClearPendingIRQ(RADIO_IRQn);
//	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->EVENTS_ADDRESS = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;
	
	NRF_RADIO->TASKS_TXEN  = 1;
}

static uint32_t write_tx_payload(uesb_payload_t *payload) // ~50us @ 61 bytes SB
{
	if (m_tx_fifo.count >= UESB_CORE_TX_FIFO_SIZE) return UESB_ERROR_TX_FIFO_FULL;

	//DISABLE_RF_IRQ;
	
	memcpy(m_tx_fifo.payload_ptr[0], payload, sizeof(uesb_payload_t));
		
	m_tx_fifo.count++;
	
	//ENABLE_RF_IRQ;
	
	start_tx_transaction();
	
	return true;
}

uint32_t uesb_write_tx_payload(uesb_payload_t *payload)
{
	return write_tx_payload(payload);
}


uint32_t uesb_start_tx()
{
	if (m_tx_fifo.count == 0) return UESB_ERROR_TX_FIFO_EMPTY;
	start_tx_transaction();
	return true;
}


uint32_t uesb_flush_tx(void)
{
	DISABLE_RF_IRQ;
	m_tx_fifo.count = 0;
	ENABLE_RF_IRQ;
	return true;
}



uint32_t uesb_get_clear_interrupts(uint32_t *interrupts)
{
//	DISABLE_RF_IRQ;
	*interrupts       = m_interrupt_flags;
	m_interrupt_flags = 0;
//	ENABLE_RF_IRQ;
	return true;
}





static void on_radio_disabled_esb_dpl_tx_noack()
{
	m_interrupt_flags |= UESB_INT_TX_SUCCESS_MSK;
//	tx_fifo_remove_last();

	if (m_event_handler != 0) m_event_handler();
	start_tx_transaction();
}



void check_for_radio_flags()
{
	if (NRF_RADIO->EVENTS_READY && (NRF_RADIO->INTENSET & RADIO_INTENSET_READY_Msk))
	{
		NRF_RADIO->EVENTS_READY = 0;

	}

	if (NRF_RADIO->EVENTS_END && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk))
	{
		NRF_RADIO->EVENTS_END = 0;



		// Call the correct on_radio_end function, depending on the current protocol state
		//	on_radio_end();
		
	}

	if (NRF_RADIO->EVENTS_DISABLED && (NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk))
	{
		NRF_RADIO->EVENTS_DISABLED = 0;

	  
		// Call the correct on_radio_disable function, depending on the current protocol state
		//if (on_radio_disabled_esb_dpl_tx_noack)
		//{
			on_radio_disabled_esb_dpl_tx_noack();
		//}
	}

}



void uesb_event_handler()
{
    static uint32_t rf_interrupts;

    
    uesb_get_clear_interrupts(&rf_interrupts);
    
    if(rf_interrupts & UESB_INT_TX_SUCCESS_MSK)
    {   
    }
    
    if(rf_interrupts & UESB_INT_TX_FAILED_MSK)
    {
      //  uesb_flush_tx();
    }
    
    if(rf_interrupts & UESB_INT_RX_DR_MSK)
    {
    
    }
    
    
}

  nrf_st_address user_radio_addr;
 uesb_config_t uesb_config       = UESB_DEFAULT_CONFIG;
void cafe_radio_configuration(void){

  
   
    uesb_init(&uesb_config);
    update_nrf_radio_address(user_radio_addr);

    tx_payload.length  = 8;
    tx_payload.pipe    = 0;
    tx_payload.data[0] = 'A';//0x01;
    tx_payload.data[1] = 0x00;
    tx_payload.data[2] = 0x00;
    tx_payload.data[3] = 0x00;
    tx_payload.data[4] = 0x11;
    
    memcpy((char *)tx_payload.data,"Pigeon  \n\0",10);
		
	
    
    uint8_t package_id=0;
    
      uesb_write_tx_payload(&tx_payload);
			
    

}

void single_cafe_radio_configuration(void){

    const char pipe_addr[8]         = {0x60, 0xF4, 0xAA, 0x12, 0x0E,0x0F,0x10,0x11};
    const char base_addr0[6]        ={ 0x34, 0x56, 0x78, 0x23};
    const char base_addr1[6]        ={ 0x34, 0x56, 0x78, 0x9A};

    memcpy (user_radio_addr.logic_pipe , pipe_addr  , 8);
    memcpy( user_radio_addr.base_addr0 , base_addr0 , 5);
    memcpy( user_radio_addr.base_addr1 , base_addr1 , 5);
    	
   
    uesb_config.rf_channel          = 5;
    uesb_config.crc                 = UESB_CRC_16BIT;
    uesb_config.retransmit_count    = 6;
    uesb_config.retransmit_delay    = 500;
    uesb_config.dynamic_ack_enabled = 0;
    uesb_config.bitrate             = UESB_BITRATE_2MBPS;
    uesb_config.event_handler       = uesb_event_handler;
    


}
