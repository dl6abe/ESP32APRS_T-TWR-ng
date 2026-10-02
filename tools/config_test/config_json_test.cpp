// Host-native test harness for config_json.cpp's pure functions
// (configToJsonDoc/jsonDocToConfig/looksLikePlausibleOldConfig) - compiles
// the real production file, not a copy, against arduino_compat/ plus
// ArduinoJson's own (host-compilable) headers. Run via run_config_test.sh.
// Same technique and intent as config_test.cpp (the text-backup-format
// sibling of this suite) - see that file's header comment for the
// generic "add a field, nothing here needs to change" rationale, which
// applies identically here since both walk the same configFields[] table.
//
// What this adds beyond config_test.cpp:
//  1. Every field round-trips through configToJsonDoc()/jsonDocToConfig().
//  2. A key missing from the JSON doc leaves `config`'s current value
//     untouched (not reset to zero/default by this function itself).
//  3. A key present with the WRONG JSON type (e.g. a JSON string for a
//     numeric field, a JSON number for a string field) is skipped and
//     counted in `invalid`, never coerced and never crashes.
//  4. An oversized string value is clamped, not overflowed.
//  5. "_version" is present after serialize, and tolerated if
//     missing/unexpected on parse.
//  6. looksLikePlausibleOldConfig() accepts realistic data, rejects
//     uniformly-blank (0xFF/0x00) flash, and - the actual bug this issue
//     exists to fix - accepts a struct that's merely a different shape
//     (only the first N fields populated) rather than truly corrupt.
#include "config_json.h"
#include <cstdio>
#include <cstring>

Configuration config;
char VERSION[9] = "20260927";
char VERSION_BUILD = 'A';
int failures = 0;

String fieldValueAsString(const ConfigField &f)
{
	switch (f.type)
	{
	case CFT_BOOL:
		return String(*(bool *)f.ptr ? 1 : 0);
	case CFT_I8:
		return String((int)*(char *)f.ptr);
	case CFT_U8:
		return String((unsigned int)*(uint8_t *)f.ptr);
	case CFT_U16:
		return String(*(uint16_t *)f.ptr);
	case CFT_U32:
		return String(*(uint32_t *)f.ptr);
	case CFT_INT:
		return String(*(int *)f.ptr);
	case CFT_FLOAT:
		return String(*(float *)f.ptr, 6);
	case CFT_STR:
		return String((char *)f.ptr);
	}
	return String("");
}

void expectEq(const char *fieldName, const String &got, const String &want)
{
	if (!(got == want))
	{
		printf("FAIL %-20s got='%s' want='%s'\n", fieldName, got.c_str(), want.c_str());
		failures++;
	}
}

