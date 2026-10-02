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


int tlmList_Find(char *call)
{
  int i;
  // while (psramBusy) delay(1);
  // psramBusy = true;
  for (i = 0; i < TLMLISTSIZE; i++)
  {
    if (strstr(Telemetry[i].callsign, call) != NULL)
    {
      psramBusy = false;
      return i;
    }
  }
  // psramBusy = false;
  return -1;
}

int tlmListOld()
{
  // while (psramBusy) delay(1);
  // psramBusy = true;
  int i, ret = 0;
  time_t minimum = Telemetry[0].time;
  for (i = 1; i < TLMLISTSIZE; i++)
  {
    if (Telemetry[i].time < minimum)
    {
      minimum = Telemetry[i].time;
      ret = i;
    }
    if (Telemetry[i].time > time(NULL))
      Telemetry[i].time = 0;
  }
  // psramBusy = false;
  return ret;
}

TelemetryType getTlmList(int idx)
{
  TelemetryType ret;
  while (psramBusy)
    delay(1);
  psramBusy = true;
  memcpy(&ret, &Telemetry[idx], sizeof(TelemetryType));
  psramBusy = false;
  return ret;
}

int pkgList_Find(char *call)
{
  int i;
  for (i = 0; i < PKGLISTSIZE; i++)
  {
    if (strstr(pkgList[(int)i].calsign, call) != NULL)
      return i;
  }
  return -1;
}

int pkgList_Find(char *call, uint16_t type)
{
  int i;
  for (i = 0; i < PKGLISTSIZE; i++)
  {
    if ((strstr(pkgList[i].calsign, call) != NULL) && (pkgList[i].type == type))
      return i;
  }
  return -1;
}

int pkgListOld()
{
  int i, ret = -1;
  time_t minimum = time(NULL) + 86400; // pkgList[0].time;
  for (i = 0; i < PKGLISTSIZE; i++)
  {
    if (pkgList[(int)i].time < minimum)
    {
      minimum = pkgList[(int)i].time;
      ret = i;
    }
  }
  return ret;
}

void sort(pkgListType a[], int size)
{
  pkgListType t;
  char *ptr1;
  char *ptr2;
  char *ptr3;
  ptr1 = (char *)&t;
  while (psramBusy)
    delay(1);
  psramBusy = true;
  for (int i = 0; i < (size - 1); i++)
  {
    for (int o = 0; o < (size - (i + 1)); o++)
    {
      if (a[o].time < a[o + 1].time)
      {
        ptr2 = (char *)&a[o];
        ptr3 = (char *)&a[o + 1];
        memcpy(ptr1, ptr2, sizeof(pkgListType));
        memcpy(ptr2, ptr3, sizeof(pkgListType));
        memcpy(ptr3, ptr1, sizeof(pkgListType));
      }
    }
  }
  psramBusy = false;
}

void sortPkgDesc(pkgListType a[], int size)
{
  pkgListType t;
  char *ptr1;
  char *ptr2;
  char *ptr3;
  ptr1 = (char *)&t;
  while (psramBusy)
    delay(1);
  psramBusy = true;
  for (int i = 0; i < (size - 1); i++)
  {
    for (int o = 0; o < (size - (i + 1)); o++)
    {
      if (a[o].pkg < a[o + 1].pkg)
      {
        ptr2 = (char *)&a[o];
        ptr3 = (char *)&a[o + 1];
        memcpy(ptr1, ptr2, sizeof(pkgListType));
        memcpy(ptr2, ptr3, sizeof(pkgListType));
        memcpy(ptr3, ptr1, sizeof(pkgListType));
      }
    }
  }
  psramBusy = false;
}

