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
#include <NimBLEDevice.h>
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

char VERSION[9] = "00000000";
char VERSION_BUILD = '?';

void initVersion()
{
  // __DATE__ is "Mmm dd yyyy" (day space-padded below 10), __TIME__ is "hh:mm:ss" - both fixed by the C++ standard.
  static const char *monthAbbrev[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char monStr[4] = {0};
  int day = 0, year = 0;
  sscanf(__DATE__, "%3s %d %d", monStr, &day, &year);
  int month = 1;
  for (int i = 0; i < 12; i++)
  {
    if (strncmp(monStr, monthAbbrev[i], 3) == 0)
    {
      month = i + 1;
      break;
    }
  }
  snprintf(VERSION, sizeof(VERSION), "%04d%02d%02d", year, month, day);

  int hour = 0, minute = 0, second = 0;
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);
  VERSION_BUILD = 'A' + (hour % 24);
}

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

struct pbuf_t aprs;
ParseAPRS aprsParse;

TinyGPSPlus gps;

#define VBAT_PIN 36
#define SQL_PIN -1
HardwareSerial SerialRF(1);

SA868 sa868(&SerialRF, SA868_RX_PIN, SA868_TX_PIN);

#define PWR_VDD 4

#define MODEM_PWRKEY 5
#define MODEM_TX 13
#define MODEM_RX 14

#define LED_TX -1
#define LED_RX 5

#define PPP_APN "internet"
#define PPP_USER ""
#define PPP_PASS ""

char send_aprs_table, send_aprs_symbol;

RTC_DATA_ATTR time_t systemUptime;
time_t wifiUptime = 0;

boolean KISS = false;
bool aprsUpdate = false;

boolean gotPacket = false;
AX25Msg incomingPacket;

bool lastPkg = false;
bool afskSync = false;
String lastPkgRaw = "";
float dBV = 0;
int mVrms = 0;

float vbat;
int battPercent = -1; // -1 = no battery connected (e.g. running on USB/external power only); updated in topBar()
bool powerCharging = false; // updated in topBar()
bool powerVbusIn = false;   // USB/external power present; updated in topBar()

long timeNetwork, timeAprs, timeGui;

unsigned long previousMillis = 0; // กำหนดตัวแปรเก็บค่า เวลาสุดท้ายที่ทำงาน
long interval = 10000;            // กำหนดค่าตัวแปร ให้ทำงานทุกๆ 10 วินาที
int conStat = 0;
int conStatNetwork = 0;

cppQueue PacketBuffer(sizeof(AX25Msg), 10, IMPLEMENTATION); // Instantiate queue

statusType status;
RTC_DATA_ATTR igateTLMType igateTLM;
#ifdef BOARD_HAS_PSRAM
txQueueType *txQueue;
#else
RTC_DATA_ATTR txQueueType txQueue[PKGTXSIZE];
#endif

RTC_DATA_ATTR double LastLat, LastLng;
RTC_DATA_ATTR time_t lastTimeStamp;

extern RTC_DATA_ATTR uint8_t digiCount;

String RF_VERSION;

XPowersAXP2101 PMU;

Configuration config;

pkgListType *pkgList;

TelemetryType *Telemetry;

TaskHandle_t taskNetworkHandle;
TaskHandle_t taskAPRSHandle;
TaskHandle_t mainDisplayHandle;
TaskHandle_t taskGpsHandle;
TaskHandle_t taskTNCHandle;

HardwareSerial SerialGPS(2);
NimBLEServer *pServer = NULL;
NimBLECharacteristic *pTxCharacteristic;
bool BTdeviceConnected = false;
bool BToldDeviceConnected = false;
uint8_t BTtxValue = 0;

// class MySecurity : public BLESecurityCallbacks
// {

//   uint32_t onPassKeyRequest()
//   {
//     ESP_LOGI(LOG_TAG, "PassKeyRequest");
//     return passkey;
//   }
//   void onPassKeyNotify(uint32_t pass_key)
//   {
//     ESP_LOGI(LOG_TAG, "The passkey Notify number:%d", pass_key);
//   }
//   bool onConfirmPIN(uint32_t pass_key)
//   {
//     ESP_LOGI(LOG_TAG, "The passkey YES/NO number:%d", pass_key);
//     vTaskDelay(5000);
//     return true;
//   }
//   bool onSecurityRequest()
//   {
//     ESP_LOGI(LOG_TAG, "SecurityRequest");
//     return true;
//   }

//   void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl)
//   {
//     ESP_LOGI(LOG_TAG, "Starting BLE!");
//   }
// };

class MyServerCallbacks : public NimBLEServerCallbacks
{
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo)
  {
    BTdeviceConnected = true;
    projLog(LOGCAT_BLUETOOTH, "BLE connected: %s", connInfo.getAddress().toString().c_str());
  };

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason)
  {
    BTdeviceConnected = false;
    projLog(LOGCAT_BLUETOOTH, "BLE disconnected: %s (reason %d)", connInfo.getAddress().toString().c_str(), reason);
  }
};

class MyCallbacks : public NimBLECharacteristicCallbacks
{
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
  {
    NimBLEAttValue rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
      // Serial.println("*********");
      // Serial.print("Received Value: ");
      char raw[500];
      int i = 0;
      memset(raw, 0, sizeof(raw));
      for (i = 0; i < rxValue.length(); i++)
      {
        if (i > sizeof(raw))
          break;
        raw[i] = (char)rxValue[i];
        if (raw[i] == 0)
          break;
      }
      if (config.bt_mode == 1)
      { // TNC2RAW MODE
        projLog(LOGCAT_BLUETOOTH, "BLE RX TNC2RAW: %d bytes", (int)strlen(raw));
        pkgTxPush(raw, strlen(raw), 1);
      }
      else if (config.bt_mode == 2)
      {
        // KISS MODE
        projLog(LOGCAT_BLUETOOTH, "BLE RX KISS: %d bytes", i);
        for (int n = 0; n < i; n++)
          kiss_serial((uint8_t)raw[n]);
      }
    }
  }
};

// Set your Static IP address for wifi AP
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 254);
IPAddress subnet(255, 255, 255, 0);

IPAddress vpn_IP(192, 168, 44, 195);

int pkgTNC_count = 0;


void GPS_INIT()
{
  SerialGPS.begin(9600, SERIAL_8N1, GNSS_RX, GNSS_TX);
  SerialGPS.flush();
  while (SerialGPS.available() > 0)
    SerialGPS.read();
  // SerialGPS.print("\r\n");
  // SerialGPS.print("$PMTK161,1*28\r\n");
  // SerialGPS.print("$PMTK225,0*2B\r\n");
  // SerialGPS.print("$PMTK869,1,0*34\r\n");
}

// SmartBeaconing(tm) algorithm adapted from HamHUD
char EVENT_TX_POSITION = 0;
unsigned char SB_SPEED = 0, SB_SPEED_OLD = 0;
int16_t SB_HEADING = 0;
uint16_t tx_interval = 0, igate_tx_interval, digi_tx_interval; // How often we transmit, in seconds
unsigned int tx_counter, igate_tx_counter, digi_tx_counter;    // Incremented every second
int16_t last_heading, heading_change;                          // Keep track of corner pegging
uint16_t trk_interval = 15;


// DST-aware replacement for the old fixed "config.timeZone * SECS_PER_HOUR"
// math: given a UTC epoch, returns the correct total UTC offset (base zone
// + DST if currently in effect) for that instant, per the POSIX TZ string
// applied by applyTimeZone(). Requires tzset() to have already run (it has,
// via applyTimeZone() at boot and on every change).
//
// This toolchain's struct tm doesn't expose tm_gmtoff (the __TM_GMTOFF
// guard in time.h is never defined here), so the offset is derived
// indirectly: localtime_r() still correctly applies TZ+DST to the
// broken-down fields even without that extra field, so re-run those fields
// back through makeTime() (TimeLib's naive calendar-to-epoch math, already
// used everywhere else in this codebase) as if they were UTC - the
// difference from the real UTC epoch is exactly the local offset.
long localTZOffsetSeconds(time_t utcTime)
{
  struct tm tmLocal;
  localtime_r(&utcTime, &tmLocal);
  tmElements_t te;
  te.Year = tmLocal.tm_year + 1900 - 1970;
  te.Month = tmLocal.tm_mon + 1;
  te.Day = tmLocal.tm_mday;
  te.Hour = tmLocal.tm_hour;
  te.Minute = tmLocal.tm_min;
  te.Second = tmLocal.tm_sec;
  return (long)makeTime(te) - (long)utcTime;
}

