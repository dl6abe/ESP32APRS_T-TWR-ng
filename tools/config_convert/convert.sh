#!/bin/sh
# Converts an old key=value config backup (downloaded from /configBackup)
# to the device's JSON config format, entirely offline - no device
# needed. Compiles the real src/config_backup.cpp + src/config_json.cpp
# (not copies), same technique as tools/config_test/.
#
# Usage:
#   tools/config_convert/convert.sh input.cfg output.json
#   tools/config_convert/convert.sh input.cfg            # writes JSON to stdout
#   tools/config_convert/convert.sh < input.cfg          # reads stdin too
set -e
cd "$(dirname "$0")/../.."

CXX="${CXX:-clang++}"
ARDUINOJSON_INC=".pio/libdeps/esp32s3box/ArduinoJson/src"
if [ ! -d "$ARDUINOJSON_INC" ]; then
	echo "error: $ARDUINOJSON_INC not found - run 'pio run -e esp32s3box' once first" >&2
	exit 1
fi

BIN="$(mktemp -t config_convert)"
trap 'rm -f "$BIN"' EXIT

"$CXX" -std=c++17 -O0 -Wall -Wextra \
	-I tools/config_test/arduino_compat \
	-I include \
	-I "$ARDUINOJSON_INC" \
	tools/config_convert/convert_backup_to_json.cpp src/config_backup.cpp src/config_json.cpp \
	tools/config_test/arduino_compat/host_shims.cpp \
	-o "$BIN"

if [ $# -ge 1 ]; then
	IN="$1"
else
	IN=/dev/stdin
fi

if [ $# -ge 2 ]; then
	"$BIN" < "$IN" > "$2"
	echo "Wrote $2" >&2
else
	"$BIN" < "$IN"
fi
