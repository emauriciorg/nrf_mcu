#ifndef _ADC_APP_H_
#define	_ADC_APP_H_
#include <stdint.h>


#define FET_ADC 23

void adc_app_init(void);
int32_t adc_app_read(void);

#endif /* _ADC_APP_H_ */
