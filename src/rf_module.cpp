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
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLESecurity.h>
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


void setupPowerRF()
{
  // bool result = PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);
  // if (result == false) {
  //     while (1) {
  //         Serial.println("PMU is not online...");
  //         delay(500);
  //     }
  // }
  //! DC3 Radio Pixels VDD , Don't change
  PMU.setDC3Voltage(3400);
  PMU.enableDC3();
}

bool SA868_waitResponse(String &data, String rsp, uint32_t timeout)
{
  uint32_t startMillis = millis();
  data.clear();
  do
  {
    while (SerialRF.available() > 0)
    {
      int8_t ch = SerialRF.read();
      data += static_cast<char>(ch);
      if (rsp.length() && data.endsWith(rsp))
      {
        return true;
      }
    }
  } while (millis() - startMillis < timeout);
  return false;
}

int SA868_getRSSI()
{
  return sa868.getRSSI();
}

String SA868_getVERSION()
{
  String data;

  if (config.rf_type == RF_SA8x8_OpenEdit)
  {
    SA868_Version version = sa868.Version();
    char str[17]; // worst case "255.255.255.255\0" = 16 bytes
    snprintf(str, sizeof(str), "%d.%d.%d.%d", version.major, version.minor, version.patch, version.revision);
    return String(str);
  }
  else
  {
    SerialRF.printf("AT+VERSION\r\n");
    if (SA868_waitResponse(data, "\r\n", 1000))
    {
      String version = data.substring(0, data.indexOf("\r\n"));
      return version;
    }
    else
    {
      // timeout or error
      return "-";
    }
  }
  return "-";
}

String FRS_getVERSION()
{
  String data;
  String version;

  SerialRF.printf("AT+DMOVER\r\n");
  if (SA868_waitResponse(data, "\r\n", 1000))
  {
    int st = data.indexOf("DMOVER:");
    if (st > 0)
    {
      version = data.substring(st + 7, data.indexOf("\r\n"));
    }
    else
    {
      version = "Not found";
    }
    return version;
  }
  else
  {
    // timeout or error
    return "-";
  }
}

unsigned long SA818_Timeout = 0;

void RF_MODULE_SLEEP()
{
  if (config.rf_type == RF_SA8x8_OpenEdit)
    sa868.setLowPower();
  else
    digitalWrite(POWER_PIN, LOW);
  digitalWrite(PULLDOWN_PIN, LOW);
  // PMU.disableDC3();
}

String hexToString(String hex)
{ // for String to HEX conversion
  String text = "";
  for (int k = 0; k < hex.length(); k++)
  {
    if (k % 2 != 0)
    {
      char temp[3];
      sprintf(temp, "%c%c", hex[k - 1], hex[k]);
      int number = (int)strtol(temp, NULL, 16);
      text += char(number);
    }
  }
  return text;
}

String ctcssToHex(unsigned int decValue, int section)
{ // use to convert the CTCSS reading which RF module needed
  if (decValue == 7777)
  {
    return hexToString("FF");
  }
  String d1d0 = String(decValue);
  if (decValue < 1000)
  {
    d1d0 = "0" + d1d0;
  }
  if (section == 1)
  {
    return hexToString(d1d0.substring(2, 4));
  }
  else
  {
    return hexToString(d1d0.substring(0, 2));
  }
}

