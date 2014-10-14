/*
 * reputation_management.c
 *      Author: Hassen Ghariani <gharianihassen@gmail.com>
 *      Author: Romain Perier <romain.perier@labri.fr>
 */

#include <assert.h>
#include "reasoning_history_p.h"
#include "reasoning_service.h"
#include "reputation_management.h"

#if IS_SIMU && ROLE_REASONING

#define REPUTATION_TABLE_SIZE 16

#warning "FIXME: Alignment"
typedef struct
{
	uint8_t reputation;
        bool tp_reported;
        sensorid_t sensorid;
        uint16_t tp; // correct contributions
        uint16_t td; // total contributions
        uint16_t alarm_contribution;
} reputation_t;

static reputation_t reputation_table[REPUTATION_TABLE_SIZE];
uint32_t reputation_alarm_count;

static inline bool slot_is_empty(uint16_t slot)
{
	return (reputation_table[slot].sensorid == UINT8_MAX) && (reputation_table[slot].reputation == 0);
}

static uint8_t sensor_contribution_index_lookup(sensorid_t sensorid, uint8_t header)
{
	uint8_t i;
	for (i = 0; i < SENSOR_STATE_COUNT; i++)
	{
		if ((sensor_state_table[i].sensorid == sensorid) && (sensor_state_table[i].header == header))
			return i;
	}
	return -1;
}

static uint8_t reputation_table_index_lookup (sensorid_t id)
{
	uint8_t i;
	
	// The slot is already reserved
	for (i = 0; i < REPUTATION_TABLE_SIZE; i++)
	{
		if (reputation_table[i].sensorid == id)
			return i;
	}
	
	// Find the first free slot
	for (i = 0; i < REPUTATION_TABLE_SIZE; i++)
	{
		if (slot_is_empty(i))
		{
			reputation_table[i].sensorid = id;
			reputation_table[i].reputation = 100;
			return i;
		}
	}
}

static uint8_t sensor_update_true_positive(sensorid_t id)
{
	uint8_t i = reputation_table_index_lookup(id);
	
	reputation_table[i].tp++;
	if (reputation_table[i].tp > reputation_table[i].td)
	  reputation_table[i].tp = reputation_table[i].td;
	return i;
}

static void update_sensor_reputation(uint8_t index, float delta, uint16_t threshold_percent)
{
	uint8_t i;
	uint8_t contribution;
	sensorid_t sensor_ID;

	i = reputation_table_index_lookup(sensor_state_table[index].sensorid);
	contribution = sensor_state_table[index].contribution * 100 / 15;

	sensor_ID = sensor_state_table[index].sensorid;
	float alert_level = get_criticality_threshold() * contribution / 100;
	uint8_t valPerDec = alert_level / (reputation_table[i].reputation / 100);
	uint8_t old_reputation = reputation_table[i].reputation;

	float new_alert_level = alert_level + delta;

	reputation_table[i].reputation = new_alert_level / valPerDec * 100;
	DEBUG("REASONING", LOG_CRITICAL, "REPUTATION MANAGEMENT: %s-%d : Adjusting reputation from %u to %u\n",
	      modality_string(modality_from_sensor_id(sensor_ID)), id_from_sensor_id(sensor_ID), old_reputation,
		reputation_table[i].reputation);
}

float compute_sensor_delta(float alarm_delta, uint8_t contribution)
{
	uint8_t contrib_percent = contribution * 100/15;
	return (alarm_delta * contrib_percent / 100);
}

void reputation_management_report_false_positive(timestamp_t timestamp, uint8_t critical_level)
{

	uint8_t i = 0, header = 0;
	uint16_t percent = 0;
	float global_delta = 0;
	
	if ((critical_level & 0x1) == 0)
		return;

	DEBUG("REASONING", LOG_CRITICAL, "REPUTATION MANAGEMENT: False positive reported for "
	      "event timestamp = %u, critical_level = %u\n", timestamp,
	      critical_level);
	
	header = reasoning_history_find_suspicious_event_index(timestamp, critical_level);
	critical_level = reasoning_history_get_event_critical_level(header);
	timestamp = reasoning_history_get_event_timestamp(header);
	
	percent = critical_level >> 1;
	if (percent > 0)
	  percent++;
	percent = (percent * 100 / 32) + 100;
	
	global_delta = (get_criticality_threshold() * percent/100.0) - (get_criticality_threshold() * 0.95);

	DEBUG("REASONING", LOG_CRITICAL, "REPUTATION MANAGEMENT: Match found with event header = %u "
	      "timestamp = %u, critical_level = %u :  global_delta = %f\n", header, timestamp, critical_level, global_delta);
	
	// Find all sensors attached to this event
	for (i = 0; i < SENSOR_STATE_COUNT; i++)
	{
		if (sensor_state_table[i].header == header && 
		sensor_state_table[i].contribution != 0)
		{
		         float sensor_delta = compute_sensor_delta(global_delta, sensor_state_table[i].contribution);
			 DEBUG("REASONING", LOG_CRITICAL, "REPUTATION MANAGEMENT: %s-%u: "
			" got a contribution of %u, delta=%f\n", 	modality_string( modality_from_sensor_id(sensor_state_table[i].sensorid) ),
			       id_from_sensor_id(sensor_state_table[i].sensorid), sensor_state_table[i].contribution, sensor_delta);
			update_sensor_reputation(i, - sensor_delta, percent);
		}
	}

}

