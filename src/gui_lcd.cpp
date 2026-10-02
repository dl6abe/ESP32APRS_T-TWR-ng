/*
 Name:		ESP32APRS T-TWR Plus
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/

#include "gui_lcd.h"
#include "esp_adc_cal.h"
#include "AFSK.h"
#include "qrcode.h"
#include "wifi_config.h"
#include "webservice.h"

#include <HTTPClient.h>
#include <ESP32httpUpdate.h>
#include "AiEsp32RotaryEncoder.h"
#include "sa868.h"

#define SerialLOG Serial

unsigned long dimTimeout = 0;
unsigned long currentTime;
unsigned long loopTime;
int encoder0Pos = 0;
int encoder0PosPrev = 0;
unsigned char encoder_A = 0;
unsigned char encoder_A_prev = 0;
char curTab = 0;
// Was local to mainDisp() - promoted to globals so menu-detail screens in
// other files (gui_menu_*.cpp) can reset them via guiIdleTimedOut() below,
// the same way mainDisp()'s own manual "back" path already does.
uint8_t menuSel = 0;
char curTabOld = 0;
int posNow = 0;
int timeHalfSec = 0;
int line = 16;

uint8_t gps_mode = 0;

cppQueue queTxDisp(sizeof(txDisp), 10, IMPLEMENTATION); // Instantiate queue

void pushTxDisp(uint8_t ch, const char *name, char *info)
{
    if(!config.tx_display) return;

    txDisp pkg;

    pkg.tx_ch = ch;
    strcpy(pkg.name, name);
    strcpy(pkg.info, info);
    queTxDisp.push(&pkg); // ใส่แพ็จเก็จจาก TNC ลงคิวบัพเฟอร์
}

// Shared idle-timeout check for every menu-detail screen (gui_menu_*.cpp)
// and the settings-menu list itself, below - one place implementing "go
// back to the home screen after 60s of inactivity" instead of many
// hand-rolled copies. See Gitea issue #27.
bool guiIdleTimedOut(unsigned long since)
{
    if (millis() - since < 60000UL)
        return false;
    conStat = CON_NORMAL;
    menuSel = 0;
    curTabOld = curTab + 1; // force the Dashboard tab to redraw
    return true;
}

#define ROTARY_ENCODER_A_PIN 47
#define ROTARY_ENCODER_B_PIN 46
#define ROTARY_ENCODER_BUTTON_PIN 21

#define ROTARY_ENCODER_STEPS 4
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, -1, ROTARY_ENCODER_STEPS);

void IRAM_ATTR readEncoderISR()
{
    rotaryEncoder.readEncoder_ISR();
}

const char *str_status[] = {
    "IDLE_STATUS",
    "NO_SSID_AVAIL",
    "SCAN_COMPLETED",
    "CONNECTED",
    "CONNECT_FAILED",
    "CONNECTION_LOST",
    "DISCONNECTED"};


// Menu variables
MenuSystem ms(my_renderer);

Menu mnuAbout("ABOUT");
MenuItem mnuAbout_mi2("WiFi Status", &on_wifistatus_selected);
MenuItem mnuAbout_mi3("Information", &on_information_selected);
// MenuItem mnuAbout_mi4("Dash Board", &on_dashboard_selected);

Menu mnuConfig("Save/Load");
MenuItem mnuConfig_mi1("Save Config", &on_save_selected);
MenuItem mnuConfig_mi2("Load Config", &on_load_selected);
MenuItem mnuConfig_mi3("Factory Reset", &on_factory_selected);
MenuItem mnuConfig_mi4("REBOOT", &on_reboot_selected);

Menu mnuIgateFilter("Filter");
MenuItem mnuIgateFilter_mi1("RF2INET Filter", &on_filter_rf2inet_selected);
MenuItem mnuIgateFilter_mi2("INET2RF Filter", &on_filter_inet2rf_selected);

Menu mnuAPRS("APRS");

Menu mnu1("WiFi/BT/RF");
MenuItem mnu1_mi1("WiFi AP", &on_wifi_AP_selected);
MenuItem mnu1_mi2("WiFi Station", &on_wifi_Client_selected);
MenuItem mnu1_mi3("Bluetooth", &on_bluetooth_selected);
MenuItem mnu1_mi4("RF Module", &on_rfconfig_selected);

Menu mnu2("IGATE MODE");
MenuItem mnu2_mi1("APRS-IS", &on_aprsserver_selected);
MenuItem mnu2_mi2("Position", &on_igate_position_selected);
MenuItem mnu2_mi3("Function", &on_igate_function_selected);
MenuItem mnu2_mi4("Beacon", &on_igate_beacon_selected);
// MenuItem mnu2_mi5("Filter", &on_filter_digi_selected);

Menu mnu3("TRACKER MODE");
MenuItem mnu3_mi1("Position", &on_tracker_position_selected);
MenuItem mnu3_mi2("Function", &on_tracker_function_selected);
MenuItem mnu3_mi3("Option", &on_tracker_option_selected);
MenuItem mnu3_mi4("SmartBeacon", &on_smartbeacon_selected);

Menu mnu4("DIGI MODE");
MenuItem mnu4_mi1("Position", &on_digi_position_selected);
MenuItem mnu4_mi2("Function", &on_digi_function_selected);
MenuItem mnu4_mi3("Option", &on_digi_option_selected);
MenuItem mnu4_mi4("Filter", &on_filter_digi_selected);

Menu mnu5("SYSTEM");
// MenuItem mnu5_mi1("Save/Load", NULL);
MenuItem mnu5_mi2("OLED Setting", &on_display_selected);
MenuItem mnu5_mi3("Display Filter", &on_filter_display_selected);


void iconMenuShow(int tab)
{
    if (tab < 1)
        tab = 1;
    if (tab > MAX_MENU)
        tab = MAX_MENU;
    int tabArr = tab - 1;
    // uint8_twifi = 0, i;
    int x;
    String str;
    display.fillRect(0, 16, 128, 48, BLACK);
    display.drawLine(0, 17, 127, 17, 1);

    if (tabArr < 4)
        display.fillRoundRect((tabArr) * 32, 22, 32, 32, 7, 1);
    else
        display.fillRoundRect(96, 22, 32, 32, 7, 1);

    display.fillRect(tabArr * (128 / MAX_MENU), 18, 128 / MAX_MENU, 2, 1);

    uint16_t c = 0;
    for (int i = 0; i < 4; i++)
    {
        if (tabArr < 4)
        {
            if (i == tabArr)
                c = 0;
            else
                c = 1;
            display.drawBitmap(2 + (32 * i), 24, menuList[i].icon, 28, 28, c);
        }
        else
        {
            int idx = i + (tabArr - 3);
            if ((idx + 1) > MAX_MENU)
                break;
            if (idx == tabArr)
                c = 0;
            else
                c = 1;
            display.drawBitmap(2 + (32 * i), 24, menuList[idx].icon, 28, 28, c);
        }
    }

    str = String(menuList[tabArr].name);
    x = str.length() * 6;
    display.setCursor((126 - x) / 2, 57);
    display.print(str);
    display.display();
}

QRCode qrcode;

void drawQrCode(const char *qrStr, const char *lines)
{
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, qrStr);

    // QR Code Starting Point
    int offset_x = 67;
    int offset_y = 3;

    display.clearDisplay();

    for (int y = 0; y < qrcode.size; y++)
    {
        for (int x = 0; x < qrcode.size; x++)
        {
            int newX = offset_x + (x * 2);
            int newY = offset_y + (y * 2);

            if (qrcode_getModule(&qrcode, x, y))
            {
                display.fillRect(newX, newY, 2, 2, 0);
            }
            else
            {
                display.fillRect(newX, newY, 2, 2, 1);
            }
        }
    }
    display.drawRect(64, 0, 64, 64, 1);

    display.setFont(&Picopixel);
    display.setTextColor(1, 0);
    display.setCursor(0, 5);
    display.print(lines);
    display.display();
    display.setFont();
}

// const char *MESSAGE_CONFIGURE_WIFI = {"SCAN QR CONNECT\nTO WiFi AP"};
bool qrcodeActive = false;
bool qrcodeSelect = 0;
void qrcodeDisp()
{
    char msg[100];
    // Create the QR code
    if (qrcodeActive == true)
        return;
    qrcodeActive = true;
    char link[128]; // must fit "WIFI:S:<32-byte ssid>;T:WPA;P:<63-byte pass>;;" (up to 112 bytes) without truncating the QR-connect string
    if (qrcodeSelect == 0)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            String ip = WiFi.localIP().toString();
            char wifiName[20];
            WiFi.SSID().toCharArray(wifiName, 20, 0);
            wifiName[19] = 0;
            snprintf(msg, sizeof(msg), "SCAN QR TO OPEN\nWEB BROWSER\nCONFIGURATION\n\nWiFI STA SSID:\n%s", wifiName);
            sprintf(link, "http://%s", ip.c_str());
            drawQrCode(link, msg);
        }
        else
        {
            if (config.wifi_mode & WIFI_AP_FIX)
            { // WiFi AP active
                String ip = WiFi.softAPIP().toString();
                char wifiName[20];
                WiFi.softAPSSID().toCharArray(wifiName, 20, 0);
                wifiName[19] = 0;
                snprintf(msg, sizeof(msg), "SCAN QR TO OPEN\nWEB BROWSER\nCONFIGURATION\n\nWiFI AP SSID:\n%s", wifiName);
                sprintf(link, "http://%s", ip.c_str());
                drawQrCode(link, msg);
            }
            else
            {
                display.clearDisplay();
                display.setTextWrap(1);
                display.setCursor(0, 30);
                display.printf("WiFi STA disconnect\nand\nWiFi AP not enable");
                display.display();
            }
        }
    }
    else
    {
        // QR Connect to wifi AP
        char wifiName[20];
        WiFi.softAPSSID().toCharArray(wifiName, 20, 0);
        wifiName[19] = 0;
        snprintf(msg, sizeof(msg), "SCAN QR CONNECT\nTO WiFi AP\n\nWiFI AP SSID:\n%s", wifiName);
        snprintf(link, sizeof(link), "WIFI:S:%s;T:WPA;P:%s;;", config.wifi_ap_ssid, config.wifi_ap_pass);
        drawQrCode(link, msg);
    }
}

void statisticsDisp()
{

    // uint8_twifi = 0, i;
    int x;
    String str;
    display.fillRect(0, 16, 128, 10, WHITE);
    display.drawLine(0, 16, 0, 63, WHITE);
    display.drawLine(127, 16, 127, 63, WHITE);
    display.drawLine(0, 63, 127, 63, WHITE);
    display.fillRect(1, 25, 126, 38, BLACK);
    display.setTextColor(BLACK);
    display.setCursor(30, 17);
    display.print("STATISTICS");
    // display.setCursor(108, 17);
    // display.print("1/5");
    display.setTextColor(WHITE);

    display.setCursor(3, 26);
    display.print("ALL TX/RX");
    str = String(status.txCount, DEC) + "/" + String(status.rxCount, DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 26);
    display.print(str);

    display.setCursor(3, 35);
    display.print("2RF/2INET");
    str = String(status.inet2rf, DEC) + "/" + String(status.rf2inet, DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 35);
    display.print(str);

    display.setCursor(3, 44);
    display.print("RPT DIGI");
    str = String(status.digiCount, DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 44);
    display.print(str);

    display.setCursor(3, 53);
    display.print("DROP/ERR");
    str = String(status.dropCount, DEC) + "/" + String(status.errorCount, DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 53);
    display.print(str);

    display.display();
}

void pkgLastDisp()
{

    uint8_t k = 0;
    int i;
    int y;
    String str;
    // String times;
    // pkgListType *ptr[100];

    display.fillRect(0, 16, 128, 10, WHITE);
    display.drawLine(0, 16, 0, 63, WHITE);
    display.drawLine(127, 16, 127, 63, WHITE);
    display.drawLine(0, 63, 127, 63, WHITE);
    display.fillRect(1, 25, 126, 38, BLACK);
    display.setTextColor(BLACK);
    display.setCursor(27, 17);
    display.print("LAST STATIONS");
    // display.setCursor(108, 17);
    // display.print("2/5");
    display.setTextColor(WHITE);

    sort(pkgList, PKGLISTSIZE);
    k = 0;
    for (i = 0; i < PKGLISTSIZE; i++)
    {
        pkgListType pkg = getPkgList(i);
        if (pkg.time > 0)
        {
            y = 26 + (k * 9);
            // display.drawBitmap(3, y, &SYMBOL[0][0], 11, 6, WHITE);
            display.fillRoundRect(2, y, 7, 8, 2, WHITE);
            display.setCursor(3, y);
            pkg.calsign[10] = 0;
            display.setTextColor(BLACK);
            switch (pkg.type)
            {
            case PKG_OBJECT:
                display.print("O");
                break;
            case PKG_ITEM:
                display.print("I");
                break;
            case PKG_MESSAGE:
                display.print("M");
                break;
            case PKG_WX:
                display.print("W");
                break;
            case PKG_TELEMETRY:
                display.print("T");
                break;
            case PKG_QUERY:
                display.print("Q");
                break;
            case PKG_STATUS:
                display.print("S");
                break;
            default:
                display.print("*");
                break;
            }
            display.setTextColor(WHITE);
            display.setCursor(10, y);
            display.print(pkg.calsign);
            display.setCursor(126 - 48, y);
            struct tm tmstruct;
            localtime_r(&pkg.time, &tmstruct);
            display.printf("%02d:%02d:%02d", tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
            // display.printf("%02d:%02d:%02d", hour(pkg.time), minute(pkg.time), second(pkg.time));
            k++;
            if (k >= 4)
                break;
        }
    }
    display.display();
}

void pkgCountDisp()
{

    // uint8_twifi = 0, k = 0, l;
    uint k = 0;
    int i;
    // char list[4];
    int x, y;
    String str;

    display.fillRect(0, 16, 128, 10, WHITE);
    display.drawLine(0, 16, 0, 63, WHITE);
    display.drawLine(127, 16, 127, 63, WHITE);
    display.drawLine(0, 63, 127, 63, WHITE);
    display.fillRect(1, 25, 126, 38, BLACK);
    display.setTextColor(BLACK);
    display.setCursor(30, 17);
    display.print("TOP PACKAGE");
    // display.setCursor(108, 17);
    // display.print("3/5");
    display.setTextColor(WHITE);

    sortPkgDesc(pkgList, PKGLISTSIZE);
    k = 0;
    for (i = 0; i < PKGLISTSIZE; i++)
    {
        pkgListType pkg = getPkgList(i);
        if (pkg.time > 0)
        {
            y = 26 + (k * 9);
            // display.drawBitmapV(2, y-1, &SYMBOL[pkgList[i].symbol][0], 11, 8, WHITE);
            pkg.calsign[10] = 0;
            display.fillRoundRect(2, y, 7, 8, 2, WHITE);
            display.setCursor(3, y);
            pkg.calsign[10] = 0;
            display.setTextColor(BLACK);
            switch (pkg.type)
            {
            case PKG_OBJECT:
                display.print("O");
                break;
            case PKG_ITEM:
                display.print("I");
                break;
            case PKG_MESSAGE:
                display.print("M");
                break;
            case PKG_WX:
                display.print("W");
                break;
            case PKG_TELEMETRY:
                display.print("T");
                break;
            case PKG_QUERY:
                display.print("Q");
                break;
            case PKG_STATUS:
                display.print("S");
                break;
            default:
                display.print("*");
                break;
            }
            display.setTextColor(WHITE);
            display.setCursor(10, y);
            display.print(pkg.calsign);
            str = String(pkg.pkg, DEC);
            x = str.length() * 6;
            display.setCursor(126 - x, y);
            display.print(str);
            k++;
            if (k >= 4)
                break;
        }
    }
    display.display();
}

void systemDisp()
{
    int x;
    String str;
    time_t upTime = now() - systemUptime; // - startTime;

    display.fillRect(0, 16, 128, 10, WHITE);
    display.drawLine(0, 16, 0, 63, WHITE);
    display.drawLine(127, 16, 127, 63, WHITE);
    display.drawLine(0, 63, 127, 63, WHITE);
    display.fillRect(1, 25, 126, 38, BLACK);
    display.setTextColor(BLACK);
    display.setCursor(30, 17);
    display.print("SYSTEM INFO");
    display.setTextColor(WHITE);

    display.setCursor(3, 26);
    display.print("UpTIME:");
    // systemUptime stays 0 until the first successful NTP/GPS time sync
    // (main.cpp) - showing a duration computed from "now() - 0" before that
    // gives a decades-long reading instead of a real duration (issue #50).
    if (systemUptime == 0)
        str = "N/A";
    else
        str = String(day(upTime) - 1, DEC) + "D " + String(hour(upTime), DEC) + ":" + String(minute(upTime), DEC) + ":" + String(second(upTime), DEC);
    x = str.length() * 6;
    display.setCursor(126 - x, 26);
    display.print(str);

    display.setCursor(3, 35);
    display.print("RAM:");
    str = String((float)ESP.getFreeHeap() / 1000, 1) + " KByte";
    x = str.length() * 6;
    display.setCursor(126 - x, 35);
    display.print(str);

    display.setCursor(3, 44);
    display.print("PSRAM:");
    str = String((float)ESP.getFreePsram() / 1000, 1) + " KByte";
    x = str.length() * 6;
    display.setCursor(126 - x, 44);
    display.print(str);

    display.setCursor(3, 53);
    display.print("VBAT:");
    float vbat = (float)PMU.getBattVoltage() / 1000;
    int battPercent = PMU.getBatteryPercent(); // -1 if no battery is connected (e.g. running on USB/external power only)
    if (battPercent >= 0)
        str = String(battPercent) + "% " + String(vbat, 2) + "V";
    else
        str = String(vbat, 2) + " V.";
    x = str.length() * 6;
    display.setCursor(126 - x, 53);
    display.print(str);

    display.display();
}

void gpsDisp()
{
    int x;
    String str;

    if (gps_mode == 0)
    {
        display.fillRect(0, 16, 128, 10, WHITE);
        display.drawLine(0, 16, 0, 63, WHITE);
        display.drawLine(127, 16, 127, 63, WHITE);
        display.drawLine(0, 63, 127, 63, WHITE);
        display.fillRect(1, 25, 126, 38, BLACK);
        display.setTextColor(BLACK);
        display.setCursor(35, 17);
        display.print("GPS INFO");
        // display.setCursor(108, 17);
        // display.print("5/5");
        display.setTextColor(WHITE);

        display.setCursor(3, 26);
        display.print("LAT:");
        str = String(gps.location.lat(), 5);
        x = str.length() * 6;
        display.setCursor(80 - x, 26);
        display.print(str);

        display.setCursor(3, 35);
        display.print("LON:");
        str = String(gps.location.lng(), 5);
        x = str.length() * 6;
        display.setCursor(80 - x, 35);
        display.print(str);

        display.drawYBitmap(90, 26, &Icon_TableB[50][0], 16, 16, WHITE);
        display.setCursor(110, 32);
        display.print(gps.satellites.value());

        display.setCursor(3, 44);
        display.print("SPD:");
        str = String(gps.speed.kmph(), 1) + "kph";
        /*x = str.length() * 6;
        display.setCursor(62 - x, 44);*/
        display.print(str);

        display.setCursor(80, 44);
        display.print("ALT:");
        str = String(gps.altitude.meters(), 0) + "m";
        x = str.length() * 6;
        display.setCursor(126 - x, 44);
        display.print(str);

        display.setCursor(3, 54);
        // display.print("TIME:");
        str = String(gps.date.day(), DEC) + "/" + String(gps.date.month(), DEC) + "/" + String(gps.date.year(), DEC);
        display.setCursor(3, 53);
        display.print(str);
        str = String(gps.time.hour(), DEC) + ":" + String(gps.time.minute(), DEC) + ":" + String(gps.time.second(), DEC) + "z";
        x = str.length() * 6;
        display.setCursor(126 - x, 53);
        display.print(str);
    }
    else
    {
        // display.clearDisplay();
        display.fillRect(0, 0, 128, 64, BLACK);
        display.drawYBitmap(90, 0, &Icon_TableB[50][0], 16, 16, WHITE);
        display.setCursor(107, 7);
        display.setTextSize(1);
        display.setFont(&FreeSansBold9pt7b);
        display.print(gps.satellites.value());

        struct tm tmstruct;
        char strTime[10];
        tmstruct.tm_year = 0;
        getLocalTime(&tmstruct, 100);
        sprintf(strTime, "%02d:%02d:%02d", tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
        display.setCursor(0, 14);
        display.print(strTime);

        if (config.dim == 2)
        { // Auto dim timeout
            if (millis() > (dimTimeout + 60000))
            {
                display.dim(true);
            }
            else
            {
                display.dim(false);
            }
        }
        else if (config.dim == 3)
        { // Dim for time
            if (tmstruct.tm_hour > 5 && tmstruct.tm_hour < 19)
            {
                display.dim(false);
            }
            else
            {
                display.dim(true);
            }
        }

        display.setFont(&FreeSerifItalic9pt7b);
        display.setCursor(80, 28);
        display.printf("km/h");

        display.setFont(&Seven_Segment24pt7b);
        display.setCursor(70, 63);
        display.print(SB_SPEED, DEC);

        compass_label(25, 42, 19, 0.0F, WHITE);
        compass_arrow(25, 42, 16, SB_HEADING, WHITE);
        display.drawLine(0, 16, 60, 16, WHITE);
        display.drawLine(60, 16, 70, 29, WHITE);
        display.drawLine(50, 16, 60, 29, WHITE);
        display.drawLine(60, 29, 127, 29, WHITE);
        display.setFont();
    }
    display.display();
}

