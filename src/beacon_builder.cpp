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


// smartbeacon() needs these; they're defined in main.cpp (also used by
// taskAPRS() there), not moved here since they're shared with code that stays.
extern uint16_t tx_interval;
extern unsigned int tx_counter;
extern int16_t last_heading, heading_change;
extern uint16_t trk_interval;

void smartbeacon(void)
{
  // Adaptive beacon rate
  if (SB_SPEED >= config.trk_hspeed)
  {
    tx_interval = (uint16_t)config.trk_maxinterval;
  }
  else
  {
    if (SB_SPEED <= config.trk_lspeed)
    {
      // Speed is below lower limit - use minimum rate
      tx_interval = (uint16_t)config.trk_slowinterval;
    }
    else
    {
      // In between, do SmartBeaconing calculations
      if (SB_SPEED <= 0)
        SB_SPEED = 1;
      tx_interval = (trk_interval * config.trk_hspeed) / SB_SPEED;
      if (tx_interval < 5)
        tx_interval = 5;
      if (tx_interval > config.trk_maxinterval)
        tx_interval = (uint16_t)config.trk_maxinterval;

      // Corner pegging - note that we use two-degree units
      if (last_heading > SB_HEADING)
        heading_change = last_heading - SB_HEADING;
      else
        heading_change = SB_HEADING - last_heading;
      if (heading_change > 90)
        heading_change = 180 - heading_change;
      if (heading_change > config.trk_minangle)
      {
        // Make sure last heading is updated during turns to avoid immedate xmit
        if (tx_counter > config.trk_mininterval)
        {
          EVENT_TX_POSITION = 3;
        }
        else
        {
          // last_heading = SB_HEADING;
          tx_interval = config.trk_mininterval;
        }
      }
    }
  }
}

// void printTime()
// {
// 	SerialLOG.print("[");
// 	SerialLOG.print(hour());
// 	SerialLOG.print(":");
// 	SerialLOG.print(minute());
// 	SerialLOG.print(":");
// 	SerialLOG.print(second());
// 	SerialLOG.print("]");
// }


String compress_position(double nowLat, double nowLng, int alt_feed, double course, uint16_t spdKnot, char table, char symbol, bool gps)
{
  String str_comp = "";
  String lat, lon;
  lat = deg2lat(nowLat);
  lon = deg2lon(nowLng);
  ESP_LOGE("GPS", "Aprs Compress");
  // Translate from semicircles to Base91 format
  char aprs_position[13];
  long latitude = semicircles((char *)lat.c_str(), (nowLat < 0));
  long longitude = semicircles((char *)lon.c_str(), (nowLng < 0));
  long ltemp = 1073741824L - latitude; // 90 degrees - latitude
  ESP_LOGE("GPS", "lat=%u lon=%u", latitude, longitude);
  memset(aprs_position, 0, sizeof(aprs_position));

  base91encode(ltemp, aprs_position);
  ltemp = 1073741824L + (longitude >> 1); // 180 degrees + longitude
  base91encode(ltemp, aprs_position + 4);
  // Encode heading
  uint8_t c = (uint8_t)(course / 4);
  // Scan lookup table to encode speed
  uint8_t s = (uint8_t)(log(spdKnot + 1) / log(1.08));
  if ((spdKnot <= 5) && (alt_feed > 0) && config.trk_altitude)
  {
    if (gps && config.trk_cst)
    {
      // Send Altitude
      aprs_position[11] = '!' + 0x30; // t current,GGA
      int alt = (int)alt_feed;
      int cs = (int)(log(alt) / log(1.002));
      c = (uint8_t)(cs / 91);
      s = (uint8_t)(cs - ((int)c * 91));
      if (s > 91)
        s = 91;
      aprs_position[9] = '!' + c;  // c
      aprs_position[10] = '!' + s; // s
    }
    else
    {
      // Send Range
      aprs_position[11] = '!' + 0x00; //
      aprs_position[9] = '{';         // c = {
      if (!sa868.isHighPower())
      {
        s = 10;
      }
      else
      {
        s = 30;
      }
      aprs_position[10] = '!' + s; // s
    }
  }
  else
  {
    // Send course and speed
    aprs_position[9] = '!' + c;  // c
    aprs_position[10] = '!' + s; // s

    if (gps)
    {
      aprs_position[11] = '!' + 0x20 + 0x18 + 0x06; // t 0x20 1=current,0x18 11=RMC,0x06 110=Other tracker
    }
    else
    {
      aprs_position[11] = '!' + 0x00 + 0x18 + 0x06; // t
    }
  }
  aprs_position[12] = 0;
  // waveFlag = false;
  aprs_position[8] = symbol; // Symbol
  str_comp = String(table) + String(aprs_position);
  return str_comp;
}

