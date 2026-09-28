// Minimal host-side stand-in for Arduino.h, just enough to compile the real
// include/main.h (for the Configuration struct + config_fields.h's field
// table) and src/config_backup.cpp with a plain host compiler. Not a
// general Arduino shim - only implements what those files actually use;
// grep for what changed before assuming more is needed. Modeled after
// tools/aprs_test/arduino_compat/Arduino.h, expanded because main.h pulls
// in much more than parse_aprs.cpp does.
#pragma once

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <string>

typedef unsigned int uint;
typedef uint8_t byte;
typedef bool boolean;
typedef void *SemaphoreHandle_t;
#define RTC_DATA_ATTR

inline bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

class String
{
public:
	String() {}
	String(const char *s) : s_(s ? s : "") {}
	String(const std::string &s) : s_(s) {}
	String(int v) : s_(std::to_string(v)) {}
	String(unsigned int v) : s_(std::to_string(v)) {}
	String(long v) : s_(std::to_string(v)) {}
	String(unsigned long v) : s_(std::to_string(v)) {}
	String(float v, int decimals) { char buf[64]; snprintf(buf, sizeof(buf), "%.*f", decimals, (double)v); s_ = buf; }
	String(double v, int decimals) { char buf[64]; snprintf(buf, sizeof(buf), "%.*f", decimals, v); s_ = buf; }

	const char *c_str() const { return s_.c_str(); }
	size_t length() const { return s_.length(); }
	bool operator==(const char *other) const { return s_ == other; }
	bool operator==(const String &other) const { return s_ == other.s_; }
	bool operator!=(const String &other) const { return s_ != other.s_; }
	char operator[](size_t i) const { return s_[i]; }

	String &operator+=(const String &other) { s_ += other.s_; return *this; }
	String &operator+=(char c) { s_ += c; return *this; }
	String operator+(const String &other) const { return String((s_ + other.s_).c_str()); }

	int indexOf(char c) const { auto p = s_.find(c); return p == std::string::npos ? -1 : (int)p; }
	int indexOf(char c, int from) const { auto p = s_.find(c, from); return p == std::string::npos ? -1 : (int)p; }
	String substring(int from) const { return String(s_.substr(from).c_str()); }
	String substring(int from, int to) const { return String(s_.substr(from, to - from).c_str()); }
	void trim()
	{
		size_t a = s_.find_first_not_of(" \t\r\n");
		if (a == std::string::npos) { s_.clear(); return; }
		size_t b = s_.find_last_not_of(" \t\r\n");
		s_ = s_.substr(a, b - a + 1);
	}
	int toInt() const { return s_.empty() ? 0 : atoi(s_.c_str()); }
	float toFloat() const { return s_.empty() ? 0.0f : (float)atof(s_.c_str()); }
	void clear() { s_.clear(); }
	void reserve(size_t) {}

private:
	std::string s_;
};

inline String operator+(const char *a, const String &b) { return String(a) + b; }
inline bool operator==(const char *a, const String &b) { return b == a; }

#define log_d(...) do { printf(__VA_ARGS__); printf("\n"); } while (0)
