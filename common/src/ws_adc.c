#include "ws_adc.h"
#include "nrf_adc.h"
#include "nrf_gpio.h"
#include <stdio.h>
#ifdef NRF52
	#include "nrf_saadc.h"
#endif



#define DEBUG_ADC

#ifdef DEBUG_ADC
	#define ADC_OUT(...)  printf(__VA_ARGS__)
#else
	#define ADC_OUT(...)
#endif

#define RESISTOR_1              1000
#define RESISTOR_2              390
#define SUPPLY_VOLTAGE          2.8
#define ADC_RESOLUTION          1024
#define ADC_SCALING             3
#define ADC_REFERENCE_DIVIDER   3 
#define VOLTAGE_UNIT            1000

#define CALCULATE_VOLTAGE_FROM_ADC(adc_value)   (VOLTAGE_UNIT*(ADC_SCALING * (adc_value * (SUPPLY_VOLTAGE/ADC_REFERENCE_DIVIDER))/ ADC_RESOLUTION))   
//	voltage_read_in_volts = scaling* ((adc_read * (VDD/reference)) / resolution);

#define CALCULATE_BATTERY_VOLTAGE(resistor_2_voltage)  (resistor_2_voltage*((RESISTOR_1+RESISTOR_2)/RESISTOR_2))

void ws_adc_setup(void)
{
	
	nrf_adc_config_t  adc_setup_parameters;
	adc_setup_parameters.resolution = NRF_ADC_CONFIG_RES_10BIT; 
	adc_setup_parameters.scaling    = NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD;    
	adc_setup_parameters.reference  = NRF_ADC_CONFIG_REF_SUPPLY_ONE_THIRD;  
	
	nrf_gpio_cfg_output(FET_ADC);
	nrf_adc_configure (&adc_setup_parameters);
//	nrf_saadc_channel_init(uint8_t channel, nrf_saadc_channel_config_t const * const config);

}

int32_t ws_adc_read(void)
{

	nrf_gpio_pin_set(FET_ADC);
				
	int32_t adc_read;
	
	adc_read= nrf_adc_convert_single(NRF_ADC_CONFIG_INPUT_2);
	//Vin = 550 * (VDD/4) / (1024 * (1/6)) = 2.66.

#ifdef VOLTAGE_READ_IN_VOLTS	
	float voltage_read_in_volts;
	voltage_read_in_volts = scaling* ((adc_read * (VDD/reference)) / resolution);
	ADC_OUT ("[%d %.3f v]\n", adc_read, voltage_read_in_millivolts);
	
#else
	int32_t voltage_read_in_millivolts;
	float k2 =(RESISTOR_1+RESISTOR_2)/RESISTOR_2;
	voltage_read_in_millivolts = CALCULATE_VOLTAGE_FROM_ADC(adc_read) ; 
	ADC_OUT ("[NADC %d] [VBAT %d V][V1 %d] [k2 %.4f]\n", 
		adc_read,
		CALCULATE_BATTERY_VOLTAGE(voltage_read_in_millivolts),
		voltage_read_in_millivolts,
		k2);
#endif	
	nrf_gpio_pin_clear(FET_ADC);
	
	return 0;
}
