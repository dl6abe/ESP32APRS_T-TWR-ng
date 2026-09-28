#!/bin/sh
# Builds a standalone host binary from the real production file
# (src/wifi_config.cpp - not a copy). No Arduino shim needed at all:
# wifi_config.cpp has zero Arduino dependencies.
set -e
cd "$(dirname "$0")/../.."

CXX="${CXX:-clang++}"

"$CXX" -std=c++17 -O2 -Wall -Wextra \
	-I include \
	tools/wifi_validate_test/wifi_validate_test.cpp src/wifi_config.cpp \
	-o tools/wifi_validate_test/wifi_validate_test

exec tools/wifi_validate_test/wifi_validate_test