void msgBox(String msg)
{
    int x = msg.length() * 6;
    int x1 = 64 - (x / 2);

    display.fillRect(x1 - 7, 26, x + 14, 28, BLACK);
    display.drawRect(x1 - 5, 28, x + 10, 24, WHITE);
    display.drawLine(x1 - 3, 53, x1 + (x + 10) - 4, 53, WHITE);
    display.drawLine(x1 + (x + 10) - 4, 30, x1 + (x + 10) - 4, 53, WHITE);
    display.setCursor(x1, 37);
    display.print(msg);
    display.display();
}

uint32_t readADC_Cal(int ADC_Raw)
{
    esp_adc_cal_characteristics_t adc_chars;

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    return (esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}

void topBar(int ws)
{
    // int ang = analogRead(39);
    //  float vbat;
    uint8_t vbatScal = 0;
    int wifiSignal = ws;
    uint8_t wifi = 0, i;
    int x, y;
    if (!(config.wifi_mode & WIFI_STA_FIX))
        wifiSignal = -30;
    // display.setTextColor(WHITE);
    display.fillRect(0, 0, 128, 16, BLACK);
    display.drawRoundRect(40, 0, 60, 16, 3, 1);
    // Draw Attena Signal
    display.drawTriangle(0, 0, 6, 0, 3, 3, WHITE);
    display.drawLine(3, 0, 3, 7, WHITE);
    x = 5;
    y = 3;
    int wifiLevel = (wifiSignal + 100) / 10;
    if (wifiLevel > 5)
        wifiLevel = 5;
    if (wifiLevel < 0)
        wifiLevel = 0;
    wifi = (uint8_t)wifiLevel;
    for (i = 0; i < wifi; i++)
    {
        display.drawLine(x, 7 - y, x, 7, WHITE);
        x += 2;
        y++;
    }

    display.setCursor(0, 8);
    if (config.wifi_mode & WIFI_STA_FIX)
    {
        display.print(wifiSignal);
        display.print("dBm");
    }
    else
    {
        display.print("DIS");
    }

    vbat = (float)PMU.getBattVoltage() / 1000;
    battPercent = PMU.getBatteryPercent(); // global, shared with handle_dashboard() (webservice.cpp) - -1 if no battery is connected
    powerCharging = PMU.isCharging();
    powerVbusIn = PMU.isVbusIn();

    x = 109;
    display.drawLine(0 + x, 1, 2 + x, 1, WHITE);
    display.drawLine(0 + x, 6, 2 + x, 6, WHITE);
    display.drawLine(0 + x, 2, 0 + x, 5, WHITE);
    display.drawLine(2 + x, 0, 18 + x, 0, WHITE);
    display.drawLine(2 + x, 7, 18 + x, 7, WHITE);
    display.drawLine(18 + x, 1, 18 + x, 6, WHITE);
    if (battPercent >= 0)
        vbatScal = battPercent / 20; // 0-100% -> 0-5 bars, from the PMU's own fuel-gauge reading
    else if (vbat < 3.3)
        vbatScal = 0;
    else
        vbatScal = (uint8_t)ceil((vbat - 3.3) * 6); // fallback: coarse linear estimate from voltage alone
    if (vbatScal > 5)
        vbatScal = 5;
    x = 16 + 109;
    for (i = 0; i < vbatScal; i++)
    {
        display.drawLine(x, 2, x, 5, WHITE);
        x--;
        display.drawLine(x, 2, x, 5, WHITE);
        x -= 2;
    }
    display.setCursor(104, 8);
    display.print(vbat, 1);
    display.print("V");
    // Wifi Status
    // display.setCursor(15,0);
    // display.print("WiFi");
    if (config.wifi_mode & WIFI_STA_FIX)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            display.fillRect(15, 0, 24, 8, BLACK);
        }
        else
        {
            display.setCursor(15, 0);
            display.print("WiFi");
        }
    }
    else if (config.wifi_mode & WIFI_AP_FIX)
    {
        display.setCursor(15, 0);
        display.print(" AP");
    }

    if (config.bt_master)
    {
        display.drawBitmap(42, 2, iconBluetooth, 11, 11, 1);
    }
    // DCS Status
    // display.setCursor(50,0);
    //  display.println("DCS");

    xSemaphoreTake(aprsClientMutex, portMAX_DELAY);
    bool aprsIsConnected = aprsClient.connected();
    xSemaphoreGive(aprsClientMutex);
    if (aprsIsConnected)
    {
        display.drawBitmap(54, 2, iconClound, 12, 12, 1);
    }

    // GPS icon: off with no fix at all, blinking with a fix that isn't
    // "usable" yet (the same hdop<10 && satellites>3 threshold beacon
    // building/SmartBeacon already gate on - see beacon_builder.cpp/main.cpp),
    // steady on once the fix is usable.
    if (gps.location.isValid())
    {
        bool usableFix = (gps.hdop.hdop() < 10) && (gps.satellites.value() > 3);
        if (usableFix || (millis() / 500) % 2 == 0)
        {
            display.drawBitmap(70, 2, iconLocation, 12, 12, 1);
        }
    }

    if (wireguard_active())
    {
        display.drawBitmap(85, 2, iconLink, 12, 12, 1);
    }

    // TEMPORARY visual test for the SmartTracker icon (Gitea issue #31/#32)
    // - unconditional, always-blinking, so it can be checked on real
    // hardware before the actual WiFi-loss detection logic exists. Not the
    // real feature yet: remove this block (or replace with the real
    // condition + rotating-slot logic) once #31/#32 are implemented.
    // Disabled for now on request - re-enable by flipping this to #if 1.
