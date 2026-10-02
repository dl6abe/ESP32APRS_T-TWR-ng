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


// Manual "Send Beacon Now" button (Dashboard) - reuses the existing
// EVENT_TX_POSITION flag taskAPRS already polls for automatic/smartbeacon
// sends (main.cpp, "if (EVENT_TX_POSITION > 0)"); event code 9 is unused by
// any of the automatic triggers (1, 4-8), so it's distinguishable in logs.
// Rate-limited client-side-visible: at most once per 60s, tracked here
// since this is the only place that sets it from outside taskAPRS.
static unsigned long trk_lastManualBeacon = 0;

void handle_trackerSendBeacon()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	if (!config.trk_en)
	{
		server.send(200, "text/plain", "FAIL: Tracker is disabled");
		return;
	}
	unsigned long now = millis();
	if (trk_lastManualBeacon != 0 && (now - trk_lastManualBeacon) < 60000UL)
	{
		unsigned long waitSec = (60000UL - (now - trk_lastManualBeacon) + 999) / 1000;
		server.send(200, "text/plain", "FAIL: Please wait " + String(waitSec) + "s");
		return;
	}
	trk_lastManualBeacon = now;
	EVENT_TX_POSITION = 9;
	projLog(LOGCAT_APRS_RF, "Manual beacon requested from Dashboard");
	server.send(200, "text/plain", "OK");
}

