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

#define DEBUG_TNC

#define EEPROM_SIZE 2048

#include <Wire.h>
#include "Adafruit_SSD1306.h"
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include "sa868.h"


extern ParseAPRS aprsParse; // used by APRSConnect() for passCode(); defined in main.cpp, not in a shared header

// Developer-identifying APRS status report, sent once per APRS-IS (re)connect
// - APRS-IS only, never RF (see APRSConnect()). Adapted from
// richonguzman/LoRa_APRS_Tracker's device-info status message, with the
// doubled "iGate (igate)" text collapsed to one mention. DL6ABE is the
// developer of this fork, not the operator's configured callsign - fixed,
// not derived from config, same as Ricardo's own message credits himself
// regardless of who runs his firmware.
void sendDeviceInfo()
{
  xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
  if (!aprsClient.connected())
  {
    xSemaphoreGive(aprsClientMutex);
    return;
  }
  String status = "DL6ABE>" APRS_TOCALL ":>Device: DL6ABE: ESP32APRS T-TWR+ iGate/Digi/Tracker V" + String(VERSION) + String(VERSION_BUILD);
  size_t wrote = aprsClient.println(status);
  IPAddress remoteIp = aprsClient.remoteIP();
  xSemaphoreGive(aprsClientMutex);
  projLog(LOGCAT_APRS_INET, "Sent device info (%d/%d bytes, remote %s): %s", (int)wrote, status.length() + 2, remoteIp.toString().c_str(), status.c_str());
}

// Battery/power telemetry, sent once per APRS-IS (re)connect - APRS-IS only,
// never RF. Standard plain-text APRS telemetry (APRS101), not Ricardo's
// RF-oriented compressed encoding - this project has no RF-bandwidth reason
// to use that, and plain format is what aprs.fi/telemetry tools expect
// natively. Uses config.aprs_mycall (this is per-operator device state, not
// a developer credit like sendDeviceInfo()).
uint16_t deviceTlmSeq = 0;
void sendDeviceTelemetry()
{
  if (strcmp("NOCALL", config.aprs_mycall) == 0)
    return;

  xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
  if (!aprsClient.connected())
  {
    xSemaphoreGive(aprsClientMutex);
    return;
  }

  // Full on-air station identity (matches APRSConnect()'s login and every
  // other beacon this device sends) - not just config.aprs_mycall, which
  // omits the SSID. Telemetry sent under the bare root call was landing on
  // a different aprs.fi station page than the one this device actually
  // beacons as (found 2026-09-27: a user checking aprs.fi for their own
  // iGate's SSID saw no telemetry, because it was posted under the bare
  // callsign instead).
  String myStation = String(config.aprs_mycall);
  if (config.aprs_ssid != 0)
    myStation += "-" + String(config.aprs_ssid);

  String addressee = myStation;
  while (addressee.length() < 9)
    addressee += " ";

  // Definition messages: channel 1 = voltage (real = val*0.005 + 3.0V,
  // i.e. val = (voltage - 3.0) * 200, clamped to a 0-255 byte), channel 2 =
  // battery percent (0-100, sent as-is). Channels 3-5 unused. Digital bit 1
  // = charging, bit 2 = USB/external power present.
  aprsClient.println(myStation + ">" APRS_TOCALL "::" + addressee + ":PARM.Voltage,Percent");
  aprsClient.println(myStation + ">" APRS_TOCALL "::" + addressee + ":UNIT.V,%");
  aprsClient.println(myStation + ">" APRS_TOCALL "::" + addressee + ":EQNS.0,0.005,3.0,0,1,0");

  int a1 = (int)((vbat - 3.0) * 200);
  if (a1 < 0)
    a1 = 0;
  if (a1 > 255)
    a1 = 255;
  int a2 = (battPercent >= 0) ? battPercent : 0;
  if (a2 > 255)
    a2 = 255;

  char bits[9];
  snprintf(bits, sizeof(bits), "%d%d000000", powerCharging ? 1 : 0, powerVbusIn ? 1 : 0);

  char seqStr[4];
  snprintf(seqStr, sizeof(seqStr), "%03d", deviceTlmSeq % 1000);
  deviceTlmSeq++;

  String tlm = myStation + ">" APRS_TOCALL ":T#" + seqStr + "," + String(a1) + "," + String(a2) + ",0,0,0," + String(bits);
  size_t wrote = aprsClient.println(tlm);
  xSemaphoreGive(aprsClientMutex);
  projLog(LOGCAT_APRS_INET, "Sent telemetry (%d/%d bytes): %s", (int)wrote, tlm.length() + 2, tlm.c_str());
}