String getTimeStamp()
{
  char strtmp[50];
  time_t now;
  time(&now);
  struct tm *info = gmtime(&now);
  sprintf(strtmp, "%02d%02d%02dz", info->tm_mday, info->tm_hour, info->tm_min);
  return String(strtmp);
}

String trk_gps_postion(String comment)
{
  String rawData = "";
  String lat, lon;
  double nowLat, nowLng;
  char rawTNC[300];
  char aprs_table, aprs_symbol;
  char timestamp[16]; // 8 real chars ("MMddHHmm") + headroom for the compiler's worst-case %02d width
  struct tm tmstruct;
  double dist, course, speed;
  time_t nowTime;

  memset(rawTNC, 0, sizeof(rawTNC));
  getLocalTime(&tmstruct, 5000);
  snprintf(timestamp, sizeof(timestamp), "%02d%02d%02d%02d", (tmstruct.tm_mon + 1), tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);
  time(&nowTime);
  // nowTime = gps.time.value();
  if (lastTimeStamp == 0)
    lastTimeStamp = nowTime;

  // lastTimeStamp=10000;
  // nowTime=lastTimeStamp+1800;
  time_t tdiff = nowTime - lastTimeStamp;

  // aprs_table = config.aprs_table;
  // aprs_symbol = config.aprs_symbol;
  if (config.trk_smartbeacon)
  {
    if (SB_SPEED < config.trk_lspeed)
    {
      aprs_table = config.trk_symstop[0];
      aprs_symbol = config.trk_symstop[1];
      SB_SPEED = 0;
    }
    else
    {
      aprs_table = config.trk_symmove[0];
      aprs_symbol = config.trk_symmove[1];
    }
  }
  else
  {
    aprs_table = config.trk_symbol[0];
    aprs_symbol = config.trk_symbol[1];
  }

  if (gps.location.isValid() && (gps.hdop.hdop() < 10.0))
  {
    nowLat = gps.location.lat();
    nowLng = gps.location.lng();

    uint16_t spdKnot;
    if (LastLng == 0 || LastLat == 0)
    {
      course = gps.course.deg();
      spdKnot = (uint16_t)gps.speed.knots();
    }
    else
    {
      dist = distance(LastLng, LastLat, nowLng, nowLat);
      course = direction(LastLng, LastLat, nowLng, nowLat);
      if (dist > 50.0F)
        dist = 0;
      if (tdiff > 10 && (nowTime > lastTimeStamp))
        speed = dist / ((double)tdiff / 3600);
      else
        speed = 0.0F;

      if (speed > 999)
        speed = 999.0F;
      // uint16_t spdMph=(uint16_t)(speed / 1.609344);
      spdKnot = (uint16_t)(speed * 0.53996F);
      if (spdKnot > 94)
        spdKnot = 0;
    }

    LastLat = nowLat;
    LastLng = nowLng;
    lastTimeStamp = nowTime;

    if (config.trk_compress)
    { // Compress DATA

      String compPosition = compress_position(nowLat, nowLng, gps.altitude.feet(), course, spdKnot, aprs_table, aprs_symbol, (gps.satellites.value() > 3));
      // ESP_LOGE("GPS", "Compress=%s", aprs_position);
      if (strlen(config.trk_item) >= 3)
      {
        char object[10];
        memset(object, 0x20, 10);
        memcpy(object, config.trk_item, strlen(config.trk_item));
        object[9] = 0;
        if (config.trk_timestamp)
        {
          String timeStamp = getTimeStamp();
          sprintf(rawTNC, ";%s*%s%s", object, timeStamp.c_str(), compPosition.c_str());
        }
        else
        {
          sprintf(rawTNC, ")%s!%s", config.trk_item, compPosition.c_str());
        }
      }
      else
      {
        if (config.trk_timestamp)
        {
          String timeStamp = getTimeStamp();
          sprintf(rawTNC, "/%s%s", timeStamp.c_str(), compPosition.c_str());
        }
        else
        {
          sprintf(rawTNC, "!%s", compPosition.c_str());
        }
      }
    }
    else
    { // None compress DATA
      int lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss;
      char lon_ew = 'E';
      char lat_ns = 'N';
      if (nowLat < 0)
        lat_ns = 'S';
      if (nowLng < 0)
        lon_ew = 'W';
      DD_DDDDDtoDDMMSS(nowLat, &lat_dd, &lat_mm, &lat_ss);
      DD_DDDDDtoDDMMSS(nowLng, &lon_dd, &lon_mm, &lon_ss);
      char csd_spd[8];
      memset(csd_spd, 0, sizeof(csd_spd));
      if (config.trk_cst)
      {
        sprintf(csd_spd, "%03d/%03d", (int)gps.course.deg(), (int)gps.speed.knots());
      }
      if (strlen(config.trk_item) >= 3)
      {
        char object[10];
        memset(object, 0x20, 10);
        memcpy(object, config.trk_item, strlen(config.trk_item));
        object[9] = 0;
        if (config.trk_timestamp)
        {
          String timeStamp = getTimeStamp();
          sprintf(rawTNC, ";%s*%s%02d%02d.%02d%c%c%03d%02d.%02d%c%c%s", object, timeStamp.c_str(), lat_dd, lat_mm, lat_ss, lat_ns, aprs_table, lon_dd, lon_mm, lon_ss, lon_ew, aprs_symbol, csd_spd);
        }
        else
        {
          sprintf(rawTNC, ")%s!%02d%02d.%02d%c%c%03d%02d.%02d%c%c%s", config.trk_item, lat_dd, lat_mm, lat_ss, lat_ns, aprs_table, lon_dd, lon_mm, lon_ss, lon_ew, aprs_symbol, csd_spd);
        }
      }
      else
      {
        if (config.trk_timestamp)
        {
          String timeStamp = getTimeStamp();
          sprintf(rawTNC, "/%s%02d%02d.%02d%c%c%03d%02d.%02d%c%c%s", timeStamp.c_str(), lat_dd, lat_mm, lat_ss, lat_ns, aprs_table, lon_dd, lon_mm, lon_ss, lon_ew, aprs_symbol, csd_spd);
        }
        else
        {
          sprintf(rawTNC, "!%02d%02d.%02d%c%c%03d%02d.%02d%c%c%s", lat_dd, lat_mm, lat_ss, lat_ns, aprs_table, lon_dd, lon_mm, lon_ss, lon_ew, aprs_symbol, csd_spd);
        }
      }
      if (config.trk_altitude)
      {

        if (gps.altitude.isValid())
        {
          char strAltitude[10];
          memset(strAltitude, 0, sizeof(strAltitude));
          sprintf(strAltitude, "/A=%06d", (int)gps.altitude.feet());
          strcat(rawTNC, strAltitude);
        }
      }
    }
  }
  else
  {
    sprintf(rawTNC, ">%s ", config.trk_item);
  }

  String tnc2Raw = "";
  char strtmp[300];
  if (config.trk_ssid == 0)
    sprintf(strtmp, "%s>APTWR", config.trk_mycall);
  else
    sprintf(strtmp, "%s-%d>APTWR", config.trk_mycall, config.trk_ssid);
  tnc2Raw = String(strtmp);
  if (config.trk_path < 5)
  {
    if (config.trk_path > 0)
      tnc2Raw += "-" + String(config.trk_path);
  }
  else
  {
    tnc2Raw += ",";
    tnc2Raw += getPath(config.trk_path);
  }
  tnc2Raw += ":";
  tnc2Raw += String(rawTNC);
  tnc2Raw += comment + " " + String(config.trk_comment);
  return tnc2Raw;
}

