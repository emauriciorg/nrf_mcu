#ifndef __IO_EXPANDER_DV_H__
#define __IO_EXPANDER_DV_H__


#include "nrf_drv_twi.h"
#include "io_expander_addr.h"

void io_init_io_expander(void);
void io_read_port(void);

#endif

