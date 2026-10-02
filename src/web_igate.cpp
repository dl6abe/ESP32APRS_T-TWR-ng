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


void handle_igate()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	bool aprsEn = false;
	bool rf2inetEn = false;
	bool inet2rfEn = false;
	bool posGPS = false;
	bool bcnEN = false;
	bool pos2RF = false;
	bool pos2INET = false;
	bool timeStamp = false;

	if (server.hasArg("commitIGATE"))
	{

		for (int i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "igateEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						aprsEn = true;
				}
			}
			if (server.argName(i) == "myCall")
			{
				if (server.arg(i) != "")
				{
					String name=server.arg(i);
					name.trim();
					strcpy(config.aprs_mycall, name.c_str());
				}
			}
			if (server.argName(i) == "igateObject")
			{
				if (server.arg(i) != "")
				{
					String name=server.arg(i);
					name.trim();
					strcpy(config.igate_object, name.c_str());
				}
				else
				{
					config.igate_object[0] = 0;
				}
			}
			if (server.argName(i) == "mySSID")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.aprs_ssid = server.arg(i).toInt();
					if (config.aprs_ssid > 15)
						config.aprs_ssid = 1; // matches this page's own compiled default (setConfigDefaults()), not an unrelated value
				}
			}
			if (server.argName(i) == "igatePosInv")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_interval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "igatePosLat")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_lat = server.arg(i).toFloat();
				}
			}

			if (server.argName(i) == "igatePosLon")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_lon = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "igatePosAlt")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_alt = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "igatePosSel")
			{
				if (server.arg(i) != "")
				{
					if (server.arg(i).toInt() == 1)
						posGPS = true;
				}
			}

			if (server.argName(i) == "igateTable")
			{
				if (server.arg(i) != "")
				{
					config.igate_symbol[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "igateSymbol")
			{
				if (server.arg(i) != "")
				{
					config.igate_symbol[1] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "aprsHost")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.aprs_host, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "aprsPort")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.aprs_port = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "aprsFilter")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.aprs_filter, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "igatePath")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_path = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "igateComment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.igate_comment, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "texttouse")
			{
				if (server.arg(i) != "")
				{
					// No client-side maxlength existed on this field at all
					// (unlike most other text inputs on this page) - a raw
					// POST with an oversized value overflowed igate_phg[8]
					// directly. Clamp per CLAUDE.md's buffer-safety rule.
					strncpy(config.igate_phg, server.arg(i).c_str(), sizeof(config.igate_phg) - 1);
					config.igate_phg[sizeof(config.igate_phg) - 1] = 0;
				}
			}
			if (server.argName(i) == "aprsComment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.igate_comment, server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "rf2inetEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						rf2inetEn = true;
				}
			}
			if (server.argName(i) == "inet2rfEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						inet2rfEn = true;
				}
			}
			if (server.argName(i) == "igatePos2RF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2RF = true;
				}
			}
			if (server.argName(i) == "igatePos2INET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2INET = true;
				}
			}
			if (server.argName(i) == "igateBcnEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						bcnEN = true;
				}
			}
			if (server.argName(i) == "igateTimeStamp")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						timeStamp = true;
				}
			}
		}

		config.igate_en = aprsEn;
		config.rf2inet = rf2inetEn;
		config.inet2rf = inet2rfEn;
		config.igate_gps = posGPS;
		config.igate_bcn = bcnEN;
		config.igate_loc2rf = pos2RF;
		config.igate_loc2inet = pos2INET;
		config.igate_timestamp = timeStamp;

		saveEEPROM();
		initInterval=true;
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitIGATEfilter"))
	{
		config.rf2inetFilter = 0;
		config.inet2rfFilter = 0;
		for (int i = 0; i < server.args(); i++)
		{
			// config rf2inet filter
			if (server.argName(i) == "rf2inetFilterMessage")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_MESSAGE;
				}
			}

			if (server.argName(i) == "rf2inetFilterTelemetry")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_TELEMETRY;
				}
			}

			if (server.argName(i) == "rf2inetFilterStatus")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_STATUS;
				}
			}

			if (server.argName(i) == "rf2inetFilterWeather")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_WX;
				}
			}

			if (server.argName(i) == "rf2inetFilterObject")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_OBJECT;
				}
			}

			if (server.argName(i) == "rf2inetFilterItem")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_ITEM;
				}
			}

			if (server.argName(i) == "rf2inetFilterQuery")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_QUERY;
				}
			}
			if (server.argName(i) == "rf2inetFilterBuoy")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_BUOY;
				}
			}
			if (server.argName(i) == "rf2inetFilterPosition")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_POSITION;
				}
			}
			// config inet2rf filter

			if (server.argName(i) == "inet2rfFilterMessage")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_MESSAGE;
				}
			}

			if (server.argName(i) == "inet2rfFilterTelemetry")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_TELEMETRY;
				}
			}

			if (server.argName(i) == "inet2rfFilterStatus")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_STATUS;
				}
			}

			if (server.argName(i) == "inet2rfFilterWeather")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_WX;
				}
			}

			if (server.argName(i) == "inet2rfFilterObject")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_OBJECT;
				}
			}

			if (server.argName(i) == "inet2rfFilterItem")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_ITEM;
				}
			}

			if (server.argName(i) == "inet2rfFilterQuery")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_QUERY;
				}
			}
			if (server.argName(i) == "inet2rfFilterBuoy")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_BUOY;
				}
			}
			if (server.argName(i) == "inet2rfFilterPosition")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_POSITION;
				}
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}else{
		String html = "<script type=\"text/javascript\">\n";
		html += "document.querySelectorAll('form').forEach(function (form) {\n";
		html += "form.addEventListener('submit', function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formIgate\") document.getElementById(\"submitIGATE\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formIgateFilter\") document.getElementById(\"submitIGATEfilter\").disabled=true;\n";
		html += "fetch('/igate', { method: 'POST', body: data })\n";
		html += ".then(function () { alert(\"Submited Successfully\"); })\n";
		html += ".catch(function () { alert(\"An error occurred.\"); });\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n<script type=\"text/javascript\">\n";
		html += "function openWindowSymbol() {\n";
		html += "var i, l, options = [{\n";
		html += "value: 'first',\n";
		html += "text: 'First'\n";
		html += "}, {\n";
		html += "value: 'second',\n";
		html += "text: 'Second'\n";
		html += "}],\n";
		html += "newWindow = window.open(\"\", null, \"height=400,width=400,status=no,toolbar=no,menubar=no,location=no\");\n";

		int i;

		html += "newWindow.document.write(\"<table border=\\\"1\\\" align=\\\"center\\\">\");\n";
		html += "newWindow.document.write(\"<tr><th colspan=\\\"16\\\">Table '/'</th></tr><tr>\");\n";
		for (i = 33; i < 129; i++)
		{
			html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(" + String(i) + ",1);\\\" src=\\\"/icon.png?c=" + String(i) + "&t=1\\\"></td>\");\n";
			if (((i % 16) == 0) && (i < 126))
				html += "newWindow.document.write(\"</tr><tr>\");\n";
		}
		html += "newWindow.document.write(\"</tr></table><br />\");\n";
		html += "newWindow.document.write(\"<table border=\\\"1\\\" align=\\\"center\\\">\");\n";
		html += "newWindow.document.write(\"<tr><th colspan=\\\"16\\\">Table '\\\'</th></tr><tr>\");\n";
		for (i = 33; i < 129; i++)
		{
			html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(" + String(i) + ",2);\\\" src=\\\"/icon.png?c=" + String(i, DEC) + "&t=2\\\"></td>\");\n";
			if (((i % 16) == 0) && (i < 126))
				html += "newWindow.document.write(\"</tr><tr>\");\n";
		}
		html += "newWindow.document.write(\"</tr></table>\");\n";

		// html += "newWindow.document.write(\"</select>\");\");\n";
		html += "}\n";

		html += "function setValue(symbol,table) {\n";
		html += "document.getElementById('igateSymbol').value = String.fromCharCode(symbol);\n";
		html += "if(table==1){\n document.getElementById('igateTable').value='/';\n";
		html += "}else if(table==2){\n document.getElementById('igateTable').value='\\\\';\n}\n";
		html += "document.getElementById('igateImgSymbol').src = \"/icon.png?c=\"+symbol.toString()+\"&t=\"+table.toString();\n";
		html += "\n}\n";
		html += "function calculatePHGR(){document.forms.formIgate.texttouse.value=\"PHG\"+calcPower(document.forms.formIgate.power.value)+calcHeight(document.forms.formIgate.haat.value)+calcGain(document.forms.formIgate.gain.value)+calcDirection(document.forms.formIgate.direction.selectedIndex)}function Log2(e){return Math.log(e)/Math.log(2)}function calcPerHour(e){return e<10?e:String.fromCharCode(65+(e-10))}function calcHeight(e){return String.fromCharCode(48+Math.round(Log2(e/10),0))}function calcPower(e){if(e<1)return 0;if(e>=1&&e<4)return 1;if(e>=4&&e<9)return 2;if(e>=9&&e<16)return 3;if(e>=16&&e<25)return 4;if(e>=25&&e<36)return 5;if(e>=36&&e<49)return 6;if(e>=49&&e<64)return 7;if(e>=64&&e<81)return 8;if(e>=81)return 9}function calcDirection(e){if(e==\"0\")return\"0\";if(e==\"1\")return\"1\";if(e==\"2\")return\"2\";if(e==\"3\")return\"3\";if(e==\"4\")return\"4\";if(e==\"5\")return\"5\";if(e==\"6\")return\"6\";if(e==\"7\")return\"7\";if(e==\"8\")return\"8\"}function calcGain(e){return e>9?\"9\":e<0?\"0\":Math.round(e,0)}\n";
		html += "function onRF2INETCheck() {\n";
		html += "if (document.querySelector('#rf2inetEnable').checked) {\n";
		// Checkbox has been checked
		html += "document.getElementById(\"rf2inetFilterGrp\").disabled=false;\n";
		html += "} else {\n";
		// Checkbox has been unchecked
		html += "document.getElementById(\"rf2inetFilterGrp\").disabled=true;\n";
		html += "}\n}\n";
		html += "function onINET2RFCheck() {\n";
		html += "if (document.querySelector('#inet2rfEnable').checked) {\n";
		// Checkbox has been checked
		html += "document.getElementById(\"inet2rfFilterGrp\").disabled=false;\n";
		html += "} else {\n";
		// Checkbox has been unchecked
		html += "document.getElementById(\"inet2rfFilterGrp\").disabled=true;\n";
		html += "}\n}\n";
		html += "function onIgatePosSelChange() {\n";
		html += "var isGPS = document.querySelector('input[name=\"igatePosSel\"][value=\"1\"]').checked;\n";
		html += "document.getElementById(\"igateFixPosGrp\").style.display = isGPS ? \"none\" : \"\";\n";
		html += "}\n";
		html += "</script>\n";

		/************************ IGATE Mode **************************/
		html += "<div class=\"dash\">\n";
		html += "<form id='formIgate' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">[IGATE] Internet Gateway Mode</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String igateEnFlag = "";
		if (config.igate_en)
			igateEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>Enable</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"igateEnable\" value=\"OK\" " + igateEnFlag + "><span class=\"slider round\"></span></label></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"myCall\">Station Callsign</label><div class=\"dash-field-body\"><input maxlength=\"7\" size=\"9\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.aprs_mycall) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"mySSID\">Station SSID</label><div class=\"dash-field-body\"><select name=\"mySSID\" id=\"mySSID\">\n";
		for (uint8_t ssid = 0; ssid <= 15; ssid++)
		{
			if (config.aprs_ssid == ssid)
			{
				html += "<option value=\"" + String(ssid) + "\" selected>" + String(ssid) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(ssid) + "\">" + String(ssid) + "</option>\n";
			}
		}
		html += "</select></div></div>\n";

		String table = "1";
		if (config.igate_symbol[0] == 47)
			table = "1";
		if (config.igate_symbol[0] == 92)
			table = "2";
		html += "<div class=\"dash-field\"><label>Station Symbol</label><div class=\"dash-field-body\">Table:<input maxlength=\"1\" size=\"1\" id=\"igateTable\" name=\"igateTable\" type=\"text\" value=\"" + String(config.igate_symbol[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"igateSymbol\" name=\"igateSymbol\" type=\"text\" value=\"" + String(config.igate_symbol[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"igateImgSymbol\" onclick=\"openWindowSymbol();\" src=\"/icon.png?c=" + String((int)config.igate_symbol[1]) + "&t=" + table + "\"> <span class=\"dash-hint\">Click icon to select a symbol</span></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"igateObject\">Item/Obj Name</label><div class=\"dash-field-body\"><input maxlength=\"9\" size=\"9\" id=\"igateObject\" name=\"igateObject\" type=\"text\" value=\"" + String(config.igate_object) + "\" /><span class=\"dash-hint\">Leave blank if unused (3-9 characters)</span></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"igatePath\">Path</label><div class=\"dash-field-body\"><select name=\"igatePath\" id=\"igatePath\">\n";
		for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
		{
			String pthLabel = String(PATH_NAME[pthIdx]);
			if (pthIdx == PATH_DEFAULT_FIXED)
				pthLabel += " (default)";
			if (config.igate_path == pthIdx)
			{
				html += "<option value=\"" + String(pthIdx) + "\" selected>" + pthLabel + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(pthIdx) + "\">" + pthLabel + "</option>\n";
			}
		}
		html += "</select></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"aprsHost\">Server Host</label><div class=\"dash-field-body\"><input maxlength=\"20\" size=\"20\" id=\"aprsHost\" name=\"aprsHost\" type=\"text\" value=\"" + String(config.aprs_host) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"aprsPort\">Server Port</label><div class=\"dash-field-body\"><input min=\"1\" max=\"65535\" step=\"1\" id=\"aprsPort\" name=\"aprsPort\" type=\"number\" value=\"" + String(config.aprs_port) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"aprsFilter\">Server Filter</label><div class=\"dash-field-body\"><input maxlength=\"30\" size=\"30\" id=\"aprsFilter\" name=\"aprsFilter\" type=\"text\" value=\"" + String(config.aprs_filter) + "\" /><span class=\"dash-hint\">Filter syntax: <a target=\"_blank\" href=\"http://www.aprs-is.net/javAPRSFilter.aspx\">aprs-is.net/javAPRSFilter.aspx</a></span></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"igateComment\">Text Comment</label><div class=\"dash-field-body\"><input maxlength=\"50\" size=\"50\" id=\"igateComment\" name=\"igateComment\" type=\"text\" value=\"" + String(config.igate_comment) + "\" /></div></div>\n";

		String rf2inetEnFlag = "";
		if (config.rf2inet)
			rf2inetEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>RF2INET</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" id=\"rf2inetEnable\" name=\"rf2inetEnable\" onclick=\"onRF2INETCheck()\" value=\"OK\" " + rf2inetEnFlag + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">Switch RF to Internet gateway</span></div></div>\n";

		String inet2rfEnFlag = "";
		if (config.inet2rf)
			inet2rfEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>INET2RF</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" id=\"inet2rfEnable\" name=\"inet2rfEnable\" onclick=\"onINET2RFCheck()\" value=\"OK\" " + inet2rfEnFlag + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">Switch Internet to RF gateway</span></div></div>\n";

		String timeStampFlag = "";
		if (config.igate_timestamp)
			timeStampFlag = "checked";
		html += "<div class=\"dash-field\"><label>Time Stamp</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"igateTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-section-title\">Position</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String igateBcnEnFlag = "";
		if (config.igate_bcn)
			igateBcnEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>Beacon</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"igateBcnEnable\" value=\"OK\" " + igateBcnEnFlag + "><span class=\"slider round\"></span></label><label>Interval <input min=\"0\" max=\"3600\" step=\"1\" id=\"igatePosInv\" name=\"igatePosInv\" type=\"number\" value=\"" + String(config.igate_interval) + "\" /> sec</label></div></div>\n";

		String igatePosFixFlag = "";
		String igatePosGPSFlag = "";
		String igatePos2RFFlag = "";
		String igatePos2INETFlag = "";
		if (config.igate_gps)
			igatePosGPSFlag = "checked=\"checked\"";
		else
			igatePosFixFlag = "checked=\"checked\"";

		if (config.igate_loc2rf)
			igatePos2RFFlag = "checked";
		if (config.igate_loc2inet)
			igatePos2INETFlag = "checked";
		html += "<div class=\"dash-field\"><label>Location Source</label><div class=\"dash-field-body\"><label><input type=\"radio\" name=\"igatePosSel\" value=\"0\" onchange=\"onIgatePosSelChange()\" " + igatePosFixFlag + "/> Fix</label><label><input type=\"radio\" name=\"igatePosSel\" value=\"1\" onchange=\"onIgatePosSelChange()\" " + igatePosGPSFlag + "/> GPS</label></div></div>\n";
		html += "<div class=\"dash-field\"><label>TX Channel</label><div class=\"dash-field-body\"><label><input type=\"checkbox\" name=\"igatePos2RF\" value=\"OK\" " + igatePos2RFFlag + "/> RF</label><label><input type=\"checkbox\" name=\"igatePos2INET\" value=\"OK\" " + igatePos2INETFlag + "/> Internet</label></div></div>\n";
		// Lat/Lon/Alt only make sense for a Fix position - hidden while GPS is
		// selected (issue - Location Source Fix/GPS field visibility).
		html += "<div id=\"igateFixPosGrp\"" + String(config.igate_gps ? " style=\"display:none\"" : "") + ">\n";
		html += "<div class=\"dash-field\"><label for=\"igatePosLat\">Latitude</label><div class=\"dash-field-body\"><input min=\"-90\" max=\"90\" step=\"0.00001\" id=\"igatePosLat\" name=\"igatePosLat\" type=\"number\" value=\"" + String(config.igate_lat, 5) + "\" /><span class=\"dash-hint\">degrees (positive for North, negative for South)</span></div></div>\n";
		html += "<div class=\"dash-field\"><label for=\"igatePosLon\">Longitude</label><div class=\"dash-field-body\"><input min=\"-180\" max=\"180\" step=\"0.00001\" id=\"igatePosLon\" name=\"igatePosLon\" type=\"number\" value=\"" + String(config.igate_lon, 5) + "\" /><span class=\"dash-hint\">degrees (positive for East, negative for West)</span></div></div>\n";
		html += "<div class=\"dash-field\"><label for=\"igatePosAlt\">Altitude</label><div class=\"dash-field-body\"><input min=\"0\" max=\"10000\" step=\"0.1\" id=\"igatePosAlt\" name=\"igatePosAlt\" type=\"number\" value=\"" + String(config.igate_alt, 2) + "\" /><span class=\"dash-hint\">meters - value 0 is not sent</span></div></div>\n";
		html += "</div>\n"; // #igateFixPosGrp
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-section-title\">PHG (Power-Height-Gain)</div>\n";
		html += "<div class=\"dash-panel\">\n";

		html += "<div class=\"dash-field\"><label for=\"power\">Radio TX Power</label><div class=\"dash-field-body\"><select name=\"power\" id=\"power\">\n";
		html += "<option value=\"1\" selected>1</option>\n";
		html += "<option value=\"5\">5</option>\n";
		html += "<option value=\"10\">10</option>\n";
		html += "<option value=\"15\">15</option>\n";
		html += "<option value=\"25\">25</option>\n";
		html += "<option value=\"35\">35</option>\n";
		html += "<option value=\"50\">50</option>\n";
		html += "<option value=\"65\">65</option>\n";
		html += "<option value=\"80\">80</option>\n";
		html += "</select> Watts</div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"gain\">Antenna Gain</label><div class=\"dash-field-body\"><input size=\"3\" min=\"0\" max=\"100\" step=\"0.1\" id=\"gain\" name=\"gain\" type=\"number\" value=\"6\" /> dBi</div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"haat\">Height</label><div class=\"dash-field-body\"><select name=\"haat\" id=\"haat\">\n";
		int k = 10;
		for (uint8_t w = 0; w < 10; w++)
		{
			if (w == 0)
			{
				html += "<option value=\"" + String(k) + "\" selected>" + String(k) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(k) + "\">" + String(k) + "</option>\n";
			}
			k += k;
		}
		html += "</select> Feet</div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"direction\">Antenna Direction</label><div class=\"dash-field-body\"><select name=\"direction\" id=\"direction\">\n";
		html += "<option>Omni</option><option>NE</option><option>E</option><option>SE</option><option>S</option><option>SW</option><option>W</option><option>NW</option><option>N</option>\n";
		html += "</select></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"texttouse\">PHG Text</label><div class=\"dash-field-body\"><input name=\"texttouse\" type=\"text\" size=\"9\" maxlength=\"" + String(sizeof(config.igate_phg) - 1) + "\" style=\"background-color: rgb(97, 239, 170);\" value=\"" + String(config.igate_phg) + "\"/> <button type=\"button\" onclick=\"javascript:calculatePHGR()\">Calculate PHG</button></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitIGATE' name=\"commitIGATE\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitIGATE\"/>\n";
		html += "</form>\n";

		html += "<form id='formIgateFilter' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">[IGATE] Filter</div>\n";

		html += "<div class=\"dash-panel\">\n";
		if (config.rf2inet)
			html += "<fieldset id=\"rf2inetFilterGrp\" class=\"dash-filter-grp\">\n";
		else
			html += "<fieldset id=\"rf2inetFilterGrp\" class=\"dash-filter-grp\" disabled>\n";
		html += "<legend>RF2INET Filter - Pass RF to Internet</legend>\n";
		html += "<div class=\"dash-checkbox-grid\">\n";

		String filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_MESSAGE)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"rf2inetFilterMessage\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Message</label>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_STATUS)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"rf2inetFilterStatus\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Status</label>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_TELEMETRY)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"rf2inetFilterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Telemetry</label>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_WX)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"rf2inetFilterWeather\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Weather</label>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_OBJECT)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"rf2inetFilterObject\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Object</label>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_ITEM)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"rf2inetFilterItem\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Item</label>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_QUERY)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"rf2inetFilterQuery\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Query</label>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_BUOY)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"rf2inetFilterBuoy\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Buoy</label>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_POSITION)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"rf2inetFilterPosition\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Position</label>\n";

		html += "</div>\n";
		html += "</fieldset>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-panel\">\n";
		if (config.inet2rf)
			html += "<fieldset id=\"inet2rfFilterGrp\" class=\"dash-filter-grp\">\n";
		else
			html += "<fieldset id=\"inet2rfFilterGrp\" class=\"dash-filter-grp\" disabled>\n";
		html += "<legend>INET2RF Filter - Pass Internet to RF</legend>\n";
		html += "<div class=\"dash-checkbox-grid\">\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_MESSAGE)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"inet2rfFilterMessage\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Message</label>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_STATUS)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"inet2rfFilterStatus\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Status</label>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_TELEMETRY)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"inet2rfFilterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Telemetry</label>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_WX)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"inet2rfFilterWeather\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Weather</label>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_OBJECT)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"inet2rfFilterObject\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Object</label>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_ITEM)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"inet2rfFilterItem\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Item</label>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_QUERY)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"inet2rfFilterQuery\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Query</label>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_BUOY)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"inet2rfFilterBuoy\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Buoy</label>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_POSITION)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"inet2rfFilterPosition\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Position</label>\n";

		html += "</div>\n";
		html += "</fieldset>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitIGATEfilter' name=\"commitIGATEfilter\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitIGATEfilter\"/>\n";
		html += "</form>\n";
		html += "</div>\n"; // .dash
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

