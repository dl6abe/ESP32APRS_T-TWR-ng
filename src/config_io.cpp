/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

#include <Arduino.h>
#include "main.h"
#include <LibAPRSesp.h>
#include <limits.h>
#include <KISS.h>
#include "webservice.h"
#include <WiFiUdp.h>
#include "ESP32Ping.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include "XPowersLib.h"
#include "cppQueue.h"
#include "digirepeater.h"
#include "igate.h"
#include "wireguardif.h"
#include "wireguard.h"
#include "wireguard_vpn.h"

#include "time.h"

#include <TinyGPSPlus.h>
#include <pbuf.h>
#include <parse_aprs.h>

#include <WiFiUdp.h>

#include <WiFiClientSecure.h>

#include "AFSK.h"

#include "gui_lcd.h"
#include "config_json.h"

#define DEBUG_TNC

#define EEPROM_SIZE 2048

#include <Wire.h>
#include "Adafruit_SSD1306.h"
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include "sa868.h"


// Despite the name, writes /config.json on LittleFS (see include/main.h's
// declaration comment and config_json.h) - kept so the ~40 existing call
// sites across this codebase don't all need touching for this rename.
void saveEEPROM()
{
  if (!saveConfigJsonImpl())
  {
    projLog(LOGCAT_SYSTEM, "saveEEPROM(): failed to persist %s", CONFIG_JSON_PATH);
  }
}

