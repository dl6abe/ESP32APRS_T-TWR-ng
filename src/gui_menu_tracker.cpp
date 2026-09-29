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
void on_smartbeacon_selected(MenuItem *p_menu_item)
{
    int i;
    // MyCheckBox chkBox[3];
    int max_sel = 7;
    MySymbolBox symBox[2];
    MyComboBox cbBox[5];
    String str;
    int x;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=SMART BEACON=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    display.setCursor(0, 20);
    display.print("MINV");
    cbBox[0].isValue = true;
    cbBox[0].x = 25;
    cbBox[0].y = 18;
    cbBox[0].length = 3;
    cbBox[0].char_max = 999;
    cbBox[0].SetIndex(config.trk_maxinterval);

    display.setCursor(63, 20);
    display.print("LINV");
    cbBox[1].isValue = true;
    cbBox[1].x = 87;
    cbBox[1].y = 18;
    cbBox[1].length = 4;
    cbBox[1].char_max = 9999;
    cbBox[1].SetIndex(config.trk_slowinterval);

    display.setCursor(0, 32);
    display.print("HSPD");
    cbBox[2].isValue = true;
    cbBox[2].x = 25;
    cbBox[2].y = 30;
    cbBox[2].length = 3;
    cbBox[2].char_max = 300;
    cbBox[2].SetIndex(config.trk_hspeed);

    display.setCursor(0, 44);
    display.print("LSPD");
    cbBox[3].isValue = true;
    cbBox[3].x = 25;
    cbBox[3].y = 42;
    cbBox[3].length = 2;
    cbBox[3].char_max = 99;
    cbBox[3].SetIndex(config.trk_lspeed);

    display.setCursor(0, 56);
    display.print("ANG:");
    cbBox[4].isValue = true;
    cbBox[4].x = 25;
    cbBox[4].y = 54;
    cbBox[4].length = 2;
    cbBox[4].char_max = 180;
    cbBox[4].SetIndex(config.trk_minangle);

    display.setCursor(67, 35);
    display.print("MOV");
    symBox[0].x = 65;
    symBox[0].y = 43;
    symBox[0].table = config.trk_symmove[0];
    symBox[0].symbol = config.trk_symmove[1];
    symBox[0].SetIndex(config.trk_symmove[1]);
    symBox[0].Show();

    display.setCursor(99, 35);
    display.print("STP");
    symBox[1].x = 98;
    symBox[1].y = 43;
    symBox[1].table = config.trk_symstop[0];
    symBox[1].symbol = config.trk_symstop[1];
    symBox[1].SetIndex(config.trk_symstop[1]);
    symBox[1].Show();

    display.display();
    encoder0Pos = 0;
    delay(100);
    unsigned long lastActivity = millis();
    do
    {
        if (encoder0Pos >= max_sel)
            encoder0Pos = 0;
        if (encoder0Pos < 0)
            encoder0Pos = max_sel - 1;
        if (keyPrev != encoder0Pos)
        {
            keyPrev = encoder0Pos;
            lastActivity = millis();
            for (i = 0; i < 5; i++)
            {
                cbBox[i].isSelect = false;
            }
            symBox[0].isSelect = false;
            symBox[1].isSelect = false;

            if (encoder0Pos < 5)
                cbBox[encoder0Pos].isSelect = true;
            if (encoder0Pos > 4)
                symBox[encoder0Pos - 5].isSelect = true;
            for (i = 0; i < 5; i++)
            {
                cbBox[i].Show();
            }
            symBox[0].Show();
            symBox[1].Show();
        }
        else
        {
            delay(50);
            if (guiIdleTimedOut(lastActivity))
                break;
        }
        if (digitalRead(keyPush) == LOW)
        {
            currentTime = millis();
            lastActivity = millis();
            while (digitalRead(keyPush) == LOW)
            {
                if ((millis() - currentTime) > 2000)
                    break; // OK Timeout
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (i < 5)
                {
                    switch (i)
                    {
                    case 0:
                        cbBox[i].SelectValue(5, 999, 1);
                        config.trk_maxinterval = cbBox[i].GetValue();
                        break;
                    case 1:
                        cbBox[i].SelectValue(60, 9999, 60);
                        config.trk_slowinterval = cbBox[i].GetValue();
                        break;
                    case 2:
                        cbBox[i].SelectValue(5, 300, 1);
                        config.trk_hspeed = cbBox[i].GetValue();
                        break;
                    case 3:
                        cbBox[i].SelectValue(1, 99, 1);
                        config.trk_lspeed = cbBox[i].GetValue();
                        break;
                    case 4:
                        cbBox[i].SelectValue(5, 90, 1);
                        config.trk_minangle = cbBox[i].GetValue();
                        break;
                    }
                    encoder0Pos = keyPrev;
                    cbBox[i].Show();
                }
                else if (encoder0Pos > 4)
                {
                    i -= 5;
                    symBox[i].SelectItem();
                    switch (i)
                    {
                    case 0:
                        config.trk_symmove[0] = symBox[i].table;
                        config.trk_symmove[1] = symBox[i].symbol;
                        break;
                    case 1:
                        config.trk_symstop[0] = symBox[i].table;
                        config.trk_symstop[1] = symBox[i].symbol;
                        break;
                    }
                    encoder0Pos = keyPrev;
                    symBox[i].Show();
                }
                while (digitalRead(keyPush) == LOW)
                    ;
            }
            else
            {
                break;
            }
        }
    } while (1);
    msgBox("KEY EXIT");
    while (digitalRead(keyPush) == LOW)
        ;
    saveEEPROM();
}


