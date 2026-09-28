#!/bin/sh
# Builds a standalone host binary from the real production files
# (src/config_backup.cpp, include/config_fields.h, include/main.h - not
# copies), using arduino_compat/ as the only stand-in for the Arduino
# framework, then runs it. Same technique as tools/aprs_test/.
set -e
cd "$(dirname "$0")/../.."

CXX="${CXX:-clang++}"

"$CXX" -std=c++17 -O0 -g -Wall -Wextra \
	-I tools/config_test/arduino_compat \
	-I include \
	tools/config_test/config_test.cpp src/config_backup.cpp \
	-o tools/config_test/config_test

exec tools/config_test/config_test
