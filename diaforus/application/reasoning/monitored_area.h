/*
 * MONITORED_AREA.h
 *      Author: Romain Perier <romain.perier@labri.fr>
 */

#ifndef MONITORED_AREA_H
#define MONITORED_AREA_H

#include <stdint.h>
#include <FreeRTOS.h>

typedef struct
{
    uint8_t area;
    portTickType crossing_duration;
    uint16_t value;
    portTickType previous_time;
    portTickType time;
} monitored_area_t;

#endif /* MONITORED_AREA_H */
