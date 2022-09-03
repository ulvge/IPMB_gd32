#include "adc/api_adc.h"
#include "OSPort.h"

void sample_init()
{
	adc_init();
}


/*get temprate value */
float get_temprate_convers_value(uint16_t channel)
{
	  uint16_t adcx;
	  float  temperate;
	  adcx = get_adc_average_convers_value(channel,TIMES);
	  
		temperate = (float)adcx*(VREFVOL/ADC_BIT);
	
	  /* get temperate conversion value */
		temperate =(SENSOR_V25_VALUE-temperate)/SENSOR_AVG_SLOPE+SENSOR_TEMP25_VALUE;
	
	  return temperate;
	
}

/* get vref voltage value*/
float get_vref_voltage_convers_value(uint16_t channel)
{
	 uint16_t adcx;
	  float  convers_value;
	  adcx = get_adc_average_convers_value(channel,TIMES);
	  
		convers_value = (float)adcx*(VREFVOL/ADC_BIT);
   return convers_value;

}

/* get  voltage value*/
float get_voltage_convers_value(uint16_t channel)
{
	  uint16_t adcx;
	  float voltage ,dc_voltage;
	
	  adcx = get_adc_average_convers_value(channel,TIMES);
	  voltage =(float)adcx*(VREFVOL/ADC_BIT);
	
	  if(channel == VOL_3_3V)
		{
		   dc_voltage = ((PARTIAL_PRESSURE_CONFF1+TWO_PARTIAL_PRESSURE_CONFF)*voltage)/TWO_PARTIAL_PRESSURE_CONFF;
			
		}else if(channel == VOL_5V)
		{
				dc_voltage = (PARTIAL_PRESSURE_CONFF2+ONE_PARTIAL_PRESSURE_CONFF1)*voltage;
			 
		}else if(channel == VOL_12V)
		{
				dc_voltage = (PARTIAL_PRESSURE_CONFF2+ONE_PARTIAL_PRESSURE_CONFF1)*voltage;
		}else
		   dc_voltage = voltage;
	  
	  return dc_voltage;
}


bool get_raw_adc_data_value(uint16_t channel, uint16_t* value)
{
  	*value = get_adc_convers_value(channel); 

	return true;
}
