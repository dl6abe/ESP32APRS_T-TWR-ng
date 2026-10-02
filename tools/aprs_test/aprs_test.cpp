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

// Regression guard: parse_aprs_comment()'s altitude-extension handling used
// to do "rest = res + 9" after finding "/A=nnnnnn", which discards
// everything *before* the match instead of excising just the "/A=nnnnnn"
// substring - any real comment text ahead of an altitude extension (a very
// common ordering) was silently dropped. Reported live 2026-09-28: a real
// packet's "MCX Bergsee JN48CS#NiWiS MCN/23/B=050" comment prefix vanished,
// leaving only the "/N7" that happened to follow the altitude field.
static void test_altitude_extension_preserves_surrounding_comment()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4845.27NM00811.11EsBergsee JN48CS/A=000666/N7", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "altitude extension: parse_aprs() returns 1");
	check(pb.flags & F_ALT, "altitude extension: F_ALT flag set");
	check((int)(pb.altitude + 0.5) == 203, "altitude extension: 000666 ft decodes to ~203 m");
	checkStr(pb.comment, "Bergsee JN48CS/N7", "altitude extension: comment text on both sides of /A=nnnnnn is preserved");
}

// Edge case for the same fix: a comment that is *only* an altitude
// extension, nothing before or after it. parse_remove_part() returns NULL
// when removing its match empties the string entirely - the caller has to
// handle that instead of strlen(NULL) crashing.
static void test_altitude_only_comment_does_not_crash()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4845.27NM00811.11Es/A=000666", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "altitude-only comment: parse_aprs() returns 1 (no crash)");
	check(pb.flags & F_ALT, "altitude-only comment: F_ALT flag still set");
}

// Ground truth: APRS Protocol Reference 1.2c (wb2osz/aprsspec, ch.12
// "Weather Reports"), "Complete Weather Report Format - with Lat/Long
// position and Timestamp" example, used verbatim. Checks every field this
// project decodes against the spec's own worked example, and documents the
// one deliberate simplification: the trailing APRS-Software/WX-Unit code
// ("wRSW") is left as part of the comment rather than stripped, since its
// unit-type half is an open-ended 2-4 char field ("Users may specify any
// other 2-4 character code") with no reliable way to tell where it ends
// and free-text comment begins - safer to keep it than risk eating real
// comment text on a guess.
static void test_wx_spec_example_complete_report()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:@092345z4903.50N/07201.75W_220/004g005t-07r000p000P000h50b09900wRSW", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "wx spec example: parse_aprs() returns 1");
	check(pb.wx_report.wind_dir == 220, "wx spec example: wind direction 220 deg");
	check(within(pb.wx_report.wind_speed, 4 * 0.44704, 0.01), "wx spec example: wind speed 004 mph decodes to m/s");
	check(within(pb.wx_report.wind_gust, 5 * 0.44704, 0.01), "wx spec example: gust 005 mph decodes to m/s");
	check(within(pb.wx_report.temp, (-7.0 - 32.0) / 1.8, 0.01), "wx spec example: temperature -07 F decodes to C");
	check(within(pb.wx_report.rain_1h, 0.0, 0.001), "wx spec example: rain last hour 000");
	check(within(pb.wx_report.rain_24h, 0.0, 0.001), "wx spec example: rain last 24h 000");
	check(within(pb.wx_report.rain_midnight, 0.0, 0.001), "wx spec example: rain since midnight 000");
	check(pb.wx_report.humidity == 50, "wx spec example: humidity 50%");
	check(within(pb.wx_report.pressure, 990.0, 0.01), "wx spec example: pressure 09900 -> 990.0 mbar");
	checkStr(pb.comment, "wRSW", "wx spec example: trailing software/unit code left in comment (not stripped)");
}

