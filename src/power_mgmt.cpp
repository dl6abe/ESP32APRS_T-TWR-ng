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


bool powerEvent = true;

void powerSave()
{
  if (config.oled_enable)
  {
    display.dim(true);
  }
  else
  {
    // if (digitalRead(PWR_VDD))
    // {
    //   powerEvent = false;
    //   display.clearDisplay();
    //   display.display();
    //   // if (!config.trk_smartbeacon)
    //   //{
    //   digitalWrite(PWR_VDD, LOW);
    //   //}
    // }
  }
}

void powerWakeup()
{
  if (config.oled_enable)
  {
    display.dim(false);
  }
  else
  {
    // if (!digitalRead(PWR_VDD))
    // {
    //   powerEvent = true;
    //   digitalWrite(PWR_VDD, HIGH);
    //   delay(10);
    //   display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false);
    //   display.clearDisplay();
    //   display.display();
    //   GPS_INIT();
    // }
  }
}

bool powerStatus()
{
  return 1;
  // digitalRead(PWR_VDD);
}

void setupPower()
{
  bool result = PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);
  if (result == false)
  {
    while (1)
    {
      projLog(LOGCAT_SYSTEM, "PMU is not online...");
      delay(500);
    }
  }

  // Set the minimum common working voltage of the PMU VBUS input,
  // below this value will turn off the PMU
  PMU.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_3V88);

  // Set the maximum current of the PMU VBUS input,
  // higher than this value will turn off the PMU
  PMU.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_2000MA);

  // Get the VSYS shutdown voltage
  uint16_t vol = PMU.getSysPowerDownVoltage();
  projLog(LOGCAT_SYSTEM, "->  getSysPowerDownVoltage:%u", vol);

  // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
  PMU.setSysPowerDownVoltage(2600);

  //! DC1 ESP32S3 Core VDD , Don't change
  // PMU.setDC1Voltage(3300);

  //! DC3 Radio & Pixels VDD , Don't change
  PMU.setDC3Voltage(3400);

  //! ALDO2 MICRO TF Card VDD, Don't change
  PMU.setALDO2Voltage(3300);

  //! ALDO4 GNSS VDD, Don't change
  PMU.setALDO4Voltage(3300);

  //! BLDO1 MIC VDD, Don't change
  PMU.setBLDO1Voltage(3300);

  //! The following supply voltages can be controlled by the user
  // DC5 IMAX=2A
  // 1200mV
  // 1400~3700mV,100mV/step,24steps
  PMU.setDC5Voltage(3300);

  // ALDO1 IMAX=300mA
  // 500~3500mV, 100mV/step,31steps
  PMU.setALDO1Voltage(3300);

  // ALDO3 IMAX=300mA
  // 500~3500mV, 100mV/step,31steps
  PMU.setALDO3Voltage(3300);

  // BLDO2 IMAX=300mA
  // 500~3500mV, 100mV/step,31steps
  PMU.setBLDO2Voltage(3300);

  //! END

  // Turn on the power that needs to be used
  //! DC1 ESP32S3 Core VDD , Don't change
  // PMU.enableDC3();

  //! External pin power supply
  PMU.enableDC5();
  PMU.enableALDO1();
  PMU.enableALDO3();
  PMU.enableBLDO2();

  //! ALDO2 MICRO TF Card VDD
  PMU.enableALDO2();

  //! ALDO4 GNSS VDD
  PMU.enableALDO4();

  //! BLDO1 MIC VDD
  PMU.enableBLDO1();

  //! DC3 Radio & Pixels VDD
  PMU.enableDC3();

  // power off when not in use
  PMU.disableDC2();
  PMU.disableDC4();
  PMU.disableCPUSLDO();
  PMU.disableDLDO1();
  PMU.disableDLDO2();

  projLog(LOGCAT_SYSTEM, "DCDC=======================================================================");
  projLog(LOGCAT_SYSTEM, "DC1  : %s   Voltage:%u mV ", PMU.isEnableDC1() ? "+" : "-", PMU.getDC1Voltage());
  projLog(LOGCAT_SYSTEM, "DC2  : %s   Voltage:%u mV ", PMU.isEnableDC2() ? "+" : "-", PMU.getDC2Voltage());
  projLog(LOGCAT_SYSTEM, "DC3  : %s   Voltage:%u mV ", PMU.isEnableDC3() ? "+" : "-", PMU.getDC3Voltage());
  projLog(LOGCAT_SYSTEM, "DC4  : %s   Voltage:%u mV ", PMU.isEnableDC4() ? "+" : "-", PMU.getDC4Voltage());
  projLog(LOGCAT_SYSTEM, "DC5  : %s   Voltage:%u mV ", PMU.isEnableDC5() ? "+" : "-", PMU.getDC5Voltage());
  projLog(LOGCAT_SYSTEM, "ALDO=======================================================================");
  projLog(LOGCAT_SYSTEM, "ALDO1: %s   Voltage:%u mV", PMU.isEnableALDO1() ? "+" : "-", PMU.getALDO1Voltage());
  projLog(LOGCAT_SYSTEM, "ALDO2: %s   Voltage:%u mV", PMU.isEnableALDO2() ? "+" : "-", PMU.getALDO2Voltage());
  projLog(LOGCAT_SYSTEM, "ALDO3: %s   Voltage:%u mV", PMU.isEnableALDO3() ? "+" : "-", PMU.getALDO3Voltage());
  projLog(LOGCAT_SYSTEM, "ALDO4: %s   Voltage:%u mV", PMU.isEnableALDO4() ? "+" : "-", PMU.getALDO4Voltage());
  projLog(LOGCAT_SYSTEM, "BLDO=======================================================================");
  projLog(LOGCAT_SYSTEM, "BLDO1: %s   Voltage:%u mV", PMU.isEnableBLDO1() ? "+" : "-", PMU.getBLDO1Voltage());
  projLog(LOGCAT_SYSTEM, "BLDO2: %s   Voltage:%u mV", PMU.isEnableBLDO2() ? "+" : "-", PMU.getBLDO2Voltage());
  projLog(LOGCAT_SYSTEM, "===========================================================================");

  // Set the time of pressing the button to turn off
  PMU.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
  uint8_t opt = PMU.getPowerKeyPressOffTime();
  projLog(LOGCAT_SYSTEM, "PowerKeyPressOffTime:");
  switch (opt)
  {
  case XPOWERS_POWEROFF_4S:
    projLog(LOGCAT_SYSTEM, "4 Second");
    break;
  case XPOWERS_POWEROFF_6S:
    projLog(LOGCAT_SYSTEM, "6 Second");
    break;
  case XPOWERS_POWEROFF_8S:
    projLog(LOGCAT_SYSTEM, "8 Second");
    break;
  case XPOWERS_POWEROFF_10S:
    projLog(LOGCAT_SYSTEM, "10 Second");
    break;
  default:
    break;
  }
  // Set the button power-on press time
  PMU.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
  opt = PMU.getPowerKeyPressOnTime();
  projLog(LOGCAT_SYSTEM, "PowerKeyPressOnTime:");
  switch (opt)
  {
  case XPOWERS_POWERON_128MS:
    projLog(LOGCAT_SYSTEM, "128 Ms");
    break;
  case XPOWERS_POWERON_512MS:
    projLog(LOGCAT_SYSTEM, "512 Ms");
    break;
  case XPOWERS_POWERON_1S:
    projLog(LOGCAT_SYSTEM, "1 Second");
    break;
  case XPOWERS_POWERON_2S:
    projLog(LOGCAT_SYSTEM, "2 Second");
    break;
  default:
    break;
  }

  Serial.println("===========================================================================");
  // It is necessary to disable the detection function of the TS pin on the board
  // without the battery temperature detection function, otherwise it will cause abnormal charging
  PMU.disableTSPinMeasure();

  // Enable internal ADC detection
  PMU.enableBattDetection();
  PMU.enableVbusVoltageMeasure();
  PMU.enableBattVoltageMeasure();
  PMU.enableSystemVoltageMeasure();

  /*
    The default setting is CHGLED is automatically controlled by the PMU.
  - XPOWERS_CHG_LED_OFF,
  - XPOWERS_CHG_LED_BLINK_1HZ,
  - XPOWERS_CHG_LED_BLINK_4HZ,
  - XPOWERS_CHG_LED_ON,
  - XPOWERS_CHG_LED_CTRL_CHG,
  * */
  PMU.setChargingLedMode(XPOWERS_CHG_LED_BLINK_1HZ);

  // Force add pull-up
  pinMode(PMU_IRQ, INPUT_PULLUP);
  // attachInterrupt(PMU_IRQ, setFlag, FALLING);

  // Disable all interrupts
  PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  // Clear all interrupt flags
  PMU.clearIrqStatus();
  // Enable the required interrupt function
  PMU.enableIRQ(
      XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |    // BATTERY
      XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |  // VBUS
      XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |     // POWER KEY
      XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ // CHARGE
  );

  // Set the precharge charging current
  PMU.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_150MA);

  // Set constant current charge current limit
  //! Using inferior USB cables and adapters will not reach the maximum charging current.
  //! Please pay attention to add a suitable heat sink above the PMU when setting the charging current to 1A
  PMU.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_1000MA);

  // Set stop charging termination current
  PMU.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_150MA);

  // Set charge cut-off voltage
  PMU.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

  // Disable the PMU long press shutdown function
  // PMU.disableLongPressShutdown();
  PMU.enableLongPressShutdown();

  // Get charging target current
  const uint16_t currTable[] = {
      0, 0, 0, 0, 100, 125, 150, 175, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
  uint8_t val = PMU.getChargerConstantCurr();
  projLog(LOGCAT_SYSTEM, "Val = %d", val);
  projLog(LOGCAT_SYSTEM, "Setting Charge Target Current : %d", currTable[val]);

  // Get charging target voltage
  const uint16_t tableVoltage[] = {
      0, 4000, 4100, 4200, 4350, 4400, 255};
  val = PMU.getChargeTargetVoltage();
  projLog(LOGCAT_SYSTEM, "Setting Charge Target Voltage : %d", tableVoltage[val]);
}