void on_tracker_position_selected(MenuItem *p_menu_item)
{
    int max_sel = 7;
    MyTextBox txtBox[3];
    MySymbolBox symBox;
    MyCheckBox chkGPS, chkSMBeacon;
    MyComboBox cbBox;
    String str;
    char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("TRACKER POSITION");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    display.setCursor(0, 18);
    display.print("MyCall:");
    txtBox[0].x = 41;
    txtBox[0].y = 16;
    txtBox[0].length = 7;
    txtBox[0].type = 0;
    str = String(config.trk_mycall);
    str.toUpperCase();
    str.toCharArray(&ch[0], 10);
    strcpy(txtBox[0].text, ch);

    display.setCursor(95, 18);
    display.print("-");
    cbBox.isValue = true;
    cbBox.x = 101;
    cbBox.y = 16;
    cbBox.length = 2;
    cbBox.maxItem(15);
    cbBox.char_max = 15;
    cbBox.SetIndex(config.trk_ssid);

    display.setCursor(0, 30);
    display.print("LAT:");
    txtBox[1].x = 26;
    txtBox[1].y = 28;
    txtBox[1].length = 8;
    txtBox[1].type = 1;
    sprintf(txtBox[1].text, "%.5f", config.trk_lat);

    display.setCursor(0, 42);
    display.print("LON:");
    // TextBoxShow(&mylon[0], 26, 40,9);
    txtBox[2].x = 26;
    txtBox[2].y = 40;
    txtBox[2].length = 9;
    txtBox[2].type = 1;
    sprintf(txtBox[2].text, "%.5f", config.trk_lon);

    chkGPS.Checked = config.trk_gps;
    chkGPS.isSelect = false;
    chkGPS.x = 0;
    chkGPS.y = 54;
    sprintf(chkGPS.text, "GPS");

    chkSMBeacon.Checked = config.trk_smartbeacon;
    chkSMBeacon.isSelect = false;
    chkSMBeacon.x = 35;
    chkSMBeacon.y = 54;
    sprintf(chkSMBeacon.text, "SmartBCN");

    display.fillRect(98, 31, 30, 11, WHITE);
    display.setTextColor(BLACK);
    display.setCursor(100, 33);
    display.print("ICON");
    display.setTextColor(WHITE);
    symBox.x = 98;
    symBox.y = 43;
    symBox.table = config.trk_symbol[0];
    symBox.symbol = config.trk_symbol[1];
    symBox.SetIndex(config.trk_symbol[1]);

    display.display();
    encoder0Pos = 0;
    delay(100);
    unsigned long lastActivity = millis();
    do
    {
        if (encoder0Pos >= max_sel)
            encoder0Pos = 0;
        if (encoder0Pos < 0)
            encoder0Pos = max_sel - 1;
        if (keyPrev != encoder0Pos)
        {
            keyPrev = encoder0Pos;
            lastActivity = millis();
            for (i = 0; i < max_sel; i++)
            {
                if (i < 3)
                    txtBox[i].isSelect = false;
                if (i == 3)
                    cbBox.isSelect = false;
                if (i == 6)
                    symBox.isSelect = false;
                if (i == 5)
                    chkSMBeacon.isSelect = false;
                if (i == 4)
                    chkGPS.isSelect = false;
            }
            if (encoder0Pos < 3)
                txtBox[encoder0Pos].isSelect = true;
            if (encoder0Pos == 6)
                symBox.isSelect = true;
            if (encoder0Pos == 5)
                chkSMBeacon.isSelect = true;
            if (encoder0Pos == 4)
                chkGPS.isSelect = true;
            if (encoder0Pos == 3)
                cbBox.isSelect = true;
            for (i = 0; i < 3; i++)
                txtBox[i].TextBoxShow();
            symBox.Show();
            chkGPS.CheckBoxShow();
            chkSMBeacon.CheckBoxShow();
            cbBox.Show();
        }
        else
        {
            delay(50);
            if (guiIdleTimedOut(lastActivity))
                break;
        }
        if (digitalRead(keyPush) == LOW)
        {
            currentTime = millis();
            lastActivity = millis();
            while (digitalRead(keyPush) == LOW)
            {
                if ((millis() - currentTime) > 2000)
                {
                    break; // OK Timeout
                }
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (i == 6)
                {
                    symBox.SelectItem();
                    config.trk_symbol[0] = symBox.table;
                    config.trk_symbol[1] = symBox.symbol;
                    encoder0Pos = keyPrev;
                }
                else if (i == 4)
                {
                    chkGPS.Toggle();
                    config.trk_gps = chkGPS.Checked;
                    if (config.trk_gps == false)
                    {
                        chkSMBeacon.Checked = false;
                        chkSMBeacon.CheckBoxShow();
                    }
                    encoder0Pos = keyPrev;
                    chkGPS.CheckBoxShow();
                }
                else if (i == 5)
                {
                    chkSMBeacon.Toggle();
                    config.trk_smartbeacon = chkSMBeacon.Checked;
                    encoder0Pos = keyPrev;
                    chkSMBeacon.CheckBoxShow();
                }
                else if (i == 3)
                {
                    cbBox.SelectValue(0, 15, 1);
                    config.trk_ssid = cbBox.GetValue();
                    encoder0Pos = keyPrev;
                    cbBox.Show();
                }
                else
                {
                    txtBox[i].TextBox();
                    switch (i)
                    {
                    case 0:
                        strncpy(config.trk_mycall, txtBox[i].text, 7);
                        config.trk_mycall[7] = 0;
                        break;
                    case 1:
                        config.trk_lat = atof(txtBox[i].text); // strcpy(config.mylat, txtBox[i].text);
                        break;
                    case 2:
                        config.trk_lon = atof(txtBox[i].text); // strcpy(config.mylon, txtBox[i].text);
                        break;
                    }
                    encoder0Pos = keyPrev + 1;
                }
                while (digitalRead(keyPush) == LOW)
                    ;
            }
            else
            {
                break;
            }
        }
    } while (1);
    msgBox("KEY EXIT");
    while (digitalRead(keyPush) == LOW)
        ;
    saveEEPROM();
    initInterval=true;
}

