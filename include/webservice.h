/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

#include "main.h"
#ifndef WEBSERVICE_H
#define WEBSERVICE_H

#include <Update.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <time.h>
#include <TimeLib.h>
#include <TinyGPSPlus.h>
#include "sa868.h"

String escapeHtml(const String &input); // web_symbols.cpp - escape before concatenating any user/RF-sourced string into HTML

// "DST-TRACE 1-4" (-> literal "DST1".."DST4") and "ECHO" removed 2026-09-29 -
// not documented anywhere as real APRS digipeater aliases (checked against
// the APRS spec's New-N-Paradigm addendum and community references); no
// digipeater would ever act on them, so packets sent with these "paths"
// were only ever heard directly. Added "WIDE1-1,WIDE2-1" as a real preset -
// previously the actually-recommended default path only existed as
// UserDefine-2's factory-default *text*, not as a selectable system entry
// (see Gitea issue - Path Definition discussion).
//
// Indices are stored directly in config.igate_path/digi_path/trk_path
// (uint8_t), so removing/reordering entries shifts what an already-saved
// index points to - reselect the Path dropdown on iGate/DIGI/Tracker once
// after upgrading. Not worth a migration shim for a single hand-flashed
// unit; revisit if this ever needs to survive an OTA update to deployed
// hardware.
#define PATH_LEN 13
#define PATH_DEFAULT_FIXED 4  // WIDE1-1 - recommended default for iGate/DIGI (fixed infrastructure, single hop)
#define PATH_DEFAULT_MOBILE 5 // WIDE1-1,WIDE2-1 - recommended default for Tracker (mobile, New-N-Paradigm)
const char PATH_NAME[PATH_LEN][20] = {"OFF", "TRACE1-1", "TRACE2-2", "TRACE3-3", "WIDE1-1", "WIDE1-1,WIDE2-1", "RFONLY", "RELAY", "GATE", "UserDefine 1", "UserDefine 2", "UserDefine 3", "UserDefine 4"};

// ใช้ตัวแปรโกลบอลในไฟล์ main.cpp
extern SA868 sa868;
extern statusType status;
extern digiTLMType digiTLM;
extern Configuration config;
extern TaskHandle_t taskNetworkHandle;
extern TaskHandle_t taskAPRSHandle;
extern TaskHandle_t taskTNCHandle;
extern TaskHandle_t taskGpsHandle;
extern time_t systemUptime;
extern String RF_VERSION;
extern pkgListType *pkgList;
extern TinyGPSPlus gps;
extern float vbat;
extern int battPercent;
extern bool powerCharging;
extern bool powerVbusIn;
extern WiFiClient aprsClient;
extern bool initInterval;

// #ifdef __cplusplus
// extern "C"
// {
// #endif
// 	uint8_t temprature_sens_read();
// #ifdef __cplusplus
// }
// #endif
// uint8_t temprature_sens_read();

void serviceHandle();
void setHTML(byte page);
void handle_root();
void handle_setting();
void handle_service();
void handle_system();
void handle_firmware();
void handle_default();
void handle_configBackup();  // GET  - downloads current config as key=value text
void handle_configRestore(); // POST - completion callback for the /configRestore upload (see webService())
#ifdef SDCRAD
void handle_storage();
void handle_download();
void handle_delete();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
#endif
void webService();
void handle_radio();
extern void RF_MODULE(bool boot);

// webservice.cpp's shared server object and page-build buffer - needed by
// the handlers below, now split into their own translation units
// (web_symbols.cpp/web_files.cpp/web_radio_vpn.cpp/web_system_wireless.cpp/
// web_igate.cpp/web_digi.cpp/web_tracker.cpp/web_misc.cpp).
extern WebServer server;
extern String webString;
extern bool defaultSetting;

// Handlers defined in those split files (handle_system/handle_radio/
// handle_default already declared above).
void handle_symbol_icon();
void handle_symbol();
void handle_lastHeard();
void handle_storage();
void handle_download();
void handle_delete();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void handle_vpn();
void handle_igate();
void handle_digi();
void handle_tracker();
void handle_trackerSendBeacon(); // web_tracker.cpp - manual "Send Beacon Now" button on the Dashboard
void handle_wireless();
void handle_realtime();
void handle_about();
void handle_htmx_js(); // web_assets.cpp - vendored htmx.min.js, served standalone (no internet needed)
void handle_console();    // web_console.cpp - Console tab page + start/stop toggle
void handle_consoleLog(); // web_console.cpp - GET: raw text/plain snapshot of the recording buffer

#endif
