/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

// Field-descriptor table driving config_backup.cpp's export/import (key=value
// text, see FORK_NOTES.md). One entry per Configuration field (main.h) -
// generic export/import loops walk this table instead of ~90 hand-written,
// easy-to-typo pairs of "print field"/"parse field" code. Added 2026-09-27
// alongside the struct-size-changing debug-logging fields, specifically so
// config survives a firmware update that changes sizeof(Configuration) -
// see FORK_NOTES.md for why that matters (the raw EEPROM checksum doesn't).
#ifndef CONFIG_FIELDS_H
#define CONFIG_FIELDS_H

#include "main.h"

enum ConfigFieldType
{
	CFT_BOOL,
	CFT_I8,	 // signed char used as a small integer (e.g. wifi_mode), not text
	CFT_U8,	 // uint8_t / unsigned char
	CFT_U16, // uint16_t
	CFT_U32, // uint32_t
	CFT_INT, // int / unsigned int
	CFT_FLOAT,
	CFT_STR, // fixed char[] buffer - size below is the buffer size
};

struct ConfigField
{
	const char *name;
	void *ptr;
	ConfigFieldType type;
	size_t size; // only meaningful for CFT_STR
};

extern const ConfigField configFields[];
extern const size_t configFieldCount;

String buildConfigBackup();
// Applies key=value lines from text to `config` in place - does not call
// saveEEPROM() or reboot, caller's responsibility. Unknown keys are
// skipped; fields absent from `text` are left untouched (whatever `config`
// already held - typically defaultConfig()'s values on a fresh/reset
// device, or the previous live values otherwise). A present key whose
// value doesn't look like a number for a numeric field (isValidNumber())
// is *also* left untouched rather than silently becoming 0 - a bad value
// in one field shouldn't blank out a previously-good one. Always logged
// at LOGCAT_SYSTEM; the three optional out-params additionally let a
// caller (e.g. the web /configRestore handler) surface the same counts to
// whoever triggered the restore, not just to the syslog.
void applyConfigBackup(const String &text, uint16_t *appliedOut = nullptr,
						uint16_t *unknownOut = nullptr, uint16_t *invalidOut = nullptr);

#endif
