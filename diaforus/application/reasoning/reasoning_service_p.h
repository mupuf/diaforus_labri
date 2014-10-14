/*
 * reasoning_service_p.h
 *      Author: Romain Perier <romain.perier@labri.fr>
 */

#ifndef REASONING_SERVICE_P_H
#define REASONING_SERVICE_P_H

#include "reasoning_common.h"
#include "reasoning_service.h"
#include "sensors.h"

#define CRITICALITY_VARIATION_TIME (get_min_intrusion_duration() / 10)

typedef struct
{
	portTickType time; // 4 bytes
	sensorid_t sensor_ID; // 2 bytes
	uint8_t value; // 1 byte
} sensor_update_t; // 7 bytes (needs to be packed)

/// stores the state of sensors
typedef struct 
{
	sensor_update_t sensors[ALERT_HISTORY_SIZE];
	uint8_t size;
} sensors_update_t;

/* calculates the current amplitude of an alert (with amplitude = 1) initiated at t = time */
static inline uint8_t compute_linear_criticality(portTickType time, int global_duration)
{
	portTickType time_delta_ms = time_get() - time;
	int linear_time = 100 - ((time_delta_ms * 100)/ global_duration);

	if (linear_time < 0)
	  linear_time = 0;
	if (linear_time > 100)
	  linear_time = 100;
	return linear_time;
}

extern sensors_update_t sensors_updates;
extern uint16_t reasoning_alarm_count, reasoning_alert_count;
extern uint8_t sensor_count;

#endif /* REASONING_SERVICE_P_H */
