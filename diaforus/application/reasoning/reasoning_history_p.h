/*
 * reasoning_history_p.h
 *      Author: Romain Perier <romain.perier@labri.fr>
 */

#ifndef REASONING_HISTORY_P_H
#define REASONING_HISTORY_P_H

#include <stdint.h>
#include "sensors.h"

#define SENSOR_STATE_COUNT SENSOR_CONTRIBUTION_HISTORY_SIZE

typedef struct
{
	sensorid_t sensorid;
	uint8_t header : 4;
	uint8_t contribution: 4;
} sensor_state_t;

extern sensor_state_t	sensor_state_table[SENSOR_STATE_COUNT];

#endif /* REASONING_HISTORY_P_H */
