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
#include "config_json.h"

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
	// Without this, phone browsers render the page at a desktop-width
	// viewport (~980px) and zoom the whole thing out to fit, rather than
	// laying it out at the device's actual CSS width - every other mobile
	// fix here (the .dash-grid/.dash-field wrapping, #lastHeard's
	// overflow-x) is moot without it.
	webString += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n";
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
	// htmx drives the Dashboard tab's auto-refreshing panels (hx-get/hx-trigger,
	// see handle_dashboard()) - loaded once here so it survives tab switches;
	// harmless on every other tab since it only acts on hx-* attributes.
	webString += "<script type=\"text/javascript\" src=\"/htmx.min.js\"></script>\n";
	webString += "<script type=\"text/javascript\">\n";
	webString += "function loadInto(id, url, cb) {\n";
	webString += "fetch(url).then(function (r) { return r.text(); }).then(function (html) {\n";
	webString += "var el = document.getElementById(id);\n";
	webString += "el.innerHTML = html;\n";
	// #contentmain starts with an inline font-size:2pt (keeps the empty
	// placeholder invisible before its first tab loads) - innerHTML only
	// replaces children, never the element's own attributes, so without this
	// it stays stuck forever and anything loaded into it that doesn't set
	// its own explicit font-size silently inherits an unreadable 2pt.
	webString += "el.style.fontSize = '';\n";
	// htmx only auto-activates hx-* elements that exist at its own load time,
	// or that it swaps in itself - content injected by our own innerHTML
	// assignment here needs an explicit htmx.process() or its hx-get panels
	// (Dashboard's sidebarInfo/lastHeard) never fire their first request.
	webString += "if (window.htmx) { htmx.process(el); }\n";
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
	webString += "} else if (tabName == 'Console') {\n";
	webString += "loadInto(\"contentmain\", \"/console\");\n";
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
	// webString += "<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">ESP32APRS T-TWR Plus Firmware v" + String(VERSION) + "</div>\n";
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
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'Console')\">Console</button>\n";
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
	webString += "ESP32APRS Web Configuration v" + String(VERSION) + String(VERSION_BUILD) + "<br />DL6ABE\n";
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