String trk_fix_position(String comment)
{
  char strtmp[500], loc[100];
  String tnc2Raw = "";
  memset(strtmp, 0, sizeof(strtmp));
  memset(loc, 0, sizeof(loc));
  if (config.trk_compress)
  { // Compress DATA

    String compPosition = compress_position(config.trk_lat, config.trk_lon, (int)(config.trk_alt * 3.28F), 0, 0, config.trk_symbol[0], config.trk_symbol[1], true);
    // ESP_LOGE("GPS", "Compress=%s", aprs_position);
    if (strlen(config.trk_item) >= 3)
    {
      char object[10];
      memset(object, 0x20, 10);
      memcpy(object, config.trk_item, strlen(config.trk_item));
      object[9] = 0;
      if (config.trk_timestamp)
      {
        String timeStamp = getTimeStamp();
        sprintf(loc, ";%s*%s%s", object, timeStamp.c_str(), compPosition.c_str());
      }
      else
      {
        sprintf(loc, ")%s!%s", config.trk_item, compPosition.c_str());
      }
    }
    else
    {
      if (config.trk_timestamp)
      {
        String timeStamp = getTimeStamp();
        sprintf(loc, "/%s%s", timeStamp.c_str(), compPosition.c_str());
      }
      else
      {
        sprintf(loc, "!%s", compPosition.c_str());
      }
    }
  }
  else
  {
    int lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss;

    char lon_ew = 'E';
    char lat_ns = 'N';
    if (config.trk_lat < 0)
      lat_ns = 'S';
    if (config.trk_lon < 0)
      lon_ew = 'W';

    DD_DDDDDtoDDMMSS(config.trk_lat, &lat_dd, &lat_mm, &lat_ss);
    DD_DDDDDtoDDMMSS(config.trk_lon, &lon_dd, &lon_mm, &lon_ss);

    if (strlen(config.trk_item) >= 3)
    {
      char object[10];
      memset(object, 0x20, 10);
      memcpy(object, config.trk_item, strlen(config.trk_item));
      object[9] = 0;
      if (config.trk_timestamp)
      {
        String timeStamp = getTimeStamp();
        sprintf(loc, ";%s*%s%02d%02d.%02d%c%c%03d%02d.%02d%c%c", object, timeStamp.c_str(), lat_dd, lat_mm, lat_ss, lat_ns, config.trk_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.trk_symbol[1]);
      }
      else
      {
        sprintf(loc, ")%s!%02d%02d.%02d%c%c%03d%02d.%02d%c%c", config.trk_item, lat_dd, lat_mm, lat_ss, lat_ns, config.trk_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.trk_symbol[1]);
      }
    }
    else
    {
      if (config.trk_timestamp)
      {
        String timeStamp = getTimeStamp();
        sprintf(loc, "/%s%02d%02d.%02d%c%c%03d%02d.%02d%c%c", timeStamp.c_str(), lat_dd, lat_mm, lat_ss, lat_ns, config.trk_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.trk_symbol[1]);
      }
      else
      {
        sprintf(loc, "!%02d%02d.%02d%c%c%03d%02d.%02d%c%c", lat_dd, lat_mm, lat_ss, lat_ns, config.trk_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.trk_symbol[1]);
      }
    }

    if (config.trk_alt > 0)
    {
      char strAltitude[12];
      memset(strAltitude, 0, sizeof(strAltitude));
      sprintf(strAltitude, "/A=%06d", (int)(config.trk_alt * 3.28F));
      strcat(loc, strAltitude);
    }
  }

  if (config.trk_ssid == 0)
    sprintf(strtmp, "%s>APTWR", config.trk_mycall);
  else
    sprintf(strtmp, "%s-%d>APTWR", config.trk_mycall, config.trk_ssid);
  tnc2Raw = String(strtmp);
  if (config.trk_path < 5)
  {
    if (config.trk_path > 0)
      tnc2Raw += "-" + String(config.trk_path);
  }
  else
  {
    tnc2Raw += ",";
    tnc2Raw += getPath(config.trk_path);
  }
  tnc2Raw += ":";
  tnc2Raw += String(loc);
  tnc2Raw += comment + " " + String(config.trk_comment);
  return tnc2Raw;
}

