/*
 * coap_service.c
 *      Author: Romain Perier <romain.perier@labri.fr>
 */
#include "application_config.h"
#ifdef  APPLICATION_REASONING
#include "coap_service.h"

#include <stdint.h>
#include <stdio.h>
#include "rest.h"
#include "udp.h"
#include "reasoning_history.h"
#include "reputation_management.h"
#include "reasoning_service_p.h"

#define WELL_KNOWN_VERSION 1

static u8 history_buffer[244]  __attribute__(( section(".slowdata") ));

RESOURCE_RW(min_intrusion_duration, get_min_intrusion_duration_handler, set_min_intrusion_duration_handler);
RESOURCE_RW(max_intrusion_duration, get_max_intrusion_duration_handler, set_max_intrusion_duration_handler);
RESOURCE_RW(latency_mode, get_latency_mode_handler, set_latency_mode_handler);
RESOURCE_RO(well_known, get_well_known);

#if ROLE_REASONING
RESOURCE_RW(hist_analyze_per, get_history_analyze_period_handler, set_history_analyze_period_handler);
RESOURCE_RO(criticality_lvl, get_criticality_lvl);
RESOURCE_RO(history_part1, get_history_part1);
RESOURCE_RO(history_part2, get_history_part2);
RESOURCE_RO(history_sensor_part1, get_history_sensor_part1);
RESOURCE_RO(history_sensor_part2, get_history_sensor_part2);
RESOURCE_RO(history_sensor_part3, get_history_sensor_part3);

RESOURCE_WO(false_positive, report_false_positive);
RESOURCE_WO(false_negative, report_false_negative);

RESOURCE_RO(alert_alarm_ratio, get_alert_alarm_ratio);
#endif

void application_coap_service_init()
{
    INIT_RESOURCE_RW(min_intrusion_duration, get_min_intrusion_duration_handler, set_min_intrusion_duration_handler);
    INIT_RESOURCE_RW(max_intrusion_duration, get_max_intrusion_duration_handler, set_max_intrusion_duration_handler);
    INIT_RESOURCE_RW(latency_mode, get_latency_mode_handler, set_latency_mode_handler);
    INIT_RESOURCE_RO(well_known, get_well_known);

#if ROLE_REASONING
    INIT_RESOURCE_RW(hist_analyze_per, get_history_analyze_period_handler, set_history_analyze_period_handler);
    INIT_RESOURCE_RO(criticality_lvl, get_criticality_lvl);
    INIT_RESOURCE_RO(history_part1, get_history_part1);
    INIT_RESOURCE_RO(history_part2, get_history_part2);
    INIT_RESOURCE_RO(history_sensor_part1, get_history_sensor_part1);
    INIT_RESOURCE_RO(history_sensor_part2, get_history_sensor_part2);
    INIT_RESOURCE_RO(history_sensor_part3, get_history_sensor_part3);
    INIT_RESOURCE_WO(false_positive, report_false_positive);
    INIT_RESOURCE_WO(false_negative, report_false_negative);

    INIT_RESOURCE_RO(alert_alarm_ratio, get_alert_alarm_ratio);
#endif

    rest_activate_resource(&resource_min_intrusion_duration);
    rest_activate_resource(&resource_max_intrusion_duration);
    rest_activate_resource(&resource_latency_mode);
    rest_activate_resource(&resource_well_known);

#if ROLE_REASONING
    rest_activate_resource(&resource_hist_analyze_per);
    rest_activate_resource(&resource_criticality_lvl);
    rest_activate_resource(&resource_history_part1);
    rest_activate_resource(&resource_history_part2);
    rest_activate_resource(&resource_history_sensor_part1);
    rest_activate_resource(&resource_history_sensor_part2);
    rest_activate_resource(&resource_history_sensor_part3);
    rest_activate_resource(&resource_false_positive);
    rest_activate_resource(&resource_false_negative);
	rest_activate_resource(&resource_alert_alarm_ratio);
#endif
}

void get_well_known(REQUEST* request, RESPONSE* response) {
        u8 payload[] = { WELL_KNOWN_VERSION, ROLE_GATEWAY, ROLE_BROKER, ROLE_REASONING };

	response->ver = request->ver;
	response->option_count = 0;
	response->tid = request->tid;
	rest_set_payload(response, payload, 4);
	rest_set_response_status(response, OK_200);
}

void set_min_intrusion_duration_handler(REQUEST *request, RESPONSE *response)
{

}

void get_min_intrusion_duration_handler(REQUEST *request, RESPONSE *response)
{

}

void set_max_intrusion_duration_handler(REQUEST *request, RESPONSE *response)
{

}

void get_max_intrusion_duration_handler(REQUEST *request, RESPONSE *response)
{

}

