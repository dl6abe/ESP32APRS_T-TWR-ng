// Definitions for the handful of real-firmware functions that
// config_backup.cpp (and, going forward, config_json.cpp) calls but that
// arduino_compat/'s header stubs only declare via main.h, not define.
// Kept separate from the Arduino.h header stubs since these are function
// *bodies*, not type stand-ins - main.h still provides the declaration.
//
// Real definitions for both live in src/utils.cpp, which also pulls in
// LibAPRSesp.h/KISS.h (radio/AX25 stack) - not host-compilable, so these
// are small standalone re-implementations of just the two functions
// actually called from the config path, not a link against the real file.
// Keep in sync with src/utils.cpp if either one's logic ever changes.
#include <Arduino.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

void projLog(uint16_t category, const char *fmt, ...)
{
	(void)category;
	// stderr, not stdout - tools/config_convert/ pipes pure data on stdout
	// (e.g. the converted JSON), and this must never land mixed into that.
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

boolean isValidNumber(String str)
{
	for (size_t i = 0; i < str.length(); i++)
	{
		if (isDigit(str[i]))
			return true;
	}
	return false;
}
