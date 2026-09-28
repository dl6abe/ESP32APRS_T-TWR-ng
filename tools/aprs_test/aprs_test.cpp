// Host-native logic tests for ParseAPRS (src/parse_aprs.cpp), built directly
// against the real production file - no copy, no mocks beyond the Arduino.h
// shim in arduino_compat/. Run: tools/aprs_test/run_aprs_test.sh
//
// No test framework: each case calls check()/checkStr() independently and
// keeps going on failure, so one bad case doesn't hide the next fifty. This
// mirrors a small host-native test tool from another APRS tracker project
// (its own tools/aprs_test), adapted to this project's parser.
//
// These are interop tests, not pure unit tests: a single wrong base91 digit
// won't show up on the bench (there's no second radio sitting next to it) -
// it shows up as an unreadable packet on aprs.fi. So the compressed-position
// vector here is cross-checked against an independently-written APRS
// encoder/decoder (github.com/richonguzman/APRSPacketLib's
// tools/aprs_vectors.json), not just re-derived from this project's own math.

#include <cstdio>
#include <cstring>
#include <parse_aprs.h>
#include <pbuf.h>

static int failures = 0;

static void check(bool condition, const char *description)
{
	if (!condition)
	{
		failures++;
		std::printf("FAIL: %s\n", description);
	}
}

static void checkStr(const char *actual, const char *expected, const char *description)
{
	if (std::strcmp(actual, expected) != 0)
	{
		failures++;
		std::printf("FAIL: %s (got \"%s\", want \"%s\")\n", description, actual, expected);
	}
}

static bool within(double actual, double expected, double tolerance)
{
	double d = actual - expected;
	if (d < 0)
		d = -d;
	return d <= tolerance;
}

// Mirrors the pbuf_t setup done in gui_lcd.cpp / webservice.cpp before
// calling parse_aprs() - see test/test_parse_aprs/test_main.cpp (the
// on-device counterpart of this file) and FORK_NOTES.md for the
// toCharArray() off-by-one this deliberately does NOT reproduce.
static void fill_pbuf_from_tnc2(const char *raw, struct pbuf_t &pb)
{
	std::string line(raw);
	memset(&pb, 0, sizeof(pbuf_t));
	pb.buf_len = sizeof(pb.data);

	int len = (int)line.length();
	if (len > (int)sizeof(pb.data) - 1)
		len = (int)sizeof(pb.data) - 1;
	pb.packet_len = len;
	memcpy(pb.data, line.c_str(), len);
	pb.data[len] = 0;

	std::string::size_type start_info = line.find(':');
	std::string::size_type end_ssid = line.find(',');
	std::string::size_type start_dst = line.find('>', 2);
	std::string::size_type start_dstssid = line.find('-', start_dst);

	if (end_ssid == std::string::npos || end_ssid > start_info)
		end_ssid = start_info;

	if (start_dstssid != std::string::npos && start_dstssid > start_dst && start_dstssid < start_dst + 10)
		pb.dstcall_end_or_ssid = &pb.data[start_dstssid];
	else
		pb.dstcall_end_or_ssid = &pb.data[end_ssid];

	pb.info_start = &pb.data[start_info + 1];
	pb.dstname = &pb.data[start_dst + 1];
	pb.dstname_len = (uint8_t)(end_ssid - start_dst);
	pb.dstcall_end = &pb.data[end_ssid];
	pb.srccall_end = &pb.data[start_dst];
}

static void test_passcode()
{
	ParseAPRS aprs;

	char n0call[] = "N0CALL";
	check(aprs.passCode(n0call) == 13023, "passCode(N0CALL) == 13023 (the well-known APRS-IS reference value)");

	char withSsid[] = "N0CALL-9";
	check(aprs.passCode(withSsid) == 13023, "passCode strips the SSID before hashing");

	char lower[] = "n0call";
	check(aprs.passCode(lower) == 13023, "passCode is case-insensitive");

	char other[] = "OH2XYZ-15";
	check(aprs.passCode(other) <= 0x7fff, "passCode always fits in 15 bits");
}

static void test_distance_direction()
{
	ParseAPRS aprs;

	check(within(aprs.distance(24.9, 60.2, 24.9, 60.2), 0.0, 0.001),
		  "distance() between identical points is 0");

	// ~111.1 km per degree of latitude near the equator.
	check(within(aprs.distance(0.0, 0.0, 0.0, 1.0), 111.13, 1.0),
		  "distance() for 1 degree of latitude is ~111 km");

	check(within(aprs.direction(0.0, 0.0, 0.0, 1.0), 0.0, 0.5),
		  "direction() due north is 0 degrees");

	check(within(aprs.direction(0.0, 0.0, 1.0, 0.0), 90.0, 0.5),
		  "direction() due east is 90 degrees");
}