// Regression guard for the actual bug report (live traffic, 2026-09-28/29):
// weather packets whose free-text comment happens to contain letters that
// also name optional weather fields (t/r/p/h/b/l/u) got that comment
// silently chewed up, because the old code used strchr(rest, letter) - a
// search across the *entire* remaining text, including whatever comment
// followed the real (and possibly incomplete) weather data block. Per spec,
// "the remaining parameters may... not even exist", so a real packet
// missing e.g. the rain field is completely normal; the old code would
// then find the 'r' in "Wetterstation" instead (case sensitive - MUST use
// a real report's actual free text, not a synthetic string picked to avoid
// the letters). Only wind/gust/temp (mandatory) are present here - no
// rain/humidity/pressure at all - immediately followed by real free text.
static void test_wx_comment_survives_when_optional_fields_missing()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75W_220/004g005t077Wetterstation Batt=4.30V", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "wx missing-fields: parse_aprs() returns 1");
	check(pb.wx_report.wind_dir == 220, "wx missing-fields: wind direction still decodes");
	check(within(pb.wx_report.temp, (77.0 - 32.0) / 1.8, 0.01), "wx missing-fields: temperature still decodes");
	check(!(pb.wx_report.flags & W_R1H), "wx missing-fields: rain flag not set (field absent, not fabricated)");
	checkStr(pb.comment, "Wetterstation Batt=4.30V", "wx missing-fields: free-text comment is not corrupted");
}

// Spec: "Where an item of weather data is unknown or irrelevant, its value
// may be expressed as a series of dots or spaces." A placeholder field
// must still be recognized and stripped from the comment (it's real
// weather-report syntax, not free text) but must not produce a decoded
// value.
static void test_wx_dot_placeholders_stripped_not_decoded()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75W_220/004g...t...h50b09900Home Station", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "wx dot placeholders: parse_aprs() returns 1");
	check(!(pb.wx_report.flags & W_WG), "wx dot placeholders: gust flag not set for 'g...'");
	check(!(pb.wx_report.flags & W_TEMP), "wx dot placeholders: temp flag not set for 't...'");
	check(pb.wx_report.humidity == 50, "wx dot placeholders: a real field after the placeholders still decodes");
	checkStr(pb.comment, "Home Station", "wx dot placeholders: dots consumed as fields, not left in the comment");
}

// Spec: optional fields "may be in a different order". Deliberately out of
// the g/t/r/p/P/h/b canonical order here (pressure and humidity before the
// rain fields) to prove the field-by-field loop doesn't assume one fixed
// sequence.
static void test_wx_optional_fields_out_of_order()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75W_220/004g005t077b09900h50r000p000P000Reordered", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "wx out-of-order: parse_aprs() returns 1");
	check(within(pb.wx_report.pressure, 990.0, 0.01), "wx out-of-order: pressure decodes even though it came before rain");
	check(pb.wx_report.humidity == 50, "wx out-of-order: humidity decodes in its actual (non-canonical) position");
	check(within(pb.wx_report.rain_1h, 0.0, 0.001), "wx out-of-order: rain-1h still decodes after pressure/humidity");
	checkStr(pb.comment, "Reordered", "wx out-of-order: trailing comment still isolated correctly");
}

// Real-world comments (live traffic, 2026-09-29) using MeshCom's own
// "/B=nnn/Nn" battery-percent convention (see reference/MeshCom-Firmware,
// aprs_functions.cpp's log example: ".../B=005/A=000161/P=1004.9/...") -
// not an APRS-spec extension (only PHG, RNG and /A= are), so this parser
// must not try to interpret it and must pass all four straight through
// unmodified.
static void test_comment_unrecognized_vendor_extensions_pass_through()
{
	ParseAPRS aprs;
	struct pbuf_t pb;

	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75WrSOLAR MeshCom Test#Martin/B=081/N4", pb);
	aprs.parse_aprs(&pb);
	checkStr(pb.comment, "SOLAR MeshCom Test#Martin/B=081/N4", "vendor extension passthrough: MeshCom Test/B=/N comment untouched");

	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75WrSolarPanel 25W#Martin/B=090/N7", pb);
	aprs.parse_aprs(&pb);
	checkStr(pb.comment, "SolarPanel 25W#Martin/B=090/N7", "vendor extension passthrough: SolarPanel/B=/N comment untouched");

	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75Wr/B=100/N9", pb);
	aprs.parse_aprs(&pb);
	checkStr(pb.comment, "/B=100/N9", "vendor extension passthrough: bare /B=/N comment untouched");

	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75WrMCN Bergsee JN48CS#NiWiS MCN/17/B=100/N6", pb);
	aprs.parse_aprs(&pb);
	checkStr(pb.comment, "MCN Bergsee JN48CS#NiWiS MCN/17/B=100/N6", "vendor extension passthrough: Bergsee/B=/N comment untouched");
}