// Unwritten/never-saved EEPROM reads back as 0xFF, not 0x00 - a bare
// posixTZ[0]=='\0' check doesn't catch that (confirmed live: an EEPROM
// saved before this field existed loaded as a string of raw 0xFF bytes,
// not an empty one). A real POSIX TZ string is plain printable ASCII and
// null-terminated well within the buffer, so check for that instead.
bool isValidPosixTZ(const char *tz)
{
  size_t len = strnlen(tz, sizeof(config.posixTZ));
  if (len == 0 || len >= sizeof(config.posixTZ))
    return false;
  for (size_t i = 0; i < len; i++)
    if (tz[i] < 0x20 || tz[i] > 0x7E)
      return false;
  return true;
}

// Applies config.posixTZ to the C library (TZ env + tzset()) and (re)starts
// NTP against config.ntp_host. Call once at boot after EEPROM load, and
// again whenever posixTZ or ntp_host changes via console/web.
void applyTimeZone()
{
  if (!isValidPosixTZ(config.posixTZ))
  {
    strcpy(config.posixTZ, "UTC0"); // guard against garbage from an EEPROM saved before this field existed
    saveEEPROM();                   // one-time correction, so this doesn't need re-checking every boot
  }
  configTzTime(config.posixTZ, config.ntp_host);
}

time_t setGpsTime()
{
  time_t time;
  tmElements_t timeinfo;
  if (gps.time.isValid())
  {
    timeinfo.Year = (gps.date.year()) - 1970;
    timeinfo.Month = gps.date.month();
    timeinfo.Day = gps.date.day();
    timeinfo.Hour = gps.time.hour();
    timeinfo.Minute = gps.time.minute();
    timeinfo.Second = gps.time.second();
    time_t timeStamp = makeTime(timeinfo);
    time = timeStamp + localTZOffsetSeconds(timeStamp);
    setTime(time);
    // setTime(timeinfo.Hour,timeinfo.Minute,timeinfo.Second,timeinfo.Day, timeinfo.Month, timeinfo.Year);
    return time;
  }
  return 0;
}

time_t getGpsTime()
{
  time_t time;
  tmElements_t timeinfo;
  if (gps.time.isValid())
  {
    timeinfo.Year = (gps.date.year()) - 1970;
    timeinfo.Month = gps.date.month();
    timeinfo.Day = gps.date.day();
    timeinfo.Hour = gps.time.hour();
    timeinfo.Minute = gps.time.minute();
    timeinfo.Second = gps.time.second();
    time_t timeStamp = makeTime(timeinfo);
    time = timeStamp + localTZOffsetSeconds(timeStamp);
    return time;
  }
  return 0;
}


void aprs_msg_callback(struct AX25Msg *msg)
{
  AX25Msg pkg;

  memcpy(&pkg, msg, sizeof(AX25Msg));
  PacketBuffer.push(&pkg); // ใส่แพ็จเก็จจาก TNC ลงคิวบัพเฟอร์
  status.rxCount++;
  // String tnc2;
  //         packet2Raw(tnc2, pkg);
  //         log_d("RX: %s", tnc2.c_str());
}

void printTime()
{
  struct tm tmstruct;
  getLocalTime(&tmstruct, 500);
  Serial.print("[");
  Serial.print(tmstruct.tm_hour);
  Serial.print(":");
  Serial.print(tmstruct.tm_min);
  Serial.print(":");
  Serial.print(tmstruct.tm_sec);
  Serial.print("]");
}

uint8_t gwRaw[PKGLISTSIZE][66];
uint8_t gwRawSize[PKGLISTSIZE];
int gwRaw_count = 0, gwRaw_idx_rd = 0, gwRaw_idx_rw = 0;

void pushGwRaw(uint8_t *raw, uint8_t size)
{
  if (gwRaw_count > PKGLISTSIZE)
    return;
  if (++gwRaw_idx_rw >= PKGLISTSIZE)
    gwRaw_idx_rw = 0;
  if (size > 65)
    size = 65;
  memcpy(&gwRaw[gwRaw_idx_rw][0], raw, size);
  gwRawSize[gwRaw_idx_rw] = size;
  gwRaw_count++;
}

uint8_t popGwRaw(uint8_t *raw)
{
  uint8_t size = 0;
  if (gwRaw_count <= 0)
    return 0;
  if (++gwRaw_idx_rd >= PKGLISTSIZE)
    gwRaw_idx_rd = 0;
  size = gwRawSize[gwRaw_idx_rd];
  memcpy(raw, &gwRaw[gwRaw_idx_rd][0], size);
  if (gwRaw_count > 0)
    gwRaw_count--;
  return size;
}



WiFiClient aprsClient;
// Guards every aprsClient read/write - taskAPRS (core 0) and taskNetwork
// (core 1) both use it with no other synchronization, and WiFiClient isn't
// safe for concurrent access from two cores; unsynchronized writes were
// interleaving on the wire and getting silently dropped by APRS-IS (found
// 2026-09-27: device logged "Sent device info"/"Sent telemetry" but a live
// APRS-IS monitor showed neither packet ever arriving). Created in setup()
// before any task that touches aprsClient starts.
SemaphoreHandle_t aprsClientMutex;



String NMEA;


void setupSDCard()
{
  // SPI Bus
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  if (SD.begin(SD_CS, SPI))
  {
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE)
    {
      projLog(LOGCAT_SYSTEM, "No SD_MMC card attached");
      return;
    }
    else
    {
      projLog(LOGCAT_SYSTEM, "SD_MMC Card Type: ");
      if (cardType == CARD_MMC)
      {
        projLog(LOGCAT_SYSTEM, "MMC");
      }
      else if (cardType == CARD_SD)
      {
        projLog(LOGCAT_SYSTEM, "SDSC");
      }
      else if (cardType == CARD_SDHC)
      {
        projLog(LOGCAT_SYSTEM, "SDHC");
      }
      else
      {
        projLog(LOGCAT_SYSTEM, "UNKNOWN");
      }
      uint32_t cardSize = SD.cardSize() / (1024 * 1024);
      uint32_t cardTotal = SD.totalBytes() / (1024 * 1024);
      uint32_t cardUsed = SD.usedBytes() / (1024 * 1024);
      projLog(LOGCAT_SYSTEM, "SD Card Size: %u MB", cardSize);
      projLog(LOGCAT_SYSTEM, "Total space: %u MB", cardTotal);
      projLog(LOGCAT_SYSTEM, "Used space: %u MB", cardUsed);
    }
  }
}

long oledSleepTimeout = 0;
bool oledSleeping = false;
bool showDisp = false;

