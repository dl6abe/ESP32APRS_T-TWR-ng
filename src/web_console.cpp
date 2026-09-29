/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

// Web UI "Console" tab (Gitea issue #24) - lets a user copy debug output out
// of the browser instead of needing a serial cable. A third projLog() sink
// (see log_output.cpp), same category filter as Serial/syslog
// (config.logCategoryMask, System page's "Debug Logging" section) so there's
// no separate category UI here.
//
// Design decisions (2026-09-29, see Gitea issue #24):
// - Start/stop toggle, not always-on: the ring buffer is only allocated
//   while a user has explicitly turned recording on, so it costs no RAM the
//   rest of the time.
// - PSRAM, not internal heap: this board has BOARD_HAS_PSRAM
//   (platformio.ini) and the internal heap already has a 60KB-free reboot
//   safety threshold (main.cpp) - a 32KB debug buffer has no business
//   competing with that.
// - Manual refresh, not htmx auto-polling: unlike the Dashboard's hx-get
//   panels, replacing the whole textarea's content every few seconds would
//   reset the user's text selection while they're trying to copy something.
// - Copy via textarea select() + document.execCommand('copy'), not
//   navigator.clipboard.writeText(): this device serves plain HTTP (no TLS),
//   normally on a LAN IP, and the modern Clipboard API requires a secure
//   context - it silently does nothing on a plain-HTTP LAN address in
//   Chrome/Firefox. execCommand('copy') is deprecated but is the one
//   approach that actually works here.
#include "webservice.h"
#include "esp_heap_caps.h"

#define CONSOLE_LOG_BUF_SIZE (32 * 1024)

static char *consoleBuf = nullptr;
static size_t consoleBufSize = 0;
static size_t consoleBufUsed = 0;
static bool consoleActive = false;
static SemaphoreHandle_t consoleMutex = nullptr;

// Must be called once from setup(), before any task that logs is created -
// same requirement/reasoning as logInit() (log_output.cpp). Only creates the
// (tiny) mutex; the actual PSRAM buffer is allocated by consoleLogStart(),
// on demand.
void consoleLogInit()
{
	consoleMutex = xSemaphoreCreateMutex();
}

bool consoleLogActive()
{
	return consoleActive;
}

// Allocates the ring buffer from PSRAM and starts recording. Returns false
// (and leaves recording off) if the allocation failed, e.g. PSRAM already
// exhausted by something else.
static bool consoleLogStart()
{
	if (consoleActive)
		return true;

	xSemaphoreTake(consoleMutex, portMAX_DELAY);
	consoleBuf = (char *)heap_caps_malloc(CONSOLE_LOG_BUF_SIZE, MALLOC_CAP_SPIRAM);
	if (consoleBuf != nullptr)
	{
		consoleBufSize = CONSOLE_LOG_BUF_SIZE;
		consoleBufUsed = 0;
		consoleActive = true;
	}
	xSemaphoreGive(consoleMutex);

	return consoleActive;
}

// Frees the ring buffer - RAM is only held while a user is actually using
// this tab, per the design decision above.
static void consoleLogStop()
{
	xSemaphoreTake(consoleMutex, portMAX_DELAY);
	consoleActive = false;
	if (consoleBuf != nullptr)
	{
		free(consoleBuf);
		consoleBuf = nullptr;
	}
	consoleBufSize = 0;
	consoleBufUsed = 0;
	xSemaphoreGive(consoleMutex);
}

// Called from projLog() (log_output.cpp) for every category-enabled log
// line, same msg it also sends to Serial/syslog. catName/msg are already
// formatted by the caller; this only adds the "[CATEGORY] " prefix and
// trailing newline expected in the console's own output.
void consoleLogAppend(const char *catName, const char *msg)
{
	if (!consoleActive)
		return;

	char line[300]; // headroom over projLog()'s own 256-byte msg[] plus "[CATEGORY] " and '\n'
	int n = snprintf(line, sizeof(line), "[%s] %s\n", catName, msg);
	if (n <= 0)
		return;
	size_t len = (size_t)n;
	if (len >= sizeof(line))
		len = sizeof(line) - 1; // snprintf truncated - keep what fit

	xSemaphoreTake(consoleMutex, portMAX_DELAY);
	if (!consoleActive || consoleBuf == nullptr) // consoleLogStop() may have run while waiting for the lock
	{
		xSemaphoreGive(consoleMutex);
		return;
	}
	if (len > consoleBufSize)
	{
		xSemaphoreGive(consoleMutex); // single line bigger than the whole buffer - can't happen at 32KB, but don't corrupt memory if it ever does
		return;
	}
	if (consoleBufUsed + len > consoleBufSize)
	{
		// Evict the oldest whole lines until there's room - cut only at a
		// '\n' boundary so the buffer never starts mid-line.
		size_t need = (consoleBufUsed + len) - consoleBufSize;
		size_t cut = 0;
		while (cut < need && cut < consoleBufUsed)
		{
			char *nl = (char *)memchr(consoleBuf + cut, '\n', consoleBufUsed - cut);
			if (nl == nullptr)
			{
				cut = consoleBufUsed;
				break;
			}
			cut = (size_t)(nl - consoleBuf) + 1;
		}
		memmove(consoleBuf, consoleBuf + cut, consoleBufUsed - cut);
		consoleBufUsed -= cut;
	}
	memcpy(consoleBuf + consoleBufUsed, line, len);
	consoleBufUsed += len;
	xSemaphoreGive(consoleMutex);
}

