/*
 * reputation_management.h
 *      Author: Hassen Ghariani <gharianihassen@gmail.com>
 *      Author: Romain Perier <romain.perier@labri.fr>
 */

#ifndef REPUTATION_MANAGEMENT_H
#define REPUTATION_MANAGEMENT_H

#include "sensors.h"
#include "reasoning_history.h"

void reputation_management_report_false_positive(timestamp_t timestamp, uint8_t critical_level);

void reputation_management_report_false_negative(timestamp_t timestamp);

void reputation_management_auto_adjust(uint8_t alert_index);

uint8_t reputation_management_get_sensor_reputation(sensorid_t id);

void reputation_management_update_total_detection(sensorid_t id);

void reputation_management_undo_tp(sensorid_t id);

void reputation_management_init();

#endif /* REPUTATION_MANAGEMENT_H */
