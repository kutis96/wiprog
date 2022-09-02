#if (ARDUINO >= 100)
#include "Arduino.h"
#endif

#include <list>

#ifndef SPIFLASH_H
#define SPIFLASH_H

class SPIFlash {
  public:
    SPIFlash(int _cs) : cspin(_cs) {};
    void begin() {pinMode(cspin, OUTPUT); disable(); reset();};
    void end() {pinMode(cspin, INPUT); };
    void reset();
    uint32_t getJedecId();
    uint64_t getUniqueId();
    void fastRead(uint32_t address, uint32_t howMany, uint8_t* buffer);
    void fastReadBegin(uint32_t address);
    uint8_t fastReadByte();
    void fastReadEnd();
    void busyWait();
    void busyWait2();
    void writeEnable();
    void writeDisable();
    void chipErase();
    void sectorErase(uint32_t address); //4K sectors
    void pageProgram(uint32_t address, uint8_t* buffer256);
  private:
    void enable();
    void disable();
    uint8_t transfer(uint8_t what);
    uint32_t transfer24(uint32_t address);
    uint64_t transfer64(uint64_t address);
    
    int cspin;
};

#endif
