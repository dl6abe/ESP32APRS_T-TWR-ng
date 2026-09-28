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
void on_aprsserver_selected(MenuItem *p_menu_item)
{
    int max_sel = 5;
    MyTextBox txtBox[3];
    MyComboBox cbBox;
    MyCheckBox chkBox;
    String str;
    // char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=APRS SERVER=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox.Checked = config.igate_en;
    chkBox.isSelect = false;
    chkBox.x = 0;
    chkBox.y = 18;
    sprintf(chkBox.text, "Enable");
    // display.setCursor(0, 18);
    // display.print("HOST:");
    txtBox[0].x = 0;
    txtBox[0].y = 28;
    txtBox[0].length = 17;
    txtBox[0].type = 0;
    strcpy(txtBox[0].text, config.aprs_host);

    display.setCursor(58, 18);
    display.print("PORT");
    txtBox[1].x = 85;
    txtBox[1].y = 16;
    txtBox[1].length = 5;
    txtBox[1].type = 1;
    sprintf(txtBox[1].text, "%d", config.aprs_port);
    // strcpy(txtBox[1].text, config.wifi_password);

    display.setCursor(0, 42);
    display.print("Filter:");
    txtBox[2].x = 0;
    txtBox[2].y = 52;
    txtBox[2].length = 17;
    txtBox[2].type = 0;
    strcpy(txtBox[2].text, config.aprs_filter);

    cbBox.isValue = false;
    cbBox.x = 53;
    cbBox.y = 40;
    cbBox.length = 8;
    cbBox.AddItem(0, "CALLSIGN");   // g/HS*/E2*
    cbBox.AddItem(1, "THAI MSG");   // g/HS*/E2*
    cbBox.AddItem(2, "THAI ALL");   // b/HS*/E2*
    cbBox.AddItem(3, "THAI IGATE"); // e/HS*/E2*
    cbBox.AddItem(4, "THAI DIGI");  // d/HS*/E2*
    cbBox.AddItem(5, "NO RECV");    // m/1
    cbBox.maxItem(6);
    // cbBox.char_max = 999;
    cbBox.SetIndex(0);

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
            for (i = 0; i < 3; i++)
                txtBox[i].isSelect = false;
            cbBox.isSelect = false;
            chkBox.isSelect = false;
            if (encoder0Pos < 3)
                txtBox[encoder0Pos].isSelect = true;
            else if (encoder0Pos == 3)
                cbBox.isSelect = true;
            else if (encoder0Pos == 4)
                chkBox.isSelect = true;
            for (i = 0; i < 3; i++)
                txtBox[i].TextBoxShow();
            cbBox.Show();
            chkBox.CheckBoxShow();
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
                if ((millis() - currentTime) > 2000)
                {
                    break; // OK Timeout
                }
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (i < 3)
                {
                    txtBox[i].TextBox();
                    switch (i)
                    {
                    case 0:
                        strcpy(config.aprs_host, txtBox[0].text);
                        break;
                    case 1:
                        config.aprs_port = atol(txtBox[1].text);
                        break;
                    case 2:
                        strcpy(config.aprs_filter, txtBox[2].text);
                        break;
                    }
                }
                else if (encoder0Pos == 3)
                {
                    cbBox.SelectItem();
                    switch (cbBox.GetIndex())
                    {
                    case 0:
                        strcpy(txtBox[2].text, "b/HS5TQA-9");
                        break;
                    case 1:
                        strcpy(txtBox[2].text, "g/HS*/E2*");
                        break;
                    case 2:
                        strcpy(txtBox[2].text, "b/HS*/E2*");
                        break;
                    case 3:
                        strcpy(txtBox[2].text, "e/HS*/E2*");
                        break;
                    case 4:
                        strcpy(txtBox[2].text, "d/HS*/E2*");
                        break;
                    case 5:
                        strcpy(txtBox[2].text, "m/1");
                        break;
                    }
                    strcpy(config.aprs_filter, txtBox[2].text);
                }
                else if (encoder0Pos == 4)
                {
                    chkBox.Toggle();
                    config.igate_en = chkBox.Checked;
                    encoder0Pos = keyPrev;
                    chkBox.CheckBoxShow();
                }
                encoder0Pos = keyPrev + 1;
                while (digitalRead(keyPush) == LOW)
                    ;
            }
            else
            {
                break;
            }
        }
    } while (1);
    /*display.clearDisplay();
    display.setCursor(30, 4);
    display.print("SAVE & EXIT");
    display.display();*/
    msgBox("KEY EXIT");
    while (digitalRead(keyPush) == LOW)
        ;
    saveEEPROM();
    // client.flush();
    // client.clearWriteError();
    // delay(500);
    // client.stop();
    // conStatNetwork = CON_SERVER;
}

