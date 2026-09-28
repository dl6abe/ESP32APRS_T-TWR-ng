// Unity tests for ParseAPRS (src/parse_aprs.cpp).
//
// Runs on real T-TWR hardware (Unity over serial), since parse_aprs.cpp
// depends on Arduino.h/String and this project has no host-native build:
//   pio test -e test_parse_aprs --upload-port <port>
//
// Only exercises the public API of ParseAPRS (parse_aprs, distance, direction,
// passCode, deg2lat, deg2lon) - everything else in the class is private.

#include <Arduino.h>
#include <unity.h>
#include <parse_aprs.h>
#include <pbuf.h>

static ParseAPRS aprs;

// Mirrors the pbuf_t setup done in gui_lcd.cpp / webservice.cpp before calling
// parse_aprs(), but without the off-by-one truncation bug those call sites had
// (see FORK_NOTES.md) - this is a from-scratch harness for the parser itself.
static void fill_pbuf_from_tnc2(const char *raw, struct pbuf_t &pb)
{
	String line(raw);
	memset(&pb, 0, sizeof(pbuf_t));
	pb.buf_len = sizeof(pb.data);

	int len = line.length();
	if (len > (int)sizeof(pb.data) - 1)
		len = (int)sizeof(pb.data) - 1;
	pb.packet_len = len;
	line.toCharArray(&pb.data[0], len + 1);

	int start_info = line.indexOf(':', 0);
	int end_ssid = line.indexOf(',', 0);
	int start_dst = line.indexOf('>', 2);
	int start_dstssid = line.indexOf('-', start_dst);

	if ((end_ssid < 0) || (end_ssid > start_info))
		end_ssid = start_info;

	if ((start_dstssid > start_dst) && (start_dstssid < start_dst + 10))
		pb.dstcall_end_or_ssid = &pb.data[start_dstssid];
	else
		pb.dstcall_end_or_ssid = &pb.data[end_ssid];

	pb.info_start = &pb.data[start_info + 1];
	pb.dstname = &pb.data[start_dst + 1];
	pb.dstname_len = end_ssid - start_dst;
	pb.dstcall_end = &pb.data[end_ssid];
	pb.srccall_end = &pb.data[start_dst];
}

// --- passCode() -------------------------------------------------------

void test_passcode_n0call(void)
{
	char call[] = "N0CALL";
	TEST_ASSERT_EQUAL_UINT16(13023, aprs.passCode(call));
}

void test_passcode_strips_ssid(void)
{
	char call[] = "N0CALL-9";
	TEST_ASSERT_EQUAL_UINT16(13023, aprs.passCode(call));
}

void test_passcode_is_case_insensitive(void)
{
	char call[] = "n0call";
	TEST_ASSERT_EQUAL_UINT16(13023, aprs.passCode(call));
}

void test_passcode_always_positive_15bit(void)
{
	char call[] = "OH2XYZ-15";
	uint16_t code = aprs.passCode(call);
	TEST_ASSERT_TRUE(code <= 0x7fff);
}

// --- distance() / direction() -----------------------------------------

void test_distance_same_point_is_zero(void)
{
	double d = aprs.distance(24.9, 60.2, 24.9, 60.2);
	TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, d);
}

void test_distance_one_degree_latitude(void)
{
	// ~111.1 km per degree of latitude near the equator.
	double d = aprs.distance(0.0, 0.0, 0.0, 1.0);
	TEST_ASSERT_DOUBLE_WITHIN(1.0, 111.13, d);
}

void test_direction_due_north_is_zero(void)
{
	double dir = aprs.direction(0.0, 0.0, 0.0, 1.0);
	TEST_ASSERT_DOUBLE_WITHIN(0.5, 0.0, dir);
}

void test_direction_due_east_is_90(void)
{
	double dir = aprs.direction(0.0, 0.0, 1.0, 0.0);
	TEST_ASSERT_DOUBLE_WITHIN(0.5, 90.0, dir);
}

// --- parse_aprs(): uncompressed position -------------------------------

void test_parse_uncompressed_position(void)
{
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75W-Test comment", pb);

	int rc = aprs.parse_aprs(&pb);

	TEST_ASSERT_EQUAL_INT(1, rc);
	TEST_ASSERT_TRUE(pb.packettype & T_POSITION);
	TEST_ASSERT_TRUE(pb.flags & F_HASPOS);
	TEST_ASSERT_FLOAT_WITHIN(0.001, 49.058333, pb.lat);
	TEST_ASSERT_FLOAT_WITHIN(0.001, -72.029167, pb.lng);
	TEST_ASSERT_EQUAL_CHAR('/', pb.symbol[0]);
	TEST_ASSERT_EQUAL_CHAR('-', pb.symbol[1]);
	TEST_ASSERT_EQUAL_STRING("Test comment", pb.comment);
}

