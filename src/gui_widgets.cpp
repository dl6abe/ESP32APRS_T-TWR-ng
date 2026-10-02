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
void MyTextBox::TextBox()
{
    int w = (length * 7) + 4;
    ;
    int char_with = 7;
    int i;
    bool ok = false;
    display.fillRect(x, y, w, 11, BLACK);
    display.drawRect(x, y, w, 11, WHITE);
    curr_cursor = strlen(text);
    if (curr_cursor > 0)
    {
        curr_cursor--;
        encoder0Pos = text[curr_cursor];
    }
    else
    {
        encoder0Pos = 0x30; // 0x20-0x7F
    }

    do
    {
        if (type == 0)
        {
            char_min = 0x20;
            char_max = 0x7F;
        }
        else if (type == 1)
        {
            char_min = 0x2B;
            char_max = 0x39;
        }
        else if (type == 2)
        {
            char_min = 0x41;
            char_max = 0x5A;
        }

        if (encoder0Pos > char_max)
            encoder0Pos = char_min;
        if (encoder0Pos < char_min)
            encoder0Pos = char_max;

        display.fillRect(x + 1, y + 1, w - 2, 9, BLACK);
        for (i = 0; i <= (int)strlen(text); i++)
        {
            if (curr_cursor == i)
            {
                display.fillRect((i * char_with) + x + 1, y, 7, 10, WHITE);
                display.setTextColor(BLACK);
            }
            display.setCursor((i * char_with) + x + 2, y + 2);
            display.print(text[i]);
            display.setTextColor(WHITE);
        }
        text[curr_cursor] = encoder0Pos;
        display.display();
        delay(50);
        if ((digitalRead(keyPush) == LOW))
        {
            currentTime = millis();
            while (digitalRead(keyPush) == LOW)
            {
                if ((millis() - currentTime) > 2000)
                    break; // OK Timeout
            };
            if ((millis() - currentTime) < 1500)
            {
                delay(100);
                currentTime = millis();
                while (digitalRead(keyPush) == HIGH)
                {
                    if ((millis() - currentTime) > 1000)
                        break; // OK Timeout
                };
                if ((millis() - currentTime) < 1000)
                { // Duble Click
                    // text[curr_cursor] = 0;
                    // for (i = curr_cursor; i < sizeof(text); i++) text[i] = 0;
                    if (curr_cursor == 0)
                    {
                        ok = true;
                        memset(text, 0, sizeof(text));
                    }
                    if (curr_cursor > 0)
                        curr_cursor--;
                    encoder0Pos = text[curr_cursor];
                }
                else
                { // One Click
                    if (curr_cursor < (length - 1))
                        curr_cursor++;
                    else
                        curr_cursor = length - 1;
                    // text[curr_cursor] = 0;
                }
                for (i = curr_cursor; i < 50; i++)
                    text[i] = 0;
                while (digitalRead(keyPush) == LOW)
                    ;
            }
            else
            {
                ok = true;
            }
        }
    } while (!ok);
    display.fillRect(x + 1, y + 1, w - 2, 9, BLACK);
    for (i = 0; i <= curr_cursor; i++)
    {
        display.drawChar((i * char_with) + x + 2, y + 2, text[i], BLACK, WHITE, 1);
        /*display.setCursor((i * char_with) + x + 2, y + 2);
        display.print(text[i]);*/
    }
    text[i] = 0;
    display.display();
    // msgBox(F("KEY Back"));
    while (digitalRead(keyPush) == LOW)
        ;
}

void MyTextBox::TextBoxShow()
{
    int w = (length * 7) + 4;
    int char_with = 7;
    int i;
    display.fillRect(x, y, w, 11, BLACK);
    display.drawRect(x, y, w, 11, WHITE);
    display.fillRect(x + 1, y + 1, w - 2, 9, BLACK);
    if (isSelect)
        display.drawRect(x + 1, y + 1, w - 2, 9, WHITE);
    for (i = 0; i <= (int)strlen(text); i++)
    {
        // for (i = 0; i <= curr_cursor; i++) {
        if (i >= length)
            break;
        display.drawChar((i * char_with) + x + 2, y + 2, text[i], WHITE, BLACK, 1);
        /*display.setCursor((i * char_with) + x + 2, y + 2);
        display.print(text[i]);*/
    }
    display.display();
}

void MyCheckBox::Toggle()
{
    if (Checked)
        Checked = false;
    else
        Checked = true;
}

