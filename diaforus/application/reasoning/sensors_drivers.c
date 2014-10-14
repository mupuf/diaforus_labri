#include "reasoning_service.h"
#include "sensors_drivers.h"
#include "application_config.h"

#include "hw_modality.h"

#if !IS_SIMU
#include "gpio_public.h"
#include "mcp23018_public.h"
#include "i2c_public.h"
	#if !IS_SIMU && (HW_MODALITY==WITH_SPIRIT)
	#include "spirit_public.h"
	static int sensorState;

	void handle_spirit_response(u8* data_ptr){
		u8 status, value;
		status = data_ptr[3];
		value  = data_ptr[4];	
		if (value == 0x3F)
			sensorState = 0;
		else
			sensorState = 1;
	}
	#endif
#endif



void sensors_drivers_init(sensor_t sensors[], size_t len)
{
	int i;
	sensor_t *s;
	for (i = 0; i < len; i++)
	{
		s = &sensors[i];

		switch (s->connection)
		{
		case CONNECT_GPIO:
			break;
		case CONNECT_ADC:
			break;
		case CONNECT_I2C:

//disable the initialization of the i2c/io expander driver because it has already been done
#if 0 && !IS_SIMU && (HW_MODALITY==WITH_DEMOBOARD)
			// initialize the I2C driver + the IO expander.
			i2c_init();
			mcp23018_reset();
			mcp23018_init();
#endif			
			break;
		}
#if !IS_SIMU && (HW_MODALITY==WITH_SPIRIT)			
		if (s->port.gpio == 0x00){ 
			init_spirit_drv(handle_spirit_response);
			sensorState =0;
		}
#endif
	}
}

int sensors_drivers_read_value(sensor_t *sensor)
{
	int value = 0;

	switch (sensor->connection)
	{
	case CONNECT_GPIO:
#if(!IS_SIMU)
#if (HW_MODALITY==WITH_SPIRIT)
		if (sensor->port.gpio == 0x00){
				value = sensorState;
				transmitCMD(GET_DEVICE_INFORMATION_CMD);
		}
#endif		
		if (sensor->port.gpio == 0x02){			
			if(gpio_read(GPIOID2)>0){
				value = 1;		
			}
		}
		
		if (sensor->port.gpio == 0x03){
			if(gpio_read(GPIOID3)== 0){
				value = 1;		
			}
		}
		break;
#endif
	case CONNECT_ADC:
	case CONNECT_I2C:
		return 0;
	default:
		return -1;
	}

	/* calibration stage */
	if (sensor->value_type == DIGITAL_VALUE)
	{
		if (sensor->calibration.invert)
			value = !value;
	}
	else if (sensor->value_type == ANALOG_VALUE)
	{
		value += sensor->calibration.analog.offset;
		value *= sensor->calibration.analog.mult;
		value /= sensor->calibration.analog.div;
	}

	return value;
}
void sensors_drivers_write_value(sensor_t *sensor, u8 value )
{
#if(!IS_SIMU)

	switch (sensor->connection)
	{
	case CONNECT_GPIO:

		if (sensor->port.gpio == 0x02){			
			if(value > 0){
				gpio_write(GPIOID2, GPIOID2);		
			} else {
				gpio_write(GPIOID2, ((~GPIOID2)& GPIOID2));	
			}
		}
		if (sensor->port.gpio == 0x03){			
			if(value > 0){
				gpio_write(GPIOID3, GPIOID3);		
			} else {
				gpio_write(GPIOID3, ((~GPIOID3)& GPIOID3));	
			}
		}
		if (sensor->port.gpio == 0x0D){		
			if(value > 0){
				gpio_write(GPIOID13, GPIOID13);		
			} else {
				gpio_write(GPIOID13, ((~GPIOID13)& GPIOID13));	
			}
		}
		return;

	default:
		return;
	}

#endif
}
