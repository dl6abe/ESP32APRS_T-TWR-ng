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

		// html += "<h2>System Setting</h2>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>System Setting</b></span></th>\n";
		html += "<tr>";
		// html += "<form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formTime\" method=\"post\">\n";
		html += "<td style=\"text-align: right;\">LOCAL<br/>DATE/TIME </td>\n";
		html += "<td style=\"text-align: left;\"><br /><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formTime\" method=\"post\">\n<input name=\"SetTime\" type=\"text\" value=\"" + String(strTime) + "\" />\n";
		html += "<span class=\"input-group-addon\">\n<span class=\"glyphicon glyphicon-calendar\">\n</span></span>\n";
		// html += "<div class=\"col-sm-3 col-xs-6\"><button class=\"btn btn-primary\" data-args=\"[true]\" data-method=\"getDate\" type=\"button\" data-related-target=\"#SetTime\" />Get Date</button></div>\n";
		html += "<button type='submit' id='updateTime'  name=\"commit\"> Time Update </button>\n";
		html += "<input type=\"hidden\" name=\"updateTime\"/></form>\n</td>\n";
		// html += "<input class=\"btn btn-primary\" id=\"updateTime\" name=\"updateTime\" type=\"submit\" value=\"Time Update\" maxlength=\"80\"/></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td style=\"text-align: right;\">NTP Host </td>\n";
		html += "<td style=\"text-align: left;\"><br /><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formNTP\" method=\"post\"><input name=\"SetTimeNtp\" type=\"text\" value=\"" + String(config.ntp_host) + "\" />\n";
		html += "<button type='submit' id='updateTimeNtp'  name=\"commit\"> NTP Update </button>\n";
		html += "<input type=\"hidden\" name=\"updateTimeNtp\"/></form>\n</td>\n";
		// html += "<input class=\"btn btn-primary\" id=\"updateTimeNtp\" name=\"updateTimeNtp\" type=\"submit\" value=\"NTP Update\" maxlength=\"80\"/></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td style=\"text-align: right;\">Time Zone </td>\n";
		html += "<td style=\"text-align: left;\"><br /><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formTimeZone\" method=\"post\">\n";
		html += "<input name=\"SetPosixTZ\" type=\"text\" size=\"30\" maxlength=\"" + String(sizeof(config.posixTZ) - 1) + "\" value=\"" + escapeHtml(String(config.posixTZ)) + "\" placeholder=\"e.g. CET-1CEST,M3.5.0,M10.5.0/3\" />\n";
		html += "<button type='submit' id='updateTimeZone'  name=\"commit\"> TZ Update </button><br />\n";
		html += "<label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>POSIX TZ string - handles DST automatically. Reference: <a href=\"https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv\" target=\"_blank\">timezone table</a></i></label>\n";
		html += "<input type=\"hidden\" name=\"updateTimeZone\"/></form>\n</td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td style=\"text-align: right;\">SYSTEM REBOOT </td>\n";
		html += "<td style=\"text-align: left;\"><br /><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formReboot\" method=\"post\"> <button type='submit' id='REBOOT'  name=\"commit\" style=\"background-color:red;color:white\"> REBOOT </button>\n";
		html += " <input type=\"hidden\" name=\"REBOOT\"/></form>\n</td>\n";
		// html += "<td style=\"text-align: left;\"><input type='submit' class=\"btn btn-danger\" id=\"REBOOT\" name=\"REBOOT\" value='REBOOT'></td>\n";
		html += "</tr></table><br /><br />\n";

		/************************ CONFIG BACKUP/RESTORE **************************/
		// Plain key=value text (config_fields.h/config_backup.cpp), not the raw
		// EEPROM bytes the OLED's Save/Load menu items use - this survives a
		// firmware update that changes sizeof(Configuration), see FORK_NOTES.md.
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>Config Backup / Restore</b></span></th>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\">Backup </td>\n";
		html += "<td style=\"text-align: left;\"><a href=\"/configBackup\" download=\"esp32aprs-config.cfg\"><button type='button'>Download Backup</button></a></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\">Restore </td>\n";
		html += "<td style=\"text-align: left;\"><form method='POST' action='#' enctype='multipart/form-data' id='formConfigRestore'>";
		html += "<input id=\"restoreFile\" name=\"restore\" type=\"file\" />";
		html += "<button type='submit' id='submitConfigRestore' style=\"background-color:red;color:white\">Restore &amp; Reboot</button>";
		html += "</form></td>\n";
		html += "</tr></table><br /><br />\n";
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
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>Web Authentication</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Web USER:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"32\" maxlength=\"32\" class=\"form-control\" name=\"webauth_user\" type=\"text\" value=\"" + String(config.http_username) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Web PASSWORD:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"63\" maxlength=\"63\" class=\"form-control\" name=\"webauth_pass\" type=\"password\" value=\"" + String(config.http_password) + "\" /></td>\n";
		html += "</tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitWebAuth'  name=\"commit\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWebAuth\"/>\n";
		html += "</form><br /><br />";

		/************************ DEBUG LOGGING **************************/
		// Runtime-toggleable console/syslog categories for this project's own
		// log_d()-equivalent calls migrated to projLog() (main.h/log_output.cpp)
		// - separate from CORE_DEBUG_LEVEL (platformio.ini), which is a
		// compile-time cap covering the whole firmware including framework
		// internals; see the LOGCAT_* comment in main.h for why these are two
		// different knobs, not one.
		html += "<form id='formDebugLog' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>Debug Logging</b></span></th>\n";
		html += "<tr><td align=\"right\"><b>Categories:</b></td><td style=\"text-align: left;\">\n";
		html += "<label><input type=\"checkbox\" name=\"logSystem\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_SYSTEM) ? " checked" : "") + " /> System</label><br />\n";
		html += "<label><input type=\"checkbox\" name=\"logWeb\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_WEB) ? " checked" : "") + " /> Web</label><br />\n";
		html += "<label><input type=\"checkbox\" name=\"logGps\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_GPS) ? " checked" : "") + " /> GPS</label><br />\n";
		html += "<label><input type=\"checkbox\" name=\"logAprsRf\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_APRS_RF) ? " checked" : "") + " /> APRS RF</label><br />\n";
		html += "<label><input type=\"checkbox\" name=\"logAprsInet\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_APRS_INET) ? " checked" : "") + " /> APRS Internet</label><br />\n";
		html += "<label><input type=\"checkbox\" name=\"logRfModule\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_RF_MODULE) ? " checked" : "") + " /> RF Module</label><br />\n";
		html += "<label><input type=\"checkbox\" name=\"logBluetooth\" value=\"OK\"" + String((config.logCategoryMask & LOGCAT_BLUETOOTH) ? " checked" : "") + " /> Bluetooth</label>\n";
		html += "</td></tr>\n";
		html += "<tr><td align=\"right\"><b>Syslog server:</b></td><td style=\"text-align: left;\">\n";
		html += "<label><input type=\"checkbox\" name=\"syslogEnable\" value=\"OK\"" + String(config.syslog_en ? " checked" : "") + " /> Enable</label><br />\n";
		html += "Host: <input size=\"30\" maxlength=\"39\" name=\"syslogHost\" type=\"text\" value=\"" + String(config.syslog_host) + "\" />\n";
		html += "Port: <input size=\"6\" name=\"syslogPort\" type=\"number\" min=\"1\" max=\"65535\" value=\"" + String(config.syslog_port) + "\" />\n";
		html += "</td></tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitDebugLog'  name=\"commit\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitDebugLog\"/>\n";
		html += "</form><br /><br />";

		/************************ PATH USER define **************************/
		html += "<form id='formPath' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>PATH USER Define</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH_1:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path1\" type=\"text\" value=\"" + String(config.path[0]) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH_2:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path2\" type=\"text\" value=\"" + String(config.path[1]) + "\" /></td>\n";
		html += "</tr>\n";
				html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH_3:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path3\" type=\"text\" value=\"" + String(config.path[2]) + "\" /></td>\n";
		html += "</tr>\n";
				html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH_4:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path4\" type=\"text\" value=\"" + String(config.path[3]) + "\" /></td>\n";
		html += "</tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitPath'  name=\"commitPath\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitPath\"/>\n";
		html += "</form><br /><br />";

		html += "<form id='formDisp' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>Display Setting</h2>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>Display Setting</b></span></th>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>OLED Enable</b></td>\n";
		String oledFlageEn = "";
		if (config.oled_enable == true)
			oledFlageEn = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"oledEnable\" value=\"OK\" " + oledFlageEn + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>TX Display</b></td>\n";
		String txdispFlageEn = "";
		if (config.tx_display == true)
			txdispFlageEn = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"txdispEnable\" value=\"OK\" " + txdispFlageEn + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*All TX Packet for display affter filter.</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>RX Display</b></td>\n";
		String rxdispFlageEn = "";
		if (config.rx_display == true)
			rxdispFlageEn = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"rxdispEnable\" value=\"OK\" " + rxdispFlageEn + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*All RX Packet for display affter filter.</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>Head Up</b></td>\n";
		String hupFlageEn = "";
		if (config.h_up == true)
			hupFlageEn = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"hupEnable\" value=\"OK\" " + hupFlageEn + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*The compass will rotate in the direction of movement.</i></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>Popup Delay</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"dispDelay\" id=\"dispDelay\">\n";
		for (int i = 0; i < 16; i += 1)
		{
			if (config.dispDelay == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(i) + " Sec</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(i) + " Sec</option>\n";
		}
		html += "</select>\n";
		html += "</td></tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>OLED Sleep</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"oled_timeout\" id=\"oled_timeout\">\n";
		for (int i = 0; i <= 600; i += 30)
		{
			String label = (i == 0) ? "Never" : (String(i) + " Sec");
			if (config.oled_timeout == i)
				html += "<option value=\"" + String(i) + "\" selected>" + label + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + label + "</option>\n";
		}
		html += "</select>\n";
		html += "</td></tr>\n";
		html += "<tr>\n";
		// Same 5 modes as the OLED-menu's DIM combobox (gui_menu_system.cpp) -
		// was device-menu-only before this, no web exposure.
		html += "<td style=\"text-align: right;\"><b>Dim Mode*</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"dimMode\" id=\"dimMode\">\n";
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
		html += "</select>\n";
		html += "<label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*HI: always full brightness.<br />LOW: always dimmed.<br />AUTO DIM: full brightness, dims after 60s idle.<br />DAY/NIGHT: full brightness 05:00-19:00, dimmed otherwise.<br />CONTRAST: fixed brightness set by the Contrast value below.</i></label>\n";
		html += "</td></tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>Contrast</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"5\" name=\"contrast\" type=\"number\" min=\"0\" max=\"200\" value=\"" + String(config.contrast) + "\" /><label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*Only used when Dim Mode is CONTRAST.</i></label></td>\n";
		html += "</tr>\n";
		String rfFlageEn = "";
		if (config.dispRF == true)
			rfFlageEn = "checked";
		String inetFlageEn = "";
		if (config.dispINET == true)
			inetFlageEn = "checked";
		html += "<tr><td style=\"text-align: right;\"><b>RX Channel</b></td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"dispRF\" value=\"OK\" " + rfFlageEn + "/>RF <input type=\"checkbox\" name=\"dispINET\" value=\"OK\" " + inetFlageEn + "/>Internet </td></tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Filter DX:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input type=\"number\" name=\"filterDX\" min=\"0\" max=\"9999\"\n";
		html += "step=\"1\" value=\"" + String(config.filterDistant) + "\" /> Km.  <label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*Value 0 is all distant allow.</i></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Filter:</b></td>\n";

		html += "<td align=\"center\">\n";
		html += "<fieldset id=\"filterDispGrp\">\n";
		html += "<legend>Show in popup display</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
		html += "<tr style=\"background:unset;\">";

		// html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"dispTNC\" name=\"dispTNC\" type=\"checkbox\" value=\"OK\" " + rfFlageEn + "/>From RF</td>\n";

		// html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"dispINET\" name=\"dispINET\" type=\"checkbox\" value=\"OK\" " + inetFlageEn + "/>From INET</td>\n";

		String filterMessageFlageEn = "";
		if (config.dispFilter & FILTER_MESSAGE)
			filterMessageFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterMessage\" name=\"filterMessage\" type=\"checkbox\" value=\"OK\" " + filterMessageFlageEn + "/>Message</td>\n";

		String filterStatusFlageEn = "";
		if (config.dispFilter & FILTER_STATUS)
			filterStatusFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterStatus\" name=\"filterStatus\" type=\"checkbox\" value=\"OK\" " + filterStatusFlageEn + "/>Status</td>\n";

		String filterTelemetryFlageEn = "";
		if (config.dispFilter & FILTER_TELEMETRY)
			filterTelemetryFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterTelemetry\" name=\"filterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterTelemetryFlageEn + "/>Telemetry</td>\n";

		String filterWeatherFlageEn = "";
		if (config.dispFilter & FILTER_WX)
			filterWeatherFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterWeather\" name=\"filterWeather\" type=\"checkbox\" value=\"OK\" " + filterWeatherFlageEn + "/>Weather</td>\n";

		String filterObjectFlageEn = "";
		if (config.dispFilter & FILTER_OBJECT)
			filterObjectFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterObject\" name=\"filterObject\" type=\"checkbox\" value=\"OK\" " + filterObjectFlageEn + "/>Object</td>\n";

		String filterItemFlageEn = "";
		if (config.dispFilter & FILTER_ITEM)
			filterItemFlageEn = "checked";
		html += "</tr><tr style=\"background:unset;\"><td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterItem\" name=\"filterItem\" type=\"checkbox\" value=\"OK\" " + filterItemFlageEn + "/>Item</td>\n";

		String filterQueryFlageEn = "";
		if (config.dispFilter & FILTER_QUERY)
			filterQueryFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterQuery\" name=\"filterQuery\" type=\"checkbox\" value=\"OK\" " + filterQueryFlageEn + "/>Query</td>\n";

		String filterBuoyFlageEn = "";
		if (config.dispFilter & FILTER_BUOY)
			filterBuoyFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterBuoy\" name=\"filterBuoy\" type=\"checkbox\" value=\"OK\" " + filterBuoyFlageEn + "/>Buoy</td>\n";

		String filterPositionFlageEn = "";
		if (config.dispFilter & FILTER_POSITION)
			filterPositionFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterPosition\" name=\"filterPosition\" type=\"checkbox\" value=\"OK\" " + filterPositionFlageEn + "/>Position</td>\n";

		html += "<td style=\"border:unset;\"></td>";
		html += "</tr></table></fieldset>\n";

		html += "</td></tr></table><br />\n";
		html += "<div><button type='submit' id='submitDISP'  name=\"commitDISP\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitDISP\"/>\n";
		html += "</form><br />";
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
		for (int n = 0; n < 5; n++)
			config.wifi_sta[n].enable = false;
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
		html += "<form id='formWiFiAP' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>WiFi Access Point</h2>\n";
		html += "<table>\n";
		// html += "<tr>\n";
		// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
		// html += "<th><span><b>Value</b></span></th>\n";
		// html += "</tr>\n";
		html += "<th colspan=\"2\"><span><b>WiFi Access Point</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\" width=\"120\"><b>Enable:</b></td>\n";
		String wifiAPEnFlag = "";
		if (config.wifi_mode & WIFI_AP_FIX)
			wifiAPEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"wifiAP\" value=\"OK\" " + wifiAPEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>WiFi AP SSID:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"32\" maxlength=\"32\" class=\"form-control\" id=\"wifi_ssidAP\" name=\"wifi_ssidAP\" type=\"text\" value=\"" + String(config.wifi_ap_ssid) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>WiFi AP PASSWORD:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"63\" maxlength=\"63\" class=\"form-control\" id=\"wifi_passAP\" name=\"wifi_passAP\" type=\"password\" value=\"" + String(config.wifi_ap_pass) + "\" /></td>\n";
		html += "</tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitWiFiAP'  name=\"commit\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWiFiAP\"/>\n";
		html += "</form><br />";
		/************************ WiFi Client **************************/
		html += "<br />\n";
		html += "<form id='formWiFiClient' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>WiFi Multi Station</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>WiFi STA Enable:</b></td>\n";
		String wifiClientEnFlag = "";
		if (config.wifi_mode & WIFI_STA_FIX)
			wifiClientEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"wificlient\" value=\"OK\" " + wifiClientEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>WiFi RF Power:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"wifi_pwr\" id=\"wifi_pwr\">\n";
		for (int i = 0; i < 12; i++)
		{
			if (config.wifi_power == wifiPwr[i][0])
				html += "<option value=\"" + String(wifiPwr[i][0], 0) + "\" selected>" + String(wifiPwr[i][1], 1) + " dBm</option>\n";
			else
				html += "<option value=\"" + String(wifiPwr[i][0], 0) + "\" >" + String(wifiPwr[i][1], 1) + " dBm</option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";
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

		html += "</table><br />\n";
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
		html += "<div><button type='submit' id='submitWiFiClient'  name=\"commit\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWiFiClient\"/>\n";
		html += "</form><br />";
		/************************ Bluetooth **************************/
		html += "<br />\n";
		html += "<form id='formBluetooth' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>Bluetooth Master (BLE)</h2>\n";
		html += "<table>\n";
		// html += "<tr>\n";
		// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
		// html += "<th><span><b>Value</b></span></th>\n";
		// html += "</tr>\n";
		html += "<th colspan=\"2\"><span><b>Bluetooth Master (BLE)</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Enable:</b></td>\n";
		String btEnFlag = "";
		if (config.bt_master)
			btEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"btMaster\" value=\"OK\" " + btEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>NAME:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"20\" id=\"bt_name\" name=\"bt_name\" type=\"text\" value=\"" + String(config.bt_name) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>UUID:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"37\" size=\"38\" id=\"bt_uuid\" name=\"bt_uuid\" type=\"text\" value=\"" + String(config.bt_uuid) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>UUID RX:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"37\" size=\"38\" id=\"bt_uuid_rx\" name=\"bt_uuid_rx\" type=\"text\" value=\"" + String(config.bt_uuid_rx) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>UUID TX:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"37\" size=\"38\" id=\"bt_uuid_tx\" name=\"bt_uuid_tx\" type=\"text\" value=\"" + String(config.bt_uuid_tx) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<td align=\"right\"><b>MODE:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"bt_mode\" id=\"bt_mode\">\n";
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
		html += "</select>\n";

		html += "<label style=\"font-size: 8pt;text-align: right;\">*See the following for generating UUIDs: <a href=\"https://www.uuidgenerator.net\" target=\"_blank\">https://www.uuidgenerator.net</a></label></td>\n";
		html += "</tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitBluetooth'  name=\"commit\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitBluetooth\"/>\n";
		html += "</form>";
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

