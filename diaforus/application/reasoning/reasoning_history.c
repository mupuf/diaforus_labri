/*
 * reasoning_history.c
 *      Author: Romain Perier <romain.perier@labri.fr>
 */

#include <stdint.h>

#include "reasoning_common.h"

#if IS_SIMU
# include <assert.h>
#endif

#include "reasoning_config.h"
#include "reasoning_history.h"
#include "reasoning_history_p.h"
#include "reasoning_service.h"
#include "reasoning_service_p.h"
#include "reputation_management.h"
#include "sensors.h"

#define ALERT_HISTORY_SIZE (SUSPICIOUS_STATE_HISTORY_SIZE * 3)

#if ROLE_REASONING
typedef struct
{
	timestamp_t timestamp;
	uint8_t critical_level;
} event_t;

typedef struct
{
       sensorid_t sensorid;
       uint16_t involved_events; // bit field
       uint8_t alarm : 7; // used in at least one alarm
       uint8_t last_alert : 1;
       timestamp_t time;
} alert_t;

static timestamp_t	last_event_timestamp;
static timestamp_t	duration = HISTORY_ANALYZE_PERIOD;
static timestamp_t      last_alarm_emitted;
static uint32_t         last_involved_sensors;
static event_t		event_table[SUSPICIOUS_STATE_HISTORY_SIZE] __attribute__ (( section (".slowdata") ));
static alert_t          alert_table[ALERT_HISTORY_SIZE] __attribute__ (( section (".slowdata") ));

sensor_state_t	sensor_state_table[SENSOR_STATE_COUNT] __attribute__ (( section (".slowdata") ));

extern uint32_t reputation_alarm_count;

/*
 * This function get all events greater than @timestamp, and then
 * for all these events we get the smallest : so we get the nearest event to @timestamp
 */
static uint8_t find_next_nearest_event(timestamp_t timestamp, uint8_t id)
{
	timestamp_t smallest = UINT32_MAX;
	uint8_t index = 255, i = 0;
	for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
	{
		if (i == id)
			continue;
		if (event_table[i].timestamp >= timestamp &&
			event_table[i].timestamp <= smallest)
		{
			index = i;
			smallest = event_table[i].timestamp;
		}
	}
	return index;
}

static uint8_t find_next_strict_nearest_event(timestamp_t timestamp)
{
	timestamp_t smallest = UINT32_MAX;
	uint8_t index = 255, i = 0;
	for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
	{
		if (event_table[i].timestamp > timestamp &&
			event_table[i].timestamp < smallest)
		{
			index = i;
			smallest = event_table[i].timestamp;
		}
	}
	return index;
}

#warning "FIXME: use log"
static uint32_t compute_event_score(uint8_t id)
{
	// An alarm is always more important than a suspicious event,
	// the score must take this criterion in account.
	uint8_t nearest_index = find_next_nearest_event(event_table[id].timestamp, id);
	timestamp_t nearest_timestamp = nearest_index == 255 ? last_event_timestamp
		: event_table[nearest_index].timestamp;

	timestamp_t timestamp_to_ms = port_tick_to_ms(nearest_timestamp - event_table[id].timestamp);

	uint32_t score = (timestamp_to_ms * event_table[id].critical_level) + ((event_table[id].critical_level & 1) << 30);
	uint32_t bonus = compute_linear_criticality(event_table[id].timestamp, get_min_intrusion_duration()) * 2 * ((1 << 30) / 100);

	//PRINTF("REPUTATION MANAGEMENT: bonus for %u is %u\n", id, bonus);

	if (bonus + score < score)
	  score = UINT32_MAX;
	else
	  score = bonus + score;
	
	//DEBUG("REASONING HISTORY", LOG_CRITICAL, "SCORE: id=%u -> next nearest event has timestamp %u : score=%lu\n", id, nearest_timestamp, score);
	return score;
}

static inline bool duration_has_expired (timestamp_t timestamp)
{
	return (time_get() - timestamp) >= duration;
}

static inline bool event_slot_is_free(int slot)
{
	return (event_table[slot].timestamp == 0) &&
		(event_table[slot].critical_level == 0);
}

static inline bool alert_slot_is_free(uint8_t index)
{
  return (alert_table[index].sensorid == UINT16_MAX) && (alert_table[index].involved_events == 0) && (alert_table[index].alarm == 127) && (alert_table[index].last_alert == 1) && (alert_table[index].time == 0);
}

