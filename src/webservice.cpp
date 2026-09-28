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
#include "glcdfont.c" // classic 5x7 font, for overlay-character glyphs in handle_symbol_icon()
#include "config_fields.h"

// Web Server;
WebServer server(80);

String webString;

bool defaultSetting = false;

// Set at UPLOAD_FILE_START in the /update upload handler (webService(),
// below) and checked by every later stage of that same upload plus its
// completion handler - the only way to share auth state across the
// separate per-chunk HTTPUpload callback invocations.
bool updateAuthorized = false;

void serviceHandle()
{
	server.handleClient();
}

void handle_logout()
{
	webString = "Log out";
	server.send(401, "text/html", webString);
}

void setMainPage()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	webString = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
	webString += "<meta name=\"robots\" content=\"index\" />\n";
	webString += "<meta name=\"robots\" content=\"follow\" />\n";
	webString += "<meta name=\"language\" content=\"English\" />\n";
	webString += "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n";
	webString += "<meta name=\"GENERATOR\" content=\"configure 20230924\" />\n";
	webString += "<meta name=\"Author\" content=\"Somkiat Nakhonthai (HS5TQA)\" />\n";
	webString += "<meta name=\"Description\" content=\"Web Embedded Configuration\" />\n";
	webString += "<meta name=\"KeyWords\" content=\"ESP32APRS,ESP32APRS_T-TWR\" />\n";
	webString += "<meta http-equiv=\"Cache-Control\" content=\"no-cache, no-store, must-revalidate\" />\n";
	webString += "<meta http-equiv=\"pragma\" content=\"no-cache\" />\n";
	webString += "<meta http-equiv=\"Expires\" content=\"0\" />\n";
	webString += "<title>ESP32APRS_T-TWR</title>\n";
	webString += "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\" />\n";
	webString += "<script type=\"text/javascript\">\n";
	webString += "function loadInto(id, url, cb) {\n";
	webString += "fetch(url).then(function (r) { return r.text(); }).then(function (html) {\n";
	webString += "var el = document.getElementById(id);\n";
	webString += "el.innerHTML = html;\n";
	// innerHTML never executes embedded <script> tags (unlike jQuery's
	// .load(), which this replaces) - re-insert each one via a freshly
	// created element so the browser actually runs it.
	webString += "var scripts = el.querySelectorAll('script');\n";
	webString += "for (var i = 0; i < scripts.length; i++) {\n";
	webString += "var s = document.createElement('script');\n";
	webString += "if (scripts[i].src) s.src = scripts[i].src;\n";
	webString += "s.textContent = scripts[i].textContent;\n";
	webString += "scripts[i].parentNode.replaceChild(s, scripts[i]);\n";
	webString += "}\n";
	webString += "if (cb) cb();\n";
	webString += "});\n";
	webString += "}\n";
	webString += "function selectTab(evt, tabName) {\n";
	webString += "var i, tabcontent, tablinks;\n";
	webString += "tablinks = document.getElementsByClassName(\"nav-tabs\");\n";
	webString += "for (i = 0; i < tablinks.length; i++) {\n";
	webString += "tablinks[i].className = tablinks[i].className.replace(\" active\", \"\");\n";
	webString += "}\n";
	webString += "\n";
	webString += "//document.getElementById(tabName).style.display = \"block\";\n";
	webString += "if (tabName == 'DashBoard') {\n";
	webString += "loadInto(\"contentmain\", \"/dashboard\");\n";
	webString += "} else if (tabName == 'Radio') {\n";
	webString += "loadInto(\"contentmain\", \"/radio\");\n";
	webString += "} else if (tabName == 'IGATE') {\n";
	webString += "loadInto(\"contentmain\", \"/igate\");\n";
	webString += "} else if (tabName == 'DIGI') {\n";
	webString += "loadInto(\"contentmain\", \"/digi\");\n";
	webString += "} else if (tabName == 'TRACKER') {\n";
	webString += "loadInto(\"contentmain\", \"/tracker\");\n";
	webString += "} else if (tabName == 'VPN') {\n";
	webString += "loadInto(\"contentmain\", \"/vpn\");\n";
	webString += "} else if (tabName == 'Wireless') {\n";
	webString += "loadInto(\"contentmain\", \"/wireless\");\n";
	webString += "} else if (tabName == 'System') {\n";
	webString += "loadInto(\"contentmain\", \"/system\");\n";
	webString += "} else if (tabName == 'File') {\n";
	webString += "loadInto(\"contentmain\", \"/file\");\n";
	webString += "} else if (tabName == 'About') {\n";
	webString += "loadInto(\"contentmain\", \"/about\");\n";
	webString += "}\n";
	webString += "\n";
	webString += "if (evt != null) evt.currentTarget.className += \" active\";\n";
	webString += "}\n";
	webString += "</script>\n";
	webString += "</head>\n";
	webString += "\n";
	webString += "<body onload=\"selectTab(event, 'DashBoard')\">\n";
	webString += "\n";
	webString += "<div class=\"container\">\n";
	webString += "<div class=\"header\">\n";
	// webString += "<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">ESP32APRS T-TWR Plus Firmware V" + String(VERSION) + "</div>\n";
	// webString += "<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\"><a href=\"/logout\">[LOG OUT]</a></div>\n";
	webString += "<h1>ESP32APRS T-TWR Plus</h1>\n";
	webString += "<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\"><a href=\"/logout\">[LOG OUT]</a></div>\n";
	webString += "<div class=\"row\">\n";
	webString += "<ul class=\"nav nav-tabs\" style=\"margin: 5px;\">\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'DashBoard')\">DashBoard</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'Radio')\" id=\"btnRadio\">Radio</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'IGATE')\">IGATE</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'DIGI')\">DIGI</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'TRACKER')\">TRACKER</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'VPN')\">VPN</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'Wireless')\">Wireless</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'System')\">System</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'File')\">File</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'About')\">About</button>\n";
	webString += "</ul>\n";
	webString += "</div>\n";
	webString += "</div>\n";
	webString += "\n";

	webString += "<div class=\"contentwide\" id=\"contentmain\"  style=\"font-size: 2pt;\">\n";
	webString += "\n";
	webString += "</div>\n";
	webString += "<br />\n";
	webString += "<div class=\"footer\">\n";
	webString += "ESP32APRS Web Configuration V" + String(VERSION) + String(VERSION_BUILD) + "<br />DL6ABE\n";
	webString += "<br />\n";
	webString += "</div>\n";
	webString += "</div>\n";
	webString += "<!-- <script type=\"text/javascript\" src=\"/nice-select.min.js\"></script> -->\n";
	webString += "<script type=\"text/javascript\">\n";
	webString += "var selectize = document.querySelectorAll('select')\n";
	webString += "var options = { searchable: true };\n";
	webString += "selectize.forEach(function (select) {\n";
	webString += "if (select.length > 30 && null === select.onchange && !select.name.includes(\"ExtendedId\")) {\n";
	webString += "select.classList.add(\"small\", \"selectize\");\n";
	webString += "tabletd = select.closest('td');\n";
	webString += "tabletd.style.cssText = 'overflow-x:unset';\n";
	webString += "NiceSelect.bind(select, options);\n";
	webString += "}\n";
	webString += "});\n";
	webString += "</script>\n";
	webString += "</body>\n";
	webString += "</html>";
	server.send(200, "text/html", webString); // send to someones browser when asked
}

