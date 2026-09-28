/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

// Runtime-toggleable, syslog-forwarding replacement for scattered log_d()
// calls in this project's own code - see the LOGCAT_* comment in main.h for
// why this exists instead of using CORE_DEBUG_LEVEL/esp_log_level_set() for
// this. Not a replacement for log_d() everywhere: only call sites migrated
// to projLog() get runtime category toggling + syslog forwarding; the rest
// still follow the plain compile-time CORE_DEBUG_LEVEL cap.
#include "main.h"
#include <WiFiUdp.h>
#include <Syslog.h>

static WiFiUDP syslogUdp;
static Syslog *syslogClient = nullptr;
static SemaphoreHandle_t syslogMutex = nullptr;

// (Re)creates the Syslog client from current config - call once at boot
// (after loading config) and again any time config.syslog_* is saved from
// the web UI (see handle_system()'s commitDebugLog branch).
void syslogReconnect()
{
	if (syslogMutex == nullptr)
		syslogMutex = xSemaphoreCreateMutex();

	xSemaphoreTake(syslogMutex, portMAX_DELAY);
	if (syslogClient != nullptr)
	{
		delete syslogClient;
		syslogClient = nullptr;
	}
	if (config.syslog_en && strlen(config.syslog_host) > 0)
	{
		const char *mycall = (strcmp("NOCALL", config.aprs_mycall) == 0) ? "ESP32APRS" : config.aprs_mycall;
		syslogClient = new Syslog(syslogUdp, config.syslog_host, config.syslog_port, mycall, "ESP32APRS-TWR", LOG_USER, SYSLOG_PROTO_IETF);
	}
	xSemaphoreGive(syslogMutex);
}

static const char *categoryName(uint16_t category)
{
	switch (category)
	{
	case LOGCAT_SYSTEM:
		return "SYSTEM";
	case LOGCAT_WEB:
		return "WEB";
	case LOGCAT_GPS:
		return "GPS";
	case LOGCAT_APRS_RF:
		return "APRS_RF";
	case LOGCAT_APRS_INET:
		return "APRS_INET";
	case LOGCAT_RF_MODULE:
		return "RF_MODULE";
	default:
		return "LOG";
	}
}

// category is one of the LOGCAT_* bits (main.h) - checked against
// config.logCategoryMask before doing any formatting/output work.
void projLog(uint16_t category, const char *fmt, ...)
{
	if (!(config.logCategoryMask & category))
		return;

	char msg[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	Serial.printf("[%s] %s\n", categoryName(category), msg);

	if (syslogClient != nullptr && syslogMutex != nullptr)
	{
		xSemaphoreTake(syslogMutex, portMAX_DELAY);
		if (syslogClient != nullptr) // re-check: syslogReconnect() may have run while waiting for the lock
			syslogClient->log(LOG_INFO, msg);
		xSemaphoreGive(syslogMutex);
	}
}