void reputation_management_report_false_negative(timestamp_t timestamp)
{
	uint8_t i = 0, header = 0, critical_level = 0;
	uint8_t percent;
	float global_delta;

	header = reasoning_history_find_nearest_event(timestamp);
	critical_level = reasoning_history_get_event_critical_level(header);
	
	if (critical_level & 0x1)
		return;
	timestamp = reasoning_history_get_event_timestamp(header);
	percent = critical_level >> 1;
	global_delta = (get_criticality_threshold() * 1.1) - (get_criticality_threshold() * percent / 100);
	
	DEBUG("REASONING", LOG_CRITICAL, "REPUTATION MANAGEMENT: False negative reported for "
	      "event with critical_level = %u, timestamp = %u : global_delta = %f\n", critical_level, timestamp, global_delta);

	// Find all sensors attached to this event
	for (i = 0; i < SENSOR_STATE_COUNT; i++)
	{
		if (sensor_state_table[i].header == header && 
		sensor_state_table[i].contribution != 0)
		{
			float sensor_delta = compute_sensor_delta(global_delta,
				sensor_state_table[i].contribution);
			DEBUG("REASONING", LOG_CRITICAL, "REPUTATION MANAGEMENT: %s-%u: "
			" got a contribution of %u, delta=%f\n", 	modality_string( modality_from_sensor_id(sensor_state_table[i].sensorid) ),
			       id_from_sensor_id(sensor_state_table[i].sensorid), sensor_state_table[i].contribution, sensor_delta);
			update_sensor_reputation(i, sensor_delta, percent);
		}
	}
}