void handle_tracker()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	bool trakerEn = false;
	bool smartTrackerEn = false;
	bool smartEn = false;
	bool compEn = false;

	bool posGPS = false;
	bool pos2RF = false;
	bool pos2INET = false;
	bool optCST = false;
	bool optAlt = false;
	bool optBat = false;
	bool optSat = false;
	bool timeStamp = false;

	if (server.hasArg("commitTRACKER"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "trackerEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						trakerEn = true;
				}
			}
			if (server.argName(i) == "smartTrackerEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						smartTrackerEn = true;
				}
			}
			if (server.argName(i) == "smartBcnEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						smartEn = true;
				}
			}
			if (server.argName(i) == "compressEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						compEn = true;
				}
			}
			if (server.argName(i) == "trackerOptCST")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						optCST = true;
				}
			}
			if (server.argName(i) == "trackerOptAlt")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						optAlt = true;
				}
			}
			if (server.argName(i) == "trackerOptBat")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						optBat = true;
				}
			}
			if (server.argName(i) == "trackerOptSat")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						optSat = true;
				}
			}
			if (server.argName(i) == "myCall")
			{
				if (server.arg(i) != "")
				{
					String name=server.arg(i);
					name.trim();
					strcpy(config.trk_mycall, name.c_str());
				}
			}
			if (server.argName(i) == "trackerObject")
			{
				if (server.arg(i) != "")
				{
					String name=server.arg(i);
					name.trim();
					strcpy(config.trk_item, name.c_str());
				}
				else
				{
					config.trk_item[0] = 0;
				}
			}
			if (server.argName(i) == "mySSID")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_ssid = server.arg(i).toInt();
					if (config.trk_ssid > 15)
						config.trk_ssid = 7; // matches this page's own compiled default (setConfigDefaults()), not an unrelated value
				}
			}
			if (server.argName(i) == "trackerPosInv")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_interval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "trackerPosLat")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_lat = server.arg(i).toFloat();
				}
			}

			if (server.argName(i) == "trackerPosLon")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_lon = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "trackerPosAlt")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_alt = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "trackerPosSel")
			{
				if (server.arg(i) != "")
				{
					if (server.arg(i).toInt() == 1)
						posGPS = true;
				}
			}
			if (server.argName(i) == "hspeed")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_hspeed = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "lspeed")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_lspeed = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "slowInterval")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_slowinterval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "maxInterval")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_maxinterval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "minInterval")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_mininterval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "minAngle")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_minangle = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "trackerTable")
			{
				if (server.arg(i) != "")
				{
					config.trk_symbol[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "trackerSymbol")
			{
				if (server.arg(i) != "")
				{
					config.trk_symbol[1] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "moveTable")
			{
				if (server.arg(i) != "")
				{
					config.trk_symmove[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "moveSymbol")
			{
				if (server.arg(i) != "")
				{
					config.trk_symmove[1] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "stopTable")
			{
				if (server.arg(i) != "")
				{
					config.trk_symstop[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "stopSymbol")
			{
				if (server.arg(i) != "")
				{
					config.trk_symstop[1] = server.arg(i).charAt(0);
				}
			}

			if (server.argName(i) == "trackerPath")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_path = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "trackerComment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.trk_comment, server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "trackerPos2RF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2RF = true;
				}
			}
			if (server.argName(i) == "trackerPos2INET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2INET = true;
				}
			}
			if (server.argName(i) == "trackerTimeStamp")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						timeStamp = true;
				}
			}
		}
		config.trk_en = trakerEn;
		config.trk_smarttracker = smartTrackerEn;
		config.trk_smartbeacon = smartEn;
		config.trk_compress = compEn;

		config.trk_gps = posGPS;
		config.trk_loc2rf = pos2RF;
		config.trk_loc2inet = pos2INET;

		config.trk_cst = optCST;
		config.trk_altitude = optAlt;
		config.trk_bat = optBat;
		config.trk_sat = optSat;
		config.trk_timestamp = timeStamp;

		saveEEPROM();
		initInterval=true;
		String html = "OK";
		server.send(200, "text/html", html);
		return; // without this, execution fell through into building and
				// sending the full GET-branch page too, as a second
				// server.send() on the same request
	}

	String html = "<script type=\"text/javascript\">\n";
	html += "document.querySelectorAll('form').forEach(function (form) {\n";
	html += "form.addEventListener('submit', function (e) {\n";
	html += "e.preventDefault();\n";
	html += "var data = new FormData(e.currentTarget);\n";
	html += "document.getElementById(\"submitTRACKER\").disabled=true;\n";
	html += "fetch('/tracker', { method: 'POST', body: data })\n";
	html += ".then(function () { alert(\"Submited Successfully\"); })\n";
	html += ".catch(function () { alert(\"An error occurred.\"); });\n";
	html += "});\n";
	html += "});\n";
	html += "</script>\n<script type=\"text/javascript\">\n";
	html += "function openWindowSymbol(sel) {\n";
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
		html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(\"+sel.toString()+\"," + String(i) + ",1);\\\" src=\\\"/icon.png?c=" + String(i) + "&t=1\\\"></td>\");\n";
		if (((i % 16) == 0) && (i < 126))
			html += "newWindow.document.write(\"</tr><tr>\");\n";
	}
	html += "newWindow.document.write(\"</tr></table><br />\");\n";
	html += "newWindow.document.write(\"<table border=\\\"1\\\" align=\\\"center\\\">\");\n";
	html += "newWindow.document.write(\"<tr><th colspan=\\\"16\\\">Table '\\\'</th></tr><tr>\");\n";
	for (i = 33; i < 129; i++)
	{
		html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(\"+sel.toString()+\"," + String(i) + ",2);\\\" src=\\\"/icon.png?c=" + String(i, DEC) + "&t=2\\\"></td>\");\n";
		if (((i % 16) == 0) && (i < 126))
			html += "newWindow.document.write(\"</tr><tr>\");\n";
	}
	html += "newWindow.document.write(\"</tr></table>\");\n";

	html += "}\n";

	html += "function setValue(sel,symbol,table) {\n";
	html += "var txtsymbol=document.getElementById('trackerSymbol');\n";
	html += "var txttable=document.getElementById('trackerTable');\n";
	html += "var imgicon=document.getElementById('trackerImgSymbol');\n";
	html += "if(sel==1){\n";
	html += "txtsymbol=document.getElementById('moveSymbol');\n";
	html += "txttable=document.getElementById('moveTable');\n";
	html += "imgicon= document.getElementById('moveImgSymbol');\n";
	html += "}else if(sel==2){\n";
	html += "txtsymbol=document.getElementById('stopSymbol');\n";
	html += "txttable=document.getElementById('stopTable');\n";
	html += "imgicon= document.getElementById('stopImgSymbol');\n";
	html += "}\n";
	html += "txtsymbol.value = String.fromCharCode(symbol);\n";
	html += "if(table==1){\n txttable.value='/';\n";
	html += "}else if(table==2){\n txttable.value='\\\\';\n}\n";
	html += "imgicon.src = \"/icon.png?c=\"+symbol.toString()+\"&t=\"+table.toString();\n";
	html += "\n}\n";
	html += "function onSmartCheck() {\n";
	html += "if (document.querySelector('#smartBcnEnable').checked) {\n";
	// Checkbox has been checked
	html += "document.getElementById(\"smartbcnGrp\").disabled=false;\n";
	html += "document.getElementById(\"trackerIntervalGrp\").style.display=\"none\";\n";
	html += "} else {\n";
	// Checkbox has been unchecked
	html += "document.getElementById(\"smartbcnGrp\").disabled=true;\n";
	html += "document.getElementById(\"trackerIntervalGrp\").style.display=\"\";\n";
	html += "}\n}\n";
	html += "function onTrackerPosSelChange() {\n";
	html += "var isGPS = document.querySelector('input[name=\"trackerPosSel\"][value=\"1\"]').checked;\n";
	html += "document.getElementById(\"trackerFixPosGrp\").style.display = isGPS ? \"none\" : \"\";\n";
	html += "}\n";

	html += "</script>\n";

	/************************ tracker Mode **************************/
	html += "<div class=\"dash\">\n";
	html += "<form id='formtracker' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
	html += "<div class=\"dash-section-title\">[TRACKER] Tracker Position Mode</div>\n";
	html += "<div class=\"dash-panel\">\n";

	String trackerEnFlag = "";
	if (config.trk_en)
		trackerEnFlag = "checked";
	html += "<div class=\"dash-field\"><label>Enable</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"trackerEnable\" value=\"OK\" " + trackerEnFlag + "><span class=\"slider round\"></span></label></div></div>\n";

	// SmartTracker (issue #31): only the toggle so far - the actual
	// WiFi-loss auto-transmit behavior isn't implemented yet. Not the
	// existing "Smart Beacon" feature below (dynamic beacon interval,
	// config.trk_smartbeacon) - a different, unrelated setting.
	String smartTrackerEnFlag = "";
	if (config.trk_smarttracker)
		smartTrackerEnFlag = "checked";
	html += "<div class=\"dash-field\"><label>SmartTracker</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"smartTrackerEnable\" value=\"OK\" " + smartTrackerEnFlag + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">Auto-transmit only while WiFi is unavailable (not implemented yet)</span></div></div>\n";

	html += "<div class=\"dash-field\"><label for=\"myCall\">Station Callsign</label><div class=\"dash-field-body\"><input maxlength=\"7\" size=\"9\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.trk_mycall) + "\" /></div></div>\n";

	html += "<div class=\"dash-field\"><label for=\"mySSID\">Station SSID</label><div class=\"dash-field-body\"><select name=\"mySSID\" id=\"mySSID\">\n";
	for (uint8_t ssid = 0; ssid <= 15; ssid++)
	{
		if (config.trk_ssid == ssid)
		{
			html += "<option value=\"" + String(ssid) + "\" selected>" + String(ssid) + "</option>\n";
		}
		else
		{
			html += "<option value=\"" + String(ssid) + "\">" + String(ssid) + "</option>\n";
		}
	}
	html += "</select></div></div>\n";

	html += "<div class=\"dash-field\"><label for=\"trackerObject\">Item/Obj Name</label><div class=\"dash-field-body\"><input maxlength=\"9\" size=\"9\" id=\"trackerObject\" name=\"trackerObject\" type=\"text\" value=\"" + String(config.trk_item) + "\" /><span class=\"dash-hint\">If not used, leave it blank. In use 3-9 charactor</span></div></div>\n";

	html += "<div class=\"dash-field\"><label for=\"trackerPath\">Path</label><div class=\"dash-field-body\"><select name=\"trackerPath\" id=\"trackerPath\">\n";
	for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
	{
		String pthLabel = String(PATH_NAME[pthIdx]);
		if (pthIdx == PATH_DEFAULT_MOBILE)
			pthLabel += " (default)";
		if (config.trk_path == pthIdx)
		{
			html += "<option value=\"" + String(pthIdx) + "\" selected>" + pthLabel + "</option>\n";
		}
		else
		{
			html += "<option value=\"" + String(pthIdx) + "\">" + pthLabel + "</option>\n";
		}
	}
	html += "</select></div></div>\n";

	html += "<div class=\"dash-field\"><label for=\"trackerComment\">Text Comment</label><div class=\"dash-field-body\"><input maxlength=\"50\" size=\"50\" id=\"trackerComment\" name=\"trackerComment\" type=\"text\" value=\"" + String(config.trk_comment) + "\" /></div></div>\n";

	String compressEnFlag = "";
	if (config.trk_compress)
		compressEnFlag = "checked";
	html += "<div class=\"dash-field\"><label>Compress</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"compressEnable\" value=\"OK\" " + compressEnFlag + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">Switch compress packet</span></div></div>\n";

	String timeStampFlag = "";
	if (config.trk_timestamp)
		timeStampFlag = "checked";
	html += "<div class=\"dash-field\"><label>Time Stamp</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" name=\"trackerTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></div></div>\n";

	String trackerPos2RFFlag = "";
	String trackerPos2INETFlag = "";
	if (config.trk_loc2rf)
		trackerPos2RFFlag = "checked";
	if (config.trk_loc2inet)
		trackerPos2INETFlag = "checked";
	html += "<div class=\"dash-field\"><label>TX Channel</label><div class=\"dash-field-body\"><label><input type=\"checkbox\" name=\"trackerPos2RF\" value=\"OK\" " + trackerPos2RFFlag + "/> RF</label><label><input type=\"checkbox\" name=\"trackerPos2INET\" value=\"OK\" " + trackerPos2INETFlag + "/> Internet</label></div></div>\n";

	String trackerOptBatFlag = "";
	String trackerOptSatFlag = "";
	String trackerOptAltFlag = "";
	String trackerOptCSTFlag = "";
	if (config.trk_bat)
		trackerOptBatFlag = "checked";
	if (config.trk_sat)
		trackerOptSatFlag = "checked";
	if (config.trk_altitude)
		trackerOptAltFlag = "checked";
	if (config.trk_cst)
		trackerOptCSTFlag = "checked";
	html += "<div class=\"dash-field\"><label>Option</label><div class=\"dash-field-body\">";
	html += "<label><input type=\"checkbox\" name=\"trackerOptCST\" value=\"OK\" " + trackerOptCSTFlag + "/> Course/Speed</label>";
	html += "<label><input type=\"checkbox\" name=\"trackerOptAlt\" value=\"OK\" " + trackerOptAltFlag + "/> Altitude</label>";
	html += "<label><input type=\"checkbox\" name=\"trackerOptBat\" value=\"OK\" " + trackerOptBatFlag + "/> Battery</label>";
	html += "<label><input type=\"checkbox\" name=\"trackerOptSat\" value=\"OK\" " + trackerOptSatFlag + "/> Satellite</label>";
	html += "</div></div>\n";
	html += "</div>\n"; // .dash-panel

	html += "<div class=\"dash-section-title\">Position</div>\n";
	html += "<div class=\"dash-panel\">\n";

	String smartBcnEnFlag = "";
	if (config.trk_smartbeacon)
		smartBcnEnFlag = "checked";
	html += "<div class=\"dash-field\"><label>Smart Beacon</label><div class=\"dash-field-body\"><label class=\"switch\"><input type=\"checkbox\" id=\"smartBcnEnable\" name=\"smartBcnEnable\" onclick=\"onSmartCheck()\" value=\"OK\" " + smartBcnEnFlag + "><span class=\"slider round\"></span></label><span class=\"dash-hint\">Dynamic interval based on speed/heading (see Smart Beacon section below) - overrides the fixed Interval while enabled</span></div></div>\n";

	// Interval is ignored once Smart Beacon computes tx_interval itself
	// (see main.cpp/beacon_builder.cpp) - hidden while Smart Beacon is on so
	// it can't be mistaken for the active setting.
	html += "<div id=\"trackerIntervalGrp\"" + String(config.trk_smartbeacon ? " style=\"display:none\"" : "") + ">\n";
	html += "<div class=\"dash-field\"><label for=\"trackerPosInv\">Interval</label><div class=\"dash-field-body\"><input min=\"0\" max=\"3600\" step=\"1\" id=\"trackerPosInv\" name=\"trackerPosInv\" type=\"number\" value=\"" + String(config.trk_interval) + "\" /> Sec.</div></div>\n";
	html += "</div>\n"; // #trackerIntervalGrp

	String trackerPosFixFlag = "";
	String trackerPosGPSFlag = "";

	if (config.trk_gps)
		trackerPosGPSFlag = "checked=\"checked\"";
	else
		trackerPosFixFlag = "checked=\"checked\"";

	html += "<div class=\"dash-field\"><label>Location Source</label><div class=\"dash-field-body\"><label><input type=\"radio\" name=\"trackerPosSel\" value=\"0\" onchange=\"onTrackerPosSelChange()\" " + trackerPosFixFlag + "/> Fix</label><label><input type=\"radio\" name=\"trackerPosSel\" value=\"1\" onchange=\"onTrackerPosSelChange()\" " + trackerPosGPSFlag + "/> GPS</label></div></div>\n";

	String table = "1";
	if (config.trk_symbol[0] == 47)
		table = "1";
	if (config.trk_symbol[0] == 92)
		table = "2";
	html += "<div class=\"dash-field\"><label>Symbol Icon</label><div class=\"dash-field-body\">Table:<input maxlength=\"1\" size=\"1\" id=\"trackerTable\" name=\"trackerTable\" type=\"text\" value=\"" + String(config.trk_symbol[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"trackerSymbol\" name=\"trackerSymbol\" type=\"text\" value=\"" + String(config.trk_symbol[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"trackerImgSymbol\" onclick=\"openWindowSymbol(0);\" src=\"/icon.png?c=" + String((int)config.trk_symbol[1]) + "&t=" + table + "\"> <span class=\"dash-hint\">Click icon to select a symbol</span></div></div>\n";

	// Lat/Lon/Alt only make sense for a Fix position - hidden while GPS is
	// selected (issue - Location Source Fix/GPS field visibility).
	html += "<div id=\"trackerFixPosGrp\"" + String(config.trk_gps ? " style=\"display:none\"" : "") + ">\n";
	html += "<div class=\"dash-field\"><label for=\"trackerPosLat\">Latitude</label><div class=\"dash-field-body\"><input min=\"-90\" max=\"90\" step=\"0.00001\" id=\"trackerPosLat\" name=\"trackerPosLat\" type=\"number\" value=\"" + String(config.trk_lat, 5) + "\" /><span class=\"dash-hint\">degrees (positive for North, negative for South)</span></div></div>\n";
	html += "<div class=\"dash-field\"><label for=\"trackerPosLon\">Longitude</label><div class=\"dash-field-body\"><input min=\"-180\" max=\"180\" step=\"0.00001\" id=\"trackerPosLon\" name=\"trackerPosLon\" type=\"number\" value=\"" + String(config.trk_lon, 5) + "\" /><span class=\"dash-hint\">degrees (positive for East, negative for West)</span></div></div>\n";
	html += "<div class=\"dash-field\"><label for=\"trackerPosAlt\">Altitude</label><div class=\"dash-field-body\"><input min=\"0\" max=\"10000\" step=\"0.1\" id=\"trackerPosAlt\" name=\"trackerPosAlt\" type=\"number\" value=\"" + String(config.trk_alt, 2) + "\" /><span class=\"dash-hint\">meters - value 0 is not sent</span></div></div>\n";
	html += "</div>\n"; // #trackerFixPosGrp
	html += "</div>\n"; // .dash-panel

	html += "<div class=\"dash-section-title\">Smart Beacon</div>\n";
	html += "<div class=\"dash-panel\">\n";
	if (config.trk_smartbeacon)
		html += "<fieldset id=\"smartbcnGrp\" class=\"dash-filter-grp\">\n";
	else
		html += "<fieldset id=\"smartbcnGrp\" class=\"dash-filter-grp\" disabled>\n";
	html += "<legend>Smart beacon configuration</legend>\n";

	table = "1";
	if (config.trk_symmove[0] == 47)
		table = "1";
	if (config.trk_symmove[0] == 92)
		table = "2";
	html += "<div class=\"dash-field\"><label>Move Symbol</label><div class=\"dash-field-body\">Table:<input maxlength=\"1\" size=\"1\" id=\"moveTable\" name=\"moveTable\" type=\"text\" value=\"" + String(config.trk_symmove[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"moveSymbol\" name=\"moveSymbol\" type=\"text\" value=\"" + String(config.trk_symmove[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"moveImgSymbol\" onclick=\"openWindowSymbol(1);\" src=\"/icon.png?c=" + String((int)config.trk_symmove[1]) + "&t=" + table + "\"> <span class=\"dash-hint\">Click icon to select the MOVE symbol</span></div></div>\n";

	table = "1";
	if (config.trk_symstop[0] == 47)
		table = "1";
	if (config.trk_symstop[0] == 92)
		table = "2";
	html += "<div class=\"dash-field\"><label>Stop Symbol</label><div class=\"dash-field-body\">Table:<input maxlength=\"1\" size=\"1\" id=\"stopTable\" name=\"stopTable\" type=\"text\" value=\"" + String(config.trk_symstop[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"stopSymbol\" name=\"stopSymbol\" type=\"text\" value=\"" + String(config.trk_symstop[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"stopImgSymbol\" onclick=\"openWindowSymbol(2);\" src=\"/icon.png?c=" + String((int)config.trk_symstop[1]) + "&t=" + table + "\"> <span class=\"dash-hint\">Click icon to select the STOP symbol</span></div></div>\n";

	html += "<div class=\"dash-field\"><label for=\"hspeed\">High Speed</label><div class=\"dash-field-body\"><input size=\"3\" min=\"10\" max=\"1000\" step=\"1\" id=\"hspeed\" name=\"hspeed\" type=\"number\" value=\"" + String(config.trk_hspeed) + "\" /> km/h</div></div>\n";
	html += "<div class=\"dash-field\"><label for=\"lspeed\">Low Speed</label><div class=\"dash-field-body\"><input size=\"3\" min=\"1\" max=\"250\" step=\"1\" id=\"lspeed\" name=\"lspeed\" type=\"number\" value=\"" + String(config.trk_lspeed) + "\" /> km/h</div></div>\n";
	html += "<div class=\"dash-field\"><label for=\"slowInterval\">Slow Interval</label><div class=\"dash-field-body\"><input size=\"3\" min=\"60\" max=\"3600\" step=\"1\" id=\"slowInterval\" name=\"slowInterval\" type=\"number\" value=\"" + String(config.trk_slowinterval) + "\" /> Sec.</div></div>\n";
	html += "<div class=\"dash-field\"><label for=\"maxInterval\">Max Interval</label><div class=\"dash-field-body\"><input size=\"3\" min=\"10\" max=\"255\" step=\"1\" id=\"maxInterval\" name=\"maxInterval\" type=\"number\" value=\"" + String(config.trk_maxinterval) + "\" /> Sec.</div></div>\n";
	html += "<div class=\"dash-field\"><label for=\"minInterval\">Min Interval</label><div class=\"dash-field-body\"><input size=\"3\" min=\"1\" max=\"100\" step=\"1\" id=\"minInterval\" name=\"minInterval\" type=\"number\" value=\"" + String(config.trk_mininterval) + "\" /> Sec.</div></div>\n";
	html += "<div class=\"dash-field\"><label for=\"minAngle\">Min Angle</label><div class=\"dash-field-body\"><input size=\"3\" min=\"1\" max=\"359\" step=\"1\" id=\"minAngle\" name=\"minAngle\" type=\"number\" value=\"" + String(config.trk_minangle) + "\" /> Degree.</div></div>\n";

	html += "</fieldset>\n";
	html += "</div>\n"; // .dash-panel

	html += "<div class=\"dash-form-actions\"><button type='submit' id='submitTRACKER' name=\"commitTRACKER\">Apply Change</button></div>\n";
	html += "<input type=\"hidden\" name=\"commitTRACKER\"/>\n";
	html += "</form>\n";
	html += "</div>\n"; // .dash
	server.send(200, "text/html", html); // send to someones browser when asked
}

