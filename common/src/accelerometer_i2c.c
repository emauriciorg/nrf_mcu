#include "accelerometer_i2c.h"

#include <stdio.h>

#include "nrf.h"
#include "app_error.h"
#include "app_util_platform.h"
#include "nrf_drv_twi.h"
#include "app_twi.h"
#include "nrf_delay.h"
#include "mma8652.h"


#define ACCELEROMETER_INT_PIN       2
#define ACCELEROMETER_SCL_PIN       5
#define ACCELEROMETER_SDA_PIN       4
#define MAX_PENDING_TRANSACTIONS    8
#define BUFFER_SIZE                 24

#define ACC_MSG(...) printf(__VA_ARGS__);

static app_twi_t m_app_twi = APP_TWI_INSTANCE(0);
static uint8_t m_buffer[BUFFER_SIZE];
static uint8_t addr = 0x00;
static bool transaction_pending;


#define MMA8652_READ(p_reg_addr, p_buffer, byte_cnt)                                        \
			APP_TWI_WRITE(MMA8652_ADDR, p_reg_addr, 1,        APP_TWI_NO_STOP), \
			APP_TWI_READ (MMA8652_ADDR, p_buffer,   byte_cnt, 0)

#define MMA8652_WRITE(p_reg_addr, p_buffer, byte_cnt)                                       \
			APP_TWI_WRITE(MMA8652_ADDR, p_reg_addr, 1,        APP_TWI_NO_STOP), \
			APP_TWI_WRITE(MMA8652_ADDR, p_buffer,   byte_cnt, 0)

#define ws_accelerometer_read_reg_BLOCKING(address, value)                         \
				addr = address;				  \
				ws_accelerometer_read_reg();			  \
				nrf_delay_ms(1);			  \
				while (transaction_pending == true){	  \
					ACC_MSG("Wating\r\n");       \
					nrf_delay_ms(1);		  \
				}		                          \
				value = m_buffer[0];



const nrf_drv_twi_t m_twi_global_accelerometer = NRF_DRV_TWI_INSTANCE(0);
void twi_handler_adx(nrf_drv_twi_evt_t const * p_event, void * p_context);


static volatile bool m_set_mode_done = false;


uint8_t regW,regR = 0;
uint8_t waiting_for_ack_response=0;
uint8_t waiting_for_reading_response=0;
uint8_t i2c_mode = 0;


#ifdef ALTERNATIVE_TWI_CONFIG

void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
	ret_code_t err_code;
	static sample_t m_sample;

    switch(p_event->type)
    {
	case NRF_DRV_TWI_EVT_DONE:
	    if ((p_event->type == NRF_DRV_TWI_EVT_DONE) &&
		(p_event->xfer_desc.type == NRF_DRV_TWI_XFER_TX))
	    {
		if(m_set_mode_done != true)
		{
		    m_set_mode_done  = true;
		    return;
		}
		m_xfer_done = false;
		/* Read 4 bytes from the specified address. */
		err_code = nrf_drv_twi_rx(&m_twi_mma_7660, MMA865_ADDR, (uint8_t*)&m_sample, sizeof(m_sample));
		APP_ERROR_CHECK(err_code);
	    }
	    else
	    {
		read_data(&m_sample);
		m_xfer_done = true;
	    }
	    break;
	default:
	    break;
    }
}
#endif


void ws_accelerometer_setup(){
	ret_code_t err_code;
	const nrf_drv_twi_config_t m_twi_accelerometer = {
		.scl                = ACCELEROMETER_SCL_PIN,
		.sda                = ACCELEROMETER_SDA_PIN,
		.frequency          = NRF_TWI_FREQ_100K,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH
	};

	APP_TWI_INIT(&m_app_twi, &m_twi_accelerometer, MAX_PENDING_TRANSACTIONS, err_code);
	APP_ERROR_CHECK(err_code);

#ifdef ALTERNATIVE_TWI_CONFIG
	err_code = nrf_drv_twi_init(&m_twi_mma_7660, &twi_mma_7660_config, twi_handler, NULL);
	APP_ERROR_CHECK(err_code);
	nrf_drv_twi_enable(&m_twi_mma_7660);
#endif

}


void ws_accelerometer_read_reg_cb(ret_code_t result, void * p_user_data)
{
    if (result != NRF_SUCCESS){
	ACC_MSG("ws_accelerometer_read_registers_cb - [error: %d][%x]\r\n", (int)result, m_buffer[0]);
	return;
    }

    ACC_MSG("accelerometer data: [0x%x],[0x%x],[0x%x]\r\n",m_buffer[0],m_buffer[1],m_buffer[2]);
    transaction_pending = false;
}