void reputation_management_auto_adjust(uint8_t alert_index)
{
        sensorid_t sensorid;
	uint8_t alarms;
	uint16_t i, j, id;
	float ratio;

	sensorid = reasoning_history_alert_sensorid(alert_index);
	alarms = reasoning_history_alert_involved_in_alarms(alert_index);
	
	PRINTF("REPUTATION MANAGEMENT: automatic adjustment for alert index %u sensorid: NODE %u %s-%u, involved in alarms: %s\n", alert_index, node_id_from_sensor_id(sensorid), modality_string(modality_from_sensor_id(sensorid)),
	       id_from_sensor_id(sensorid), alarms ? "TRUE" : "FALSE");

	if (alarms)
	{
		reputation_management_update_total_detection(sensorid);
		id = sensor_update_true_positive(sensorid);
		if (reputation_alarm_count)
		  ratio = 100 * ( (float)reputation_table[id].tp / reputation_table[id].td - (float)reputation_table[id].alarm_contribution / reputation_alarm_count );
		else
		  ratio = 100 * ( (float)reputation_table[id].tp / reputation_table[id].td - (float)reputation_table[id].alarm_contribution);

		PRINTF("REPUTATION MANAGEMENT: ratio=%f (%u/%u - %u/%u)\n", ratio, reputation_table[id].tp, reputation_table[id].td, reputation_table[id].alarm_contribution, reputation_alarm_count);
	}
	else
	{
		reputation_management_update_total_detection(sensorid);
		id = reputation_table_index_lookup(sensorid);
		if (reputation_alarm_count)
		  ratio = 100 * ( (float)reputation_table[id].tp / reputation_table[id].td - (float)reputation_table[id].alarm_contribution / reputation_alarm_count );
		else
		  ratio = 100 * ( (float)reputation_table[id].tp / reputation_table[id].td - (float)reputation_table[id].alarm_contribution);


		PRINTF("REPUTATION MANAGEMENT: ratio=%f (%u/%u - %u/%u)\n", ratio, reputation_table[id].tp, reputation_table[id].td, reputation_table[id].alarm_contribution, reputation_alarm_count);
	}
	
	/*
	// True positive
	if (critical_level & 0x1)
	{
	  
		// Find all correlated alerts and then increase the reputation of the corresponding sensor
		for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
		{
		  if ((reasoning_history_alert_involved_events(i) & (1 << header)) && reasoning_history_alert_involved_in_alarms(i)) 
			{
			        reputation_management_update_total_detection(reasoning_history_alert_sensorid(i));
				id = sensor_update_true_positive(reasoning_history_alert_sensorid(i));
				ratio = (float)reputation_table[id].tp / reputation_table[id].td;

				PRINTF("REPUTATION MANAGEMENT: NODE %u %s-%u: "
				       " ratio=%f (%u/%u) (correlated)\n",  node_id_from_sensor_id(reasoning_history_alert_sensorid(i)), modality_string( modality_from_sensor_id(reasoning_history_alert_sensorid(i)) ),
				       id_from_sensor_id(reasoning_history_alert_sensorid(i)), ratio, reputation_table[id].tp, reputation_table[id].td);
			}
		}
		
		// Find all non correlated sensors and decrease their reputation
		for (i = 0; i < REPUTATION_TABLE_SIZE; i++)
		{
			if (slot_is_empty(i))
				continue;
			j = sensor_contribution_index_lookup(reputation_table[i].sensorid, header);
			// The sensor is not found in the contributions table for this event,
			// so it's not correlated
			if (j == UINT8_MAX)
			  {
			    reputation_management_update_total_detection(reputation_table[i].sensorid);
			    id = reputation_table_index_lookup(reputation_table[i].sensorid);
			    ratio = (float)reputation_table[id].tp / reputation_table[id].td;

			    PRINTF("REPUTATION MANAGEMENT: NODE %u %s-%u: "
				   " ratio=%f (%u/%u) (non-correlated)\n", node_id_from_sensor_id(reputation_table[i].sensorid), modality_string( modality_from_sensor_id(reputation_table[i].sensorid) ),
				   id_from_sensor_id(reputation_table[i].sensorid), ratio, reputation_table[id].tp, reputation_table[id].td);
			    
			  }
			
		}
	}

	// True negative
	else
	{
	       // Find all correlated alert and then decrease their reputation
               for (i = 0; i < SUSPICIOUS_STATE_HISTORY_SIZE; i++)
		{
		  if (reasoning_history_alert_involved_events(i) & (1 << header))
			{
			        reputation_management_update_total_detection(reasoning_history_alert_sensorid(i));
				id = reputation_table_index_lookup(reasoning_history_alert_sensorid(i));
				ratio = (float)reputation_table[id].tp / reputation_table[id].td;

				PRINTF("REPUTATION MANAGEMENT: NODE %u %s-%u: "
				       " ratio=%f (%u/%u) (correlated)\n",  node_id_from_sensor_id(reasoning_history_alert_sensorid(i)), modality_string( modality_from_sensor_id(reasoning_history_alert_sensorid(i)) ),
				       id_from_sensor_id(reasoning_history_alert_sensorid(i)), ratio, reputation_table[id].tp, reputation_table[id].td);
			}
		}
	}
	*/
}

uint8_t reputation_management_get_sensor_reputation(sensorid_t id)
{
	uint8_t i;
	for (i = 0; i < REPUTATION_TABLE_SIZE; i++)
		if (reputation_table[i].sensorid == id)
			return reputation_table[i].reputation;
	return 100;
}

void reputation_management_update_total_detection(sensorid_t id)
{
	uint8_t i;

	i = reputation_table_index_lookup(id);

	reputation_table[i].td++;
	if (reputation_table[i].td >= 1000)
	{
		reputation_table[i].td /= 2;
		reputation_table[i].tp /= 2;
	}
}

void reputation_management_update_alarm_contribution(sensorid_t id)
{
  uint8_t i;

  i = reputation_table_index_lookup(id);
  reputation_table[i].alarm_contribution++;
}

void reputation_management_undo_tp(sensorid_t id)
{
    uint8_t i;
    i = reputation_table_index_lookup(id);
    reputation_table[i].tp_reported = false;
}

void reputation_management_init()
{
        int i;

	memset(reputation_table, 0, REPUTATION_TABLE_SIZE * sizeof(reputation_t));
	for (i = 0; i <  REPUTATION_TABLE_SIZE; i++)
	    reputation_table[i].sensorid = UINT8_MAX;
	reputation_alarm_count = 0;
}
#endif

