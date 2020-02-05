 #ifndef _CAFE_DATATYPE_H_
#define _CAFE_DATATYPE_H_


#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
//#include "nrf51.h"
//#include "nrf51_bitfields.h"

#include "nrf_gpio.h"


#define CUSTOM_PIPE_INDEX 0X02
#ifdef SLAVE1
	#define CUSTOM_PIPE 0xF4 
	#define SLAVE_name "S1"
#endif

#ifdef SLAVE2
	#define CUSTOM_PIPE 0xAA
	#define SLAVE_name "S2"
#endif
#ifdef SLAVE3
	#define CUSTOM_PIPE 0x12
	#define SLAVE_name "S3"
#endif



// Hard coded parameters - change if necessary
#define     USER_PACKET_OVERHEAD    3
#define     BLE_PACKET_OVERHEAD     2

#define     RADIO_PACKET_OVERHEAD   (BLE_PACKET_OVERHEAD+USER_PACKET_OVERHEAD) 

#define     RADIO_PACKET_LEN    200
#define     RADIO_RX_FIFO_LEN          8
#define     RADIO_TX_FIFO_LEN          8

/*#define     radioSYS_TIMER                  NRF_TIMER2
#define     radioSYS_TIMER_IRQ_Handler      TIMER2_IRQHandler

#define     radioPPI_TIMER_START            4
#define     radioPPI_TIMER_STOP             5
#define     radioPPI_RX_TIMEOUT             6

// Interrupt flags
#define     radioINT_RX_DR_MSK              0x04
*/


#define     CAFE_SYS_TIMER                  NRF_TIMER2
#define     CAFE_SYS_TIMER_IRQ_Handler      TIMER2_IRQHandler

#define     CAFE_PPI_TIMER_START            4
#define     CAFE_PPI_TIMER_STOP             5
#define     CAFE_PPI_RX_TIMEOUT             6
#define     CAFE_PPI_TX_START               7

// Interrupt flags
#define     CAFE_INT_TX_SUCCESS_MSK         0x01
#define     CAFE_INT_TX_FAILED_MSK          0x02
#define     CAFE_INT_RX_DR_MSK              0x04

#define     CAFE_PID_RESET_VALUE            0xFF


typedef struct  {
                 uint8_t   base_addr0_tx[5];
                 uint8_t   base_addr0 [5]  ;
                 uint8_t   base_addr1 [5]  ;
                 uint8_t   logic_pipe [8]  ;
}nrf_st_address;


typedef enum {
    RADIO_INVALID_MODE,
    RADIO_TRANSMITTER_MODE,          // Primary transmitter
    RADIO_RECEIVER_MODE           // Primary receiver
} radio_mode_t;



//nrf51_bitfields
typedef enum {
    radio2MBPS   = RADIO_MODE_MODE_Nrf_2Mbit,
    radio1MBPS   = RADIO_MODE_MODE_Nrf_1Mbit,
    radio250KBPS = RADIO_MODE_MODE_Nrf_250Kbit
} radiobitrate_t;

typedef enum {
    radioCRC_16BIT = RADIO_CRCCNF_LEN_Two,
    radioCRC_8BIT  = RADIO_CRCCNF_LEN_One,
    radioCRC_OFF   = RADIO_CRCCNF_LEN_Disabled
} radiocrc_t;

typedef enum {
    radioTX_POWER_4DBM     = RADIO_TXPOWER_TXPOWER_Pos4dBm,
    radioTX_POWER_0DBM     = RADIO_TXPOWER_TXPOWER_0dBm,
    radioTX_POWER_NEG4DBM  = RADIO_TXPOWER_TXPOWER_Neg4dBm,
    radioTX_POWER_NEG8DBM  = RADIO_TXPOWER_TXPOWER_Neg8dBm,
    radioTX_POWER_NEG12DBM = RADIO_TXPOWER_TXPOWER_Neg12dBm,
    radioTX_POWER_NEG16DBM = RADIO_TXPOWER_TXPOWER_Neg16dBm,
    radioTX_POWER_NEG20DBM = RADIO_TXPOWER_TXPOWER_Neg20dBm,
    radioTX_POWER_NEG30DBM = RADIO_TXPOWER_TXPOWER_Neg30dBm
} radiotx_power_t;

