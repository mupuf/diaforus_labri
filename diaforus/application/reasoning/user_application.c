/*
 * user_application.c
 *
 * Author: Nicola Costagliola
 * Author: Martin Peres
 * Author: Romain Perier
 * Author: Hassen Ghariani <gharianihassen@gmail.com>
 */

/* Only for reasoning debugging */
#include "application_config.h"

#ifdef  APPLICATION_REASONING

#include <FreeRTOS.h>
#include <task.h>
#include <croutine.h>
#include <stdlib.h>

#include "reasoning_debug.h"

#include "user_application.h"
#include "reasoning_service.h"
#include "reputation_management.h"
#include "reasoning_service_p.h"
#include "hooks.h"

#include "sensors_drivers.h"
#include "sensor_seismic.h"
#include "sensor_pir.h"
#include "sensor_spirit.h"
#include "sensor_switch.h"

#include "sensors_config.h"
#include "reasoning_config.h"
#include "coap_service.h"
#include "debug_led_mapping.h"
#if IS_SIMU
#include <unistd.h>
#endif

static bool acked_by_reasoning = false;
static timestamp_t next_periodic_ack_check = 0;
static uint8_t bootstraping_ack_sub = -1;
uint8_t sensor_count = SENSOR_COUNT;

#if IS_SIMU
static void *crash_handler(int signum);
#endif

#if !IS_SIMU
#include "gpio_public.h"
    #if (HW_MODALITY==WITH_ACTUATOR)
		u8 actuator_cnt;
	#endif
#endif   
		
#define BOOTSTRAPING_CHECK_PERIOD 20000

/*----------------------------------------------------------------------------*/

#if 0
static bool sensor_criticality_analog(sensor_t* sensor)
{
	uint8_t value = sensor_history_value(&sensor->history, 0);

	/* Nothing happened recently, we restart the value exported to the reasoning node */
	if ((value >= sensor->abs_threshold) && sensor_can_emit_alert(sensor))
	{
		DEBUG("APP", LOG_DEBUG, "%s-%i: Absolute alert: threshold=%i, alert_delta=%ims\n",
		      modality_string(sensor->modality), sensor->id,
		      sensor->abs_threshold, time_get() - sensor->last_alert);
		sensor->last_alert = time_get();
		sensor->normalized_value = 1;
		return true;
	}

	/* Fast variations monitoring */
	portTickType diffTime = time_get() - sensor->last_alert;
	if ((value >= sensor->abs_threshold) && (diffTime >= CRITICALITY_VARIATION_TIME))
	{
		uint8_t polled_values = 0, i = 0, sum = 0;

		polled_values = diffTime / sensor->periodicity;

		for (i = 0; i < polled_values; i++)
		{
			uint8_t v = sensor_history_value(&sensor->history, i);
			DEBUG("APP", LOG_DEBUG, "%s-%i: bitmap[%d] = %u\n", modality_string(sensor->modality), sensor->id, i, v);
			sum += v;
		}

		if ((sum * 100 / polled_values) >= 75)
		{
			sensor->normalized_value++;
			if (sensor->normalized_value > 3)
				sensor->normalized_value = 3;
		}
		else if (sensor->normalized_value > 1)
			sensor->normalized_value--;

		DEBUG("APP", LOG_DEBUG, "%s-%u: Periodic alert: threshold=%u, polled_values=%u, normalized_value=%u, alert_delta=%ums\n",
		      modality_string(sensor->modality), sensor->id,
		      sensor->abs_threshold, polled_values, sensor->normalized_value, diffTime);
		sensor->last_alert = time_get();
		return true;
	}

	return false;
}
#endif

static bool sensor_criticality_digital(sensor_t* sensor)
{
	uint8_t value = sensor->last_value;

	/* Nothing happened recently, we restart the value exported to the reasoning node */
	if ((value >= sensor->abs_threshold) && sensor_can_emit_alert(sensor))
	{
		DEBUG("APP", LOG_DEBUG, "%s-%i: Absolute alert: threshold=%i, alert_delta=%ims\n",
		      modality_string(sensor->modality), sensor->id,
		      sensor->abs_threshold, time_get() - sensor->last_alert);

		sensor->last_alert = time_get();

		if (sensor->normalized_value < 3)
			sensor->normalized_value++;

		return true;
	} else if (sensor->normalized_value > 0)
		sensor->normalized_value--;

	return false;
}