////////////////////////////////////////////////////////////
// handler for web server request: http://IpAddress/      //
////////////////////////////////////////////////////////////

void handle_css()
{
	const char* css = ".container{width:800px;text-align:left;margin:auto;border-radius:10px 10px 10px 10px;-moz-border-radius:10px 10px 10px 10px;-webkit-border-radius:10px 10px 10px 10px;-khtml-border-radius:10px 10px 10px 10px;-ms-border-radius:10px 10px 10px 10px;box-shadow:3px 3px 3px #707070;background:#fff;border-color: #2194ec;padding: 0px;border-width: 5px;border-style:solid;}body,font{font:12px verdana,arial,sans-serif;color:#fff}.header{background:#2194ec;text-decoration:none;color:#fff;font-family:verdana,arial,sans-serif;text-align:left;padding:5px 0;border-radius:10px 10px 0 0;-moz-border-radius:10px 10px 0 0;-webkit-border-radius:10px 10px 0 0;-khtml-border-radius:10px 10px 0 0;-ms-border-radius:10px 10px 0 0}.content{margin:0 0 0 166px;padding:1px 5px 5px;color:#000;background:#fff;text-align:center;font-size: 8pt;}.contentwide{padding:50px 5px 5px;color:#000;background:#fff;text-align:center}.contentwide h2{color:#000;font:1em verdana,arial,sans-serif;text-align:center;font-weight:700;padding:0;margin:0;font-size: 12pt;}.footer{background:#2194ec;text-decoration:none;color:#fff;font-family:verdana,arial,sans-serif;font-size:9px;text-align:center;padding:10px 0;border-radius:0 0 10px 10px;-moz-border-radius:0 0 10px 10px;-webkit-border-radius:0 0 10px 10px;-khtml-border-radius:0 0 10px 10px;-ms-border-radius:0 0 10px 10px;clear:both}#tail{height:450px;width:805px;overflow-y:scroll;overflow-x:scroll;color:#0f0;background:#000}table{vertical-align:middle;text-align:center;empty-cells:show;padding-left:3;padding-right:3;padding-top:3;padding-bottom:3;border-collapse:collapse;border-color:#0f07f2;border-style:solid;border-spacing:0px;border-width:3px;text-decoration:none;color:#fff;background:#000;font-family:verdana,arial,sans-serif;font-size : 12px;width:100%;white-space:nowrap}table th{font-size: 10pt;font-family:lucidia console,Monaco,monospace;text-shadow:1px 1px #0e038c;text-decoration:none;background:#0525f7;border:1px solid silver}table tr:nth-child(even){background:#f7f7f7}table tr:nth-child(odd){background:#eeeeee}table td{color:#000;font-family:lucidia console,Monaco,monospace;text-decoration:none;border:1px solid #010369}body{background:#edf0f5;color:#000}a{text-decoration:none}a:link,a:visited{text-decoration:none;color:#0000e0;font-weight:400}th:last-child a.tooltip:hover span{left:auto;right:0}ul{padding:5px;margin:10px 0;list-style:none;float:left}ul li{float:left;display:inline;margin:0 10px}ul li a{text-decoration:none;float:left;color:#999;cursor:pointer;font:900 14px/22px arial,Helvetica,sans-serif}ul li a span{margin:0 10px 0 -10px;padding:1px 8px 5px 18px;position:relative;float:left}h1{text-shadow:2px 2px #303030;text-align:center}.toggle{position:absolute;margin-left:-9999px;visibility:hidden}.toggle+label{display:block;position:relative;cursor:pointer;outline:none}input.toggle-round-flat+label{padding:1px;width:33px;height:18px;background-color:#ddd;border-radius:10px;transition:background .4s}input.toggle-round-flat+label:before,input.toggle-round-flat+label:after{display:block;position:absolute;}input.toggle-round-flat+label:before{top:1px;left:1px;bottom:1px;right:1px;background-color:#fff;border-radius:10px;transition:background .4s}input.toggle-round-flat+label:after{top:2px;left:2px;bottom:2px;width:16px;background-color:#ddd;border-radius:12px;transition:margin .4s,background .4s}input.toggle-round-flat:checked+label{background-color:#dd4b39}input.toggle-round-flat:checked+label:after{margin-left:14px;background-color:#dd4b39}@-moz-document url-prefix(){select,input{margin:0;padding:0;border-width:1px;font:12px verdana,arial,sans-serif}input[type=button],button,input[type=submit]{padding:0 3px;border-radius:3px 3px 3px 3px;-moz-border-radius:3px 3px 3px 3px}}.nice-select.small,.nice-select-dropdown li.option{height:24px!important;min-height:24px!important;line-height:24px!important}.nice-select.small ul li:nth-of-type(2){clear:both}.nav{margin-bottom:0;padding-left:10;list-style:none}.nav>li{position:relative;display:block}.nav>li>a{position:relative;display:block;padding:5px 10px}.nav>li>a:hover,.nav>li>a:focus{text-decoration:none;background-color:#eee}.nav>li.disabled>a{color:#999}.nav>li.disabled>a:hover,.nav>li.disabled>a:focus{color:#999;text-decoration:none;background-color:initial;cursor:not-allowed}.nav .open>a,.nav .open>a:hover,.nav .open>a:focus{background-color:#eee;border-color:#428bca}.nav .nav-divider{height:1px;margin:9px 0;overflow:hidden;background-color:#e5e5e5}.nav>li>a>img{max-width:none}.nav-tabs{border-bottom:1px solid #ddd}.nav-tabs>li{float:left;margin-bottom:-1px}.nav-tabs>li>a{margin-right:0;line-height:1.42857143;border:1px solid #ddd;border-radius:10px 10px 0 0}.nav-tabs>li>a:hover{border-color:#eee #eee #ddd}.nav-tabs>button{margin-right:0;line-height:1.42857143;border:2px solid #ddd;border-radius:10px 10px 0 0}.nav-tabs>button:hover{background-color:#25bbfc;border-color:#428bca;color:#eaf2f9;border-bottom-color:transparent;}.nav-tabs>button.active,.nav-tabs>button.active:hover,.nav-tabs>button.active:focus{color:#f7fdfd;background-color:#1aae0d;border:1px solid #ddd;border-bottom-color:transparent;cursor:default}.nav-tabs>li.active>a,.nav-tabs>li.active>a:hover,.nav-tabs>li.active>a:focus{color:#428bca;background-color:#e5e5e5;border:1px solid #ddd;border-bottom-color:transparent;cursor:default}.nav-tabs.nav-justified{width:100%;border-bottom:0}.nav-tabs.nav-justified>li{float:none}.nav-tabs.nav-justified>li>a{text-align:center;margin-bottom:5px}.nav-tabs.nav-justified>.dropdown .dropdown-menu{top:auto;left:auto}.nav-status{float:left;margin:0;padding:3px;width:160px;font-weight:400;min-height:600}#bar,#prgbar {background-color: #f1f1f1;border-radius: 14px}#bar {background-color: #3498db;width: 0%;height: 14px}.switch{position:relative;display:inline-block;width:34px;height:16px}.switch input{opacity:0;width:0;height:0}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#f55959;-webkit-transition:.4s;transition:.4s}.slider:before{position:absolute;content:\"\";height:12px;width:12px;left:2px;bottom:2px;background-color:#fff;-webkit-transition:.4s;transition:.4s}input:checked+.slider{background-color:#5ca30a}input:focus+.slider{box-shadow:0 0 1px #5ca30a}input:checked+.slider:before{-webkit-transform:translateX(16px);-ms-transform:translateX(16px);transform:translateX(16px)}.slider.round{border-radius:34px}.slider.round:before{border-radius:50%}\n";
	server.send_P(200,"text/css",css);
}