#if 0
    if ((millis() / 500) % 2 == 0)
    {
        display.drawBitmap(85, 2, iconSmartTracker, 12, 12, 1);
    }
#endif

    display.setCursor(110, 0);

    if (config.dim == 2)
    { // Auto dim timeout
        if (millis() > (dimTimeout + 60000))
        {
            display.dim(true);
        }
        else
        {
            display.dim(false);
        }
    }
    else if (config.dim == 3)
    { // Dim for time
        if (hour() > 5 && hour() < 17)
        {
            display.dim(false);
        }
        else
        {
            display.dim(true);
        }
    }

    // OLED sleep (config.oled_timeout, seconds; 0 = never sleep). Separate
    // from the dim modes above - this is a real SSD1306 power-off, not a
    // contrast reduction, so it stacks with whichever dim mode is active.
    // oledWake() (called on encoder/button activity and on every popup,
    // see dispWindow()) is what resets oledSleepTimeout going forward.
    if (config.oled_timeout > 0 && !oledSleeping && millis() > oledSleepTimeout)
    {
        display.ssd1306_command(SSD1306_DISPLAYOFF);
        oledSleeping = true;
    }

    // Only this task touches display.* - see the comment on
    // applyPendingDisplaySettings() in gui_lcd.h for why.
    applyPendingDisplaySettings();

    display.display();
}

uint8_t KeyDelay(uint8_t pin)
{
    int8_t Key = 0;
    int i = 0;
    uint8_t ret = 0;
    do
    {
        delay(1);
        if (digitalRead(pin))
            Key++;
        else
            Key--;
    } while (++i < 10);
    if (Key > 0)
        ret = 1;
    else
        ret = 0;
    return ret;
}

const int8_t KNOBDIR[] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};

volatile int8_t _oldState;

volatile long _position;        // Internal position (4 times _positionExt)
volatile long _positionExt;     // External position
volatile long _positionExtPrev; // External position (used only for direction checking)

portMUX_TYPE muxKey = portMUX_INITIALIZER_UNLOCKED;
unsigned long keyPeriadTime;
void IRAM_ATTR doEncoder()
{
    portENTER_CRITICAL_ISR(&muxKey);
    int sig1 = digitalRead(keyA);
    int sig2 = digitalRead(keyB);
    int8_t thisState = sig1 | (sig2 << 1);

    if (_oldState != thisState)
    {
        _position += KNOBDIR[thisState | (_oldState << 2)];
        _oldState = thisState;

        // Only commit a step once the knob is back at its mechanical rest
        // position (both pins HIGH - INPUT_PULLUP wiring, pinMode() above),
        // not on every intermediate quadrature state along the way. Without
        // this, a small wobble near a _position/4 boundary (e.g. contact
        // bounce, or just touching the knob without turning it to the next
        // detent) could already cross the boundary and register a full menu
        // step, even though the knob never reached the next click. Found
        // live 2026-09-29 (user report: slight movement without a click
        // already changes the selected menu element).
        if (thisState == 3)
        {
            // Serial.printf("Key:%d\n", _position >> 2);
            _positionExt = _position >> 2;
            if (_positionExtPrev != _positionExt)
            {
                if (_positionExtPrev > _positionExt)
                {
                    encoder0Pos--;
                }
                else if (_positionExtPrev < _positionExt)
                {
                    encoder0Pos++;
                }
                _positionExtPrev = _positionExt;
                // Serial.printf("Key:%d\n", _positionExt);
            }
        }
    }
    portEXIT_CRITICAL_ISR(&muxKey);
}

