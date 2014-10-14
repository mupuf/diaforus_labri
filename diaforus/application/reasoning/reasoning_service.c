/*
 * reasoning_service.c
 *
 *      Author: maissa
 *      Author:	martin <martin.peres@ensi-bourges.fr>
 *	Author:	Hassen Ghariani <gharianihassen@gmail.com>
 */

#include <FreeRTOS.h>
#include <task.h>
#include "reasoning_debug.h"

#include "reasoning_service.h"
#include "reasoning_service_p.h"
#include "reputation_management.h"

#include <stdint.h>


/*********** Static configuration ***********/
#include "reasoning_config.h"
#include "monitored_areas_config.h"

/*********** Type definitions ***********/
#define ERROR_VALUE -1 // not used
#define KNOWN_NODES_SIZE 16

/*********** Global variables ***********/
sensors_update_t sensors_updates __attribute__ (( section (".slowdata") ));

/// Threshold before sending an alarm concerning the whole area
static int criticality_threshold;
static int latency_mode = LATENCY_MODE;
static uint32_t min_intrusion_duration = MIN_INTRUSION_DURATION;
static uint32_t max_intrusion_duration = MAX_INTRUSION_DURATION;

static uint8_t reasoning_sub = -1;
static uint8_t reasoning_bootstraping_sub = -1;
static uint8_t failure_sub = 0;
static uint8_t monitored_areas_subs[MONITORED_AREAS_COUNT];
static uint16_t known_nodes[ KNOWN_NODES_SIZE];

uint16_t reasoning_alarm_count = 0, reasoning_alert_count = 0;

/*********** COAP accessors/mutators ***********/
int
get_criticality_threshold()
{
	return criticality_threshold * latency_mode / 100;
}

int
set_criticality_threshold(int value)
{
	criticality_threshold = value;
	return 0;
}

int
get_latency_mode()
{
	return latency_mode;
}

int
set_latency_mode(int mode)
{
	if (mode < 25 || mode > 100)
		return ERROR_VALUE;
	latency_mode = mode;
	return 0;
}

uint32_t
get_max_intrusion_duration()
{
	return max_intrusion_duration;
}

int
set_max_intrusion_duration(uint32_t duration)
{
	max_intrusion_duration = duration;
	return 0;
}

uint32_t
get_min_intrusion_duration()
{
	return min_intrusion_duration;
}

int
set_min_intrusion_duration(uint32_t duration)
{
	min_intrusion_duration = duration;
	return 0;
}

#if ROLE_REASONING

/******** Sensor history helpers ********/

static uint8_t sensors_find_suitable_index(sensorid_t sensor_ID)
{
	portTickType duration = time_get() - sensors_updates.sensors[0].time;
	uint8_t i, oldest = 0;

	for (i = 0; i < sensors_updates.size; i++)
	{
		if (sensors_updates.sensors[i].sensor_ID == sensor_ID)
			return i;
		if (time_get() - sensors_updates.sensors[i].time > duration)
		{
			oldest = i;
			duration = time_get() - sensors_updates.sensors[i].time;
		}
	}

	/* this sensor_id isn't in the table:
	 * - push it back in the table if there is some space left
	 * - replace the oldest value otherwise
	 */
	if (sensors_updates.size < ALERT_HISTORY_SIZE)
		return sensors_updates.size++;
	else
		return oldest;
}


static void sensor_add_event(sensorid_t sensor_ID, value_t value)
{
	uint8_t index = sensors_find_suitable_index(sensor_ID);

	sensors_updates.sensors[index].sensor_ID = sensor_ID;
	sensors_updates.sensors[index].time = time_get();
	sensors_updates.sensors[index].value = value;

	uint16_t criticality = get_criticality_level();

	reasoning_alert_count++;

	DEBUG("REASONING", LOG_CRITICAL, "Received alert from NODE_ID = %d, modality = %s, ID = %d, value = %d. "
	       "Criticality = %u\n",
	       node_id_from_sensor_id(sensor_ID),
	       modality_string(modality_from_sensor_id(sensor_ID)),
	       id_from_sensor_id(sensor_ID),
	       value,
	       criticality);

	/*if (reasoning_history_is_full())
	{
		reputation_management_update_total_detection(sensor_ID);
		reputation_management_undo_tp(sensor_ID);
		}*/
	reasoning_history_register_new_alert(sensor_ID, 0, 0);
	reasoning_history_update(criticality);
}

#if MONITORED_AREAS_COUNT
static void area_add_event(uint8_t index, uint16_t percent)
{
	monitored_areas[index].value = percent;
	monitored_areas[index].previous_time = monitored_areas[index].time;
	monitored_areas[index].time = time_get();
	
#if IS_SIMU
	uint16_t criticality = get_criticality_level();
	
	DEBUG("REASONING", LOG_CRITICAL, "Received alarm from monitored area = %u, percent = %u. Criticality = %u\n",
	       monitored_areas[index].area,
	       percent,
	       criticality);
#endif
}
#endif

