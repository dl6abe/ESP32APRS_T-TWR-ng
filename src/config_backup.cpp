/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

#include "config_fields.h"

#define F_(name, type) {#name, &config.name, type, sizeof(config.name)}
#define F_WIFI(i, field, type) {"wifi" #i "_" #field, &config.wifi_sta[i].field, type, sizeof(config.wifi_sta[i].field)}
#define F_PATH(i) {"path" #i, &config.path[i], CFT_STR, sizeof(config.path[i])}

const ConfigField configFields[] = {
	F_(timeZone, CFT_FLOAT),
	F_(synctime, CFT_BOOL),
	F_(title, CFT_BOOL),

	F_(wifi_mode, CFT_I8),
	F_(wifi_power, CFT_I8),
	F_WIFI(0, enable, CFT_BOOL), F_WIFI(0, wifi_ssid, CFT_STR), F_WIFI(0, wifi_pass, CFT_STR),
	F_WIFI(1, enable, CFT_BOOL), F_WIFI(1, wifi_ssid, CFT_STR), F_WIFI(1, wifi_pass, CFT_STR),
	F_WIFI(2, enable, CFT_BOOL), F_WIFI(2, wifi_ssid, CFT_STR), F_WIFI(2, wifi_pass, CFT_STR),
	F_WIFI(3, enable, CFT_BOOL), F_WIFI(3, wifi_ssid, CFT_STR), F_WIFI(3, wifi_pass, CFT_STR),
	F_WIFI(4, enable, CFT_BOOL), F_WIFI(4, wifi_ssid, CFT_STR), F_WIFI(4, wifi_pass, CFT_STR),
	F_(wifi_ap_ch, CFT_I8),
	F_(wifi_ap_ssid, CFT_STR),
	F_(wifi_ap_pass, CFT_STR),

	F_(bt_slave, CFT_BOOL),
	F_(bt_master, CFT_BOOL),
	F_(bt_mode, CFT_I8),
	F_(bt_uuid, CFT_STR),
	F_(bt_uuid_rx, CFT_STR),
	F_(bt_uuid_tx, CFT_STR),
	F_(bt_name, CFT_STR),
	F_(bt_pin, CFT_U32),
	F_(bt_power, CFT_I8),

	F_(rf_en, CFT_BOOL),
	F_(rf_type, CFT_U8),
	F_(freq_rx, CFT_FLOAT),
	F_(freq_tx, CFT_FLOAT),
	F_(offset_rx, CFT_INT),
	F_(offset_tx, CFT_INT),
	F_(tone_rx, CFT_INT),
	F_(tone_tx, CFT_INT),
	F_(band, CFT_U8),
	F_(sql_level, CFT_U8),
	F_(rf_power, CFT_BOOL),
	F_(volume, CFT_U8),
	F_(mic, CFT_U8),

	F_(igate_en, CFT_BOOL),
	F_(rf2inet, CFT_BOOL),
	F_(inet2rf, CFT_BOOL),
	F_(igate_loc2rf, CFT_BOOL),
	F_(igate_loc2inet, CFT_BOOL),
	F_(rf2inetFilter, CFT_U16),
	F_(inet2rfFilter, CFT_U16),
	F_(aprs_ssid, CFT_U8),
	F_(aprs_port, CFT_U16),
	F_(aprs_mycall, CFT_STR),
	F_(aprs_host, CFT_STR),
	F_(aprs_passcode, CFT_STR),
	F_(aprs_moniCall, CFT_STR),
	F_(aprs_filter, CFT_STR),
	F_(igate_bcn, CFT_BOOL),
	F_(igate_gps, CFT_BOOL),
	F_(igate_timestamp, CFT_BOOL),
	F_(igate_lat, CFT_FLOAT),
	F_(igate_lon, CFT_FLOAT),
	F_(igate_alt, CFT_FLOAT),
	F_(igate_interval, CFT_U16),
	F_(igate_symbol, CFT_STR),
	F_(igate_object, CFT_STR),
	F_(igate_phg, CFT_STR),
	F_(igate_path, CFT_U8),
	F_(igate_comment, CFT_STR),

	F_(digi_en, CFT_BOOL),
	F_(digi_loc2rf, CFT_BOOL),
	F_(digi_loc2inet, CFT_BOOL),
	F_(digi_timestamp, CFT_BOOL),
	F_(digi_ssid, CFT_U8),
	F_(digi_mycall, CFT_STR),
	F_(digi_path, CFT_U8),
	F_(digi_delay, CFT_U16),
	F_(digiFilter, CFT_U16),
	F_(digi_bcn, CFT_BOOL),
	F_(digi_compress, CFT_BOOL),
	F_(digi_altitude, CFT_BOOL),
	F_(digi_gps, CFT_BOOL),
	F_(digi_lat, CFT_FLOAT),
	F_(digi_lon, CFT_FLOAT),
	F_(digi_alt, CFT_FLOAT),
	F_(digi_interval, CFT_U16),
	F_(digi_symbol, CFT_STR),
	F_(digi_phg, CFT_STR),
	F_(digi_comment, CFT_STR),

	F_(trk_en, CFT_BOOL),
	F_(trk_loc2rf, CFT_BOOL),
	F_(trk_loc2inet, CFT_BOOL),
	F_(trk_timestamp, CFT_BOOL),
	F_(trk_ssid, CFT_U8),
	F_(trk_mycall, CFT_STR),
	F_(trk_path, CFT_U8),
	F_(trk_gps, CFT_BOOL),
	F_(trk_lat, CFT_FLOAT),
	F_(trk_lon, CFT_FLOAT),
	F_(trk_alt, CFT_FLOAT),
	F_(trk_interval, CFT_U16),
	F_(trk_smartbeacon, CFT_BOOL),
	F_(trk_compress, CFT_BOOL),
	F_(trk_altitude, CFT_BOOL),
	F_(trk_cst, CFT_BOOL),
	F_(trk_bat, CFT_BOOL),
	F_(trk_sat, CFT_BOOL),
	F_(trk_dx, CFT_BOOL),
	F_(trk_hspeed, CFT_U16),
	F_(trk_lspeed, CFT_U8),
	F_(trk_maxinterval, CFT_U8),
	F_(trk_mininterval, CFT_U8),
	F_(trk_minangle, CFT_U8),
	F_(trk_slowinterval, CFT_U16),
	F_(trk_symbol, CFT_STR),
	F_(trk_symmove, CFT_STR),
	F_(trk_symstop, CFT_STR),
	F_(trk_comment, CFT_STR),
	F_(trk_item, CFT_STR),

	F_(oled_enable, CFT_BOOL),
	F_(oled_timeout, CFT_INT),
	F_(dim, CFT_U8),
	F_(contrast, CFT_U8),
	F_(startup, CFT_U8),

	F_(dispDelay, CFT_INT),
	F_(filterDistant, CFT_INT),
	F_(h_up, CFT_BOOL),
	F_(tx_display, CFT_BOOL),
	F_(rx_display, CFT_BOOL),
	F_(dispFilter, CFT_U16),
	F_(dispRF, CFT_BOOL),
	F_(dispINET, CFT_BOOL),

	F_(audio_hpf, CFT_BOOL),
	F_(audio_bpf, CFT_BOOL),
	F_(preamble, CFT_U8),
	F_(tx_timeslot, CFT_U16),
	F_(ntp_host, CFT_STR),

	F_(vpn, CFT_BOOL),
	F_(modem, CFT_BOOL),
	F_(wg_port, CFT_U16),
	F_(wg_peer_address, CFT_STR),
	F_(wg_local_address, CFT_STR),
	F_(wg_netmask_address, CFT_STR),
	F_(wg_gw_address, CFT_STR),
	F_(wg_public_key, CFT_STR),
	F_(wg_private_key, CFT_STR),

	F_(http_username, CFT_STR),
	F_(http_password, CFT_STR),

	F_PATH(0), F_PATH(1), F_PATH(2), F_PATH(3),

	F_(gpio_sql_pin, CFT_U8),

	F_(logCategoryMask, CFT_U16),
	F_(syslog_en, CFT_BOOL),
	F_(syslog_host, CFT_STR),
	F_(syslog_port, CFT_U16),

	F_(posixTZ, CFT_STR),
};

