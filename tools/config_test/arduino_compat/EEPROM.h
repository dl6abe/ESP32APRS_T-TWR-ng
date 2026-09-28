#pragma once
#include <cstdint>
class EEPROMClass {
public:
  bool begin(size_t) { return true; }
  uint8_t read(int) { return 0; }
  void write(int, uint8_t) {}
  void readBytes(int, void*, size_t) {}
  void writeBytes(int, const void*, size_t) {}
  bool commit() { return true; }
};
extern EEPROMClass EEPROM;