void handle_dashboard()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	webString = "<script type=\"text/javascript\">\n";
	webString += "function reloadSidebarInfo() {\n";
	webString += "loadInto(\"sidebarInfo\", \"/sidebarInfo\", function () { setTimeout(reloadSidebarInfo, 10000); });\n";
	webString += "}\n";
	webString += "setTimeout(reloadSidebarInfo, 200);\n";
	webString += "function reloadlastHeard() {\n";
	webString += "loadInto(\"lastHeard\", \"/lastHeard\", function () { setTimeout(reloadlastHeard, 10000); });\n";
	webString += "}\n";
	webString += "setTimeout(reloadlastHeard, 300);\n";
	webString += "window.dispatchEvent(new Event('resize'));\n";
	webString += "</script>\n";

	webString += "<div class=\"nav-status\">\n";
	webString += "<div id=\"sidebarInfo\">\n";
	webString += "</div>\n";
	webString += "<br />\n";

	webString += "<table>\n";
	webString += "<tr>\n";
	webString += "<th colspan=\"2\">Radio Info</th>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>Freq TX</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.freq_tx, 4) + " MHz</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>Freq RX</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.freq_rx, 4) + " MHz</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>H/L</td>\n";
	if (config.rf_power)
		webString += "<td>HIGH</td>\n";
	else
		webString += "<td>LOW</td>\n";
	webString += "</tr>\n";
	webString += "</table>\n";
	webString += "\n";
	webString += "<br />\n";
	webString += "<table>\n";
	webString += "<tr>\n";
	webString += "<th colspan=\"2\">APRS SERVER</th>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>HOST</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.aprs_host) + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>PORT</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.aprs_port) + "</td>\n";
	webString += "</tr>\n";
	webString += "</table>\n";
	webString += "<br />\n";
	webString += "<table>\n";
	webString += "<tr>\n";
	webString += "<th colspan=\"2\">WiFi</th>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>MODE</td>\n";
	String strWiFiMode = "OFF";
	if (config.wifi_mode == WIFI_STA_FIX)
	{
		strWiFiMode = "STA";
	}
	else if (config.wifi_mode == WIFI_AP_FIX)
	{
		strWiFiMode = "AP";
	}
	else if (config.wifi_mode == WIFI_AP_STA_FIX)
	{
		strWiFiMode = "AP+STA";
	}
	webString += "<td style=\"background: #ffffff;\">" + strWiFiMode + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>SSID</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(WiFi.SSID()) + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>RSSI</td>\n";
	if (WiFi.isConnected())
		webString += "<td style=\"background: #ffffff;\">" + String(WiFi.RSSI()) + " dBm</td>\n";
	else
		webString += "<td style=\"background:#606060; color:#b0b0b0;\" aria-disabled=\"true\">Disconnect</td>\n";
	webString += "</tr>\n";
	webString += "</table>\n";
	webString += "<br />\n";
	webString += "<table>\n";
	webString += "<tr>\n";
	webString += "<th colspan=\"2\">Power Info</th>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>VOLTAGE:</td>\n";
	webString += "<td style=\"background: #ffffff;text-align: left;\">" + String(vbat, 2) + " V</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>PERCENT:</td>\n";
	if (battPercent >= 0)
		webString += "<td style=\"background: #ffffff;text-align: left;\">" + String(battPercent) + " %</td>\n";
	else
		webString += "<td style=\"background:#606060; color:#b0b0b0;\" aria-disabled=\"true\">N/A</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>SOURCE:</td>\n";
	if (powerCharging)
		webString += "<td style=\"background:#0b0; color:#030;text-align: left;\">CHARGING</td>\n";
	else if (powerVbusIn)
		webString += "<td style=\"background:#0b0; color:#030;text-align: left;\">USB</td>\n";
	else
		webString += "<td style=\"background:#606060; color:#b0b0b0;\" aria-disabled=\"true\">BATTERY</td>\n";
	webString += "</tr>\n";
	webString += "</table>\n";
	webString += "<br />\n";
	webString += "<table>\n";
	webString += "<tr>\n";
	webString += "<th colspan=\"2\">Bluetooth</th>\n";
	webString += "</tr>\n";
	webString += "<td>Master</td>\n";
	if (config.bt_master)
		webString += "<td style=\"background:#0b0; color:#030; width:50%;\">Enabled</td>\n";
	else
		webString += "<td style=\"background:#606060; color:#b0b0b0;\" aria-disabled=\"true\">Disabled</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<tr>\n";
	webString += "<td>NAME</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.bt_name) + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<tr>\n";
	webString += "<td>MODE</td>\n";
	String btMode = "";
	if (config.bt_mode == 1)
	{
		btMode = "TNC2";
	}
	else if (config.bt_mode == 2)
	{
		btMode = "KISS";
	}
	else
	{
		btMode = "NONE";
	}
	webString += "<td style=\"background: #ffffff;\">" + btMode + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "</table>\n";
	webString += "</div>\n";

	webString += "</div>\n";
	webString += "\n";
	webString += "<div class=\"content\">\n";
	// webString += "<b>LAST HEARD</b>\n";
	webString += "<div id=\"lastHeard\">\n";
	webString += "</div>\n";

	server.send(200, "text/html", webString); // send to someones browser when asked
	delay(100);
	webString.clear();
}

