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
void on_display_selected(MenuItem *p_menu_item)
{
    // MyTextBox txtBox;
    MyCheckBox chkBoxWiFi;
    MyComboBox cbDim, cbContrast, cbStartup;
    String str;
    int x;
    int max_sel = 4;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("Display Config");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBoxWiFi.Checked = config.title;
    chkBoxWiFi.x = 0;
    chkBoxWiFi.y = 18;
    sprintf(chkBoxWiFi.text, "TITLE");

    display.setCursor(0, 30);
    display.print("DIM:");
    cbDim.isValue = false;
    cbDim.x = 25;
    cbDim.y = 28;
    cbDim.length = 9;
    cbDim.maxItem(5);
    cbDim.AddItem(0, "HI");
    cbDim.AddItem(1, "LOW");
    cbDim.AddItem(2, "AUTO DIM");
    cbDim.AddItem(3, "DAY/NIGHT");
    cbDim.AddItem(4, "CONTRAST");

    // cbBox.char_max = 999;
    cbDim.SetIndex(config.dim);

    display.setCursor(0, 42);
    display.print("CONTRAST:");
    cbContrast.isValue = true;
    cbContrast.x = 55;
    cbContrast.y = 40;
    cbContrast.length = 3;
    cbContrast.maxItem(255);
    cbContrast.char_max = 255;
    cbContrast.SetIndex(config.contrast);

    display.setCursor(0, 54);
    display.print("FDP:");
    cbStartup.isValue = false;
    cbStartup.x = 25;
    cbStartup.y = 52;
    cbStartup.length = 10;
    cbStartup.maxItem(6);
    cbStartup.AddItem(0, "STATUS");
    cbStartup.AddItem(1, "LAST STA");
    cbStartup.AddItem(2, "TOP PKG");
    cbStartup.AddItem(3, "SYS INFO");
    cbStartup.AddItem(4, "GPS INFO");
    cbStartup.AddItem(5, "CST/SPD");
    if (config.startup > 5)
        config.startup = 5;
    cbStartup.SetIndex(config.startup);

    display.display();
    encoder0Pos = 0;
    delay(100);
    do
    {
        if (encoder0Pos >= max_sel)
            encoder0Pos = 0;
        if (encoder0Pos < 0)
            encoder0Pos = max_sel - 1;
        if (keyPrev != encoder0Pos)
        {
            keyPrev = encoder0Pos;
            cbDim.isSelect = false;
            cbContrast.isSelect = false;
            cbStartup.isSelect = false;
            chkBoxWiFi.isSelect = false;
            if (encoder0Pos == 1)
                cbDim.isSelect = true;
            if (encoder0Pos == 2)
                cbContrast.isSelect = true;
            if (encoder0Pos == 3)
                cbStartup.isSelect = true;
            if (encoder0Pos == 0)
                chkBoxWiFi.isSelect = true;
            cbDim.Show();
            cbContrast.Show();
            chkBoxWiFi.CheckBoxShow();
            cbStartup.Show();
        }
        else
        {
            delay(50);
        }
        if (digitalRead(keyPush) == LOW)
        {
            currentTime = millis();
            while (digitalRead(keyPush) == LOW)
            {
                delay(10);
                if ((millis() - currentTime) > 2000)
                {
                    // msgBox("KEY Back");
                    break; // OK Timeout
                }
            };
            if ((millis() - currentTime) < 1500)
            {
                if (encoder0Pos == 0)
                {
                    chkBoxWiFi.Toggle();
                    config.title = chkBoxWiFi.Checked;
                    encoder0Pos = keyPrev;
                    chkBoxWiFi.CheckBoxShow();
                }
                else if (encoder0Pos == 1)
                {
                    cbDim.SelectItem();
                    config.dim = cbDim.GetIndex();
                    if (config.dim == 1)
                    {
                        display.dim(true);
                    }
                    else if (config.dim == 4)
                    {
                        // display.dim(true);
                        display.ssd1306_command(SSD1306_SETCONTRAST);
                        display.ssd1306_command(config.contrast);
                    }
                    else
                    {
                        display.dim(false);
                    }
                    cbDim.Show();
                }
                else if (encoder0Pos == 2)
                {
                    cbContrast.SelectValue(0, 200, 1);
                    config.contrast = cbContrast.GetValue();
                    // display.ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9
                    // display.ssd1306_command(config.contrast);
                    // display.ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
                    // display.ssd1306_command(config.contrast);
                    display.ssd1306_command(SSD1306_SETCONTRAST);
                    display.ssd1306_command(config.contrast);
                    cbContrast.Show();
                }
                else if (encoder0Pos == 3)
                {
                    cbStartup.SelectItem();
                    config.startup = cbStartup.GetIndex();
                    cbStartup.Show();
                }
                encoder0Pos = keyPrev;
                while (digitalRead(keyPush) == LOW)
                    delay(10);
            }
            else
            {
                break;
            }
        }
        delay(10);
    } while (1);
    /*display.clearDisplay();
    display.setCursor(30, 4);
    display.print("SAVE & EXIT");
    display.display();*/
    msgBox("KEY EXIT");
    while (digitalRead(keyPush) == LOW)
        delay(10);
}