// --- Dashboard stat-card icons (Lucide, ISC license, https://lucide.dev) ---
// Trimmed inline SVGs (no license comment/class attr); stroke is currentColor
// so .dash-card CSS colors the icon together with its status text.
static const char *const ICON_RADIO = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><path d=\"M16.247 7.761a6 6 0 0 1 0 8.478\"/><path d=\"M19.075 4.933a10 10 0 0 1 0 14.134\"/><path d=\"M4.925 19.067a10 10 0 0 1 0-14.134\"/><path d=\"M7.753 16.239a6 6 0 0 1 0-8.478\"/><circle cx=\"12\" cy=\"12\" r=\"2\"/></svg>";
static const char *const ICON_SERVER = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><rect width=\"20\" height=\"8\" x=\"2\" y=\"2\" rx=\"2\" ry=\"2\"/><rect width=\"20\" height=\"8\" x=\"2\" y=\"14\" rx=\"2\" ry=\"2\"/><line x1=\"6\" x2=\"6.01\" y1=\"6\" y2=\"6\"/><line x1=\"6\" x2=\"6.01\" y1=\"18\" y2=\"18\"/></svg>";
static const char *const ICON_WIFI = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><path d=\"M12 20h.01\"/><path d=\"M2 8.82a15 15 0 0 1 20 0\"/><path d=\"M5 12.859a10 10 0 0 1 14 0\"/><path d=\"M8.5 16.429a5 5 0 0 1 7 0\"/></svg>";
static const char *const ICON_BATTERY = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><path d=\"m11 7-3 5h4l-3 5\"/><path d=\"M14.856 6H16a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2h-2.935\"/><path d=\"M22 14v-4\"/><path d=\"M5.14 18H4a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h2.936\"/></svg>";
static const char *const ICON_BLUETOOTH = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><path d=\"m7 7 10 10-5 5V2l5 5L7 17\"/></svg>";
static const char *const ICON_SATELLITE = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><path d=\"M18 12a6 6 0 00-6-6\"/><path d=\"M2.824 10.459a8 8 0 0010.717 10.717c.558-.276.623-1.012.183-1.452l-9.448-9.448c-.44-.44-1.176-.375-1.452.183\"/><path d=\"M22 12A10 10 0 0012 2\"/><path d=\"m9 15 4-4\"/></svg>";
static const char *const ICON_CPU = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><path d=\"M12 20v2\"/><path d=\"M12 2v2\"/><path d=\"M17 20v2\"/><path d=\"M17 2v2\"/><path d=\"M2 12h2\"/><path d=\"M2 17h2\"/><path d=\"M2 7h2\"/><path d=\"M20 12h2\"/><path d=\"M20 17h2\"/><path d=\"M20 7h2\"/><path d=\"M7 20v2\"/><path d=\"M7 2v2\"/><rect x=\"4\" y=\"4\" width=\"16\" height=\"16\" rx=\"2\"/><rect x=\"8\" y=\"8\" width=\"8\" height=\"8\" rx=\"1\"/></svg>";
static const char *const ICON_TOGGLE = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><circle cx=\"15\" cy=\"12\" r=\"3\"/><rect width=\"20\" height=\"14\" x=\"2\" y=\"5\" rx=\"7\"/></svg>";
// "Off" variant of ICON_TOGGLE (Lucide toggle-left) - knob drawn at cx=9
// (left side) instead of cx=15, so the dashboard switch icon actually moves
// between enabled/disabled instead of always showing the "on" position
// (Gitea issue #51).
static const char *const ICON_TOGGLE_OFF = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><circle cx=\"9\" cy=\"12\" r=\"3\"/><rect width=\"20\" height=\"14\" x=\"2\" y=\"5\" rx=\"7\"/></svg>";
static const char *const ICON_GLOBE = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><circle cx=\"12\" cy=\"12\" r=\"10\"/><path d=\"M12 2a14.5 14.5 0 0 0 0 20 14.5 14.5 0 0 0 0-20\"/><path d=\"M2 12h20\"/></svg>";
static const char *const ICON_CHART = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><path d=\"M3 3v16a2 2 0 0 0 2 2h16\"/><path d=\"M18 17V9\"/><path d=\"M13 17V5\"/><path d=\"M8 17v-3\"/></svg>";
static const char *const ICON_DRIVE = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><path d=\"M10 16h.01\"/><path d=\"M2.212 11.577a2 2 0 0 0-.212.896V18a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-5.527a2 2 0 0 0-.212-.896L18.55 5.11A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z\"/><path d=\"M21.946 12.013H2.054\"/><path d=\"M6 16h.01\"/></svg>";
static const char *const ICON_CLOCK = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><circle cx=\"12\" cy=\"12\" r=\"10\"/><path d=\"M12 6v6l4 2\"/></svg>";
static const char *const ICON_PIN = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" ><path d=\"M20 10c0 4.993-5.539 10.193-7.399 11.799a1 1 0 0 1-1.202 0C9.539 20.193 4 14.993 4 10a8 8 0 0 1 16 0\"/><circle cx=\"12\" cy=\"10\" r=\"3\"/></svg>";

// Builds one dashboard/sidebar stat tile: icon + label + value. statusClass
// is "" for a neutral tile or "ok"/"off" to tint it green/gray - replaces the
// per-row inline background-color styling the old tables used for the same
// on/off/connected states.
static String dashCard(const char *icon, const String &label, const String &value, const char *statusClass = "")
{
	String html = "<article class=\"dash-card";
	if (statusClass[0])
	{
		html += " ";
		html += statusClass;
	}
	html += "\"><div class=\"dash-card-icon\">";
	html += icon;
	html += "</div><div class=\"dash-card-body\"><span class=\"dash-card-label\">";
	html += label;
	html += "</span><span class=\"dash-card-value\">";
	html += value;
	html += "</span></div></article>\n";
	return html;
}