void displayInfo()
{
    SerialLOG.print(F("Location: "));
    if (gps.location.isValid())
    {
        SerialLOG.print(gps.location.lat(), 6);
        SerialLOG.print(F(","));
        SerialLOG.print(gps.location.lng(), 6);
    }
    else
    {
        SerialLOG.print(F("INVALID"));
    }

    SerialLOG.print(F("  Date/Time: "));
    if (gps.date.isValid())
    {
        SerialLOG.print(gps.date.month());
        SerialLOG.print(F("/"));
        SerialLOG.print(gps.date.day());
        SerialLOG.print(F("/"));
        SerialLOG.print(gps.date.year());
    }
    else
    {
        SerialLOG.print(F("INVALID"));
    }

    SerialLOG.print(F(" "));
    if (gps.time.isValid())
    {
        if (gps.time.hour() < 10)
            SerialLOG.print(F("0"));
        SerialLOG.print(gps.time.hour());
        SerialLOG.print(F(":"));
        if (gps.time.minute() < 10)
            SerialLOG.print(F("0"));
        SerialLOG.print(gps.time.minute());
        SerialLOG.print(F(":"));
        if (gps.time.second() < 10)
            SerialLOG.print(F("0"));
        SerialLOG.print(gps.time.second());
        SerialLOG.print(F("."));
        if (gps.time.centisecond() < 10)
            SerialLOG.print(F("0"));
        SerialLOG.print(gps.time.centisecond());
    }
    else
    {
        SerialLOG.print(F("INVALID"));
    }

    SerialLOG.println();
}

// long lastGPS = 0;

unsigned long saveTimeout = 0;
unsigned long menuTimeout = 0;
unsigned long disp_delay = 0;
uint8_t dispMode = 0;
String rawDisp;
int selTab = 0;
bool dispPush = 0;

bool oledLock = 0;

void setOLEDLock(bool lck)
{
    oledLock = lck;
}

volatile bool displayApplyPending = false;
static volatile bool oledWakeRequested = false;

// See the comment on oledSleepTimeout/oledSleeping in gui_lcd.h. Resets the
// sleep countdown and, if the panel is currently off, requests a wake -
// call this from anything that counts as "activity". Safe to call from any
// task: it only touches plain variables, never display.* directly (mainDisp
// is the only task allowed to touch the hardware - see gui_lcd.h).
void oledWake()
{
    oledSleepTimeout = millis() + ((long)config.oled_timeout * 1000);
    if (oledSleeping)
        oledWakeRequested = true;
}

// mainDisp task only - applies any pending display.dim()/contrast/wake
// change requested (from any task) via displayApplyPending/oledWakeRequested.
// Call once per frame, before display.display().
void applyPendingDisplaySettings()
{
    if (oledWakeRequested)
    {
        display.ssd1306_command(SSD1306_DISPLAYON);
        oledSleeping = false;
        oledWakeRequested = false;
    }
    if (displayApplyPending)
    {
        if (config.dim == 1)
            display.dim(true);
        else if (config.dim == 4)
        {
            display.ssd1306_command(SSD1306_SETCONTRAST);
            display.ssd1306_command(config.contrast);
        }
        else
            display.dim(false);
        displayApplyPending = false;
    }
}

extern unsigned long timeGui;
uint16_t pttStat = 0;
RTC_DATA_ATTR uint16_t lastStatDisp = 0;

