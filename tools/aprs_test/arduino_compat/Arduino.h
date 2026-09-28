// Minimal host-side stand-in for Arduino.h, just enough to compile the real
// src/parse_aprs.cpp (and include/pbuf.h, include/parse_aprs.h) with a plain
// host compiler. Not a general Arduino shim - only implements what those
// files actually use; grep for what changed before assuming more is needed.
#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>

typedef unsigned int uint;

inline bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

class String
{
public:
	String() {}
	String(const char *s) : s_(s ? s : "") {}
	const char *c_str() const { return s_.c_str(); }
	size_t length() const { return s_.length(); }
	bool operator==(const char *other) const { return s_ == other; }
	bool operator==(const String &other) const { return s_ == other.s_; }

private:
	std::string s_;
};

inline bool operator==(const char *a, const String &b) { return b == a; }
