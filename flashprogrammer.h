#if (ARDUINO >= 100)
#include "Arduino.h"
#endif

#include "spiflash.h"

#ifndef FLASHPROGRAMMER_H
#define FLASHPROGRAMMER_H

class FlashProgrammer {
public:
  FlashProgrammer(SPIFlash& flash) : _flash(flash) {};
  void begin(uint32_t offset);
  void write(uint8_t b);
  void end();
private:
  uint32_t _address;
  uint8_t _buffer[256];
  uint8_t _bufcontent;
  void _performWrite();
  SPIFlash& _flash;
};

#endif