void on_igate_position_selected(MenuItem *p_menu_item)
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
    str = String("=IGATE POSITION=");
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
    str = String(config.aprs_mycall);
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
    cbBox.SetIndex(config.aprs_ssid);

    display.setCursor(0, 30);
    display.print("LAT:");
    txtBox[1].x = 26;
    txtBox[1].y = 28;
    txtBox[1].length = 8;
    txtBox[1].type = 1;
    sprintf(txtBox[1].text, "%.5f", config.igate_lat);

    display.setCursor(0, 42);
    display.print("LON:");
    txtBox[2].x = 26;
    txtBox[2].y = 40;
    txtBox[2].length = 9;
    txtBox[2].type = 1;
    sprintf(txtBox[2].text, "%.5f", config.igate_lon);

    chkGPS.Checked = config.igate_gps;
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
    symBox.table = config.igate_symbol[0];
    symBox.symbol = config.igate_symbol[1];
    symBox.SetIndex(config.igate_symbol[1]);

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
        }
        if (digitalRead(keyPush) == LOW)
        {
            currentTime = millis();
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
                    config.igate_symbol[0] = symBox.table;
                    config.igate_symbol[1] = symBox.symbol;
                    encoder0Pos = keyPrev;
                }
                else if (i == 4)
                {
                    chkGPS.Toggle();
                    config.igate_gps = chkGPS.Checked;
                    encoder0Pos = keyPrev;
                    chkGPS.CheckBoxShow();
                }
                else if (i == 3)
                {
                    cbBox.SelectValue(0, 15, 1);
                    config.aprs_ssid = cbBox.GetValue();
                    encoder0Pos = keyPrev;
                    cbBox.Show();
                }
                else
                {
                    txtBox[i].TextBox();
                    switch (i)
                    {
                    case 0:
                        strncpy(config.aprs_mycall, txtBox[i].text, 7);
                        config.aprs_mycall[7] = 0;
                        break;
                    // case 1: config.myssid = atol(txtBox[i].text);
                    //	break;
                    case 1:
                        config.igate_lat = atof(txtBox[i].text); // strcpy(config.mylat, txtBox[i].text);
                        break;
                    case 2:
                        config.igate_lon = atof(txtBox[i].text); // strcpy(config.mylon, txtBox[i].text);
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

void on_igate_function_selected(MenuItem *p_menu_item)
{
    int max_sel = 5;
    MyTextBox txtBox;
    MyCheckBox chkBox[3];
    MyComboBox cbBox;
    String str;
    // char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=IGATE FUNCTION=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = config.rf2inet;
    chkBox[0].x = 0;
    chkBox[0].y = 18;
    sprintf(chkBox[0].text, "RF2INET");

    chkBox[1].Checked = config.inet2rf;
    chkBox[1].x = 60;
    chkBox[1].y = 18;
    sprintf(chkBox[1].text, "INET2RF");

    chkBox[2].Checked = false;
    chkBox[2].x = 0;
    chkBox[2].y = 29;
    sprintf(chkBox[2].text, "TIME STAMP");

    // display.setCursor(35, 30);
    // display.print("INTERVAL");
    // cbBox[0].isValue = true;
    // cbBox[0].x = 85;
    // cbBox[0].y = 28;
    // cbBox[0].length = 4;
    // cbBox[0].char_max = 1800;
    // cbBox[0].SetIndex(0);

    display.setCursor(0, 40);
    display.print("PTH:");
    cbBox.isValue = false;
    cbBox.x = 20;
    cbBox.y = 38;
    cbBox.length = 13;
    cbBox.maxItem(PATH_LEN);
    for (i = 0; i < PATH_LEN; i++)
    {
        cbBox.AddItem(i, PATH_NAME[i]);
        //if (!strcmp(&config.igate_path[0], &config.path[i][0]))
        //    sel = i;
    }
    cbBox.SetIndex(config.igate_path);

    display.setCursor(0, 54);
    display.print("TXT:");
    txtBox.x = 20;
    txtBox.y = 52;
    txtBox.length = 14;
    txtBox.type = ALL;
    strcpy(txtBox.text, config.igate_comment);

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

            for (i = 0; i < 3; i++)
                chkBox[i].isSelect = false;
            cbBox.isSelect = false;
            txtBox.isSelect = false;

            if (encoder0Pos < 3)
                chkBox[encoder0Pos].isSelect = true;
            if (encoder0Pos == 3)
                cbBox.isSelect = true;
            if (encoder0Pos == 4)
                txtBox.isSelect = true;
            for (i = 0; i < 3; i++)
                chkBox[i].CheckBoxShow();
            cbBox.Show();
            txtBox.TextBoxShow();
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
                ;
                if ((millis() - currentTime) > 2000)
                    break; // OK Timeout
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (i == 4)
                {
                    txtBox.TextBox();
                    strcpy(config.igate_comment, txtBox.text);
                    encoder0Pos = keyPrev;
                }
                else if (i == 3)
                {
                    cbBox.SelectItem();
                    int n = cbBox.GetIndex();
                    if (n < PATH_LEN)
                    {
                        config.igate_path=n;
                    }
                    encoder0Pos = keyPrev;
                    cbBox.Show();
                }
                else
                {
                    chkBox[i].Toggle();
                    switch (i)
                    {
                    case 0:
                        config.rf2inet = chkBox[i].Checked;
                        break;
                    case 1:
                        config.inet2rf = chkBox[i].Checked;
                        break;
                    case 2:
                        config.igate_timestamp = chkBox[i].Checked;
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

void on_igate_beacon_selected(MenuItem *p_menu_item)
{
    int max_sel = 6;
    MyTextBox txtBox[2];
    MyCheckBox chkBox[3];
    MyComboBox cbBox;
    String str;
    // char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=IGATE BEACON=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = config.igate_bcn;
    chkBox[0].x = 0;
    chkBox[0].y = 18;
    sprintf(chkBox[0].text, "BCN");

    chkBox[1].Checked = config.igate_loc2rf;
    chkBox[1].x = 0;
    chkBox[1].y = 30;
    sprintf(chkBox[1].text, "POS2RF");

    chkBox[2].Checked = config.igate_loc2inet;
    chkBox[2].x = 60;
    chkBox[2].y = 30;
    sprintf(chkBox[2].text, "POS2INET");

    display.setCursor(35, 20);
    display.print("INTERVAL");
    cbBox.isValue = true;
    cbBox.x = 85;
    cbBox.y = 18;
    cbBox.length = 4;
    cbBox.char_max = 1800;
    cbBox.SetIndex(config.igate_interval);

    display.setCursor(0, 42);
    display.print("OBJECT");
    txtBox[0].x = 40;
    txtBox[0].y = 40;
    txtBox[0].length = 11;
    txtBox[0].type = 0;
    strcpy(txtBox[0].text, config.igate_object);

    display.setCursor(0, 54);
    display.print("PHG");
    txtBox[1].x = 20;
    txtBox[1].y = 52;
    txtBox[1].length = 8;
    txtBox[1].type = 0;
    strcpy(txtBox[1].text, config.igate_phg);

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
            for (i = 0; i < 3; i++)
                chkBox[i].isSelect = false;
            cbBox.isSelect = false;
            txtBox[0].isSelect = false;
            txtBox[1].isSelect = false;

            if (encoder0Pos == 0)
                chkBox[0].isSelect = true;
            if (encoder0Pos == 2)
                chkBox[1].isSelect = true;
            if (encoder0Pos == 3)
                chkBox[2].isSelect = true;
            if (encoder0Pos == 1)
                cbBox.isSelect = true;
            if (encoder0Pos == 4)
                txtBox[0].isSelect = true;
            if (encoder0Pos == 5)
                txtBox[1].isSelect = true;
            for (i = 0; i < 3; i++)
                chkBox[i].CheckBoxShow();
            cbBox.Show();
            txtBox[0].TextBoxShow();
            txtBox[1].TextBoxShow();
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
                ;
                if ((millis() - currentTime) > 2000)
                    break; // OK Timeout
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (i == 5)
                {
                    txtBox[1].TextBox();
                    strcpy(config.igate_phg, txtBox[1].text);
                    encoder0Pos = keyPrev;
                }
                else if (i == 4)
                {
                    txtBox[0].TextBox();
                    strcpy(config.igate_object, txtBox[0].text);
                    encoder0Pos = keyPrev;
                }
                else if (i == 1)
                {
                    cbBox.SelectValue(0, 1800, 60);
                    config.igate_interval = cbBox.GetValue();
                    encoder0Pos = keyPrev;
                    cbBox.Show();
                }
                else
                {

                    switch (i)
                    {
                    case 0:
                        chkBox[0].Toggle();
                        config.igate_bcn = chkBox[0].Checked;
                        chkBox[0].CheckBoxShow();
                        break;
                    case 2:
                        chkBox[1].Toggle();
                        config.igate_loc2rf = chkBox[1].Checked;
                        chkBox[1].CheckBoxShow();
                        break;
                    case 3:
                        chkBox[2].Toggle();
                        config.igate_loc2inet = chkBox[2].Checked;
                        chkBox[2].CheckBoxShow();
                        break;
                    }
                    encoder0Pos = keyPrev;
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