boolean APRSConnect()
{
  // Serial.println("Connect TCP Server");
  String login = "";
  int cnt = 0;
  xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
  uint8_t con = aprsClient.connected();
  // Serial.println(con);
  if (con <= 0)
  {
    if (!aprsClient.connect(config.aprs_host, config.aprs_port)) // เชื่อมต่อกับเซิร์ฟเวอร์ TCP
    {
      // Serial.print(".");
      delay(100);
      cnt++;
      if (cnt > 50) // วนร้องขอการเชื่อมต่อ 50 ครั้ง ถ้าไม่ได้ให้รีเทิร์นฟังค์ชั่นเป็น False
      {
        xSemaphoreGive(aprsClientMutex);
        return false;
      }
    }

    if (strcmp("NOCALL", config.aprs_mycall) == 0)
    {
      config.igate_en = false;
      xSemaphoreGive(aprsClientMutex);
      return false;
    }

    // ขอเชื่อมต่อกับ aprsc
    if (strlen(config.igate_object) >= 3)
    {
      uint16_t passcode = aprsParse.passCode(config.igate_object);
      login = "user " + String(config.igate_object) + " pass " + String(passcode, DEC) + " vers ESP32APRS T-TWR+ " + String(VERSION) + String(VERSION_BUILD) + " filter " + String(config.aprs_filter);
    }
    else
    {
      uint16_t passcode = aprsParse.passCode(config.aprs_mycall);
      if (config.aprs_ssid == 0)
        login = "user " + String(config.aprs_mycall) + " pass " + String(passcode, DEC) + " vers ESP32APRS T-TWR+ " + String(VERSION) + String(VERSION_BUILD) + " filter " + String(config.aprs_filter);
      else
        login = "user " + String(config.aprs_mycall) + "-" + String(config.aprs_ssid) + " pass " + String(passcode, DEC) + " vers ESP32APRS T-TWR+ " + String(VERSION) + String(VERSION_BUILD) + " filter " + String(config.aprs_filter);
    }
    size_t wrote = aprsClient.println(login);
    IPAddress remoteIp = aprsClient.remoteIP();
    xSemaphoreGive(aprsClientMutex);
    projLog(LOGCAT_APRS_INET, "APRS-IS login (%d/%d bytes, remote %s): %s", (int)wrote, login.length() + 2, remoteIp.toString().c_str(), login.c_str());
    delay(500);
    return true;
  }
  xSemaphoreGive(aprsClientMutex);
  return true;
}

String sendIsAckMsg(String toCallSign, char *msgId)
{
  char str[300];
  char call[11];
  int i;
  memset(&call[0], 0, 11);
  strncpy(&call[0], toCallSign.c_str(), sizeof(call) - 1);
  i = strlen(call);
  for (; i < 9; i++)
    call[i] = 0x20;
  memset(&str[0], 0, 300);

  String path = getPath(config.igate_path); // config.igate_path is a numeric index, not a string - must go through getPath() before use in "%s"
  if (config.aprs_ssid > 0)
    snprintf(str, sizeof(str), "%s-%d>" APRS_TOCALL ",%s::%s:ack%s", config.aprs_mycall, config.aprs_ssid, path.c_str(), call, msgId);
  else
    snprintf(str, sizeof(str), "%s>" APRS_TOCALL ",%s::%s:ack%s", config.aprs_mycall, path.c_str(), call, msgId);
  return String(str);
}

void sendIsPkg(char *raw)
{
  char str[500];
  sprintf(str, "%s-%d>" APRS_TOCALL "%s:%s", config.aprs_mycall, config.aprs_ssid, VERSION, raw);
  String tnc2Raw = String(str);
  xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
  if (aprsClient.connected())
    aprsClient.println(tnc2Raw); // Send packet to Inet
  xSemaphoreGive(aprsClientMutex);
  // if (config.rf_en && config.digi_en)
  if (config.digi_en)
    pkgTxPush(str, strlen(str), 0);
}

void sendIsPkgMsg(char *raw)
{
  char str[300];
  char call[16]; // headroom over the 9-char padded callsign field for "-" + a 3-digit SSID
  int i;
  memset(&call[0], 0, sizeof(call));
  if (config.aprs_ssid == 0)
    snprintf(call, sizeof(call), "%s", config.aprs_mycall);
  else
    snprintf(call, sizeof(call), "%s-%d", config.aprs_mycall, config.aprs_ssid);
  i = strlen(call);
  for (; i < 9; i++)
    call[i] = 0x20;

  if (config.aprs_ssid == 0)
    sprintf(str, "%s>" APRS_TOCALL "::%s:%s", config.aprs_mycall, call, raw);
  else
    sprintf(str, "%s-%d>" APRS_TOCALL "::%s:%s", config.aprs_mycall, config.aprs_ssid, call, raw);

  String tnc2Raw = String(str);
  xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
  if (aprsClient.connected())
    aprsClient.println(tnc2Raw); // Send packet to Inet
  xSemaphoreGive(aprsClientMutex);
  if (config.rf_en && config.digi_en)
    pkgTxPush(str, strlen(str), 0);
  // APRS_sendTNC2Pkt(tnc2Raw); // Send packet to RF
}

