#ifndef __ACCELOMETER_I2C_H_
#define __ACCELOMETER_I2C_H_
#include <stdint.h>
#define ADXL_ADDRESS_OPT1_W   0X3A// BASE ADDR 0X1D <<1 || W/R
#define ADXL_ADDRESS_OPT1_R   0X3B
#define ADXL_ADDRESS_OPT2_W   0XA6//sdo grounded 1100 1000 -> 0110 0100 , 
#define ADXL_ADDRESS_OPT2_R   0XA7//		 

void adxl345_read(void);
void twi_configure(void);


void ws_accelerometer_setup(void);
void ws_accelerometer_read_reg(void);
void ws_accelerometer_load_addr(uint8_t address_to_load);
int ws_accelerometer_on_start_configuration(void);
#endif
