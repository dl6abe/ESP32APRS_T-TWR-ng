// Minimal host-side stand-in for Arduino.h, just enough to compile the real
// lib/Queue/src/cppQueue.cpp with a plain host compiler. cppQueue itself
// doesn't use any Arduino-specific symbol (no Serial/F()/etc.) - this only
// needs to exist so the #include succeeds. Not a general Arduino shim; grep
// for what changed before assuming more is needed.
#pragma once

#include <cstdint>
#include <cstddef>
