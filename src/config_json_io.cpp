/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

// Device-only I/O wrappers around config_json.cpp's pure functions - the
// LittleFS/EEPROM calls here have no host-native stand-in, so unlike
// config_json.cpp this file is not compiled by tools/config_test/ (same
// "reviewed, not unit-tested" stance ARCHITECTURE.md already takes on
// gui_lcd.cpp). Kept in its own translation unit specifically so
// config_json.cpp's pure functions stay free of any LittleFS/Arduino-core
// include, not just unused by the test harness.

#include "config_json.h"
#include <Arduino.h>
#include <LittleFS.h>
#include "main.h"

static SemaphoreHandle_t configMutex = nullptr;

void configJsonInit()
{
	configMutex = xSemaphoreCreateMutex();
}

bool saveConfigJsonImpl()
{
	xSemaphoreTake(configMutex, portMAX_DELAY);

	JsonDocument doc;
	configToJsonDoc(doc);

	File f = LittleFS.open(CONFIG_JSON_TMP_PATH, FILE_WRITE);
	if (!f)
	{
		projLog(LOGCAT_SYSTEM, "saveConfigJsonImpl(): failed to open %s for write", CONFIG_JSON_TMP_PATH);
		xSemaphoreGive(configMutex);
		return false;
	}
	size_t written = serializeJson(doc, f);
	f.close();
	// serializeJson() returns 0 on failure (e.g. the write itself failed
	// partway through) - checked here, unlike upstream V0.5's
	// saveConfiguration() which ignores this return value entirely and
	// always reports success regardless (see FORK_NOTES.md/issue #2).
	if (written == 0)
	{
		projLog(LOGCAT_SYSTEM, "saveConfigJsonImpl(): serializeJson() wrote 0 bytes");
		LittleFS.remove(CONFIG_JSON_TMP_PATH);
		xSemaphoreGive(configMutex);
		return false;
	}

	// Atomic swap: the next boot (or a concurrent loadConfigJson()) only
	// ever sees "no file" or "complete, valid file" - never a half-written
	// one, even across a power loss exactly at this point.
	if (LittleFS.exists(CONFIG_JSON_PATH))
		LittleFS.remove(CONFIG_JSON_PATH);
	bool renamed = LittleFS.rename(CONFIG_JSON_TMP_PATH, CONFIG_JSON_PATH);
	if (!renamed)
	{
		projLog(LOGCAT_SYSTEM, "saveConfigJsonImpl(): rename to %s failed", CONFIG_JSON_PATH);
	}

	xSemaphoreGive(configMutex);
	return renamed;
}

bool loadConfigJson()
{
	xSemaphoreTake(configMutex, portMAX_DELAY);

	if (!LittleFS.exists(CONFIG_JSON_PATH))
	{
		// Expected/benign - e.g. the first boot after migrating, or a
		// brand-new device. Not logged at normal visibility; distinct from
		// the "file exists but failed to parse" branch below on purpose.
		xSemaphoreGive(configMutex);
		return false;
	}

	File f = LittleFS.open(CONFIG_JSON_PATH, FILE_READ);
	if (!f)
	{
		projLog(LOGCAT_SYSTEM, "loadConfigJson(): %s exists but could not be opened", CONFIG_JSON_PATH);
		xSemaphoreGive(configMutex);
		return false;
	}

	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, f);
	f.close();
	if (error)
	{
		// A real corruption event (e.g. a power loss mid-write on an older
		// firmware build before atomic rename existed, or flash wear) -
		// logged loudly (syslog-reachable on an unattended tower), unlike
		// "no file at all". Preserve the corrupt file for forensics before
		// anything overwrites it - upstream V0.5 has no equivalent
		// backup-before-overwrite (see FORK_NOTES.md/issue #2).
		projLog(LOGCAT_SYSTEM, "loadConfigJson(): %s failed to parse: %s", CONFIG_JSON_PATH, error.c_str());
		LittleFS.remove(CONFIG_JSON_CORRUPT_PATH); // best-effort, drop any older corrupt copy
		LittleFS.rename(CONFIG_JSON_PATH, CONFIG_JSON_CORRUPT_PATH);
		xSemaphoreGive(configMutex);
		return false;
	}

	uint16_t applied = 0, invalid = 0;
	jsonDocToConfig(doc, &applied, &invalid);
	projLog(LOGCAT_SYSTEM, "Config load: %d applied, %d invalid (kept default)", applied, invalid);

	xSemaphoreGive(configMutex);
	return true;
}

bool migrateFromEepromOrDefault()
{
	byte *ptr = (byte *)&config;
	EEPROM.readBytes(1, ptr, sizeof(Configuration));
	uint8_t chkSum = checkSum(ptr, sizeof(Configuration));
	bool checksumOk = (EEPROM.read(0) == chkSum);
	bool plausible = looksLikePlausibleOldConfig(config);

	if (checksumOk || plausible)
	{
		projLog(LOGCAT_SYSTEM, "Migrating config from EEPROM to %s (checksumOk=%d plausible=%d)",
				CONFIG_JSON_PATH, checksumOk, plausible);
		// `config` already holds the migrated values from the raw read
		// above - nothing left to convert field-by-field, just persist it.
		saveConfigJsonImpl();
		return true;
	}

	projLog(LOGCAT_SYSTEM, "EEPROM has no usable config (blank/corrupt) - using compiled-in defaults");
	defaultConfig();
	return false;
}