void on_tracker_function_selected(MenuItem *p_menu_item)
{
    int max_sel = 8;
    // MyTextBox txtBox[2];
    MyCheckBox chkBox[6];
    MyComboBox cbBox[2];
    String str;
    // char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=TRACKER FUNCION=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = config.trk_en;
    chkBox[0].x = 0;
    chkBox[0].y = 18;
    sprintf(chkBox[0].text, "BCN");

    chkBox[1].Checked = config.trk_loc2rf;
    chkBox[1].x = 0;
    chkBox[1].y = 30;
    sprintf(chkBox[1].text, "POS2RF");

    chkBox[2].Checked = config.trk_loc2inet;
    chkBox[2].x = 60;
    chkBox[2].y = 30;
    sprintf(chkBox[2].text, "POS2INET");

    chkBox[3].Checked = config.trk_compress;
    chkBox[3].x = 0;
    chkBox[3].y = 40;
    sprintf(chkBox[3].text, "COMP");

    chkBox[4].Checked = config.trk_altitude;
    chkBox[4].x = 40;
    chkBox[4].y = 40;
    sprintf(chkBox[4].text, "ALT");

    chkBox[5].Checked = config.trk_cst;
    chkBox[5].x = 75;
    chkBox[5].y = 40;
    sprintf(chkBox[5].text, "CSR/SPD");

    display.setCursor(35, 20);
    display.print("INTERVAL");
    cbBox[0].isValue = true;
    cbBox[0].x = 85;
    cbBox[0].y = 18;
    cbBox[0].length = 4;
    cbBox[0].char_max = 1800;
    cbBox[0].SetIndex(config.trk_interval);

    display.setCursor(0, 52);
    display.print("PTH:");
    cbBox[1].isValue = false;
    cbBox[1].x = 20;
    cbBox[1].y = 50;
    cbBox[1].length = 13;
    cbBox[1].maxItem(PATH_LEN);
    for (i = 0; i < PATH_LEN; i++)
    {
        cbBox[1].AddItem(i, PATH_NAME[i]);
    }
    cbBox[1].SetIndex(config.trk_path);

    display.display();
    encoder0Pos = 0;
    delay(100);
    unsigned long lastActivity = millis();
    do
    {
        if (encoder0Pos >= max_sel)
            encoder0Pos = 0;
        if (encoder0Pos < 0)
            encoder0Pos = max_sel - 1;
        if (keyPrev != encoder0Pos)
        {
            keyPrev = encoder0Pos;
            lastActivity = millis();
            for (i = 0; i < 6; i++)
                chkBox[i].isSelect = false;
            cbBox[0].isSelect = false;
            cbBox[1].isSelect = false;

            if (encoder0Pos < 6)
                chkBox[encoder0Pos].isSelect = true;
            if (encoder0Pos == 6)
                cbBox[0].isSelect = true;
            if (encoder0Pos == 7)
                cbBox[1].isSelect = true;

            for (i = 0; i < 6; i++)
                chkBox[i].CheckBoxShow();
            cbBox[0].Show();
            cbBox[1].Show();
        }
        else
        {
            delay(50);
            if (guiIdleTimedOut(lastActivity))
                break;
        }
        if (digitalRead(keyPush) == LOW)
        {
            currentTime = millis();
            lastActivity = millis();
            while (digitalRead(keyPush) == LOW)
            {
                ;
                if ((millis() - currentTime) > 2000)
                    break; // OK Timeout
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (i == 7) // Select PATH
                {
                    cbBox[1].SelectItem();
                    int n = cbBox[1].GetIndex();
                    if (n < PATH_LEN)
                    {
                        config.trk_path=n;
                    }
                    encoder0Pos = keyPrev;
                    cbBox[1].Show();
                }
                else if (i == 6)
                {
                    cbBox[0].SelectValue(0, 1800, 60);
                    config.trk_interval = cbBox[0].GetValue();
                    encoder0Pos = keyPrev;
                    cbBox[0].Show();
                }
                else
                {
                    chkBox[i].Toggle();
                    switch (i)
                    {
                    case 0:
                        config.trk_en = chkBox[i].Checked;
                        break;
                    case 1:
                        config.trk_loc2rf = chkBox[i].Checked;
                        break;
                    case 2:
                        config.trk_loc2inet = chkBox[i].Checked;
                        break;
                    case 3:
                        config.trk_compress = chkBox[i].Checked;
                        break;
                    case 4:
                        config.trk_altitude = chkBox[i].Checked;
                        break;
                    case 5:
                        config.trk_cst = chkBox[i].Checked;
                        break;
                    }
                    encoder0Pos = keyPrev;
                    chkBox[i].CheckBoxShow();
                }
                while (digitalRead(keyPush) == LOW)
                    ;
            }
            else
            {
                break;
            }
        }
    } while (1);
    msgBox("KEY EXIT");
    while (digitalRead(keyPush) == LOW)
        ;
    saveEEPROM();
    initInterval=true;
}