// Regression guard for real live traffic (2026-09-29: DL0BB-10, DK6GC-10,
// DL9CN-10, all "LoRa APRS.../WX Station" style devices): a weather report
// with *no* wind direction/speed field at all, going straight from the
// symbol into "g...t...". The outer weather-vs-plain-comment routing in
// parse_aprs_uncompressed() used to require a "nnn/sss" wind prefix or a
// leading 'c' before even trying parse_aprs_wx() - packets shaped like
// this fell through to parse_aprs_comment() instead, which doesn't know
// about weather fields, so the whole "g...t075h36b10204..." block showed
// up raw in the comment.
static void test_wx_no_wind_prefix_still_detected()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75W_g...t075h36b10204LoRa APRS OV A03 mit WX Station Batt=4.09V", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "wx no wind prefix: parse_aprs() returns 1");
	check(!(pb.wx_report.flags & W_WD), "wx no wind prefix: wind direction flag not set (field genuinely absent)");
	check(within(pb.wx_report.temp, (75.0 - 32.0) / 1.8, 0.01), "wx no wind prefix: temperature still decodes");
	check(pb.wx_report.humidity == 36, "wx no wind prefix: humidity still decodes");
	checkStr(pb.comment, "LoRa APRS OV A03 mit WX Station Batt=4.09V", "wx no wind prefix: free-text comment isolated correctly");
}

// Regression guard for the actual bug report (live traffic, 2026-09-29):
// same class of bug as test_wx_comment_survives_when_optional_fields_missing()
// above, but in the non-slash-prefixed wind course/speed branch (compressed-
// weather "cDDDsSSS..." prefix), which still used the old
// strchr(rest, letter)-anywhere-in-string approach instead of front-anchored
// wx_take_field(). A comment containing a bare 's' partway through a word
// ("Wetterstation") had 4 bytes ("stat") silently chewed out of it, because
// strchr(rest, 's') found that 's', not a real speed field.
static void test_wx_course_speed_prefix_does_not_eat_comment_letters()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("N0CALL>APRS:!4903.50N/07201.75W_g...t073h00b06925LoRa APRS iGate mit Wetterstation Batt=4.36V", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "wx course/speed prefix: parse_aprs() returns 1");
	check(within(pb.wx_report.temp, (73.0 - 32.0) / 1.8, 0.01), "wx course/speed prefix: temperature still decodes");
	checkStr(pb.comment, "LoRa APRS iGate mit Wetterstation Batt=4.36V", "wx course/speed prefix: 'Wetterstation' not corrupted into 'Wetterion'");
	// Spec: humidity "00" means 100%, not 0% (the field can't otherwise
	// encode 100 in two digits) - same bug report packet covers both fixes.
	check(pb.wx_report.humidity == 100, "wx course/speed prefix: humidity '00' decodes to 100%, not 0%");
}

// Regression guard for real live traffic (2026-09-29: DK6GC-10, DL0BB-10) -
// compressed-position weather reports (position starts with a base91
// symbol-table char, not a lat/lon digit) were never routed to
// parse_aprs_wx() at all: parse_aprs_compressed() unconditionally called
// parse_aprs_comment() on everything after the 13-byte compressed position
// block, regardless of sym_code, so the entire "g...t073h00b06925..." data
// block came out raw and undecoded in the comment. Same detection rule as
// parse_aprs_uncompressed() (sym_code == '_' or a leading 'g') was missing
// here entirely, not just buggy.
static void test_compressed_position_weather_report_is_decoded()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("DK6GC-10>APLRG1,TCPIP*,qAC,T2IRELAND:=L5o:sPSxr_ !G.../...g...t073h00b06925LoRa APRS iGate mit Wetterstation Batt=4.37V", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "compressed wx: parse_aprs() returns 1");
	check(within(pb.lat, 48.732944, 0.001), "compressed wx: position still decodes");
	check(within(pb.wx_report.temp, (73.0 - 32.0) / 1.8, 0.01), "compressed wx: temperature decodes");
	check(pb.wx_report.humidity == 100, "compressed wx: humidity '00' decodes to 100%");
	checkStr(pb.comment, "LoRa APRS iGate mit Wetterstation Batt=4.37V", "compressed wx: free-text comment isolated correctly");

	struct pbuf_t pb2;
	fill_pbuf_from_tnc2("DL0BB-10>APLRG1,TCPIP*,qAC,T2CSNGRAD:!L5m)zPTme_  G.../...g...t073h36b10210LoRa  APRS OV A03 mit WX Station Batt=4.09V", pb2);

	int rc2 = aprs.parse_aprs(&pb2);

	check(rc2 == 1, "compressed wx (2nd station): parse_aprs() returns 1");
	check(pb2.wx_report.humidity == 36, "compressed wx (2nd station): humidity decodes normally (not the '00' edge case)");
	checkStr(pb2.comment, "LoRa  APRS OV A03 mit WX Station Batt=4.09V", "compressed wx (2nd station): free-text comment isolated correctly (double space preserved)");
}

