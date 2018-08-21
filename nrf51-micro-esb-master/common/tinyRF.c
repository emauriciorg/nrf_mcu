#include "tinyRF.h"
#include <string.h>
//retake on : follow program flow before programming
static  uint8_t                 payload_buffer[32 + 2];
static  uint8_t                 data_to_send[32];

static tiny_rf_config_t            tiny_rf_config;



// Hard coded parameters - change if necessary
#define     tinytx_CORE_MAX_PAYLOAD_LENGTH    32
#define     tinytx_CORE_TX_FIFO_SIZE          8
#define     tinytx_CORE_RX_FIFO_SIZE          8

#define     tinytx_SYS_TIMER                  NRF_TIMER2
#define     tinytx_SYS_TIMER_IRQ_Handler      TIMER2_IRQHandler

#define     tinytx_PPI_TIMER_START            4
#define     tinytx_PPI_TIMER_STOP             5
#define     tinytx_PPI_RX_TIMEOUT             6
#define     tinytx_PPI_TX_START               7

// Interrupt flags
#define     tinytx_INT_TX_SUCCESS_MSK         0x01
#define     tinytx_INT_TX_FAILED_MSK          0x02
#define     tinytx_INT_RX_DR_MSK              0x04

#define     tinytx_
void tiny_tx_transaction(tiny_payload_t*  tx_payload)
{
    
   
    
    payload_buffer[0] = 32;

    memcpy(&payload_buffer[2], tx_payload, 32);
    
    NRF_RADIO->SHORTS   = (RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk | 
                                                      RADIO_SHORTS_ADDRESS_RSSISTART_Msk | RADIO_SHORTS_DISABLED_RSSISTOP_Msk );
   
    NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Msk;// interrupt enabled
    
//    m_tinytx_mainstate    = tinytx_STATE_PTX_TX;
    
    NRF_RADIO->TXADDRESS = 0;
    NRF_RADIO->RXADDRESSES = 1 << 0;

    NRF_RADIO->FREQUENCY = 2;

    NRF_RADIO->PACKETPTR = (uint32_t)payload_buffer;

    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);

    NRF_RADIO->EVENTS_ADDRESS = NRF_RADIO->EVENTS_PAYLOAD = NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_TXEN  = 1;
}


   

