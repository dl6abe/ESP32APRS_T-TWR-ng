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
void on_wifi_AP_selected(MenuItem *p_menu_item)
{
    MyTextBox txtBox[2];
    MyCheckBox chkBoxWiFi;
    MyComboBox cbBox;
    String str;
    // char ch[10];
    int x, i;
    int max_sel = 4;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("WIFI AP CFG");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    display.setCursor(0, 18);
    display.print("SSID:");
    txtBox[0].x = 0;
    txtBox[0].y = 27;
    txtBox[0].length = 17;
    txtBox[0].type = 0;
    strcpy(txtBox[0].text, config.wifi_ap_ssid);

    if (config.wifi_mode & WIFI_AP_FIX)
    {
        chkBoxWiFi.Checked = true;
    }
    else
    {
        chkBoxWiFi.Checked = false;
    }

    chkBoxWiFi.x = 82;
    chkBoxWiFi.y = 18;
    sprintf(chkBoxWiFi.text, "Enable");

    display.setCursor(0, 44);
    display.print("PASS:");
    txtBox[1].x = 0;
    txtBox[1].y = 53;
    txtBox[1].length = 14;
    txtBox[1].type = 0;
    strcpy(txtBox[1].text, config.wifi_ap_pass);

    display.setCursor(55, 42);
    display.print("PWR:");
    display.setCursor(110, 42);
    display.print("dBm");
    cbBox.isValue = true;
    cbBox.x = 80;
    cbBox.y = 40;
    cbBox.length = 2;
    cbBox.maxItem(20);
    cbBox.char_max = 20;
    cbBox.SetIndex(config.wifi_power);

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
                txtBox[i].isSelect = false;
            chkBoxWiFi.isSelect = false;
            cbBox.isSelect = false;
            if (encoder0Pos < 2)
                txtBox[encoder0Pos].isSelect = true;
            if (encoder0Pos == 2)
                chkBoxWiFi.isSelect = true;
            if (encoder0Pos == 3)
                cbBox.isSelect = true;
            for (i = 0; i < 2; i++)
                txtBox[i].TextBoxShow();
            chkBoxWiFi.CheckBoxShow();
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
                    // msgBox("KEY Back");
                    break; // OK Timeout
                }
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (encoder0Pos < 2)
                {
                    txtBox[i].TextBox();
                    switch (i)
                    {
                    case 0:
                        if (isValidWifiSSID(txtBox[0].text, strlen(txtBox[0].text)))
                            strcpy(config.wifi_ap_ssid, txtBox[0].text);
                        break;
                    case 1:
                        if (isValidWifiPassword(txtBox[1].text, strlen(txtBox[1].text)))
                            strcpy(config.wifi_ap_pass, txtBox[1].text);
                        break;
                    }
                    encoder0Pos = keyPrev + 1;
                }
                else if (encoder0Pos == 2)
                {
                    chkBoxWiFi.Toggle();
                    if (chkBoxWiFi.Checked)
                    {
                        config.wifi_mode |= WIFI_AP_FIX;
                    }
                    else
                    {
                        config.wifi_mode &= ~WIFI_AP_FIX;
                    }
                    encoder0Pos = keyPrev;
                    chkBoxWiFi.CheckBoxShow();
                }
                else if (encoder0Pos == 3)
                {
                    cbBox.SelectValue(0, 20, 1);
                    config.wifi_power = (unsigned char)cbBox.GetValue();
                    encoder0Pos = keyPrev;
                    cbBox.Show();
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
    // if (config.wifi_enable)
    // {
    //     WiFi.disconnect(false);
    //     conStatNetwork = CON_WIFI;
    // }
    // else
    // {
    //     WiFi.disconnect(true);
    //     conStatNetwork = CON_WIFI;
    // }
}