// GET - raw snapshot of the current buffer as plain text, fetched by the
// Console tab's Refresh button and assigned straight to a textarea's
// .value (never innerHTML), so no HTML-escaping is needed here even though
// RF/APRS-sourced text can appear in some log lines.
void handle_consoleLog()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}

	String snapshot;
	if (consoleActive && consoleBuf != nullptr)
	{
		xSemaphoreTake(consoleMutex, portMAX_DELAY);
		snapshot.reserve(consoleBufUsed + 1);
		snapshot.concat(consoleBuf, consoleBufUsed);
		xSemaphoreGive(consoleMutex);
	}
	server.send(200, "text/plain", snapshot);
}

void handle_console()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}

	if (server.hasArg("commitConsoleLog"))
	{
		bool wantEnable = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "consoleLogEnable" && server.arg(i) == "OK")
				wantEnable = true;
		}

		String result = "OK";
		if (wantEnable)
		{
			if (!consoleLogStart())
				result = "FAIL: PSRAM allocation failed";
		}
		else
		{
			consoleLogStop();
		}
		server.send(200, "text/plain", result);
		return;
	}

	String html = "<div class=\"dash\">\n";
	html += "<div class=\"dash-section-title\">Debug Console</div>\n";
	html += "<div class=\"dash-panel\">\n";

	html += "<form id=\"formConsoleLog\" method=\"POST\" action=\"#\" enctype=\"multipart/form-data\">\n";
	html += "<div class=\"dash-field\"><label>Recording</label><div class=\"dash-field-body\">";
	html += "<label class=\"switch\"><input type=\"checkbox\" name=\"consoleLogEnable\" value=\"OK\"";
	html += consoleLogActive() ? " checked" : "";
	html += "><span class=\"slider round\"></span></label>";
	html += "<span class=\"dash-hint\">Uses the categories enabled under System &gt; Debug Logging. Recording stops and frees its buffer when turned off.</span>";
	html += "</div></div>\n";
	html += "<input type=\"hidden\" name=\"commitConsoleLog\"/>\n";
	html += "<div class=\"dash-form-actions\"><button type='submit' id='submitConsoleLog'>Apply Change</button></div>\n";
	html += "</form>\n";

	html += "<div class=\"dash-field\"><label>Output</label><div class=\"dash-field-body\" style=\"display:block\">";
	html += "<textarea id=\"consoleOutput\" readonly rows=\"20\" style=\"width:100%;box-sizing:border-box;font-family:monospace;font-size:12px;white-space:pre;\"></textarea>";
	html += "</div></div>\n";
	html += "</div>\n"; // .dash-panel

	html += "<div class=\"dash-form-actions\">";
	html += "<button type=\"button\" id=\"btnConsoleRefresh\">Refresh</button> ";
	html += "<button type=\"button\" id=\"btnConsoleCopy\">Copy</button>";
	html += "</div>\n";
	html += "</div>\n"; // .dash

	// Own inline <script>, re-executed by loadInto() (webservice.cpp) after
	// this fragment is injected into #contentmain - same pattern
	// handle_system() uses for its own forms, since this tab isn't loaded
	// as a full page and a plain <form action="#"> would otherwise submit
	// against the top-level page instead of this handler.
	html += "<script>\n";
	html += "document.getElementById('formConsoleLog').addEventListener('submit', function (e) {\n";
	html += "e.preventDefault();\n";
	html += "var data = new FormData(e.currentTarget);\n";
	html += "document.getElementById('submitConsoleLog').disabled = true;\n";
	html += "fetch('/console', { method: 'POST', body: data })\n";
	html += ".then(function (r) { return r.text(); })\n";
	html += ".then(function (t) { document.getElementById('submitConsoleLog').disabled = false; if (t !== 'OK') alert(t); })\n";
	html += ".catch(function () { document.getElementById('submitConsoleLog').disabled = false; alert('An error occurred.'); });\n";
	html += "});\n";
	html += "document.getElementById('btnConsoleRefresh').addEventListener('click', function () {\n";
	html += "fetch('/consoleLog').then(function (r) { return r.text(); }).then(function (t) {\n";
	html += "document.getElementById('consoleOutput').value = t;\n";
	html += "});\n";
	html += "});\n";
	html += "document.getElementById('btnConsoleCopy').addEventListener('click', function () {\n";
	html += "var ta = document.getElementById('consoleOutput');\n";
	html += "ta.focus();\n";
	html += "ta.select();\n";
	html += "ta.setSelectionRange(0, ta.value.length);\n";
	html += "document.execCommand('copy');\n";
	html += "});\n";
	html += "</script>\n";

	server.send(200, "text/html", html);
}