char *htmlBuffer;
void setup()
{
  initVersion();
  // systemUptime is RTC_DATA_ATTR, which survives the RTS-pin/software reset
  // esptool uses after flashing (only a true power-on reset clears it) - the
  // "if (systemUptime == 0)" first-NTP-sync capture further down (main loop)
  // silently no-ops if a stale non-zero value survived from before this
  // boot, so the dashboard/OLED uptime keeps counting from a previous boot
  // instead of this one. Force it back to 0 on every real boot so that
  // capture always fires fresh.
  systemUptime = 0;
  aprsClientMutex = xSemaphoreCreateMutex(); // before any task that touches aprsClient starts
  logInit();                                 // before any task that calls projLog() starts
  consoleLogInit();                          // before any task that calls projLog() starts (web_console.cpp)
  byte *ptr;
#ifdef BOARD_HAS_PSRAM
  pkgList = (pkgListType *)ps_malloc(sizeof(pkgListType) * PKGLISTSIZE);
  Telemetry = (TelemetryType *)malloc(sizeof(TelemetryType) * TLMLISTSIZE);
  txQueue = (txQueueType *)ps_malloc(sizeof(txQueueType) * PKGTXSIZE);
  // TNC2Raw = (int *)ps_malloc(sizeof(int) * PKGTXSIZE);
#else
  pkgList = (pkgListType *)malloc(sizeof(pkgListType) * PKGLISTSIZE);
  Telemetry = (TelemetryType *)malloc(sizeof(TelemetryType) * TLMLISTSIZE);
  txQueue = (txQueueType *)malloc(sizeof(txQueueType) * PKGTXSIZE);
  // TNC2Raw = (int *)malloc(sizeof(int) * PKGTXSIZE);
#endif

  memset(pkgList, 0, sizeof(pkgListType) * PKGLISTSIZE);
  memset(Telemetry, 0, sizeof(TelemetryType) * TLMLISTSIZE);
  memset(txQueue, 0, sizeof(txQueueType) * PKGTXSIZE);
  // memset(TNC2Raw, 0, sizeof(TNC2Raw) * PKGTXSIZE);

  pinMode(ENCODER_OK_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PTT_PIN, INPUT_PULLUP); // PTT BUTTON

  // Set up serial port
  Serial.begin(115200); // debug

  // Serial.println();
  projLog(LOGCAT_SYSTEM, "Start ESP32IGate V%s", String(VERSION).c_str());
  // log_d("Push BOOT after 3 sec for Factory Default config.");
  projLog(LOGCAT_SYSTEM, "Total heap: %d", ESP.getHeapSize());

  Wire.begin(I2C_SDA, I2C_SCL, 400000L);

  // Setup Power PMU AXP2101
  setupPower();
  setupSDCard();

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false); // initialize with the I2C addr 0x3C (for the 128x64)
  // Initialising the UI will init the display too.
  // clear the display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(0, 15);
  display.print("APRS");
  display.setCursor(55, 32);
  display.print("T-TWR+");
  display.drawYBitmap(0, 16, LOGO, 48, 48, WHITE);
  display.drawRoundRect(52, 16, 75, 22, 3, WHITE);

  display.setFont();
  display.setTextColor(WHITE);

  display.setCursor(60, 40);
  display.printf("V%s%c", VERSION, VERSION_BUILD); // "FW Ver " no longer fits: VERSION is now 8 chars (YYYYMMDD), was 3 ("0.4")
  display.setCursor(60, 55);
  display.print("Copy@2023");
  display.display();

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    projLog(LOGCAT_SYSTEM, "failed to initialise EEPROM"); // delay(100000);
  }

  delay(1000);

  if (digitalRead(ENCODER_OK_PIN) == LOW)
  {
    defaultConfig();
    projLog(LOGCAT_SYSTEM, "Manual Default configure!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(25, 25);
    display.println("Factory");
    display.setCursor(30, 45);
    display.print("RESET!");
    display.setFont();
    display.setTextColor(WHITE);
    display.display();
    delay(2000);
  }

  // ตรวจสอบคอนฟิกซ์ผิดพลาด
  ptr = (byte *)&config;
  EEPROM.readBytes(1, ptr, sizeof(Configuration));
  uint8_t chkSum = checkSum(ptr, sizeof(Configuration));
  projLog(LOGCAT_SYSTEM, "EEPROM Check %0Xh=%0Xh(%dByte)", EEPROM.read(0), chkSum, sizeof(Configuration));
  if (EEPROM.read(0) != chkSum)
  {
    projLog(LOGCAT_SYSTEM, "Config EEPROM Error!");
    display.clearDisplay();
    display.drawYBitmap(50, 0, iconAlert, 28, 28, WHITE);
    display.setCursor(25, 33);
    display.print("EEPROM Error!");
    display.setCursor(23, 45);
    display.print("Factory Reset");
    display.display();
    defaultConfig();
    delay(2000);
  }

  syslogReconnect(); // needs config loaded above; actual sending waits for WiFi regardless
  applyTimeZone();   // sets TZ env + tzset() so localTZOffsetSeconds()/DST are correct even before NTP/WiFi ever connects (e.g. for GPS-derived time)

  if (config.bt_master == true)
  {
    // Create the BLE Device
    NimBLEDevice::init(config.bt_name);
    // NimBLEDevice::setSecurityAuth(true, true, true);  // The line you told me to add
    // NimBLEDevice::setSecurityPasskey(config.bt_pin);

    // Create the BLE Server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    NimBLEService *pService = pServer->createService(config.bt_uuid);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        config.bt_uuid_tx,
        NIMBLE_PROPERTY::NOTIFY);
    // NimBLE manages the CCCD (2902) descriptor for NOTIFY/INDICATE characteristics
    // automatically - no manual addDescriptor() call needed (unlike Bluedroid's BLE2902).

    NimBLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        config.bt_uuid_rx,
        NIMBLE_PROPERTY::WRITE);

    // pRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    // NimBLE starts services automatically when the server starts (pService->start() is a no-op here)

    // Unlike the old Bluedroid-based BLE library, NimBLE-Arduino does not
    // include the device name or service UUID in the advertisement by
    // default (confirmed 2026-09-29: default advertisement was just the
    // AD flags, "02 01 06" - phones couldn't find/identify the device).
    // Must be set explicitly.
    pServer->getAdvertising()->setName(config.bt_name);
    pServer->getAdvertising()->addServiceUUID(config.bt_uuid);

    // Start advertising
    pServer->getAdvertising()->start();
  }

  RF_MODULE(true);

  display.clearDisplay();
  display.setTextSize(1);
  display.display();

  projLog(LOGCAT_SYSTEM, "Free heap: %d", ESP.getFreeHeap());
  projLog(LOGCAT_SYSTEM, "Total PSRAM: %d", ESP.getPsramSize());
  projLog(LOGCAT_SYSTEM, "Free PSRAM: %d", ESP.getFreePsram());
  // log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());

  // enableLoopWDT();
  // enableCore0WDT();
  // enableCore1WDT();

  // Task 1
  xTaskCreatePinnedToCore(
      taskAPRS,        /* Function to implement the task */
      "taskAPRS",      /* Name of the task */
      8192,            /* Stack size in words */
      NULL,            /* Task input parameter */
      3,               /* Priority of the task */
      &taskAPRSHandle, /* Task handle. */
      0);              /* Core where the task should run */

  xTaskCreatePinnedToCore(
      taskGPS,        /* Function to implement the task */
      "taskGPS",      /* Name of the task */
      4096,           /* Stack size in words */
      NULL,           /* Task input parameter */
      2,              /* Priority of the task */
      &taskGpsHandle, /* Task handle. */
      0);             /* Core where the task should run */

  if (config.wifi_mode != 0)
  {
    // Task 2
    xTaskCreatePinnedToCore(
        taskNetwork,        /* Function to implement the task */
        "taskNetwork",      /* Name of the task */
        8192,               /* Stack size in words */
        NULL,               /* Task input parameter */
        2,                  /* Priority of the task */
        &taskNetworkHandle, /* Task handle. */
        1);                 /* Core where the task should run */
  }
  // Task 3
  xTaskCreatePinnedToCore(
      mainDisp,           /* Function to implement the task */
      "mainDisplay",      /* Name of the task */
      16384,              /* Stack size in words */
      NULL,               /* Task input parameter */
      1,                  /* Priority of the task */
      &mainDisplayHandle, /* Task handle. */
      1);                 /* Core where the task should run */

  xTaskCreatePinnedToCore(
      taskTNC,        /* Function to implement the task */
      "taskTNC",      /* Name of the task */
      8192,           /* Stack size in words */
      NULL,           /* Task input parameter */
      1,              /* Priority of the task */
      &taskTNCHandle, /* Task handle. */
      0);             /* Core where the task should run */
}

int pkgCount = 0;



long sendTimer = 0;
unsigned long deviceInfoTickInterval = 0; // sendDeviceInfo()/sendDeviceTelemetry(), every 10 min while connected to APRS-IS
bool AFSKInitAct = false;
int btn_count = 0;
long timeCheck = 0;
bool afterVoice = false;