static uint8_t find_suitable_event_index(timestamp_t timestamp, uint8_t critical_level)
{
        uint8_t index = 0, i = 0;
	uint32_t score = UINT32_MAX;

	// Find and delete all expired event
	for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
	{
		if (duration_has_expired(event_table[i].timestamp))
		{
			DEBUG("REASONING HISTORY", LOG_DEBUG, "find_suitable_event_index "
				"evicted %u (expired)\n", i);
			index = i;
			goto remove_subentries;
		}
	}

	// If we find a free slot, we get it
	for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
	{
		if (event_slot_is_free(i))
		{
			DEBUG("REASONING HISTORY", LOG_DEBUG, "find_suitable_event_index "
			"found a free slot at %u\n", i);
			index = i;
			goto exit;
		}
	}

	for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
	{
		// Otherwise, we look for the less relevant event (the one with the smallest score)
		uint32_t tmp = compute_event_score(i);
		//PRINTF("REASONING HISTORY: event %u has score %u\n", i, tmp);
		if (tmp < score)
		{
			index = i;
			score = tmp;
		}
	}

remove_subentries:
	PRINTF("REASONING HISTORY: Removing event %u from history with score %u\n", index, score);

	// When the event_table is full and the less revelant event is found
	// we remove every sensors contribution attached to this event
	for (i = 0; i < SENSOR_STATE_COUNT; i++)
		if (sensor_state_table[i].header == index)
			__builtin_memset(sensor_state_table + i, 0, sizeof(*sensor_state_table));
	if (score != UINT32_MAX)
	{
		DEBUG("REASONING HISTORY", LOG_DEBUG, "REASONING HISTORY: find_suitable_event_index evicted"
			" index %u with score %u\n", index, score);
	}

	// Remove the evicted event pointed by "event_table[index]" from alert_table
	// When an alert obtains "involved_events" equals to zero, the alert is evicted from the table (no longer used)
	for (i = 0; i < ALERT_HISTORY_SIZE; i++)
	{
	        if (alert_slot_is_free(i))
	            continue;
		
		// If this alert was involved for this event, remove the contribution to the event
		if (alert_table[i].involved_events & (1 << index))
		{
			alert_table[i].involved_events &= ~(1 << index);
		
			// If the alert is no longer used
			if (alert_table[i].involved_events == 0)
			  {
			    
			    // Adjust the reputation for the corresponding sensor
			    // and then evict the entry from the table
			    PRINTF("REASONING HISTORY: Removing alert %u\n", i);
			    reputation_management_auto_adjust(i);
			    alert_table[i].sensorid = UINT16_MAX;
			    alert_table[i].alarm = 127;
			    alert_table[i].last_alert = 1;
			    alert_table[i].time = 0;
			  }
		}
	}

exit:
	return index;
}

static uint8_t find_suitable_sensor_state_index(sensorid_t id, uint8_t contrib_header)
{
	uint8_t index = 0, least_relevant_header = 0, i = 0;
	uint32_t score = UINT32_MAX;
	
	for (i = 0; i < SENSOR_STATE_COUNT; i++)
	{
		// If we find a free slot, we get it
		if (sensor_state_table[i].sensorid == 0 && 
		sensor_state_table[i].header == 0 &&
		sensor_state_table[i].contribution == 0)
		{
			DEBUG("REASONING HISTORY", LOG_DEBUG, "find_suitable_sensor_state_index "
			"found a free slot at %u\n", i);
			return i;
		}
	}

	for (i = 0; i < SENSOR_STATE_COUNT; i++)
	{
		// Otherwise we look for the less relevant event
		if (compute_event_score(sensor_state_table[i].header) < score)
		{
			least_relevant_header = i;
			score = compute_event_score(sensor_state_table[i].header);
		}
	}

	// Once the less revelant event is found, we look for the
	// sensor with the smallest contribution
	index = 0;
	for (i = 0; i < SENSOR_STATE_COUNT; i++)
	{
		if (sensor_state_table[i].header == least_relevant_header &&
		sensor_state_table[i].contribution < sensor_state_table[index].contribution)
		{
			index = i;
		}
	}
	DEBUG("REASONING HISTORY", LOG_DEBUG, "REASONING HISTORY: find_suitable_sensor_state_index"
		" evicted index %u , attached to header %u (score=%u) with "
		"contribution %u\n", index, least_relevant_header, score);
	return index;
}

static inline void write_event_at(uint8_t index, timestamp_t timestamp, uint8_t critical_level)
{
#if IS_SIMU
	assert (index >= 0 && index < SUSPICIOUS_STATE_HISTORY_SIZE);
#endif
	event_table[index].timestamp = timestamp;
	event_table[index].critical_level = critical_level;
}

static inline void write_sensor_state_at(uint8_t index, sensorid_t id, uint8_t contrib_header)
{
#if IS_SIMU
	assert (index >= 0 && index < SENSOR_STATE_COUNT);
#endif
	sensor_state_table[index].sensorid = id;
	sensor_state_table[index].header = contrib_header & 0xf;
	sensor_state_table[index].contribution = contrib_header >> 4;
}

