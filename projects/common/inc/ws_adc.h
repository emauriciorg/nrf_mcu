#ifndef _ADC_APP_H_
#define	_ADC_APP_H_
#include <stdint.h>


#define FET_ADC 23

void ws_adc_setup(void);
int32_t ws_adc_read(void);

#endif /* _ADC_APP_H_ */