void mainDisp(void *pvParameters)
{
    pinMode(keyA, INPUT_PULLUP);
    pinMode(keyB, INPUT_PULLUP);
    pinMode(keyPush, INPUT_PULLUP);

    conStatNetwork = CON_WIFI;
    conStat = CON_NORMAL;

    // showDisp=false;
    curTab = 3;
    // oledSleepTimeout = millis() + (config.oled_timeout * 1000);

    mnuAbout.add_item(&mnuAbout_mi2);
    mnuAbout.add_item(&mnuAbout_mi3);
    // mnuAbout.add_item(&mnuAbout_mi4);
    mnuConfig.add_item(&mnuConfig_mi1);
    mnuConfig.add_item(&mnuConfig_mi2);
    mnuConfig.add_item(&mnuConfig_mi3);
    mnuConfig.add_item(&mnuConfig_mi4);
    mnuIgateFilter.add_item(&mnuIgateFilter_mi1);
    mnuIgateFilter.add_item(&mnuIgateFilter_mi2);

    ms.get_root_menu().add_menu(&mnu1); // Wiress
    mnu1.add_item(&mnu1_mi1);
    mnu1.add_item(&mnu1_mi2);
    mnu1.add_item(&mnu1_mi3);
    mnu1.add_item(&mnu1_mi4);

    ms.get_root_menu().add_menu(&mnuAPRS); // APRS

    mnuAPRS.add_menu(&mnu2); // IGATE
    mnu2.add_item(&mnu2_mi1);
    mnu2.add_item(&mnu2_mi2);
    mnu2.add_item(&mnu2_mi3);
    mnu2.add_item(&mnu2_mi4);
    mnu2.add_menu(&mnuIgateFilter);

    mnuAPRS.add_menu(&mnu3); // TRACKET
    mnu3.add_item(&mnu3_mi1);
    mnu3.add_item(&mnu3_mi2);
    mnu3.add_item(&mnu3_mi3);
    mnu3.add_item(&mnu3_mi4);

    mnuAPRS.add_menu(&mnu4); // DIGI
    mnu4.add_item(&mnu4_mi1);
    mnu4.add_item(&mnu4_mi2);
    mnu4.add_item(&mnu4_mi3);
    mnu4.add_item(&mnu4_mi4);

    ms.get_root_menu().add_menu(&mnu5); // SYSTEM
    mnu5.add_menu(&mnuConfig);
    mnu5.add_item(&mnu5_mi2);
    mnu5.add_item(&mnu5_mi3);
    mnu5.add_menu(&mnuAbout);

    attachInterrupt(keyA, doEncoder, CHANGE);
    attachInterrupt(keyB, doEncoder, CHANGE);

    if (config.startup > 5)
        config.startup = 0;
    if (config.startup < 5)
    {
        curTab = config.startup + 1;
        gps_mode = 0;
    }
    else
    {
        curTab = 5;
        gps_mode = 1;
    }
    topBar(-1);

    if (config.igate_gps == false)
    {
        tx_counter = 0;
        tx_interval = 10;
    }

    unsigned long timeGuiOld = millis();
    timeGui = 0;
    saveTimeout = millis();
    curTabOld = 0;
    menuSel = 0;

    pttStat = 0;
    for (;;)
    {
        unsigned long now = millis();
        timeGui = now - timeGuiOld;
        timeGuiOld = now;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // Prevent RF interference with OLED
        if (oledLock == true)
            continue;
        if(pttStat>0){
            if(pttStat==1){
                display.clearDisplay();
                display.setTextSize(1);
                display.setFont(&FreeSansBold9pt7b);
                display.setCursor(5,14);
                display.print("FM VOICE");
                display.setFont(NULL);
                display.setCursor(30, 20);
                display.printf("%.4f MHz",config.freq_tx);
                display.setCursor(30, 30);
                display.printf("%.4f MHz",config.freq_rx);
                display.setCursor(30, 40);
                if(config.rf_power)
                    display.printf("PWR: HIGH");
                else
                    display.printf("PWR: LOW");
                display.fillRect(1, 19, 25, 10, 1);
                display.drawRect(0, 18, 128, 32, 1);
                display.setTextColor(BLACK, WHITE);
                display.drawLine(0, 28, 127, 28, 1);
                display.drawLine(0, 38, 127, 38, 1);
                display.fillRect(1, 29, 25, 10, 1);
                display.fillRect(1, 39, 25, 10, 1);
                display.setCursor(8, 20);
                display.print("TX:");
                display.setCursor(8, 30);
                display.print("RX:");
                display.setCursor(2, 40);
                display.print("PWR:");
                display.setTextColor(WHITE);
                display.display();
            }else{
                display.fillRect(100, 0, 28, 16, BLACK);
                display.setCursor(105, 7);
                display.printf("%.1f",(float)(pttStat)/100);
                display.display();
            }
            delay(100);
            continue;
        }        

        if (getTransmit())
        {
            delay(1000);
            continue;
        }

        if (millis() > (saveTimeout + 300000))
        {
            powerSave();
        }

        if (conStat == CON_NORMAL)
        {
            menuTimeout = millis();
            if ((raw_count > 0) && (disp_delay == 0))
            {
                saveTimeout = millis();
                dispPush = false;
                int idx = 0;
                if (popTNC2Raw(idx) > -1)
                {
                    pkgListType pkg = getPkgList(idx);
                    rawDisp = String(pkg.raw);
                    dispWindow(rawDisp, dispMode, true);
                    selTab = idx;
                    if (menuSel == 0)
                        curTabOld = curTab + 1;
                }
                // selTab = 1;
            }

            if (!getTransmit())
            {
                if (queTxDisp.getCount() > 0)
                { // have tx info display
                    txDisp txs;
                    if (queTxDisp.pop(&txs))
                    {
                        dispTxWindow(txs);
                        delay(1000);
                        if (menuSel == 0)
                            curTabOld = curTab + 1;
                    }
                }
            }

            if (millis() > (unsigned long)timeHalfSec)
            {

                timeHalfSec = millis() + 500 + disp_delay;
                // powerWakeup();
                disp_delay = 0;
                dispMode = 0;
                // dispFlagTX=0;
                if (powerStatus() && (raw_count == 0))
                {
                    if (menuSel == 0 || menuSel == 1 || menuSel == 2 || menuSel == 4)
                        topBar(WiFi.RSSI());
                    if (menuSel == 0)
                    {
                        if (curTab != curTabOld)
                        {
                            iconMenuShow(curTab);
                            curTabOld = curTab;
                        }
                    }
                    else if (menuSel == 1)
                    {
                        statisticsDisp();
                    }
                    else if (menuSel == 2)
                    {
                        if (disp_delay <= 0)
                            pkgLastDisp();
                    }
                    else if (menuSel == 3)
                    {
                        if (gps_mode != 1)
                            topBar(WiFi.RSSI());
                        gpsDisp();
                    }
                    else if (menuSel == 4)
                    {
                        systemDisp();
                    }
                    else if (menuSel == 5)
                    {
                        conStat = CON_MENU;
                        ms.reset();
                        ms.display();
                    }
                    else if (menuSel == 6)
                    {
                        qrcodeDisp();
                    }
                    else if (menuSel == 7) // ABout
                    {
                        on_information_selected(NULL);
                    }
                    else
                    {
                        menuSel = 0;
                    }
                }
            }
            else if (disp_delay > 0)
            {
                if (encoder0Pos != posNow)
                {
                    timeHalfSec = millis() + 2000 + disp_delay;
                    saveTimeout = millis();
                    pkgListType pkg = getPkgList(selTab);
                    if (config.dim == 2)
                        dimTimeout = millis();
                    oledWake();
                    if (encoder0Pos > posNow)
                    {
                        selTab++;
                        for (; selTab < PKGLISTSIZE; selTab++)
                        {
                            if (pkg.time > 0)
                                break;
                        }
                        if (selTab >= PKGLISTSIZE)
                            selTab = 0;
                    }
                    else
                    {
                        selTab--;
                        for (; selTab >= 0; selTab--)
                        {
                            if (pkg.time > 0)
                                break;
                        }
                        if (selTab < 0)
                            selTab = PKGLISTSIZE - 1;
                    }
                    posNow = encoder0Pos;

                    if (pkg.time > 0)
                    {
                        rawDisp = String(pkg.raw);
                        dispWindow(rawDisp, dispMode, false);
                    }
                }
            }

            if (raw_count == 0)
            {
                if (encoder0Pos != posNow)
                {
                    timeHalfSec = 0;
                    powerWakeup();
                    saveTimeout = millis();
                    if (config.dim == 2)
                        dimTimeout = millis();
                    oledWake();
                    if (encoder0Pos > posNow)
                    {
                        curTab++;
                        if (curTab > MAX_MENU)
                            curTab = MAX_MENU;
                    }
                    else
                    {
                        curTab--;
                        if (curTab < 1)
                            curTab = 1;
                    }
                    posNow = encoder0Pos;
                }
            }
        }

        if ((digitalRead(keyPush) == LOW) && (conStat != CON_MENU))
        {
            saveTimeout = millis();
            powerWakeup();
            currentTime = millis();
            delay(500);
            // conStat = CON_MENU;
            // TaskGPS.Enable(false);
            if (digitalRead(keyPush) == HIGH)
            { // ONE Click
                if (config.dim == 2)
                    dimTimeout = millis();
                oledWake();
                if (disp_delay > 0)
                { // Select MODE Decode/RAW
                    dispPush = false;
                    disp_delay = config.dispDelay * 1000;
                    timeHalfSec = millis() + disp_delay;
                    if (dispMode == 0)
                        dispMode = 1;
                    else
                        dispMode = 0;
                    dispWindow(rawDisp, dispMode, false);
                }
                else
                {
                    if (menuSel == 0)
                    {
                        menuSel = curTab;
                    }
                    else
                    {
                        menuSel = 0;
                        curTabOld = curTab + 1; // Refresh windows first
                        qrcodeActive = false;
                    }
                }
            }

            while (digitalRead(keyPush) == LOW)
            {
                delay(10);
                if ((millis() - currentTime) > 2000)
                {
                    msgBox("ENTER");
                    while (digitalRead(keyPush) == LOW)
                        ;
                    if (menuSel == 3) // GPS switch info/speed
                    {
                        if (gps_mode == 0)
                            gps_mode = 1;
                        else
                            gps_mode = 0;
                    }
                    else if (menuSel == 6) // qrcode switch web/wifi
                    {
                        if (qrcodeSelect == 0)
                            qrcodeSelect = 1;
                        else
                            qrcodeSelect = 0;
                        qrcodeActive = false;
                    }
                    else if (menuSel == 2)
                    {
                        dispPush = true;
                        disp_delay = 600 * 1000;
                        dispWindow(rawDisp, dispMode, false);
                    }
                    break;
                }
            };
            while (digitalRead(keyPush) == LOW)
                delay(10);
        }

        if (conStat == CON_MENU)
        {
            delay(10);
            if (guiIdleTimedOut(menuTimeout))
            {
                menuTimeout = millis();
                powerSave();
            }

            if (encoder0Pos != posNow)
            {
                saveTimeout = millis();
                menuTimeout = millis();
                line = 15; // line variable reset
                if (encoder0Pos > posNow)
                {
                    ms.next();
                    // ms.display();
                }
                else
                {
                    ms.prev();
                    // ms.display();
                }
                ms.display();
                posNow = encoder0Pos;
            }
            else
            {
                if ((digitalRead(keyPush) == LOW))
                {
                    saveTimeout = millis();
                    menuTimeout = millis();
                    currentTime = millis();
                    line = 15; // line variable reset
                    while (digitalRead(keyPush) == LOW)
                    {
                        delay(10);
                        if ((millis() - currentTime) > 1500)
                            break;
                    };
                    if ((millis() - currentTime) > 1000)
                    {
                        if (!ms.back())
                        {
                            conStat = CON_NORMAL;
                            menuSel = 0;
                            curTabOld = curTab + 1; // Refresh windows first
                            msgBox("BACK TO MENU");
                        }
                        else
                        {
                            msgBox("BACK");
                        }
                    }
                    else
                    {
                        ms.select();
                    }

                    while (digitalRead(keyPush) == LOW)
                    {
                        delay(10);
                    }
                    ms.display();
                    menuTimeout = millis();
                }
            }
            // ms.display();
        }
    }
}

// Routine
void line_angle(signed int startx, signed int starty, unsigned int length, unsigned int angle, unsigned int color)
{
    display.drawLine(startx, starty, (startx + length * cosf(angle * 0.017453292519)), (starty + length * sinf(angle * 0.017453292519)), color);
}

int xSpiGlcdSelFontHeight = 8;
int xSpiGlcdSelFontWidth = 5;

void compass_label(signed int startx, signed int starty, unsigned int length, double angle, unsigned int color)
{
    double angleNew;
    // ushort Color[2];
    uint8_t x_N, y_N, x_S, y_S;
    int x[4], y[4], i;
    int xOffset, yOffset;
    yOffset = (xSpiGlcdSelFontHeight / 2);
    xOffset = (xSpiGlcdSelFontWidth / 2);
    // GLCD_WindowMax();
    angle += 270.0F;
    angleNew = angle;
    for (i = 0; i < 4; i++)
    {
        if (angleNew > 360.0F)
            angleNew -= 360.0F;
        x[i] = startx + (length * cosf(angleNew * 0.017453292519));
        y[i] = starty + (length * sinf(angleNew * 0.017453292519));
        x[i] -= xOffset;
        y[i] -= yOffset;
        angleNew += 90.0F;
    }
    angleNew = angle + 45.0F;
    for (i = 0; i < 4; i++)
    {
        if (angleNew > 360.0F)
            angleNew -= 360.0F;
        x_S = startx + ((length - 3) * cosf(angleNew * 0.017453292519));
        y_S = starty + ((length - 3) * sinf(angleNew * 0.017453292519));
        x_N = startx + ((length + 3) * cosf(angleNew * 0.017453292519));
        y_N = starty + ((length + 3) * sinf(angleNew * 0.017453292519));
        angleNew += 90.0F;
        display.drawLine(x_S, y_S, x_N, y_N, color);
    }
    display.drawCircle(startx, starty, length, color);
    display.setFont();
    display.drawChar((uint8_t)x[0], (uint8_t)y[0], 'N', WHITE, BLACK, 1);
    display.drawChar((uint8_t)x[1], (uint8_t)y[1], 'E', WHITE, BLACK, 1);
    display.drawChar((uint8_t)x[2], (uint8_t)y[2], 'S', WHITE, BLACK, 1);
    display.drawChar((uint8_t)x[3], (uint8_t)y[3], 'W', WHITE, BLACK, 1);
}

void compass_arrow(signed int startx, signed int starty, unsigned int length, double angle, unsigned int color)
{
    double angle1, angle2;
    int xdst, ydst, x1sta, y1sta, x2sta, y2sta;
    int length2 = length / 2;
    angle += 270.0F;
    if (angle > 360.0F)
        angle -= 360.0F;
    xdst = startx + length * cosf(angle * 0.017453292519);
    ydst = starty + length * sinf(angle * 0.017453292519);
    angle1 = angle + 135.0F;
    if (angle1 > 360.0F)
        angle1 -= 360.0F;
    angle2 = angle + 225.0F;
    if (angle2 > 360.0F)
        angle2 -= 360.0F;
    x1sta = startx + length2 * cosf(angle1 * 0.017453292519);
    y1sta = starty + length2 * sinf(angle1 * 0.017453292519);
    x2sta = startx + length2 * cosf(angle2 * 0.017453292519);
    y2sta = starty + length2 * sinf(angle2 * 0.017453292519);
    display.drawLine(startx, starty, xdst, ydst, color);
    display.drawLine(xdst, ydst, x1sta, y1sta, color);
    display.drawLine(x1sta, y1sta, startx, starty, color);
    display.drawLine(startx, starty, x2sta, y2sta, color);
    display.drawLine(x2sta, y2sta, xdst, ydst, color);
}

