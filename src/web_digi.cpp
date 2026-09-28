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
					strcpy(config.digi_phg, server.arg(i).c_str());
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
		html += "</script>\n";

		/************************ DIGI Mode **************************/
		html += "<form id='formDIGI' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>[DIGI] Digital Repeater Mode</h2>\n";
		html += "<table>\n";
		// html += "<tr>\n";
		// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
		// html += "<th><span><b>Value</b></span></th>\n";
		// html += "</tr>\n";
		html += "<th colspan=\"2\"><span><b>[DIGI] Dital Repeater Mode</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Enable:</b></td>\n";
		String digiEnFlag = "";
		if (config.digi_en)
			digiEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"digiEnable\" value=\"OK\" " + digiEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Callsign:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"7\" size=\"6\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.digi_mycall) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station SSID:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"mySSID\" id=\"mySSID\">\n";
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
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Symbol:</b></td>\n";
		String table = "1";
		if (config.digi_symbol[0] == 47)
			table = "1";
		if (config.digi_symbol[0] == 92)
			table = "2";
		html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"digiTable\" name=\"digiTable\" type=\"text\" value=\"" + String(config.digi_symbol[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"digiSymbol\" name=\"digiSymbol\" type=\"text\" value=\"" + String(config.digi_symbol[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"digiImgSymbol\" onclick=\"openWindowSymbol();\" src=\"/icon.png?c=" + String((int)config.digi_symbol[1]) + "&t=" + table + "\"> <i>*Click icon for select symbol</i></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"digiPath\" id=\"digiPath\">\n";
		for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
		{
			if (config.digi_path == pthIdx)
			{
				html += "<option value=\"" + String(pthIdx) + "\" selected>" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(pthIdx) + "\">" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		//html += "<td style=\"text-align: left;\"><input maxlength=\"72\" size=\"72\" id=\"digiPath\" name=\"digiPath\" type=\"text\" value=\"" + String(config.digi_path) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Text Comment:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"50\" size=\"50\" id=\"digiComment\" name=\"digiComment\" type=\"text\" value=\"" + String(config.digi_comment) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr><td style=\"text-align: right;\"><b>Repeat Delay:</b></td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"100\" id=\"digiDelay\" name=\"digiDelay\" type=\"number\" value=\"" + String(config.digi_delay) + "\" /> mSec. <i>*0 is auto,Other random of delay time</i></td></tr>";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Time Stamp:</b></td>\n";
		String timeStampFlag = "";
		if (config.digi_timestamp)
			timeStampFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"digiTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr><td align=\"right\"><b>POSITION:</b></td>\n";
		html += "<td align=\"center\">\n";
		html += "<table>";
		String digiBcnEnFlag = "";
		if (config.digi_bcn)
			digiBcnEnFlag = "checked";

		html += "<tr><td style=\"text-align: right;\">Beacon:</td><td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"digiBcnEnable\" value=\"OK\" " + digiBcnEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\">  Interval:<input min=\"0\" max=\"3600\" step=\"1\" id=\"digiPosInv\" name=\"digiPosInv\" type=\"number\" value=\"" + String(config.digi_interval) + "\" />Sec.</label></td></tr>";
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
		html += "<tr><td style=\"text-align: right;\">Location:</td><td style=\"text-align: left;\"><input type=\"radio\" name=\"digiPosSel\" value=\"0\" " + digiPosFixFlag + "/>Fix <input type=\"radio\" name=\"digiPosSel\" value=\"1\" " + digiPosGPSFlag + "/>GPS </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">TX Channel:</td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"digiPos2RF\" value=\"OK\" " + digiPos2RFFlag + "/>RF <input type=\"checkbox\" name=\"digiPos2INET\" value=\"OK\" " + digiPos2INETFlag + "/>Internet </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Latitude:</td><td style=\"text-align: left;\"><input min=\"-90\" max=\"90\" step=\"0.00001\" id=\"digiPosLat\" name=\"digiPosLat\" type=\"number\" value=\"" + String(config.digi_lat, 5) + "\" />degrees (positive for North, negative for South)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Longitude:</td><td style=\"text-align: left;\"><input min=\"-180\" max=\"180\" step=\"0.00001\" id=\"digiPosLon\" name=\"digiPosLon\" type=\"number\" value=\"" + String(config.digi_lon, 5) + "\" />degrees (positive for East, negative for West)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Altitude:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"0.1\" id=\"digiPosAlt\" name=\"digiPosAlt\" type=\"number\" value=\"" + String(config.digi_alt, 2) + "\" /> meter. *Value 0 is not send height</td></tr>\n";
		html += "</table></td>";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>PHG:</b></td>\n";
		html += "<td align=\"center\">\n";
		html += "<table>";
		html += "<tr>\n";
		html += "<td align=\"right\">Radio TX Power</td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"power\" id=\"power\">\n";
		html += "<option value=\"1\" selected>1</option>\n";
		html += "<option value=\"5\">5</option>\n";
		html += "<option value=\"10\">10</option>\n";
		html += "<option value=\"15\">15</option>\n";
		html += "<option value=\"25\">25</option>\n";
		html += "<option value=\"35\">35</option>\n";
		html += "<option value=\"50\">50</option>\n";
		html += "<option value=\"65\">65</option>\n";
		html += "<option value=\"80\">80</option>\n";
		html += "</select> Watts</td>\n";
		html += "</tr>\n";
		html += "<tr><td style=\"text-align: right;\">Antenna Gain</td><td style=\"text-align: left;\"><input size=\"3\" min=\"0\" max=\"100\" step=\"0.1\" id=\"gain\" name=\"gain\" type=\"number\" value=\"6\" /> dBi</td></tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\">Height</td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"haat\" id=\"haat\">\n";
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
		html += "</select> Feet</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\">Antenna/Direction</td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"direction\" id=\"direction\">\n";
		html += "<option>Omni</option><option>NE</option><option>E</option><option>SE</option><option>S</option><option>SW</option><option>W</option><option>NW</option><option>N</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr><td align=\"right\"><b>PHG Text</b></td><td align=\"left\"><input name=\"texttouse\" type=\"text\" size=\"6\" style=\"background-color: rgb(97, 239, 170);\" value=\"" + String(config.digi_phg) + "\"/> <input type=\"button\" value=\"Calculate PHG\" onclick=\"javascript:calculatePHGR()\" /></td></tr>\n";

		html += "</table></tr>";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Filter:</b></td>\n";

		html += "<td align=\"center\">\n";
		html += "<fieldset id=\"FilterGrp\">\n";
		html += "<legend>Digipeat these types</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
		html += "<tr style=\"background:unset;\">";

		String filterFlageEn = "";
		if (config.digiFilter & FILTER_MESSAGE)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterMessage\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Message</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_STATUS)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterStatus\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Status</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_TELEMETRY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Telemetry</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_WX)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterWeather\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Weather</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_OBJECT)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterObject\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Object</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_ITEM)
			filterFlageEn = "checked";
		html += "</tr><tr style=\"background:unset;\"><td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterItem\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Item</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_QUERY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterQuery\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Query</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_BUOY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterBuoy\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Buoy</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_POSITION)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterPosition\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Position</td>\n";

		html += "<td style=\"border:unset;\"></td>";
		html += "</tr></table></fieldset>\n";
		html += "</td></tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitDIGI'  name=\"commitDIGI\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitDIGI\"/>\n";
		html += "</form><br />";
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