// USB-serial recovery console: works regardless of WiFi/config state (no
// dependency on the web UI or any task that's conditional on config), so
// it's the last resort if a bad config leaves the device unreachable over
// the network - e.g. the WiFi credentials themselves are wrong, or (see
// FORK_NOTES.md) a firmware update wiped config back to defaults before the
// backup/restore mechanism existed. "set" syntax modeled after the console
// convention used in npr-fw-freertos (github.com/llatva/npr-fw-freertos).
String consoleLine;
bool consolePromptShown = false;
bool consoleLastWasCR = false; // suppresses a \r\n pair triggering the line twice
void consolePoll()
{
  if (!consolePromptShown)
  {
    Serial.print("\r\nready> ");
    consolePromptShown = true;
  }
  while (Serial.available())
  {
    char c = Serial.read();
    // Accept \r, \n, or \r\n as a line ending - found 2026-09-29 live on
    // hardware: the previous code only treated bare \n as end-of-line and
    // silently discarded \r, so a terminal sending a bare \r on Enter (a
    // common convention) had every command silently swallowed, with no
    // visible reaction at all. Confirmed via a direct serial test: \r
    // alone did nothing, \n alone and \r\n both worked.
    if (c == '\r' || c == '\n')
    {
      if (c == '\n' && consoleLastWasCR)
      {
        consoleLastWasCR = false;
        continue; // second half of a \r\n pair - already handled by the \r
      }
      consoleLastWasCR = (c == '\r');
      consoleLine.trim();
      if (consoleLine.length() > 0)
      {
        if (consoleLine.startsWith("set wifi_ssid "))
        {
          strncpy(config.wifi_sta[0].wifi_ssid, consoleLine.c_str() + 14, sizeof(config.wifi_sta[0].wifi_ssid) - 1);
          config.wifi_sta[0].wifi_ssid[sizeof(config.wifi_sta[0].wifi_ssid) - 1] = 0;
          Serial.printf("\r\nwifi_ssid set to '%s' (not saved yet - run 'save')\r\n", config.wifi_sta[0].wifi_ssid);
        }
        else if (consoleLine.startsWith("set wifi_pass "))
        {
          strncpy(config.wifi_sta[0].wifi_pass, consoleLine.c_str() + 14, sizeof(config.wifi_sta[0].wifi_pass) - 1);
          config.wifi_sta[0].wifi_pass[sizeof(config.wifi_sta[0].wifi_pass) - 1] = 0;
          Serial.print("\r\nwifi_pass set (not saved yet - run 'save')\r\n");
        }
        else if (consoleLine.startsWith("set oled_timeout "))
        {
          // Display settings apply live + save immediately, no reboot -
          // unlike wifi_ssid/wifi_pass, nothing here needs a network
          // restart to take effect, and the whole point is to see the
          // change on the physical screen right away.
          int val = consoleLine.substring(17).toInt();
          config.oled_timeout = (val < 0) ? 0 : val;
          oledWake(); // reset the countdown against the new value, wake if currently off
          saveEEPROM();
          Serial.printf("\r\noled_timeout set to %d sec and saved (0 = never sleep)\r\n", config.oled_timeout);
        }
        else if (consoleLine.startsWith("set dim "))
        {
          int val = consoleLine.substring(8).toInt();
          config.dim = (val < 0 || val > 4) ? 0 : val;
          // Don't touch display.* from this task - mainDisp owns the I2C
          // bus (see gui_lcd.h). Just flag it; mainDisp applies it next frame.
          displayApplyPending = true;
          oledWake(); // a brightness change means nothing if the panel is asleep
          saveEEPROM();
          Serial.printf("\r\ndim mode set to %d and saved (0=HI 1=LOW 2=AUTO 3=DAY/NIGHT 4=CONTRAST)\r\n", config.dim);
        }
        else if (consoleLine.startsWith("set contrast "))
        {
          int val = consoleLine.substring(13).toInt();
          config.contrast = (val < 0) ? 0 : (val > 200 ? 200 : val);
          displayApplyPending = true; // same - mainDisp applies it, not this task
          oledWake(); // same - don't let a sleeping panel mask the change
          saveEEPROM();
          Serial.printf("\r\ncontrast set to %d and saved%s\r\n", config.contrast, (config.dim == 4) ? "" : " (won't be visible until dim mode is set to 4/CONTRAST)");
        }
        else if (consoleLine.startsWith("set posixtz "))
        {
          String val = consoleLine.substring(12);
          if (val.length() >= sizeof(config.posixTZ) || !isValidPosixTZ(val.c_str()))
          {
            Serial.printf("\r\nInvalid TZ string (max %d printable chars, e.g. CET-1CEST,M3.5.0,M10.5.0/3)\r\n", sizeof(config.posixTZ) - 1);
          }
          else
          {
            strncpy(config.posixTZ, val.c_str(), sizeof(config.posixTZ) - 1);
            config.posixTZ[sizeof(config.posixTZ) - 1] = 0;
            applyTimeZone();
            saveEEPROM();
            Serial.printf("\r\nposixtz set to '%s' and saved\r\n", config.posixTZ);
          }
        }
        else if (consoleLine == "save")
        {
          config.wifi_sta[0].enable = true;
          config.wifi_mode = WIFI_AP_STA_FIX; // keep the AP reachable too, in case the new STA credentials are also wrong
          saveEEPROM();
          Serial.print("\r\nSaved. Rebooting...\r\n");
          delay(200);
          ESP.restart();
        }
        else if (consoleLine == "show")
        {
          Serial.printf("\r\nwifi_ssid='%s' wifi_pass='%s' wifi_mode=%d oled_timeout=%d dim=%d contrast=%d posixtz='%s'\r\n",
                         config.wifi_sta[0].wifi_ssid, config.wifi_sta[0].wifi_pass, config.wifi_mode,
                         config.oled_timeout, config.dim, config.contrast, config.posixTZ);
        }
        else if (consoleLine == "help")
        {
          Serial.print("\r\nCommands: set wifi_ssid <val> | set wifi_pass <val> | save | "
                        "set oled_timeout <sec> | set dim <0-4> | set contrast <0-200> | "
                        "set posixtz <POSIX TZ string, e.g. CET-1CEST,M3.5.0,M10.5.0/3> | show | help\r\n");
        }
        else
        {
          Serial.print("\r\nUnknown command - try 'help'\r\n");
        }
      }
      consoleLine = "";
      Serial.print("\r\nready> ");
    }
    else
    {
      consoleLastWasCR = false; // any real character breaks a pending \r..\n pairing
      consoleLine += c;
      Serial.write(c); // echo - some terminals don't echo locally, leaving typed input invisible
    }
  }
}

void loop()
{
  vTaskDelay(10 / portTICK_PERIOD_MS);
  consolePoll();

  if (ESP.getFreeHeap() < 60000)
    ESP.restart();

  if (config.bt_master)
  {
    // disconnecting
    if (!BTdeviceConnected && BToldDeviceConnected)
    {
      delay(500);                  // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      BToldDeviceConnected = BTdeviceConnected;
    }
    // connecting
    if (BTdeviceConnected && !BToldDeviceConnected)
    {
      // do stuff here on connecting
      BToldDeviceConnected = BTdeviceConnected;
    }
  }

  // PTT push to FM Voice
  if (digitalRead(BUTTON_PTT_PIN) == LOW)
  {
    pttStat = 1;
    // setTransmit(true);
    AFSKInitAct = false;
    delay(50);

    digitalWrite(POWER_PIN, config.rf_power); // RF Power
    // sa868.setTxPower(sa868._config.rf_power);
    digitalWrite(SA868_MIC_SEL, LOW); // Select = MIC
    digitalWrite(SA868_PTT_PIN, LOW); // PTT RF
    if (config.rf_type == RF_SA8x8_OpenEdit)
    {
      sa868.TxOn();
    }
    delay(500);
    LED_Color(100, 50, 0);
    while (digitalRead(BUTTON_PTT_PIN) == LOW)
    {
      pttStat++;
      delay(100);
    }
    pttStat = 0;
    burstAfterVoice();
    char sts[50];
    if (gps.location.isValid() && (gps.hdop.hdop() < 10.0))
      sprintf(sts, "POSITION GPS\nSPD %dkPh/%d\n", SB_SPEED, SB_HEADING);
    else
      sprintf(sts, "POSITION GPS\nGPS INVALID\n");
    pushTxDisp(TXCH_RF, "After Voice", sts);
    LED_Color(0, 0, 0);
    AFSKInitAct = true;
    if (config.rf_type == RF_SA8x8_OpenEdit)
    {
      delay(200);
      sa868.TxOff();
      sa868.setLowPower();
      delay(1000);
      sa868.RxOn();
    }
    // setTransmit(false);
  }

  if (digitalRead(0) == LOW)
  {
    btn_count++;
    if (btn_count > 1000) // Push BOOT 10sec
    {
      // digitalWrite(LED_PIN, HIGH);
      //  digitalWrite(LED_TX_PIN, HIGH);
    }
  }
  else
  {
    if (btn_count > 0)
    {
      // Serial.printf("btn_count=%dms\n", btn_count * 10);
      if (btn_count > 1000) // Push BOOT 10sec to Factory Default
      {
        // digitalWrite(LED_RX, LOW);
        // digitalWrite(LED_TX, LOW);
        // defaultConfig();
        projLog(LOGCAT_SYSTEM, "SYSTEM REBOOT NOW!");
        // esp_restart();
      }
      else
      {
        EVENT_TX_POSITION = 1;
      }
      btn_count = 0;
    }
  }

  if (millis() > timeCheck)
  {
    // log_d("taskAPRS: %d mS\ttaskNetwork: %d mS\ttaskGUI: %d mS\n", timeAprs, timeNetwork, timeGui);
    timeCheck = millis() + 1000;
    // if (ESP.getFreeHeap() < 60000)
    //     esp_restart();
    // Serial.println(String(ESP.getFreeHeap()));
    // Popup Display
    // if (dispBuffer.getCount() > 0)
    // {
    //     if (millis() > timeHalfSec)
    //     {
    //         char tnc2[300];
    //         dispBuffer.pop(&tnc2);
    //         dispWindow(String(tnc2), 0, false);
    //     }
    // }
    // else
    // {
    //     // Sleep display
    //     if (millis() > timeHalfSec)
    //     {
    //         if (timeHalfSec > 0)
    //         {
    //             timeHalfSec = 0;
    //             oledSleepTimeout = millis() + (config.oled_timeout * 1000);
    //         }
    //         else
    //         {
    //             if (millis() > oledSleepTimeout && oledSleepTimeout > 0)
    //             {
    //                 oledSleepTimeout = 0;
    //                 display.clearDisplay();
    //                 display.display();
    //             }
    //         }
    //     }
    // }
  }
}


