#include "sensor_switch.h"
#include "sensors_drivers.h"

#include "sim_common.h"
#include "hw_modality.h"
#include "debug_led_mapping.h"

value_t sensor_poll_switch(sensor_t *sensor)
{
	static int cnt;
	value_t value;

#if !IS_SIMU && (HW_MODALITY==WITH_DEMOBOARD)

	/* read twice to avoid some caching issues */
	value = (value_t)(mcp23018_read_switches() >> 3);
	value = (value_t)(mcp23018_read_switches() >> 3);

	value = (value >> sensor->port.i2c.address) & 0x1;
	
	value = sensor_calibration_digital(sensor, value);

	if (sensor->id == 0) {
		debug_led(LED_SWITCH_0_POLLING, (cnt++) % 2);
		debug_led(LED_SWITCH_0, value);
	}
#else
	value = 0;
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