static bool sensor_criticality_pir_seismic(sensor_t* sensor)
{
	uint8_t value = sensor->last_value;

	/* Nothing happened recently, we restart the value exported to the reasoning node */
	if ((value >= sensor->abs_threshold) && sensor_can_emit_alert(sensor))
	{
		sensor->last_alert = time_get();

		sensor->normalized_value = 3;
		return true;
	}
	sensor->normalized_value = 0;

	return false;
}

static bool sensor_criticality(sensor_t *sensor)
{
	switch (sensor->modality)
	{
	case PIR_MOD:
	case SEISMIC_MOD:
	case SPIRIT_MOD:	
	case SWITCH_MOD:
		return sensor_criticality_pir_seismic(sensor);
	
		//return sensor_criticality_digital(sensor);
	case INVALID_MOD:
	case NUMBER_OF_MODALITIES:
		break;
	}
	return false;
}


value_t sensor_poll(sensor_t *sensor)
{
	value_t value;

	/* set when the next read should happen */
	sensor->next_read = time_get() + sensor->periodicity;

	switch (sensor->modality)
	{
	case PIR_MOD:
		value = sensor_poll_pir(sensor);
		break;
	case SPIRIT_MOD:
		value = sensor_poll_spirit(sensor);
		break;
	case SEISMIC_MOD:
		value = sensor_poll_seismic(sensor);
		break;
	case SWITCH_MOD:
		value = sensor_poll_switch(sensor);
		break;	
	default:
		value = 0;
	}

	DEBUG("APP", LOG_DEBUG, "SENSOR %s ID %d : Read (%i)\n",
			modality_string(sensor->modality), sensor->id, value);

	return value;
}

void application()
{
	DEBUG("APP", LOG_DEBUG, "Application started");


	acked_by_reasoning = false;

	// Memory initialization (for real nodes)
	sensors_init();
	coap_service_init();//to remove because now this call is in hk_appli_initialization2()
#if ROLE_REASONING
	reasoning_history_init();
	reputation_management_init();
#endif

	debug_led(LED_APPLICATION_STARTED, 1);

	/* Init the sensors */
	sensors_drivers_init(sensors, SENSOR_COUNT);

	/* --- Init reasoning unconditionally ---*/
	reasoning_init();
 
#if !ROLE_REASONING
#if !IS_SIMU && (HW_MODALITY==WITH_ACTUATOR)
	actuator_cnt = 0;
	const char * subListAttributes[5] = {"AlmLvl", "AlmTsp" ,"AlmAr", "AlmLst","AlmDrt" };
	Operator subListOperators[5] = { GE, GE, GE, GE, GE };
	value_t subListValues[5] = {0, 0, 0, 0, 0};
	acked_by_reasoning =true;
	bootstraping_ack_sub = Subscribe(subListAttributes, subListOperators, subListValues, 5);
#else
	/* Bootstraping */
	const char * subListAttributes[2] = { "BTCN", "BTCNB" };
	Operator subListOperators[2] = { EQ, GE };
	value_t subListValues[2] = { NODE_ID, 0 };

	bootstraping_ack_sub = Subscribe(subListAttributes, subListOperators, subListValues, 2);
	
	const char * pubAttributes[] = { "BTNEWN", "BTAREA", "BTNB" };
	value_t pubValues[] = { NODE_ID, AREA_ID, SENSOR_COUNT };

	Publish(pubAttributes, pubValues, 3, 0);
	next_periodic_ack_check = time_get() + BOOTSTRAPING_CHECK_PERIOD;
#endif
#endif
	DEBUG("APP", LOG_DEBUG, "Application has started\n");
}