void set_latency_mode_handler(REQUEST *request, RESPONSE *response)
{
	u8 latencies[] = { 100, 75, 50, 25 };
	u8 id;

	if (!strcmp(request->payload, "green"))
		id = 0;
	else if (!strcmp(request->payload, "yellow"))
		id = 1;
	else if (!strcmp(request->payload, "orange"))
		id = 2;
	else if (!strcmp(request->payload, "red"))
		id = 3;
	else
		return;
	set_latency_mode(latencies[id]);
}

void get_latency_mode_handler(REQUEST *request, RESPONSE *response)
{
	const char *latencies[] = { "green", "yellow", "orange", "red"};
	u8 id;

	id = (100 - (get_latency_mode() & 0xff)) / 25;
	response->ver = request->ver;
	response->option_count = 0;
	response->tid = request->tid;
	rest_set_payload(response, latencies[id], strlen(latencies[id]) + 1);
	rest_set_response_status(response, OK_200);
}

#if ROLE_REASONING

// In diaforus the maximum data len is 68 bytes
void get_history_part1(REQUEST *request, RESPONSE *response)
{
       int i;

       memset(history_buffer, 0, sizeof(history_buffer));
       reasoning_history_suspicious_events_serialize(history_buffer);
       response->ver = request->ver;
       response->option_count = 0;
       response->tid = request->tid;
       rest_set_response_status(response, OK_200);
       rest_set_payload(response, history_buffer, 68);
}

void get_history_part2(REQUEST *request, RESPONSE *response)
{
       response->ver = request->ver;
       response->option_count = 0;
       response->tid = request->tid;
       rest_set_response_status(response, OK_200);
       rest_set_payload(response, history_buffer + 68, 12);
}

void get_history_sensor_part1(REQUEST *request, RESPONSE *response)
{
       response->ver = request->ver;
       response->option_count = 0;
       response->tid = request->tid;
       rest_set_response_status(response, OK_200);
       rest_set_payload(response, history_buffer + 80, 68);
}

void get_history_sensor_part2(REQUEST *request, RESPONSE *response)
{
       response->ver = request->ver;
       response->option_count = 0;
       response->tid = request->tid;
       rest_set_response_status(response, OK_200);
       rest_set_payload(response, history_buffer + 148, 68);
}

void get_history_sensor_part3(REQUEST *request, RESPONSE *response)
{
       response->ver = request->ver;
       response->option_count = 0;
       response->tid = request->tid;
       rest_set_response_status(response, OK_200);
       rest_set_payload(response, history_buffer + 216, 8);
}

void get_criticality_lvl(REQUEST *request, RESPONSE *response)
{
        u16 payload[2];

	payload[0] = htons(get_criticality_threshold());
	payload[1] = htons(get_criticality_level());
	response->ver = request->ver;
	response->option_count = 0;
	response->tid = request->tid;
	rest_set_response_status(response, OK_200);
	rest_set_payload(response, payload, sizeof(u16) * 2);
}

void set_history_analyze_period_handler(REQUEST *request, RESPONSE *response)
{

}

void get_history_analyze_period_handler(REQUEST *request, RESPONSE *response)
{

}

void get_alert_alarm_ratio(REQUEST* request, RESPONSE* response) {
	u16 payload[] = { htons(reasoning_alarm_count), htons(reasoning_alert_count) };

	response->ver = request->ver;
	response->option_count = 0;
	response->tid = request->tid;
	rest_set_payload(response, payload, sizeof(payload));
	rest_set_response_status(response, OK_200);
}
#endif

unsigned long itoa(const char *str, const char **endstr)
{
	unsigned long n = 0;
	
	if (endstr)
			*endstr = str;

	while (*str != '\0')
	{
		if (*str >= '0' && *str <= '9')
			n = n * 10 + (*str) - '0';
		else
			return -1;

		str++;
		if (endstr)
			*endstr = str;
	}

	return n;
}

#if ROLE_REASONING
void report_false_positive(REQUEST *request, RESPONSE *response)
{
  timestamp_t timestamp;
  uint8_t critical_level;
  char *tmp;

  tmp = __builtin_strchr((char *)request->payload, ':');

  if (!tmp) {
    //DEBUG("APP", LOG_CRITICAL, "report_false_positive requires at least 2 arguments seperated by ':'\n");
    return;
  }
  
  *tmp++ = '\0';
  timestamp = itoa((char*)request->payload, NULL);
  critical_level = itoa(tmp, NULL);
  reputation_management_report_false_positive(timestamp, critical_level);
}

void report_false_negative(REQUEST *request, RESPONSE *response)
{
  timestamp_t timestamp;

  timestamp = itoa((char *)request->payload, NULL);
  reputation_management_report_false_negative(timestamp);
}
#endif
#endif
