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
#include "base64.hpp"
#include "wireguard_vpn.h"
#include <LibAPRSesp.h>
#include <parse_aprs.h>
#include "wifi_config.h"


extern bool afskSync;
extern String lastPkgRaw;
extern float dBV;
extern int mVrms;
void handle_realtime()
{
	// char jsonMsg[1000];
	char *jsonMsg;
	time_t timeStamp;
	time(&timeStamp);

	if (afskSync && (lastPkgRaw.length() > 5))
	{
		int input_length = lastPkgRaw.length();
		jsonMsg = (char *)malloc((input_length * 2) + 70);
		char *input_buffer = (char *)malloc(input_length + 2);
		char *output_buffer = (char *)malloc(input_length * 2);
		if (output_buffer)
		{
			lastPkgRaw.toCharArray(input_buffer, lastPkgRaw.length(), 0);
			lastPkgRaw.clear();
			encode_base64((unsigned char *)input_buffer, input_length, (unsigned char *)output_buffer);
			// Serial.println(output_buffer);
			sprintf(jsonMsg, "{\"Active\":\"1\",\"mVrms\":\"%d\",\"RAW\":\"%s\",\"timeStamp\":\"%li\"}", mVrms, output_buffer, timeStamp);
			// Serial.println(jsonMsg);
			free(input_buffer);
			free(output_buffer);
		}
	}
	else
	{
		jsonMsg = (char *)malloc(100);
		if (afskSync)
			sprintf(jsonMsg, "{\"Active\":\"1\",\"mVrms\":\"%d\",\"RAW\":\"REVDT0RFIEZBSUwh\",\"timeStamp\":\"%li\"}", mVrms, timeStamp);
		else
			sprintf(jsonMsg, "{\"Active\":\"0\",\"mVrms\":\"0\",\"RAW\":\"\",\"timeStamp\":\"%li\"}", timeStamp);
	}
	afskSync = false;
	server.send(200, "text/html", String(jsonMsg));

	delay(100);
	free(jsonMsg);
}

// void handle_test()
// {
// 	if (server.hasArg("REBOOT"))
// 	{
// 		esp_restart();
// 	}
// 	if (server.hasArg("sendBeacon"))
// 	{
// 		String tnc2Raw = send_fix_location();
// 		if (config.rf_en)
// 			pkgTxPush(tnc2Raw.c_str(), tnc2Raw.length(), 0);
// 		// APRS_sendTNC2Pkt(tnc2Raw); // Send packet to RF
// 	}
// 	else if (server.hasArg("sendRaw"))
// 	{
// 		for (uint8_t i = 0; i < server.args(); i++)
// 		{
// 			if (server.argName(i) == "raw")
// 			{
// 				if (server.arg(i) != "")
// 				{
// 					String tnc2Raw = server.arg(i);
// 					if (config.rf_en)
// 					{
// 						pkgTxPush(tnc2Raw.c_str(), tnc2Raw.length(), 0);
// 						// APRS_sendTNC2Pkt(server.arg(i)); // Send packet to RF
// 						// Serial.println("Send RAW: " + tnc2Raw);
// 					}
// 				}
// 				break;
// 			}
// 		}
// 	}
// 	// setHTML(6);

// 	webString += "<table>\n";
// 	webString += "<tr><td><form accept-charset=\"UTF-8\" action=\"/test\" class=\"form-horizontal\" id=\"test_form\" method=\"post\">\n";
// 	webString += "<div style=\"margin-left: 20px;\"><input type='submit' class=\"btn btn-danger\" name=\"sendBeacon\" value='SEND BEACON'></div><br />\n";
// 	webString += "<div style=\"margin-left: 20px;\">TNC2 RAW: <input id=\"raw\" name=\"raw\" type=\"text\" size=\"60\" value=\"" + String(config.aprs_mycall) + ">APE32I,WIDE1-1:>Test Status\"/></div>\n";
// 	webString += "<div style=\"margin-left: 20px;\"><input type='submit' class=\"btn btn-primary\" name=\"sendRaw\" value='SEND RAW'></div> <br />\n";
// 	webString += "<div style=\"margin-left: 20px;\"><input type='submit' class=\"btn btn-danger\" name=\"REBOOT\" value='REBOOT'></div><br />\n";
// 	webString += "</form></td></tr>\n";
// 	webString += "<tr><td><hr width=\"80%\" /></td></tr>\n";
// 	webString += "<tr><td><div id=\"vumeter\" style=\"width: 300px; height: 200px; margin: 10px;\"></div></td>\n";
// 	webString += "<tr><td><div style=\"margin: 15px;\">Terminal<br /><textarea id=\"raw_txt\" name=\"raw_txt\" rows=\"25\" cols=\"80\" /></textarea></div></td></tr>\n";
// 	webString += "</table>\n";