void setConfigDefaults()
{
  projLog(LOGCAT_SYSTEM, "Default configure mode!");
  config.synctime = true;
  config.timeZone = 7; // dead, see posixTZ
  strcpy(config.posixTZ, "UTC0");
  config.tx_timeslot = 2000; // ms

  config.wifi_mode = WIFI_AP_STA_FIX;
  config.wifi_power = 44;
  config.wifi_ap_ch = 6;
  config.wifi_sta[0].enable = true;
  sprintf(config.wifi_sta[0].wifi_ssid, "APRSTH");
  sprintf(config.wifi_sta[0].wifi_pass, "aprsthnetwork");
  for (int i = 1; i < 5; i++)
  {
    config.wifi_sta[i].enable = false;
    config.wifi_sta[i].wifi_ssid[0] = 0;
    config.wifi_sta[i].wifi_pass[0] = 0;
  }
  sprintf(config.wifi_ap_ssid, "ESP32APRS");
  sprintf(config.wifi_ap_pass, "aprsthnetwork");
  // Blutooth
  config.bt_slave = false;
  config.bt_master = false;
  config.bt_mode = 1; // 0-None,1-TNC2RAW,2-KISS
  config.bt_power = 1;
  sprintf(config.bt_uuid, "6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
  sprintf(config.bt_uuid_rx, "6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
  sprintf(config.bt_uuid_tx, "6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
  sprintf(config.bt_name, "ESP32APRS");
  config.bt_pin = 0;

  //--RF Module
  config.rf_en = true;
  config.rf_type = RF_SA868_VHF;
  config.freq_rx = 144.3900;
  config.freq_tx = 144.3900;
  config.offset_rx = 0;
  config.offset_tx = 0;
  config.tone_rx = 0;
  config.tone_tx = 0;
  config.band = 0;
  config.sql_level = 1;
  config.rf_power = LOW;
  config.volume = 1;
  config.mic = 8;

  // IGATE
  config.igate_bcn = false;
  config.igate_en = false;
  config.rf2inet = true;
  config.inet2rf = false;
  config.igate_loc2rf = false;
  config.igate_loc2inet = true;
  config.rf2inetFilter = 0xFFF; // All
  config.inet2rfFilter = config.digiFilter = FILTER_OBJECT | FILTER_ITEM | FILTER_MESSAGE | FILTER_MICE | FILTER_POSITION | FILTER_WX;
  //--APRS-IS
  config.aprs_ssid = 1;
  config.aprs_port = 14580;
  sprintf(config.aprs_mycall, "NOCALL");
  sprintf(config.aprs_host, "aprs.dprns.com");
  config.aprs_passcode[0] = 0;
  snprintf(config.aprs_moniCall, sizeof(config.aprs_moniCall), "%s-%d", config.aprs_mycall, config.aprs_ssid);
  sprintf(config.aprs_filter, "m/10");
  //--Position
  config.igate_gps = true; // Location Source default: GPS, not Fix (2026-10-01)
  config.igate_lat = 13.7555;
  config.igate_lon = 100.4930;
  config.igate_alt = 0;
  config.igate_interval = 600;
  config.igate_timestamp = false;
  sprintf(config.igate_symbol, "/&");
  config.igate_object[0] = 0;
  config.igate_phg[0] = 0;
  config.igate_path = PATH_DEFAULT_FIXED; // WIDE1-1
  sprintf(config.igate_comment, "IGate MODE");

  // DIGI REPEATER
  config.digi_en = false;
  config.digi_loc2rf = true;
  config.digi_loc2inet = false;
  config.digi_ssid = 3;
  config.digi_timestamp = false;
  sprintf(config.digi_mycall, "NOCALL");
  config.digi_path = PATH_DEFAULT_FIXED; // WIDE1-1
  //--Position
  config.digi_gps = true; // Location Source default: GPS, not Fix (2026-10-01)
  config.digi_lat = 13.7555;
  config.digi_lon = 100.4930;
  config.digi_alt = 0;
  config.digi_interval = 600;
  config.digi_delay = 0;
  config.digiFilter = FILTER_OBJECT | FILTER_ITEM | FILTER_MESSAGE | FILTER_MICE | FILTER_POSITION | FILTER_WX;
  sprintf(config.digi_symbol, "/#");
  config.digi_phg[0] = 0;
  sprintf(config.digi_comment, "DIGI MODE");

  // Tracker
  config.trk_en = false;
  config.trk_loc2rf = true;
  config.trk_loc2inet = false;
  config.trk_bat = false;
  config.trk_sat = false;
  config.trk_dx = false;
  config.trk_ssid = 7;
  config.trk_timestamp = false;
  sprintf(config.trk_mycall, "NOCALL");
  config.trk_path = PATH_DEFAULT_MOBILE; // WIDE1-1,WIDE2-1

  //--Position
  config.trk_gps = true; // Location Source default: GPS, not Fix (2026-10-01)
  config.trk_lat = 13.7555;
  config.trk_lon = 100.4930;
  config.trk_alt = 0;
  config.trk_interval = 600;
  // SmartTracker (issue #31) - opt-in, off by default
  config.trk_smarttracker = false;
  // Smart beacon
  config.trk_smartbeacon = true;
  config.trk_compress = true;
  config.trk_altitude = true;
  config.trk_cst = true;
  config.trk_hspeed = 120;
  config.trk_lspeed = 5;
  config.trk_maxinterval = 30;
  config.trk_mininterval = 5;
  config.trk_minangle = 25;
  config.trk_slowinterval = 600;

  sprintf(config.trk_symbol, "/[");
  sprintf(config.trk_symmove, "/>");
  sprintf(config.trk_symstop, "\\>");
  sprintf(config.trk_mycall, "NOCALL");
  sprintf(config.trk_comment, "TRACKER MODE");
  config.trk_item[0] = 0;

  // OLED DISPLAY
  config.oled_enable = true;
  // 0 = never sleep (see gui_lcd.cpp's oled_timeout check). Was 60s -
  // on unattended hardware (repeaters/igates on towers, no one around to
  // touch the encoder and wake it back up via oledWake()) that meant the
  // display went dark a minute after every boot and stayed that way
  // forever, confusing enough on first encounter to report as "always
  // black" (2026-10-01).
  config.oled_timeout = 0;
  config.dim = 0; // "HI" - full brightness, no dimming (console help: 0=HI 1=LOW 2=AUTO 3=DAY/NIGHT 4=CONTRAST)
  config.contrast = 0;
  config.startup = 0;

  // Display
  config.dispDelay = 3; // Popup display 3 sec
  config.dispRF = true;
  config.dispINET = false;
  config.filterDistant = 0;
  config.dispFilter = FILTER_OBJECT | FILTER_ITEM | FILTER_MESSAGE | FILTER_MICE | FILTER_POSITION | FILTER_WX | FILTER_STATUS | FILTER_BUOY | FILTER_QUERY;
  config.h_up = true;
  config.tx_display = true;
  config.rx_display = true;
  config.audio_hpf = false;
  config.audio_bpf = false;
  config.preamble = 3;
  sprintf(config.ntp_host, "ntp.dprns.com");

  sprintf(config.path[0], "TRACE4-4");
  sprintf(config.path[1], "WIDE1-1,WIDE2-1");
  sprintf(config.path[2], "YBOX");
  sprintf(config.path[3], "RS0ISS");

  // VPN Wireguard
  config.vpn = false;
  config.wg_port = 51820;
  sprintf(config.wg_peer_address, "vpn.dprns.com");
  sprintf(config.wg_local_address, "192.168.1.2");
  sprintf(config.wg_netmask_address, "255.255.255.0");
  sprintf(config.wg_gw_address, "192.168.1.1");
  config.wg_public_key[0] = 0;
  config.wg_private_key[0] = 0;

  sprintf(config.http_username, "admin");
  sprintf(config.http_password, "admin");

  config.gpio_sql_pin = -1;

  // Console/syslog debug logging - SYSTEM on by default (boot/power/config
  // messages), the rest off until explicitly enabled on the System page.
  config.logCategoryMask = LOGCAT_SYSTEM;
  // Enabled by default (2026-10-01) - still inert without a host
  // (syslogReconnect() requires both syslog_en and a non-empty
  // syslog_host before it creates a client), so this just saves the
  // checkbox click once someone does set a host; no behavior change on
  // its own.
  config.syslog_en = true;
  config.syslog_host[0] = 0;
  config.syslog_port = 514;
}

void defaultConfig()
{
  setConfigDefaults();
  saveEEPROM();
}

unsigned long NTP_Timeout;
unsigned long pingTimeout;

bool psramBusy = false;

// const char *lastTitle = "LAST HEARD";