void taskGPS(void *pvParameters)
{
  GPS_INIT();
  for (;;)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);

    while (SerialGPS.available())
      gps.encode(SerialGPS.read());

    if (gps.time.isValid() && gps.time.isUpdated())
    {
      time_t timeGps = getGpsTime(); // Local gps time
      time_t nowTime;
      time(&nowTime);
      // nowTime is UTC (set by settimeofday below) while timeGps is local
      // (see getGpsTime()), so this also fires on the first sync after
      // boot, not only on real drift - intentional, matches the upstream
      // fix this replaces the once-only firstGpsTime gate with.
      if (timeGps > 1700000000 && abs((long)(timeGps - nowTime)) > 5)
      {
        setTime(timeGps);
        // Same first-sync capture as the NTP path (main loop) - without
        // WiFi/NTP this is the only place systemUptime ever gets set, so on
        // a WiFi-less GPS-only deployment it would otherwise stay 0 forever
        // and the dashboard/OLED uptime would show the full wall-clock time
        // as "days" instead of actual time since boot. timeGps is already
        // local wall-clock (see getGpsTime()), matching what now() reads.
        if (systemUptime == 0)
        {
          systemUptime = timeGps;
        }
        time_t rtc = timeGps - localTZOffsetSeconds(timeGps);
        timeval tv = {rtc, 0};
        timezone tz = {0, 0}; // ignored by the actual localtime()/DST machinery - TZ env (applyTimeZone()) is what matters
        settimeofday(&tv, &tz);
#ifdef DEBUG
        projLog(LOGCAT_GPS, "SET GPS Timestamp = %u Year=%d", timeGps, year());
#endif
      }
    }
  }
}