void handle_css()
{
	const char* css = ".container{width:95%;max-width:1400px;text-align:left;margin:auto;border-radius:10px 10px 10px 10px;-moz-border-radius:10px 10px 10px 10px;-webkit-border-radius:10px 10px 10px 10px;-khtml-border-radius:10px 10px 10px 10px;-ms-border-radius:10px 10px 10px 10px;box-shadow:3px 3px 3px #707070;background:#fff;border-color: #2194ec;padding: 0px;border-width: 5px;border-style:solid;}body,font{font:12px verdana,arial,sans-serif;color:#fff}.header{background:#2194ec;text-decoration:none;color:#fff;font-family:verdana,arial,sans-serif;text-align:left;padding:5px 0;border-radius:10px 10px 0 0;-moz-border-radius:10px 10px 0 0;-webkit-border-radius:10px 10px 0 0;-khtml-border-radius:10px 10px 0 0;-ms-border-radius:10px 10px 0 0}.content{margin:0 0 0 166px;padding:1px 5px 5px;color:#000;background:#fff;text-align:center;font-size: 8pt;}.contentwide{padding:50px 5px 5px;color:#000;background:#fff;text-align:center}.contentwide h2{color:#000;font:1em verdana,arial,sans-serif;text-align:center;font-weight:700;padding:0;margin:0;font-size: 12pt;}.footer{background:#2194ec;text-decoration:none;color:#fff;font-family:verdana,arial,sans-serif;font-size:9px;text-align:center;padding:10px 0;border-radius:0 0 10px 10px;-moz-border-radius:0 0 10px 10px;-webkit-border-radius:0 0 10px 10px;-khtml-border-radius:0 0 10px 10px;-ms-border-radius:0 0 10px 10px;clear:both}#tail{height:450px;width:805px;overflow-y:scroll;overflow-x:scroll;color:#0f0;background:#000}table{vertical-align:middle;text-align:center;empty-cells:show;padding-left:3;padding-right:3;padding-top:3;padding-bottom:3;border-collapse:collapse;border-color:#0f07f2;border-style:solid;border-spacing:0px;border-width:3px;text-decoration:none;color:#fff;background:#000;font-family:verdana,arial,sans-serif;font-size : 12px;width:100%;white-space:nowrap}table th{font-size: 10pt;font-family:lucidia console,Monaco,monospace;text-shadow:1px 1px #0e038c;text-decoration:none;background:#0525f7;border:1px solid silver}table tr:nth-child(even){background:#f7f7f7}table tr:nth-child(odd){background:#eeeeee}table td{color:#000;font-family:lucidia console,Monaco,monospace;text-decoration:none;border:1px solid #010369}body{background:#edf0f5;color:#000}a{text-decoration:none}a:link,a:visited{text-decoration:none;color:#0000e0;font-weight:400}th:last-child a.tooltip:hover span{left:auto;right:0}ul{padding:5px;margin:10px 0;list-style:none;float:left}ul li{float:left;display:inline;margin:0 10px}ul li a{text-decoration:none;float:left;color:#999;cursor:pointer;font:900 14px/22px arial,Helvetica,sans-serif}ul li a span{margin:0 10px 0 -10px;padding:1px 8px 5px 18px;position:relative;float:left}h1{text-shadow:2px 2px #303030;text-align:center}.toggle{position:absolute;margin-left:-9999px;visibility:hidden}.toggle+label{display:block;position:relative;cursor:pointer;outline:none}input.toggle-round-flat+label{padding:1px;width:33px;height:18px;background-color:#ddd;border-radius:10px;transition:background .4s}input.toggle-round-flat+label:before,input.toggle-round-flat+label:after{display:block;position:absolute;}input.toggle-round-flat+label:before{top:1px;left:1px;bottom:1px;right:1px;background-color:#fff;border-radius:10px;transition:background .4s}input.toggle-round-flat+label:after{top:2px;left:2px;bottom:2px;width:16px;background-color:#ddd;border-radius:12px;transition:margin .4s,background .4s}input.toggle-round-flat:checked+label{background-color:#dd4b39}input.toggle-round-flat:checked+label:after{margin-left:14px;background-color:#dd4b39}@-moz-document url-prefix(){select,input{margin:0;padding:0;border-width:1px;font:12px verdana,arial,sans-serif}input[type=button],button,input[type=submit]{padding:0 3px;border-radius:3px 3px 3px 3px;-moz-border-radius:3px 3px 3px 3px}}.nice-select.small,.nice-select-dropdown li.option{height:24px!important;min-height:24px!important;line-height:24px!important}.nice-select.small ul li:nth-of-type(2){clear:both}.nav{margin-bottom:0;padding-left:10;list-style:none}.nav>li{position:relative;display:block}.nav>li>a{position:relative;display:block;padding:5px 10px}.nav>li>a:hover,.nav>li>a:focus{text-decoration:none;background-color:#eee}.nav>li.disabled>a{color:#999}.nav>li.disabled>a:hover,.nav>li.disabled>a:focus{color:#999;text-decoration:none;background-color:initial;cursor:not-allowed}.nav .open>a,.nav .open>a:hover,.nav .open>a:focus{background-color:#eee;border-color:#428bca}.nav .nav-divider{height:1px;margin:9px 0;overflow:hidden;background-color:#e5e5e5}.nav>li>a>img{max-width:none}.nav-tabs{border-bottom:1px solid #ddd}.nav-tabs>li{float:left;margin-bottom:-1px}.nav-tabs>li>a{margin-right:0;line-height:1.42857143;border:1px solid #ddd;border-radius:10px 10px 0 0}.nav-tabs>li>a:hover{border-color:#eee #eee #ddd}.nav-tabs>button{margin-right:0;line-height:1.42857143;border:2px solid #ddd;border-radius:10px 10px 0 0}.nav-tabs>button:hover{background-color:#25bbfc;border-color:#428bca;color:#eaf2f9;border-bottom-color:transparent;}.nav-tabs>button.active,.nav-tabs>button.active:hover,.nav-tabs>button.active:focus{color:#f7fdfd;background-color:#1aae0d;border:1px solid #ddd;border-bottom-color:transparent;cursor:default}.nav-tabs>li.active>a,.nav-tabs>li.active>a:hover,.nav-tabs>li.active>a:focus{color:#428bca;background-color:#e5e5e5;border:1px solid #ddd;border-bottom-color:transparent;cursor:default}.nav-tabs.nav-justified{width:100%;border-bottom:0}.nav-tabs.nav-justified>li{float:none}.nav-tabs.nav-justified>li>a{text-align:center;margin-bottom:5px}.nav-tabs.nav-justified>.dropdown .dropdown-menu{top:auto;left:auto}.nav-status{float:left;margin:0;padding:3px;width:160px;font-weight:400;min-height:600}#bar,#prgbar {background-color: #f1f1f1;border-radius: 14px}#prgbar{width:100%}#bar {background-color: #3498db;width: 0%;height: 14px}.switch{position:relative;display:inline-block;width:34px;height:16px}.switch input{opacity:0;width:0;height:0}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#f55959;-webkit-transition:.4s;transition:.4s}.slider:before{position:absolute;content:\"\";height:12px;width:12px;left:2px;bottom:2px;background-color:#fff;-webkit-transition:.4s;transition:.4s}input:checked+.slider{background-color:#5ca30a}input:focus+.slider{box-shadow:0 0 1px #5ca30a}input:checked+.slider:before{-webkit-transform:translateX(16px);-ms-transform:translateX(16px);transform:translateX(16px)}.slider.round{border-radius:34px}.slider.round:before{border-radius:50%}:root{--dash-bg:#f4f6f9;--dash-card:#fff;--dash-fg:#1c2530;--dash-muted:#5b6b7c;--dash-border:#e1e6ec;--dash-accent:#2194ec;--dash-accent-strong:#1972b8;--dash-ok:#1aae0d;--dash-ok-bg:#eafcf0;--dash-warn:#b8860a;--dash-warn-bg:#fff6e0;--dash-bad:#d6392f;--dash-bad-bg:#fdeaea;--dash-off:#98a2ad;--dash-off-bg:#f1f3f5;--dash-shadow:0 1px 3px rgba(20,30,40,.08)}@media (prefers-color-scheme:dark){:root{--dash-bg:#161a20;--dash-card:#1e242c;--dash-fg:#e7ebef;--dash-muted:#8b98a5;--dash-border:#2a313a;--dash-accent-strong:#124a80;--dash-ok-bg:#123322;--dash-warn:#e0a935;--dash-warn-bg:#332a12;--dash-bad:#e2685f;--dash-bad-bg:#3a1f1d;--dash-off:#6b7684;--dash-off-bg:#242a31;--dash-shadow:0 1px 3px rgba(0,0,0,.4)}}body{background:var(--dash-bg);color:var(--dash-fg)}.header,.footer{background:var(--dash-accent-strong)}.dash{text-align:left;background:var(--dash-bg);padding:16px;border-radius:12px;font-size:14px}.dash-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px;margin:0 0 16px}.dash-card{display:flex;align-items:center;gap:12px;background:var(--dash-card);border:1px solid var(--dash-border);border-radius:10px;padding:12px 14px;box-shadow:var(--dash-shadow);margin:0}.dash-card-icon{flex:0 0 auto;width:28px;height:28px;color:var(--dash-accent)}.dash-card-icon svg{width:100%;height:100%}.dash-card-body{display:flex;flex-direction:column;min-width:0}.dash-card-label{font-size:11px;letter-spacing:.03em;text-transform:uppercase;color:var(--dash-muted)}.dash-card-value{font-size:15px;font-weight:600;color:var(--dash-fg);word-break:break-word}.dash-card.ok{background:var(--dash-ok-bg)}.dash-card.ok .dash-card-icon,.dash-card.ok .dash-card-value{color:var(--dash-ok)}.dash-card.warn{background:var(--dash-warn-bg)}.dash-card.warn .dash-card-icon,.dash-card.warn .dash-card-value{color:var(--dash-warn)}.dash-card.bad{background:var(--dash-bad-bg)}.dash-card.bad .dash-card-icon,.dash-card.bad .dash-card-value{color:var(--dash-bad)}.dash-card.off{background:var(--dash-off-bg)}.dash-card.off .dash-card-icon,.dash-card.off .dash-card-value{color:var(--dash-off)}.dash-section-title{font-size:12px;font-weight:700;letter-spacing:.04em;text-transform:uppercase;color:var(--dash-muted);margin:20px 0 8px}.dash-section-title:first-child{margin-top:0}.dash table{background:var(--dash-card);border:1px solid var(--dash-border);border-radius:10px;overflow:hidden;box-shadow:var(--dash-shadow);margin:0 0 16px;color:var(--dash-fg)}.dash table th{background:var(--dash-accent);color:#fff}.dash table td{background:var(--dash-card);color:var(--dash-fg);border-color:var(--dash-border)}.dash table tr:nth-child(even) td{background:var(--dash-bg)}#lastHeard{overflow-x:auto;width:100%}.dash-panel{background:var(--dash-card);border:1px solid var(--dash-border);border-radius:10px;padding:4px 20px;box-shadow:var(--dash-shadow);margin:0 0 16px}.dash-field{display:flex;flex-wrap:wrap;align-items:center;gap:4px 16px;padding:10px 0;border-bottom:1px solid var(--dash-border)}.dash-field:last-child{border-bottom:none}.dash-field>label:first-child{flex:0 0 170px;font-size:13px;font-weight:600;color:var(--dash-muted)}.dash-field-body{flex:1 1 260px;display:flex;align-items:center;gap:6px 10px;flex-wrap:wrap;color:var(--dash-fg)}.dash-field-body label{display:inline-flex;align-items:center;gap:4px;font-weight:400;color:var(--dash-fg)}.dash-field-body input[type=text],.dash-field-body input[type=number],.dash-field-body select{box-sizing:content-box;background:var(--dash-bg);border:1px solid var(--dash-border);border-radius:8px;padding:6px 10px;font-size:14px;color:var(--dash-fg)}.dash-field-body img{width:24px;height:24px;object-fit:contain;border-radius:4px;background:var(--dash-bg);border-color:var(--dash-border)}.dash-hint{font-size:11px;color:var(--dash-muted);flex-basis:100%}.dash-checkbox-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:8px 16px;padding:10px 2px}.dash-checkbox-grid label{display:flex;align-items:center;gap:6px;font-size:13px;color:var(--dash-fg)}.dash-form-actions{margin:0 0 24px}.dash-form-actions button,.dash-form-actions input[type=submit],.dash-field-body button{background:var(--dash-accent);color:#fff;border:none;border-radius:8px;padding:9px 18px;font-size:14px;font-weight:600;cursor:pointer}.dash-form-actions button:hover,.dash-form-actions input[type=submit]:hover,.dash-field-body button:hover{background:var(--dash-accent-strong)}fieldset.dash-filter-grp{border:1px solid var(--dash-border);border-radius:8px;padding:4px 16px 12px;margin:0 0 16px}fieldset.dash-filter-grp legend{padding:0 6px;color:var(--dash-muted);font-size:12px;text-transform:uppercase;font-weight:600}fieldset.dash-filter-grp:disabled{opacity:.5}.dash-card-action{margin-top:4px;background:var(--dash-accent);color:#fff;border:none;border-radius:6px;padding:4px 10px;font-size:12px;font-weight:600;cursor:pointer}.dash-card-action:hover{background:var(--dash-accent-strong)}.dash-card-action:disabled{opacity:.6;cursor:default}\n";
	server.send_P(200,"text/css",css);
}