static void test_parse_uncompressed_position()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75W-Test comment", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "uncompressed position: parse_aprs() returns 1");
	check(pb.packettype & T_POSITION, "uncompressed position: T_POSITION flag set");
	check(pb.flags & F_HASPOS, "uncompressed position: F_HASPOS flag set");
	check(within(pb.lat, 49.058333, 0.001), "uncompressed position: latitude decodes correctly");
	check(within(pb.lng, -72.029167, 0.001), "uncompressed position: longitude decodes correctly");
	check(pb.symbol[0] == '/' && pb.symbol[1] == '-', "uncompressed position: symbol table/code decode correctly");
	checkStr(pb.comment, "Test comment", "uncompressed position: comment text preserved");
}

static void test_parse_compressed_position()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	// Body "/6/Y`QG2.>8NG" is lat=48.13863 lon=11.57341 (Munich), course=90deg,
	// speed=30kn, base91-encoded per the APRS Protocol Reference v1.2 spec.
	// Vector cross-checked against github.com/richonguzman/APRSPacketLib
	// (tools/aprs_vectors.json, "Munich" fixed_point + course/speed vectors) -
	// independent of this project's own encoder, per the file header above.
	fill_pbuf_from_tnc2("OH2XYZ>APRS:!/6/Y`QG2.>8NG Munich Beacon", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "compressed position: parse_aprs() returns 1");
	check(pb.packettype & T_POSITION, "compressed position: T_POSITION flag set");
	check(pb.flags & F_HASPOS, "compressed position: F_HASPOS flag set");
	check(within(pb.lat, 48.13863, 0.001), "compressed position: latitude decodes correctly");
	check(within(pb.lng, 11.57341, 0.001), "compressed position: longitude decodes correctly");
	check(pb.symbol[0] == '/' && pb.symbol[1] == '>', "compressed position: symbol table/code decode correctly");
	check(pb.course == 92, "compressed position: course decodes (base91-quantized: 91 -> '8' -> 92)");
	check(within(pb.speed, 57.26, 0.5), "compressed position: speed decodes to ~57 km/h");
	checkStr(pb.comment, " Munich Beacon", "compressed position: comment text preserved");
}

static void test_parse_message()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS::N0CALL2 :Hello World{001", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "message: parse_aprs() returns 1");
	check(pb.packettype & T_MESSAGE, "message: T_MESSAGE flag set");
}

// Regression guard: src/gui_lcd.cpp and src/webservice.cpp used to build this
// pbuf_t with packet_len used as toCharArray()'s *buffer size* (its 2nd
// arg is a size, not a char count), silently dropping the packet's last
// byte - see FORK_NOTES.md. This harness's fill_pbuf_from_tnc2() copies the
// full string correctly, so this checks parse_aprs() itself isn't the one
// truncating.
static void test_full_comment_not_truncated()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75W-END", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "truncation guard: parse_aprs() returns 1");
	checkStr(pb.comment, "END", "truncation guard: last byte of the packet is not dropped");
}

// A hand-built packet, not a real capture (this project has no live-traffic
// recording set up), covering something the tests above don't: a real
// digipeat path (WIDE1-1,WIDE2-1) ahead of the ':' and an uppercase
// symbol-table overlay character ('I'), both valid per
// valid_sym_table_uncompressed() but untested by the simpler fixtures above.
static void test_parse_with_digipeat_path()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL-9>APDR16,WIDE1-1,WIDE2-1:!5006.20NI00958.90E&Test comment", pb);
	int rc = aprs.parse_aprs(&pb);
	check(rc == 1, "packet with digipeat path: parse_aprs() returns 1");
	check(pb.flags & F_HASPOS, "packet with digipeat path: F_HASPOS flag set");
	check(within(pb.lat, 50.103333, 0.001), "packet with digipeat path: latitude decodes correctly");
	check(within(pb.lng, 9.981667, 0.001), "packet with digipeat path: longitude decodes correctly");
}

int main()
{
	test_passcode();
	test_distance_direction();
	test_parse_uncompressed_position();
	test_parse_compressed_position();
	test_parse_message();
	test_full_comment_not_truncated();
	test_parse_with_digipeat_path();

	if (failures == 0)
		std::printf("All tests passed.\n");
	else
		std::printf("%d failure(s)\n", failures);

	return failures == 0 ? 0 : 1;
}