int main()
{
	// --- Test 1: round-trip every field through configToJsonDoc() -> jsonDocToConfig() ---
	for (size_t i = 0; i < configFieldCount; i++)
	{
		const ConfigField &f = configFields[i];
		switch (f.type)
		{
		case CFT_BOOL:
			*(bool *)f.ptr = true;
			break;
		case CFT_I8:
			*(char *)f.ptr = (char)(7 + (i % 100));
			break;
		case CFT_U8:
			*(uint8_t *)f.ptr = (uint8_t)(11 + (i % 200));
			break;
		case CFT_U16:
			*(uint16_t *)f.ptr = (uint16_t)(1000 + i);
			break;
		case CFT_U32:
			*(uint32_t *)f.ptr = (uint32_t)(100000 + i);
			break;
		case CFT_INT:
			*(int *)f.ptr = (int)(42 + i);
			break;
		case CFT_FLOAT:
			*(float *)f.ptr = 1.5f + (float)i;
			break;
		case CFT_STR:
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "val%zu", i);
			strncpy((char *)f.ptr, buf, f.size - 1);
			((char *)f.ptr)[f.size - 1] = 0;
			break;
		}
		}
	}

	JsonDocument doc;
	configToJsonDoc(doc);

	if (!doc["_version"].is<int>() || doc["_version"].as<int>() != CONFIG_JSON_VERSION)
	{
		printf("FAIL _version missing/wrong after configToJsonDoc()\n");
		failures++;
	}

	Configuration expected = config;
	memset(&config, 0, sizeof(Configuration));

	uint16_t applied = 0, invalid = 0;
	jsonDocToConfig(doc, &applied, &invalid);

	if (invalid != 0)
	{
		printf("FAIL round-trip: %u field(s) reported invalid, want 0\n", invalid);
		failures++;
	}
	if (applied != configFieldCount)
	{
		printf("FAIL round-trip: applied=%u want=%zu\n", applied, configFieldCount);
		failures++;
	}
	for (size_t i = 0; i < configFieldCount; i++)
	{
		const ConfigField &f = configFields[i];
		ConfigField expectedField = f;
		expectedField.ptr = (char *)&expected + ((char *)f.ptr - (char *)&config);
		expectEq(f.name, fieldValueAsString(f), fieldValueAsString(expectedField));
	}

	// --- Test 2: a key missing from the doc leaves config's current value untouched ---
	memset(&config, 0, sizeof(Configuration));
	strncpy(config.aprs_mycall, "UNTOUCHD", sizeof(config.aprs_mycall) - 1);
	{
		JsonDocument emptyDoc;
		emptyDoc["_version"] = CONFIG_JSON_VERSION;
		uint16_t a = 0, inv = 0;
		jsonDocToConfig(emptyDoc, &a, &inv);
		expectEq("aprs_mycall (untouched, missing key)", String(config.aprs_mycall), String("UNTOUCHD"));
		if (a != 0 || inv != 0)
		{
			printf("FAIL empty-doc counters: applied=%u invalid=%u want=0/0\n", a, inv);
			failures++;
		}
	}

	// --- Test 3: wrong JSON type for a field is skipped, not coerced, and counted ---
	memset(&config, 0, sizeof(Configuration));
	config.aprs_port = 14580; // pre-existing value, must survive a type-mismatched key
	strncpy(config.aprs_mycall, "DL6ABE", sizeof(config.aprs_mycall) - 1);
	{
		JsonDocument badDoc;
		badDoc["aprs_port"] = "notanumber"; // string where a number (CFT_U16) is expected
		badDoc["aprs_ssid"] = "also-bad";	 // string where a number (CFT_U8) is expected
		uint16_t a = 0, inv = 0;
		jsonDocToConfig(badDoc, &a, &inv);
		if (config.aprs_port != 14580)
		{
			printf("FAIL aprs_port clobbered by type mismatch: got=%u want=14580\n", config.aprs_port);
			failures++;
		}
		expectEq("aprs_mycall (untouched, type-mismatched keys elsewhere)", String(config.aprs_mycall), String("DL6ABE"));
		if (a != 0 || inv != 2)
		{
			printf("FAIL type-mismatch counters: applied=%u invalid=%u want=0/2\n", a, inv);
			failures++;
		}
	}
	// ... and the reverse direction: a JSON number for a string field (CFT_STR).
	memset(&config, 0, sizeof(Configuration));
	strncpy(config.aprs_mycall, "DL6ABE", sizeof(config.aprs_mycall) - 1);
	{
		JsonDocument badDoc;
		badDoc["aprs_mycall"] = 12345;
		uint16_t a = 0, inv = 0;
		jsonDocToConfig(badDoc, &a, &inv);
		expectEq("aprs_mycall (untouched, number for a string field)", String(config.aprs_mycall), String("DL6ABE"));
		if (a != 0 || inv != 1)
		{
			printf("FAIL number-for-string counters: applied=%u invalid=%u want=0/1\n", a, inv);
			failures++;
		}
	}

	// --- Test 4: an oversized string value is clamped, not overflowed ---
	memset(&config, 0, sizeof(Configuration));
	{
		String longVal = "";
		for (int i = 0; i < 100; i++)
			longVal += "X";
		JsonDocument d;
		d["aprs_mycall"] = longVal.c_str();
		jsonDocToConfig(d);
		if (strlen(config.aprs_mycall) != sizeof(config.aprs_mycall) - 1)
		{
			printf("FAIL aprs_mycall clamp: length=%zu want=%zu\n", strlen(config.aprs_mycall), sizeof(config.aprs_mycall) - 1);
			failures++;
		}
	}

	// --- Test 5: looksLikePlausibleOldConfig() ---
	{
		// 5a: a realistic, fully-populated struct is plausible.
		Configuration realistic;
		memset(&realistic, 0, sizeof(Configuration));
		for (size_t i = 0; i < configFieldCount; i++)
		{
			const ConfigField &f = configFields[i];
			size_t offset = (size_t)((char *)f.ptr - (char *)&config);
			void *destPtr = (char *)&realistic + offset;
			switch (f.type)
			{
			case CFT_BOOL:
				*(bool *)destPtr = true;
				break;
			case CFT_STR:
				strncpy((char *)destPtr, "REALISTIC", f.size - 1);
				((char *)destPtr)[f.size - 1] = 0;
				break;
			default:
				break; // zero is fine for the numeric fields this check cares about
			}
		}
		realistic.wifi_mode = WIFI_AP_STA_FIX;
		realistic.aprs_ssid = 1;
		realistic.digi_ssid = 3;
		realistic.trk_ssid = 7;
		realistic.rf_type = RF_SA868_VHF;
		if (!looksLikePlausibleOldConfig(realistic))
		{
			printf("FAIL looksLikePlausibleOldConfig(): rejected a realistic struct\n");
			failures++;
		}

		// 5b: uniformly 0xFF (erased, never-written flash) is NOT plausible.
		Configuration blankFF;
		memset(&blankFF, 0xFF, sizeof(Configuration));
		if (looksLikePlausibleOldConfig(blankFF))
		{
			printf("FAIL looksLikePlausibleOldConfig(): accepted all-0xFF blank flash\n");
			failures++;
		}

		// 5c: uniformly zeroed is NOT plausible either.
		Configuration blankZero;
		memset(&blankZero, 0x00, sizeof(Configuration));
		if (looksLikePlausibleOldConfig(blankZero))
		{
			printf("FAIL looksLikePlausibleOldConfig(): accepted all-zero struct\n");
			failures++;
		}

		// 5d: the actual bug this issue exists to fix - a struct that's a
		// different SHAPE (as if saved by firmware with fewer fields, so
		// everything after some point reads back as the compiler's
		// zero-init, not as 0xFF) must still be accepted as plausible, not
		// rejected just because part of it looks "unset."
		Configuration differentShape = realistic;
		for (size_t i = configFieldCount / 2; i < configFieldCount; i++)
		{
			const ConfigField &f = configFields[i];
			if (f.type != CFT_STR)
				continue;
			size_t offset = (size_t)((char *)f.ptr - (char *)&config);
			void *destPtr = (char *)&differentShape + offset;
			memset(destPtr, 0, f.size); // zeroed tail, not 0xFF - still has SOME real data earlier
		}
		if (!looksLikePlausibleOldConfig(differentShape))
		{
			printf("FAIL looksLikePlausibleOldConfig(): rejected a merely differently-shaped struct\n");
			failures++;
		}
	}

	if (failures == 0)
	{
		printf("OK: all config_json tests passed (%zu fields)\n", configFieldCount);
		return 0;
	}
	printf("FAILED: %d check(s) failed\n", failures);
	return 1;
}
