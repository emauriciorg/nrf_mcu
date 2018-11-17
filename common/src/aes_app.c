/** 
******************************************************************************
* \file    aes_app.c
* \brief   Encription routines .
******************************************************************************
*/
#include "../inc/aes_app.h"

#include <string.h>
#include <stdio.h>

#include "../inc/common_structs.h"
#include "../inc/command_list.h"
#include "nrf_gpio.h"
#include "../inc/bbn_board.h"

/**TO DO:  ota keying and bigger block encryption
 * 
 */

//#define DEBUG_CLI_AES
#ifdef DEBUG_CLI_AES
	#include "app_uart.h"

	#define AES_OUT(...)  printf(__VA_ARGS__)
#else
	#define AES_OUT(...)
#endif



#define CCM_KEY_SIZE          16
#define CCM_IV_SIZE            8
#define CCM_MIC_SIZE           4
#define CCM_SCRATCH_ARE_SIZE  60
#define DATA_SIZE	      60
#define CCM_IN_HEADER_INDEX    0
#define CCM_IN_LENGTH_INDEX    1
#define CCM_IN_RFU_INDEX       2
#define CCM_IN_PAYLOAD_INDEX   3


typedef struct{
    uint8_t  key[CCM_KEY_SIZE];
    uint64_t counter;
    uint8_t  direction;
    uint8_t  iv[CCM_IV_SIZE];  
}st_ccm_data;

typedef struct {
	char generate_key:1,
	     cypher      :1,
	     random      :1,
	     init        :1;
}st_cypher_op;


static uint8_t  aes_key_holder[CCM_SCRATCH_ARE_SIZE];

static st_ccm_data  ccm_data;


void aes_ccm_rng_fill_buffer(uint8_t *buf, uint32_t bufsize)
{
    NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Enabled << RNG_CONFIG_DERCEN_Pos;
    while (bufsize--){

        NRF_RNG->EVENTS_VALRDY = 0;
        NRF_RNG->TASKS_START   = 1;
        while (NRF_RNG->EVENTS_VALRDY == 0);
        *buf++ = NRF_RNG->VALUE;
    
    }
}




uint8_t aes_decrypt_data(uint8_t *src_data, uint8_t  length, uint8_t *out_data){
	
	AES_OUT("\n\n________Decript starts_________________\n");
 	
	memset(&ccm_data, 0, sizeof(st_ccm_data));

 
#ifdef DEBUG_CLI_AES
	char i;
		AES_OUT("INPUT data :[");
 	for (i=0;i<src_data[CCM_IN_LENGTH_INDEX];i++)
                      AES_OUT("%d ", src_data[CCM_IN_PAYLOAD_INDEX+i]); 	
	AES_OUT("]\n");
#endif	
	//key setup
 	NRF_CCM->ENABLE = CCM_ENABLE_ENABLE_Enabled << CCM_ENABLE_ENABLE_Pos;
    	NRF_CCM->SCRATCHPTR = (uint32_t)aes_key_holder;

    	//pointer assignations
	NRF_CCM->CNFPTR     = (uint32_t) &ccm_data;
	NRF_CCM->INPTR      = (uint32_t) src_data;
	NRF_CCM->OUTPTR     = (uint32_t) out_data;

	NRF_CCM->MODE               = 1<<0; //mode : encrypt :0 , decrypt :1
    	NRF_CCM->EVENTS_ENDKSGEN    = 0;
    	NRF_CCM->EVENTS_ERROR       = 0;
    	NRF_CCM->TASKS_KSGEN        = 1;

    	while (NRF_CCM->EVENTS_ENDKSGEN == 0);
    	

    	if (NRF_CCM->EVENTS_ERROR) {
    		AES_OUT("CCM->EVENTS_ENDKSGEN\n");
		return false;
	}
	
	AES_OUT("\nAES KEY:\n[");

#ifdef DEBUG_CLI_AES
	for (i=0;i<CCM_SCRATCH_ARE_SIZE;i++)  AES_OUT("%d ", aes_key_holder[i]);	
	AES_OUT("]\n");	
#endif
	NRF_CCM->EVENTS_ENDCRYPT      = 0;
	NRF_CCM->EVENTS_ERROR         = 0;
	NRF_CCM->TASKS_CRYPT          = 1;
	
	while (NRF_CCM->EVENTS_ENDCRYPT == 0);


	if (NRF_CCM->EVENTS_ERROR){
		AES_OUT("\nCCM->EVENT ERROR\n");
		return false;
	}


	if (NRF_CCM->MICSTATUS == (CCM_MICSTATUS_MICSTATUS_CheckFailed << CCM_MICSTATUS_MICSTATUS_Pos)){
		AES_OUT("\nCCM->MICSTATUS  ERROR\n");
    	 	return false;
    	}
	
	AES_OUT("\ndecrypt result: \n hd %d ,len %d \n[",out_data[CCM_IN_HEADER_INDEX] ,out_data[CCM_IN_LENGTH_INDEX] );
	
#ifdef DEBUG_CLI_AES
	
	for (i=0;i<out_data[CCM_IN_LENGTH_INDEX];i++){
		AES_OUT("%d ", out_data[CCM_IN_PAYLOAD_INDEX+i]);
     	}
     	AES_OUT("\n");
	
	for (i=0;i<out_data[CCM_IN_LENGTH_INDEX];i++) {
		AES_OUT("%c", out_data [CCM_IN_PAYLOAD_INDEX+i]);
	}
#endif
	
	return true;
}


