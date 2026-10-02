#!/bin/sh
# Builds two standalone host binaries from the real production files (not
# copies), using arduino_compat/ as the only stand-in for the Arduino
# framework, then runs both. Same technique as tools/aprs_test/.
set -e
cd "$(dirname "$0")/../.."

CXX="${CXX:-clang++}"

# ArduinoJson is host-compilable by design (no Arduino-framework
# dependency), so config_json_test.cpp just needs its headers on the
# include path - populated under .pio/libdeps once `pio run -e
# esp32s3box` has been run after platformio.ini's lib_deps gained it.
ARDUINOJSON_INC=".pio/libdeps/esp32s3box/ArduinoJson/src"
if [ ! -d "$ARDUINOJSON_INC" ]; then
	echo "error: $ARDUINOJSON_INC not found - run 'pio run -e esp32s3box' once first" >&2
	exit 1
fi

"$CXX" -std=c++17 -O0 -g -Wall -Wextra \
	-I tools/config_test/arduino_compat \
	-I include \
	tools/config_test/config_test.cpp src/config_backup.cpp \
	tools/config_test/arduino_compat/host_shims.cpp \
	-o tools/config_test/config_test

"$CXX" -std=c++17 -O0 -g -Wall -Wextra \
	-I tools/config_test/arduino_compat \
	-I include \
	-I "$ARDUINOJSON_INC" \
	tools/config_test/config_json_test.cpp src/config_json.cpp src/config_backup.cpp \
	tools/config_test/arduino_compat/host_shims.cpp \
	-o tools/config_test/config_json_test

tools/config_test/config_test
tools/config_test/config_json_test