extern cppQueue adcq;
void taskTNC(void *pvParameters)
{
  for (;;)
  {
    if (AFSKInitAct == true)
      AFSK_Poll(true, config.rf_power);
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

String getPath(int idx)
{
  String ret = "";
  switch (idx)
  {
  case 0: // OFF
    ret = "";
    break;
  case 1: // TRACE1-1
    ret = "TRACE1-1";
    break;
  case 2:
    ret = "TRACE2-2";
    break;
  case 3:
    ret = "TRACE3-3";
    break;
  case 4: // WIDE1-1 - default for iGate/DIGI, see PATH_DEFAULT_FIXED
    ret = "WIDE1-1";
    break;
  case 5: // WIDE1-1,WIDE2-1 - default for Tracker, see PATH_DEFAULT_MOBILE
    ret = "WIDE1-1,WIDE2-1";
    break;
  case 6:
    ret = "RFONLY";
    break;
  case 7:
    ret = "RELAY";
    break;
  case 8:
    ret = "GATE";
    break;
  case 9: // UserDefine1
    ret = String(config.path[0]);
    break;
  case 10: // UserDefine2
    ret = String(config.path[1]);
    break;
  case 11: // UserDefine3
    ret = String(config.path[2]);
    break;
  case 12: // UserDefine4
    ret = String(config.path[3]);
    break;
  default:
    ret = "WIDE1-1";
    break;
  }
  return ret;
}

long timeSlot;
bool initInterval = true;
void taskAPRS(void *pvParameters)
{
  char sts[50];
  unsigned long tickInterval = 0;
  unsigned long iGatetickInterval = 0;
  unsigned long DiGiInterval = 0;

  PacketBuffer.clean();

  afskSetDCOffset(1740);
  afskSetADCAtten(3);
  APRS_init();
  APRS_setCallsign(config.aprs_mycall, config.aprs_ssid);
  // APRS_setPath1(config.igate_path, 1);
  APRS_setPreamble(300);
  APRS_setTail(0);
  sendTimer = millis() - (config.igate_interval * 1000) + 30000;
  igateTLM.TeleTimeout = millis() + 60000; // 1Min
  AFSKInitAct = true;
  timeSlot = millis();
  timeAprs = 0;
  afskSetHPF(config.audio_hpf);
  afskSetBPF(config.audio_bpf);
  // AFSK_TimerEnable(true);

  unsigned long timeAprsOld = millis();

  initInterval = true;
  tx_interval = config.trk_interval;
  tx_counter = tx_interval - 10;
  projLog(LOGCAT_SYSTEM, "Task APRS has been start");
  for (;;)
  {
    unsigned long now = millis();
    if (initInterval)
    {
      tickInterval = DiGiInterval = iGatetickInterval = millis() + 15000;
      initInterval = false;
    }
    timeAprs = now - timeAprsOld;
    timeAprsOld = now;
    // wdtSensorTimer = now;
    time_t timeStamp;
    time(&timeStamp);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // serviceHandle();

    if (AFSKInitAct == false)
    {
      continue;
    }

    if (config.rf_en)
    { // RF Module enable
      // SEND RF in time slot
      if (now > (timeSlot + 10))
      {
        // Transmit in timeslot if enabled
        // if (config.gpio_sql_pin > -1)
        // { // Set SQL pin
        //   if (!digitalRead(config.gpio_sql_pin))
        //   { // RX State Fail
        //     if (pkgTxSend())
        //       timeSlot = millis() + config.tx_timeslot; // Tx Time Slot >2sec.
        //     else
        //       timeSlot = millis() + 3000;
        //   }
        //   else
        //   {
        //     timeSlot = millis() + 1500;
        //   }
        // }
        // else
        // {
        //   if (pkgTxSend())
        //     timeSlot = millis() + config.tx_timeslot; // Tx Time Slot > 2sec.
        //   else
        //     timeSlot = millis() + 3000;
        // }
        if (pkgTxSend())
          timeSlot = millis() + config.tx_timeslot; // Tx Time Slot > 2sec.
        else
          timeSlot = millis() + 3000;
      }
    }

    if (config.trk_en)
    { // TRACKER MODE

      if (millis() > tickInterval)
      {
        tickInterval = millis() + 1000;

        tx_counter++;
        // log_d("TRACKER tx_counter=%d\t INTERVAL=%d\n", tx_counter, tx_interval);
        // Outside of Smart Beacon (or without a GPS fix to drive it), tx_interval
        // must always mirror config.trk_interval - otherwise it can be left
        // stuck at whatever Smart Beacon last computed (e.g. trk_slowinterval
        // while stationary) for up to that many seconds after Smart Beacon gets
        // switched off, since the "else" branches below previously only
        // resynced it once the stale interval had already elapsed once.
        if (!(config.trk_smartbeacon && config.trk_gps))
        {
          tx_interval = config.trk_interval;
        }
        // Recompute tx_interval from current speed/heading BEFORE the interval
        // timeout check below runs, so switching Smart Beacon back on doesn't
        // fire one immediate beacon against the stale interval it inherited
        // from plain/fixed mode (e.g. a short test interval) before this tick
        // gets a chance to settle it onto Slow/Max Interval.
        if (config.trk_gps && gps.speed.isValid() && gps.location.isValid() && gps.course.isValid() && (gps.hdop.hdop() < 10.0) && (gps.satellites.value() > 3))
        {
          SB_SPEED_OLD = SB_SPEED;
          if (gps.speed.isUpdated())
            SB_SPEED = (unsigned char)gps.speed.kmph();
          if (gps.course.isUpdated())
            SB_HEADING = (int16_t)gps.course.deg();
          if (config.trk_smartbeacon) // SMART BEACON CAL
          {
            if (SB_SPEED < config.trk_lspeed && SB_SPEED_OLD > config.trk_lspeed) // Speed slow down to STOP
            {                                                                     // STOPING
              SB_SPEED_OLD = 0;
              if (tx_counter > config.trk_mininterval)
                EVENT_TX_POSITION = 7;
              tx_interval = config.trk_slowinterval;
            }
            else
            {
              smartbeacon();
            }
          }
          else if (tx_counter > tx_interval)
          { // send gps location
            EVENT_TX_POSITION = 8;
            tx_interval = config.trk_interval;
          }
        }

        //  Check interval timeout
        if (config.trk_smartbeacon && config.trk_gps)
        {
          if (tx_counter > tx_interval)
          {
            if (tx_counter > config.trk_mininterval)
              EVENT_TX_POSITION = 4;
          }
          else
          {
            if (tx_counter >= (tx_interval + 5))
            {
              EVENT_TX_POSITION = 5;
            }
          }
        }
        else if (tx_counter > tx_interval)
        {
          EVENT_TX_POSITION = 6;
          tx_interval = config.trk_interval;
        }
      }

      if (EVENT_TX_POSITION > 0)
      {
        String rawData;
        String cmn = "";
        if (config.trk_sat)
          cmn += "SAT:" + String(gps.satellites.value()) + ",HDOP:" + String(gps.hdop.hdop(), 1);
        if (config.trk_bat)
        {
          if (config.trk_sat)
            cmn += ",";
          cmn += "BAT:" + String((float)PMU.getBattVoltage() / 1000, 1) + "V";
          // cmn += "BAT:" + String(vbat, 1) + "V";
        }
        if (config.trk_gps) // TRACKER by GPS
        {
          rawData = trk_gps_postion(cmn);
        }
        else // TRACKER by FIX position
        {
          rawData = trk_fix_position(cmn);
        }

        projLog(LOGCAT_APRS_RF, "TRACKER RAW: %s", rawData.c_str());
        projLog(LOGCAT_APRS_RF, "TRACKER EVENT_TX_POSITION=%d\t INTERVAL=%d", EVENT_TX_POSITION, tx_interval);
        tx_counter = 0;
        EVENT_TX_POSITION = 0;
        last_heading = SB_HEADING;

        if (config.trk_gps)
        {
          if (gps.location.isValid() && (gps.hdop.hdop() < 10.0))
            sprintf(sts, "POSITION GPS\nSPD %dkPh/%d\nINTERVAL %ds", SB_SPEED, SB_HEADING, tx_interval);
          else
            sprintf(sts, "POSITION GPS\nGPS INVALID\nINTERVAL %ds", tx_interval);
        }
        else
        {
          sprintf(sts, "POSITION FIX\nINTERVAL %ds", tx_interval);
        }

        if (config.trk_loc2rf)
        { // TRACKER SEND TO RF
          char *rawP = (char *)malloc(rawData.length());
          memcpy(rawP, rawData.c_str(), rawData.length());
          // rawData.toCharArray(rawP, rawData.length());
          pkgTxPush(rawP, rawData.length(), 0);
          pushTxDisp(TXCH_RF, "TX TRACKER", sts);
          free(rawP);
        }
        if (config.trk_loc2inet)
        { // TRACKER SEND TO APRS-IS
          xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
          bool sent = aprsClient.connected();
          if (sent)
            aprsClient.println(rawData); // Send packet to Inet
          xSemaphoreGive(aprsClientMutex);
          if (sent)
            pushTxDisp(TXCH_TCP, "TX TRACKER", sts);
        }
      }
    }

    // LOAD DATA incomming
    bool newIGatePkg = false;
    bool newDigiPkg = false;
    if (PacketBuffer.getCount() > 0)
    {
      String tnc2;
      // นำข้อมูลแพ็จเกจจาก TNC ออกจากคิว
      PacketBuffer.pop(&incomingPacket);
      // igateProcess(incomingPacket);
      packet2Raw(tnc2, incomingPacket);
      newIGatePkg = true;
      newDigiPkg = true;
      if (config.bt_master)
      { // Output TNC2RAW to BT Serial
        if (BTdeviceConnected)
        {
          if (config.bt_mode == 1)
          {
            char *rawP = (char *)malloc(tnc2.length());
            memcpy(rawP, tnc2.c_str(), tnc2.length());
            pTxCharacteristic->setValue((uint8_t *)rawP, tnc2.length());
            pTxCharacteristic->notify();
            free(rawP);
            projLog(LOGCAT_BLUETOOTH, "BLE TX TNC2RAW: %d bytes", (int)tnc2.length());
          }
          else if (config.bt_mode == 2)
          { // KISS
            uint8_t pkg[500];
            int sz = kiss_wrapper(pkg);
            pTxCharacteristic->setValue(pkg, sz);
            pTxCharacteristic->notify();
            projLog(LOGCAT_BLUETOOTH, "BLE TX KISS: %d bytes", sz);
          }
        }
      }

      projLog(LOGCAT_APRS_RF, "RX: %s", tnc2.c_str());

      uint16_t type = pkgType((char *)incomingPacket.info);
      char call[11];
      if (incomingPacket.src.ssid > 0)
        sprintf(call, "%s-%d", incomingPacket.src.call, incomingPacket.src.ssid);
      else
        sprintf(call, "%s", incomingPacket.src.call);

      // +1 and an explicit terminator: pkgListUpdate() (and everything
      // downstream - handle_lastHeard(), gui_lcd.cpp - via strlen(pkg.raw))
      // treats this as a null-terminated C string. Without the +1 here,
      // this malloc had exactly zero room for a terminator, so strlen()
      // read past the allocation into whatever adjacent heap bytes
      // happened to be there - a heap buffer over-read, not just a logic
      // bug, and the actual source of the "real comment + garbage tail"
      // reports (see FORK_NOTES.md's "never trust a packet-derived length
      // as a buffer size" - same rule, this time on the allocation size
      // rather than a copy length).
      char *rawP = (char *)malloc(tnc2.length() + 1);
      memcpy(rawP, tnc2.c_str(), tnc2.length());
      rawP[tnc2.length()] = 0;
      int idx = pkgListUpdate(call, rawP, type, 0);
      free(rawP);
      if (idx > -1)
      {

        if (config.rx_display && config.dispRF && (type & config.dispFilter))
        {
          pushTNC2Raw(idx);
          projLog(LOGCAT_APRS_RF, "RF_putQueueDisp:[pkgList_idx=%d,Type=%d] %s", idx, type, call);
        }
      }

      lastPkg = true;
      lastPkgRaw = tnc2;
      // ESP_BT.println(tnc2);
      status.allCount++;
    }

    // IGate Process
    if (config.igate_en)
    {
      // IGATE Position
      if (config.igate_bcn)
      {
        if (millis() > iGatetickInterval)
        {

          String rawData = "";
          if (config.igate_gps)
          { // IGATE Send GPS position
            if (gps.location.isValid() && (gps.hdop.hdop() < 10.0))
              rawData = igate_position(gps.location.lat(), gps.location.lng(), gps.altitude.meters(), "");
          }
          else
          { // IGATE Send fix position
            rawData = igate_position(config.igate_lat, config.igate_lon, config.igate_alt, "");
          }
          if (rawData != "")
          {
            iGatetickInterval = millis() + (config.igate_interval * 1000);
            projLog(LOGCAT_APRS_RF, "IGATE_POSITION: %s", rawData.c_str());

            if (config.igate_gps)
              sprintf(sts, "POSITION GPS\nINTERVAL %ds", tx_interval);
            else
              sprintf(sts, "POSITION FIX\nINTERVAL %ds", tx_interval);
            if (config.igate_loc2rf)
            { // IGATE SEND POSITION TO RF
              char *rawP = (char *)malloc(rawData.length());
              // rawData.toCharArray(rawP, rawData.length());
              memcpy(rawP, rawData.c_str(), rawData.length());
              pkgTxPush(rawP, rawData.length(), 0);
              pushTxDisp(TXCH_RF, "TX IGATE", sts);
              free(rawP);
            }
            if (config.igate_loc2inet)
            { // IGATE SEND TO APRS-IS
              xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
              bool sent = aprsClient.connected();
              if (sent)
                aprsClient.println(rawData); // Send packet to Inet
              xSemaphoreGive(aprsClientMutex);
              if (sent)
              {
                status.txCount++;
                pushTxDisp(TXCH_TCP, "TX IGATE", sts);
              }
            }
          }
        }
      }
      // IGATE send to inet
      if (newIGatePkg)
      {
        newIGatePkg = false;
        if (config.rf2inet && aprsClient.connected())
        {
          int ret = 0;
          uint16_t type = pkgType((const char *)&incomingPacket.info[0]);
          // IGate Filter RF->INET
          if ((type & config.rf2inetFilter))
            ret = igateProcess(incomingPacket);
          if (ret == 0)
          {
            status.dropCount++;
            igateTLM.DROP++;
          }
          else
          {
            status.rf2inet++;
            igateTLM.RF2INET++;
            igateTLM.TX++;
          }
        }
      }
    }

    // Digi Repeater Process
    if (config.digi_en)
    {
      // DIGI Position
      if (config.digi_bcn)
      {
        if (millis() > DiGiInterval)
        {

          String rawData;
          if (config.digi_gps)
          { // DIGI Send GPS position
            if (gps.location.isValid() && (gps.hdop.hdop() < 10.0))
              rawData = digi_position(gps.location.lat(), gps.location.lng(), gps.altitude.meters(), "");
          }
          else
          { // DIGI Send fix position
            rawData = digi_position(config.digi_lat, config.digi_lon, config.digi_alt, "");
          }
          if (rawData != "")
          {
            DiGiInterval = millis() + (config.digi_interval * 1000);
            projLog(LOGCAT_APRS_RF, "DIGI_POSITION: %s", rawData.c_str());

            if (config.digi_gps)
              sprintf(sts, "POSITION GPS\nINTERVAL %ds", tx_interval);
            else
              sprintf(sts, "POSITION FIX\nINTERVAL %ds", tx_interval);
            if (config.digi_loc2rf)
            { // DIGI SEND POSITION TO RF
              char *rawP = (char *)malloc(rawData.length());
              // rawData.toCharArray(rawP, rawData.length());
              memcpy(rawP, rawData.c_str(), rawData.length());
              pkgTxPush(rawP, rawData.length(), 0);
              pushTxDisp(TXCH_RF, "TX DIGI POS", sts);
              free(rawP);
            }
            if (config.digi_loc2inet)
            { // DIGI SEND TO APRS-IS
              xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
              bool sent = aprsClient.connected();
              if (sent)
                aprsClient.println(rawData); // Send packet to Inet
              xSemaphoreGive(aprsClientMutex);
              if (sent)
              {
                status.txCount++;
                pushTxDisp(TXCH_TCP, "TX DIGI POS", sts);
              }
            }
          }
        }
      }

      // Repeater packet
      if (newDigiPkg)
      {
        newDigiPkg = false;
        uint16_t type = pkgType((const char *)&incomingPacket.info[0]);
        // Digi repeater filter
        if ((type & config.digiFilter))
        {
          // Packet recheck
          pkgTxDuplicate(incomingPacket); // Search duplicate in tx and drop packet for renew
          int dlyFlag = digiProcess(incomingPacket);
          if (dlyFlag > 0)
          {
            int digiDelay;
            status.digiCount++;
            if (dlyFlag == 1)
            {
              digiDelay = 0;
            }
            else
            {
              if (config.digi_delay == 0)
              { // Auto mode
                // if (digiCount > 20)
                //   digiDelay = random(5000);
                // else if (digiCount > 10)
                //   digiDelay = random(3000);
                // else if (digiCount > 0)
                //   digiDelay = random(1500);
                // else
                digiDelay = random(100);
              }
              else
              {
                digiDelay = random(config.digi_delay);
              }
            }

            String digiPkg;
            packet2Raw(digiPkg, incomingPacket);
            projLog(LOGCAT_APRS_RF, "DIGI_REPEAT: %s", digiPkg.c_str());
            projLog(LOGCAT_APRS_RF, "DIGI delay=%d ms.", digiDelay);
            char *rawP = (char *)malloc(digiPkg.length());
            // digiPkg.toCharArray(rawP, digiPkg.length());
            memcpy(rawP, digiPkg.c_str(), digiPkg.length());
            pkgTxPush(rawP, digiPkg.length(), digiDelay);
            sprintf(sts, "--src call--\n%s\nDelay: %dms.", incomingPacket.src.call, digiDelay);
            pushTxDisp(TXCH_DIGI, "DIGI REPEAT", sts);
            free(rawP);
          }
        }
      }
    }
  }
}

int mqttRetry = 0;
long wifiTTL = 0;
WiFiMulti wifiMulti;

// WiFi connect timeout per AP. Increase when connecting takes longer.
const uint32_t connectTimeoutMs = 10000;
uint8_t APStationNum = 0;

// void Wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info){
//   log_d("Successfully connected to Access Point");
// }

// void Get_IPAddress(WiFiEvent_t event, WiFiEventInfo_t info){
//   log_d("WIFI is connected!");
//   log_d("IP address: %s",WiFi.localIP().toString().c_str());
// }

// void Wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info){
//   log_d("Disconnected from WIFI access point\n");
//   log_d("WiFi lost connection. Reason: ");
//   log_d("%s\n",info.wifi_sta_disconnected.reason);
//   log_d("Reconnecting...");
// }

void taskNetwork(void *pvParameters)
{
  projLog(LOGCAT_SYSTEM, "Task Network has been start");

  // WiFi.onEvent(Wifi_connected,SYSTEM_EVENT_STA_CONNECTED);
  // WiFi.onEvent(Get_IPAddress, SYSTEM_EVENT_STA_GOT_IP);
  // WiFi.onEvent(Wifi_disconnected, SYSTEM_EVENT_STA_DISCONNECTED);

  if (config.wifi_mode == WIFI_STA_FIX)
  { /**< WiFi station mode */
    WiFi.mode(WIFI_MODE_STA);
  }
  else if (config.wifi_mode == WIFI_AP_FIX)
  { /**< WiFi soft-AP mode */
    WiFi.mode(WIFI_MODE_AP);
  }
  if (config.wifi_mode == WIFI_AP_STA_FIX)
  { /**< WiFi station + soft-AP mode */
    WiFi.mode(WIFI_MODE_APSTA);
  }
  else
  {
    WiFi.mode(WIFI_MODE_NULL);
  }

  if (config.wifi_mode & WIFI_STA_FIX)
  {
    for (int i = 0; i < 5; i++)
    {
      if (config.wifi_sta[i].enable)
      {
        wifiMulti.addAP(config.wifi_sta[i].wifi_ssid, config.wifi_sta[i].wifi_pass);
      }
    }
    WiFi.setTxPower((wifi_power_t)config.wifi_power);
    WiFi.setHostname("ESP32APRS_T-TWR");
  }

  if (config.wifi_mode & WIFI_AP_FIX)
  {
    // กำหนดค่าการทำงานไวไฟเป็นแอสเซสพ้อย
    WiFi.softAP(config.wifi_ap_ssid, config.wifi_ap_pass); // Start HOTspot removing password will disable security
    WiFi.softAPConfig(local_IP, gateway, subnet);
    Serial.print("Access point running. IP address: ");
    Serial.print(WiFi.softAPIP());
    Serial.println("");
    // webService();
  }

  if (wifiMulti.run() == WL_CONNECTED)
  {
    // Serial.println("");
    projLog(LOGCAT_SYSTEM, "Wi-Fi CONNECTED!");
    projLog(LOGCAT_SYSTEM, "IP address: %s", WiFi.localIP().toString().c_str());
    // Single grep-able banner line - version + IP together, so both are
    // readable from a serial capture without cross-referencing two log
    // lines from different points in setup()/taskNetwork().
    projLog(LOGCAT_SYSTEM, "BOOT_INFO: version=V%s%c ip=%s", VERSION, VERSION_BUILD, WiFi.localIP().toString().c_str());
    webService();
    NTP_Timeout = millis() + 2000;
  }

  pingTimeout = millis() + 10000;
  unsigned long timeNetworkOld = millis();
  timeNetwork = 0;
  if (config.wifi_mode & WIFI_AP_STA_FIX)
    webService();
  for (;;)
  {
    unsigned long now = millis();
    timeNetwork = now - timeNetworkOld;
    timeNetworkOld = now;
    // wdtNetworkTimer = millis();
    serviceHandle();
    vTaskDelay(10 / portTICK_PERIOD_MS);

    if (config.wifi_mode & WIFI_AP_FIX)
    {
      APStationNum = WiFi.softAPgetStationNum();
      if (APStationNum > 0)
      {
        if (WiFi.isConnected() == false)
        {
          vTaskDelay(9 / portTICK_PERIOD_MS);
          continue;
        }
      }
    }

    // if (WiFi.status() == WL_CONNECTED)
    if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED)
    {
      // webService();
      // serviceHandle();

      if (millis() > NTP_Timeout)
      {
        NTP_Timeout = millis() + 86400000;
        // Serial.println("Config NTP");
        // setSyncProvider(getNtpTime);
        projLog(LOGCAT_SYSTEM, "Contacting Time Server");
        applyTimeZone();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        time_t systemTime;
        time(&systemTime); // true UTC
        // Shift to match the GPS path's convention (setGpsTime()/getGpsTime()):
        // TimeLib's "now" holds local wall time, not UTC - display/APRS
        // timestamp code elsewhere already relies on that. Without this
        // shift, TimeLib would silently disagree with itself depending on
        // whether GPS or NTP synced last.
        setTime(systemTime + localTZOffsetSeconds(systemTime));
        if (systemUptime == 0)
        {
          // ::now() (TimeLib), not time(NULL): systemUptime is diffed
          // against now() at display time (webservice.cpp/gui_lcd.cpp),
          // which is TimeLib's local-wall-clock value set just above -
          // time(NULL) is the ESP-IDF system clock, still true UTC, so using
          // it here made every uptime reading off by exactly the timezone
          // offset. Explicit :: because this loop shadows now() with its own
          // "unsigned long now = millis()" a few lines up.
          systemUptime = ::now();
        }
        pingTimeout = millis() + 2000;
        if (config.vpn)
        {
          if (!wireguard_active())
          {
            projLog(LOGCAT_SYSTEM, "Setup Wiregurad VPN!");
            wireguard_setup();
          }
        }
      }

      if (config.igate_en)
      {
        xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
        bool isConnected = aprsClient.connected();
        xSemaphoreGive(aprsClientMutex);
        if (isConnected == false)
        {
          APRSConnect();
        }
        else
        {
          if (millis() > deviceInfoTickInterval)
          {
            sendDeviceInfo();
            sendDeviceTelemetry();
            deviceInfoTickInterval = millis() + 600000; // every 10 min
          }
          xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
          bool hasData = aprsClient.available();
          String line;
          if (hasData)
            line = aprsClient.readStringUntil('\n'); // อ่านค่าที่ Server ตอบหลับมาทีละบรรทัด
          xSemaphoreGive(aprsClientMutex);
          if (hasData)
          {
            pingTimeout = millis() + 300000; // Reset ping timout
            projLog(LOGCAT_APRS_INET, "APRS-IS RX: %s", line.c_str());
#ifdef DEBUG_IS
            printTime();
            Serial.print("APRS-IS ");
            Serial.println(line);
#endif
            status.isCount++;
            int start_val = line.indexOf(">", 0); // หาตำแหน่งแรกของ >
            if (start_val > 3)
            {
              // if (config.dispINET == true){
              //     if(!dispBuffer.isFull()) dispBuffer.push(line.c_str());
              // }
#ifdef BOARD_HAS_PSRAM
              // char *raw = (char *)malloc(line.length() + 1);
#else
              char *raw = (char *)malloc(line.length() + 1);
#endif
              String src_call = line.substring(0, start_val);
              String msg_call = "::" + src_call;

              status.allCount++;
              status.rxCount++;
              igateTLM.RX++;

              projLog(LOGCAT_APRS_INET, "INET: %s", line.c_str());
              char raw[500];
              memset(&raw[0], 0, sizeof(raw));
              start_val = line.indexOf(":", 10); // Search of info in ax25
              if (start_val > 5)
              {
                String info = line.substring(start_val + 1);
                // info.toCharArray(&raw[0], info.length(), 0);
                // Clamp to the fixed stack buffer's size, not the
                // APRS-IS-server-supplied line length - an unusually long
                // line would otherwise overflow `raw` (see FORK_NOTES.md's
                // "never trust a packet-derived length as a buffer size").
                size_t infoLen = info.length();
                if (infoLen > sizeof(raw) - 1)
                  infoLen = sizeof(raw) - 1;
                memcpy(raw, info.c_str(), infoLen);

                uint16_t type = pkgType(&raw[0]);
                int start_dstssid = line.indexOf("-", 1); // get SSID -
                if (start_dstssid < 0)
                  start_dstssid = line.indexOf(" ", 1); // get ssid space
                char ssid = 0;
                if (start_dstssid > 0)
                  ssid = line.charAt(start_dstssid + 1);

                if (ssid > 47 && ssid < 58)
                {
                  size_t len = src_call.length();
                  char call[15];
                  memset(call, 0, sizeof(call));
                  if (len > 15)
                    len = 15;
                  memcpy(call, src_call.c_str(), len);
                  call[14] = 0;
                  memset(raw, 0, sizeof(raw));
                  // Clamp before copying, not just terminate after: the old
                  // code only forced raw[499]=0 afterward, which stops a
                  // later strlen() from running off the end but does not
                  // stop the memcpy itself from overflowing `raw` first if
                  // line.length() >= sizeof(raw).
                  size_t lineLen = line.length();
                  if (lineLen > sizeof(raw) - 1)
                    lineLen = sizeof(raw) - 1;
                  memcpy(raw, line.c_str(), lineLen);
                  raw[sizeof(raw) - 1] = 0;
                  int idx = pkgListUpdate(call, raw, type, 1);
                  int cnt = 0;
                  if (idx > -1)
                  {
                    // Put queue affter filter for display popup
                    if (config.rx_display && config.dispINET && (type & config.dispFilter))
                    {
                      cnt = pushTNC2Raw(idx);
                      projLog(LOGCAT_APRS_INET, "INET_putQueueDisp:[pkgList_idx=%d/queue=%d,Type=%d] %s", idx, cnt, type, call);
                    }
                  }

                  // INET2RF affter filter
                  if (config.rf_en && config.inet2rf)
                  {
                    if (type & config.inet2rfFilter)
                    {
                      char strtmp[300];
                      String tnc2Raw = "";
                      if (config.aprs_ssid == 0)
                        sprintf(strtmp, "%s>" APRS_TOCALL, config.aprs_mycall);
                      else
                        sprintf(strtmp, "%s-%d>" APRS_TOCALL, config.aprs_mycall, config.aprs_ssid);
                      tnc2Raw = String(strtmp);
                      tnc2Raw += ",RFONLY"; // fix path to rf only not send loop to inet
                      tnc2Raw += ":}";      // 3rd-party frame
                      tnc2Raw += line;
                      pkgTxPush(tnc2Raw.c_str(), tnc2Raw.length(), 0);
                      char sts[50];
                      sprintf(sts, "--SRC CALL--\n%s\n", src_call.c_str());
                      pushTxDisp(TXCH_3PTY, "TX INET->RF", sts);
                      status.inet2rf++;
                      igateTLM.INET2RF++;
                      projLog(LOGCAT_APRS_INET, "INET2RF: %s", line.c_str());
                    }
                  }
                }
              }
            }
            // free(raw);
          }
        }
      }
      else if (aprsClient.connected())
      {
        // iGate was turned off while a connection was already open -
        // nothing above closes it (this block simply stops running), so
        // the socket was left dangling: aprsClient.connected() (and the
        // OLED status bar's cloud icon, gui_lcd.cpp's topBar()) kept
        // reporting "connected" indefinitely, until the far end eventually
        // timed it out. Close it explicitly the moment igate_en goes
        // false. Found live 2026-09-29 (user report: cloud icon stayed on
        // after disabling iGate).
        xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
        aprsClient.stop();
        xSemaphoreGive(aprsClientMutex);
        projLog(LOGCAT_APRS_INET, "iGate disabled - closed APRS-IS connection");
      }

      if (millis() > pingTimeout)
      {
        pingTimeout = millis() + 300000;
        projLog(LOGCAT_SYSTEM, "Ping GW to %s", WiFi.gatewayIP().toString().c_str());
        if (ping_start(WiFi.gatewayIP(), 3, 0, 0, 5) == true)
        {
          projLog(LOGCAT_SYSTEM, "GW Success!!");
        }
        else
        {
          projLog(LOGCAT_SYSTEM, "GW Fail!");
          WiFi.disconnect();
          WiFi.reconnect();
          wifiTTL = 0;
        }
        if (config.vpn)
        {
          IPAddress vpnIP;
          vpnIP.fromString(String(config.wg_gw_address));
          projLog(LOGCAT_SYSTEM, "Ping VPN to %s", vpnIP.toString().c_str());
          if (ping_start(vpnIP, 2, 0, 0, 10) == true)
          {
            projLog(LOGCAT_SYSTEM, "VPN Ping Success!!");
          }
          else
          {
            projLog(LOGCAT_SYSTEM, "VPN Ping Fail!");
            wireguard_remove();
            delay(3000);
            wireguard_setup();
          }
        }
      }
    }
    else if (config.wifi_mode & WIFI_AP_FIX)
    { // WiFi connected
      serviceHandle();
    }
  } // for loop
}