void on_tracker_option_selected(MenuItem *p_menu_item)
{
    int max_sel = 6;
    MyTextBox txtBox[3];
    MyCheckBox chkBox[3];
    String str;
    // char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=TRACKER OPTION=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = config.trk_bat;
    chkBox[0].x = 0;
    chkBox[0].y = 18;
    sprintf(chkBox[0].text, "BAT");

    chkBox[1].Checked = config.trk_sat;
    chkBox[1].x = 40;
    chkBox[1].y = 18;
    sprintf(chkBox[1].text, "SAT");

    chkBox[2].Checked = config.trk_dx;
    chkBox[2].x = 75;
    chkBox[2].y = 18;
    sprintf(chkBox[2].text, "DX");

    // display.setCursor(0, 30);
    // display.print("OBJ:");
    // txtBox[0].x = 30;
    // txtBox[0].y = 28;
    // txtBox[0].length = 11;
    // txtBox[0].type = 0;
    // strcpy(txtBox[0].text, config.trk_object);

    display.setCursor(0, 42);
    display.print("ITEM");
    txtBox[1].x = 30;
    txtBox[1].y = 40;
    txtBox[1].length = 11;
    txtBox[1].type = 0;
    strcpy(txtBox[1].text, config.trk_item);

    display.setCursor(0, 54);
    display.print("TXT");
    txtBox[2].x = 20;
    txtBox[2].y = 52;
    txtBox[2].length = 14;
    txtBox[2].type = 0;
    strcpy(txtBox[2].text, config.trk_comment);

    display.display();
    encoder0Pos = 0;
    delay(100);
    unsigned long lastActivity = millis();
    do
    {
        if (encoder0Pos >= max_sel)
            encoder0Pos = 0;
        if (encoder0Pos < 0)
            encoder0Pos = max_sel - 1;
        if (keyPrev != encoder0Pos)
        {
            keyPrev = encoder0Pos;
            lastActivity = millis();
            for (i = 0; i < 3; i++)
                chkBox[i].isSelect = false;
            for (i = 0; i < 3; i++)
                txtBox[i].isSelect = false;

            if (encoder0Pos < 3)
                chkBox[encoder0Pos].isSelect = true;
            if (encoder0Pos > 2)
                txtBox[encoder0Pos - 3].isSelect = true;
            for (i = 0; i < 3; i++)
                chkBox[i].CheckBoxShow();
            for (i = 0; i < 3; i++)
                txtBox[i].TextBoxShow();
        }
        else
        {
            delay(50);
            if (guiIdleTimedOut(lastActivity))
                break;
        }
        if (digitalRead(keyPush) == LOW)
        {
            currentTime = millis();
            lastActivity = millis();
            while (digitalRead(keyPush) == LOW)
            {
                ;
                if ((millis() - currentTime) > 2000)
                    break; // OK Timeout
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (i > 2)
                {
                    switch (i)
                    {
                    case 3:
                        txtBox[0].TextBox();
                        // strcpy(config.trk_object, txtBox[0].text);
                        break;
                    case 4:
                        txtBox[1].TextBox();
                        strcpy(config.trk_item, txtBox[1].text);
                        break;
                    case 5:
                        txtBox[2].TextBox();
                        strcpy(config.trk_comment, txtBox[2].text);
                        break;
                    }
                    encoder0Pos = keyPrev;
                }
                else
                {
                    chkBox[i].Toggle();
                    switch (i)
                    {
                    case 0:
                        config.trk_bat = chkBox[i].Checked;
                        break;
                    case 1:
                        config.trk_sat = chkBox[i].Checked;
                        break;
                    case 2:
                        config.trk_dx = chkBox[i].Checked;
                        break;
                    }
                    encoder0Pos = keyPrev;
                    chkBox[i].CheckBoxShow();
                }
                while (digitalRead(keyPush) == LOW)
                    ;
            }
            else
            {
                break;
            }
        }
    } while (1);
    msgBox("KEY EXIT");
    while (digitalRead(keyPush) == LOW)
        ;
    saveEEPROM();
    initInterval=true;
}