void MyCheckBox::CheckBoxShow()
{
    int w = (strlen(text) * 6) + 10;
    // int margin_x = 1, char_with = 7;
    // int i;
    display.fillRect(x, y, w, 8, BLACK);
    display.drawRect(x, y, 8, 8, WHITE);
    display.fillRect(x + 1, y + 1, 6, 6, BLACK);
    if (Checked)
    {
        display.drawLine(x + 1, y + 1, x + 7, y + 7, WHITE);
        display.drawLine(x, y + 7, x + 7, y + 1, WHITE);
    }
    display.setCursor(x + 10, y);
    display.print(text);
    if (isSelect)
    {
        display.setCursor(x + 11, y + 1);
        display.print(text);
    }
    display.display();
}

void MyButtonBox::Toggle()
{
    if (Checked)
        Checked = false;
    else
        Checked = true;
}

void MyButtonBox::Show()
{
    int w = (strlen(text) * 6) + 4;
    // int w = (length * 7) + 4;
    // int char_with = 7;
    // unsigned int i;
    display.fillRect(x, y, w, 12, BLACK);
    if (isSelect)
    {
        // if (Border) {
        // display.fillRect(x, y, w, 11, WHITE);
        /*display.drawLine(x + 1, y + 12, x + w + 1, y + 12, WHITE);
        display.drawLine(x + w + 1, y + 1, x + w + 1, y + 12, WHITE);*/
        //}
        display.fillRect(x, y, w, 11, WHITE);
        display.setTextColor(BLACK);
        display.setCursor(x + 2, y + 2);
        display.print(text);
    }
    else
    {
        if (Border)
        {
            display.drawRect(x, y, w, 11, WHITE);
            display.drawLine(x + 1, y + 11, x + w, y + 11, WHITE);
            display.drawLine(x + w, y + 1, x + w, y + 11, WHITE);
        }
        display.setTextColor(WHITE);
        display.setCursor(x + 2, y + 2);
        display.print(text);
    }
    display.setTextColor(WHITE);
}

void MyComboBox::SelectValue(long val_min, long val_max, long step)
{
    int w = (length * 7) + 4;
    int keyPrev = encoder0Pos;
    bool ok = false;
    display.fillRect(x, y, w, 11, BLACK);
    display.drawRect(x, y, w, 11, WHITE);

    // current = atol(text);
    if (current > val_max)
        current = val_min;

    display.fillRect(x + 1, y + 1, w - 2, 9, WHITE);
    display.setTextColor(BLACK);

    display.setCursor(x + 2, y + 2);
    display.print(current, DEC);
    display.setTextColor(WHITE);
    display.display();

    do
    {

        if (encoder0Pos != keyPrev)
        {
            if (encoder0Pos < keyPrev)
            {
                current -= step;
            }
            else if (encoder0Pos > keyPrev)
            {
                current += step;
            }
            if (current > val_max)
                current = val_min;
            if (current < val_min)
                current = val_max;

            keyPrev = encoder0Pos;
            // sprintf(text, "%l", current);

            display.fillRect(x + 1, y + 1, w - 2, 9, WHITE);
            display.setTextColor(BLACK);
            /*for (i = 0; i <= strlen(text); i++) {
                display.setCursor((i * char_with) + x + 2, y + 2);
                display.print(text[i]);
            }*/
            display.setCursor(x + 2, y + 2);
            display.print(current, DEC);
            display.setTextColor(WHITE);
            display.display();
        }

        delay(50);
        if ((digitalRead(keyPush) == LOW))
        {
            currentTime = millis();
            delay(500);
            // if (digitalRead(keyPush) == HIGH)
            ok = true;
        }
    } while (!ok);
    display.fillRect(x + 1, y + 1, w - 2, 9, BLACK);
    display.setCursor(x + 2, y + 2);
    display.print(current, DEC);
    // text[i] = 0;
    display.display();
    // msgBox(F("KEY Back"));
    while (digitalRead(keyPush) == LOW)
        ;
}

void MyComboBox::AddItem(int index, char *str)
{
    strcpy(&item[index][0], (const char *)str);
}

void MyComboBox::AddItem(int index, const char *str)
{
    strcpy(&item[index][0], str);
}

void MyComboBox::GetItem(int index, char *str)
{
    strcpy(str, &item[index][0]);
}

void MyComboBox::maxItem(unsigned char index)
{
    char_max = index;
}

unsigned long MyComboBox::GetValue()
{
    return current;
}

unsigned char MyComboBox::GetIndex()
{
    return current_index;
}
void MyComboBox::SetIndex(unsigned int i)
{
    if (isValue)
    {
        current = i;
    }
    else
    {
        current_index = i;
        if (current_index >= char_max)
            current_index = 0;
    }
}