static uint8_t register_important_event (timestamp_t timestamp, uint8_t criticality_level)
{
	uint8_t index = find_suitable_event_index(timestamp, criticality_level);
	write_event_at(index, timestamp, criticality_level);
	return index;
}

static void register_sensor_state (sensorid_t sensor_ID, uint8_t contrib_header)
{
	uint8_t index = find_suitable_sensor_state_index(sensor_ID, contrib_header);
	write_sensor_state_at(index, sensor_ID, contrib_header);
}

static uint8_t compute_criticality_history(uint16_t criticality, int criticality_threshold, bool alarm)
{
	uint32_t ratio = (criticality * 100) / criticality_threshold;
	uint8_t criticality_level;

	if (ratio > 499)
		ratio = 499;
	if (! alarm)
		criticality_level = ratio;
	else
	{
		ratio -= 100;
		criticality_level = ratio * 32 / 100;
		if (criticality_level > 0)
		  criticality_level--;
	}
	criticality_level = (criticality_level << 1) | (alarm & 0x1);
	return criticality_level;
}

/* Returns the sensor contribution stored in 4 bits */
static inline uint8_t compute_sensor_contribution(const sensor_update_t *su, uint16_t criticality)
{
        uint8_t reputation = reputation_management_get_sensor_reputation(su->sensor_ID);
	return (((su->value * compute_linear_criticality(su->time, get_max_intrusion_duration() * 2) * reputation / 100)) * 15) / criticality;
}

static uint32_t get_involved_sensors(uint8_t header)
{
	uint8_t i;
	uint32_t value = 0;

	for (i = 0; i < SENSOR_STATE_COUNT; i++)
	{
		if (sensor_state_table[i].header == header)
		{
			value |= (1 << node_id_from_sensor_id(sensor_state_table[i].sensorid));
		}
	}
	return value;
}

static uint8_t find_last_alert_for(sensorid_t sensorid)
{
	uint8_t i;
  
	for (i = 0; i < ALERT_HISTORY_SIZE; i++)
	{
	  if (!alert_slot_is_free(i) && (alert_table[i].sensorid == sensorid) && alert_table[i].last_alert)
			return i;
	}
	return UINT8_MAX;
}

/* Compare the criticality to the corresponding thresholds and notify the history
 in case of suspect events or an alarm */
void reasoning_history_update(uint16_t criticality)
{
	int criticality_threshold;
	uint32_t involved_sensors;
	uint8_t i, j, criticality_level, index, contrib_header, alert_index;

	criticality_threshold = get_criticality_threshold();

	if (criticality_threshold == 0)
		return;

	criticality_level = compute_criticality_history(criticality, criticality_threshold,
				    criticality >= criticality_threshold);

	last_event_timestamp = time_get();
	index = register_important_event(last_event_timestamp, criticality_level);

	DEBUG("REASONING HISTORY", LOG_CRITICAL, "%s : criticality = %u, criticality_level = %u, "
			 "timestamp %u, threshold = %d\n",
		criticality_level & 0x1 ? "Alarm" : "Suspicious Event",
		criticality, criticality_level, last_event_timestamp, criticality_threshold);

	for (i = 0; i < sensors_updates.size; i++)
	{
		contrib_header = compute_sensor_contribution(sensors_updates.sensors + i, criticality);

		if (contrib_header == 0)
			continue;

		DEBUG("REASONING HISTORY", LOG_DEBUG, "%s-%u: has a contribution of %f percent in %f\n",
			modality_string( modality_from_sensor_id(sensors_updates.sensors[i].sensor_ID) ),
		      id_from_sensor_id(sensors_updates.sensors[i].sensor_ID), contrib_header / 15.0, criticality);

		contrib_header = (contrib_header << 4) | index;
		register_sensor_state(sensors_updates.sensors[i].sensor_ID, contrib_header);
		
		// Get the corresponding alert (the more recent alert for this sensor)

		alert_index = find_last_alert_for(sensors_updates.sensors[i].sensor_ID);
		if (alert_index != UINT8_MAX)
		{
		    // Update the alert
		    PRINTF("REASONING HISTORY: Updating alert at %u with event at %u\n", alert_index, index);
		    alert_table[alert_index].involved_events |= (1 << index);
		    if (criticality_level & 0x1)
		        alert_table[alert_index].alarm = 1;
		}
	}
	
	if (criticality_level & 0x1)
	{
                reputation_alarm_count++;
		for (i = 0; i < sensors_updates.size; i++)
		{
		  reputation_management_update_alarm_contribution(sensors_updates.sensors[i].sensor_ID);
		}
		
	        involved_sensors = get_involved_sensors(index);
		if ((last_involved_sensors == involved_sensors) && ((time_get() - last_alarm_emitted) < get_min_intrusion_duration()))
		  return;
		
		reasoning_emit_alarm((criticality / criticality_threshold) * 100, last_event_timestamp, involved_sensors);
		last_involved_sensors = involved_sensors;
		last_alarm_emitted = time_get();
	}
}