String igate_position(double lat, double lon, double alt, String comment)
{
  String tnc2Raw = "";
  int lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss;
  char strtmp[500], loc[100];
  char lon_ew = 'E';
  char lat_ns = 'N';
  if (lat < 0)
    lat_ns = 'S';
  if (lon < 0)
    lon_ew = 'W';
  memset(strtmp, 0, sizeof(strtmp));
  memset(loc, 0, sizeof(loc));
  DD_DDDDDtoDDMMSS(lat, &lat_dd, &lat_mm, &lat_ss);
  DD_DDDDDtoDDMMSS(lon, &lon_dd, &lon_mm, &lon_ss);
  char strAltitude[12];
  memset(strAltitude, 0, sizeof(strAltitude));
  if (alt > 0)
  {
    sprintf(strAltitude, "/A=%06d", (int)(alt * 3.28F));
  }
  if (strlen(config.igate_object) >= 3)
  {
    char object[10];
    memset(object, 0x20, 10);
    memcpy(object, config.igate_object, strlen(config.igate_object));
    object[9] = 0;
    if (config.igate_timestamp)
    {
      String timeStamp = getTimeStamp();
      sprintf(loc, ";%s*%s%02d%02d.%02d%c%c%03d%02d.%02d%c%c", object, timeStamp.c_str(), lat_dd, lat_mm, lat_ss, lat_ns, config.igate_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.igate_symbol[1]);
    }
    else
    {
      sprintf(loc, ")%s!%02d%02d.%02d%c%c%03d%02d.%02d%c%c", config.igate_object, lat_dd, lat_mm, lat_ss, lat_ns, config.igate_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.igate_symbol[1]);
    }
  }
  else
  {
    if (config.igate_timestamp)
    {
      String timeStamp = getTimeStamp();
      sprintf(loc, "/%s%02d%02d.%02d%c%c%03d%02d.%02d%c%c", timeStamp.c_str(), lat_dd, lat_mm, lat_ss, lat_ns, config.igate_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.igate_symbol[1]);
    }
    else
    {
      sprintf(loc, "!%02d%02d.%02d%c%c%03d%02d.%02d%c%c", lat_dd, lat_mm, lat_ss, lat_ns, config.igate_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.igate_symbol[1]);
    }
  }
  if (config.aprs_ssid == 0)
    sprintf(strtmp, "%s>APTWR", config.aprs_mycall);
  else
    sprintf(strtmp, "%s-%d>APTWR", config.aprs_mycall, config.aprs_ssid);
  tnc2Raw = String(strtmp);
  if (config.igate_path < 5)
  {
    if (config.igate_path > 0)
      tnc2Raw += "-" + String(config.igate_path);
  }
  else
  {
    tnc2Raw += ",";
    tnc2Raw += getPath(config.igate_path);
  }
  tnc2Raw += ":";
  tnc2Raw += String(loc);
  tnc2Raw += String(config.igate_phg) + String(strAltitude);
  tnc2Raw += comment + " " + String(config.igate_comment);
  return tnc2Raw;
}