void handle_dashboard()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	String strWiFiMode = "OFF";
	if (config.wifi_mode == WIFI_STA_FIX)
		strWiFiMode = "STA";
	else if (config.wifi_mode == WIFI_AP_FIX)
		strWiFiMode = "AP";
	else if (config.wifi_mode == WIFI_AP_STA_FIX)
		strWiFiMode = "AP+STA";

	String btMode = "NONE";
	if (config.bt_mode == 1)
		btMode = "TNC2";
	else if (config.bt_mode == 2)
		btMode = "KISS";

	String powerSource = "BATTERY";
	const char *powerClass = "off";
	if (powerCharging)
	{
		powerSource = "CHARGING";
		powerClass = "ok";
	}
	else if (powerVbusIn)
	{
		powerSource = "USB";
		powerClass = "ok";
	}

	// The two hx-get panels below replace the old loadInto()/setTimeout polling
	// script - htmx's own "load, every 10s" trigger does the same 10s refresh
	// declaratively, with no per-page JS to maintain.
	webString = "<div class=\"dash\">\n";
	webString += "<div id=\"lastHeard\" hx-get=\"/lastHeard\" hx-trigger=\"load, every 10s\" hx-swap=\"innerHTML\"></div>\n";
	webString += "<div id=\"sidebarInfo\" hx-get=\"/sidebarInfo\" hx-trigger=\"load, every 10s\" hx-swap=\"innerHTML\"></div>\n";

	webString += "<div class=\"dash-section-title\">Radio</div>\n";
	webString += "<div class=\"dash-grid\">\n";
	webString += dashCard(ICON_RADIO, "FREQ TX", String(config.freq_tx, 4) + " MHz");
	webString += dashCard(ICON_RADIO, "FREQ RX", String(config.freq_rx, 4) + " MHz");
	webString += dashCard(ICON_RADIO, "POWER", config.rf_power ? "HIGH" : "LOW", config.rf_power ? "ok" : "");
	webString += dashCard(ICON_SERVER, "APRS HOST", String(config.aprs_host) + ":" + String(config.aprs_port));
	webString += "</div>\n";

	webString += "<div class=\"dash-section-title\">Connectivity</div>\n";
	webString += "<div class=\"dash-grid\">\n";
	webString += dashCard(ICON_WIFI, "WIFI " + strWiFiMode, WiFi.isConnected() ? String(WiFi.SSID()) : "Disconnected", WiFi.isConnected() ? "ok" : "off");
	webString += dashCard(ICON_WIFI, "RSSI", WiFi.isConnected() ? String(WiFi.RSSI()) + " dBm" : "-", WiFi.isConnected() ? "" : "off");
	webString += dashCard(ICON_BLUETOOTH, "BLUETOOTH " + String(config.bt_name), config.bt_master ? btMode : "Disabled", config.bt_master ? "ok" : "off");
	webString += "</div>\n";

	webString += "<div class=\"dash-section-title\">Power</div>\n";
	webString += "<div class=\"dash-grid\">\n";
	webString += dashCard(ICON_BATTERY, "VOLTAGE", String(vbat, 2) + " V");
	webString += dashCard(ICON_BATTERY, "CHARGE", battPercent >= 0 ? String(battPercent) + " %" : "N/A");
	webString += dashCard(ICON_BATTERY, "SOURCE", powerSource, powerClass);
	webString += "</div>\n";

	webString += "</div>\n"; // .dash

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
	// Pure elapsed-seconds duration math, not TimeLib's day()/hour()/minute() -
	// those extract calendar-of-epoch fields from an absolute timestamp, which
	// only looked right here by coincidence (epoch day 1 = Jan 1st) for the
	// first ~31 days of uptime, then wrapped back down once the device had
	// been running longer than that - bad for hardware meant to run unattended
	// for months (see FORK_NOTES.md).
	// systemUptime stays 0 until the first successful NTP/GPS time sync
	// (main.cpp) - showing "now() - 0" before that gives a ~56-year reading
	// instead of a real duration (Gitea issue #50).
	String uptime;
	if (systemUptime == 0)
	{
		uptime = "N/A (no time sync yet)";
	}
	else
	{
		time_t uptimeSec = now() - systemUptime;
		uint32_t upDays = uptimeSec / 86400;
		uint32_t upHours = (uptimeSec % 86400) / 3600;
		uint32_t upMinutes = (uptimeSec % 3600) / 60;
		uptime = String(upDays) + "d " + String(upHours) + "h " + String(upMinutes) + "m";
	}
	bool sdPresent = SD.cardType() != CARD_NONE;
	uint32_t cardTotal = SD.totalBytes() / (1024 * 1024);
	uint32_t cardUsed = SD.usedBytes() / (1024 * 1024);

	xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
	bool aprsIsConnected = aprsClient.connected();
	xSemaphoreGive(aprsClientMutex);
	bool vpnActive = wireguard_active();

	String html = "<div class=\"dash-section-title\">System</div>\n";
	html += "<div class=\"dash-grid\">\n";
	html += dashCard(ICON_CLOCK, "UPTIME", uptime, systemUptime == 0 ? "off" : "");
	html += dashCard(ICON_CPU, "RAM", String((int)(ESP.getFreeHeap() / 1000)) + "/" + String((int)(ESP.getHeapSize() / 1000)) + " KB");
	html += dashCard(ICON_CPU, "PSRAM", String((int)(ESP.getFreePsram() / 1000)) + "/" + String((int)(ESP.getPsramSize() / 1000)) + " KB");
	html += "</div>\n";

	html += "<div class=\"dash-section-title\">Modes &amp; Status</div>\n";
	html += "<div class=\"dash-grid\">\n";
	html += dashCard(config.igate_en ? ICON_TOGGLE : ICON_TOGGLE_OFF, "IGATE", config.igate_en ? "Enabled" : "Disabled", config.igate_en ? "ok" : "off");
	html += dashCard(config.digi_en ? ICON_TOGGLE : ICON_TOGGLE_OFF, "DIGI", config.digi_en ? "Enabled" : "Disabled", config.digi_en ? "ok" : "off");
	// SmartTracker (issue #31): shows the *toggle* state only - "Smart
	// Enabled" here does not yet mean it's actually auto-transmitting right
	// now (the WiFi-loss auto-trigger isn't implemented), just that both
	// switches are on.
	String trackerState = "Disabled";
	const char *trackerClass = "off";
	if (config.trk_en)
	{
		trackerState = config.trk_smarttracker ? "Smart Enabled" : "Enabled";
		trackerClass = "ok";
	}
	// Custom markup (not dashCard()) - needs to embed the "Send Beacon Now"
	// button inside the card body (issue #33: moved here from its own
	// Dashboard button so it sits next to the state it acts on).
	html += "<article class=\"dash-card";
	if (trackerClass[0])
	{
		html += " ";
		html += trackerClass;
	}
	html += "\"><div class=\"dash-card-icon\">";
	html += config.trk_en ? ICON_TOGGLE : ICON_TOGGLE_OFF;
	html += "</div><div class=\"dash-card-body\"><span class=\"dash-card-label\">TRACKER</span><span class=\"dash-card-value\">";
	html += trackerState;
	html += "</span>";
	if (config.trk_en)
	{
		html += "<button class=\"dash-card-action\" onclick=\"fetch('/trackerSendBeacon').then(r=>r.text()).then(t=>{if(t!=='OK')alert(t);}).catch(e=>alert('Error: '+e));\">Send Beacon Now</button>";
	}
	html += "</div></article>\n";
	html += dashCard(ICON_GLOBE, "APRS-IS", aprsIsConnected ? "Connected" : "Disconnected", aprsIsConnected ? "ok" : "off");
	html += dashCard(ICON_GLOBE, "VPN", vpnActive ? "Connected" : "Disconnected", vpnActive ? "ok" : "off");
	html += dashCard(ICON_DRIVE, "SD CARD", sdPresent ? String(cardUsed) + "/" + String(cardTotal) + " MB" : "Not available", sdPresent ? "" : "off");
	html += "</div>\n";

	html += "<div class=\"dash-section-title\">Statistics</div>\n";
	html += "<div class=\"dash-grid\">\n";
	html += dashCard(ICON_CHART, "PACKET RX", String(status.rxCount));
	html += dashCard(ICON_CHART, "PACKET TX", String(status.txCount));
	html += dashCard(ICON_CHART, "RF &gt; INET", String(status.rf2inet));
	html += dashCard(ICON_CHART, "INET &gt; RF", String(status.inet2rf));
	html += dashCard(ICON_CHART, "DIGI", String(status.digiCount));
	html += dashCard(ICON_CHART, "DROP/ERR", String(status.dropCount) + "/" + String(status.errorCount), (status.dropCount + status.errorCount) > 0 ? "off" : "");
	html += "</div>\n";

	html += "<div class=\"dash-section-title\">GPS</div>\n";
	html += "<div class=\"dash-grid\">\n";
	html += dashCard(ICON_PIN, "LAT", String(gps.location.lat(), 5));
	html += dashCard(ICON_PIN, "LON", String(gps.location.lng(), 5));
	html += dashCard(ICON_PIN, "ALT", String(gps.altitude.meters(), 1) + " m");
	html += dashCard(ICON_SATELLITE, "SATELLITES", String(gps.satellites.value()), gps.satellites.value() > 0 ? "ok" : "off");
	// HDOP color tiers: <=2 ideal/excellent, <=5 good (still fine for APRS),
	// <=10 moderate, >10 fair/poor - matches this project's own "usable fix"
	// threshold of hdop<10 used elsewhere (main.cpp/beacon_builder.cpp).
	double hdop = gps.hdop.hdop();
	const char *hdopClass;
	if (!gps.location.isValid())
		hdopClass = "off";
	else if (hdop <= 5.0)
		hdopClass = "ok";
	else if (hdop <= 10.0)
		hdopClass = "warn";
	else
		hdopClass = "bad";
	html += dashCard(ICON_SATELLITE, "HDOP", gps.location.isValid() ? String(hdop, 1) : "N/A", hdopClass);
	html += "</div>\n";

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
	String content = restoreUploadBuf;
	restoreUploadBuf.clear();

	uint16_t applied = 0, unknown = 0, invalid = 0;
	String formatName;

	// Accept either the /configBackup-downloaded key=value text format
	// (always starts with a "#" comment line, see buildConfigBackup()) or
	// a raw /config.json file copied straight off a device - detected by
	// the JSON doc always starting with '{' (configToJsonDoc() always
	// writes an object), which the text format never does.
	String trimmed = content;
	trimmed.trim();
	if (trimmed.startsWith("{"))
	{
		formatName = "JSON";
		JsonDocument doc;
		DeserializationError error = deserializeJson(doc, content);
		if (error)
		{
			server.sendHeader("Connection", "close");
			server.send(400, "text/plain", String("ERROR: invalid JSON (") + error.c_str() + ") - config NOT changed");
			return;
		}
		jsonDocToConfig(doc, &applied, &invalid);
		// Unlike the text format's explicit per-line key scan, a JSON doc
		// never needs an "unknown key" counting pass - unrecognized keys
		// are simply never looked at (see config_json.cpp), so `unknown`
		// stays 0 here, not because nothing was skipped but because
		// nothing needs tracking for it.
	}
	else
	{
		formatName = "text";
		applyConfigBackup(content, &applied, &unknown, &invalid);
	}

	saveEEPROM();
	server.sendHeader("Connection", "close");
	// Surface the counts here, not just in the syslog - a restore that
	// skipped fields (unknown key, or a value that didn't look like a
	// number for a numeric field) should be visibly different from a
	// clean one, right in the browser response.
	String result = "OK (" + formatName + "): " + String(applied) + " applied, " + String(unknown) +
					 " unknown, " + String(invalid) + " invalid (kept previous value)";
	server.send(200, "text/plain", result);
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
	server.on("/trackerSendBeacon", handle_trackerSendBeacon);
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
	server.on("/htmx.min.js", handle_htmx_js);
	server.on("/console", handle_console);
	server.on("/consoleLog", handle_consoleLog);
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
				Serial.printf("Firmware Update FILE: %s\r\n", upload.filename.c_str());
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
