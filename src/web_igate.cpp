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
						config.aprs_ssid = 13;
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
					strcpy(config.igate_phg, server.arg(i).c_str());
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
		html += "</script>\n";

		/************************ IGATE Mode **************************/
		html += "<form id='formIgate' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>[IGATE] Internet Gateway Mode</h2>\n";
		html += "<table>\n";
		// html += "<tr>\n";
		// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
		// html += "<th><span><b>Value</b></span></th>\n";
		// html += "</tr>\n";
		html += "<th colspan=\"2\"><span><b>[IGATE] Internet Gateway Mode</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Enable:</b></td>\n";
		String igateEnFlag = "";
		if (config.igate_en)
			igateEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"igateEnable\" value=\"OK\" " + igateEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Callsign:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"7\" size=\"6\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.aprs_mycall) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station SSID:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"mySSID\" id=\"mySSID\">\n";
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
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Symbol:</b></td>\n";
		String table = "1";
		if (config.igate_symbol[0] == 47)
			table = "1";
		if (config.igate_symbol[0] == 92)
			table = "2";
		html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"igateTable\" name=\"igateTable\" type=\"text\" value=\"" + String(config.igate_symbol[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"igateSymbol\" name=\"igateSymbol\" type=\"text\" value=\"" + String(config.igate_symbol[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"igateImgSymbol\" onclick=\"openWindowSymbol();\" src=\"/icon.png?c=" + String((int)config.igate_symbol[1]) + "&t=" + table + "\"> <i>*Click icon for select symbol</i></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Item/Obj Name:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"9\" size=\"9\" id=\"igateObject\" name=\"igateObject\" type=\"text\" value=\"" + String(config.igate_object) + "\" /><i> *If not used, leave it blank.In use 3-9 charactor</i></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"igatePath\" id=\"igatePath\">\n";
		for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
		{
			if (config.igate_path == pthIdx)
			{
				html += "<option value=\"" + String(pthIdx) + "\" selected>" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(pthIdx) + "\">" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		//html += "<td style=\"text-align: left;\"><input maxlength=\"72\" size=\"72\" id=\"igatePath\" name=\"igatePath\" type=\"text\" value=\"" + String(config.igate_path) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Server Host:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"20\" size=\"20\" id=\"aprsHost\" name=\"aprsHost\" type=\"text\" value=\"" + String(config.aprs_host) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Server Port:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"1\" max=\"65535\" step=\"1\" id=\"aprsPort\" name=\"aprsPort\" type=\"number\" value=\"" + String(config.aprs_port) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Server Filter:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"30\" size=\"30\" id=\"aprsFilter\" name=\"aprsFilter\" type=\"text\" value=\"" + String(config.aprs_filter) + "\" /> *Filter: <a target=\"_blank\" href=\"http://www.aprs-is.net/javAPRSFilter.aspx\">http://www.aprs-is.net/javAPRSFilter.aspx</a></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Text Comment:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"50\" size=\"50\" id=\"igateComment\" name=\"igateComment\" type=\"text\" value=\"" + String(config.igate_comment) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>RF2INET:</b></td>\n";
		String rf2inetEnFlag = "";
		if (config.rf2inet)
			rf2inetEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" id=\"rf2inetEnable\" name=\"rf2inetEnable\" onclick=\"onRF2INETCheck()\" value=\"OK\" " + rf2inetEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Switch RF to Internet gateway</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>INET2RF:</b></td>\n";
		String inet2rfEnFlag = "";
		if (config.inet2rf)
			inet2rfEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" id=\"inet2rfEnable\" name=\"inet2rfEnable\" onclick=\"onINET2RFCheck()\" value=\"OK\" " + inet2rfEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Switch Internet to RF gateway</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Time Stamp:</b></td>\n";
		String timeStampFlag = "";
		if (config.igate_timestamp)
			timeStampFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"igateTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n<tr>";

		html += "<td align=\"right\"><b>POSITION:</b></td>\n";
		html += "<td align=\"center\">\n";
		html += "<table>";
		String igateBcnEnFlag = "";
		if (config.igate_bcn)
			igateBcnEnFlag = "checked";

		html += "<tr><td style=\"text-align: right;\">Beacon:</td><td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"igateBcnEnable\" value=\"OK\" " + igateBcnEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\">  Interval:<input min=\"0\" max=\"3600\" step=\"1\" id=\"igatePosInv\" name=\"igatePosInv\" type=\"number\" value=\"" + String(config.igate_interval) + "\" />Sec.</label></td></tr>";
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
		html += "<tr><td style=\"text-align: right;\">Location:</td><td style=\"text-align: left;\"><input type=\"radio\" name=\"igatePosSel\" value=\"0\" " + igatePosFixFlag + "/>Fix <input type=\"radio\" name=\"igatePosSel\" value=\"1\" " + igatePosGPSFlag + "/>GPS </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">TX Channel:</td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"igatePos2RF\" value=\"OK\" " + igatePos2RFFlag + "/>RF <input type=\"checkbox\" name=\"igatePos2INET\" value=\"OK\" " + igatePos2INETFlag + "/>Internet </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Latitude:</td><td style=\"text-align: left;\"><input min=\"-90\" max=\"90\" step=\"0.00001\" id=\"igatePosLat\" name=\"igatePosLat\" type=\"number\" value=\"" + String(config.igate_lat, 5) + "\" />degrees (positive for North, negative for South)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Longitude:</td><td style=\"text-align: left;\"><input min=\"-180\" max=\"180\" step=\"0.00001\" id=\"igatePosLon\" name=\"igatePosLon\" type=\"number\" value=\"" + String(config.igate_lon, 5) + "\" />degrees (positive for East, negative for West)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Altitude:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"0.1\" id=\"igatePosAlt\" name=\"igatePosAlt\" type=\"number\" value=\"" + String(config.igate_alt, 2) + "\" /> meter. *Value 0 is not send height</td></tr>\n";
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

		html += "<tr><td align=\"right\"><b>PHG Text</b></td><td align=\"left\"><input name=\"texttouse\" type=\"text\" size=\"6\" style=\"background-color: rgb(97, 239, 170);\" value=\"" + String(config.igate_phg) + "\"/> <input type=\"button\" value=\"Calculate PHG\" onclick=\"javascript:calculatePHGR()\" /></td></tr>\n";
		html += "</table></td>";
		html += "</tr>\n";

		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitIGATE'  name=\"commitIGATE\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitIGATE\"/>\n";
		html += "</form><br /><br />";

		html += "<form id='formIgateFilter' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>[IGATE] Filter</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>RF2INET Filter:</b></td>\n";

		html += "<td align=\"center\">\n";
		if (config.rf2inet)
			html += "<fieldset id=\"rf2inetFilterGrp\">\n";
		else
			html += "<fieldset id=\"rf2inetFilterGrp\" disabled>\n";
		html += "<legend>Pass RF to Internet</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
		html += "<tr style=\"background:unset;\">";

		String filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_MESSAGE)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterMessage\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Message</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_STATUS)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterStatus\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Status</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_TELEMETRY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Telemetry</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_WX)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterWeather\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Weather</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_OBJECT)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterObject\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Object</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_ITEM)
			filterFlageEn = "checked";
		html += "</tr><tr style=\"background:unset;\"><td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterItem\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Item</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_QUERY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterQuery\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Query</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_BUOY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterBuoy\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Buoy</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_POSITION)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterPosition\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Position</td>\n";

		html += "<td style=\"border:unset;\"></td>";
		html += "</tr></table></fieldset>\n";
		html += "</td></tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>INET2RF Filter:</b></td>\n";

		html += "<td align=\"center\">\n";
		if (config.inet2rf)
			html += "<fieldset id=\"inet2rfFilterGrp\">\n";
		else
			html += "<fieldset id=\"inet2rfFilterGrp\" disabled>\n";
		html += "<legend>Pass Internet to RF</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
		html += "<tr style=\"background:unset;\">";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_MESSAGE)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterMessage\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Message</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_STATUS)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterStatus\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Status</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_TELEMETRY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Telemetry</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_WX)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterWeather\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Weather</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_OBJECT)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterObject\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Object</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_ITEM)
			filterFlageEn = "checked";
		html += "</tr><tr style=\"background:unset;\"><td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterItem\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Item</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_QUERY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterQuery\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Query</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_BUOY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterBuoy\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Buoy</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_POSITION)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterPosition\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Position</td>\n";

		html += "<td style=\"border:unset;\"></td>";
		html += "</tr></table></fieldset>\n";
		html += "</td></tr>\n";

		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitIGATEfilter'  name=\"commitIGATEfilter\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitIGATEfilter\"/>\n";
		html += "</form><br />";
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