void iterative_tasks()
{
	int i;

	DEBUG("APP", LOG_DEBUG, "Iterative_tasks():\n");
#if !ROLE_REASONING
	if (!acked_by_reasoning && time_get() >= next_periodic_ack_check) {
		const char * pubAttributes[] = { "BTNEWN", "BTAREA", "BTNB" };
		value_t pubValues[] = { NODE_ID, AREA_ID, SENSOR_COUNT };

		DEBUG("BOOTSTRAPING", LOG_INFO, "Timeout expired for new node registration\n");
		Publish(pubAttributes, pubValues, 3, 0);
		next_periodic_ack_check = time_get() + BOOTSTRAPING_CHECK_PERIOD;
	}
#endif
	/* poll the connected sensors and emit alerts if needed */
	for (i = 0; i < SENSOR_COUNT; i++)
	{
#if !IS_SIMU && (HW_MODALITY==WITH_ACTUATOR)
	    if (actuator_cnt > 0){
	    	actuator_cnt --;
	    	sensors_drivers_write_value(&sensors[i],1 );
	    } else {
	    	sensors_drivers_write_value(&sensors[i],0 );
	    }
	    
#else		
	    debug_led(LED_SWITCH_0_PUBLISH, 0);

		if (time_get() < sensors[i].next_read)
			continue;

		sensor_poll(&sensors[i]);
		if (sensor_criticality(&sensors[i]))
		{
			const char * pubAttributes[3];
			value_t pubValues[3];

			pubAttributes[0] = "SENSID";
			pubValues[0] = sensor_id(&sensors[i]);

			pubAttributes[1] = "VAL";
			pubValues[1] = sensors[i].normalized_value;

			pubAttributes[2] = "AREA";
			pubValues[2] = AREA_ID;

			debug_led(LED_SWITCH_0_PUBLISH, 1);
			Publish(pubAttributes, pubValues, 3, 0);
		}
#endif
	}
	DEBUG("APP", LOG_DEBUG, "</Iterative_tasks>\n\n");
}

void Notify(const char * const attributes[], const value_t values[], uint8_t subscriptionId)
{
	DEBUG("APP", LOG_DEBUG, "Notification data received (sub_id = %i)\n");
#if ROLE_REASONING
	if (!reasoning_update(attributes, values, subscriptionId))
		return;
#endif
	if (subscriptionId == bootstraping_ack_sub) {
#if !IS_SIMU && (HW_MODALITY==WITH_ACTUATOR)
		acked_by_reasoning = true;
		actuator_cnt = 10;
#else	
		acked_by_reasoning = (values[0] == NODE_ID && values[1] == SENSOR_COUNT);
		DEBUG("BOOTSTRAPING", LOG_INFO, "Confirmation received from reasoning (acked = %u)\n", acked_by_reasoning);
#endif
	}
	/* The notification isn't meant for the reasoning nor the gateway */
}


void simulate_stimulus(uint8_t sensor_id, uint8_t value)
{
	modality_t modality = (sensor_id >> 4) & 0xf;
	uint8_t id = sensor_id & 0xf, i = 0;

	DEBUG("APP", LOG_DEBUG, "Stimulus received for sensor %s-%u : value=%u\n",
	      modality_string(modality), id, value);

	if (modality == SPIRIT_MOD) {
		DEBUG("APP", LOG_DEBUG,
		      "Stimulus received for beam %u : value = %u\n",
		      (value >> 4) & 0xf, value & 0xf);
	}

	for (i = 0; i < SENSOR_COUNT; i++)
	{
		if (sensors[i].modality == modality && sensors[i].id == id)
		{
			if (modality == PIR_MOD || modality == SEISMIC_MOD)
				sensors[i].stimulus[0] = value;
			else if (modality == SPIRIT_MOD)
			{
				uint8_t beam = (value >> 4) & 0xf;
				sensors[i].stimulus[beam] = value & 0xf;
			}
			return;
		}
	}

	DEBUG("APP", LOG_CRITICAL,
	      "Stimulus received for a non-existing sensor %s-%u : value=%u\n",
	      modality_string(modality), id, value);
}
#endif