uint8_t aes_encrypt_data( uint8_t *input_data, uint8_t packet_length, uint8_t *out_data)
{
	char  i;
	uint8_t local_packet_length=0;
	uint8_t src_data[ CCM_IN_PAYLOAD_INDEX  +  DATA_SIZE ];

	memset(src_data      , 0 , sizeof(src_data)    );
	memset(aes_key_holder, 0 , CCM_SCRATCH_ARE_SIZE);

// 	if (!cypher_op->init){
 		memset(&ccm_data, 0, sizeof(st_ccm_data));
// 		cypher_op->init=1;
//	}
 	
	//default key is zero 	
 	//random pre-key generation 	
        ccm_data.counter=0;
#if 0  //uncomment to generatre random key, 
 	AES_OUT("\nKEY SEED MODE: ");

 	if (cypher_op->random){
 		AES_OUT("rng random \n\n");
 		ccm_rng_fill_buffer(ccm_data.key, CCM_KEY_SIZE);
		AES_OUT("KEY SEED : ");

 		for(i=0; i<CCM_KEY_SIZE;i++) AES_OUT("%d ",ccm_data.key[i]);
 	        AES_OUT("\nIV SEED  : ");
 		ccm_rng_fill_buffer(ccm_data.iv,  CCM_IV_SIZE);
 		
 		for(i=0; i<CCM_IV_SIZE;i++)  AES_OUT("%d ",ccm_data.iv[i]);

	}else{
 		AES_OUT("fixed(zeroed)\n");
	}
	AES_OUT("\n");

#endif

	//key setup
 	NRF_CCM->ENABLE     = CCM_ENABLE_ENABLE_Enabled << CCM_ENABLE_ENABLE_Pos;    
    	NRF_CCM->SCRATCHPTR = (uint32_t)aes_key_holder;
    	//pointer assignations
	NRF_CCM->CNFPTR     = (uint32_t) &ccm_data;
	NRF_CCM->INPTR      = (uint32_t) src_data ;
	NRF_CCM->OUTPTR     = (uint32_t) out_data ;

	NRF_CCM->MODE = 0<<0; 
    	NRF_CCM->EVENTS_ENDKSGEN = 0;
    	NRF_CCM->EVENTS_ERROR    = 0;
    	NRF_CCM->TASKS_KSGEN     = 1;
    	
    	while (NRF_CCM->EVENTS_ENDKSGEN == 0);
    	
    	if (NRF_CCM->EVENTS_ERROR) {
    		AES_OUT("CCM->EVENT ERROR\n");
		return false;
	}

	AES_OUT("\nAES KEY:\n[");
	
	for (i=0;i<CCM_SCRATCH_ARE_SIZE;i++){
                AES_OUT("%d ", aes_key_holder[i]);	
	}
	AES_OUT("]\n");	
	
 		
	if (packet_length >27){
 		local_packet_length = 27;
		packet_length=packet_length-27;
	}
	else {
 		local_packet_length = packet_length;
 		packet_length=0;
	}

	memcpy(&src_data[CCM_IN_PAYLOAD_INDEX], input_data, local_packet_length);	
	src_data[ CCM_IN_HEADER_INDEX ] = 0;
	src_data[ CCM_IN_LENGTH_INDEX ] = (uint8_t)local_packet_length;
	src_data[ CCM_IN_RFU_INDEX    ] = 0;

#ifdef DEBUG_CLI_AES
	AES_OUT("\nINPUT DATA : [%d] [%s]\n\n",local_packet_length,&src_data[CCM_IN_PAYLOAD_INDEX]);
	for (i=0;i<src_data[CCM_IN_LENGTH_INDEX];i++)
                      AES_OUT("%d ", src_data[CCM_IN_PAYLOAD_INDEX+i]); 	
	AES_OUT("]\n");
#endif

	NRF_CCM->EVENTS_ENDCRYPT       = 0;
	NRF_CCM->EVENTS_ERROR          = 0;
	NRF_CCM->TASKS_CRYPT           = 1;
				
	while (NRF_CCM->EVENTS_ENDCRYPT == 0);			
	if (NRF_CCM->EVENTS_ERROR){
		AES_OUT("\ncCCM->EVENT ERROR\n");
		return false;
	}

#ifdef DEBUG_CLI_AES
	AES_OUT("\nOUT DATA: \n hd %d ,len %d \n[",out_data[CCM_IN_HEADER_INDEX] ,out_data[CCM_IN_LENGTH_INDEX] );
	for (i=0; i<out_data[ CCM_IN_LENGTH_INDEX ]+ CCM_MIC_SIZE ;i++)   AES_OUT("%d ", out_data[CCM_IN_PAYLOAD_INDEX+i]);
	AES_OUT("]\n");
	for (i=0; i<out_data[ CCM_IN_LENGTH_INDEX ]+ CCM_MIC_SIZE ;i++)  AES_OUT("%d ", out_data[CCM_IN_PAYLOAD_INDEX+i]);
#endif
	return true;

}