void dispTxWindow(txDisp txs)
{
    if (config.tx_display == false)
        return;

    display.clearDisplay();
    disp_delay = config.dispDelay * 1000;
    timeHalfSec = millis() + disp_delay;
    // send_aprs_table = txs.table;
    // send_aprs_symbol = txs.symbol;

    // display.fillRect(0, 0, 128, 16, WHITE);
    // const uint8_t *ptrSymbol;
    // uint8_t symIdx = send_aprs_symbol - 0x21;
    // if (symIdx > 95)
    //     symIdx = 0;
    // if (send_aprs_table == '/')
    // {
    //     ptrSymbol = &Icon_TableA[symIdx][0];
    // }
    // else if (send_aprs_table == '\\')
    // {
    //     ptrSymbol = &Icon_TableB[symIdx][0];
    // }
    // else
    // {
    //     if (send_aprs_table < 'A' || send_aprs_table > 'Z')
    //     {
    //         send_aprs_table = 'N';
    //         send_aprs_symbol = '&';
    //         symIdx = 5; // &
    //     }
    //     ptrSymbol = &Icon_TableB[symIdx][0];
    // }
    // display.drawYBitmap(0, 0, ptrSymbol, 16, 16, WHITE);
    // if (!(send_aprs_table == '/' || send_aprs_table == '\\'))
    // {
    //     display.drawChar(5, 4, send_aprs_table, BLACK, WHITE, 1);
    //     display.drawChar(6, 5, send_aprs_table, BLACK, WHITE, 1);
    // }

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(0, 14);
    txs.name[sizeof(txs.name) - 1] = 0;
    if (strlen(txs.name))
    {
        display.printf("%s", txs.name);
    }

    display.setFont(&FreeSerifItalic9pt7b);

    if (txs.tx_ch == TXCH_TCP)
    {
        display.setCursor(5, 42);
        display.print("TCP");
        display.setCursor(15, 57);
        display.print("IP");
    }
    else if (txs.tx_ch == TXCH_RF)
    {
        display.setCursor(3, 42);
        display.print("SEND");
        display.setCursor(15, 57);
        display.print("RF");
    }
    else if (txs.tx_ch == TXCH_DIGI)
    {
        display.setCursor(3, 42);
        display.print("RPT");
        display.setCursor(15, 57);
        display.print("RF");
    }
    else if (txs.tx_ch == TXCH_3PTY)
    {
        display.setCursor(3, 42);
        display.print("FWD");
        display.setCursor(15, 57);
        display.print("RF");
    }

    display.setFont();
    display.setTextColor(WHITE);
    // display.setCursor(115, 0);
    // display.print("TX");

    display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
    display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
    display.setTextColor(BLACK);
    display.setCursor(40, 18);
    display.print("TX STATUS");

    display.setTextColor(WHITE);
    // display.setCursor(50, 30);

    char *pch;
    int y = 30;
    pch = strtok(txs.info, "\n");
    while (pch != NULL)
    {
        display.setCursor(50, y);
        display.printf("%s", pch);
        pch = strtok(NULL, "\n");
        y += 9;
    }

    display.display();
}

// char* directions[] = { "S", "SW", "W", "NW", "N", "NE", "E", "SE", "S" };
const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

