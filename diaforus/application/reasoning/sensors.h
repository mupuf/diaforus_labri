#ifndef SENSORS_H_
#define SENSORS_H_

#include "reasoning_common.h"
#include "pubsub_common.h"
#include "history.h"

#define SENSOR_SPIRIT_BEAM_COUNT 1

extern uint32_t get_min_intrusion_duration();
typedef uint16_t sensorid_t;

typedef struct
{
#if IS_SIMU
	bool event;
	bool pulsation;
	uint8_t t_start;
	uint8_t t_duration;
	value_t amplitude_max;
#endif
} sensor_seismic_priv_t;

typedef struct
{
#if IS_SIMU
	int t_prime;
#endif
} sensor_pir_priv_t;

/// represents the different modalities
typedef enum {
	INVALID_MOD = -1,
	PIR_MOD = 0,
	SPIRIT_MOD = 1,
	SEISMIC_MOD = 2,
	SWITCH_MOD = 3,
	NUMBER_OF_MODALITIES,
} modality_t;

enum sensor_connectity_t {
	CONNECT_GPIO=1,
	CONNECT_ADC=2,
	CONNECT_I2C=3
};

struct sensor_connectity_i2c_t
{
	uint8_t address;
	uint8_t reg;
};

enum value_type_t
{
	DIGITAL_VALUE=0,
	ANALOG_VALUE=1
};

struct calibration_analog_t
{
	int16_t offset;
	int8_t mult;
	int8_t div;
};

typedef struct
{
	/* characteristics */
	modality_t modality;
	uint8_t id;
	enum sensor_connectity_t connection;
	union
	{
		uint8_t gpio;
		uint8_t adc;
		struct sensor_connectity_i2c_t i2c;
	} port;
	
	enum value_type_t value_type;
	union
	{
		struct calibration_analog_t analog;
		bool invert;
	} calibration;

	/* reading */
	uint16_t periodicity;
	uint16_t reemission_delay;
	uint32_t next_read;
	value_t stimulus[SENSOR_SPIRIT_BEAM_COUNT];
	history_t history;
	value_t last_value;

	/* alerts */
	value_t abs_threshold;
	value_t rel_threshold;
	value_t old_variation;
	uint8_t normalized_value;
	portTickType last_alert;

	/* private extra storage */
	union
	{
		sensor_seismic_priv_t seismic_priv;
		sensor_pir_priv_t pir_priv;
	};
} sensor_t;

static inline const char* modality_string(modality_t modality)
{
	switch(modality)
	{
	case PIR_MOD:
		return "PIR";
	case SPIRIT_MOD:
		return "SPIR";
	case SEISMIC_MOD:
		return "SEISMIC";
	case SWITCH_MOD:
			return "SWITCH";
	default:
		return "INVALID";
	}
}
static inline bool sensor_can_emit_alert(sensor_t* sensor)
{
	return sensor->last_alert == 0 || (time_get() - sensor->last_alert) > sensor->reemission_delay;
}

/****************** SensorID helpers ***********************************/
static inline sensorid_t sensor_id(sensor_t *s)
{
	return NODE_ID << 8 | (s->modality & 0xf) << 4 | (s->id & 0xf);
}
static inline uint8_t node_id_from_sensor_id(sensorid_t sensorID)
{
	return sensorID >> 8;
}
static inline modality_t modality_from_sensor_id (sensorid_t sensorID)
{
	return (sensorID >> 4) & 0xf;
}
static inline uint8_t id_from_sensor_id (sensorid_t sensorID)
{
	return (sensorID) & 0xf;
}

/**************** Calibration helpers **********************************/
static inline value_t sensor_calibration_digital(sensor_t *sensor, value_t value)
{
	if (sensor->calibration.invert == true)
		return !value;
	else
		return value;
}

#endif