void on_wifi_Client_selected(MenuItem *p_menu_item)
{
    MyTextBox txtBox[2];
    MyCheckBox chkBoxWiFi;
    MyComboBox cbBox;
    String str;
    // char ch[10];
    int x, i;
    int max_sel = 4;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("WIFI STATION");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    display.setCursor(0, 18);
    display.print("SSID:");
    txtBox[0].x = 0;
    txtBox[0].y = 27;
    txtBox[0].length = 17;
    txtBox[0].type = 0;
    strcpy(txtBox[0].text, config.wifi_sta[0].wifi_ssid);

    if (config.wifi_mode & WIFI_STA_FIX)
    {
        chkBoxWiFi.Checked = true;
    }
    else
    {
        chkBoxWiFi.Checked = false;
    }

    chkBoxWiFi.x = 82;
    chkBoxWiFi.y = 18;
    sprintf(chkBoxWiFi.text, "Enable");

    display.setCursor(0, 44);
    display.print("PASS:");
    txtBox[1].x = 0;
    txtBox[1].y = 53;
    txtBox[1].length = 14;
    txtBox[1].type = 0;
    strcpy(txtBox[1].text, config.wifi_sta[0].wifi_pass);

    display.setCursor(55, 42);
    display.print("PWR:");
    display.setCursor(110, 42);
    display.print("dBm");
    cbBox.isValue = true;
    cbBox.x = 80;
    cbBox.y = 40;
    cbBox.length = 2;
    cbBox.maxItem(20);
    cbBox.char_max = 20;
    cbBox.SetIndex(config.wifi_power);

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
                txtBox[i].isSelect = false;
            chkBoxWiFi.isSelect = false;
            cbBox.isSelect = false;
            if (encoder0Pos < 2)
                txtBox[encoder0Pos].isSelect = true;
            if (encoder0Pos == 2)
                chkBoxWiFi.isSelect = true;
            if (encoder0Pos == 3)
                cbBox.isSelect = true;
            for (i = 0; i < 2; i++)
                txtBox[i].TextBoxShow();
            chkBoxWiFi.CheckBoxShow();
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
                    // msgBox("KEY Back");
                    break; // OK Timeout
                }
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (encoder0Pos < 2)
                {
                    txtBox[i].TextBox();
                    switch (i)
                    {
                    case 0:
                        if (isValidWifiSSID(txtBox[0].text, strlen(txtBox[0].text)))
                            strcpy(config.wifi_sta[0].wifi_ssid, txtBox[0].text);
                        break;
                    case 1:
                        if (isValidWifiPassword(txtBox[1].text, strlen(txtBox[1].text)))
                            strcpy(config.wifi_sta[0].wifi_pass, txtBox[1].text);
                        break;
                    }
                    encoder0Pos = keyPrev + 1;
                }
                else if (encoder0Pos == 2)
                {
                    chkBoxWiFi.Toggle();
                    if (chkBoxWiFi.Checked)
                    {
                        config.wifi_mode |= WIFI_STA_FIX;
                    }
                    else
                    {
                        config.wifi_mode &= ~WIFI_STA_FIX;
                    }
                    encoder0Pos = keyPrev;
                    chkBoxWiFi.CheckBoxShow();
                }
                else if (encoder0Pos == 3)
                {
                    cbBox.SelectValue(0, 20, 1);
                    config.wifi_power = (unsigned char)cbBox.GetValue();
                    encoder0Pos = keyPrev;
                    cbBox.Show();
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
    // if (config.wifi_enable)
    // {
    //     WiFi.disconnect(false);
    //     conStatNetwork = CON_WIFI;
    // }
    // else
    // {
    //     WiFi.disconnect(true);
    //     conStatNetwork = CON_WIFI;
    // }
}

void on_bluetooth_selected(MenuItem *p_menu_item)
{
    MyTextBox txtBox[2];
    MyCheckBox chkBoxWiFi;
    MyComboBox cbBox;
    String str;
    // char ch[10];
    int x, i;
    int max_sel = 4;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=BLUETOOTH=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    display.setCursor(0, 18);
    display.print("NAME:");
    txtBox[0].x = 0;
    txtBox[0].y = 27;
    txtBox[0].length = 17;
    txtBox[0].type = 0;
    strcpy(txtBox[0].text, config.bt_name);

    chkBoxWiFi.Checked = config.bt_master;

    chkBoxWiFi.x = 82;
    chkBoxWiFi.y = 18;
    sprintf(chkBoxWiFi.text, "Enable");

    display.setCursor(0, 44);
    display.print("PIN:");
    txtBox[1].x = 0;
    txtBox[1].y = 53;
    txtBox[1].length = 6;
    txtBox[1].type = 0;
    strcpy(txtBox[1].text, String(config.bt_pin).c_str());

    display.setCursor(55, 42);
    display.print("PWR:");
    display.setCursor(110, 42);
    display.print("dBm");
    cbBox.isValue = true;
    cbBox.x = 80;
    cbBox.y = 40;
    cbBox.length = 2;
    cbBox.maxItem(20);
    cbBox.char_max = 20;
    cbBox.SetIndex(config.bt_power);

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
                txtBox[i].isSelect = false;
            chkBoxWiFi.isSelect = false;
            cbBox.isSelect = false;
            if (encoder0Pos < 2)
                txtBox[encoder0Pos].isSelect = true;
            if (encoder0Pos == 2)
                chkBoxWiFi.isSelect = true;
            if (encoder0Pos == 3)
                cbBox.isSelect = true;
            for (i = 0; i < 2; i++)
                txtBox[i].TextBoxShow();
            chkBoxWiFi.CheckBoxShow();
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
                    // msgBox("KEY Back");
                    break; // OK Timeout
                }
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (encoder0Pos < 2)
                {
                    txtBox[i].TextBox();
                    switch (i)
                    {
                    case 0:
                        strcpy(config.bt_name, txtBox[0].text);
                        break;
                    case 1:
                        config.bt_pin = String(txtBox[1].text).toInt();
                        break;
                    }
                    encoder0Pos = keyPrev + 1;
                }
                else if (encoder0Pos == 2)
                {
                    chkBoxWiFi.Toggle();
                    config.bt_master = chkBoxWiFi.Checked;
                    encoder0Pos = keyPrev;
                    chkBoxWiFi.CheckBoxShow();
                }
                else if (encoder0Pos == 3)
                {
                    cbBox.SelectValue(0, 20, 1);
                    config.bt_power = (unsigned char)cbBox.GetValue();
                    encoder0Pos = keyPrev;
                    cbBox.Show();
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
}