void handle_sidebar()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	String html = "<table>\n";
	html += "<tr>\n";
	html += "<th colspan=\"2\">System Info</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	time_t tn = now() - systemUptime;
	String uptime = String(day(tn) - 1, DEC) + "D " + String(hour(tn), DEC) + ":" + String(minute(tn), DEC);
	html += "<td>UPTIME:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + uptime + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>RAM:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String((int)(ESP.getFreeHeap() / 1000)) + "/" + String((int)(ESP.getHeapSize() / 1000)) + " KB</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>PSRAM:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String((int)(ESP.getFreePsram() / 1000)) + "/" + String((int)(ESP.getPsramSize() / 1000)) + " KB</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>SD:</td>\n";
	uint32_t cardTotal = SD.totalBytes() / (1024 * 1024);
	uint32_t cardUsed = SD.usedBytes() / (1024 * 1024);
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(cardUsed) + "/" + String(cardTotal) + " MB</td>\n";
	html += "</tr>\n";
	html += "</table>\n";
	html += "<br />\n";
	html += "<table style=\"background:white;border-collapse: unset;\">\n";
	html += "<tr>\n";
	html += "<th colspan=\"2\">Modes Enabled</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	if (config.igate_en)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">IGATE</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\">IGATE</th>\n";

	if (config.digi_en)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">DIGI</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\">DIGI</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	if (config.trk_en)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">TRACKER</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\">TRACKER</th>\n";
	html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\">SAT</th>\n";
	html += "</tr>\n";
	html += "</table>\n";
	html += "<br />\n";
	html += "<table style=\"background:white;border-collapse: unset;\">\n";
	html += "<tr>\n";
	html += "<th colspan=\"2\">Network Status</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
	bool aprsIsConnected = aprsClient.connected();
	xSemaphoreGive(aprsClientMutex);
	if (aprsIsConnected == true)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">APRS-IS</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\" aria-disabled=\"true\">APRS-IS</th>\n";
	if (wireguard_active() == true)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">VPN</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\" aria-disabled=\"true\">VPN</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\" aria-disabled=\"true\">4G LTE</th>\n";
	html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\" aria-disabled=\"true\">MQTT</th>\n";
	html += "</tr>\n";
	html += "</table>\n";
	html += "<br />\n";
	html += "<table>\n";
	html += "<tr>\n";
	html += "<th colspan=\"2\">STATISTICS</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">PACKET RX:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.rxCount) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">PACKET TX:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.txCount) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">RF2INET:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.rf2inet) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">INET2RF:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.inet2rf) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">DIGI:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.digiCount) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">DROP/ERR:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.dropCount) + "/" + String(status.errorCount) + "</td>\n";
	html += "</tr>\n";
	html += "</table>\n";
	html += "<br />\n";
	html += "<table>\n";
	html += "<tr>\n";
	html += "<th colspan=\"2\">GPS Info</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>LAT:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(gps.location.lat(), 5) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>LON:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(gps.location.lng(), 5) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>ALT:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(gps.altitude.meters(), 1) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>SAT:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(gps.satellites.value()) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>HDOP:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(gps.hdop.hdop(), 1) + "</td>\n";
	html += "</tr>\n";
	html += "</table>\n";

	html += "<script>\n";
	html += "window.dispatchEvent(new Event('resize'));\n";
	html += "</script>\n";
	server.send(200, "text/html", html); // send to someones browser when asked
	delay(100);
	html.clear();
}


