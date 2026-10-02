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


void handle_digi()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	bool digiEn = false;
	bool posGPS = false;
	bool bcnEN = false;
	bool pos2RF = false;
	bool pos2INET = false;
	bool timeStamp = false;

	if (server.hasArg("commitDIGI"))
	{
		config.digiFilter = 0;
		for (int i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "digiEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						digiEn = true;
				}
			}
			if (server.argName(i) == "myCall")
			{
				if (server.arg(i) != "")
				{
					String name=server.arg(i);
					name.trim();
					strcpy(config.digi_mycall, name.c_str());
				}
			}
			if (server.argName(i) == "mySSID")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_ssid = server.arg(i).toInt();
					if (config.digi_ssid > 15)
						config.digi_ssid = 3;
				}
			}
			if (server.argName(i) == "digiDelay")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_delay = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "digiPosInv")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_interval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "digiPosLat")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_lat = server.arg(i).toFloat();
				}
			}

			if (server.argName(i) == "digiPosLon")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_lon = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "digiPosAlt")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_alt = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "digiPosSel")
			{
				if (server.arg(i) != "")
				{
					if (server.arg(i).toInt() == 1)
						posGPS = true;
				}
			}

			if (server.argName(i) == "digiTable")
			{
				if (server.arg(i) != "")
				{
					config.digi_symbol[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "digiSymbol")
			{
				if (server.arg(i) != "")
				{
					config.digi_symbol[1] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "digiPath")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_path = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "digiComment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.digi_comment, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "texttouse")
			{
				if (server.arg(i) != "")
				{
					// No client-side maxlength existed on this field at all
					// (unlike most other text inputs on this page) - a raw
					// POST with an oversized value overflowed digi_phg[8]
					// directly. Clamp per CLAUDE.md's buffer-safety rule.
					strncpy(config.digi_phg, server.arg(i).c_str(), sizeof(config.digi_phg) - 1);
					config.digi_phg[sizeof(config.digi_phg) - 1] = 0;
				}
			}
			if (server.argName(i) == "digiComment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.digi_comment, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "digiPos2RF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2RF = true;
				}
			}
			if (server.argName(i) == "digiPos2INET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2INET = true;
				}
			}
			if (server.argName(i) == "digiBcnEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						bcnEN = true;
				}
			}
			// Filter
			if (server.argName(i) == "FilterMessage")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_MESSAGE;
				}
			}

			if (server.argName(i) == "FilterTelemetry")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_TELEMETRY;
				}
			}

			if (server.argName(i) == "FilterStatus")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_STATUS;
				}
			}

			if (server.argName(i) == "FilterWeather")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_WX;
				}
			}

			if (server.argName(i) == "FilterObject")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_OBJECT;
				}
			}

			if (server.argName(i) == "FilterItem")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_ITEM;
				}
			}

			if (server.argName(i) == "FilterQuery")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_QUERY;
				}
			}
			if (server.argName(i) == "FilterBuoy")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_BUOY;
				}
			}
			if (server.argName(i) == "FilterPosition")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_POSITION;
				}
			}
			if (server.argName(i) == "digiTimeStamp")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						timeStamp = true;
				}
			}
		}
		config.digi_en = digiEn;
		config.digi_gps = posGPS;
		config.digi_bcn = bcnEN;
		config.digi_loc2rf = pos2RF;
		config.digi_loc2inet = pos2INET;
		config.digi_timestamp = timeStamp;

		saveEEPROM();
		initInterval=true;
		String html = "OK";
		server.send(200, "text/html", html);
	}else{
		String html = "<script type=\"text/javascript\">\n";
		html += "document.querySelectorAll('form').forEach(function (form) {\n";
		html += "form.addEventListener('submit', function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "document.getElementById(\"submitDIGI\").disabled=true;\n";
		html += "fetch('/digi', { method: 'POST', body: data })\n";
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
		html += "newWindow = window.open(\"\", null, \"height=400,width=400,status=no,toolbar=no,menubar=no,titlebar=no,location=no\");\n";

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
		html += "}\n";

		html += "function setValue(symbol,table) {\n";
		html += "document.getElementById('digiSymbol').value = String.fromCharCode(symbol);\n";
		html += "if(table==1){\n document.getElementById('digiTable').value='/';\n";
		html += "}else if(table==2){\n document.getElementById('digiTable').value='\\\\';\n}\n";
		html += "document.getElementById('digiImgSymbol').src = \"/icon.png?c=\"+symbol.toString()+\"&t=\"+table.toString();\n";
		html += "\n}\n";
		html += "function calculatePHGR(){document.forms.formDIGI.texttouse.value=\"PHG\"+calcPower(document.forms.formDIGI.power.value)+calcHeight(document.forms.formDIGI.haat.value)+calcGain(document.forms.formDIGI.gain.value)+calcDirection(document.forms.formDIGI.direction.selectedIndex)}function Log2(e){return Math.log(e)/Math.log(2)}function calcPerHour(e){return e<10?e:String.fromCharCode(65+(e-10))}function calcHeight(e){return String.fromCharCode(48+Math.round(Log2(e/10),0))}function calcPower(e){if(e<1)return 0;if(e>=1&&e<4)return 1;if(e>=4&&e<9)return 2;if(e>=9&&e<16)return 3;if(e>=16&&e<25)return 4;if(e>=25&&e<36)return 5;if(e>=36&&e<49)return 6;if(e>=49&&e<64)return 7;if(e>=64&&e<81)return 8;if(e>=81)return 9}function calcDirection(e){if(e==\"0\")return\"0\";if(e==\"1\")return\"1\";if(e==\"2\")return\"2\";if(e==\"3\")return\"3\";if(e==\"4\")return\"4\";if(e==\"5\")return\"5\";if(e==\"6\")return\"6\";if(e==\"7\")return\"7\";if(e==\"8\")return\"8\"}function calcGain(e){return e>9?\"9\":e<0?\"0\":Math.round(e,0)}\n";
		html += "function onDigiPosSelChange() {\n";
		html += "var isGPS = document.querySelector('input[name=\"digiPosSel\"][value=\"1\"]').checked;\n";
		html += "document.getElementById(\"digiFixPosGrp\").style.display = isGPS ? \"none\" : \"\";\n";
		html += "}\n";
		html += "</script>\n";

		/************************ DIGI Mode **************************/
		html += "<div class=\"dash\">\n";
		html += "<form id='formDIGI' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<div class=\"dash-section-title\">[DIGI] Digital Repeater Mode</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String digiEnFlag = "";
		if (config.digi_en)
			digiEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>Enable</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"digiEnable\" value=\"OK\" " + digiEnFlag + "><span class=\"slider round\"></span></label></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"myCall\">Station Callsign</label><div class=\"dash-field-body\"><input maxlength=\"7\" size=\"9\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.digi_mycall) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"mySSID\">Station SSID</label><div class=\"dash-field-body\"><select name=\"mySSID\" id=\"mySSID\">\n";
		for (uint8_t ssid = 0; ssid <= 15; ssid++)
		{
			if (config.digi_ssid == ssid)
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
		if (config.digi_symbol[0] == 47)
			table = "1";
		if (config.digi_symbol[0] == 92)
			table = "2";
		html += "<div class=\"dash-field\"><label>Station Symbol</label><div class=\"dash-field-body\">Table:<input maxlength=\"1\" size=\"1\" id=\"digiTable\" name=\"digiTable\" type=\"text\" value=\"" + String(config.digi_symbol[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"digiSymbol\" name=\"digiSymbol\" type=\"text\" value=\"" + String(config.digi_symbol[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"digiImgSymbol\" onclick=\"openWindowSymbol();\" src=\"/icon.png?c=" + String((int)config.digi_symbol[1]) + "&t=" + table + "\"> <span class=\"dash-hint\">Click icon to select a symbol</span></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"digiPath\">Path</label><div class=\"dash-field-body\"><select name=\"digiPath\" id=\"digiPath\">\n";
		for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
		{
			String pthLabel = String(PATH_NAME[pthIdx]);
			if (pthIdx == PATH_DEFAULT_FIXED)
				pthLabel += " (default)";
			if (config.digi_path == pthIdx)
			{
				html += "<option value=\"" + String(pthIdx) + "\" selected>" + pthLabel + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(pthIdx) + "\">" + pthLabel + "</option>\n";
			}
		}
		html += "</select></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"digiComment\">Text Comment</label><div class=\"dash-field-body\"><input maxlength=\"50\" size=\"50\" id=\"digiComment\" name=\"digiComment\" type=\"text\" value=\"" + String(config.digi_comment) + "\" /></div></div>\n";

		html += "<div class=\"dash-field\"><label for=\"digiDelay\">Repeat Delay</label><div class=\"dash-field-body\"><input min=\"0\" max=\"10000\" step=\"100\" id=\"digiDelay\" name=\"digiDelay\" type=\"number\" value=\"" + String(config.digi_delay) + "\" /> mSec.<span class=\"dash-hint\">0 is auto, otherwise random delay time</span></div></div>\n";

		String timeStampFlag = "";
		if (config.digi_timestamp)
			timeStampFlag = "checked";
		html += "<div class=\"dash-field\"><label>Time Stamp</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"digiTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-section-title\">Position</div>\n";
		html += "<div class=\"dash-panel\">\n";

		String digiBcnEnFlag = "";
		if (config.digi_bcn)
			digiBcnEnFlag = "checked";
		html += "<div class=\"dash-field\"><label>Beacon</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"digiBcnEnable\" value=\"OK\" " + digiBcnEnFlag + "><span class=\"slider round\"></span></label><label>Interval <input min=\"0\" max=\"3600\" step=\"1\" id=\"digiPosInv\" name=\"digiPosInv\" type=\"number\" value=\"" + String(config.digi_interval) + "\" /> Sec.</label></div></div>\n";

		String digiPosFixFlag = "";
		String digiPosGPSFlag = "";
		String digiPos2RFFlag = "";
		String digiPos2INETFlag = "";
		if (config.digi_gps)
			digiPosGPSFlag = "checked=\"checked\"";
		else
			digiPosFixFlag = "checked=\"checked\"";

		if (config.digi_loc2rf)
			digiPos2RFFlag = "checked";
		if (config.digi_loc2inet)
			digiPos2INETFlag = "checked";
		html += "<div class=\"dash-field\"><label>Location Source</label><div class=\"dash-field-body\"><label><input type=\"radio\" name=\"digiPosSel\" value=\"0\" onchange=\"onDigiPosSelChange()\" " + digiPosFixFlag + "/> Fix</label><label><input type=\"radio\" name=\"digiPosSel\" value=\"1\" onchange=\"onDigiPosSelChange()\" " + digiPosGPSFlag + "/> GPS</label></div></div>\n";
		html += "<div class=\"dash-field\"><label>TX Channel</label><div class=\"dash-field-body\"><label><input type=\"checkbox\" name=\"digiPos2RF\" value=\"OK\" " + digiPos2RFFlag + "/> RF</label><label><input type=\"checkbox\" name=\"digiPos2INET\" value=\"OK\" " + digiPos2INETFlag + "/> Internet</label></div></div>\n";
		// Lat/Lon/Alt only make sense for a Fix position - hidden while GPS is
		// selected, same pattern as web_igate.cpp/web_tracker.cpp.
		html += "<div id=\"digiFixPosGrp\"" + String(config.digi_gps ? " style=\"display:none\"" : "") + ">\n";
		html += "<div class=\"dash-field\"><label for=\"digiPosLat\">Latitude</label><div class=\"dash-field-body\"><input min=\"-90\" max=\"90\" step=\"0.00001\" id=\"digiPosLat\" name=\"digiPosLat\" type=\"number\" value=\"" + String(config.digi_lat, 5) + "\" /><span class=\"dash-hint\">degrees (positive for North, negative for South)</span></div></div>\n";
		html += "<div class=\"dash-field\"><label for=\"digiPosLon\">Longitude</label><div class=\"dash-field-body\"><input min=\"-180\" max=\"180\" step=\"0.00001\" id=\"digiPosLon\" name=\"digiPosLon\" type=\"number\" value=\"" + String(config.digi_lon, 5) + "\" /><span class=\"dash-hint\">degrees (positive for East, negative for West)</span></div></div>\n";
		html += "<div class=\"dash-field\"><label for=\"digiPosAlt\">Altitude</label><div class=\"dash-field-body\"><input min=\"0\" max=\"10000\" step=\"0.1\" id=\"digiPosAlt\" name=\"digiPosAlt\" type=\"number\" value=\"" + String(config.digi_alt, 2) + "\" /><span class=\"dash-hint\">meters - value 0 is not sent</span></div></div>\n";
		html += "</div>\n"; // #digiFixPosGrp
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

		html += "<div class=\"dash-field\"><label for=\"texttouse\">PHG Text</label><div class=\"dash-field-body\"><input name=\"texttouse\" type=\"text\" size=\"6\" maxlength=\"" + String(sizeof(config.digi_phg) - 1) + "\" style=\"background-color: rgb(97, 239, 170);\" value=\"" + String(config.digi_phg) + "\"/> <button type=\"button\" onclick=\"javascript:calculatePHGR()\">Calculate PHG</button></div></div>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-section-title\">Filter</div>\n";
		html += "<div class=\"dash-panel\">\n";
		html += "<fieldset id=\"FilterGrp\" class=\"dash-filter-grp\">\n";
		html += "<legend>Digipeat these types</legend>\n";
		html += "<div class=\"dash-checkbox-grid\">\n";

		String filterFlageEn = "";
		if (config.digiFilter & FILTER_MESSAGE)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"FilterMessage\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Message</label>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_STATUS)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"FilterStatus\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Status</label>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_TELEMETRY)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"FilterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Telemetry</label>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_WX)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"FilterWeather\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Weather</label>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_OBJECT)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"FilterObject\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Object</label>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_ITEM)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"FilterItem\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Item</label>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_QUERY)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"FilterQuery\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Query</label>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_BUOY)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"FilterBuoy\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Buoy</label>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_POSITION)
			filterFlageEn = "checked";
		html += "<label><input class=\"field_checkbox\" name=\"FilterPosition\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/> Position</label>\n";

		html += "</div>\n";
		html += "</fieldset>\n";
		html += "</div>\n"; // .dash-panel

		html += "<div class=\"dash-form-actions\"><button type='submit' id='submitDIGI' name=\"commitDIGI\">Apply Change</button></div>\n";
		html += "<input type=\"hidden\" name=\"commitDIGI\"/>\n";
		html += "</form>\n";
		html += "</div>\n"; // .dash
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