// 	webString += "</body></html>\n";
// 	server.send(200, "text/html", webString); // send to someones browser when asked

// 	delay(100);
// 	webString.clear();
// }

void handle_about()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	char strCID[50];
	uint64_t chipid = ESP.getEfuseMac();
	sprintf(strCID, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

	webString.clear();
	webString += "<div class=\"dash\">\n";
	webString += "<div class=\"dash-section-title\">System Information</div>\n";
	webString += "<div class=\"dash-panel\">\n";
	webString += "<table>";
	webString += "<tr><td align=\"right\"><b>Hardware Version: </b></td><td align=\"left\"> LILYGO T-TWR Plus </td></tr>";
	webString += "<tr><td align=\"right\"><b>Firmware Version: </b></td><td align=\"left\"> v" + String(VERSION) + String(VERSION_BUILD) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>RF Analog Module: </b></td><td align=\"left\"> MODEL: " + String(RF_TYPE[config.rf_type]) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>ESP32 Model: </b></td><td align=\"left\"> " + String(ESP.getChipModel()) + "</td></tr>";
	webString += "<tr><td align=\"right\"><b>Chip ID: </b></td><td align=\"left\"> " + String(strCID) + "</td></tr>";
	webString += "<tr><td align=\"right\"><b>Revision: </b></td><td align=\"left\"> " + String(ESP.getChipRevision()) + "</td></tr>";
	webString += "<tr><td align=\"right\"><b>Flash: </b></td><td align=\"left\">" + String(ESP.getFlashChipSize() / 1000) + " KByte</td></tr>";
	webString += "<tr><td align=\"right\"><b>PSRAM: </b></td><td align=\"left\">" + String(ESP.getPsramSize() / 1000) + " KByte</td></tr>";
	webString += "</table>";
	webString += "</div>\n"; // .dash-panel

	webString += "<div class=\"dash-section-title\">Library Versions</div>\n";
	webString += "<div class=\"dash-panel\">\n";
	webString += "<table>";
	// Kept in sync by hand with platformio.ini's lib_deps - update both together.
	webString += "<tr><td align=\"right\"><b>Arduino Core: </b></td><td align=\"left\">2.0.11 (espressif32 6.4.0)</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Adafruit SSD1306: </b></td><td align=\"left\">2.5.17</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>base64: </b></td><td align=\"left\">1.4.0</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>TinyGPSPlus-ESP32: </b></td><td align=\"left\">0.0.2</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>ESP32httpUpdate: </b></td><td align=\"left\">2.1.145</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>XPowersLib: </b></td><td align=\"left\">0.1.9</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Adafruit NeoPixel: </b></td><td align=\"left\">1.15.5</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Ai Esp32 Rotary Encoder: </b></td><td align=\"left\">1.7.0</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Wireguard client for LwIP: </b></td><td align=\"left\">1.0.1</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>QRCode: </b></td><td align=\"left\">0.0.1</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>EspSoftwareSerial: </b></td><td align=\"left\">8.2.0</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>NimBLE-Arduino: </b></td><td align=\"left\">2.5.1</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Adafruit GFX: </b></td><td align=\"left\">1.12.6 (vendored)</td></tr>\n";
	webString += "</table>";
	webString += "</div>\n"; // .dash-panel

	webString += "<div class=\"dash-section-title\">WiFi Status</div>\n";
	webString += "<div class=\"dash-panel\">\n";
	webString += "<table>\n";
	webString += "<tr><td align=\"right\"><b>Mode:</b></td>\n";
	webString += "<td align=\"left\">";
	if (config.wifi_mode == WIFI_AP_FIX)
	{
		webString += "AP";
	}
	else if (config.wifi_mode == WIFI_STA_FIX)
	{
		webString += "STA";
	}
	else if (config.wifi_mode == WIFI_AP_STA_FIX)
	{
		webString += "AP+STA";
	}
	else
	{
		webString += "OFF";
	}

	wifi_power_t wpr = WiFi.getTxPower();
	String wifipower = "";
	if (wpr < 8)
	{
		wifipower = "-1 dBm";
	}
	else if (wpr < 21)
	{
		wifipower = "2 dBm";
	}
	else if (wpr < 29)
	{
		wifipower = "5 dBm";
	}
	else if (wpr < 35)
	{
		wifipower = "8.5 dBm";
	}
	else if (wpr < 45)
	{
		wifipower = "11 dBm";
	}
	else if (wpr < 53)
	{
		wifipower = "13 dBm";
	}
	else if (wpr < 61)
	{
		wifipower = "15 dBm";
	}
	else if (wpr < 69)
	{
		wifipower = "17 dBm";
	}
	else if (wpr < 75)
	{
		wifipower = "18.5 dBm";
	}
	else if (wpr < 77)
	{
		wifipower = "19 dBm";
	}
	else if (wpr < 80)
	{
		wifipower = "19.5 dBm";
	}
	else
	{
		wifipower = "20 dBm";
	}

	webString += "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>MAC:</b></td>\n";
	webString += "<td align=\"left\">" + String(WiFi.macAddress()) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Channel:</b></td>\n";
	webString += "<td align=\"left\">" + String(WiFi.channel()) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>TX Power:</b></td>\n";
	webString += "<td align=\"left\">" + String(WiFi.getTxPower()) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>SSID:</b></td>\n";
	webString += "<td align=\"left\">" + String(WiFi.SSID()) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Local IP:</b></td>\n";
	webString += "<td align=\"left\">" + WiFi.localIP().toString() + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Gateway IP:</b></td>\n";
	webString += "<td align=\"left\">" + WiFi.gatewayIP().toString() + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>DNS:</b></td>\n";
	webString += "<td align=\"left\">" + WiFi.dnsIP().toString() + "</td></tr>\n";
	webString += "</table>\n";
	webString += "</div>\n"; // .dash-panel

	webString += "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form' class=\"form-horizontal\">\n";
	webString += "<div class=\"dash-section-title\">Firmware Update</div>\n";
	webString += "<div class=\"dash-panel\">\n";
	webString += "<div class=\"dash-field\"><label for=\"file\">File</label><div class=\"dash-field-body\"><input id=\"file\" name=\"update\" type=\"file\" onchange='sub(this)' /></div></div>\n";
	webString += "<div class=\"dash-field\"><label>Progress</label><div class=\"dash-field-body\"><div id='prgbar'><div id='bar' style=\"width: 0px;\"><label id='prg'></label></div></div></div></div>\n";
	webString += "</div>\n"; // .dash-panel
	webString += "<div class=\"dash-form-actions\"><input type='submit' class=\"btn btn-danger\" id=\"update_sumbit\" value='Firmware Update'></div>\n";

	webString += "</form>\n";
	webString += "<script>"
				 "function sub(obj){"
				 "var fileName = obj.value.split('\\\\');"
				 "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
				 "};"
				 "document.getElementById('upload_form').addEventListener('submit', function(e){"
				 "e.preventDefault();"
				 "var data = new FormData(document.getElementById('upload_form'));"
				 "document.getElementById('update_sumbit').disabled = true;"
				 "var xhr = new XMLHttpRequest();"
				 "xhr.upload.addEventListener('progress', function(evt) {"
				 "if (evt.lengthComputable) {"
				 "var per = evt.loaded / evt.total;"
				 "document.getElementById('prg').innerHTML = Math.round(per*100) + '%';"
				 "document.getElementById('bar').style.width = Math.round(per*100) + '%';"
				 "}"
				 "}, false);"
				 "xhr.addEventListener('load', function() {"
				 "if (xhr.status >= 200 && xhr.status < 300) {"
				 "alert('Wait for system reboot 10sec');"
				 "}"
				 "});"
				 "xhr.open('POST', '/update');"
				 "xhr.send(data);"
				 "});"
				 "</script>";

	webString += "</div>\n"; // .dash
	webString += "</body></html>\n";
	server.send(200, "text/html", webString); // send to someones browser when asked
}

void handle_default()
{
	defaultSetting = true;
	defaultConfig();
	defaultSetting = false;
}

