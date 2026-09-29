/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/
#include "AFSK.h"
#include "webservice.h"
#include "wireguard_vpn.h"
#include <LibAPRSesp.h>
#include <parse_aprs.h>
#include "wifi_config.h"
#include "gui_lcd.h" // for oledWake()/displayApplyPending - request display changes, mainDisp task applies them


void handle_system()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	if (server.hasArg("updateTimeZone"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "SetPosixTZ")
			{
				if (server.arg(i).length() < sizeof(config.posixTZ) && isValidPosixTZ(server.arg(i).c_str()))
				{
					strncpy(config.posixTZ, server.arg(i).c_str(), sizeof(config.posixTZ) - 1);
					config.posixTZ[sizeof(config.posixTZ) - 1] = 0;
					applyTimeZone();
				}
				break;
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("updateTimeNtp"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "SetTimeNtp")
			{
				if (server.arg(i) != "")
				{
					// Serial.println("WEB Config NTP");
					strcpy(config.ntp_host, server.arg(i).c_str());
					applyTimeZone();
				}
				break;
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("updateTime"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "SetTime")
			{
				if (server.arg(i) != "")
				{
					// struct tm tmn;
					String date = getValue(server.arg(i), ' ', 0);
					String time = getValue(server.arg(i), ' ', 1);
					int yyyy = getValue(date, '-', 0).toInt();
					int mm = getValue(date, '-', 1).toInt();
					int dd = getValue(date, '-', 2).toInt();
					int hh = getValue(time, ':', 0).toInt();
					int ii = getValue(time, ':', 1).toInt();
					int ss = getValue(time, ':', 2).toInt();
					// int ss = 0;

					tmElements_t timeinfo;
					timeinfo.Year = yyyy - 1970;
					timeinfo.Month = mm;
					timeinfo.Day = dd;
					timeinfo.Hour = hh;
					timeinfo.Minute = ii;
					timeinfo.Second = ss;
					time_t timeStamp = makeTime(timeinfo);

					// tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec

					time_t rtc = timeStamp - localTZOffsetSeconds(timeStamp);
					timeval tv = {rtc, 0};
					timezone tz = {0, 0}; // ignored by the actual localtime()/DST machinery - TZ env (applyTimeZone()) is what matters
					settimeofday(&tv, &tz);

					// Serial.println("Update TIME " + server.arg(i));
					Serial.print("Set New Time at ");
					Serial.print(dd);
					Serial.print("/");
					Serial.print(mm);
					Serial.print("/");
					Serial.print(yyyy);
					Serial.print(" ");
					Serial.print(hh);
					Serial.print(":");
					Serial.print(ii);
					Serial.print(":");
					Serial.print(ss);
					Serial.print(" ");
					Serial.println(timeStamp);
				}
				break;
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("REBOOT"))
	{
		esp_restart();
	}
	else if (server.hasArg("commitWebAuth"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "webauth_user")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.http_username, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "webauth_pass")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.http_password, server.arg(i).c_str());
				}
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}else if (server.hasArg("commitPath"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "path1")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.path[0], server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "path2")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.path[1], server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "path3")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.path[1], server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "path4")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.path[3], server.arg(i).c_str());
				}
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitDISP"))
	{
		bool dispRX = false;
		bool dispTX = false;
		bool dispRF = false;
		bool dispINET = false;
		bool oledEN = false;

		config.dispFilter = 0;

		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "oledEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						oledEN = true;
					}
				}
			}
			if (server.argName(i) == "filterMessage")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_MESSAGE;
				}
			}

			if (server.argName(i) == "filterTelemetry")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_TELEMETRY;
				}
			}

			if (server.argName(i) == "filterStatus")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_STATUS;
				}
			}

			if (server.argName(i) == "filterWeather")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_WX;
				}
			}

			if (server.argName(i) == "filterObject")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_OBJECT;
				}
			}

			if (server.argName(i) == "filterItem")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_ITEM;
				}
			}

			if (server.argName(i) == "filterQuery")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_QUERY;
				}
			}
			if (server.argName(i) == "filterBuoy")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_BUOY;
				}
			}
			if (server.argName(i) == "filterPosition")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_POSITION;
				}
			}

			if (server.argName(i) == "dispRF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						dispRF = true;
				}
			}

			if (server.argName(i) == "dispINET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						dispINET = true;
				}
			}
			if (server.argName(i) == "txdispEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						dispTX = true;
				}
			}
			if (server.argName(i) == "rxdispEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						dispRX = true;
				}
			}

			if (server.argName(i) == "dispDelay")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						int dispDelay = server.arg(i).toInt();
						config.dispDelay = (dispDelay < 0) ? 0 : (unsigned int)dispDelay;
					}
				}
			}

			if (server.argName(i) == "oled_timeout")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.oled_timeout = server.arg(i).toInt();
						if (config.oled_timeout < 0)
							config.oled_timeout = 0;
					}
				}
			}
			if (server.argName(i) == "dimMode")
			{
				if (server.arg(i) != "" && isValidNumber(server.arg(i)))
				{
					int dimMode = server.arg(i).toInt();
					config.dim = (dimMode < 0 || dimMode > 4) ? 0 : dimMode;
				}
			}
			if (server.argName(i) == "contrast")
			{
				if (server.arg(i) != "" && isValidNumber(server.arg(i)))
				{
					int contrast = server.arg(i).toInt();
					config.contrast = (contrast < 0) ? 0 : (contrast > 200 ? 200 : contrast);
				}
			}
			if (server.argName(i) == "filterDX")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.filterDistant = server.arg(i).toInt();
					}
				}
			}
		}

		// Don't touch display.* from this task (taskNetwork) - mainDisp owns
		// the I2C bus (see gui_lcd.h comment on applyPendingDisplaySettings()).
		// Just flag it; mainDisp applies dim/contrast on its own next frame.
		displayApplyPending = true;
		oledWake(); // don't let a sleeping panel mask a brightness change made from the web UI either

		config.oled_enable = oledEN;
		config.dispINET = dispINET;
		config.dispRF = dispRF;
		config.rx_display = dispRX;
		config.tx_display = dispTX;
		// config.filterMessage = filterMessage;
		// config.filterStatus = filterStatus;
		// config.filterTelemetry = filterTelemetry;
		// config.filterWeather = filterWeather;
		// config.filterTracker = filterTracker;
		// config.filterMove = filterMove;
		// config.filterPosition = filterPosition;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitDebugLog"))
	{
		config.logCategoryMask = 0;
		config.syslog_en = false;

		for (uint8_t i = 0; i < server.args(); i++)
		{
			String name = server.argName(i);
			String val = server.arg(i);
			if (name == "logSystem" && val == "OK")
				config.logCategoryMask |= LOGCAT_SYSTEM;
			else if (name == "logWeb" && val == "OK")
				config.logCategoryMask |= LOGCAT_WEB;
			else if (name == "logGps" && val == "OK")
				config.logCategoryMask |= LOGCAT_GPS;
			else if (name == "logAprsRf" && val == "OK")
				config.logCategoryMask |= LOGCAT_APRS_RF;
			else if (name == "logAprsInet" && val == "OK")
				config.logCategoryMask |= LOGCAT_APRS_INET;
			else if (name == "logRfModule" && val == "OK")
				config.logCategoryMask |= LOGCAT_RF_MODULE;
			else if (name == "logBluetooth" && val == "OK")
				config.logCategoryMask |= LOGCAT_BLUETOOTH;
			else if (name == "syslogEnable" && val == "OK")
				config.syslog_en = true;
			else if (name == "syslogHost")
			{
				strncpy(config.syslog_host, val.c_str(), sizeof(config.syslog_host) - 1);
				config.syslog_host[sizeof(config.syslog_host) - 1] = 0;
			}
			else if (name == "syslogPort" && val != "")
				config.syslog_port = val.toInt();
		}
		saveEEPROM();
		syslogReconnect();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{

		struct tm tmstruct;
		char strTime[32]; // headroom for the compiler's worst-case %d width on tm_year
		tmstruct.tm_year = 0;
		getLocalTime(&tmstruct, 5000);
		snprintf(strTime, sizeof(strTime), "%d-%02d-%02d %02d:%02d:%02d", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);

		String html = "<script type=\"text/javascript\">\n";
		html += "document.querySelectorAll('form').forEach(function (form) {\n";
		html += "form.addEventListener('submit', function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formTime\") document.getElementById(\"updateTime\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formNTP\") document.getElementById(\"updateTimeNtp\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formTimeZone\") document.getElementById(\"updateTimeZone\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formReboot\") document.getElementById(\"REBOOT\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formDisp\") document.getElementById(\"submitDISP\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formWebAuth\") document.getElementById(\"submitWebAuth\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formDebugLog\") document.getElementById(\"submitDebugLog\").disabled=true;\n";
		html += "fetch('/system', { method: 'POST', body: data })\n";
		html += ".then(function () { alert(\"Submited Successfully\"); })\n";
		html += ".catch(function () { alert(\"An error occurred.\"); });\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n";

		html += "<div class=\"dash\">\n";
		html += "<div class=\"dash-section-title\">System Setting</div>\n";
		html += "<div class=\"dash-panel\">\n";

		html += "<div class=\"dash-field\"><label>Local Date/Time</label><div class=\"dash-field-body\"><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formTime\" method=\"post\">\n<input name=\"SetTime\" type=\"text\" value=\"" + String(strTime) + "\" />\n";
		html += "<button type='submit' id='updateTime' name=\"commit\">Time Update</button>\n";
		html += "<input type=\"hidden\" name=\"updateTime\"/></form></div></div>\n";

		html += "<div class=\"dash-field\"><label>NTP Host</label><div class=\"dash-field-body\"><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formNTP\" method=\"post\"><input name=\"SetTimeNtp\" type=\"text\" value=\"" + String(config.ntp_host) + "\" />\n";
		html += "<button type='submit' id='updateTimeNtp' name=\"commit\">NTP Update</button>\n";
		html += "<input type=\"hidden\" name=\"updateTimeNtp\"/></form></div></div>\n";

		html += "<div class=\"dash-field\"><label>Time Zone</label><div class=\"dash-field-body\"><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formTimeZone\" method=\"post\">\n";
		html += "<input name=\"SetPosixTZ\" type=\"text\" size=\"30\" maxlength=\"" + String(sizeof(config.posixTZ) - 1) + "\" value=\"" + escapeHtml(String(config.posixTZ)) + "\" placeholder=\"e.g. CET-1CEST,M3.5.0,M10.5.0/3\" />\n";
		html += "<button type='submit' id='updateTimeZone' name=\"commit\">TZ Update</button>\n";
		html += "<span class=\"dash-hint\">POSIX TZ string - handles DST automatically. Reference: <a href=\"https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv\" target=\"_blank\">timezone table</a></span>\n";
		html += "<input type=\"hidden\" name=\"updateTimeZone\"/></form></div></div>\n";

		html += "<div class=\"dash-field\"><label>System Reboot</label><div class=\"dash-field-body\"><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formReboot\" method=\"post\"><button type='submit' id='REBOOT' name=\"commit\" style=\"background-color:red;color:white\">REBOOT</button>\n";
		html += "<input type=\"hidden\" name=\"REBOOT\"/></form></div></div>\n";
		html += "</div>\n"; // .dash-panel

		/************************ CONFIG BACKUP/RESTORE **************************/
		// Plain key=value text (config_fields.h/config_backup.cpp), not the raw
		// EEPROM bytes the OLED's Save/Load menu items use - this survives a
		// firmware update that changes sizeof(Configuration), see FORK_NOTES.md.
		html += "<div class=\"dash-section-title\">Config Backup / Restore</div>\n";
		html += "<div class=\"dash-panel\">\n";
		html += "<div class=\"dash-field\"><label>Backup</label><div class=\"dash-field-body\"><a href=\"/configBackup\" download=\"esp32aprs-config.cfg\"><button type='button'>Download Backup</button></a></div></div>\n";
		html += "<div class=\"dash-field\"><label>Restore</label><div class=\"dash-field-body\"><form method='POST' action='#' enctype='multipart/form-data' id='formConfigRestore'>";
		html += "<input id=\"restoreFile\" name=\"restore\" type=\"file\" />";
		html += "<button type='submit' id='submitConfigRestore' style=\"background-color:red;color:white\">Restore &amp; Reboot</button>";
		html += "</form></div></div>\n";
		html += "</div>\n"; // .dash-panel
		html += "<script>"
				"document.getElementById('formConfigRestore').addEventListener('submit', function(e){"
				"e.preventDefault();"
				"if(!document.getElementById('restoreFile').files.length){return;}"
				"if(!confirm('This replaces the current configuration and reboots the device. Continue?')){return;}"
				"document.getElementById('submitConfigRestore').disabled = true;"
				"var data = new FormData(document.getElementById('formConfigRestore'));"
				"var xhr = new XMLHttpRequest();"
				"xhr.addEventListener('load', function() { alert('Restored. Rebooting...'); });"
				"xhr.addEventListener('error', function() { alert('Restore failed.'); });"
				"xhr.open('POST', '/configRestore');"
				"xhr.send(data);"
				"});"
				"</script>\n";

		/************************ WEB AUTH **************************/
		html += "<form id='formWebAuth' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">Web Authentication</div>\n";
		html += "<div class=\"dash-panel\">\n";
		html += "<div class=\"dash-field\"><label>Web USER</label><div class=\"dash-field-body\"><input size=\"32\" maxlength=\"32\" class=\"form-control\" name=\"webauth_user\" type=\"text\" value=\"" + String(config.http_username) + "\" /></div></div>\n";
		html += "<div class=\"dash-field\"><label>Web PASSWORD</label><div class=\"dash-field-body\"><input size=\"63\" maxlength=\"63\" class=\"form-control\" name=\"webauth_pass\" type=\"password\" value=\"" + String(config.http_password) + "\" /></div></div>\n";
		html += "</div>\n"; // .dash-panel
		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitWebAuth' name=\"commit\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWebAuth\"/>\n";
		html += "</form>\n";

		/************************ DEBUG LOGGING **************************/
		// Runtime-toggleable console/syslog categories for this project's own
		// log_d()-equivalent calls migrated to projLog() (main.h/log_output.cpp)
		// - separate from CORE_DEBUG_LEVEL (platformio.ini), which is a
		// compile-time cap covering the whole firmware including framework
		// internals; see the LOGCAT_* comment in main.h for why these are two
		// different knobs, not one.
		html += "<form id='formDebugLog' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">Debug Logging</div>\n";
		html += "<div class=\"dash-panel\">\n";
		html += "<div class=\"dash-field\"><label>Categories</label><div class=\"dash-field-body\">\n";
		html += "<label><input type=\"checkbox\" name=\"logSystem\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_SYSTEM) ? " checked" : "") + " /> System</label>\n";
		html += "<label><input type=\"checkbox\" name=\"logWeb\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_WEB) ? " checked" : "") + " /> Web</label>\n";
		html += "<label><input type=\"checkbox\" name=\"logGps\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_GPS) ? " checked" : "") + " /> GPS</label>\n";
		html += "<label><input type=\"checkbox\" name=\"logAprsRf\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_APRS_RF) ? " checked" : "") + " /> APRS RF</label>\n";
		html += "<label><input type=\"checkbox\" name=\"logAprsInet\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_APRS_INET) ? " checked" : "") + " /> APRS Internet</label>\n";
		html += "<label><input type=\"checkbox\" name=\"logRfModule\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_RF_MODULE) ? " checked" : "") + " /> RF Module</label>\n";
		html += "<label><input type=\"checkbox\" name=\"logBluetooth\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_BLUETOOTH) ? " checked" : "") + " /> Bluetooth</label>\n";
		html += "</div></div>\n";
		html += "<div class=\"dash-field\"><label>Syslog Server</label><div class=\"dash-field-body\">\n";
		html += "<label><input type=\"checkbox\" name=\"syslogEnable\" value=\"OK\"" + String(config.syslog_en ? " checked" : "") + " /> Enable</label>\n";
		html += "<label>Host <input size=\"30\" maxlength=\"39\" name=\"syslogHost\" type=\"text\" value=\"" + String(config.syslog_host) + "\" /></label>\n";
		html += "<label>Port <input size=\"6\" name=\"syslogPort\" type=\"number\" min=\"1\" max=\"65535\" value=\"" + String(config.syslog_port) + "\" /></label>\n";
		html += "</div></div>\n";
		html += "</div>\n"; // .dash-panel
		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitDebugLog' name=\"commit\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitDebugLog\"/>\n";
		html += "</form>\n";

		/************************ PATH USER define **************************/
		html += "<form id='formPath' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">PATH USER Define</div>\n";
		html += "<div class=\"dash-panel\">\n";
		html += "<div class=\"dash-field\"><label>PATH_1</label><div class=\"dash-field-body\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path1\" type=\"text\" value=\"" + String(config.path[0]) + "\" /></div></div>\n";
		html += "<div class=\"dash-field\"><label>PATH_2</label><div class=\"dash-field-body\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path2\" type=\"text\" value=\"" + String(config.path[1]) + "\" /></div></div>\n";
		html += "<div class=\"dash-field\"><label>PATH_3</label><div class=\"dash-field-body\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path3\" type=\"text\" value=\"" + String(config.path[2]) + "\" /></div></div>\n";
		html += "<div class=\"dash-field\"><label>PATH_4</label><div class=\"dash-field-body\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path4\" type=\"text\" value=\"" + String(config.path[3]) + "\" /></div></div>\n";
		html += "</div>\n"; // .dash-panel
		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitPath' name=\"commitPath\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitPath\"/>\n";
		html += "</form>\n";

		html += "<form id='formDisp' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">Display Setting</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String oledFlageEn = "";
		if (config.oled_enable == true)
			oledFlageEn = "checked";
		html += "<div class=\"dash-field\"><label>OLED Enable</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"oledEnable\" value=\"OK\" " + oledFlageEn + "><span class=\"slider round\"></span></label></div></div>\n";

		String hupFlageEn = "";
		if (config.h_up == true)
			hupFlageEn = "checked";
		html += "<div class=\"dash-field\"><label>Head Up</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"hupEnable\" value=\"OK\" " + hupFlageEn + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">The compass will rotate in the direction of movement.</span></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"oled_timeout\">OLED Sleep</label><div class=\"dash-field-body\"><select name=\"oled_timeout\" id=\"oled_timeout\">\n";
		for (int i = 0; i <= 600; i += 30)
		{
			String label = (i == 0) ? "Never" : (String(i) + " Sec");
			if (config.oled_timeout == i)
				html += "<option value=\"" + String(i) + "\" selected>" + label + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + label + "</option>\n";
		}
		html += "</select></div></div>\n";

		// Same 5 modes as the OLED-menu's DIM combobox (gui_menu_system.cpp) -
		// was device-menu-only before this, no web exposure.
		html += "<div class=\"dash-field\"><label for=\"dimMode\">Dim Mode*</label><div class=\"dash-field-body\"><select name=\"dimMode\" id=\"dimMode\">\n";
		{
			const char *dimNames[5] = {"HI", "LOW", "AUTO DIM", "DAY/NIGHT", "CONTRAST"};
			for (int i = 0; i < 5; i++)
			{
				if (config.dim == i)
					html += "<option value=\"" + String(i) + "\" selected>" + String(dimNames[i]) + "</option>\n";
				else
					html += "<option value=\"" + String(i) + "\" >" + String(dimNames[i]) + "</option>\n";
			}
		}
		html += "</select><span class=\"dash-hint\">*HI: always full brightness.<br />LOW: always dimmed.<br />AUTO DIM: full brightness, dims after 60s idle.<br />DAY/NIGHT: full brightness 05:00-19:00, dimmed otherwise.<br />CONTRAST: fixed brightness set by the Contrast value below.</span></div></div>\n";

		html += "<div class=\"dash-field\"><label>Contrast</label><div class=\"dash-field-body\"><input size=\"5\" name=\"contrast\" type=\"number\" min=\"0\" max=\"200\" value=\"" + String(config.contrast) + "\" /><span class=\"dash-hint\">Only used when Dim Mode is CONTRAST.</span></div></div>\n";
		html += "</div>\n"; // .dash-panel

		// Grouped separately from the OLED/screen settings above - these all
		// govern the same thing (which received/sent packets pop up on the
		// OLED, and for how long), not the screen hardware itself.
		html += "<div class=\"dash-section-title\">Packet Notifications</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String txdispFlageEn = "";
		if (config.tx_display == true)
			txdispFlageEn = "checked";
		html += "<div class=\"dash-field\"><label>TX Display</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"txdispEnable\" value=\"OK\" " + txdispFlageEn + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">All TX Packet for display affter filter.</span></div></div>\n";

		String rxdispFlageEn = "";
		if (config.rx_display == true)
			rxdispFlageEn = "checked";
		html += "<div class=\"dash-field\"><label>RX Display</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"rxdispEnable\" value=\"OK\" " + rxdispFlageEn + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">All RX Packet for display affter filter.</span></div></div>\n";

		String rfFlageEn = "";
		if (config.dispRF == true)
			rfFlageEn = "checked";
		String inetFlageEn = "";
		if (config.dispINET == true)
			inetFlageEn = "checked";
		html += "<div class=\"dash-field\"><label>RX Channel</label><div class=\"dash-field-body\"><label><input type=\"checkbox\" name=\"dispRF\" value=\"OK\" " + rfFlageEn + "/> RF</label><label><input type=\"checkbox\" name=\"dispINET\" value=\"OK\" " + inetFlageEn + "/> Internet</label><span class=\"dash-hint\">Which source triggers the RX Display OLED popup - RF: packets heard over the air, Internet: packets received via APRS-IS.</span></div></div>\n";

		html += "<div class=\"dash-field\"><label>Filter DX</label><div class=\"dash-field-body\"><input type=\"number\" name=\"filterDX\" min=\"0\" max=\"9999\" step=\"1\" value=\"" + String(config.filterDistant) + "\" /> Km.<span class=\"dash-hint\">Value 0 is all distant allow.</span></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"dispDelay\">Popup Delay</label><div class=\"dash-field-body\"><select name=\"dispDelay\" id=\"dispDelay\">\n";
		for (int i = 0; i < 16; i += 1)
		{
			if (config.dispDelay == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(i) + " Sec</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(i) + " Sec</option>\n";
		}
		html += "</select></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-section-title\">Filter</div>\n";
		html += "<div class=\"dash-panel\">\n";
		html += "<fieldset id=\"filterDispGrp\" class=\"dash-filter-grp\">\n";
		html += "<legend>Show in popup display</legend>\n";
		html += "<div class=\"dash-checkbox-grid\">\n";

		String filterMessageFlageEn = "";
		if (config.dispFilter & FILTER_MESSAGE)
			filterMessageFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" id=\"filterMessage\" name=\"filterMessage\" type=\"checkbox\" value=\"OK\" " + filterMessageFlageEn + "/> Message</label>\n";

		String filterStatusFlageEn = "";
		if (config.dispFilter & FILTER_STATUS)
			filterStatusFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" id=\"filterStatus\" name=\"filterStatus\" type=\"checkbox\" value=\"OK\" " + filterStatusFlageEn + "/> Status</label>\n";

		String filterTelemetryFlageEn = "";
		if (config.dispFilter & FILTER_TELEMETRY)
			filterTelemetryFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" id=\"filterTelemetry\" name=\"filterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterTelemetryFlageEn + "/> Telemetry</label>\n";

		String filterWeatherFlageEn = "";
		if (config.dispFilter & FILTER_WX)
			filterWeatherFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" id=\"filterWeather\" name=\"filterWeather\" type=\"checkbox\" value=\"OK\" " + filterWeatherFlageEn + "/> Weather</label>\n";

		String filterObjectFlageEn = "";
		if (config.dispFilter & FILTER_OBJECT)
			filterObjectFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" id=\"filterObject\" name=\"filterObject\" type=\"checkbox\" value=\"OK\" " + filterObjectFlageEn + "/> Object</label>\n";

		String filterItemFlageEn = "";
		if (config.dispFilter & FILTER_ITEM)
			filterItemFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" id=\"filterItem\" name=\"filterItem\" type=\"checkbox\" value=\"OK\" " + filterItemFlageEn + "/> Item</label>\n";

		String filterQueryFlageEn = "";
		if (config.dispFilter & FILTER_QUERY)
			filterQueryFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" id=\"filterQuery\" name=\"filterQuery\" type=\"checkbox\" value=\"OK\" " + filterQueryFlageEn + "/> Query</label>\n";

		String filterBuoyFlageEn = "";
		if (config.dispFilter & FILTER_BUOY)
			filterBuoyFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" id=\"filterBuoy\" name=\"filterBuoy\" type=\"checkbox\" value=\"OK\" " + filterBuoyFlageEn + "/> Buoy</label>\n";

		String filterPositionFlageEn = "";
		if (config.dispFilter & FILTER_POSITION)
			filterPositionFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" id=\"filterPosition\" name=\"filterPosition\" type=\"checkbox\" value=\"OK\" " + filterPositionFlageEn + "/> Position</label>\n";

		html += "</div>\n";
		html += "</fieldset>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitDISP' name=\"commitDISP\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitDISP\"/>\n";
		html += "</form>\n";
		html += "</div>\n"; // .dash
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}


void handle_wireless()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	if (server.hasArg("commitWiFiAP"))
	{
		bool wifiAP = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "wifiAP")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						wifiAP = true;
					}
				}
			}

			if (server.argName(i) == "wifi_ssidAP")
			{
				if (isValidWifiSSID(server.arg(i).c_str(), server.arg(i).length()))
				{
					strcpy(config.wifi_ap_ssid, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "wifi_passAP")
			{
				if (isValidWifiPassword(server.arg(i).c_str(), server.arg(i).length()))
				{
					strcpy(config.wifi_ap_pass, server.arg(i).c_str());
				}
			}
		}
		if (wifiAP)
		{
			config.wifi_mode |= WIFI_AP_FIX;
		}
		else
		{
			config.wifi_mode &= ~WIFI_AP_FIX;
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitWiFiClient"))
	{
		bool wifiSTA = false;
		String nameSSID, namePASS;
		// Reset all 5 slots before repopulating from whatever fields the
		// submission actually contains - mirrors how "Remove Station" (JS)
		// works: it deletes that station's <tr> (and its inputs) from the DOM
		// rather than posting empty values, so a removed station's
		// wifi_ssid{n}/wifi_pass{n} fields are simply absent from this
		// request, not present-but-empty. Without clearing here too, only
		// `enable` got reset and the old ssid/pass stayed in EEPROM forever -
		// the station reappeared on the next page load because the "still
		// has a saved ssid" half of the display check kept firing.
		for (int n = 0; n < 5; n++)
		{
			config.wifi_sta[n].enable = false;
			config.wifi_sta[n].wifi_ssid[0] = 0;
			config.wifi_sta[n].wifi_pass[0] = 0;
		}
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "wificlient")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						wifiSTA = true;
					}
				}
			}

			for (int n = 0; n < 5; n++)
			{
				nameSSID = "wifiStation" + String(n);
				if (server.argName(i) == nameSSID)
				{
					if (server.arg(i) != "")
					{
						if (String(server.arg(i)) == "OK")
						{
							config.wifi_sta[n].enable = true;
						}
					}
				}
				nameSSID = "wifi_ssid" + String(n);
				if (server.argName(i) == nameSSID)
				{
					if (isValidWifiSSID(server.arg(i).c_str(), server.arg(i).length()))
					{
						strcpy(config.wifi_sta[n].wifi_ssid, server.arg(i).c_str());
					}
				}
				namePASS = "wifi_pass" + String(n);
				if (server.argName(i) == namePASS)
				{
					if (isValidWifiPassword(server.arg(i).c_str(), server.arg(i).length()))
					{
						strcpy(config.wifi_sta[n].wifi_pass, server.arg(i).c_str());
					}
				}
			}

			if (server.argName(i) == "wifi_pwr")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.wifi_power = server.arg(i).toInt();
				}
			}
		}
		if (wifiSTA)
		{
			config.wifi_mode |= WIFI_STA_FIX;
		}
		else
		{
			config.wifi_mode &= ~WIFI_STA_FIX;
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitBluetooth"))
	{
		bool btMaster = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "btMaster")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						btMaster = true;
					}
				}
			}

			if (server.argName(i) == "bt_name")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.bt_name, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "bt_uuid")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.bt_uuid, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "bt_uuid_rx")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.bt_uuid_rx, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "bt_uuid_tx")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.bt_uuid_tx, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "bt_mode")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.bt_mode = server.arg(i).toInt();
				}
			}
		}
		config.bt_master = btMaster;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{
		String html = "<script type=\"text/javascript\">\n";
		html += "document.querySelectorAll('form').forEach(function (form) {\n";
		html += "form.addEventListener('submit', function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formBluetooth\") document.getElementById(\"submitBluetooth\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formWiFiAP\") document.getElementById(\"submitWiFiAP\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formWiFiClient\") document.getElementById(\"submitWiFiClient\").disabled=true;\n";
		html += "fetch('/wireless', { method: 'POST', body: data })\n";
		html += ".then(function () { alert(\"Submited Successfully\"); })\n";
		html += ".catch(function () { alert(\"An error occurred.\"); });\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n";
		/************************ WiFi AP **************************/
		html += "<div class=\"dash\">\n";
		html += "<form id='formWiFiAP' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">WiFi Access Point</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String wifiAPEnFlag = "";
		if (config.wifi_mode & WIFI_AP_FIX)
			wifiAPEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>Enable</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"wifiAP\" value=\"OK\" " + wifiAPEnFlag + "><span class=\"slider round\"></span></label></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wifi_ssidAP\">WiFi AP SSID</label><div class=\"dash-field-body\"><input size=\"32\" maxlength=\"32\" class=\"form-control\" id=\"wifi_ssidAP\" name=\"wifi_ssidAP\" type=\"text\" value=\"" + String(config.wifi_ap_ssid) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wifi_passAP\">WiFi AP PASSWORD</label><div class=\"dash-field-body\"><input size=\"63\" maxlength=\"63\" class=\"form-control\" id=\"wifi_passAP\" name=\"wifi_passAP\" type=\"password\" value=\"" + String(config.wifi_ap_pass) + "\" /></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitWiFiAP' name=\"commit\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWiFiAP\"/>\n";
		html += "</form>\n";
		/************************ WiFi Client **************************/
		// NOTE: the station list below is intentionally left as a native
		// <table>/<tr>/<td> structure (not the .dash-field/.dash-panel
		// pattern) because addWifiStation() below (untouched, verbatim)
		// does document.createElement('tr') + appendChild() into this exact
		// table/tbody - switching the container to a <div> would break
		// newly-added stations rendering as real table rows. It still gets
		// the dashboard card look via the '.dash table' theming rule.
		html += "<form id='formWiFiClient' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">WiFi Multi Station</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String wifiClientEnFlag = "";
		if (config.wifi_mode & WIFI_STA_FIX)
			wifiClientEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>WiFi STA Enable</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"wificlient\" value=\"OK\" " + wifiClientEnFlag + "><span class=\"slider round\"></span></label></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"wifi_pwr\">WiFi RF Power</label><div class=\"dash-field-body\"><select name=\"wifi_pwr\" id=\"wifi_pwr\">\n";
		for (int i = 0; i < 12; i++)
		{
			if (config.wifi_power == wifiPwr[i][0])
				html += "<option value=\"" + String(wifiPwr[i][0], 0) + "\" selected>" + String(wifiPwr[i][1], 1) + " dBm</option>\n";
			else
				html += "<option value=\"" + String(wifiPwr[i][0], 0) + "\" >" + String(wifiPwr[i][1], 1) + " dBm</option>\n";
		}
		html += "</select></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-panel\">\n";
		html += "<table>\n";
		html += "<tbody id=\"wifiStationList\">\n";
		for (int n = 0; n < 5; n++)
		{
			if (config.wifi_sta[n].enable || config.wifi_sta[n].wifi_ssid[0] != '\0')
			{
				html += "<tr class=\"station-row\" data-station-index=\"" + String(n) + "\">\n";
				html += "<td align=\"right\" style=\"vertical-align: top;\"><b>Station #" + String(n + 1) + ":</b></td>\n";
				html += "<td align=\"center\">\n";
				html += "<fieldset id=\"filterDispGrp" + String(n + 1) + "\">\n";
				html += "<legend>WiFi Station #" + String(n + 1) + "</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
				html += "<tr style=\"background:unset;\">";
				html += "<td align=\"right\" width=\"120\"><b>Enable:</b></td>\n";
				String wifiClientEnFlag = "";
				if (config.wifi_sta[n].enable)
					wifiClientEnFlag = "checked";
				html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"wifiStation" + String(n) + "\" value=\"OK\" " + wifiClientEnFlag + "><span class=\"slider round\"></span></label></td>\n";
				html += "</tr>\n";
				html += "<tr>\n";
				html += "<td align=\"right\"><b>WiFi SSID:</b></td>\n";
				html += "<td style=\"text-align: left;\"><input size=\"24\" maxlength=\"32\" name=\"wifi_ssid" + String(n) + "\" type=\"text\" value=\"" + String(config.wifi_sta[n].wifi_ssid) + "\" /></td>\n";
				html += "</tr>\n";
				html += "<tr>\n";
				html += "<td align=\"right\"><b>WiFi PASSWORD:</b></td>\n";
				html += "<td style=\"text-align: left;\"><input size=\"24\" maxlength=\"63\" name=\"wifi_pass" + String(n) + "\" type=\"password\" value=\"" + String(config.wifi_sta[n].wifi_pass) + "\" /></td>\n";
				html += "</tr>\n";
				html += "<tr>\n";
				html += "<td colspan=\"2\" style=\"text-align: right;\"><button type=\"button\" style=\"background-color:red;color:white;font-size:11px;padding:3px 8px;border:none;border-radius:3px;cursor:pointer;\" onclick=\"this.closest('.station-row').remove(); updateAddButtonVisibility();\">Remove Station</button></td>\n";
				html += "</tr>\n";
				html += "</table></fieldset>\n";
				html += "</td>\n";
				html += "</tr>\n";
			}
		}
		html += "</tbody>\n";
		html += "<tr id=\"wifiAddRow\" style=\"display: none;\">\n";
		html += "<td colspan=\"2\" style=\"text-align: center; padding: 10px;\"><button type=\"button\" style=\"background-color:#2194ec;color:white;font-size:12px;padding:5px 12px;border:none;border-radius:3px;cursor:pointer;\" onclick=\"addWifiStation()\">+ Add Network</button></td>\n";
		html += "</tr>\n";

		html += "</table>\n";
		html += "</div>\n"; // .dash-panel
		html += "<script>\n";
		html += "function updateAddButtonVisibility() {\n";
		html += "  const rows = document.querySelectorAll('.station-row').length;\n";
		html += "  document.getElementById('wifiAddRow').style.display = (rows < 5) ? 'table-row' : 'none';\n";
		html += "}\n";
		html += "function addWifiStation() {\n";
		html += "  const list = document.getElementById('wifiStationList');\n";
		html += "  for (let n = 0; n < 5; n++) {\n";
		html += "    if (!document.querySelector('[data-station-index=\"' + n + '\"]')) {\n";
		html += "      const row = document.createElement('tr');\n";
		html += "      row.className = 'station-row';\n";
		html += "      row.setAttribute('data-station-index', n);\n";
		html += "      row.innerHTML = `\n";
		html += "        <td align=\"right\" style=\"vertical-align: top;\"><b>Station #${n + 1}:</b></td>\n";
		html += "        <td align=\"center\">\n";
		html += "          <fieldset id=\"filterDispGrp${n + 1}\">\n";
		html += "            <legend>WiFi Station #${n + 1}</legend>\n";
		html += "            <table style=\"text-align:unset;border-width:0px;background:unset\">\n";
		html += "              <tr style=\"background:unset;\">\n";
		html += "                <td align=\"right\" width=\"120\"><b>Enable:</b></td>\n";
		html += "                <td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"wifiStation${n}\" value=\"OK\"><span class=\"slider round\"></span></label></td>\n";
		html += "              </tr>\n";
		html += "              <tr>\n";
		html += "                <td align=\"right\"><b>WiFi SSID:</b></td>\n";
		html += "                <td style=\"text-align: left;\"><input size=\"24\" maxlength=\"32\" name=\"wifi_ssid${n}\" type=\"text\" /></td>\n";
		html += "              </tr>\n";
		html += "              <tr>\n";
		html += "                <td align=\"right\"><b>WiFi PASSWORD:</b></td>\n";
		html += "                <td style=\"text-align: left;\"><input size=\"24\" maxlength=\"63\" name=\"wifi_pass${n}\" type=\"password\" /></td>\n";
		html += "              </tr>\n";
		html += "              <tr>\n";
		html += "                <td colspan=\"2\" style=\"text-align: right;\"><button type=\"button\" style=\"background-color:red;color:white;font-size:11px;padding:3px 8px;border:none;border-radius:3px;cursor:pointer;\" onclick=\"this.closest('.station-row').remove(); updateAddButtonVisibility();\">Remove Station</button></td>\n";
		html += "              </tr>\n";
		html += "            </table>\n";
		html += "          </fieldset>\n";
		html += "        </td>\n";
		html += "      `;\n";
		html += "      list.appendChild(row);\n";
		html += "      break;\n";
		html += "    }\n";
		html += "  }\n";
		html += "  updateAddButtonVisibility();\n";
		html += "}\n";
		html += "updateAddButtonVisibility();\n";
		html += "</script>\n";
		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitWiFiClient' name=\"commit\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWiFiClient\"/>\n";
		html += "</form>\n";
		/************************ Bluetooth **************************/
		html += "<form id='formBluetooth' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">Bluetooth Master (BLE)</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String btEnFlag = "";
		if (config.bt_master)
			btEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>Enable</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"btMaster\" value=\"OK\" " + btEnFlag + "><span class=\"slider round\"></span></label></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"bt_name\">Name</label><div class=\"dash-field-body\"><input maxlength=\"20\" id=\"bt_name\" name=\"bt_name\" type=\"text\" value=\"" + String(config.bt_name) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"bt_uuid\">UUID</label><div class=\"dash-field-body\"><input maxlength=\"37\" size=\"38\" id=\"bt_uuid\" name=\"bt_uuid\" type=\"text\" value=\"" + String(config.bt_uuid) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"bt_uuid_rx\">UUID RX</label><div class=\"dash-field-body\"><input maxlength=\"37\" size=\"38\" id=\"bt_uuid_rx\" name=\"bt_uuid_rx\" type=\"text\" value=\"" + String(config.bt_uuid_rx) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"bt_uuid_tx\">UUID TX</label><div class=\"dash-field-body\"><input maxlength=\"37\" size=\"38\" id=\"bt_uuid_tx\" name=\"bt_uuid_tx\" type=\"text\" value=\"" + String(config.bt_uuid_tx) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"bt_mode\">Mode</label><div class=\"dash-field-body\"><select name=\"bt_mode\" id=\"bt_mode\">\n";
		String btModeOff = "";
		String btModeTNC2 = "";
		String btModeKISS = "";
		if (config.bt_mode == 1)
		{
			btModeTNC2 = "selected";
		}
		else if (config.bt_mode == 2)
		{
			btModeKISS = "selected";
		}
		else
		{
			btModeOff = "selected";
		}
		html += "<option value=\"0\" " + btModeOff + ">NONE</option>\n";
		html += "<option value=\"1\" " + btModeTNC2 + ">TNC2</option>\n";
		html += "<option value=\"2\" " + btModeKISS + ">KISS</option>\n";
		html += "</select><span class=\"dash-hint\">See the following for generating UUIDs: <a href=\"https://www.uuidgenerator.net\" target=\"_blank\">https://www.uuidgenerator.net</a></span></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitBluetooth' name=\"commit\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitBluetooth\"/>\n";
		html += "</form>\n";
		html += "</div>\n"; // .dash
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

