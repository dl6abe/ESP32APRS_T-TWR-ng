#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <stddef.h>

/* SSID per IEEE 802.11: 0-32 octets, any byte value allowed on the wire.
 * This codebase stores SSIDs as fixed-size, null-terminated ASCII C-strings
 * (Configuration::wifi_ssid/wifi_ap_ssid, both char[32] - no room reserved
 * for a null terminator at the true 32-byte max) and displays/concatenates
 * them as plain text everywhere, so a raw-octet or multi-byte UTF-8 SSID
 * would break those assumptions. isValidWifiSSID() therefore requires
 * printable ASCII and reserves one byte for the terminator: length 1-31.
 * An empty SSID is rejected - this validates a name a user is assigning to
 * *their own* network, not an arbitrary received 802.11 SSID element. */
bool isValidWifiSSID(const char *ssid, size_t len);

/* WPA/WPA2-Personal ASCII passphrase per the WPA specification: 8-63
 * printable ASCII characters (fields are char[63], the true max, again
 * with no reserved terminator byte - so the enforced max here is 62).
 * A zero-length password is also accepted, representing an open/unsecured
 * network - that's a valid configuration this project's AP/STA settings
 * support, not a WPA passphrase at all. */
bool isValidWifiPassword(const char *pass, size_t len);

#endif