void RADIO_IRQHandler2()
{
    if(NRF_RADIO->EVENTS_READY && (NRF_RADIO->INTENSET & RADIO_INTENSET_READY_Msk))
    {
        NRF_RADIO->EVENTS_READY = 0;

    }

    if(NRF_RADIO->EVENTS_END && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk))
    {
        NRF_RADIO->EVENTS_END = 0;
    }

    if(NRF_RADIO->EVENTS_DISABLED && (NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk))
    {
        NRF_RADIO->EVENTS_DISABLED = 0;
      //     on_radio_disabled_esb_dpl_tx_tiny();
    }

}
static void ppi_init()
{
    NRF_PPI->CH[tinytx_PPI_TIMER_START].EEP = (uint32_t)&NRF_RADIO->EVENTS_READY;
    NRF_PPI->CH[tinytx_PPI_TIMER_START].TEP = (uint32_t)&tinytx_SYS_TIMER->TASKS_START;
    NRF_PPI->CH[tinytx_PPI_TIMER_STOP].EEP =  (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
    NRF_PPI->CH[tinytx_PPI_TIMER_STOP].TEP =  (uint32_t)&tinytx_SYS_TIMER->TASKS_STOP;
    NRF_PPI->CH[tinytx_PPI_RX_TIMEOUT].EEP = (uint32_t)&tinytx_SYS_TIMER->EVENTS_COMPARE[0];
    NRF_PPI->CH[tinytx_PPI_RX_TIMEOUT].TEP = (uint32_t)&NRF_RADIO->TASKS_DISABLE;
    NRF_PPI->CH[tinytx_PPI_TX_START].EEP = (uint32_t)&tinytx_SYS_TIMER->EVENTS_COMPARE[1];
    NRF_PPI->CH[tinytx_PPI_TX_START].TEP = (uint32_t)&NRF_RADIO->TASKS_TXEN;
}
void tiny_event_handler_tx(void)
{
	static uint32_t rf_interrupts;
	//static uint32_t tx_attempts;

	//tinytx_get_clear_interrupts(&rf_interrupts);

	if(rf_interrupts & tinytx_INT_TX_SUCCESS_MSK)
	{   
	}

	if(rf_interrupts & tinytx_INT_TX_FAILED_MSK)
	{
	//	tinytx_flush_tx();
	}

	if(rf_interrupts & tinytx_INT_RX_DR_MSK)
	{
		//tinytx_read_rx_payload(&rx_payload);
		// NRF_GPIO->OUTCLR = 0xFUL << 8;
		// NRF_GPIO->OUTSET = (uint32_t)((rx_payload.data[2] & 0x0F) << 8);
	}

	// tinytx_get_tx_attempts(&tx_attempts);
	// NRF_GPIO->OUTCLR = 0xFUL << 12;
	// NRF_GPIO->OUTSET = (tx_attempts & 0x0F) << 12;
}



uint32_t tiny_init(tiny_payload_t*  tx_payload)
{
  
    tiny_rf_config_t tiny_rf_config       = TYNY_RF_DEFAULT_CONFIG;
    tiny_rf_config.event_handler       = tiny_event_handler_tx;


    uint8_t rx_addr_p0[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    uint8_t rx_addr_p1[] = {0xBA, 0xDE, 0xF0, 0x12, 0x2C};
    uint8_t rx_addr_p2   = 0x66;

    load_tiny_radio_parameters();
    ppi_init();

    NVIC_SetPriority(RADIO_IRQn, tiny_rf_config.radio_irq_priority & 0x03);


    tiny_set_address(tinytx_ADDRESS_PIPE0, rx_addr_p0);
    tiny_set_address(tinytx_ADDRESS_PIPE1, rx_addr_p1);
    tiny_set_address(tinytx_ADDRESS_PIPE2, &rx_addr_p2);

    tx_payload->length  = 8;
    tx_payload->pipe    = 0;
    tx_payload->data[0] = 0x01;
    tx_payload->data[1] = 0x00;
    tx_payload->data[2] = 0x00;
    tx_payload->data[3] = 0x00;
    tx_payload->data[4] = 0x11;


  //  tiny_start_rx();

    return 0;
}


// Function that swaps the bits within each byte in a uint32. Used to convert from nRF24L type addressing to nRF51 type addressing
static uint32_t bytewise_bit_swap(uint32_t inp)
{
    inp = (inp & 0xF0F0F0F0) >> 4 | (inp & 0x0F0F0F0F) << 4;
    inp = (inp & 0xCCCCCCCC) >> 2 | (inp & 0x33333333) << 2;
    return (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
}


static void load_tiny_radio_parameters()
{

    NRF_RADIO->TXPOWER   = tiny_rf_config.tx_output_power   << RADIO_TXPOWER_TXPOWER_Pos;

    // RF bitrate
    NRF_RADIO->MODE      = tiny_rf_config.bitrate           << RADIO_MODE_MODE_Pos;

//    m_wait_for_ack_timeout_us = RX_WAIT_FOR_ACK_TIMEOUT_US_2MBPS;
    

    // CRC configuration
    NRF_RADIO->CRCCNF    = tiny_rf_config.crc               << RADIO_CRCCNF_LEN_Pos;
    NRF_RADIO->CRCINIT = 0xFFFFUL;      // Initial value
    NRF_RADIO->CRCPOLY = 0x11021UL;     // CRC poly: x^16+x^12^x^5+1

    // Packet format 
	NRF_RADIO->PCNF0 = (0 << RADIO_PCNF0_S0LEN_Pos) | (6 << RADIO_PCNF0_LFLEN_Pos) | (3 << RADIO_PCNF0_S1LEN_Pos);
    
    NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled        << RADIO_PCNF1_WHITEEN_Pos) |
                       (RADIO_PCNF1_ENDIAN_Big              << RADIO_PCNF1_ENDIAN_Pos)  |
                       ((tiny_rf_config.rf_addr_length - 1) << RADIO_PCNF1_BALEN_Pos)   |
                       (0                                   << RADIO_PCNF1_STATLEN_Pos) |
                       (tinytx_CORE_MAX_PAYLOAD_LENGTH        << RADIO_PCNF1_MAXLEN_Pos);
    // Radio address config
    NRF_RADIO->PREFIX0 = bytewise_bit_swap(tiny_rf_config.rx_address_p3 << 24 | tiny_rf_config.rx_address_p2 << 16 | tiny_rf_config.rx_address_p1[0] << 8 | tiny_rf_config.rx_address_p0[0]);
    NRF_RADIO->PREFIX1 = bytewise_bit_swap(tiny_rf_config.rx_address_p7 << 24 | tiny_rf_config.rx_address_p6 << 16 | tiny_rf_config.rx_address_p5 << 8 | tiny_rf_config.rx_address_p4);
    NRF_RADIO->BASE0   = bytewise_bit_swap(tiny_rf_config.rx_address_p0[1] << 24 | tiny_rf_config.rx_address_p0[2] << 16 | tiny_rf_config.rx_address_p0[3] << 8 | tiny_rf_config.rx_address_p0[4]);
    NRF_RADIO->BASE1   = bytewise_bit_swap(tiny_rf_config.rx_address_p1[1] << 24 | tiny_rf_config.rx_address_p1[2] << 16 | tiny_rf_config.rx_address_p1[3] << 8 | tiny_rf_config.rx_address_p1[4]);
}




uint32_t tiny_set_address(tinytx_address_type_t address, const uint8_t *data_ptr)
{
    switch(address)
    {
        case tinytx_ADDRESS_PIPE0:
            memcpy(tiny_rf_config.rx_address_p0, data_ptr, tiny_rf_config.rf_addr_length);
            break;
        case tinytx_ADDRESS_PIPE1:
            memcpy(tiny_rf_config.rx_address_p1, data_ptr, tiny_rf_config.rf_addr_length);
            break;
        case tinytx_ADDRESS_PIPE2:
            tiny_rf_config.rx_address_p2 = *data_ptr;
            break;
        case tinytx_ADDRESS_PIPE3:
            tiny_rf_config.rx_address_p3 = *data_ptr;
            break;
        case tinytx_ADDRESS_PIPE4:
            tiny_rf_config.rx_address_p4 = *data_ptr;
            break;
        case tinytx_ADDRESS_PIPE5:
            tiny_rf_config.rx_address_p5 = *data_ptr;
            break;
        case tinytx_ADDRESS_PIPE6:
            tiny_rf_config.rx_address_p6 = *data_ptr;
            break;
        case tinytx_ADDRESS_PIPE7:
            tiny_rf_config.rx_address_p7 = *data_ptr;
            break;
        default:
            return 1;
    }
    load_tiny_radio_parameters();
    return 0;
}
uint8_t led_state=0;
uint8_t data_counter=0;

static uesb_payload_t  rx_payload;

void uesb_event_handler_tx(void)
{
        static uint32_t rf_interrupts;
    static uint32_t tx_attempts;

    uesb_get_clear_interrupts(&rf_interrupts);

    if(rf_interrupts & UESB_INT_TX_SUCCESS_MSK)
    {   
    }

    if(rf_interrupts & UESB_INT_TX_FAILED_MSK)
    {
        uesb_flush_tx();
    }

    if(rf_interrupts & UESB_INT_RX_DR_MSK)
    {
        uesb_read_rx_payload(&rx_payload);
    }

     uesb_get_tx_attempts(&tx_attempts);

}






