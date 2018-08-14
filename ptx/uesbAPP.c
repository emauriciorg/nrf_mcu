#include "uesbAPP.h"

#include "nrf.h"
#include "micro_esb.h"
#include "uesb_error_codes.h"
#include "nrf_delay.h"


void uesb_event_handler()
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
		NRF_GPIO->OUTCLR = 0xFUL << 8;
		NRF_GPIO->OUTSET = (uint32_t)((rx_payload.data[2] & 0x0F) << 8);
	}

	uesb_get_tx_attempts(&tx_attempts);
	NRF_GPIO->OUTCLR = 0xFUL << 12;
	NRF_GPIO->OUTSET = (tx_attempts & 0x0F) << 12;
}



void uesb_setup(uesb_payload_t *tx_payload){

	uint8_t rx_addr_p0[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
	uint8_t rx_addr_p1[] = {0xBA, 0xDE, 0xF0, 0x12, 0x2C};

	uint8_t rx_addr_p2   = 0x66;

	uesb_config_t uesb_config       = UESB_DEFAULT_CONFIG;
	uesb_config.rf_channel          = 5;
	uesb_config.crc                 = UESB_CRC_16BIT;
	uesb_config.retransmit_count    = 6;
	uesb_config.retransmit_delay    = 500;
	uesb_config.dynamic_ack_enabled = 0;
	uesb_config.protocol            = UESB_PROTOCOL_ESB_DPL;
	uesb_config.bitrate             = UESB_BITRATE_2MBPS;
	uesb_config.event_handler       = uesb_event_handler;

	uesb_init(&uesb_config);

	uesb_set_address(UESB_ADDRESS_PIPE0, rx_addr_p0);
	uesb_set_address(UESB_ADDRESS_PIPE1, rx_addr_p1);
	uesb_set_address(UESB_ADDRESS_PIPE2, &rx_addr_p2);

	tx_payload.length  = 8;
	tx_payload.pipe    = 0;
	tx_payload.data[0] = 0x01;
	tx_payload.data[1] = 0x00;
	tx_payload.data[2] = 0x00;
	tx_payload.data[3] = 0x00;
	tx_payload.data[4] = 0x11;


}