uint16_t pkgType(const char *raw)
{
  uint16_t type = 0;
  char packettype = 0;
  const char *body;
  char *ptr;

  if (*raw == 0)
    return 0;

  packettype = (char)raw[0];
  body = &raw[1];

  switch (packettype)
  {
  case '$': // NMEA
    type |= FILTER_POSITION;
    break;
  case 0x27: /* ' */
  case 0x60: /* ` */
    type |= FILTER_POSITION;
    type |= FILTER_MICE;
    break;
  case '!':
  case '=':
    type |= FILTER_POSITION;
    if (body[18] == '_' || body[10] == '_')
    {
      type |= FILTER_WX;
      break;
    }
    // fall through
  case '/':
  case '@':
    type |= FILTER_POSITION;
    if (body[25] == '_' || body[16] == '_')
    {
      type |= FILTER_WX;
      break;
    }
    if (strchr(body, 'r') != NULL)
    {
      if (strchr(body, 'g') != NULL)
      {
        if (strchr(body, 't') != NULL)
        {
          if (strchr(body, 'P') != NULL)
          {
            type |= FILTER_WX;
          }
        }
      }
    }
    break;
  case ':':
    if (body[9] == ':' &&
        (memcmp(body + 10, "PARM", 4) == 0 ||
         memcmp(body + 10, "UNIT", 4) == 0 ||
         memcmp(body + 10, "EQNS", 4) == 0 ||
         memcmp(body + 10, "BITS", 4) == 0))
    {
      type |= FILTER_TELEMETRY;
    }
    else
    {
      type |= FILTER_MESSAGE;
    }
    break;
  case '{': // User defind
  case '<': // statcapa
  case '>':
    type |= FILTER_STATUS;
    break;
  case '?':
    type |= FILTER_QUERY;
    break;
  case ';':
    if (body[28] == '_')
      type |= FILTER_WX;
    else
      type |= FILTER_OBJECT;
    break;
  case ')':
    type |= FILTER_ITEM;
    break;
  case '}':
    type |= FILTER_THIRDPARTY;
    ptr = strchr(raw, ':');
    if (ptr != NULL)
    {
      ptr++;
      type |= pkgType(ptr);
    }
    break;
  case 'T':
    type |= FILTER_TELEMETRY;
    break;
  case '#': /* Peet Bros U-II Weather Station */
  case '*': /* Peet Bros U-I  Weather Station */
  case '_': /* Weather report without position */
    type |= FILTER_WX;
    break;
  default:
    type = 0;
    break;
  }
  return type;
}

uint16_t TNC2Raw[PKGLISTSIZE];
int raw_count = 0, raw_idx_rd = 0, raw_idx_rw = 0;

int pushTNC2Raw(int raw)
{
  if (raw < 0)
    return -1;
  if (raw_count > PKGLISTSIZE)
    return -1;
  if (++raw_idx_rw >= PKGLISTSIZE)
    raw_idx_rw = 0;
  TNC2Raw[raw_idx_rw] = raw;
  raw_count++;
  return raw_count;
}

int popTNC2Raw(int &ret)
{
  String raw = "";
  int idx = 0;
  if (raw_count <= 0)
    return -1;
  if (++raw_idx_rd >= PKGLISTSIZE)
    raw_idx_rd = 0;
  idx = TNC2Raw[raw_idx_rd];
  if (idx < PKGLISTSIZE)
    ret = idx;
  if (raw_count > 0)
    raw_count--;
  return raw_count;
}

pkgListType getPkgList(int idx)
{
  pkgListType ret;
#ifdef BOARD_HAS_PSRAM
  while (psramBusy)
    delay(1);
  psramBusy = true;
#endif
  memset(&ret, 0, sizeof(pkgListType));
  if (idx < PKGLISTSIZE)
    memcpy(&ret, &pkgList[idx], sizeof(pkgListType));
  psramBusy = false;
  return ret;
}

