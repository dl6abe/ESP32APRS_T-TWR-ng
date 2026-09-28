#!/bin/sh
# Builds a standalone host binary from the real production file
# (src/parse_aprs.cpp - not a copy), using arduino_compat/Arduino.h as the
# only stand-in for the Arduino framework, then runs it.
set -e
cd "$(dirname "$0")/../.."

CXX="${CXX:-clang++}"

"$CXX" -std=c++17 -O2 -Wall -Wextra \
	-I tools/aprs_test/arduino_compat \
	-I include \
	tools/aprs_test/aprs_test.cpp src/parse_aprs.cpp \
	-o tools/aprs_test/aprs_test

exec tools/aprs_test/aprs_test
