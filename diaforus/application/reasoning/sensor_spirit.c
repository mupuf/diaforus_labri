#include "sensor_spirit.h"
#include "sensors_drivers.h"
#include "reasoning_service_p.h"

// #include <math.h>


value_t sensor_poll_spirit(sensor_t *sensor)
{
	
#if (IS_SIMU)
	uint8_t i;
	int value;
	for (i = 0; i < SENSOR_SPIRIT_BEAM_COUNT; i++)
	{
		value = sensors_drivers_read_value(sensor);
		if (sensor->stimulus[i] != UINT8_MAX)
		{
			value = sensor->stimulus[i] & 0x1;
			sensor->stimulus[i] = UINT8_MAX;
		}
		sensor_history_add(&sensor->history, value);
	}
#else
	uint8_t value = (uint8_t) sensors_drivers_read_value(sensor);
	sensor_history_add(&sensor->history, value);
	sensor->last_value = value;
#endif
	
	return 0;
}

bool sensor_criticality_spirit(sensor_t* sensor)
{
	uint8_t i, value;

	/* Nothing happened recently, we restart the value exported to the reasoning node */
	if (sensor_can_emit_alert(sensor))
	{
		for (i = 0; i < SENSOR_SPIRIT_BEAM_COUNT; i++)
		{
			value = sensor_history_value(&sensor->history, i);

			if (value >= sensor->abs_threshold)
			{
				DEBUG("APP", LOG_DEBUG, "%s-%i: Absolute alert: threshold=%i, alert_delta=%ims\n",
					modality_string(sensor->modality), sensor->id,
					sensor->abs_threshold, time_get() - sensor->last_alert);

				sensor->last_alert = time_get();
				sensor->normalized_value = 1;
				sensor->old_variation = 6;
				return true;
			}
		}
	}

	/* Fast variations monitoring */
	value = 0;
	for (i = 0; i < SENSOR_SPIRIT_BEAM_COUNT; i++)
		value += sensor_history_value(&sensor->history, i);

	portTickType diffTime = time_get() - sensor->last_alert;

	if ((value >= sensor->abs_threshold) && (diffTime >= CRITICALITY_VARIATION_TIME))
	{
		uint16_t polled_reads = (diffTime / sensor->periodicity);
		uint16_t i = 0, j = 0, tmp = 0, sum = 0;
		
		for (i = 0; i < polled_reads; i++)
		{
			tmp = 0;
			for (j = 0; j < SENSOR_SPIRIT_BEAM_COUNT; j++)
			{
				uint8_t v = sensor_history_value(&sensor->history, (i * SENSOR_SPIRIT_BEAM_COUNT) + j);
				tmp += v;
			}
			sum += tmp;
		}

		tmp = (sum * 100) / (SENSOR_SPIRIT_BEAM_COUNT * polled_reads);
		
	        DEBUG("APP", LOG_DEBUG, "%s-%i: Relative variation:  average=%u percent (sum=%u, polled_reads=%u), alert_delta=%ims\n",
		       modality_string(sensor->modality), sensor->id, tmp, sum, polled_reads, diffTime);

		if (tmp > sensor->old_variation)
		{
			sensor->old_variation = tmp;
			DEBUG("APP", LOG_DEBUG, "%s-%i: Relative variation:  increasing variation to %u\n",
			       modality_string(sensor->modality), sensor->id, sensor->old_variation);

			if (sensor->normalized_value <= 2)
				sensor->normalized_value++;
		}
#if 0
		else if (tmp < sensor->old_variation)
		{
			sensor->old_variation = tmp;
			DEBUG("APP", LOG_DEBUG, "%s-%i: Relative variation:  decreasing variation to %u\n",
			       modality_string(sensor->modality), sensor->id, sensor->old_variation);
			if (sensor->normalized_value > 1)
				sensor->normalized_value--;
		}
#endif
		sensor->last_alert = time_get();
		return true;
	}
	return false;
}
