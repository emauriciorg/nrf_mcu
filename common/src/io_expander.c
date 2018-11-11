#include "../inc/io_expander.h"
#include "app_error.h"
#include <stdio.h>
#include "nrf.h"
#include "app_util_platform.h"
static const nrf_drv_twi_t m_twi_io_expander = NRF_DRV_TWI_INSTANCE(0);
static volatile bool m_xfer_done = true;


void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{   
    ret_code_t err_code;
    static char  m_sample;
    
    switch(p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if ((p_event->type == NRF_DRV_TWI_EVT_DONE) &&
                (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_TX))
            {
                // if(m_set_mode_done != true)
                // {
                //     m_set_mode_done  = true;
                //     return;
                // }
                m_xfer_done = false;
                /* Read 4 bytes from the specified address. */
                err_code = nrf_drv_twi_rx(&m_twi_io_expander, INPUT_PORT_REGISTER, (uint8_t*)&m_sample, sizeof(m_sample));
                APP_ERROR_CHECK(err_code);
                printf("recieved\n");
            }
            else
            {
                printf("recieved!\n");
            
               // read_data(&m_sample);
                m_xfer_done = true;
            }
            break;
        default:
            break;        
    }   
} 

void io_init_io_expander(void){
    ret_code_t err_code;
    
    const nrf_drv_twi_config_t m_twi_io_expander_config = {
       .scl                = IO_EXPANDER_INT_SCL,
       .sda                = IO_EXPANDER_INT_SDA,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH
    };
    
    err_code = nrf_drv_twi_init(&m_twi_io_expander, &m_twi_io_expander_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_twi_enable(&m_twi_io_expander);

}





void io_read_port(){
    ret_code_t err_code;
    unsigned char port_lecture=0;
	
//    err_code = nrf_drv_twi_tx(&m_twi_io_expander, INPUT_INTERRUPT_MASK, &port_lecture, sizeof(port_lecture), false);  
	err_code = nrf_drv_twi_rx(&m_twi_io_expander, 0x3A, (uint8_t*)&port_lecture, sizeof(port_lecture));
    printf("reading sensor %d\n", port_lecture);

    APP_ERROR_CHECK(err_code);
    
}




