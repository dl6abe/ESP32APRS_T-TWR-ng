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

#define PATH_LEN 17
const char PATH_NAME[PATH_LEN][15] = {"OFF", "DST-TRACE 1", "DST-TRACE 2", "DST-TRACE 3", "DST-TRACE 4", "TRACE1-1", "TRACE2-2", "TRACE3-3", "WIDE1-1","RFONLY","RELAY","GATE","ECHO","UserDefine 1","UserDefine 2","UserDefine 3","UserDefine 4"};

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
void handle_wireless();
void handle_realtime();
void handle_about();

#endif
