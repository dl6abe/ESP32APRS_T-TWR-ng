// Offline, host-native converter: takes a plain-text key=value config
// backup (as downloaded from /configBackup) on stdin and writes the
// equivalent JSON config (same shape /config.json or a JSON
// /configRestore upload uses) to stdout. No device needed - compiles the
// real src/config_backup.cpp + src/config_json.cpp (not copies), so it
// can never drift from what the firmware itself actually does. Run via
// convert.sh.
//
// Why this exists: lets an old .cfg backup be converted and inspected/
// archived as JSON, or re-uploaded via /configRestore's JSON path,
// without needing to go through a real device at all.
#include "config_fields.h"
#include "config_json.h"
#include <cstdio>
#include <iostream>
#include <sstream>

Configuration config;
char VERSION[9] = "unknown0";
char VERSION_BUILD = '-';

int main()
{
	std::ostringstream ss;
	ss << std::cin.rdbuf();
	String text(ss.str().c_str());

	// Starts from a zero-initialized `config` (this is a one-shot batch
	// conversion, not a device boot, so there's no setConfigDefaults() to
	// run first - see include/main.h for why that call matters on a real
	// boot instead). Not a problem for a genuine full backup from
	// buildConfigBackup(), which always writes every one of the 160
	// fields; a hand-trimmed/partial input would just leave the fields it
	// doesn't mention at 0/empty in the output, the same "missing key"
	// behavior documented everywhere else in this codebase.
	uint16_t applied = 0, unknown = 0, invalid = 0;
	applyConfigBackup(text, &applied, &unknown, &invalid);
	fprintf(stderr, "Parsed input: %u applied, %u unknown, %u invalid (left at 0/empty)\n",
			applied, unknown, invalid);

	JsonDocument doc;
	configToJsonDoc(doc);

	static char buf[32768];
	size_t n = serializeJsonPretty(doc, buf, sizeof(buf));
	if (n == 0 || n >= sizeof(buf))
	{
		fprintf(stderr, "error: serialized JSON didn't fit in %zu bytes\n", sizeof(buf));
		return 1;
	}
	fwrite(buf, 1, n, stdout);
	printf("\n");
	return 0;
}