static void compute_criticality_threshold()
{
	int nb_sensors = 0, i;
	for (i = 0; i <  KNOWN_NODES_SIZE; i++) {
			if (known_nodes[i] != 0)
				nb_sensors += known_nodes[i] & 0xff;
	}
	criticality_threshold = nb_sensors * 150;
}

/********** Sensor correlation **********/

/*----------------------------------------------------------------------------*/
// function that will calculate the Alarm level
uint16_t get_criticality_level()
{
	uint16_t criticality = 0;
	uint8_t i, reputation;

#if MONITORED_AREAS_COUNT
	for (i = 0; i < MONITORED_AREAS_COUNT; i++)
	{
		uint8_t value = 2;
		uint32_t linear_time =  monitored_areas[i].crossing_duration + get_max_intrusion_duration() * 2;

		if (monitored_areas[i].previous_time && (monitored_areas[i].time - monitored_areas[i].previous_time <= linear_time)) {
			value++;
			if (value > 3)
				value = 3;
		}
		criticality += compute_linear_criticality(monitored_areas[i].time, linear_time) * value;
	}
#endif

	if (criticality > (get_criticality_threshold() * 0.5))
		criticality = get_criticality_threshold() * 0.5;
	
	for (i = 0; i < sensors_updates.size; i++)
	{
		reputation = reputation_management_get_sensor_reputation(sensors_updates.sensors[i].sensor_ID);
		criticality += (compute_linear_criticality(sensors_updates.sensors[i].time, get_max_intrusion_duration() * 2) * sensors_updates.sensors[i].value * reputation/100);
	}
	return criticality;
}

/*********** Public functions ***********/

bool
reasoning_enabled()
{
#if ROLE_REASONING
	return true;
#else
	return false;
#endif
}

void
reasoning_init()
{
#if MONITORED_AREAS_COUNT
    int8_t i;
#endif

	if (reasoning_enabled())
	{
		// Initialize memory for real nodes
		criticality_threshold = 0;
		latency_mode = LATENCY_MODE;
		reasoning_sub = -1;
		reasoning_bootstraping_sub = -1;
		failure_sub = 0;
		memset(sensors_updates.sensors, 0, ALERT_HISTORY_SIZE * sizeof(sensor_update_t));
		memset(monitored_areas_subs, 0, MONITORED_AREAS_COUNT);
		memset(known_nodes, 0, KNOWN_NODES_SIZE * sizeof(uint16_t));
		reasoning_alarm_count = 0;
		reasoning_alert_count = 0;

	    // Alerts subscriptions

		//Subscription parameters to request the advertised attribute list to the broker
	    const char * subListAttributes[5] = { "SENSID", "VAL", "AREA" };
		Operator subListOperators[5] = { GE, GE, EQ }; // GE     Greater or Equal    ">="
		value_t subListValues[5] = { 0, 0, AREA_ID };

 		reasoning_sub = Subscribe(subListAttributes, subListOperators, subListValues, 3);

		// Boostraping subscriptions, used to compute the criticality threshold of the current area
		subListAttributes[0] = "BTNEWN";
		subListAttributes[1] = "BTAREA";
		subListAttributes[2] = "BTNB";

		subListOperators[0] = GE;
		subListOperators[1] = EQ;
		subListOperators[2] = GE;

		subListValues[0] = 0;
		subListValues[1] = AREA_ID;
		subListValues[2] = 0;

		reasoning_bootstraping_sub = Subscribe(subListAttributes, subListOperators, subListValues, 3);

		// Areas monitoring subscription(s)

		subListAttributes[0] = "AlmLvl";
		subListAttributes[1] = "AlmTsp";
		subListAttributes[2] = "AlmAr";
		subListAttributes[3] = "AlmLst";
		subListAttributes[4] = "AlmDrt";

		subListOperators[0] = GE;
		subListOperators[1] = GE;
		subListOperators[2] = EQ;
		subListOperators[3] = GE;
		subListOperators[4] = GE;
		
		subListValues[0] = 0;
		subListValues[1] = 0;
		subListValues[3] = 0;
		subListValues[4] = 0;

#if MONITORED_AREAS_COUNT
		for (i = 0; i < MONITORED_AREAS_COUNT; i++)
		{
			subListValues[2] = monitored_areas[i].area;
			monitored_areas_subs[i] = Subscribe(subListAttributes, subListOperators, subListValues, 5);
		}
#endif
		compute_criticality_threshold(AREA_ID, sensor_count);

		// Subscription for node failure

		subListAttributes[0] = "FAIL";
		subListAttributes[1] =  "NODEID";

		subListOperators[0] = EQ;
		subListOperators[1] = GE;

		subListValues[0] = 1;
		subListValues[1] = 0;

		failure_sub = Subscribe(subListAttributes, subListOperators, subListValues, 2);

		DEBUG("REASONING", LOG_INFO, "Activated\n");
	}
	else
		DEBUG("REASONING", LOG_DEBUG, "None\n");
	min_intrusion_duration = MIN_INTRUSION_DURATION;
	max_intrusion_duration = MAX_INTRUSION_DURATION;
}

