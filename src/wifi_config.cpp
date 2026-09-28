#include "wifi_config.h"

static bool isPrintableAscii(const char *s, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		unsigned char c = (unsigned char)s[i];
		if (c < 0x20 || c > 0x7E)
			return false;
	}
	return true;
}

bool isValidWifiSSID(const char *ssid, size_t len)
{
	if (len < 1 || len > 31)
		return false;
	return isPrintableAscii(ssid, len);
}

bool isValidWifiPassword(const char *pass, size_t len)
{
	if (len == 0)
		return true; // open network, no passphrase
	if (len < 8 || len > 62)
		return false;
	return isPrintableAscii(pass, len);
}