uint8_t reasoning_history_find_suspicious_event_index(timestamp_t timestamp, uint8_t critical_level)
{
	uint8_t i;
	for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
	{
		if (event_table[i].timestamp == timestamp &&
		event_table[i].critical_level == critical_level)
		{
			return i;
		}
	}
	return reasoning_history_find_nearest_event(timestamp);
}

uint8_t reasoning_history_find_nearest_event(timestamp_t timestamp)
{
	uint8_t i = 0, nearest_prev = 0;
	uint8_t nearest_next = find_next_strict_nearest_event(timestamp);
	timestamp_t largest = 0;

	for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
	{
		if (event_table[i].timestamp < timestamp &&
			event_table[i].timestamp > largest)
		{
			nearest_prev = i;
			largest = event_table[i].timestamp;
		}
	}
	timestamp_t prev_diff = timestamp - event_table[nearest_prev].timestamp;
	timestamp_t next_diff = event_table[nearest_next].timestamp - timestamp;

	return next_diff >= prev_diff ? nearest_prev : nearest_next;
}

bool reasoning_history_is_full()
{
	volatile uint8_t i;
	
	for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
		if (event_slot_is_free(i))
			return false;
	return true;
}

uint8_t reasoning_history_get_event_critical_level(uint8_t header)
{
	return event_table[header].critical_level;
}

timestamp_t reasoning_history_get_event_timestamp(uint8_t header)
{
	return event_table[header].timestamp;
}

void reasoning_history_set_event_duration(timestamp_t newduration)
{
	duration = newduration;
}

void reasoning_history_init(void)
{
        uint8_t i;

	last_alarm_emitted = 0;
	last_event_timestamp = 0;
	last_involved_sensors = 0;
	duration = HISTORY_ANALYZE_PERIOD;
	memset(event_table, 0, SUSPICIOUS_STATE_HISTORY_SIZE * sizeof(event_t));
	memset(sensor_state_table, 0, SENSOR_STATE_COUNT * sizeof(sensor_state_t));

	for (i = 0; i < ALERT_HISTORY_SIZE; i++)
	{
	  alert_table[i].sensorid = UINT16_MAX;
	  alert_table[i].involved_events = 0;
	  alert_table[i].alarm = 127;
	  alert_table[i].last_alert = 1;
	  alert_table[i].time = 0;
	}
}

void reasoning_history_suspicious_events_serialize(u8 *buffer)
{
   event_t nevent;
   sensorid_t sensorid;
   u8 contrib_header;
   int i = 0, j = 0, index = 0;

   for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
   {
     nevent.timestamp = htonl(event_table[i].timestamp);
     nevent.critical_level = event_table[i].critical_level;
     memcpy(buffer + index, &nevent.timestamp, 4);
     memcpy(buffer + index + 4, &nevent.critical_level, 1);
     
     index += 5;
   }
   
   for (j = 0; j < SENSOR_STATE_COUNT; j++)
   {
     sensorid = htons(sensor_state_table[j].sensorid);
     contrib_header = (sensor_state_table[j].contribution << 4 | sensor_state_table[j].header);
     memcpy(buffer + index, &sensorid, 2);
     memcpy(buffer + index + 2, &contrib_header, 1);

     index += 3;
   }
}

uint16_t reasoning_history_alert_involved_events(uint8_t index)
{
	return alert_table[index].involved_events;
}

uint8_t reasoning_history_alert_involved_in_alarms(uint8_t index)
{
	return alert_table[index].alarm;
}

sensorid_t reasoning_history_alert_sensorid(uint8_t index)
{
	return alert_table[index].sensorid;
}

void reasoning_history_register_new_alert(sensorid_t sensorid, uint16_t involved_events, uint8_t alarm)
{
	uint8_t i;

	for (i = 0; i < ALERT_HISTORY_SIZE; i++)
	{
		if (!alert_slot_is_free(i) && (alert_table[i].sensorid == sensorid)) 
		{
			if (alert_table[i].last_alert && ((time_get() - alert_table[i].time) <= get_min_intrusion_duration()))
			{
				PRINTF("REASONING HISTORY: Merging new alert with slot %u\n", i);
				goto registering;
			}
			else
				alert_table[i].last_alert = 0;
		}
	}
	for (i = 0; i < ALERT_HISTORY_SIZE; i++)
	{
		if (alert_slot_is_free(i))
			break;
	}
	PRINTF("REASONING HISTORY: Registering new alert at %u\n", i);
registering:
	alert_table[i].sensorid = sensorid;
	alert_table[i].involved_events = involved_events;
	alert_table[i].alarm = alarm;
	alert_table[i].last_alert = 1;
	alert_table[i].time = time_get();
}
#endif
