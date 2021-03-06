
#include "sensor_seismic.h"
#include "sensors_drivers.h"

// #include <math.h>

value_t sensor_poll_seismic(sensor_t *sensor)
{
	uint8_t value = (uint8_t) sensors_drivers_read_value(sensor);
	value = sensor_calibration_digital(sensor, value);
#if IS_SIMU	
	if (sensor->stimulus[0] != UINT8_MAX)
	{
		value = sensor->stimulus[0] & 0x1;
		sensor->stimulus[0] = UINT8_MAX;
	}
#endif
	sensor_history_add(&sensor->history, value);
	sensor->last_value = value;
	return value;
}