void on_information_selected(MenuItem *p_menu_item)
{
    String str;
    int x;

    char strCID[50];
    uint64_t chipid = ESP.getEfuseMac();
    sprintf(strCID, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("INFORMATION");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    display.setCursor(0, 18);
    display.print("Firmware: V");
    display.printf("%s%c\n", VERSION, VERSION_BUILD);
    display.printf("Build: %s %s\n", __DATE__, __TIME__);
    display.printf("ESP32 Model: %s\n", ESP.getChipModel());
    display.printf("ID:%s\n", strCID);
    display.printf("Flash: %dMB\n", ESP.getFlashChipSize() / 1000000);
    display.printf("RF Type: %s\n", RF_TYPE[config.rf_type]);
    display.display();
    if (p_menu_item != NULL)
    {
        while (digitalRead(keyPush) == HIGH)
            delay(10);
    }
}

void on_save_selected(MenuItem *p_menu_item)
{
    saveEEPROM();

    display.clearDisplay();
    display.setCursor(52, 4);
    display.print("SAVE");
    display.setCursor(0, 18);
    display.print("Save All Configure\n to EEPROM");
    display.display();
    delay(1000);
    while (digitalRead(keyPush) == LOW)
        delay(10);
}

void on_load_selected(MenuItem *p_menu_item)
{
    uint8_t *ptr;

    ptr = (byte *)&config;
    EEPROM.readBytes(1, ptr, sizeof(Configuration));
    uint8_t chkSum = checkSum(ptr, sizeof(Configuration));
    projLog(LOGCAT_SYSTEM, "EEPROM Check %0Xh=%0Xh(%dByte)", EEPROM.read(0), chkSum, sizeof(Configuration));

    display.clearDisplay();
    display.setCursor(52, 4);
    display.print("LOAD");
    display.setCursor(0, 18);
    if (EEPROM.read(0) != chkSum)
    {
        display.print("Load Configuration OK!");
        projLog(LOGCAT_SYSTEM, "Config EEPROM Error!");
        // defaultConfig();
    }
    else
    {
        display.print("Load Configuration Fail!");
    }
    display.display();
    delay(1000);
    while (digitalRead(keyPush) == LOW)
        delay(10);
}

void on_factory_selected(MenuItem *p_menu_item)
{
    defaultConfig();
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
    delay(1000);
    while (digitalRead(keyPush) == LOW)
        delay(10);
}

void on_reboot_selected(MenuItem *p_menu_item)
{
    display.clearDisplay();
    display.setCursor(52, 4);
    display.print("REBOOT");
    display.setCursor(0, 18);
    display.print("SYSTEM REBOOT");
    display.display();
    delay(1000);
    while (digitalRead(keyPush) == LOW)
        delay(10);
    WiFi.disconnect(true);
    ESP.restart();
}

void on_dashboard_selected(MenuItem *p_menu_item)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        conStatNetwork = CON_SERVER;
        topBar(WiFi.RSSI());
    }
    else
    {
        conStatNetwork = CON_WIFI;
    }
    conStat = CON_NORMAL;
    display.clearDisplay();
    display.display();
}

void on_wifistatus_selected(MenuItem *p_menu_item)
{
    String str;
    // char ch[10];
    int x;
    // int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("WIFI STATUS");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    // display.setTextSize(1);
    display.setCursor(0, 16);
    display.print("SSID: ");
    display.print(WiFi.SSID());
    display.setCursor(0, 24);
    display.print("RSSI: ");
    display.print(WiFi.RSSI());
    display.println(" dBm");
    // display.setCursor(20, 32);
    display.print("MAC ");
    display.println(WiFi.macAddress());
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.print("GW: ");
    display.print(WiFi.gatewayIP());
    display.display();
    while (digitalRead(keyPush) == HIGH)
        delay(10);
}

void on_txbeacon_selected(MenuItem *p_menu_item)
{
    String str;
    int x;
    String rawTNC = myBeacon(String(",WIDE1-1"));
    // sprintf(cstr, "=%s%c%s%c%s%s\r\n", config.mylat, config.mysymbol[0], config.mylon, config.mysymbol[1], config.myphg, config.mycomment);
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("nTNC TX Beacon");
    x = str.length() * 6;
    // tncTxEnable = false;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);
    display.fillRect(0, 16, 128, 48, BLACK);
    display.setCursor(1, 25);
    display.print(rawTNC);
    display.display();
    // SerialTNC.print("\r\n");
    // SerialTNC.println("}" + rawTNC);
    delay(2000);
    // tncTxEnable = true;
}

void on_txstatus_selected(MenuItem *p_menu_item)
{
    String str;
    char cstr[300];
    int x;
    sprintf(cstr, ">WiFi IGate V%s%c\r\n", VERSION, VERSION_BUILD);
    // SerialTNC.flush();
    // tncTxEnable = false;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("nTNC TX RAW");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);
    display.fillRect(0, 16, 128, 48, BLACK);
    display.setCursor(1, 25);
    display.print(cstr);
    display.display();
}

byte htod(char *val, int str, int stp)
{
    char cstr[3];
    byte ret;
    cstr[0] = val[str];
    cstr[1] = val[stp];
    cstr[2] = 0;
    ret = (byte)strtol(cstr, 0, 16);
    return ret;
}

void on_back_selected(MenuItem *p_menu_item)
{
    line = 15;
    ms.back();
    ms.display();
}