void MyComboBox::SelectItem()
{
    int w = (length * 7) + 4;
    int keyPrev = encoder0Pos;
    bool ok = false;
    Show();
    display.fillRect(x + 1, y + 1, w - 2, 9, WHITE);
    display.setTextColor(BLACK);
    display.setCursor(x + 2, y + 2);
    display.print(item[current_index]);
    display.setTextColor(WHITE);
    display.display();

    do
    {
        if (encoder0Pos != keyPrev)
        {
            if (encoder0Pos < keyPrev)
            {
                current_index++;
            }
            else if (encoder0Pos > keyPrev)
            {
                current_index--;
            }
            if (current_index >= char_max)
                current_index = char_min;
            // if (current_index < char_min) current_index = char_max;

            keyPrev = encoder0Pos;
            // tb = (double)current / 1000000;
            // dtostrf(tb, 3, 5, text);

            display.fillRect(x + 1, y + 1, w - 2, 9, WHITE);
            display.setTextColor(BLACK);
            display.setCursor(x + 2, y + 2);
            display.print(item[current_index]);
            display.setTextColor(WHITE);
            display.display();
        }

        delay(50);
        if ((digitalRead(keyPush) == LOW))
        {
            currentTime = millis();
            delay(500);
            // if (digitalRead(keyPush) == HIGH)
            ok = true;
        }
    } while (!ok);
    Show();
    // msgBox(F("KEY Back"));
    while (digitalRead(keyPush) == LOW)
        ;
    // Show();
}

void MyComboBox::Show()
{
    int w = (length * 7) + 4;
    // int char_with = 7;
    // unsigned int i;
    display.fillRect(x, y, w + 10, 11, BLACK);
    display.drawRect(x, y, w, 11, WHITE);
    display.drawRect(x + w - 1, y, 10, 11, WHITE);
    // display.fillRect(x + 1, y + 1, w - 2, 9, BLACK);
    if (isSelect)
    {
        // display.drawRect(x + 1, y + 1, w - 2, 9, WHITE);
        // display.fillTriangle(x + w + 1, y + 9, x + w + 1 + 4, y + 2, x + w + 1 + 8, y + 9, WHITE);
        // display.fillTriangle(x + w + 2 + 5, y + 2, x + w + 2 + 5 + 8, y + 2, x + w + 2 + 5 + 4, y + 9, WHITE);
        display.fillTriangle(x + w, y + 2, x + w + 8, y + 2, x + w + 4, y + 9, WHITE);
    }
    else
    {
        // display.drawTriangle(x + w + 1, y + 9, x + w + 1 + 4, y + 2, x + w + 1 + 8, y + 9, WHITE);
        // display.drawTriangle(x + w + 2 + 5, y + 2, x + w + 2 + 5 + 8, y + 2, x + w + 2 + 5 + 4, y + 9, WHITE);
        display.drawTriangle(x + w, y + 2, x + w + 8, y + 2, x + w + 4, y + 9, WHITE);
    }
    if (isValue == true)
    {
        // sprintf(text, "%d", current);
        display.setCursor(x + 2, y + 2);
        display.print(current, DEC);
    }
    else
    {
        display.setCursor(x + 2, y + 2);
        display.print(item[current_index]);
        // for (i = 0; i <= strlen(item[current_index]); i++) {
        //	display.setCursor((i * char_with) + x + 2, y + 2);
        //	display.print(item[current_index][i]);
        // }
    }
    display.display();
}

unsigned char MySymbolBox::GetTable()
{
    return table;
}
unsigned char MySymbolBox::GetSymbol()
{
    return symbol;
}
unsigned char MySymbolBox::GetIndex()
{
    return MySymbolBox::current_index;
}
void MySymbolBox::SetIndex(unsigned char i)
{
    current_index = i;
    if (current_index >= 0x80)
        current_index = 0x80;
}

