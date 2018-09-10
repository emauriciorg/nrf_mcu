#ifndef __UESB_APP_H
#define __UESB_APP_H

//#define UESB_ORIGINAL

#include "micro_esb.h"
#include "RX_CAFE.h"
#include "tinyRF.h"

void uesb_event_handler(void);
void uesb_setup_tx(uesb_payload_t *tx_payload);

void uesb_setup_rx(void);
void cafe_setup_rx(void);
#endif

