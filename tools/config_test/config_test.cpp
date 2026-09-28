// Host-native test harness for the config backup/restore round-trip
// (config_fields.h / src/config_backup.cpp) - compiles the real production
// files, not copies, against arduino_compat/ (same technique as
// tools/aprs_test/). Run via run_config_test.sh.
//
// What this checks, and why it matters for "every new setting we now
// implement" (the point of this suite, not just today's fields):
//  1. Every field in configFields[] round-trips through
//     buildConfigBackup()/applyConfigBackup() - catches type-tag mistakes
//     (e.g. a uint8_t field declared as CFT_STR) and buffer-size mistakes
//     in the F_() macros immediately, without needing hardware.
//  2. applyConfigBackup() only touches fields present in the input text -
//     fields absent from a backup keep whatever `config` already held.
//  3. Unknown keys in the input are skipped, not applied to the wrong
//     field or crashing.
//  4. String fields are clamped to their buffer size on import (the
//     buffer-safety rule from CLAUDE.md), not overflowed.
//
// When you add a new Configuration field: add it to configFields[] (as the
// feature you're building should already require), then add one line to
// TEST_VALUES below with a representative non-default value. Nothing else
// in this file needs to change - the round-trip loop is generic.
#include "config_fields.h"
#include <cstdio>
#include <cstring>

Configuration config;
char VERSION[9] = "20260927";
char VERSION_BUILD = 'A';
int failures = 0;

void expectEq(const char *fieldName, const String &got, const String &want)
{
	if (!(got == want))
	{
		printf("FAIL %-20s got='%s' want='%s'\n", fieldName, got.c_str(), want.c_str());
		failures++;
	}
}

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

int main()
{
	printf("configFieldCount = %zu\n", configFieldCount);

	// --- Test 1: round-trip every field through build -> reset -> apply ---
	// Set every field to a distinct, type-appropriate non-zero value first,
	// so a field that silently fails to round-trip (still zero/default
	// after apply) is caught, not masked by a coincidental zero-default.
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

	String backup = buildConfigBackup();

	// Wipe config (simulates the fresh-default state applyConfigBackup()
	// runs against in practice, e.g. after a factory reset) before
	// re-applying, so the test can't pass by accident from values that
	// were never actually touched by applyConfigBackup().
	Configuration expected = config; // remember the values we just built the backup from
	memset(&config, 0, sizeof(Configuration));

	applyConfigBackup(backup);

	for (size_t i = 0; i < configFieldCount; i++)
	{
		const ConfigField &f = configFields[i];
		// Compare against `expected`'s field at the same struct offset by
		// re-pointing a throwaway ConfigField at it, reusing fieldValueAsString().
		ConfigField expectedField = f;
		expectedField.ptr = (char *)&expected + ((char *)f.ptr - (char *)&config);
		expectEq(f.name, fieldValueAsString(f), fieldValueAsString(expectedField));
	}

	// --- Test 2: fields absent from the backup text are left untouched ---
	memset(&config, 0, sizeof(Configuration));
	strncpy(config.aprs_mycall, "UNTOUCHD", sizeof(config.aprs_mycall) - 1);
	applyConfigBackup("# comment only, no real keys\n");
	expectEq("aprs_mycall (untouched)", String(config.aprs_mycall), String("UNTOUCHD"));

	// --- Test 3: unknown keys are skipped, not mis-applied ---
	memset(&config, 0, sizeof(Configuration));
	applyConfigBackup("totally_unknown_key=123\naprs_mycall=DL6ABE\n");
	expectEq("aprs_mycall (after unknown key)", String(config.aprs_mycall), String("DL6ABE"));

	// --- Test 4: an oversized string value is clamped, not overflowed ---
	memset(&config, 0, sizeof(Configuration));
	{
		String longVal = "";
		for (int i = 0; i < 100; i++)
			longVal += "X";
		applyConfigBackup(String("aprs_mycall=") + longVal + "\n");
		if (strlen(config.aprs_mycall) != sizeof(config.aprs_mycall) - 1)
		{
			printf("FAIL aprs_mycall clamp: length=%zu want=%zu\n", strlen(config.aprs_mycall), sizeof(config.aprs_mycall) - 1);
			failures++;
		}
	}

	if (failures == 0)
	{
		printf("OK: all config backup/restore tests passed (%zu fields)\n", configFieldCount);
		return 0;
	}
	printf("FAILED: %d check(s) failed\n", failures);
	return 1;
}