void MySymbolBox::SelectItem()
{
    int keyPrev = encoder0Pos;
    bool ok = false;
    onSelect = true;
    if (table == '/')
    {
        tableMode = 0;
    }
    else if (table == '\\')
    {
        tableMode = 1;
    }
    else
    {
        tableMode = 2;
    }

    Show();

    do
    {
        if (encoder0Pos != keyPrev)
        {
            if (encoder0Pos < keyPrev)
            {
                current_index++;
            }
            else if (encoder0Pos > keyPrev)
            {
                current_index--;
            }
            if (tableMode == 2)
            {
                if (current_index >= 'Z')
                    current_index = 'A';
                if (current_index < 'A')
                    current_index = 'Z';
            }
            else
            {
                if (current_index >= 0x80)
                    current_index = 0x21;
                if (current_index < 0x21)
                    current_index = 0x80;
            }

            keyPrev = encoder0Pos;
            if (tableMode == 2)
            {
                table = current_index;
                symbol = '&';
            }
            else
            {
                symbol = current_index;
            }
            Show();
        }

        delay(50);
        if ((digitalRead(keyPush) == LOW))
        {
            currentTime = millis();
            while (digitalRead(keyPush) == LOW)
            {
                if ((millis() - currentTime) > 2000)
                    break; // OK Timeout
            };
            if ((millis() - currentTime) < 1500)
            {
                delay(100);
                // currentTime = millis();
                // while (digitalRead(keyPush) == HIGH) {
                //	delay(10);
                //	if ((millis() - currentTime) > 1000) break; //OK Timeout
                // };
                if (++tableMode > 2)
                    tableMode = 0;
                switch (tableMode)
                {
                case 0:
                    table = '/';
                    break;
                case 1:
                    table = '\\';
                    break;
                case 2:
                    if (table < 'A' || table > 'Z')
                        table = 'N';
                    symbol = '&';
                    break;
                }
                // if (table == '/')
                //	table = '\\';
                // else if (table == '\\')
                //	table = '/';
                Show();
                // while (digitalRead(keyPush) == LOW) delay(10);
            }
            else
            {
                ok = true;
            }
        }
    } while (!ok);
    onSelect = false;
    Show();
    // msgBox(F("KEY Back"));
    while (digitalRead(keyPush) == LOW)
        ;
    // Show();
}

void MySymbolBox::Show()
{
    // int w = 16 + 4;
    // int char_with = 7;
    // unsigned int i;
    display.fillRect(x, y, 20 + 14, 20, BLACK);
    // display.drawRect(x, y, 20, 20, WHITE);
    if (isSelect)
    {
        if (onSelect)
        {
            display.drawRoundRect(x, y, 20, 20, 5, WHITE);
        }
        else
        {
            display.drawRect(x, y, 20, 20, WHITE);
            display.drawRect(x + 1, y + 1, 18, 18, WHITE);
        }
        display.setCursor(x + 22, y + 2);
        display.print(table);
        display.setCursor(x + 22, y + 11);
        display.print(symbol);
    }
    else
    {
        display.drawRect(x, y, 20, 20, WHITE);
    }
    const uint8_t *ptrSymbol;
    uint8_t symIdx = symbol - 0x21;
    if (symIdx > 95)
        symIdx = 0;
    if (table == '/')
    {
        ptrSymbol = &Icon_TableA[symIdx][0];
    }
    else if (table == '\\')
    {
        ptrSymbol = &Icon_TableB[symIdx][0];
    }
    else
    {
        if (table < 'A' || table > 'Z')
            table = 'N';
        symbol = '&';
        symIdx = 5; // &
        ptrSymbol = &Icon_TableB[symIdx][0];
    }
    display.drawYBitmap(x + 2, y + 2, ptrSymbol, 16, 16, WHITE);
    if (!(table == '/' || table == '\\'))
    {
        display.drawChar(x + 7, y + 6, table, BLACK, WHITE, 1);
        display.drawChar(x + 8, y + 7, table, BLACK, WHITE, 1);
    }
    display.display();
}

// Renderer - class declared in gui_lcd.h (needed by MenuSystem ms(my_renderer)
// in gui_lcd.cpp's core, since a mere `extern MyRenderer my_renderer;` isn't
// enough for the compiler to see the MenuComponentRenderer base class).
MyRenderer my_renderer;

// void on_stationbeacon_selected(MenuComponent *p_menu_item)
// {
//     int i;
//     MyCheckBox chkBox[3];
//     int max_sel = 8;
//     MySymbolBox symBox[2];
//     MyComboBox cbBox[3];
//     String str;
//     int x;
//     int keyPrev = -1;
//     display.clearDisplay();
//     display.fillRect(0, 0, 128, 16, WHITE);
//     display.setTextColor(BLACK);
//     str = String("STATION BEACON");
//     x = str.length() * 6;
//     display.setCursor(64 - (x / 2), 4);
//     display.print(str);
//     display.setTextColor(WHITE);

//     chkBox[0].Checked = config.trk_compress;
//     chkBox[0].x = 0;
//     chkBox[0].y = 18;
//     sprintf(chkBox[0].text, "COMP");

//     chkBox[1].Checked = config.trk_altitude;
//     chkBox[1].x = 40;
//     chkBox[1].y = 18;
//     sprintf(chkBox[1].text, "ALT");

//     chkBox[2].Checked = config.trk_cst;
//     chkBox[2].x = 75;
//     chkBox[2].y = 18;
//     sprintf(chkBox[2].text, "CSR/SPD");