// Regression guard for real live traffic (2026-09-29, via aprs.fi):
// DH1GHL-9>TXTPT6,WIDE1-1,WIDE2-1,qAR,F4FXL-3:`~'Ml-E>/>"3C}144.575MHzHorst on Tour =
// A Mic-E position report whose status text contains the standard altitude
// extension (3 base91 digits + '}', spec ch.10) - a different encoding from
// the ASCII "/A=nnnnnn" extension parse_aprs_comment() already handles for
// other packet types. The Mic-E path didn't decode it at all: the 4 raw
// bytes leaked into the comment as undecoded noise, and the altitude was
// lost (F_ALT never set).
//
// Expected values cross-checked against the reference implementation this
// parser was ported from (Ham::APRS::FAP's _mice_to_decimal(), see
// https://github.com/gitpan/Ham-APRS-FAP/blob/master/FAP.pm): the
// status/comment text starts at offset 8 into the info field (not 9 - an
// earlier version of this fix, and this test, got that byte wrong, which is
// why the expected comment below keeps the leading '>' that a front-only
// offset-9 read used to swallow), and the altitude marker is searched for
// anywhere in that text (FAP's non-greedy `(.*?)([\x21-\x7b]{3})\}(.*)`),
// not just at its very front - real Mic-E radios commonly prefix it with
// free-text status (here, a literal leading '>' before the frequency spec).
// Found live 2026-09-30 (user report, e.g. SP7TBS-9 "6]}", DF4OR "_1") that
// the offset-9 version of this fix was itself still wrong.
static void test_mice_altitude_extension_decoded_and_stripped()
{
	ParseAPRS aprs;
	struct pbuf_t pb;
	fill_pbuf_from_tnc2("DH1GHL-9>TXTPT6,WIDE1-1,WIDE2-1,qAR,F4FXL-3:`~'Ml-E>/>\"3C}144.575MHzHorst on Tour =", pb);

	int rc = aprs.parse_aprs(&pb);

	check(rc == 1, "mic-e altitude: parse_aprs() returns 1");
	check(pb.flags & F_HASPOS, "mic-e altitude: position still decodes");
	check(pb.flags & F_ALT, "mic-e altitude: F_ALT flag set");
	check(within(pb.altitude, -47.0, 0.5), "mic-e altitude: altitude extension decodes to -47m");
	checkStr(pb.comment, ">144.575MHzHorst on Tour =", "mic-e altitude: altitude bytes stripped, status text preserved (leading '>' included)");
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
	test_altitude_extension_preserves_surrounding_comment();
	test_altitude_only_comment_does_not_crash();
	test_wx_spec_example_complete_report();
	test_wx_comment_survives_when_optional_fields_missing();
	test_wx_dot_placeholders_stripped_not_decoded();
	test_wx_optional_fields_out_of_order();
	test_comment_unrecognized_vendor_extensions_pass_through();
	test_wx_no_wind_prefix_still_detected();
	test_wx_course_speed_prefix_does_not_eat_comment_letters();
	test_compressed_position_weather_report_is_decoded();
	test_mice_altitude_extension_decoded_and_stripped();

	if (failures == 0)
		std::printf("All tests passed.\n");
	else
		std::printf("%d failure(s)\n", failures);

	return failures == 0 ? 0 : 1;
}