String digi_position(double lat, double lon, double alt, String comment)
{
  String tnc2Raw = "";
  int lat_dd, lat_mm, lat_ss, lon_dd, lon_mm, lon_ss;
  char strtmp[500], loc[100];
  char lon_ew = 'E';
  char lat_ns = 'N';
  if (lat < 0)
    lat_ns = 'S';
  if (lon < 0)
    lon_ew = 'W';
  memset(strtmp, 0, sizeof(strtmp));
  memset(loc, 0, sizeof(loc));
  DD_DDDDDtoDDMMSS(lat, &lat_dd, &lat_mm, &lat_ss);
  DD_DDDDDtoDDMMSS(lon, &lon_dd, &lon_mm, &lon_ss);
  char strAltitude[12];
  memset(strAltitude, 0, sizeof(strAltitude));
  if (alt > 0)
  {
    sprintf(strAltitude, "/A=%06d", (int)(alt * 3.28F));
  }
  if (config.digi_timestamp)
  {
    String timeStamp = getTimeStamp();
    sprintf(loc, "/%s%02d%02d.%02d%c%c%03d%02d.%02d%c%c", timeStamp.c_str(), lat_dd, lat_mm, lat_ss, lat_ns, config.digi_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.digi_symbol[1]);
  }
  else
  {
    sprintf(loc, "!%02d%02d.%02d%c%c%03d%02d.%02d%c%c", lat_dd, lat_mm, lat_ss, lat_ns, config.digi_symbol[0], lon_dd, lon_mm, lon_ss, lon_ew, config.digi_symbol[1]);
  }
  if (config.digi_ssid == 0)
    sprintf(strtmp, "%s>APTWR", config.digi_mycall);
  else
    sprintf(strtmp, "%s-%d>APTWR", config.digi_mycall, config.digi_ssid);
  tnc2Raw = String(strtmp);
  if (config.digi_path < 5)
  {
    if (config.digi_path > 0)
      tnc2Raw += "-" + String(config.digi_path);
  }
  else
  {
    tnc2Raw += ",";
    tnc2Raw += getPath(config.digi_path);
  }
  tnc2Raw += ":";
  tnc2Raw += String(loc);
  tnc2Raw += String(config.digi_phg) + String(strAltitude);
  tnc2Raw += comment + " " + String(config.digi_comment);
  return tnc2Raw;
}