void dispWindow(String line, uint8_t mode, bool filter)
{
    if (config.rx_display == false)
        return;

    oledWake(); // every popup counts as activity - wake the panel if it was sleeping

    struct pbuf_t aprs;
    bool Monitor = false;
    char text[300];
    unsigned char x = 0;
    char itemname[10];
    int start_val = line.indexOf(":}", 10);
    if (start_val > 0)
    {
        String new_info = line.substring(start_val + 2);
        start_val = new_info.indexOf(">", 0);
        if (start_val > 3 && start_val < 12)
            line = new_info;
    }
    start_val = line.indexOf(">", 0); // หาตำแหน่งแรกของ >
    if (start_val > 3 && start_val < 12)
    {
        powerWakeup();
        // Serial.println(line);
        String src_call = line.substring(0, start_val);
        memset(&aprs, 0, sizeof(pbuf_t));
        aprs.buf_len = 300;
        aprs.packet_len = line.length();
        if (aprs.packet_len > (int)sizeof(aprs.data) - 1)
            aprs.packet_len = (int)sizeof(aprs.data) - 1; // clamp: data[] is fixed-size, avoid overrun on oversized packets
        line.toCharArray(&aprs.data[0], aprs.packet_len + 1); // +1: toCharArray's 2nd arg is buffer size, not char count - avoids dropping the last byte
        int start_info = line.indexOf(":", 0);
        int end_ssid = line.indexOf(",", 0);
        int start_dst = line.indexOf(">", 2);
        int start_dstssid = line.indexOf("-", start_dst);
        if ((end_ssid < 0)||(end_ssid>start_info))
					end_ssid = start_info;
        if ((start_dstssid > start_dst) && (start_dstssid < start_dst + 10))
        {
            aprs.dstcall_end_or_ssid = &aprs.data[start_dstssid];
        }
        else
        {
            aprs.dstcall_end_or_ssid = &aprs.data[end_ssid];
        }
        aprs.info_start = &aprs.data[start_info + 1];
        aprs.dstname = &aprs.data[start_dst + 1];
        aprs.dstname_len = end_ssid - start_dst;
        aprs.dstcall_end = &aprs.data[end_ssid];
        aprs.srccall_end = &aprs.data[start_dst];

        // Serial.println(aprs.info_start);
        // aprsParse.parse_aprs(&aprs);
        if (aprsParse.parse_aprs(&aprs))
        {
            if (filter == true)
            {
                if ((config.dispFilter & FILTER_STATUS) && (aprs.packettype & T_STATUS))
                {
                    Monitor = true;
                }
                else if ((config.dispFilter & FILTER_MESSAGE) && (aprs.packettype & T_MESSAGE))
                {
                    Monitor = true;
                }
                else if ((config.dispFilter & FILTER_TELEMETRY) && (aprs.packettype & T_TELEMETRY))
                {
                    Monitor = true;
                }
                else if ((config.dispFilter & FILTER_WX) && ((aprs.packettype & T_WX) || (aprs.packettype & T_WAVE)))
                {
                    Monitor = true;
                }

                if ((config.dispFilter & FILTER_POSITION) && (aprs.packettype & T_POSITION))
                {
                    double lat, lon;
                    if (gps.location.isValid())
                    {
                        lat = gps.location.lat();
                        lon = gps.location.lng();
                    }
                    else
                    {
                        lat = config.igate_lat;
                        lon = config.igate_lon;
                    }
                    double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                    if (config.filterDistant == 0)
                    {
                        Monitor = true;
                    }
                    else
                    {
                        if (dist < config.filterDistant)
                            Monitor = true;
                        else
                            Monitor = false;
                    }
                }

                if ((config.dispFilter & FILTER_POSITION) && (aprs.packettype & T_POSITION))
                {
                    if (aprs.flags & F_CSRSPD)
                    {
                        double lat, lon;
                        if (gps.location.isValid())
                        {
                            lat = gps.location.lat();
                            lon = gps.location.lng();
                        }
                        else
                        {
                            lat = config.igate_lat;
                            lon = config.igate_lon;
                        }
                        double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                        if (config.filterDistant == 0)
                        {
                            Monitor = true;
                        }
                        else
                        {
                            if (dist < config.filterDistant)
                                Monitor = true;
                            else
                                Monitor = false;
                        }
                    }
                }

                if ((config.dispFilter & FILTER_POSITION) && (aprs.packettype & T_POSITION))
                {
                    if (aprs.flags & F_CSRSPD)
                    {
                        if (aprs.speed > 0)
                        {
                            double lat, lon;
                            if (gps.location.isValid())
                            {
                                lat = gps.location.lat();
                                lon = gps.location.lng();
                            }
                            else
                            {
                                lat = config.igate_lat;
                                lon = config.igate_lon;
                            }
                            double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                            if (config.filterDistant == 0)
                            {
                                Monitor = true;
                            }
                            else
                            {
                                if (dist < config.filterDistant)
                                    Monitor = true;
                                else
                                    Monitor = false;
                            }
                        }
                    }
                }
            }
            else
            {
                Monitor = true;
            }
        }
        else
        {
            return;
        }

        if (Monitor)
        {
            display.ssd1306_command(0xE4);
            delay(10);
            display.clearDisplay();
            if (dispPush)
            {
                disp_delay = 600 * 1000;
                display.drawRoundRect(0, 0, 128, 16, 3, WHITE);
            }
            else
            {
                disp_delay = config.dispDelay * 1000;
            }
            timeHalfSec = millis() + disp_delay;
            // display.fillRect(0, 0, 128, 16, WHITE);
            const uint8_t *ptrSymbol;
            uint8_t symIdx = aprs.symbol[1] - 0x21;
            if (symIdx > 95)
                symIdx = 0;
            if (aprs.symbol[0] == '/')
            {
                ptrSymbol = &Icon_TableA[symIdx][0];
            }
            else if (aprs.symbol[0] == '\\')
            {
                ptrSymbol = &Icon_TableB[symIdx][0];
            }
            else
            {
                if (aprs.symbol[0] < 'A' || aprs.symbol[0] > 'Z')
                {
                    aprs.symbol[0] = 'N';
                    aprs.symbol[1] = '&';
                    symIdx = 5; // &
                }
                ptrSymbol = &Icon_TableB[symIdx][0];
            }
            display.drawYBitmap(0, 0, ptrSymbol, 16, 16, WHITE);
            if (!(aprs.symbol[0] == '/' || aprs.symbol[0] == '\\'))
            {
                display.drawChar(5, 4, aprs.symbol[0], BLACK, WHITE, 1);
                display.drawChar(6, 5, aprs.symbol[0], BLACK, WHITE, 1);
            }
            display.setCursor(20, 7);
            display.setTextSize(1);
            display.setFont(&FreeSansBold9pt7b);

            if (aprs.srcname_len > 0)
            {
                memset(&itemname, 0, sizeof(itemname));
                memcpy(&itemname, aprs.srcname, aprs.srcname_len);
                Serial.println(itemname);
                display.print(itemname);
            }
            else
            {
                display.print(src_call);
            }

            display.setFont();
            display.setTextColor(WHITE);
            if (selTab < 10)
                display.setCursor(121, 0);
            else
                display.setCursor(115, 0);
            display.print(selTab);

            if (mode == 1)
            {
                display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                display.setTextColor(BLACK);
                display.setCursor(40, 18);
                display.print("TNC2 RAW");

                display.setFont();
                display.setCursor(2, 30);
                display.setTextColor(WHITE);
                display.print(line);

                display.display();
                return;
            }

            if (aprs.packettype & T_TELEMETRY)
            {
                bool show = false;
                int idx = tlmList_Find((char *)src_call.c_str());
                if (idx < 0)
                {
                    idx = tlmListOld();
                    if (idx > -1)
                        memset(&Telemetry[idx], 0, sizeof(Telemetry_struct));
                }
                if (idx > -1)
                {
                    Telemetry[idx].time = now();
                    strcpy(Telemetry[idx].callsign, (char *)src_call.c_str());

                    // for (int i = 0; i < 3; i++) Telemetry[idx].UNIT[i][5] = 0;
                    if (aprs.flags & F_UNIT)
                    {
                        memcpy(Telemetry[idx].UNIT, aprs.tlm_unit.val, sizeof(Telemetry[idx].UNIT));
                    }
                    else if (aprs.flags & F_PARM)
                    {
                        memcpy(Telemetry[idx].PARM, aprs.tlm_parm.val, sizeof(Telemetry[idx].PARM));
                    }
                    else if (aprs.flags & F_EQNS)
                    {
                        for (int i = 0; i < 15; i++)
                            Telemetry[idx].EQNS[i] = aprs.tlm_eqns.val[i];
                    }
                    else if (aprs.flags & F_BITS)
                    {
                        Telemetry[idx].BITS_FLAG = aprs.telemetry.bitsFlag;
                    }
                    else if (aprs.flags & F_TLM)
                    {
                        for (int i = 0; i < 5; i++)
                            Telemetry[idx].VAL[i] = aprs.telemetry.val[i];
                        Telemetry[idx].BITS = aprs.telemetry.bits;
                        show = true;
                    }

                    for (int i = 0; i < 4; i++)
                    { // Cut length
                        if (strstr(Telemetry[idx].PARM[i], "RxTraffic") != 0)
                            sprintf(Telemetry[idx].PARM[i], "RX");
                        if (strstr(Telemetry[idx].PARM[i], "TxTraffic") != 0)
                            sprintf(Telemetry[idx].PARM[i], "TX");
                        if (strstr(Telemetry[idx].PARM[i], "RxDrop") != 0)
                            sprintf(Telemetry[idx].PARM[i], "DROP");
                        Telemetry[idx].PARM[i][6] = 0;
                        Telemetry[idx].UNIT[i][3] = 0;
                        for (int a = 0; a < 3; a++)
                        {
                            if (Telemetry[idx].UNIT[i][a] == '/')
                                Telemetry[idx].UNIT[i][a] = 0;
                        }
                    }

                    for (int i = 0; i < 5; i++)
                    {
                        if (Telemetry[idx].PARM[i][0] == 0)
                        {
                            sprintf(Telemetry[idx].PARM[i], "CH%d", i + 1);
                        }
                    }
                }
                if (show || filter == false)
                {
                    display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                    display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                    display.setTextColor(BLACK);
                    display.setCursor(40, 18);
                    display.print("TELEMETRY");
                    display.setFont();
                    display.setTextColor(WHITE);
                    display.setCursor(2, 28);
                    display.print(Telemetry[idx].PARM[0]);
                    display.print(":");

                    if (fmod(Telemetry[idx].VAL[0], 1) == 0)
                        display.print(Telemetry[idx].VAL[0], 0);
                    else
                        display.print(Telemetry[idx].VAL[0], 1);
                    display.print(Telemetry[idx].UNIT[0]);
                    display.setCursor(65, 28);
                    display.print(Telemetry[idx].PARM[1]);
                    display.print(":");
                    if (fmod(Telemetry[idx].VAL[1], 1) == 0)
                        display.print(Telemetry[idx].VAL[1], 0);
                    else
                        display.print(Telemetry[idx].VAL[1], 1);
                    display.print(Telemetry[idx].UNIT[1]);
                    display.setCursor(2, 37);
                    display.print(Telemetry[idx].PARM[2]);
                    display.print(":");
                    if (fmod(Telemetry[idx].VAL[2], 1) == 0)
                        display.print(Telemetry[idx].VAL[2], 0);
                    else
                        display.print(Telemetry[idx].VAL[2], 1);
                    display.print(Telemetry[idx].UNIT[2]);
                    display.setCursor(65, 37);
                    display.print(Telemetry[idx].PARM[3]);
                    display.print(":");
                    if (fmod(Telemetry[idx].VAL[3], 1) == 0)
                        display.print(Telemetry[idx].VAL[3], 0);
                    else
                        display.print(Telemetry[idx].VAL[3], 1);
                    display.print(Telemetry[idx].UNIT[3]);
                    display.setCursor(2, 46);
                    display.print(Telemetry[idx].PARM[4]);
                    display.print(":");
                    display.print(Telemetry[idx].VAL[4], 1);
                    display.print(Telemetry[idx].UNIT[4]);

                    display.setCursor(4, 55);
                    display.print("BIT");
                    uint8_t bit = Telemetry[idx].BITS;
                    for (int i = 0; i < 8; i++)
                    {
                        if (bit & 0x80)
                        {
                            display.fillCircle(30 + (i * 12), 58, 3, WHITE);
                        }
                        else
                        {
                            display.drawCircle(30 + (i * 12), 58, 3, WHITE);
                        }
                        bit <<= 1;
                    }
                    // display.print(Telemetry[idx].BITS, BIN);

                    // display.setFont();
                    // display.setCursor(2, 30);
                    // memset(&text[0], 0, sizeof(text));
                    // memcpy(&text[0], aprs.comment, aprs.comment_len);
                    // display.setTextColor(WHITE);
                    // display.print(aprs.comment);
                    display.display();
                }
                return;
            }
            else if (aprs.packettype & T_STATUS)
            {
                display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                display.setTextColor(BLACK);
                display.setCursor(48, 18);
                display.print("STATUS");

                display.setFont();
                display.setCursor(2, 30);
                // memset(&text[0], 0, sizeof(text));
                // memcpy(&text[0], aprs.comment, aprs.comment_len);
                display.setTextColor(WHITE);
                display.print(aprs.comment);
                display.display();
                return;
            }
            else if (aprs.packettype & T_QUERY)
            {
                display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                display.setTextColor(BLACK);
                display.setCursor(48, 18);
                display.print("?QUERY?");
                // memset(&text[0], 0, sizeof(text));
                // memcpy(&text[0], aprs.comment, aprs.comment_len);
                display.setFont();
                display.setTextColor(WHITE);
                display.setCursor(2, 30);
                display.print(aprs.comment);
                display.display();
                return;
            }
            else if (aprs.packettype & T_MESSAGE)
            {
                if (aprs.msg.is_ack == 1)
                {
                }
                else if (aprs.msg.is_rej == 1)
                {
                }
                else
                {
                    display.drawRoundRect(0, 16, 128, 48, 5, WHITE);
                    display.fillRoundRect(1, 17, 126, 10, 2, WHITE);
                    display.setTextColor(BLACK);
                    display.setCursor(48, 18);
                    display.print("MESSAGE");
                    display.setCursor(100, 18);
                    display.print("{");
                    char txtID[7];
                    memset(txtID, 0, sizeof(txtID));
                    strncpy(&txtID[0], aprs.msg.msgid, aprs.msg.msgid_len);
                    display.printf("%s", txtID);
                    display.print("}");
                    // memset(&text[0], 0, sizeof(text));
                    // memcpy(&text[0], aprs.comment, aprs.comment_len);
                    display.setFont();
                    display.setTextColor(WHITE);
                    display.setCursor(2, 30);
                    display.print("To: ");
                    memset(text, 0, sizeof(text));
                    strncpy(&text[0], aprs.dstname, aprs.dstname_len);
                    display.print(text);
                    String mycall = "";
                    if (config.aprs_ssid > 0)
                        mycall = String(config.aprs_mycall) + String("-") + String(config.aprs_ssid, DEC);
                    else
                        mycall = String(config.aprs_mycall);
                    // if (strcmp(mycall.c_str(), text) == 0)
                    // {
                    //     display.setCursor(2, 54);
                    //     display.print("ACK:");
                    //     display.println(msgid);
                    //     String rawData = sendIsAckMsg(src_call, txtID);
                    //     log_d("IGATE_MSG: %s", rawData.c_str());
                    //     //if (config.igate_loc2rf)
                    //     { // IGATE SEND POSITION TO RF
                    //         char *rawP = (char *)malloc(rawData.length());
                    //         memcpy(rawP, rawData.c_str(), rawData.length());
                    //         pkgTxPush(rawP, rawData.length(), 0);
                    //         free(rawP);
                    //     }
                    //     // if (config.igate_loc2inet)
                    //     // { // IGATE SEND TO APRS-IS
                    //     //     if (aprsClient.connected())
                    //     //     {
                    //     //         aprsClient.println(rawData); // Send packet to Inet
                    //     //     }
                    //     // }
                    // }
                    memset(text, 0, sizeof(text));
                    strncpy(&text[0], aprs.msg.body, aprs.msg.body_len);
                    display.setCursor(2, 40);
                    display.print("Msg: ");
                    display.println(text);

                    display.display();
                }
                return;
            }
            display.setFont();
            display.drawFastHLine(0, 16, 128, WHITE);
            display.drawFastVLine(48, 16, 48, WHITE);
            x = 8;

            if (aprs.srcname_len > 0)
            {
                x += 9;
                display.fillRoundRect(51, 16, 77, 9, 2, WHITE);
                display.setTextColor(BLACK);
                display.setCursor(53, x);
                display.print("By " + src_call);
                display.setTextColor(WHITE);
            }

            if (aprs.packettype & T_WAVE)
            {
                // Serial.println("WX Display");
                if (aprs.wave_report.flags & O_TEMP)
                {
                    display.setCursor(58, x += 10);
                    display.drawYBitmap(51, x, &Temperature_Symbol[0], 5, 8, WHITE);
                    display.printf("%.2fC", aprs.wave_report.Temp);
                }
                if (aprs.wave_report.flags & O_HS)
                {
                    // display.setCursor(102, x);
                    display.setCursor(58, x += 9);
                    display.printf("Hs:");
                    display.printf("%0.1f M", aprs.wave_report.Hs / 100);
                }
                if (aprs.wave_report.flags & O_TZ)
                {
                    display.setCursor(58, x += 9);
                    display.printf("Tz: ");
                    display.printf("%0.1f S", aprs.wave_report.Tz);
                }
                // if (aprs.wave_report.flags & O_TC)
                // {
                //     display.setCursor(58, x += 9);
                //     display.printf("Tc: ");
                //     display.printf("%0.1fS.", aprs.wave_report.Tc);
                // }
                if (aprs.wave_report.flags & O_BAT)
                {
                    display.setCursor(58, x += 9);
                    display.printf("BAT: ");
                    display.printf("%0.2fV", aprs.wave_report.Bat);
                }
            }
            if (aprs.packettype & T_WX)
            {
                // Serial.println("WX Display");
                if (aprs.wx_report.flags & W_TEMP)
                {
                    display.setCursor(58, x += 10);
                    display.drawYBitmap(51, x, &Temperature_Symbol[0], 5, 8, WHITE);
                    display.printf("%.1fC", aprs.wx_report.temp);
                }
                if (aprs.wx_report.flags & W_HUM)
                {
                    display.setCursor(102, x);
                    display.drawYBitmap(95, x, &Humidity_Symbol[0], 5, 8, WHITE);
                    display.printf("%d%%", aprs.wx_report.humidity);
                }
                if (aprs.wx_report.flags & W_BAR)
                {
                    display.setCursor(58, x += 9);
                    display.drawYBitmap(51, x, &Pressure_Symbol[0], 5, 8, WHITE);
                    display.printf("%.1fhPa", aprs.wx_report.pressure);
                }
                if (aprs.wx_report.flags & W_R24H)
                {
                    // if (aprs.wx_report.rain_1h > 0) {
                    display.setCursor(58, x += 9);
                    display.drawYBitmap(51, x, &Rain_Symbol[0], 5, 8, WHITE);
                    display.printf("%.1fmm.", aprs.wx_report.rain_24h);
                    //}
                }
                if (aprs.wx_report.flags & W_PAR)
                {
                    // if (aprs.wx_report.luminosity > 10) {
                    display.setCursor(51, x += 9);
                    display.printf("%c", 0x0f);
                    display.setCursor(58, x);
                    display.printf("%dW/m", aprs.wx_report.luminosity);
                    if (aprs.wx_report.flags & W_UV)
                    {
                        display.printf(" UV%d", aprs.wx_report.uv);
                    }
                    //}
                }
                if (aprs.wx_report.flags & W_WS)
                {
                    display.setCursor(58, x += 9);
                    display.drawYBitmap(51, x, &Wind_Symbol[0], 5, 8, WHITE);
                    // int dirIdx=map(aprs.wx_report.wind_dir, -180, 180, 0, 8); ((angle+22)/45)%8]
                    int dirIdx = ((aprs.wx_report.wind_dir + 22) / 45) % 8;
                    if (dirIdx > 8)
                        dirIdx = 8;
                    display.printf("%.1fkPh(%s)", aprs.wx_report.wind_speed, directions[dirIdx]);
                }
                // Serial.printf("%.1fkPh(%d)", aprs.wx_report.wind_speed, aprs.wx_report.wind_dir);
                if (aprs.flags & F_HASPOS)
                {
                    // Serial.println("POS Display");
                    double lat, lon;
                    if (gps.location.isValid())
                    {
                        lat = gps.location.lat();
                        lon = gps.location.lng();
                    }
                    else
                    {
                        lat = config.igate_lat;
                        lon = config.igate_lon;
                    }
                    double dtmp = aprsParse.direction(lon, lat, aprs.lng, aprs.lat);
                    double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                    if (config.h_up == true)
                    {
                        // double course = gps.course.deg();
                        double course = SB_HEADING;
                        if (dtmp >= course)
                        {
                            dtmp -= course;
                        }
                        else
                        {
                            double diff = dtmp - course;
                            dtmp = diff + 360.0F;
                        }
                        compass_label(25, 37, 15, course, WHITE);
                        display.setCursor(0, 17);
                        display.printf("H");
                    }
                    else
                    {
                        compass_label(25, 37, 15, 0.0F, WHITE);
                    }
                    // compass_label(25, 37, 15, 0.0F, WHITE);
                    compass_arrow(25, 37, 12, dtmp, WHITE);
                    display.drawFastHLine(1, 63, 45, WHITE);
                    display.drawFastVLine(1, 58, 5, WHITE);
                    display.drawFastVLine(46, 58, 5, WHITE);
                    display.setCursor(4, 55);
                    if (dist > 999)
                        display.printf("%.fKm", dist);
                    else
                        display.printf("%.1fKm", dist);
                }
                else
                {
                    display.setCursor(20, 30);
                    display.printf("NO\nPOSITION");
                }
            }
            else if (aprs.flags & F_HASPOS)
            {
                // display.setCursor(50, x += 10);
                // display.printf("LAT %.5f\n", aprs.lat);
                // display.setCursor(51, x+=9);
                // display.printf("LNG %.4f\n", aprs.lng);
                String str;
                int l = 0;
                display.setCursor(50, x += 10);
                display.print("LAT:");
                str = String(aprs.lat, 5);
                l = str.length() * 6;
                display.setCursor(128 - l, x);
                display.print(str);

                display.setCursor(50, x += 9);
                display.print("LON:");
                str = String(aprs.lng, 5);
                l = str.length() * 6;
                display.setCursor(128 - l, x);
                display.print(str);

                double lat, lon;
                if (gps.location.isValid())
                {
                    lat = gps.location.lat();
                    lon = gps.location.lng();
                }
                else
                {
                    lat = config.igate_lat;
                    lon = config.igate_lon;
                }
                double dtmp = aprsParse.direction(lon, lat, aprs.lng, aprs.lat);
                double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
                if (config.h_up == true)
                {
                    // double course = gps.course.deg();
                    double course = SB_HEADING;
                    if (dtmp >= course)
                    {
                        dtmp -= course;
                    }
                    else
                    {
                        double diff = dtmp - course;
                        dtmp = diff + 360.0F;
                    }
                    compass_label(25, 37, 15, course, WHITE);
                    display.setCursor(0, 17);
                    display.printf("H");
                }
                else
                {
                    compass_label(25, 37, 15, 0.0F, WHITE);
                }
                compass_arrow(25, 37, 12, dtmp, WHITE);
                display.drawFastHLine(1, 55, 45, WHITE);
                display.drawFastVLine(1, 55, 5, WHITE);
                display.drawFastVLine(46, 55, 5, WHITE);
                display.setCursor(4, 57);
                if (dist > 999)
                    display.printf("%.fKm", dist);
                else
                    display.printf("%.1fKm", dist);
                if (aprs.flags & F_CSRSPD)
                {
                    display.setCursor(51, x += 9);
                    // display.printf("SPD %d/", aprs.course);
                    // display.setCursor(50, x += 9);
                    display.printf("SPD %.1fkPh\n", aprs.speed);
                    int dirIdx = ((aprs.course + 22) / 45) % 8;
                    if (dirIdx > 8)
                        dirIdx = 8;
                    display.setCursor(51, x += 9);
                    display.printf("CSD %d(%s)", aprs.course, directions[dirIdx]);
                }
                if (aprs.flags & F_ALT)
                {
                    display.setCursor(51, x += 9);
                    display.printf("ALT %.1fM\n", aprs.altitude);
                }
                if (aprs.flags & F_PHG)
                {
                    int power, height, gain;
                    power = (int)aprs.phg[0] - 0x30;
                    power *= power;
                    height = (int)aprs.phg[1] - 0x30;
                    height = 10 << (height + 1);
                    height = height / 3.2808;
                    gain = (int)aprs.phg[2] - 0x30;
                    display.setCursor(51, x += 9);
                    display.printf("PHG %dM.\n", height);
                    display.setCursor(51, x += 9);
                    display.printf("PWR %dWatt\n", power);
                    display.setCursor(51, x += 9);
                    display.printf("ANT %ddBi\n", gain);
                }
                if (aprs.flags & F_RNG)
                {
                    display.setCursor(51, x += 9);
                    display.printf("RNG %dKm\n", aprs.radio_range);
                }
                /*if (aprs.comment_len > 0) {
                    display.setCursor(0, 56);
                    display.print(aprs.comment);
                }*/
            }
            display.display();
        }
    }
}

String cut_string(String input, String header)
{
    if (input.indexOf(header) != -1) // ตรวจสอบว่าใน input มีข้อความเหมือนใน header หรือไม่
    {
        int num_get = input.indexOf(header); // หาตำแหน่งของข้อความ get_string ใน input
        if (num_get != -1)                   // ตรวจสอบว่าตำแหน่งที่ได้ไม่ใช่ -1 (ไม่มีข้อความ get_string ใน input)
        {
            int start_val = input.indexOf(">", num_get) + 1; // หาตำแหน่งแรกของ “
            int stop_val = input.indexOf(",", start_val);    // หาตำแหน่งสุดท้ายของ “
            return (input.substring(start_val, stop_val));   // ตัดเอาข้อความระหว่า “แรก และ ”สุดท้าย
        }
        else
        {
            return ("NULL"); // Return ข้อความ NULL เมื่อไม่ตรงเงื่อนไข
        }
    }

    return ("NULL"); // Return ข้อความ NULL เมื่อไม่ตรงเงื่อนไข
}