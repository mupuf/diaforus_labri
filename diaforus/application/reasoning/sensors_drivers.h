#ifndef SENSORS_DRIVERS_H_
#define SENSORS_DRIVERS_H_

#include "reasoning_common.h"
#include "sensors.h"

void sensors_drivers_init(sensor_t sensors[], size_t len);

int sensors_drivers_read_value(sensor_t *sensors);
void sensors_drivers_write_value(sensor_t *sensors, u8 value);

#endif