// --- parse_aprs(): compressed position ----------------------------------

void test_parse_compressed_position(void)
{
	struct pbuf_t pb;
	// Body "/6/Y`QG2.>8NG" is lat=48.13863 lon=11.57341 (Munich), course=90deg,
	// speed=30kn, base91-encoded per the APRS Protocol Reference v1.2 spec.
	// Vector cross-checked against github.com/richonguzman/APRSPacketLib
	// (tools/aprs_vectors.json, "Munich" fixed_point + course/speed vectors).
	fill_pbuf_from_tnc2("OH2XYZ>APRS:!/6/Y`QG2.>8NG Munich Beacon", pb);

	int rc = aprs.parse_aprs(&pb);

	TEST_ASSERT_EQUAL_INT(1, rc);
	TEST_ASSERT_TRUE(pb.packettype & T_POSITION);
	TEST_ASSERT_TRUE(pb.flags & F_HASPOS);
	TEST_ASSERT_FLOAT_WITHIN(0.001, 48.13863, pb.lat);
	TEST_ASSERT_FLOAT_WITHIN(0.001, 11.57341, pb.lng);
	TEST_ASSERT_EQUAL_CHAR('/', pb.symbol[0]);
	TEST_ASSERT_EQUAL_CHAR('>', pb.symbol[1]);
	// Course/speed are base91-quantized, hence not exact: 91 -> encoded '8' -> back to 92.
	TEST_ASSERT_EQUAL_UINT16(92, pb.course);
	TEST_ASSERT_DOUBLE_WITHIN(0.5, 57.26, pb.speed);
	TEST_ASSERT_EQUAL_STRING(" Munich Beacon", pb.comment);
}

// --- parse_aprs(): message -----------------------------------------------

void test_parse_message(void)
{
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS::N0CALL2 :Hello World{001", pb);

	int rc = aprs.parse_aprs(&pb);

	TEST_ASSERT_EQUAL_INT(1, rc);
	TEST_ASSERT_TRUE(pb.packettype & T_MESSAGE);
}

// --- regression guard: full packet must survive the fixed-size buffer ---
// (src/gui_lcd.cpp and src/webservice.cpp used to build this pbuf with
// packet_len used as toCharArray's *buffer size*, silently dropping the
// packet's last byte - see FORK_NOTES.md.)

void test_full_comment_not_truncated(void)
{
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75W-END", pb);

	int rc = aprs.parse_aprs(&pb);

	TEST_ASSERT_EQUAL_INT(1, rc);
	TEST_ASSERT_EQUAL_STRING("END", pb.comment);
}

// A hand-built packet, not a real capture (this project has no live-traffic
// recording set up), covering something the tests above don't: a real
// digipeat path (WIDE1-1,WIDE2-1) ahead of the ':' and an uppercase
// symbol-table overlay character ('I'), both valid per
// valid_sym_table_uncompressed() but untested by the simpler fixtures above.

void test_parse_with_digipeat_path(void)
{
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL-9>APDR16,WIDE1-1,WIDE2-1:!5006.20NI00958.90E&Test comment", pb);

	int rc = aprs.parse_aprs(&pb);

	TEST_ASSERT_EQUAL_INT(1, rc);
	TEST_ASSERT_TRUE(pb.flags & F_HASPOS);
	TEST_ASSERT_FLOAT_WITHIN(0.001, 50.103333, pb.lat);
	TEST_ASSERT_FLOAT_WITHIN(0.001, 9.981667, pb.lng);
}

void setup()
{
	delay(2000); // let the board settle before the serial test runner attaches

	UNITY_BEGIN();
	RUN_TEST(test_passcode_n0call);
	RUN_TEST(test_passcode_strips_ssid);
	RUN_TEST(test_passcode_is_case_insensitive);
	RUN_TEST(test_passcode_always_positive_15bit);
	RUN_TEST(test_distance_same_point_is_zero);
	RUN_TEST(test_distance_one_degree_latitude);
	RUN_TEST(test_direction_due_north_is_zero);
	RUN_TEST(test_direction_due_east_is_90);
	RUN_TEST(test_parse_uncompressed_position);
	RUN_TEST(test_parse_compressed_position);
	RUN_TEST(test_parse_message);
	RUN_TEST(test_full_comment_not_truncated);
	RUN_TEST(test_parse_with_digipeat_path);
	UNITY_END();
}

void loop()
{
}