void on_rfconfig_selected(MenuItem *p_menu_item)
{
    MyTextBox txtBox[2];
    MyCheckBox chkBoxRF;
    MyComboBox cbBox[2];
    String str;
    // char ch[10];
    int x, i;
    int max_sel = 5;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("=RF MODULE=");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBoxRF.Checked = config.rf_en;
    chkBoxRF.x = 0;
    chkBoxRF.y = 18;
    sprintf(chkBoxRF.text, "RF_ENABLE");

    display.setCursor(0, 30);
    display.print("FREQ_TX:");
    display.setCursor(110, 30);
    display.print("MHz");
    txtBox[0].x = 47;
    txtBox[0].y = 28;
    txtBox[0].length = 8;
    txtBox[0].type = 1;
    sprintf(txtBox[0].text, "%.4f", config.freq_tx);
    display.setCursor(0, 40);
    display.print("FREQ_RX:");
    display.setCursor(110, 40);
    display.print("MHz");
    txtBox[1].x = 47;
    txtBox[1].y = 38;
    txtBox[1].length = 8;
    txtBox[1].type = 1;
    sprintf(txtBox[1].text, "%.4f", config.freq_rx);

    display.setCursor(0, 54);
    display.print("SEQ:");
    cbBox[0].isValue = true;
    cbBox[0].x = 25;
    cbBox[0].y = 52;
    cbBox[0].length = 1;
    cbBox[0].maxItem(8);
    cbBox[0].char_max = 8;
    cbBox[0].SetIndex(config.sql_level);

    display.setCursor(60, 54);
    display.print("PWR:");
    cbBox[1].isValue = false;
    cbBox[1].x = 80;
    cbBox[1].y = 52;
    cbBox[1].length = 3;
    cbBox[1].AddItem(0, "LOW");
    cbBox[1].AddItem(1, "HI");
    cbBox[1].maxItem(2);
    cbBox[1].SetIndex(config.rf_power);

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
            chkBoxRF.isSelect = false;
            txtBox[0].isSelect = false;
            txtBox[1].isSelect = false;
            if (encoder0Pos < 2)
                cbBox[encoder0Pos].isSelect = true;
            if (encoder0Pos == 2)
                chkBoxRF.isSelect = true;
            if (encoder0Pos == 3)
                txtBox[0].isSelect = true;
            if (encoder0Pos == 4)
                txtBox[1].isSelect = true;
            for (i = 0; i < 2; i++)
                cbBox[i].Show();
            chkBoxRF.CheckBoxShow();
            txtBox[0].TextBoxShow();
            txtBox[1].TextBoxShow();
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
                delay(10);
                if ((millis() - currentTime) > 2000)
                {
                    // msgBox("KEY Back");
                    break; // OK Timeout
                }
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (encoder0Pos == 0)
                {
                    cbBox[0].SelectValue(0, 8, 1);
                    config.sql_level = cbBox[0].GetValue();
                    encoder0Pos = keyPrev + 1;
                    cbBox[1].Show();
                }
                else if (encoder0Pos == 1)
                {
                    // cbBox[1].SelectValue(0, 1, 1);
                    cbBox[1].SelectItem();
                    config.rf_power = cbBox[1].GetIndex();
                    encoder0Pos = keyPrev + 1;
                    cbBox[1].Show();
                }
                else if (encoder0Pos == 2) // Focus Check Box Enable
                {
                    chkBoxRF.Toggle();
                    config.rf_en = chkBoxRF.Checked;
                    encoder0Pos = keyPrev;
                    chkBoxRF.CheckBoxShow();
                }
                else if (encoder0Pos == 3)
                {
                    txtBox[0].TextBox();
                    // strcpy(config.freq_tx, txtBox.text);
                    config.freq_tx = atof(txtBox[0].text);
                    encoder0Pos = keyPrev;
                    txtBox[0].TextBoxShow();
                }
                else if (encoder0Pos == 4)
                {
                    txtBox[1].TextBox();
                    // strcpy(config.freq_tx, txtBox.text);
                    sa868.setRxFrequency((uint32_t)atoi(txtBox[1].text));
                    encoder0Pos = keyPrev;
                    txtBox[1].TextBoxShow();
                }
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