int pkgListUpdate(char *call, char *raw, uint16_t type, bool channel)
{
  size_t len;
  if (*call == 0)
    return -1;
  if (*raw == 0)
    return -1;

  char callsign[11];
  size_t sz = strlen(call);
  memset(callsign, 0, 11);
  if (sz > 10)
    sz = 10;
  // strncpy(callsign, call, sz);
  memcpy(callsign, call, sz);
#ifdef BOARD_HAS_PSRAM
  while (psramBusy)
    delay(1);
  psramBusy = true;
#endif
  int i = pkgList_Find(callsign, type);
  if (i > PKGLISTSIZE)
  {
    psramBusy = false;
    return -1;
  }
  if (i > -1)
  { // Found call in old pkg
    if ((channel == 1) || (channel == pkgList[i].channel))
    {
      pkgList[i].time = time(NULL);
      pkgList[i].pkg++;
      pkgList[i].type = type;
      if (channel == 0)
        pkgList[i].audio_level = (int16_t)mVrms;
      else
        pkgList[i].audio_level = 0;
      len = strlen(raw);
      // -1, not 500: leaves room for the null terminator below. Without it
      // (and without it at all, as this used to be), a slot that previously
      // held a longer packet keeps that packet's stale tail bytes past the
      // new, shorter one - handle_lastHeard() (and anything else doing
      // String(pkg.raw) / strlen(pkg.raw)) then reads past the real comment
      // into that leftover garbage. See FORK_NOTES.md's "never trust a
      // packet-derived length as a buffer size" - same bug class, this time
      // missing the terminator rather than the clamp.
      if (len > sizeof(pkgList[i].raw) - 1)
        len = sizeof(pkgList[i].raw) - 1;
      memcpy(pkgList[i].raw, raw, len);
      pkgList[i].raw[len] = 0;
      // SerialLOG.print("Update: ");
    }
  }
  else
  {
    i = pkgListOld(); // Search free in array
    if (i > PKGLISTSIZE || i < 0)
    {
      psramBusy = false;
      return -1;
    }
    // memset(&pkgList[i], 0, sizeof(pkgListType));
    pkgList[i].channel = channel;
    pkgList[i].time = time(NULL);
    pkgList[i].pkg = 1;
    pkgList[i].type = type;
    if (channel == 0)
      pkgList[i].audio_level = (int16_t)mVrms;
    else
      pkgList[i].audio_level = 0;
    // strcpy(pkgList[i].calsign, callsign);
    memcpy(pkgList[i].calsign, callsign, strlen(callsign));
    len = strlen(raw);
    if (len > sizeof(pkgList[i].raw) - 1)
      len = sizeof(pkgList[i].raw) - 1;
    memcpy(pkgList[i].raw, raw, len);
    pkgList[i].raw[len] = 0;
    // strcpy(pkgList[i].raw, raw);
    pkgList[i].calsign[10] = 0;
    // SerialLOG.print("NEW: ");
  }
  psramBusy = false;
  return i;
}

bool pkgTxDuplicate(AX25Msg ax25)
{
  while (psramBusy)
    delay(1);
  psramBusy = true;
  char callsign[12];
  for (int i = 0; i < PKGTXSIZE; i++)
  {
    if (txQueue[i].Active)
    {
      if (ax25.src.ssid > 0)
        sprintf(callsign, "%s-%d", ax25.src.call, ax25.src.ssid);
      else
        sprintf(callsign, "%s", ax25.src.call);
      if (strncmp(&txQueue[i].Info[0], callsign, strlen(callsign)) >= 0) // Check duplicate src callsign
      {
        char *ecs1 = strstr(txQueue[i].Info, ":");
        if (ecs1 == NULL)
          continue;
        ;
        if (strncmp(ecs1, (const char *)ax25.info, strlen(ecs1)) >= 0)
        { // Check duplicate aprs info
          txQueue[i].Active = false;
          psramBusy = false;
          return true;
        }
      }
    }
  }

  psramBusy = false;
  return false;
}

bool pkgTxPush(const char *info, size_t len, int dly)
{
  char *ecs = strstr(info, ">");
  if (ecs == NULL)
    return false;

  while (psramBusy)
    delay(1);
  psramBusy = true;
  // for (int i = 0; i < PKGTXSIZE; i++)
  // {
  //   if (txQueue[i].Active)
  //   {
  //     if ((strncmp(&txQueue[i].Info[0], info, info - ecs)==0)) //Check src callsign
  //     {
  //       // strcpy(&txQueue[i].Info[0], info);
  //       memset(txQueue[i].Info, 0, sizeof(txQueue[i].Info));
  //       memcpy(&txQueue[i].Info[0], info, len);
  //       txQueue[i].Delay = dly;
  //       txQueue[i].timeStamp = millis();
  //       psramBusy = false;
  //       return true;
  //     }
  //   }
  // }

  // Add
  for (int i = 0; i < PKGTXSIZE; i++)
  {
    if (txQueue[i].Active == false)
    {
      memset(txQueue[i].Info, 0, sizeof(txQueue[i].Info));
      memcpy(&txQueue[i].Info[0], info, len);
      txQueue[i].Delay = dly;
      txQueue[i].Active = true;
      txQueue[i].timeStamp = millis();
      break;
    }
  }
  psramBusy = false;
  return true;
}

