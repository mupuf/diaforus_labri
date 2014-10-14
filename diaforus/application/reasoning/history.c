/*
 * history.c
 *      Author: Romain Perier <romain.perier@labri.fr>
 */

#include "history.h"
#include "reasoning_debug.h"

uint8_t sensor_history_value(history_t *h, uint16_t pos)
{
	uint16_t index_byte = 0, index_bit = 0;
	int16_t tmp = 0;
	uint8_t value = 0;

	if (h->end <= pos)
	{
		tmp = h->end - pos - 1;
		do
			tmp += (SENSOR_HISTORY_SIZE * 8);
		while(tmp < 0);
		index_byte = tmp / 8;
		index_bit = (SENSOR_HISTORY_SIZE * 8) + (h->end - pos - 1);
	}
	else
	{
		index_byte = (h->end - pos - 1) / 8;
		index_bit = h->end - pos - 1;
	}
	index_bit %= 8;
	value = h->history[index_byte];

	DEBUG("APP", LOG_DEBUG, "%s: pos=%u, start=%u, end=%u, index_byte=%u, index_bit=%u\n", __func__, pos, h->start, h->end, index_byte, index_bit);
	return (value >> index_bit) & 0x1;
}

uint8_t sensor_history_add(history_t *h, uint8_t value)
{
	uint16_t index_byte = h->end / 8;
	uint16_t index_bit = h->end % 8;

	DEBUG("APP", LOG_DEBUG, "%s: add value=%u to start = %u, end=%u, index_byte=%u, index_bit=%u\n", __func__, value, h->start, h->end, index_byte, index_bit);

	value &= 1;

	h->history[index_byte] &= ~(1 << index_bit);
	h->history[index_byte] |= (value << index_bit);

	h->end = (h->end + 1) % (SENSOR_HISTORY_SIZE * 8);
	if (h->end == h->start)
		h->start = (h->start + 1) % (SENSOR_HISTORY_SIZE * 8);
	return value;
}