//     display.setCursor(0, 30);
//     display.print("SPD:");
//     cbBox[0].isValue = true;
//     cbBox[0].x = 25;
//     cbBox[0].y = 28;
//     cbBox[0].length = 3;
//     cbBox[0].char_max = 250;
//     cbBox[0].SetIndex(config.trk_hspeed);

//     display.setCursor(0, 42);
//     display.print("INV:");
//     cbBox[1].isValue = true;
//     cbBox[1].x = 25;
//     cbBox[1].y = 40;
//     cbBox[1].length = 2;
//     cbBox[1].char_max = 120;
//     cbBox[1].SetIndex(config.trk_maxinterval);

//     display.setCursor(0, 54);
//     display.print("ANG:");
//     cbBox[2].isValue = true;
//     cbBox[2].x = 25;
//     cbBox[2].y = 52;
//     cbBox[2].length = 2;
//     cbBox[2].char_max = 180;
//     cbBox[2].SetIndex(config.trk_minangle);

//     display.setCursor(67, 35);
//     display.print("MOV");
//     symBox[0].x = 65;
//     symBox[0].y = 43;
//     symBox[0].table = config.trk_symmove[0];
//     symBox[0].symbol = config.trk_symmove[1];
//     symBox[0].SetIndex(config.trk_symmove[1]);
//     symBox[0].Show();

//     display.setCursor(99, 35);
//     display.print("STP");
//     symBox[1].x = 98;
//     symBox[1].y = 43;
//     symBox[1].table = config.trk_symstop[0];
//     symBox[1].symbol = config.trk_symstop[1];
//     symBox[1].SetIndex(config.trk_symstop[1]);
//     symBox[1].Show();

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
//             for (i = 0; i < 3; i++)
//             {
//                 chkBox[i].isSelect = false;
//                 cbBox[i].isSelect = false;
//             }
//             symBox[0].isSelect = false;
//             symBox[1].isSelect = false;

//             if (encoder0Pos < 3)
//                 chkBox[encoder0Pos].isSelect = true;
//             if (encoder0Pos > 2 && encoder0Pos < 6)
//                 cbBox[encoder0Pos - 3].isSelect = true;
//             if (encoder0Pos > 5)
//                 symBox[encoder0Pos - 6].isSelect = true;
//             for (i = 0; i < 3; i++)
//             {
//                 chkBox[i].CheckBoxShow();
//                 cbBox[i].Show();
//             }
//             symBox[0].Show();
//             symBox[1].Show();
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
//                 if (i < 3)
//                 {
//                     chkBox[i].Toggle();
//                     switch (i)
//                     {
//                     case 0:
//                         config.trk_compress = chkBox[i].Checked;
//                         break;
//                     case 1:
//                         config.trk_altitude = chkBox[i].Checked;
//                         break;
//                     case 2:
//                         config.trk_cst = chkBox[i].Checked;
//                         break;
//                     }
//                     encoder0Pos = keyPrev;
//                     chkBox[i].CheckBoxShow();
//                 }
//                 else if (i > 2 && i < 6)
//                 {
//                     i -= 3;
//                     switch (i)
//                     {
//                     case 0:
//                         cbBox[i].SelectValue(10, 200, 1);
//                         config.trk_hspeed = cbBox[i].GetValue();
//                         break;
//                     case 1:
//                         cbBox[i].SelectValue(5, 60, 1);
//                         config.trk_maxinterval = cbBox[i].GetValue();
//                         break;
//                     case 2:
//                         cbBox[i].SelectValue(5, 90, 1);
//                         config.trk_minangle = cbBox[i].GetValue();
//                         break;
//                     }
//                     encoder0Pos = keyPrev;
//                     cbBox[i].Show();
//                 }
//                 else if (encoder0Pos > 5)
//                 {
//                     i -= 6;
//                     symBox[i].SelectItem();
//                     switch (i)
//                     {
//                     case 0:
//                         config.trk_symmove[0] = symBox[i].table;
//                         config.trk_symmove[1] = symBox[i].symbol;
//                         break;
//                     case 1:
//                         config.trk_symstop[0] = symBox[i].table;
//                         config.trk_symstop[1] = symBox[i].symbol;
//                         break;
//                     }
//                     encoder0Pos = keyPrev;
//                     symBox[i].Show();
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
//     /*display.clearDisplay();
//     display.setCursor(30, 4);
//     display.print("SAVE & EXIT");
//     display.display();*/
//     msgBox("KEY EXIT");
//     while (digitalRead(keyPush) == LOW)
//         ;
//     saveEEPROM();
// }

