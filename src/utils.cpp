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


/// Degrees to radians.
#define DEG2RAD(x) (x / 360 * 2 * PI)
/// Radians to degrees.
#define RAD2DEG(x) (x * (180 / PI))

double direction(double lon0, double lat0, double lon1, double lat1)
{
  double direction;

  /* Convert degrees into radians. */
  lon0 = DEG2RAD(lon0);
  lat0 = DEG2RAD(lat0);
  lon1 = DEG2RAD(lon1);
  lat1 = DEG2RAD(lat1);

  /* Direction from Aviation Formulary V1.42 by Ed Williams by way of
   * http://mathforum.org/library/drmath/view/55417.html */
  direction = atan2(sin(lon1 - lon0) * cos(lat1), cos(lat0) * sin(lat1) - sin(lat0) * cos(lat1) * cos(lon1 - lon0));
  if (direction < 0)
  {
    /* Make direction positive. */
    direction += 2 * PI;
  }

  return RAD2DEG(direction);
}

double distance(double lon0, double lat0, double lon1, double lat1)
{
  double dlon;
  double dlat;
  double a, c;
  /* Convert degrees into radians. */
  lon0 = DEG2RAD(lon0);
  lat0 = DEG2RAD(lat0);
  lon1 = DEG2RAD(lon1);
  lat1 = DEG2RAD(lat1);

  /* Use the haversine formula for distance calculation
   * http://mathforum.org/library/drmath/view/51879.html */
  dlon = lon1 - lon0;
  dlat = lat1 - lat0;
  a = pow(sin(dlat / 2), 2) + cos(lat0) * cos(lat1) * pow(sin(dlon / 2), 2);
  c = 2 * atan2(sqrt(a), sqrt(1 - a));

  return c * 6366.71; /* in kilometers */
}


String deg2lat(double deg)
{
  char sign;
  if (deg > 0.0F)
  {
    sign = 'N';
  }
  else
  {
    sign = 'S';
    deg *= -1;
  }

  uint id = (uint)floor(deg);
  uint im = (uint)((deg - (double)id) * 60);
  uint imm = (uint)round((((deg - (double)id) * 60) - (double)im) * 100);
  char dmm[10];
  sprintf(dmm, "%02d%02d.%02d%c", id, im, imm, sign);
  return String(dmm);
}

String deg2lon(double deg)
{
  char sign;
  if (deg > 0.0F)
  {
    sign = 'E';
  }
  else
  {
    sign = 'W';
    deg *= -1;
  }
  uint id = (uint)floor(deg);
  uint im = (uint)((deg - (double)id) * 60);
  uint imm = (uint)round((((deg - (double)id) * 60) - (double)im) * 100);
  char dmm[10];
  sprintf(dmm, "%03d%02d.%02d%c", id, im, imm, sign);
  return String(dmm);
}


String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length();

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
} // END

boolean isValidNumber(String str)
{
  for (byte i = 0; i < str.length(); i++)
  {
    if (isDigit(str.charAt(i)))
      return true;
  }
  return false;
}

uint8_t checkSum(uint8_t *ptr, size_t count)
{
  uint8_t lrc, tmp;
  uint16_t i;
  lrc = 0;
  for (i = 0; i < count; i++)
  {
    tmp = *ptr++;
    lrc = lrc ^ tmp;
  }
  return lrc;
}


float conv_coords(float in_coords)
{
  // Initialize the location.
  float f = in_coords;
  // Get the first two digits by turning f into an integer, then doing an integer divide by 100;
  // firsttowdigits should be 77 at this point.
  int firsttwodigits = ((int)f) / 100; // This assumes that f < 10000.
  float nexttwodigits = f - (float)(firsttwodigits * 100);
  float theFinalAnswer = (float)(firsttwodigits + nexttwodigits / 60.0);
  return theFinalAnswer;
}

void DD_DDDDDtoDDMMSS(float DD_DDDDD, int *DD, int *MM, int *SS)
{
  DD_DDDDD = abs(DD_DDDDD);
  *DD = (int)DD_DDDDD;
  *MM = (int)((DD_DDDDD - *DD) * 60);
  *SS = ((DD_DDDDD - *DD) * 60 - *MM) * 100;
}

