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
// Guards Serial.printf() in projLog() - found 2026-09-29 live on hardware:
// taskAPRS/taskNetwork/etc. all call projLog() with no synchronization on
// the shared Serial (HWCDC) output, so concurrent calls interleave
// mid-line (one line's text gets overwritten/merged with another's before
// its own trailing '\n'). Same class of bug as the aprsClient data race
// (commit 0dd1336) and the display I2C race (commit 6fc993c) - this is
// the Serial-output instance of the same "no lock around a shared
// peripheral accessed from multiple tasks" pattern.
static SemaphoreHandle_t serialMutex = nullptr;

// Must be called once from setup(), before any task that logs is created -
// see aprsClientMutex's init in main.cpp for the same pattern/reasoning.
void logInit()
{
	serialMutex = xSemaphoreCreateMutex();
}

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
	case LOGCAT_BLUETOOTH:
		return "BLUETOOTH";
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

	xSemaphoreTake(serialMutex, portMAX_DELAY);
	// \r\n, not just \n, matching consolePoll()'s convention - without the
	// \r a terminal that doesn't auto-translate LF->CRLF moves down a line
	// but stays at the same column, producing a "staircase" of merged
	// lines. Found live on hardware 2026-09-29 (user report).
	//
	// Trailing \r\n only, no leading one: projLog() fires far more often
	// than consolePoll()'s one-off command responses (which do lead with
	// \r\n as a deliberate visual separator), so a leading \r\n here
	// produced a blank line before every single log entry - same class of
	// double-newline bug as commit 6448d4c, found live 2026-09-29 when
	// this exact leading \r\n was tried and reverted.
	Serial.printf("[%s] %s\r\n", categoryName(category), msg);
	xSemaphoreGive(serialMutex);

	// Third sink, same category-filtered msg as Serial/syslog above - see
	// web_console.cpp. No-op (single bool check) unless a user has started
	// recording from the web UI's Console tab.
	consoleLogAppend(categoryName(category), msg);

	if (syslogClient != nullptr && syslogMutex != nullptr)
	{
		xSemaphoreTake(syslogMutex, portMAX_DELAY);
		if (syslogClient != nullptr) // re-check: syslogReconnect() may have run while waiting for the lock
			syslogClient->log(LOG_INFO, msg);
		xSemaphoreGive(syslogMutex);
	}
}