typedef enum {
    radioTXMODE_AUTO,        // Automatic TX mode - When the TX fifo is non-empty and the radio is idle packets will be sent automatically.
    radioTXMODE_MANUAL,      // Manual TX mode - Packets will not be sent until radio_start_tx() is called. Can be used to ensure consistent packet timing.
    radioTXMODE_MANUAL_START // Manual start TX mode - Packets will not be sent until radio_start_tx() is called, but transmission will continue automatically until the TX FIFO is empty.
} radiotx_mode_t;

typedef void (*radio_event_handler_t)(void);

// Main CAFE configuration struct, contains all radio parameters
typedef struct
{
   
    radio_mode_t             mode;
    radio_event_handler_t    event_handler;

    // General RF parameters
    radiobitrate_t          bitrate;
    radiocrc_t              crc;
    uint8_t                 rf_channel;
    uint8_t                 payload_length;
    uint8_t                 rf_addr_length;

    radiotx_power_t         tx_output_power;
    nrf_st_address          radio_addresses;


    // Control settings
    radiotx_mode_t          tx_mode;

    uint8_t                 radio_irq_priority;
}radioconfig_t;

#define CAFE_DEFAULT_CONFIG {.mode                  = RADIO_INVALID_MODE,                    \
                             .event_handler         = 0,              \
                             .rf_channel            = 5,                                \
                             .payload_length        = RADIO_PACKET_LEN,     \
                             .rf_addr_length        = 5,                                \
                             .bitrate               = radio2MBPS,               \
                             .crc                   = radioCRC_16BIT,                   \
                             .tx_output_power       = radioTX_POWER_0DBM,               \
                             .tx_mode               = radioTXMODE_AUTO,                 \
                             .radio_irq_priority    = 1}



#define CAFE_DEFAULT_RX_CONFIG {.mode               = RADIO_RECEIVER_MODE,                    \
                             .event_handler         = radio_start_rx_transaction,              \
                             .rf_channel            = 5,                                \
                             .payload_length        = RADIO_PACKET_LEN,     \
                             .rf_addr_length        = 5,                                \
                             .bitrate               = radio2MBPS,               \
                             .crc                   = radioCRC_16BIT,                   \
                             .tx_output_power       = radioTX_POWER_0DBM,               \
                             .tx_mode               = radioTXMODE_AUTO,                 \
                             .radio_irq_priority    = 1}



// see to nRF24L default parameters 
#define CAFE_DEFAULT_TX_CONFIG {.mode               = RADIO_TRANSMITTER_MODE,                    \
                             .event_handler         = radio_start_tx_transaction,                                \
                             .rf_channel            = 5,                                \
                             .payload_length        = RADIO_PACKET_LEN,     \
                             .rf_addr_length        = 5,                                \
                             .bitrate               = radio2MBPS,               \
                             .crc                   = radioCRC_16BIT,                   \
                             .tx_output_power       = radioTX_POWER_0DBM,               \
                             .tx_mode               = radioTXMODE_AUTO,                 \
                             .radio_irq_priority    = 1}
	


typedef union{
	struct{
		uint8_t length;                   /*Packet length*/
		uint8_t S1;                       /*Requiered field by HW*/
		uint8_t ack;                      /*Response ack requiered*/
		uint8_t crc;                      /*Optional crc */
		uint8_t address;                  /*Logical address that goes into the packet*/
		uint8_t payload[RADIO_PACKET_LEN];/*Aplicaction arguments /data*/
	} formated;
	char raw[ RADIO_PACKET_LEN + RADIO_PACKET_OVERHEAD ];
} radio_payload_t;


typedef struct{
    uint8_t pipe; /*HW radio address*/
    int8_t  rssi; /*Received Signal Strength Indicato*/
    union{
        uint8_t pending;  /*transmission packet pending to sent*/
        uint8_t available;/*reception packet pending to read*/     
    } state;
    radio_payload_t  data; /*rx,tx payload*/
} radio_packet_t;


#endif /*_CAFE_DATATYPE_H_*/