int
reasoning_update(const char * const attributes[], const value_t values[], uint8_t subscriptionId)
{
        uint8_t i;

	if (subscriptionId == reasoning_bootstraping_sub)
	{
		const char * pubAttributes[] = { "BTCN", "BTCNB" };
		value_t pubValues[] = { values[0], values[2] };

		

		for (i = 0; i <  KNOWN_NODES_SIZE; i++) {
			if ((known_nodes[i] >> 8) == values[0] &&
			    (known_nodes[i] & 0xff) == values[1]) {
				DEBUG("BOOTSTRAPING", LOG_INFO, "Node %u has already been registered with %u sensors\n", values[0], values[2]);
				goto confirmation;
			}
		}
		for (i = 0; i <  KNOWN_NODES_SIZE; i++)
			if (known_nodes[i] == 0)
				break;
		known_nodes[i] = (values[0] << 8) | (values[2] & 0xff);
		DEBUG("BOOTSTRAPING", LOG_INFO, "Registering node with %d sensors (area = %u), adjusting the criticality threshold\n", values[1], values[2]);
		compute_criticality_threshold();
		DEBUG("BOOTSTRAPING", LOG_INFO, "Sending confirmation to node %u with %u sensors\n", values[0], values[2]);
confirmation:
		Publish(pubAttributes, pubValues, 2, 0);
		return 0;
	}
	else if (subscriptionId == reasoning_sub)
	{
		// values[0] : sensor_ID of the sensor that made the update
		// values[1] : value captured from the sensor that sent the update
		sensor_add_event(values[0], values[1]);
		return 0;
	}
	else if (subscriptionId == failure_sub)
	{
		// remove the sensor node from the known_nodes table
		for (i = 0; i <  KNOWN_NODES_SIZE; i++) {
			if ((known_nodes[i] >> 8) == values[1]) {
				DEBUG("BOOTSTRAPING", LOG_INFO,
				      "Received a notification for the failure of node %d (%d sensors)\n",
				       values[1], known_nodes[i] & 0xff);
				known_nodes[i] = 0;
				compute_criticality_threshold();
				return 0;
			}
		}
		DEBUG("BOOTSTRAPING", LOG_INFO,
			"Received a notification for the failure of a unknown node %d\n",
			values[1]);
	}
	else
	{
#if MONITORED_AREAS_COUNT
		for (i = 0; i < MONITORED_AREAS_COUNT; i++)
		{
			if (subscriptionId == monitored_areas_subs[i])
			{
				area_add_event(i, values[0]);
				return 0;
			}
		}
#endif
	}
		return 1;
}

void reasoning_emit_alarm(uint16_t criticality, timestamp_t timestamp, uint32_t involved_sensors)
{
	const char * pubAttributes[] = { "AlmLvl", "AlmTsp", "AlmAr", "AlmLst", "AlmDrt" };
	value_t pubValues[] = { criticality/100, timestamp, AREA_ID, involved_sensors, 0};
	timestamp_t firstAlert = UINT32_MAX;
	int i;

	reasoning_alarm_count++;

	for (i = 0; i < sensors_updates.size; i++) {
		if (compute_linear_criticality(sensors_updates.sensors[i].time, get_max_intrusion_duration() * 2) == 0)
			continue;
		if (sensors_updates.sensors[i].time < firstAlert)
			firstAlert = sensors_updates.sensors[i].time;
	}
	pubValues[4] = timestamp - firstAlert;

	DEBUG("REASONING", LOG_INFO, "Sending an alarm : criticality = %u, timestamp = %u, area = %u, nodelist = %u, duration = %u, ratio = %f (%u/%u)\n",
	      criticality/100,
	      timestamp, AREA_ID, involved_sensors, pubValues[4], (float)reasoning_alarm_count/reasoning_alert_count, reasoning_alarm_count, reasoning_alert_count);

	
	Publish(pubAttributes, pubValues, 5, 0);
}
#else /* Not a reasoning node */
void
reasoning_init()
{
	DEBUG("REASONING", LOG_DEBUG, "None\n");
	min_intrusion_duration = MIN_INTRUSION_DURATION;
	max_intrusion_duration = MAX_INTRUSION_DURATION;
}
#endif
