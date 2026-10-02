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
void on_filter_selected(MenuComponent *p_menu_item)
{
    int max_sel = 11;
    // MyTextBox txtBox[2];
    MyCheckBox chkBox[11];
    MyComboBox cbBox[2];
    String str;
    // char ch[10];
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("Display CFG");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = config.dispRF;
    chkBox[0].x = 0;
    chkBox[0].y = 16;
    sprintf(chkBox[0].text, "RF");

    chkBox[1].Checked = config.dispINET;
    chkBox[1].x = 35;
    chkBox[1].y = 16;
    sprintf(chkBox[1].text, "INET");

    chkBox[2].Checked = (config.dispFilter & FILTER_STATUS) ? 1 : 0;
    chkBox[2].x = 75;
    chkBox[2].y = 16;
    sprintf(chkBox[2].text, "STATUS");

    chkBox[3].Checked = (config.dispFilter & FILTER_WX) ? 1 : 0;
    chkBox[3].x = 0;
    chkBox[3].y = 25;
    sprintf(chkBox[3].text, "WX");

    chkBox[4].Checked = (config.dispFilter & FILTER_TELEMETRY) ? 1 : 0;
    chkBox[4].x = 35;
    chkBox[4].y = 25;
    sprintf(chkBox[4].text, "TLM");

    chkBox[5].Checked = (config.dispFilter & FILTER_ITEM) ? 1 : 0;
    chkBox[5].x = 75;
    chkBox[5].y = 25;
    sprintf(chkBox[5].text, "ITEM");

    chkBox[6].Checked = (config.dispFilter & FILTER_MESSAGE) ? 1 : 0;
    chkBox[6].x = 0;
    chkBox[6].y = 34;
    sprintf(chkBox[6].text, "MSG");

    chkBox[7].Checked = (config.dispFilter & FILTER_POSITION) ? 1 : 0;
    chkBox[7].x = 35;
    chkBox[7].y = 34;
    sprintf(chkBox[7].text, "POS");

    chkBox[8].Checked = (config.dispFilter & FILTER_BUOY) ? 1 : 0;
    chkBox[8].x = 75;
    chkBox[8].y = 34;
    sprintf(chkBox[8].text, "BUOY");

    chkBox[9].Checked = config.h_up;
    chkBox[9].x = 0;
    chkBox[9].y = 43;
    sprintf(chkBox[9].text, "H-UP");

    chkBox[10].Checked = config.tx_display;
    chkBox[10].x = 35;
    chkBox[10].y = 43;
    sprintf(chkBox[10].text, "TXS");

    display.setCursor(0, 54);
    display.print("DLY:");
    // display.setCursor(55, 54);
    // display.print("S");
    cbBox[0].isValue = true;
    cbBox[0].x = 25;
    cbBox[0].y = 52;
    cbBox[0].length = 3;
    cbBox[0].char_max = 999;
    cbBox[0].SetIndex(config.dispDelay);

    display.setCursor(64, 54);
    display.print("DIST<");
    cbBox[1].isValue = true;
    cbBox[1].x = 64 + 30;
    cbBox[1].y = 52;
    cbBox[1].length = 3;
    cbBox[1].char_max = 999;
    cbBox[1].SetIndex(config.filterDistant);

    display.display();
    encoder0Pos = 0;
    delay(100);
    unsigned long lastActivity = millis();
    do
    {
        if (encoder0Pos >= 13)
            encoder0Pos = 0;
        if (encoder0Pos < 0)
            encoder0Pos = 12;
        if (keyPrev != encoder0Pos)
        {
            keyPrev = encoder0Pos;
            lastActivity = millis();
            for (i = 0; i < max_sel; i++)
            {
                chkBox[i].isSelect = false;
            }
            for (i = 0; i < 2; i++)
                cbBox[i].isSelect = false;
            for (i = 0; i < max_sel; i++)
                chkBox[i].isSelect = false;
            if (encoder0Pos < 11)
                chkBox[encoder0Pos].isSelect = true;
            if (encoder0Pos > 10 && encoder0Pos < 13)
                cbBox[encoder0Pos - 11].isSelect = true;
            for (i = 0; i < max_sel; i++)
                chkBox[i].CheckBoxShow();
            for (i = 0; i < 2; i++)
            {
                cbBox[i].Show();
            }
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
                if (i < 11)
                {
                    chkBox[i].Toggle();
                    switch (i)
                    {
                    case 0:
                        config.dispRF = chkBox[i].Checked;
                        break;
                    case 1:
                        config.dispINET = chkBox[i].Checked;
                        break;
                    // Was dead code (commented-out, referencing fields that
                    // don't exist - dispFilter is a bitmask, not individual
                    // bools) - the checkbox visually toggled but nothing
                    // was ever saved. Fixed to match dispFilter's real
                    // representation, same pattern as the sibling screen
                    // on_filter_display_selected() already uses correctly.
                    case 2:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_STATUS;
                        else
                            config.dispFilter &= ~FILTER_STATUS;
                        break;
                    case 3:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_WX;
                        else
                            config.dispFilter &= ~FILTER_WX;
                        break;
                    case 4:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_TELEMETRY;
                        else
                            config.dispFilter &= ~FILTER_TELEMETRY;
                        break;
                    case 5:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_ITEM;
                        else
                            config.dispFilter &= ~FILTER_ITEM;
                        break;
                    case 6:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_MESSAGE;
                        else
                            config.dispFilter &= ~FILTER_MESSAGE;
                        break;
                    case 7:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_POSITION;
                        else
                            config.dispFilter &= ~FILTER_POSITION;
                        break;
                    case 8:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_BUOY;
                        else
                            config.dispFilter &= ~FILTER_BUOY;
                        break;
                    case 9:
                        config.h_up = chkBox[i].Checked;
                        break;
                    case 10:
                        config.tx_display = chkBox[i].Checked;
                        break;
                    }
                    encoder0Pos = keyPrev;
                    chkBox[i].CheckBoxShow();
                }
                else if (i > 10 && i < 13)
                {
                    i -= 11;
                    switch (i)
                    {
                    case 0:
                        cbBox[i].SelectValue(1, 600, 1);
                        config.dispDelay = (unsigned int)cbBox[i].GetValue();
                        break;
                    case 1:
                        cbBox[i].SelectValue(0, 999, 1);
                        config.filterDistant = (unsigned int)cbBox[i].GetValue();
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
    /*display.clearDisplay();
    display.setCursor(30, 4);
    display.print("SAVE & EXIT");
    display.display();*/
    msgBox("KEY EXIT");
    while (digitalRead(keyPush) == LOW)
        ;
    saveEEPROM();
}

void on_filter_display_selected(MenuComponent *p_menu_item)
{
    MyCheckBox chkBox[10];
    MyComboBox cbBox;
    String str;
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("Display Filter");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = (config.dispFilter & FILTER_OBJECT) ? 1 : 0;
    chkBox[0].x = 0;
    chkBox[0].y = 16;
    sprintf(chkBox[0].text, "OBJ");

    chkBox[1].Checked = (config.dispFilter & FILTER_QUERY) ? 1 : 0;
    chkBox[1].x = 35;
    chkBox[1].y = 16;
    sprintf(chkBox[1].text, "QRY");

    chkBox[2].Checked = (config.dispFilter & FILTER_STATUS) ? 1 : 0;
    chkBox[2].x = 75;
    chkBox[2].y = 16;
    sprintf(chkBox[2].text, "STATUS");

    chkBox[3].Checked = (config.dispFilter & FILTER_WX) ? 1 : 0;
    chkBox[3].x = 0;
    chkBox[3].y = 25;
    sprintf(chkBox[3].text, "WX");

    chkBox[4].Checked = (config.dispFilter & FILTER_TELEMETRY) ? 1 : 0;
    chkBox[4].x = 35;
    chkBox[4].y = 25;
    sprintf(chkBox[4].text, "TLM");

    chkBox[5].Checked = (config.dispFilter & FILTER_ITEM) ? 1 : 0;
    chkBox[5].x = 75;
    chkBox[5].y = 25;
    sprintf(chkBox[5].text, "ITEM");

    chkBox[6].Checked = (config.dispFilter & FILTER_MESSAGE) ? 1 : 0;
    chkBox[6].x = 0;
    chkBox[6].y = 34;
    sprintf(chkBox[6].text, "MSG");

    chkBox[7].Checked = (config.dispFilter & FILTER_POSITION) ? 1 : 0;
    chkBox[7].x = 35;
    chkBox[7].y = 34;
    sprintf(chkBox[7].text, "POS");

    chkBox[8].Checked = (config.dispFilter & FILTER_BUOY) ? 1 : 0;
    chkBox[8].x = 75;
    chkBox[8].y = 34;
    sprintf(chkBox[8].text, "BUOY");

    chkBox[9].Checked = (config.dispFilter & FILTER_MICE) ? 1 : 0;
    chkBox[9].x = 0;
    chkBox[9].y = 43;
    sprintf(chkBox[9].text, "MICE");

    display.setCursor(0, 54);
    display.print("DX <");
    display.setCursor(75, 54);
    display.print("km.");
    cbBox.isValue = true;
    cbBox.x = 33;
    cbBox.y = 52;
    cbBox.length = 3;
    cbBox.char_max = 999;
    cbBox.SetIndex(config.filterDistant);

    display.display();
    encoder0Pos = 0;
    delay(100);
    unsigned long lastActivity = millis();
    do
    {
        if (encoder0Pos >= 13)
            encoder0Pos = 0;
        if (encoder0Pos < 0)
            encoder0Pos = 12;
        if (keyPrev != encoder0Pos)
        {
            keyPrev = encoder0Pos;
            lastActivity = millis();
            for (i = 0; i < 10; i++)
            {
                chkBox[i].isSelect = false;
            }
            cbBox.isSelect = false;
            for (i = 0; i < 10; i++)
                chkBox[i].isSelect = false;
            if (encoder0Pos < 10)
                chkBox[encoder0Pos].isSelect = true;
            if (encoder0Pos > 10 && encoder0Pos < 13)
                cbBox.isSelect = true;
            for (i = 0; i < 10; i++)
                chkBox[i].CheckBoxShow();
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
                    break; // OK Timeout
            };
            if ((millis() - currentTime) < 1500)
            {
                i = encoder0Pos;
                if (i < 10)
                {
                    chkBox[i].Toggle();
                    switch (i)
                    {
                    case 0:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_OBJECT;
                        else
                            config.dispFilter &= ~FILTER_OBJECT;
                        break;
                    case 1:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_QUERY;
                        else
                            config.dispFilter &= ~FILTER_QUERY;
                        break;
                    case 2:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_STATUS;
                        else
                            config.dispFilter &= ~FILTER_STATUS;
                        break;
                    case 3:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_WX;
                        else
                            config.dispFilter &= ~FILTER_WX;
                        break;
                    case 4:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_TELEMETRY;
                        else
                            config.dispFilter &= ~FILTER_TELEMETRY;
                        break;
                    case 5:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_ITEM;
                        else
                            config.dispFilter &= ~FILTER_ITEM;
                        break;
                    case 6:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_MESSAGE;
                        else
                            config.dispFilter &= ~FILTER_MESSAGE;
                        break;
                    case 7:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_BUOY;
                        else
                            config.dispFilter &= ~FILTER_BUOY;
                        break;
                    case 8:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_POSITION;
                        else
                            config.dispFilter &= ~FILTER_POSITION;
                        break;
                    case 9:
                        if (chkBox[i].Checked)
                            config.dispFilter |= FILTER_MICE;
                        else
                            config.dispFilter &= ~FILTER_MICE;
                        break;
                    }
                    encoder0Pos = keyPrev;
                    chkBox[i].CheckBoxShow();
                }
                else if (i > 9)
                {
                    cbBox.SelectValue(0, 999, 1);
                    config.filterDistant = (unsigned int)cbBox.GetValue();
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
    saveEEPROM();
}


void on_filter_inet2rf_selected(MenuComponent *p_menu_item)
{
    int max_sel = 10;
    MyCheckBox chkBox[10];
    String str;
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("INET2RF Filter");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = (config.inet2rfFilter & FILTER_OBJECT) ? 1 : 0;
    chkBox[0].x = 0;
    chkBox[0].y = 16;
    sprintf(chkBox[0].text, "OBJ");

    chkBox[1].Checked = (config.inet2rfFilter & FILTER_QUERY) ? 1 : 0;
    chkBox[1].x = 35;
    chkBox[1].y = 16;
    sprintf(chkBox[1].text, "QRY");

    chkBox[2].Checked = (config.inet2rfFilter & FILTER_STATUS) ? 1 : 0;
    chkBox[2].x = 75;
    chkBox[2].y = 16;
    sprintf(chkBox[2].text, "STATUS");

    chkBox[3].Checked = (config.inet2rfFilter & FILTER_WX) ? 1 : 0;
    chkBox[3].x = 0;
    chkBox[3].y = 25;
    sprintf(chkBox[3].text, "WX");

    chkBox[4].Checked = (config.inet2rfFilter & FILTER_TELEMETRY) ? 1 : 0;
    chkBox[4].x = 35;
    chkBox[4].y = 25;
    sprintf(chkBox[4].text, "TLM");

    chkBox[5].Checked = (config.inet2rfFilter & FILTER_ITEM) ? 1 : 0;
    chkBox[5].x = 75;
    chkBox[5].y = 25;
    sprintf(chkBox[5].text, "ITEM");

    chkBox[6].Checked = (config.inet2rfFilter & FILTER_MESSAGE) ? 1 : 0;
    chkBox[6].x = 0;
    chkBox[6].y = 34;
    sprintf(chkBox[6].text, "MSG");

    chkBox[7].Checked = (config.inet2rfFilter & FILTER_POSITION) ? 1 : 0;
    chkBox[7].x = 35;
    chkBox[7].y = 34;
    sprintf(chkBox[7].text, "POS");

    chkBox[8].Checked = (config.inet2rfFilter & FILTER_BUOY) ? 1 : 0;
    chkBox[8].x = 75;
    chkBox[8].y = 34;
    sprintf(chkBox[8].text, "BUOY");

    chkBox[9].Checked = (config.inet2rfFilter & FILTER_MICE) ? 1 : 0;
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
                            config.inet2rfFilter |= FILTER_OBJECT;
                        else
                            config.inet2rfFilter &= ~FILTER_OBJECT;
                        break;
                    case 1:
                        if (chkBox[i].Checked)
                            config.inet2rfFilter |= FILTER_QUERY;
                        else
                            config.inet2rfFilter &= ~FILTER_QUERY;
                        break;
                    case 2:
                        if (chkBox[i].Checked)
                            config.inet2rfFilter |= FILTER_STATUS;
                        else
                            config.inet2rfFilter &= ~FILTER_STATUS;
                        break;
                    case 3:
                        if (chkBox[i].Checked)
                            config.inet2rfFilter |= FILTER_WX;
                        else
                            config.inet2rfFilter &= ~FILTER_WX;
                        break;
                    case 4:
                        if (chkBox[i].Checked)
                            config.inet2rfFilter |= FILTER_TELEMETRY;
                        else
                            config.inet2rfFilter &= ~FILTER_TELEMETRY;
                        break;
                    case 5:
                        if (chkBox[i].Checked)
                            config.inet2rfFilter |= FILTER_ITEM;
                        else
                            config.inet2rfFilter &= ~FILTER_ITEM;
                        break;
                    case 6:
                        if (chkBox[i].Checked)
                            config.inet2rfFilter |= FILTER_MESSAGE;
                        else
                            config.inet2rfFilter &= ~FILTER_MESSAGE;
                        break;
                    case 7:
                        if (chkBox[i].Checked)
                            config.inet2rfFilter |= FILTER_BUOY;
                        else
                            config.inet2rfFilter &= ~FILTER_BUOY;
                        break;
                    case 8:
                        if (chkBox[i].Checked)
                            config.inet2rfFilter |= FILTER_POSITION;
                        else
                            config.inet2rfFilter &= ~FILTER_POSITION;
                        break;
                    case 9:
                        if (chkBox[i].Checked)
                            config.inet2rfFilter |= FILTER_MICE;
                        else
                            config.inet2rfFilter &= ~FILTER_MICE;
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

void on_filter_rf2inet_selected(MenuComponent *p_menu_item)
{
    int max_sel = 10;
    MyCheckBox chkBox[10];
    String str;
    int x, i;
    int keyPrev = -1;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 16, WHITE);
    display.setTextColor(BLACK);
    str = String("RF2INET Filter");
    x = str.length() * 6;
    display.setCursor(64 - (x / 2), 4);
    display.print(str);
    display.setTextColor(WHITE);

    chkBox[0].Checked = (config.rf2inetFilter & FILTER_OBJECT) ? 1 : 0;
    chkBox[0].x = 0;
    chkBox[0].y = 16;
    sprintf(chkBox[0].text, "OBJ");

    chkBox[1].Checked = (config.rf2inetFilter & FILTER_QUERY) ? 1 : 0;
    chkBox[1].x = 35;
    chkBox[1].y = 16;
    sprintf(chkBox[1].text, "QRY");

    chkBox[2].Checked = (config.rf2inetFilter & FILTER_STATUS) ? 1 : 0;
    chkBox[2].x = 75;
    chkBox[2].y = 16;
    sprintf(chkBox[2].text, "STATUS");

    chkBox[3].Checked = (config.rf2inetFilter & FILTER_WX) ? 1 : 0;
    chkBox[3].x = 0;
    chkBox[3].y = 25;
    sprintf(chkBox[3].text, "WX");

    chkBox[4].Checked = (config.rf2inetFilter & FILTER_TELEMETRY) ? 1 : 0;
    chkBox[4].x = 35;
    chkBox[4].y = 25;
    sprintf(chkBox[4].text, "TLM");

    chkBox[5].Checked = (config.rf2inetFilter & FILTER_ITEM) ? 1 : 0;
    chkBox[5].x = 75;
    chkBox[5].y = 25;
    sprintf(chkBox[5].text, "ITEM");

    chkBox[6].Checked = (config.rf2inetFilter & FILTER_MESSAGE) ? 1 : 0;
    chkBox[6].x = 0;
    chkBox[6].y = 34;
    sprintf(chkBox[6].text, "MSG");

    chkBox[7].Checked = (config.rf2inetFilter & FILTER_POSITION) ? 1 : 0;
    chkBox[7].x = 35;
    chkBox[7].y = 34;
    sprintf(chkBox[7].text, "POS");

    chkBox[8].Checked = (config.rf2inetFilter & FILTER_BUOY) ? 1 : 0;
    chkBox[8].x = 75;
    chkBox[8].y = 34;
    sprintf(chkBox[8].text, "BUOY");

    chkBox[9].Checked = (config.rf2inetFilter & FILTER_MICE) ? 1 : 0;
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
                            config.rf2inetFilter |= FILTER_OBJECT;
                        else
                            config.rf2inetFilter &= ~FILTER_OBJECT;
                        break;
                    case 1:
                        if (chkBox[i].Checked)
                            config.rf2inetFilter |= FILTER_QUERY;
                        else
                            config.rf2inetFilter &= ~FILTER_QUERY;
                        break;
                    case 2:
                        if (chkBox[i].Checked)
                            config.rf2inetFilter |= FILTER_STATUS;
                        else
                            config.rf2inetFilter &= ~FILTER_STATUS;
                        break;
                    case 3:
                        if (chkBox[i].Checked)
                            config.rf2inetFilter |= FILTER_WX;
                        else
                            config.rf2inetFilter &= ~FILTER_WX;
                        break;
                    case 4:
                        if (chkBox[i].Checked)
                            config.rf2inetFilter |= FILTER_TELEMETRY;
                        else
                            config.rf2inetFilter &= ~FILTER_TELEMETRY;
                        break;
                    case 5:
                        if (chkBox[i].Checked)
                            config.rf2inetFilter |= FILTER_ITEM;
                        else
                            config.rf2inetFilter &= ~FILTER_ITEM;
                        break;
                    case 6:
                        if (chkBox[i].Checked)
                            config.rf2inetFilter |= FILTER_MESSAGE;
                        else
                            config.rf2inetFilter &= ~FILTER_MESSAGE;
                        break;
                    case 7:
                        if (chkBox[i].Checked)
                            config.rf2inetFilter |= FILTER_BUOY;
                        else
                            config.rf2inetFilter &= ~FILTER_BUOY;
                        break;
                    case 8:
                        if (chkBox[i].Checked)
                            config.rf2inetFilter |= FILTER_POSITION;
                        else
                            config.rf2inetFilter &= ~FILTER_POSITION;
                        break;
                    case 9:
                        if (chkBox[i].Checked)
                            config.rf2inetFilter |= FILTER_MICE;
                        else
                            config.rf2inetFilter &= ~FILTER_MICE;
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
// void on_tncconfig_selected(MenuComponent *p_menu_item)
// {
//     int max_sel = 6;
//     MyTextBox txtBox[5];
//     // MySymbolBox symBox;
//     // MyCheckBox chkGPS;
//     MyComboBox cbBox;
//     MyCheckBox chkEn;
//     String str;
//     char ch[10];
//     int x, i;
//     int keyPrev = -1;
//     display.clearDisplay();
//     display.fillRect(0, 0, 128, 16, WHITE);
//     display.setTextColor(BLACK);
//     str = String("nTNC CONFIGURATION");
//     x = str.length() * 6;
//     display.setCursor(64 - (x / 2), 4);
//     display.print(str);
//     display.setTextColor(WHITE);

//     display.setCursor(0, 18);
//     display.print("MyCall:");
//     txtBox[0].x = 41;
//     txtBox[0].y = 16;
//     txtBox[0].length = 7;
//     txtBox[0].type = 0;
//     str = String(config.aprs_mycall);
//     str.toUpperCase();
//     str.toCharArray(&ch[0], 10);
//     strcpy(txtBox[0].text, ch);

//     // sprintf(ch, "%d", myssid);
//     display.setCursor(95, 18);
//     display.print("-");
//     cbBox.isValue = true;
//     cbBox.x = 101;
//     cbBox.y = 16;
//     cbBox.length = 2;
//     cbBox.maxItem(15);
//     cbBox.char_max = 15;
//     cbBox.SetIndex(config.aprs_ssid);
//     // txtBox[1].x = 106;
//     // txtBox[1].y = 16;
//     // txtBox[1].length = 2;
//     // txtBox[1].type = 1;
//     // sprintf(txtBox[1].text, "%d", config.aprs_ssid);

//     chkEn.Checked = config.tnc;
//     chkEn.x = 98;
//     chkEn.y = 29;
//     sprintf(chkEn.text, "TNC");

//     display.setCursor(0, 30);
//     display.print("ITEM:");
//     txtBox[2].x = 30;
//     txtBox[2].y = 28;
//     txtBox[2].length = 9;
//     strcpy(txtBox[2].text, config.tnc_item);

//     display.setCursor(0, 42);
//     display.print("PTH:");
//     txtBox[3].x = 25;
//     txtBox[3].y = 40;
//     txtBox[3].length = 14;
//     strcpy(txtBox[3].text, config.tnc_path);

//     display.setCursor(0, 54);
//     display.print("CMN:");
//     txtBox[4].x = 25;
//     txtBox[4].y = 52;
//     txtBox[4].length = 14;
//     strcpy(txtBox[4].text, config.tnc_comment);

//     display.display();
//     encoder0Pos = 0;
//     delay(100);
//     do
//     {
//         if (encoder0Pos >= max_sel)
//             encoder0Pos = 0;
//         if (encoder0Pos < 0)
//             encoder0Pos = max_sel - 1;
//         if (keyPrev != encoder0Pos)
//         {
//             keyPrev = encoder0Pos;
//             for (i = 0; i < max_sel; i++)
//             {
//                 if (i == 1)
//                     cbBox.isSelect = false;
//                 else if (i == 5)
//                     chkEn.isSelect = false;
//                 else
//                     txtBox[i].isSelect = false;
//             }
//             if (encoder0Pos == 1)
//                 cbBox.isSelect = true;
//             else if (encoder0Pos == 5)
//                 chkEn.isSelect = true;
//             else
//                 txtBox[encoder0Pos].isSelect = true;
//             for (i = 0; i < max_sel; i++)
//             {
//                 if (i == 1)
//                     cbBox.Show();
//                 else if (i == 5)
//                     chkEn.CheckBoxShow();
//                 else
//                     txtBox[i].TextBoxShow();
//             }
//         }
//         else
//         {
//             delay(50);
//         }
//         if (digitalRead(keyPush) == LOW)
//         {
//             currentTime = millis();
//             while (digitalRead(keyPush) == LOW)
//             {
//                 if ((millis() - currentTime) > 2000)
//                     break; // OK Timeout
//             };
//             if ((millis() - currentTime) < 1500)
//             {
//                 i = encoder0Pos;
//                 if (i == 1)
//                 {
//                     cbBox.SelectValue(0, 15, 1);
//                     config.aprs_ssid = cbBox.GetValue();
//                     encoder0Pos = keyPrev;
//                 }
//                 else if (i == 5)
//                 {
//                     chkEn.Toggle();
//                     config.tnc = chkEn.Checked;
//                     encoder0Pos = keyPrev;
//                     chkEn.CheckBoxShow();
//                 }
//                 else
//                 {
//                     txtBox[i].TextBox();
//                     switch (i)
//                     {
//                     case 0:
//                         strncpy(config.aprs_mycall, txtBox[i].text, 7);
//                         config.aprs_mycall[7] = 0;
//                         break;
//                     // case 1: config.aprs_ssid = atol(txtBox[i].text);
//                     //	break;
//                     case 2:
//                         strcpy(config.tnc_item, txtBox[i].text);
//                         break;
//                     case 3:
//                         strcpy(config.tnc_path, txtBox[i].text);
//                         break;
//                     case 4:
//                         strcpy(config.tnc_comment, txtBox[i].text);
//                         break;
//                     }
//                     encoder0Pos = keyPrev + 1;
//                 }
//                 while (digitalRead(keyPush) == LOW)
//                     ;
//             }
//             else
//             {
//                 break;
//             }
//         }
//     } while (1);
//     // display.clearDisplay();
//     // display.setCursor(30, 4);
//     // display.print("SAVE & EXIT");
//     // display.display();
//     msgBox("KEY EXIT");
//     while (digitalRead(keyPush) == LOW)
//         ;
// }

