/*
 * reasoning_history.h
 *      Author: Romain Perier <romain.perier@labri.fr>
 */

#ifndef REASONING_HISTORY
#define REASONING_HISTORY

#include "reasoning_common.h"
#include "sensors.h"

typedef uint32_t timestamp_t;

void reasoning_history_update(uint16_t criticality);

uint8_t reasoning_history_find_suspicious_event_index(timestamp_t timestamp, uint8_t critical_level);

uint8_t reasoning_history_find_nearest_event(timestamp_t timestamp);

uint8_t reasoning_history_get_event_critical_level(uint8_t header);

timestamp_t reasoning_history_get_event_timestamp(uint8_t header);

void reasoning_history_set_event_duration(timestamp_t newduration);

bool reasoning_history_is_full(void);

void reasoning_history_init(void);

void reasoning_history_suspicious_events_serialize(u8 *buffer);

uint16_t reasoning_history_alert_involved_events(uint8_t index);

uint8_t reasoning_history_alert_involved_in_alarms(uint8_t index);

#endif
