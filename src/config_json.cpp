/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

#include "config_json.h"
#include "main.h"

// Pure functions only - also compiled host-native by tools/config_test/
// (see config_json_test.cpp), so this file must stay free of any
// LittleFS/Arduino-core include. The device-only I/O wrappers
// (loadConfigJson/saveConfigJsonImpl/migrateFromEepromOrDefault) that
// call these live in config_json_io.cpp instead, same split
// ARCHITECTURE.md already documents for e.g. wifi_config.cpp/parse_aprs.cpp.

void configToJsonDoc(JsonDocument &doc)
{
	doc["_version"] = CONFIG_JSON_VERSION;
	for (size_t i = 0; i < configFieldCount; i++)
	{
		const ConfigField &f = configFields[i];
		switch (f.type)
		{
		case CFT_BOOL:
			doc[f.name] = *(bool *)f.ptr;
			break;
		case CFT_I8:
			doc[f.name] = (int)*(char *)f.ptr;
			break;
		case CFT_U8:
			doc[f.name] = (unsigned int)*(uint8_t *)f.ptr;
			break;
		case CFT_U16:
			doc[f.name] = *(uint16_t *)f.ptr;
			break;
		case CFT_U32:
			doc[f.name] = *(uint32_t *)f.ptr;
			break;
		case CFT_INT:
			doc[f.name] = *(int *)f.ptr;
			break;
		case CFT_FLOAT:
			doc[f.name] = *(float *)f.ptr;
			break;
		case CFT_STR:
			doc[f.name] = (const char *)f.ptr;
			break;
		}
	}
}

void jsonDocToConfig(const JsonDocument &doc, uint16_t *appliedOut, uint16_t *invalidOut)
{
	uint16_t applied = 0, invalid = 0;
	for (size_t i = 0; i < configFieldCount; i++)
	{
		const ConfigField &f = configFields[i];
		JsonVariantConst v = doc[f.name];
		if (v.isNull())
			continue; // missing key - leave `config` untouched, no counter change

		switch (f.type)
		{
		case CFT_BOOL:
			if (!v.is<bool>())
			{
				invalid++;
				break;
			}
			*(bool *)f.ptr = v.as<bool>();
			applied++;
			break;
		case CFT_I8:
			if (!v.is<int>())
			{
				invalid++;
				break;
			}
			*(char *)f.ptr = (char)v.as<int>();
			applied++;
			break;
		case CFT_U8:
			if (!v.is<int>())
			{
				invalid++;
				break;
			}
			*(uint8_t *)f.ptr = (uint8_t)v.as<int>();
			applied++;
			break;
		case CFT_U16:
			if (!v.is<int>())
			{
				invalid++;
				break;
			}
			*(uint16_t *)f.ptr = (uint16_t)v.as<int>();
			applied++;
			break;
		case CFT_U32:
			if (!v.is<uint32_t>())
			{
				invalid++;
				break;
			}
			*(uint32_t *)f.ptr = v.as<uint32_t>();
			applied++;
			break;
		case CFT_INT:
			if (!v.is<int>())
			{
				invalid++;
				break;
			}
			*(int *)f.ptr = v.as<int>();
			applied++;
			break;
		case CFT_FLOAT:
			if (!v.is<float>())
			{
				invalid++;
				break;
			}
			*(float *)f.ptr = v.as<float>();
			applied++;
			break;
		case CFT_STR:
			if (!v.is<const char *>())
			{
				invalid++;
				break;
			}
			strncpy((char *)f.ptr, v.as<const char *>(), f.size - 1);
			((char *)f.ptr)[f.size - 1] = 0;
			applied++;
			break;
		}
	}
	if (appliedOut)
		*appliedOut = applied;
	if (invalidOut)
		*invalidOut = invalid;
}

bool looksLikePlausibleOldConfig(const Configuration &c)
{
	bool anyStringNotBlank = false;

	for (size_t i = 0; i < configFieldCount; i++)
	{
		const ConfigField &f = configFields[i];
		if (f.type != CFT_STR)
			continue;

		// configFields[]'s pointers are baked in against the live global
		// `config` - rebase onto `c` by the same offset, so this function
		// works on any Configuration value, not just the live global
		// (needed both for host-native unit tests and for inspecting a
		// just-read-from-EEPROM struct before deciding to keep it).
		size_t offset = (size_t)((const char *)f.ptr - (const char *)&config);
		const char *s = (const char *)&c + offset;

		bool allFF = true, allZero = true;
		for (size_t j = 0; j < f.size; j++)
		{
			uint8_t b = (uint8_t)s[j];
			if (b != 0xFF)
				allFF = false;
			if (b != 0x00)
				allZero = false;
		}
		if (!allFF && !allZero)
			anyStringNotBlank = true;

		bool terminated = false;
		for (size_t j = 0; j < f.size; j++)
		{
			uint8_t b = (uint8_t)s[j];
			if (b == 0)
			{
				terminated = true;
				break;
			}
			if (b < 0x20 || b > 0x7E)
				return false; // garbage byte before any terminator - not plausible
		}
		if (!terminated)
			return false; // ran off the end of the buffer without a NUL - not plausible
	}

	// Every single string field simultaneously erased (0xFF, never written)
	// or zeroed - the signature of genuinely blank/factory-erased flash,
	// not a struct-size-shifted-but-otherwise-real config.
	if (!anyStringNotBlank)
		return false;

	if (c.wifi_mode < WIFI_OFF_FIX || c.wifi_mode > WIFI_AP_STA_FIX)
		return false;
	if (c.aprs_ssid > 15)
		return false;
	if (c.digi_ssid > 15)
		return false;
	if (c.trk_ssid > 15)
		return false;
	if (c.rf_type > RF_SA8x8_OpenEdit)
		return false;

	return true;
}
