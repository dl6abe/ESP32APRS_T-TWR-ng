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


void handle_tracker()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	bool trakerEn = false;
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
						config.trk_ssid = 13;
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
	html += "} else {\n";
	// Checkbox has been unchecked
	html += "document.getElementById(\"smartbcnGrp\").disabled=true;\n";
	html += "}\n}\n";

	html += "</script>\n";

	/************************ tracker Mode **************************/
	html += "<form id='formtracker' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
	// html += "<h2>[TRACKER] Tracker Position Mode</h2>\n";
	html += "<table>\n";
	// html += "<tr>\n";
	// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
	// html += "<th><span><b>Value</b></span></th>\n";
	// html += "</tr>\n";
	html += "<th colspan=\"2\"><span><b>[TRACKER] Tracker Position Mode</b></span></th>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Enable:</b></td>\n";
	String trackerEnFlag = "";
	if (config.trk_en)
		trackerEnFlag = "checked";
	html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"trackerEnable\" value=\"OK\" " + trackerEnFlag + "><span class=\"slider round\"></span></label></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Station Callsign:</b></td>\n";
	html += "<td style=\"text-align: left;\"><input maxlength=\"7\" size=\"6\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.trk_mycall) + "\" /></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Station SSID:</b></td>\n";
	html += "<td style=\"text-align: left;\">\n";
	html += "<select name=\"mySSID\" id=\"mySSID\">\n";
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
	html += "</select></td>\n";
	html += "</tr>\n";

	html += "<tr>\n";
	html += "<td align=\"right\"><b>Item/Obj Name:</b></td>\n";
	html += "<td style=\"text-align: left;\"><input maxlength=\"9\" size=\"9\" id=\"trackerObject\" name=\"trackerObject\" type=\"text\" value=\"" + String(config.trk_item) + "\" /><i> *If not used, leave it blank.In use 3-9 charactor</i></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>PATH:</b></td>\n";
	html += "<td style=\"text-align: left;\">\n";
	html += "<select name=\"trackerPath\" id=\"trackerPath\">\n";
	for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
	{
		if (config.trk_path == pthIdx)
		{
			html += "<option value=\"" + String(pthIdx) + "\" selected>" + String(PATH_NAME[pthIdx]) + "</option>\n";
		}
		else
		{
			html += "<option value=\"" + String(pthIdx) + "\">" + String(PATH_NAME[pthIdx]) + "</option>\n";
		}
	}
	html += "</select></td>\n";
	//html += "<td style=\"text-align: left;\"><input maxlength=\"72\" size=\"72\" id=\"trackerPath\" name=\"trackerPath\" type=\"text\" value=\"" + String(config.trk_path) + "\" /></td>\n";
	html += "</tr>\n";

	html += "<tr>\n";
	html += "<td align=\"right\"><b>Text Comment:</b></td>\n";
	html += "<td style=\"text-align: left;\"><input maxlength=\"50\" size=\"50\" id=\"trackerComment\" name=\"trackerComment\" type=\"text\" value=\"" + String(config.trk_comment) + "\" /></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Smart Beacon:</b></td>\n";
	String smartBcnEnFlag = "";
	if (config.trk_smartbeacon)
		smartBcnEnFlag = "checked";
	html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" id=\"smartBcnEnable\" name=\"smartBcnEnable\" onclick=\"onSmartCheck()\" value=\"OK\" " + smartBcnEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Switch use to smart beacon mode</i></label></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Compress:</b></td>\n";
	String compressEnFlag = "";
	if (config.trk_compress)
		compressEnFlag = "checked";
	html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"compressEnable\" value=\"OK\" " + compressEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Switch compress packet</i></label></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Time Stamp:</b></td>\n";
	String timeStampFlag = "";
	if (config.trk_timestamp)
		timeStampFlag = "checked";
	html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"trackerTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></td>\n";
	html += "</tr>\n";
	String trackerPos2RFFlag = "";
	String trackerPos2INETFlag = "";
	if (config.trk_loc2rf)
		trackerPos2RFFlag = "checked";
	if (config.trk_loc2inet)
		trackerPos2INETFlag = "checked";
	html += "<tr><td style=\"text-align: right;\"><b>TX Channel:</b></td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"trackerPos2RF\" value=\"OK\" " + trackerPos2RFFlag + "/>RF <input type=\"checkbox\" name=\"trackerPos2INET\" value=\"OK\" " + trackerPos2INETFlag + "/>Internet </td></tr>\n";
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
	html += "<tr><td style=\"text-align: right;\"><b>Option:</b></td><td style=\"text-align: left;\">";
	html += "<input type=\"checkbox\" name=\"trackerOptCST\" value=\"OK\" " + trackerOptCSTFlag + "/>Course/Speed ";
	html += "<input type=\"checkbox\" name=\"trackerOptAlt\" value=\"OK\" " + trackerOptAltFlag + "/>Altitude ";
	html += "<input type=\"checkbox\" name=\"trackerOptBat\" value=\"OK\" " + trackerOptBatFlag + "/>Battery ";
	html += "<input type=\"checkbox\" name=\"trackerOptSat\" value=\"OK\" " + trackerOptSatFlag + "/>Satellite";
	html += "</td></tr>\n";

	html += "<tr>";
	html += "<td align=\"right\"><b>POSITION:</b></td>\n";
	html += "<td align=\"center\">\n";
	html += "<table>";
	html += "<tr><td style=\"text-align: right;\">Interval:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"3600\" step=\"1\" id=\"trackerPosInv\" name=\"trackerPosInv\" type=\"number\" value=\"" + String(config.trk_interval) + "\" />Sec.</label></td></tr>";
	String trackerPosFixFlag = "";
	String trackerPosGPSFlag = "";

	if (config.trk_gps)
		trackerPosGPSFlag = "checked=\"checked\"";
	else
		trackerPosFixFlag = "checked=\"checked\"";

	html += "<tr><td style=\"text-align: right;\">Location:</td><td style=\"text-align: left;\"><input type=\"radio\" name=\"trackerPosSel\" value=\"0\" " + trackerPosFixFlag + "/>Fix <input type=\"radio\" name=\"trackerPosSel\" value=\"1\" " + trackerPosGPSFlag + "/>GPS </td></tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\">Symbol Icon:</td>\n";
	String table = "1";
	if (config.trk_symbol[0] == 47)
		table = "1";
	if (config.trk_symbol[0] == 92)
		table = "2";
	html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"trackerTable\" name=\"trackerTable\" type=\"text\" value=\"" + String(config.trk_symbol[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"trackerSymbol\" name=\"trackerSymbol\" type=\"text\" value=\"" + String(config.trk_symbol[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"trackerImgSymbol\" onclick=\"openWindowSymbol(0);\" src=\"/icon.png?c=" + String((int)config.trk_symbol[1]) + "&t=" + table + "\"> <i>*Click icon for select symbol</i></td>\n";
	html += "</tr>\n";
	html += "<tr><td style=\"text-align: right;\">Latitude:</td><td style=\"text-align: left;\"><input min=\"-90\" max=\"90\" step=\"0.00001\" id=\"trackerPosLat\" name=\"trackerPosLat\" type=\"number\" value=\"" + String(config.trk_lat, 5) + "\" />degrees (positive for North, negative for South)</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Longitude:</td><td style=\"text-align: left;\"><input min=\"-180\" max=\"180\" step=\"0.00001\" id=\"trackerPosLon\" name=\"trackerPosLon\" type=\"number\" value=\"" + String(config.trk_lon, 5) + "\" />degrees (positive for East, negative for West)</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Altitude:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"0.1\" id=\"trackerPosAlt\" name=\"trackerPosAlt\" type=\"number\" value=\"" + String(config.trk_alt, 2) + "\" /> meter. *Value 0 is not send height</td></tr>\n";
	html += "</table></td>";
	html += "</tr>\n";

	html += "<tr>\n";
	html += "<td align=\"right\"><b>Smart Beacon:</b></td>\n";
	html += "<td align=\"center\">\n";
	if (config.trk_smartbeacon)
		html += "<fieldset id=\"smartbcnGrp\">\n";
	else
		html += "<fieldset id=\"smartbcnGrp\" disabled>\n";
	html += "<legend>Smart beacon configuration</legend>\n<table>";
	html += "<tr>\n";
	html += "<td align=\"right\">Move Symbol:</td>\n";
	table = "1";
	if (config.trk_symmove[0] == 47)
		table = "1";
	if (config.trk_symmove[0] == 92)
		table = "2";
	html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"moveTable\" name=\"moveTable\" type=\"text\" value=\"" + String(config.trk_symmove[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"moveSymbol\" name=\"moveSymbol\" type=\"text\" value=\"" + String(config.trk_symmove[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"moveImgSymbol\" onclick=\"openWindowSymbol(1);\" src=\"/icon.png?c=" + String((int)config.trk_symmove[1]) + "&t=" + table + "\"> <i>*Click icon for select MOVE symbol</i></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\">Stop Symbol:</td>\n";
	table = "1";
	if (config.trk_symstop[0] == 47)
		table = "1";
	if (config.trk_symstop[0] == 92)
		table = "2";
	html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"stopTable\" name=\"stopTable\" type=\"text\" value=\"" + String(config.trk_symstop[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"stopSymbol\" name=\"stopSymbol\" type=\"text\" value=\"" + String(config.trk_symstop[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"stopImgSymbol\" onclick=\"openWindowSymbol(2);\" src=\"/icon.png?c=" + String((int)config.trk_symstop[1]) + "&t=" + table + "\"> <i>*Click icon for select STOP symbol</i></td>\n";
	html += "</tr>\n";
	html += "<tr><td style=\"text-align: right;\">High Speed:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"10\" max=\"1000\" step=\"1\" id=\"hspeed\" name=\"hspeed\" type=\"number\" value=\"" + String(config.trk_hspeed) + "\" /> km/h</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Low Speed:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"1\" max=\"250\" step=\"1\" id=\"lspeed\" name=\"lspeed\" type=\"number\" value=\"" + String(config.trk_lspeed) + "\" /> km/h</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Slow Interval:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"60\" max=\"3600\" step=\"1\" id=\"slowInterval\" name=\"slowInterval\" type=\"number\" value=\"" + String(config.trk_slowinterval) + "\" /> Sec.</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Max Interval:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"10\" max=\"255\" step=\"1\" id=\"maxInterval\" name=\"maxInterval\" type=\"number\" value=\"" + String(config.trk_maxinterval) + "\" /> Sec.</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Min Interval:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"1\" max=\"100\" step=\"1\" id=\"minInterval\" name=\"minInterval\" type=\"number\" value=\"" + String(config.trk_mininterval) + "\" /> Sec.</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Min Angle:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"1\" max=\"359\" step=\"1\" id=\"minAngle\" name=\"minAngle\" type=\"number\" value=\"" + String(config.trk_minangle) + "\" /> Degree.</td></tr>\n";

	html += "</table></fieldset></tr></table><br />\n";
	html += "<div><button type='submit' id='submitTRACKER'  name=\"commitTRACKER\"> Apply Change </button></div>\n";
	html += "<input type=\"hidden\" name=\"commitTRACKER\"/>\n";
	html += "</form><br />";
	server.send(200, "text/html", html); // send to someones browser when asked
}

