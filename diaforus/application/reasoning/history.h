/*
 * history.h
 *      Author: Romain Perier <romain.perier@labri.fr>
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>
#include "reasoning_config.h"

typedef struct
{
	uint8_t history[SENSOR_HISTORY_SIZE];
	uint16_t start;
	uint16_t end;
} history_t;

uint8_t sensor_history_value(history_t *h, uint16_t pos);
uint8_t sensor_history_add(history_t *h, uint8_t value);

#endif /* HISTORY_H */