void RF_MODULE(bool boot)
{
  String data;
  // todo find out what is config.rf_en
  if (config.rf_en == false)
  {
    RF_MODULE_SLEEP();
    return;
  }
  if (config.rf_type == RF_NONE)
    return;
  projLog(LOGCAT_RF_MODULE, "RF Module %s Init", RF_TYPE[config.rf_type]);
  //! DC3 Radio & Pixels VDD , Don't change
  // PMU.setDC3Voltage(3400);
  // PMU.disableDC3();
  pinMode(BUTTON_PTT_PIN, INPUT_PULLUP); // PTT BUTTON

  pinMode(SA868_MIC_SEL, OUTPUT); // MIC_SEL
  digitalWrite(SA868_MIC_SEL, LOW);

  pinMode(SA868_PD_PIN, OUTPUT);
  digitalWrite(SA868_PD_PIN, HIGH); // PWR HIGH

  pinMode(SA868_PWR_PIN, OUTPUT);
  digitalWrite(SA868_PWR_PIN, LOW); // RF POWER LOW

  pinMode(SA868_PTT_PIN, OUTPUT);
  digitalWrite(SA868_PTT_PIN, HIGH); // PTT HIGH

  setupPowerRF();
  // pinMode(POWER_PIN, OUTPUT);
  // pinMode(PULLDOWN_PIN, OUTPUT);
  // // pinMode(SQL_PIN, INPUT);
  // pinMode(PTT_PIN, OUTPUT);

  // digitalWrite(PTT_PIN, LOW);
  // digitalWrite(POWER_PIN, LOW);
  // digitalWrite(PULLDOWN_PIN, LOW);
  // delay(1000);
  // digitalWrite(PTT_PIN, HIGH);
  // digitalWrite(PULLDOWN_PIN, HIGH);
  // delay(500);
  if (boot)
  {
    SerialRF.begin(9600, SERIAL_8N1, SA868_RX_PIN, SA868_TX_PIN);
  }

  delay(1500);
  if (config.rf_type == RF_SA8x8_OpenEdit)
  {
    sa868.init();
    sa868.setRxFrequency((uint32_t)(config.freq_rx * 1000000));
    sa868.setTxFrequency((uint32_t)(config.freq_tx * 1000000));
    sa868.setVolume(config.volume);
    // sa868.setBandwidth(config.band);
    delay(200);
    sa868.TxOff();
    sa868.setLowPower();
    delay(1000);
    sa868.RxOn();
  }
  else
  {
    SerialRF.write("\r\n");

    char str[200];
    String rsp = "\r\n";
    if (config.sql_level > 8)
      config.sql_level = 8;
    if ((config.rf_type == RF_SR_1WV) || (config.rf_type == RF_SR_1WU) || (config.rf_type == RF_SR_1W350))
    {
      SerialRF.printf("AT+DMOCONNECT\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      sprintf(str, "AT+DMOSETGROUP=%01d,%0.4f,%0.4f,%d,%01d,%d,0", config.band, config.freq_tx + ((float)config.offset_tx / 1000000), config.freq_rx + ((float)config.offset_rx / 1000000), config.tone_rx, config.sql_level, config.tone_tx);
      SerialRF.println(str);
      projLog(LOGCAT_RF_MODULE, "Write to SR_FRS: %s", str);
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      // Module auto power save setting
      SerialRF.printf("AT+DMOAUTOPOWCONTR=1\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+DMOSETVOX=0\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+DMOSETMIC=6,0\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+DMOSETVOLUME=%d\r\n", config.volume);
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
    }
    else if ((config.rf_type == RF_SA868_VHF) || (config.rf_type == RF_SA868_UHF) || (config.rf_type == RF_SA868_350))
    {
      SerialRF.printf("AT+DMOCONNECT\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+DMOCONNECT\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      sprintf(str, "AT+DMOSETGROUP=%01d,%0.4f,%0.4f,%04d,%01d,%04d\r\n", config.band, config.freq_tx, config.freq_rx, config.tone_tx, config.sql_level, config.tone_rx);
      SerialRF.print(str);
      projLog(LOGCAT_RF_MODULE, "Write to SA868: %s", str);
      if (SA868_waitResponse(data, rsp, 2000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+SETTAIL=0\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+SETFILTER=1,1,1\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+DMOSETVOLUME=%d\r\n", config.volume);
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
    }
    else if ((config.rf_type == RF_SR_2WVS) || (config.rf_type == RF_SR_2WUS))
    {
      uint8_t flag = 0;  // Bit0:busy lock 0:OFF,1:ON Bit1:band 0:Wide,1:Narrow
      uint8_t flag1 = 0; // Bit0: Hi/Lo 0:2W,1:0.5W  Bit1:Middle 0:2W/0.5W 1:1W
      RF_VERSION = FRS_getVERSION();
      projLog(LOGCAT_RF_MODULE, "RF Module Version %s", RF_VERSION.c_str());

      String tone_rx, tone_tx;

      if (config.tone_rx > 0)
      {
        int idx = config.tone_rx;
        if (idx < sizeof(ctcss))
        {
          int tone = (int)(ctcss[config.tone_rx] * 10.0F);
          tone_rx = ctcssToHex(tone, 1);
          tone_rx += ctcssToHex(tone, 2);
          projLog(LOGCAT_RF_MODULE, "Tone RX[%d] rx=%0X %0X", tone, tone_rx.charAt(0), tone_rx.charAt(1));
        }
      }
      else
      {
        tone_rx = ctcssToHex(7777, 1);
        tone_rx += ctcssToHex(7777, 2);
      }

      if (config.tone_tx > 0)
      {
        int idx = config.tone_tx;
        if (idx < sizeof(ctcss))
        {
          int tone = (int)(ctcss[config.tone_tx] * 10.0F);
          tone_tx = ctcssToHex(tone, 1);
          tone_tx += ctcssToHex(tone, 2);
          projLog(LOGCAT_RF_MODULE, "Tone RX[%d] tx=%0X %0X", tone, tone_tx.charAt(0), tone_tx.charAt(1));
        }
      }
      else
      {
        tone_tx = ctcssToHex(7777, 1);
        tone_tx += ctcssToHex(7777, 2);
      }

      if (config.band == 0)
        flag |= 0x02;
      if (config.rf_power == 0)
        flag1 |= 0x02;
      sprintf(str, "AT+DMOGRP=%0.5f,%0.5f,%s,%s,%d,%d\r\n", config.freq_rx, config.freq_tx, tone_rx.c_str(), tone_tx.c_str(), flag, flag1);
      projLog(LOGCAT_RF_MODULE, "Write to SR_FRS_2W: %s", str);
      SerialRF.print(str);

      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+DMOSAV=1\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+DMOVOL=%d\r\n", config.volume);
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+DMOVOX=0\r\n");
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
      SerialRF.printf("AT+DMOFUN=%d,5,1,0,0\r\n", config.sql_level);
      if (SA868_waitResponse(data, rsp, 1000))
        projLog(LOGCAT_RF_MODULE, "%s", data.c_str());
    }
  }
}

void RF_MODULE_CHECK()
{
  while (SerialRF.available() > 0)
    SerialRF.read();
  SerialRF.println("AT+DMOCONNECT");
  delay(100);
  if (SerialRF.available() > 0)
  {
    String ret = SerialRF.readString();
    if (ret.indexOf("DMOCONNECT") > 0)
    {
      SA818_Timeout = millis();
      // Serial.println(SerialRF.readString());
      projLog(LOGCAT_RF_MODULE, "RF Module %s Activate", RF_TYPE[config.rf_type]);
    }
  }
  else
  {
    projLog(LOGCAT_RF_MODULE, "RF Module %s sleep", RF_TYPE[config.rf_type]);
    if (config.rf_type == RF_SA8x8_OpenEdit)
      sa868.setLowPower();
    digitalWrite(POWER_PIN, LOW);
    digitalWrite(PULLDOWN_PIN, LOW);
    delay(500);
    RF_MODULE(true);
  }
}
