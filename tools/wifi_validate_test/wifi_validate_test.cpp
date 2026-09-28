// Host-native tests for isValidWifiSSID()/isValidWifiPassword()
// (src/wifi_config.cpp), built directly against the real production file -
// no Arduino shim needed at all, since wifi_config.cpp has zero Arduino
// dependencies. Run: tools/wifi_validate_test/run_wifi_validate_test.sh
//
// No test framework: each case calls check() independently and keeps going
// on failure, matching the convention in tools/aprs_test/.

#include <cstdio>
#include <cstring>
#include <wifi_config.h>

static int failures = 0;

static void check(bool condition, const char *description)
{
	if (!condition)
	{
		failures++;
		std::printf("FAIL: %s\n", description);
	}
}

static void checkSSIDLen(size_t len, bool want, const char *description)
{
	char ssid[64];
	std::memset(ssid, 'A', sizeof(ssid));
	check(isValidWifiSSID(ssid, len) == want, description);
}

static void checkPassLen(size_t len, bool want, const char *description)
{
	char pass[64];
	std::memset(pass, 'A', sizeof(pass));
	check(isValidWifiPassword(pass, len) == want, description);
}

static void test_ssid_length_boundaries()
{
	checkSSIDLen(0, false, "SSID: empty is rejected");
	checkSSIDLen(1, true, "SSID: length 1 (minimum) is accepted");
	checkSSIDLen(31, true, "SSID: length 31 (max, one byte reserved for the null terminator in a char[32] field) is accepted");
	checkSSIDLen(32, false, "SSID: length 32 is rejected (would leave no room for the terminator in a char[32] field)");
	checkSSIDLen(33, false, "SSID: length 33 is rejected");
}

static void test_ssid_characters()
{
	check(isValidWifiSSID("MyHomeNetwork", 13), "SSID: plain ASCII name is accepted");
	check(isValidWifiSSID("Cafe 42!", 8), "SSID: spaces and punctuation are accepted");

	char withNul[5] = {'A', 'B', '\0', 'C', 'D'};
	check(!isValidWifiSSID(withNul, 5), "SSID: embedded NUL byte is rejected");

	char withControl[5] = {'A', 'B', 0x01, 'C', 'D'};
	check(!isValidWifiSSID(withControl, 5), "SSID: control character (0x01) is rejected");

	char withDel[5] = {'A', 'B', 0x7F, 'C', 'D'};
	check(!isValidWifiSSID(withDel, 5), "SSID: DEL (0x7F) is rejected");

	char withHighByte[5] = {'A', 'B', (char)0x80, 'C', 'D'};
	check(!isValidWifiSSID(withHighByte, 5), "SSID: non-ASCII byte (0x80) is rejected");
}

static void test_password_length_boundaries()
{
	checkPassLen(0, true, "Password: empty is accepted (open/no-security network)");
	checkPassLen(1, false, "Password: length 1 is rejected (too short for WPA, not empty)");
	checkPassLen(7, false, "Password: length 7 is rejected (below the WPA minimum of 8)");
	checkPassLen(8, true, "Password: length 8 (WPA minimum) is accepted");
	checkPassLen(62, true, "Password: length 62 (max, one byte reserved for the null terminator in a char[63] field) is accepted");
	checkPassLen(63, false, "Password: length 63 is rejected (would leave no room for the terminator in a char[63] field)");
	checkPassLen(64, false, "Password: length 64 is rejected");
}

static void test_password_characters()
{
	check(isValidWifiPassword("correcthorsebattery", 19), "Password: plain ASCII passphrase is accepted");
	check(isValidWifiPassword("P@ssw0rd! 2024", 14), "Password: punctuation and spaces are accepted");

	char withControl[10] = {'p', 'a', 's', 's', 0x09, 'w', 'o', 'r', 'd', '!'};
	check(!isValidWifiPassword(withControl, 10), "Password: control character (tab) is rejected");

	char withHighByte[9] = {'p', 'a', 's', 's', (char)0xFF, 'w', 'o', 'r', 'd'};
	check(!isValidWifiPassword(withHighByte, 9), "Password: non-ASCII byte (0xFF) is rejected");
}

int main()
{
	test_ssid_length_boundaries();
	test_ssid_characters();
	test_password_length_boundaries();
	test_password_characters();

	if (failures == 0)
		std::printf("All tests passed.\n");
	else
		std::printf("%d failure(s)\n", failures);

	return failures == 0 ? 0 : 1;
}