void burstAfterVoice()
{
  String rawData;
  String cmn = "";
  if (gps.location.isValid()) // TRACKER by GPS
  {
    rawData = trk_gps_postion(cmn);
  }
  else // TRACKER by FIX position
  {
    rawData = trk_fix_position(cmn);
  }
  digitalWrite(POWER_PIN, config.rf_power); // RF Power LOW
  digitalWrite(SA868_MIC_SEL, HIGH);        // Select = ESP2MIC
  status.txCount++;
  projLog(LOGCAT_APRS_RF, "Burst TX->RF: %s", rawData.c_str());
  APRS_setPreamble(10);
  APRS_sendTNC2Pkt(rawData); // Send packet to RF

  for (int i = 0; i < 100; i++)
  {
    if (digitalRead(SA868_PTT_PIN))
      break;
    delay(50); // TOT 5sec
  }
  digitalWrite(SA868_PWR_PIN, 0); // set RF Power Low
}

bool pkgTxSend()
{
  if (getReceive())
    return false;
  while (psramBusy)
    delay(1);
  psramBusy = true;
  char info[500];
  for (int i = 0; i < PKGTXSIZE; i++)
  {
    if (txQueue[i].Active)
    {
      int decTime = millis() - txQueue[i].timeStamp;
      if (decTime > txQueue[i].Delay)
      {
        txQueue[i].Active = false;
        memset(info, 0, sizeof(info));
        strcpy(info, txQueue[i].Info);
        psramBusy = false;
        // digitalWrite(POWER_PIN, config.rf_power); // RF Power LOW

        digitalWrite(SA868_MIC_SEL, HIGH); // Select = ESP2MIC
        status.txCount++;
        LED_Color(255, 0, 0);
        adcActive(false);
        setOLEDLock(true);
        if (config.rf_type == RF_SA8x8_OpenEdit)
        {
          sa868.setLowPower();
          sa868.TxOn();
          delay(10);
        }
        APRS_setPreamble(config.preamble * 100);
        APRS_sendTNC2Pkt(String(info)); // Send packet to RF

        projLog(LOGCAT_APRS_RF, "TX->RF: %s", info);

        for (int i = 0; i < 100; i++)
        {
          if (digitalRead(PTT_PIN))
            break;
          delay(50); // TOT 5sec
        }
        if (config.rf_type == RF_SA8x8_OpenEdit)
        {
          sa868.TxOff();
          sa868.setLowPower();
          delay(1000);
          sa868.RxOn();
        }
        // delay(2000);
        LED_Color(0, 0, 0);
        adcActive(true);
        setOLEDLock(false);
        return true;
      }
    }
  }
  psramBusy = false;
  return false;
}

uint8_t *packetData;
// ฟังชั่นถูกเรียกมาจาก ax25_decode

int packet2Raw(String &tnc2, AX25Msg &Packet)
{
  if (Packet.len < 5)
    return 0;
  tnc2 = String(Packet.src.call);
  if (Packet.src.ssid > 0)
  {
    tnc2 += String(F("-"));
    tnc2 += String(Packet.src.ssid);
  }
  tnc2 += String(F(">"));
  tnc2 += String(Packet.dst.call);
  if (Packet.dst.ssid > 0)
  {
    tnc2 += String(F("-"));
    tnc2 += String(Packet.dst.ssid);
  }
  for (int i = 0; i < Packet.rpt_count; i++)
  {
    tnc2 += String(",");
    tnc2 += String(Packet.rpt_list[i].call);
    if (Packet.rpt_list[i].ssid > 0)
    {
      tnc2 += String("-");
      tnc2 += String(Packet.rpt_list[i].ssid);
    }
    if (Packet.rpt_flags & (1 << i))
      tnc2 += "*";
  }
  tnc2 += String(F(":"));
  // Packet.info is a length-prefixed byte buffer (Packet.len), not a
  // null-terminated C string - APRS payloads can legitimately contain
  // embedded NUL bytes (e.g. some Kenwood TM-D710 power-on packets, UTF-16
  // from AGWtracker). String((const char*)) calls strlen() internally and
  // silently truncates at the first one; concat(cstr, length) doesn't.
  tnc2.concat((const char *)Packet.info, Packet.len);

  return tnc2.length();
}
