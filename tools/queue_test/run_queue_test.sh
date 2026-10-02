#!/bin/sh
# Builds a standalone host binary from the real vendored library
# (lib/Queue/src/cppQueue.cpp - not a copy) against a minimal Arduino.h
# shim, same convention as tools/aprs_test/.
set -e
cd "$(dirname "$0")/../.."

CXX="${CXX:-clang++}"

"$CXX" -std=c++17 -O2 -Wall -Wextra \
	-I tools/queue_test/arduino_compat \
	-I lib/Queue/src \
	tools/queue_test/queue_test.cpp lib/Queue/src/cppQueue.cpp \
	-o tools/queue_test/queue_test

exec tools/queue_test/queue_test