// Downloads the current config as a key=value text file (config_fields.h /
// config_backup.cpp) - survives a firmware update that changes
// sizeof(Configuration) unlike the raw EEPROM checksum, see FORK_NOTES.md.
void handle_configBackup()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	String backup = buildConfigBackup();
	server.sendHeader("Content-Disposition", "attachment; filename=esp32aprs-config.cfg");
	server.send(200, "text/plain", backup);
}

// Completion callback for the /configRestore upload - see its upload
// callback (restoreUploadBuf/restoreUploadAuthorized) in webService() below,
// same auth-before-any-effect shape as the /update fix.
bool restoreUploadAuthorized = false;
String restoreUploadBuf;
void handle_configRestore()
{
	if (!restoreUploadAuthorized)
	{
		return server.requestAuthentication();
	}
	applyConfigBackup(restoreUploadBuf);
	restoreUploadBuf.clear();
	saveEEPROM();
	server.sendHeader("Connection", "close");
	server.send(200, "text/plain", "OK");
	delay(500);
	ESP.restart();
}

bool webServiceBegin=true;
void webService()
{
	if(webServiceBegin){
		webServiceBegin=false;
	}else{
		return;
	}
	server.close();
	// web client handlers
	server.on("/", setMainPage);
	server.on("/logout", handle_logout);
#ifdef SDCARD
	server.on("/file", handle_storage);
	server.on("/download", handle_download);
	server.on("/delete", handle_delete);
#endif

	server.on("/radio", handle_radio);
	server.on("/vpn", handle_vpn);
	server.on("/default", handle_default);
	server.on("/igate", handle_igate);
	server.on("/digi", handle_digi);
	server.on("/tracker", handle_tracker);
	server.on("/system", handle_system);
	server.on("/symbol", handle_symbol);
	server.on("/icon.png", handle_symbol_icon);
	server.on("/wireless", handle_wireless);
	// server.on("/test", handle_test);
	server.on("/realtime", handle_realtime);
	server.on("/about", handle_about);
	server.on("/dashboard", handle_dashboard);
	server.on("/sidebarInfo", handle_sidebar);
	server.on("/lastHeard", handle_lastHeard);
	server.on("/style.css", handle_css);
	server.on("/configBackup", handle_configBackup);
	server.on(
		"/configRestore", HTTP_POST, handle_configRestore,
		[]()
		{
			HTTPUpload &upload = server.upload();
			if (upload.status == UPLOAD_FILE_START)
			{
				// Checked here, before any byte of the upload is applied to
				// config - same auth-before-any-effect shape as the /update
				// fix above (that endpoint originally had no auth check at
				// all; not repeating that mistake here).
				restoreUploadAuthorized = server.authenticate(config.http_username, config.http_password);
				restoreUploadBuf = "";
			}
			else if (upload.status == UPLOAD_FILE_WRITE)
			{
				if (!restoreUploadAuthorized)
					return;
				for (size_t i = 0; i < upload.currentSize; i++)
					restoreUploadBuf += (char)upload.buf[i];
			}
		});
	/*handling uploading firmware file */
	server.on(
		"/update", HTTP_POST, []()
		{
			if (!updateAuthorized)
			{
				return server.requestAuthentication();
			}
			server.sendHeader("Connection", "close");
			server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
			ESP.restart(); },
		[]()
		{
			HTTPUpload &upload = server.upload();
			if (upload.status == UPLOAD_FILE_START)
			{
				// Checked here, not just in the completion handler above -
				// this runs first, before Update.begin() ever touches flash,
				// so an unauthenticated request is rejected before any
				// write happens (found 2026-09-27: this endpoint had no
				// auth check at all, unlike every other admin route).
				updateAuthorized = server.authenticate(config.http_username, config.http_password);
				if (!updateAuthorized)
					return;
				Serial.printf("Firmware Update FILE: %s\n", upload.filename.c_str());
				if (!Update.begin(UPDATE_SIZE_UNKNOWN))
				{ // start with max available size
					Update.printError(Serial);
					delay(3);
				}
				else
				{
					// wdtDisplayTimer = millis();
					// wdtSensorTimer = millis();
					disableCore0WDT();
					disableCore1WDT();
					disableLoopWDT();

					xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
					aprsClient.stop();
					xSemaphoreGive(aprsClientMutex);
					vTaskSuspend(taskAPRSHandle);
					vTaskSuspend(taskTNCHandle);
					vTaskSuspend(taskGpsHandle);
					// vTaskSuspend(taskNetworkHandle);
					// config.igate_en = false;
					// config.rf_en = false;

					delay(3);
				}
			}
			else if (upload.status == UPLOAD_FILE_WRITE)
			{
				if (!updateAuthorized)
					return;
				/* flashing firmware to ESP*/
				if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
				{
					Update.printError(Serial);
					delay(3);
				}
			}
			else if (upload.status == UPLOAD_FILE_END)
			{
				if (!updateAuthorized)
					return;
				if (Update.end(true))
				{ // true to set the size to the current progress
					delay(3);
				}
				else
				{
					Update.printError(Serial);
					delay(3);
				}
			}
		});
	server.begin();
}
