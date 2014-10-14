#ifndef SENSOR_SPIRIT_H_
#define SENSOR_SPIRIT_H_

#include "pubsub_common.h"
#include "reasoning_config.h"
#include "reasoning_common.h"
#include "sensors.h"

value_t sensor_poll_spirit(sensor_t *sensor);
bool sensor_criticality_spirit(sensor_t* sensor);

#endif