const size_t configFieldCount = sizeof(configFields) / sizeof(configFields[0]);

String buildConfigBackup()
{
	String out;
	out.reserve(4096);
	out += "# ESP32APRS T-TWR config backup\n";
	out += "# firmware=V" + String(VERSION) + String(VERSION_BUILD) + "\n";
	for (size_t i = 0; i < configFieldCount; i++)
	{
		const ConfigField &f = configFields[i];
		out += String(f.name) + "=";
		switch (f.type)
		{
		case CFT_BOOL:
			out += String(*(bool *)f.ptr ? 1 : 0);
			break;
		case CFT_I8:
			out += String((int)*(char *)f.ptr);
			break;
		case CFT_U8:
			out += String((unsigned int)*(uint8_t *)f.ptr);
			break;
		case CFT_U16:
			out += String(*(uint16_t *)f.ptr);
			break;
		case CFT_U32:
			out += String(*(uint32_t *)f.ptr);
			break;
		case CFT_INT:
			out += String(*(int *)f.ptr);
			break;
		case CFT_FLOAT:
			out += String(*(float *)f.ptr, 6);
			break;
		case CFT_STR:
			out += String((char *)f.ptr);
			break;
		}
		out += "\n";
	}
	return out;
}

void applyConfigBackup(const String &text)
{
	int pos = 0;
	int len = text.length();
	int applied = 0, unknown = 0;
	while (pos < len)
	{
		int nl = text.indexOf('\n', pos);
		if (nl < 0)
			nl = len;
		String line = text.substring(pos, nl);
		pos = nl + 1;
		line.trim();
		if (line.length() == 0 || line[0] == '#')
			continue;
		int eq = line.indexOf('=');
		if (eq < 0)
			continue;
		String key = line.substring(0, eq);
		String value = line.substring(eq + 1);

		bool found = false;
		for (size_t i = 0; i < configFieldCount; i++)
		{
			if (key != configFields[i].name)
				continue;
			found = true;
			const ConfigField &f = configFields[i];
			switch (f.type)
			{
			case CFT_BOOL:
				*(bool *)f.ptr = (value.toInt() != 0);
				break;
			case CFT_I8:
				*(char *)f.ptr = (char)value.toInt();
				break;
			case CFT_U8:
				*(uint8_t *)f.ptr = (uint8_t)value.toInt();
				break;
			case CFT_U16:
				*(uint16_t *)f.ptr = (uint16_t)value.toInt();
				break;
			case CFT_U32:
				*(uint32_t *)f.ptr = (uint32_t)value.toInt();
				break;
			case CFT_INT:
				*(int *)f.ptr = value.toInt();
				break;
			case CFT_FLOAT:
				*(float *)f.ptr = value.toFloat();
				break;
			case CFT_STR:
				strncpy((char *)f.ptr, value.c_str(), f.size - 1);
				((char *)f.ptr)[f.size - 1] = 0;
				break;
			}
			applied++;
			break;
		}
		if (!found)
			unknown++;
	}
	projLog(LOGCAT_SYSTEM, "Config restore: %d fields applied, %d unknown keys skipped", applied, unknown);
}
