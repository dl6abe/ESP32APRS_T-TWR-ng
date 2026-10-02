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
void on_digi_position_selected(MenuComponent *p_menu_item)
{
    int max_sel = 6;
    MyTextBox txtBox[3];
    MySymbolBox symBox;
    MyCheckBox chkGPS;
    MyComboBox cbBox;
    String str;
    char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("DIGI POSITION");
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
    str = String(config.digi_mycall);
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
    cbBox.SetIndex(config.digi_ssid);

    display.setCursor(0, 30);
    display.print("LAT:");
    txtBox[1].x = 26;
    txtBox[1].y = 28;
    txtBox[1].length = 8;
    txtBox[1].type = 1;
    sprintf(txtBox[1].text, "%.5f", config.digi_lat);

    display.setCursor(0, 42);
    display.print("LON:");
    txtBox[2].x = 26;
    txtBox[2].y = 40;
    txtBox[2].length = 9;
    txtBox[2].type = 1;
    sprintf(txtBox[2].text, "%.5f", config.digi_lon);

    chkGPS.Checked = config.digi_gps;
    chkGPS.isSelect = false;
    chkGPS.x = 0;
    chkGPS.y = 54;
    sprintf(chkGPS.text, "GPS");
    // chkGPS.CheckBoxShow();

    display.fillRect(98, 31, 30, 11, WHITE);
    display.setTextColor(BLACK);
    display.setCursor(100, 33);
    display.print("ICON");
    display.setTextColor(WHITE);
    symBox.x = 98;
    symBox.y = 43;
    symBox.table = config.digi_symbol[0];
    symBox.symbol = config.digi_symbol[1];
    symBox.SetIndex(config.digi_symbol[1]);

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
                if (i == 5)
                    symBox.isSelect = false;
                if (i == 4)
                    chkGPS.isSelect = false;
            }
            if (encoder0Pos < 3)
                txtBox[encoder0Pos].isSelect = true;
            if (encoder0Pos == 5)
                symBox.isSelect = true;
            if (encoder0Pos == 4)
                chkGPS.isSelect = true;
            if (encoder0Pos == 3)
                cbBox.isSelect = true;
            for (i = 0; i < 3; i++)
                txtBox[i].TextBoxShow();
            symBox.Show();
            chkGPS.CheckBoxShow();
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
                if (i == 5)
                {
                    symBox.SelectItem();
                    config.digi_symbol[0] = symBox.table;
                    config.digi_symbol[1] = symBox.symbol;
                    encoder0Pos = keyPrev;
                }
                else if (i == 4)
                {
                    chkGPS.Toggle();
                    config.digi_gps = chkGPS.Checked;
                    encoder0Pos = keyPrev;
                    chkGPS.CheckBoxShow();
                }
                else if (i == 3)
                {
                    cbBox.SelectValue(0, 15, 1);
                    config.digi_ssid = cbBox.GetValue();
                    encoder0Pos = keyPrev;
                    cbBox.Show();
                }
                else
                {
                    txtBox[i].TextBox();
                    switch (i)
                    {
                    case 0:
                        strncpy(config.digi_mycall, txtBox[i].text, 7);
                        config.digi_mycall[7] = 0;
                        break;
                    // case 1: config.myssid = atol(txtBox[i].text);
                    //	break;
                    case 1:
                        config.digi_lat = atof(txtBox[i].text); // strcpy(config.mylat, txtBox[i].text);
                        break;
                    case 2:
                        config.digi_lon = atof(txtBox[i].text); // strcpy(config.mylon, txtBox[i].text);
                        break;
                        // case 4: strcpy(config.mysymbol, txtBox[i].text);
                        //	break;
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

void on_digi_function_selected(MenuComponent *p_menu_item)
{
    int max_sel = 8;
    // MyTextBox txtBox[2];
    MyCheckBox chkBox[4];
    MyComboBox cbBox[2];
    String str;
    // char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=DIGI FUNCION=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = config.digi_en;
    chkBox[0].x = 0;
    chkBox[0].y = 18;
    sprintf(chkBox[0].text, "Enable");

    chkBox[1].Checked = config.digi_bcn;
    chkBox[1].x = 0;
    chkBox[1].y = 29;
    sprintf(chkBox[1].text, "BCN");

    chkBox[2].Checked = config.digi_loc2rf;
    chkBox[2].x = 0;
    chkBox[2].y = 41;
    sprintf(chkBox[2].text, "POS2RF");

    chkBox[3].Checked = config.digi_loc2inet;
    chkBox[3].x = 60;
    chkBox[3].y = 41;
    sprintf(chkBox[3].text, "POS2INET");

    display.setCursor(35, 30);
    display.print("INTERVAL");
    cbBox[0].isValue = true;
    cbBox[0].x = 83;
    cbBox[0].y = 29;
    cbBox[0].length = 4;
    cbBox[0].char_max = 1800;
    cbBox[0].SetIndex(config.digi_interval);

    display.setCursor(0, 53);
    display.print("PTH");
    cbBox[1].isValue = false;
    cbBox[1].x = 20;
    cbBox[1].y = 51;
    cbBox[1].length = 13;
    cbBox[1].maxItem(PATH_LEN);
    for (i = 0; i < PATH_LEN; i++)
    {
        cbBox[1].AddItem(i, PATH_NAME[i]);
    }
    cbBox[1].SetIndex(config.digi_path);

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
            for (i = 0; i < 4; i++)
                chkBox[i].isSelect = false;
            cbBox[0].isSelect = false;
            cbBox[1].isSelect = false;

            if (encoder0Pos < 4)
                chkBox[encoder0Pos].isSelect = true;
            if (encoder0Pos == 4)
                cbBox[0].isSelect = true;
            if (encoder0Pos == 5)
                cbBox[1].isSelect = true;

            for (i = 0; i < 4; i++)
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
                if (i == 5) // Select PATH
                {
                    cbBox[1].SelectItem();
                    int n = cbBox[1].GetIndex();
                    if (n < PATH_LEN)
                    {
                        config.digi_path=n;
                        //strcpy(config.digi_path, config.path[n]);
                    }
                    encoder0Pos = keyPrev;
                    cbBox[1].Show();
                }
                else if (i == 4)
                {
                    cbBox[0].SelectValue(0, 1800, 60);
                    config.digi_interval = cbBox[0].GetValue();
                    encoder0Pos = keyPrev;
                    cbBox[0].Show();
                }
                else
                {
                    chkBox[i].Toggle();
                    switch (i)
                    {
                    case 0:
                        config.digi_en = chkBox[i].Checked;
                        break;
                    case 1:
                        config.digi_bcn = chkBox[i].Checked;
                        break;
                    case 2:
                        config.digi_loc2rf = chkBox[i].Checked;
                        break;
                    case 3:
                        config.digi_loc2inet = chkBox[i].Checked;
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

void on_digi_option_selected(MenuComponent *p_menu_item)
{
    int max_sel = 4;
    MyTextBox txtBox;
    MyCheckBox chkBox;
    MyComboBox cbBox[2];
    String str;
    // char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=DIGI OPTION=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox.Checked = false;
    chkBox.x = 0;
    chkBox.y = 18;
    sprintf(chkBox.text, "TLM");

    display.setCursor(35, 20);
    display.print("INTERVAL");
    cbBox[0].isValue = true;
    cbBox[0].x = 85;
    cbBox[0].y = 18;
    cbBox[0].length = 4;
    cbBox[0].char_max = 1800;
    cbBox[0].SetIndex(0);

    display.setCursor(0, 32);
    display.print("RPT_DELAY");
    cbBox[1].isValue = true;
    cbBox[1].x = 57;
    cbBox[1].y = 30;
    cbBox[1].length = 5;
    cbBox[1].char_max = 9999;
    cbBox[1].SetIndex(config.digi_delay);
    display.setCursor(110, 32);
    display.print("mS");

    display.setCursor(0, 44);
    display.print("TXT:");
    txtBox.x = 20;
    txtBox.y = 42;
    txtBox.length = 14;
    txtBox.type = ALL;
    strcpy(txtBox.text, config.digi_comment);

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

            for (i = 0; i < 2; i++)
                cbBox[i].isSelect = false;
            chkBox.isSelect = false;
            txtBox.isSelect = false;

            if (encoder0Pos == 0)
                chkBox.isSelect = true;
            if (encoder0Pos > 0 && encoder0Pos < 3)
                cbBox[encoder0Pos - 1].isSelect = true;
            if (encoder0Pos == 3)
                txtBox.isSelect = true;
            for (i = 0; i < 2; i++)
                cbBox[i].Show();
            chkBox.CheckBoxShow();
            txtBox.TextBoxShow();
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
                if (i == 3)
                {
                    txtBox.TextBox();
                    strcpy(config.digi_comment, txtBox.text);
                    encoder0Pos = keyPrev;
                }
                else if (i == 0)
                {
                    chkBox.Toggle();
                    // config.digi_tlm = chkBox.Checked;
                    encoder0Pos = keyPrev;
                    chkBox.CheckBoxShow();
                }
                else
                {
                    i -= 1;
                    switch (i)
                    {
                    case 0:
                        cbBox[i].SelectValue(0, 1800, 60);
                        // config.digi_tlm_interval = cbBox[i].GetValue();
                        break;
                    case 1:
                        cbBox[i].SelectValue(0, 9999, 10);
                        config.digi_delay = cbBox[i].GetValue();
                        break;
                    }
                    encoder0Pos = keyPrev;
                    cbBox[i].Show();
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


void on_filter_digi_selected(MenuComponent *p_menu_item)
{
    int max_sel = 10;
    MyCheckBox chkBox[10];
    String str;
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("DIGI Filter");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = (config.digiFilter & FILTER_OBJECT) ? 1 : 0;
    chkBox[0].x = 0;
    chkBox[0].y = 16;
    sprintf(chkBox[0].text, "OBJ");

    chkBox[1].Checked = (config.digiFilter & FILTER_QUERY) ? 1 : 0;
    chkBox[1].x = 35;
    chkBox[1].y = 16;
    sprintf(chkBox[1].text, "QRY");

    chkBox[2].Checked = (config.digiFilter & FILTER_STATUS) ? 1 : 0;
    chkBox[2].x = 75;
    chkBox[2].y = 16;
    sprintf(chkBox[2].text, "STATUS");

    chkBox[3].Checked = (config.digiFilter & FILTER_WX) ? 1 : 0;
    chkBox[3].x = 0;
    chkBox[3].y = 25;
    sprintf(chkBox[3].text, "WX");

    chkBox[4].Checked = (config.digiFilter & FILTER_TELEMETRY) ? 1 : 0;
    chkBox[4].x = 35;
    chkBox[4].y = 25;
    sprintf(chkBox[4].text, "TLM");

    chkBox[5].Checked = (config.digiFilter & FILTER_ITEM) ? 1 : 0;
    chkBox[5].x = 75;
    chkBox[5].y = 25;
    sprintf(chkBox[5].text, "ITEM");

    chkBox[6].Checked = (config.digiFilter & FILTER_MESSAGE) ? 1 : 0;
    chkBox[6].x = 0;
    chkBox[6].y = 34;
    sprintf(chkBox[6].text, "MSG");

    chkBox[7].Checked = (config.digiFilter & FILTER_POSITION) ? 1 : 0;
    chkBox[7].x = 35;
    chkBox[7].y = 34;
    sprintf(chkBox[7].text, "POS");

    chkBox[8].Checked = (config.digiFilter & FILTER_BUOY) ? 1 : 0;
    chkBox[8].x = 75;
    chkBox[8].y = 34;
    sprintf(chkBox[8].text, "BUOY");

    chkBox[9].Checked = (config.digiFilter & FILTER_MICE) ? 1 : 0;
    chkBox[9].x = 0;
    chkBox[9].y = 43;
    sprintf(chkBox[9].text, "MICE");

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
                chkBox[i].isSelect = false;
            }
            chkBox[encoder0Pos].isSelect = true;
            for (i = 0; i < max_sel; i++)
                chkBox[i].CheckBoxShow();
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
                if (i < max_sel)
                {
                    chkBox[i].Toggle();
                    switch (i)
                    {
                    case 0:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_OBJECT;
                        else
                            config.digiFilter &= ~FILTER_OBJECT;
                        break;
                    case 1:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_QUERY;
                        else
                            config.digiFilter &= ~FILTER_QUERY;
                        break;
                    case 2:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_STATUS;
                        else
                            config.digiFilter &= ~FILTER_STATUS;
                        break;
                    case 3:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_WX;
                        else
                            config.digiFilter &= ~FILTER_WX;
                        break;
                    case 4:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_TELEMETRY;
                        else
                            config.digiFilter &= ~FILTER_TELEMETRY;
                        break;
                    case 5:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_ITEM;
                        else
                            config.digiFilter &= ~FILTER_ITEM;
                        break;
                    case 6:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_MESSAGE;
                        else
                            config.digiFilter &= ~FILTER_MESSAGE;
                        break;
                    case 7:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_BUOY;
                        else
                            config.digiFilter &= ~FILTER_BUOY;
                        break;
                    case 8:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_POSITION;
                        else
                            config.digiFilter &= ~FILTER_POSITION;
                        break;
                    case 9:
                        if (chkBox[i].Checked)
                            config.digiFilter |= FILTER_MICE;
                        else
                            config.digiFilter &= ~FILTER_MICE;
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
}

