/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

// Primary config storage: a name-keyed JSON file on LittleFS, replacing the
// raw whole-struct EEPROM blob + single checksum (see FORK_NOTES.md /
// Gitea issue #2 for why that scheme silently wipes WiFi/APRS/IGATE
// settings on any Configuration size change, not just real corruption).
// Reuses configFields[] (config_fields.h/config_backup.cpp) - the same
// table already driving the text /configBackup /configRestore format -
// so a field add/remove isn't a breaking change for primary storage
// either, exactly as it already isn't for text backups today.
#ifndef CONFIG_JSON_H
#define CONFIG_JSON_H

#include "config_fields.h"
#include <ArduinoJson.h>

#define CONFIG_JSON_VERSION 1
#define CONFIG_JSON_PATH "/config.json"
#define CONFIG_JSON_TMP_PATH "/config.json.tmp"
#define CONFIG_JSON_CORRUPT_PATH "/config.json.corrupt"

// --- Pure, no-filesystem functions - host-testable (tools/config_test/) ---

// Writes every configFields[] entry (from the live `config`) into `doc`,
// plus a "_version" key for future per-version migration logic.
void configToJsonDoc(JsonDocument &doc);

// Applies `doc`'s keys to `config` in place. A key absent from `doc` is
// left untouched (same "field keeps whatever `config` already held"
// contract applyConfigBackup() already has). A key present whose JSON
// value's actual type doesn't match the field's expected type (e.g. a
// string where a number is expected) is *also* left untouched rather than
// coerced - counted in *invalidOut, never crashes, never guesses. "Leave
// untouched" only reads as "falls back to default" when the caller
// already ran setConfigDefaults() first (see the boot sequence in
// main.cpp) - this function itself has no opinion on defaults.
void jsonDocToConfig(const JsonDocument &doc, uint16_t *appliedOut = nullptr,
					  uint16_t *invalidOut = nullptr);

// Migration-only sanity check: does `c` look like real, previously-saved
// config data, as opposed to blank/erased flash? Used by
// migrateFromEepromOrDefault() to decide whether an EEPROM image that
// failed its whole-struct checksum (e.g. purely because sizeof(Configuration)
// grew) is still worth keeping rather than discarding to defaultConfig().
bool looksLikePlausibleOldConfig(const Configuration &c);

// --- Device-only I/O wrappers (src/config_json_io.cpp) - thin, reviewed
// not unit-tested, same stance ARCHITECTURE.md already takes on gui_lcd.cpp ---

// Creates the mutex guarding loadConfigJson()/saveConfigJsonImpl() - call
// once from setup(), before any task that could call either is started,
// same convention as logInit()/consoleLogInit()/aprsClientMutex's
// creation (src/main.cpp).
void configJsonInit();

// Reads CONFIG_JSON_PATH and applies it to `config` via jsonDocToConfig().
// Returns false if the file is missing (expected/benign - e.g. the first
// boot after migrating) or failed to parse (a real corruption event -
// logged distinctly from "missing", and the corrupt file is preserved as
// CONFIG_JSON_CORRUPT_PATH before anything overwrites it).
bool loadConfigJson();

// Serializes `config` via configToJsonDoc() and writes it atomically
// (temp file + rename - LittleFS supports POSIX-style rename, so a power
// loss mid-write never leaves a half-written file visible to the next
// boot). Returns false if any step (open/write/rename) failed - the
// caller must not assume success the way upstream's V0.5
// saveConfiguration() incorrectly does (see FORK_NOTES.md/issue #2).
bool saveConfigJsonImpl();

// Called once, only when loadConfigJson() has already returned false (no
// CONFIG_JSON_PATH yet). Raw-reads the old EEPROM struct into `config`
// (same read main.cpp's boot sequence already did pre-migration);
// if the EEPROM checksum still matches OR looksLikePlausibleOldConfig()
// says the data is plausible despite a checksum mismatch, persists
// `config` as-is via saveConfigJsonImpl() (true = recovered real data);
// otherwise calls the existing defaultConfig() (false = defaulted).
bool migrateFromEepromOrDefault();

#endif