void ws_accelerometer_load_addr(uint8_t address_to_load){
	addr= address_to_load;
}


void ws_accelerometer_read_reg(){

	//addr=ADXL_WHO_AM_I;
    // [these structures have to be "static" - they cannot be placed on stack
    //  since the transaction is scheduled and these structures most likely
    //  will be referred after this function returns]

	transaction_pending = true;

    static app_twi_transfer_t const transfers[] =
    {
	MMA8652_READ(&addr,
	    m_buffer, 1)
    };
    static app_twi_transaction_t const transaction =
    {
	.callback            = ws_accelerometer_read_reg_cb,
	.p_user_data         = NULL,
	.p_transfers         = transfers,
	.number_of_transfers = sizeof(transfers) / sizeof(transfers[0])
    };
    APP_ERROR_CHECK(app_twi_schedule(&m_app_twi, &transaction));
}


#if 1
//ndef NO_ACC_CONFIGURATION

int ws_accelerometer_on_start_configuration(void){

	ACC_MSG("[fsmAccelerometer]:\tws_accelerometer_on_start_configuration\r\n");

///	fsmAccelerometer_moving = false;


	uint8_t retValue;
	ws_accelerometer_read_reg_BLOCKING(MMA8652_WHO_AM_I, retValue); //Read the Who am I

	if (retValue != 0x4A){
		ACC_MSG("[fsmAccelerometer]:\tNo accelerometer communication available\r\n");
		return 0;
	}
	//transaction_pending=1;
	static uint8_t ctrl_reg_init[]        = { MMA8652_CTRL_REG1  , 0x19 };			//Set in standby (CTRL_REG1)
	static uint8_t const conf_reg_init[]  = { MMA8652_FF_MT_CFG  , 0x00 };		//Set Configuration register for motion detection with OR in X and Y and latch
	static uint8_t const thres_reg_init[] = { MMA8652_FF_MT_THS  , 0x00 };		//Set Threshold for >1.5G
	static uint8_t const deb_reg_init[]   = { MMA8652_FF_MT_COUNT, 0x01 };		//Set Debounce counter to 10
	static uint8_t const ctrl_reg4_init[] = { MMA8652_CTRL_REG4  , 0x04 };		//Enable Motion/Freefall interrupt function in (CTRL_REG4)
	static uint8_t const ctrl_reg5_init[] = { MMA8652_CTRL_REG5  , 0x04 };		//Route Motion/Freefall interrupt to INT1 pin (CTRL_REG5)

	app_twi_transfer_t const init_transfers[6] =
	{
		APP_TWI_WRITE(MMA8652_ADDR, ctrl_reg_init , sizeof(ctrl_reg_init) , 0),
		APP_TWI_WRITE(MMA8652_ADDR, conf_reg_init , sizeof(conf_reg_init) , 0),
		APP_TWI_WRITE(MMA8652_ADDR, thres_reg_init, sizeof(thres_reg_init), 0),
		APP_TWI_WRITE(MMA8652_ADDR, deb_reg_init  , sizeof(deb_reg_init)  , 0),
		APP_TWI_WRITE(MMA8652_ADDR, ctrl_reg4_init, sizeof(ctrl_reg4_init), 0),
		APP_TWI_WRITE(MMA8652_ADDR, ctrl_reg5_init, sizeof(ctrl_reg5_init), 0)
	};

	app_twi_perform(&m_app_twi, init_transfers, 6, NULL);

	//while(transaction_pending);

	uint8_t ctrl_reg1;
	ws_accelerometer_read_reg_BLOCKING(MMA8652_CTRL_REG1, ctrl_reg1); //Read CTRL_REG1
	ctrl_reg1 |= 0x01;							//Device to active mode
	ctrl_reg_init[1] = ctrl_reg1;
	app_twi_transfer_t const init_transfer_ctrl_reg1[1] =
	{
		APP_TWI_WRITE(MMA8652_ADDR, ctrl_reg_init, sizeof(ctrl_reg_init), 0)
	};
	app_twi_perform(&m_app_twi, init_transfer_ctrl_reg1, 1, NULL);


    //Setup timer for motion
//    timer_dispatch_create_timer(fsmAccelerometer, fsmAccelerometer_motion_timeout);


//	sm_set_next_event_var(fsmAccelerometer, ACC_EVT_CONF_OK);
	return 1;
}
#endif
