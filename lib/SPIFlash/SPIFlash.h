#ifndef SPIFLASH_H
#define SPIFLASH_H

#include <list>

enum SPIFlashFailure
{
  TIMEOUT_EXPIRED
};

class SPIFlash
{
public:
  SPIFlash(int _cs, long _frequency = 50e6, long _timeout = 1000) : cspin(_cs), frequency(_frequency), timeout(_timeout){};
  void begin()
  {
    pinMode(cspin, OUTPUT);
    disable();
    reset();
  };
  void end() { pinMode(cspin, INPUT); };
  void reset();
  uint32_t getJedecId();
  uint64_t getUniqueId();
  void fastRead(uint32_t address, uint32_t howMany, uint8_t *buffer);
  void fastReadBegin(uint32_t address);
  uint8_t fastReadByte();
  void fastReadEnd();
  /**
   * Busy-waits on SPI status
   */
  void busyWait();
  void writeEnable();
  void writeDisable();
  void chipErase();
  void sectorErase(uint32_t address); // 4K sectors
  void pageProgram(uint32_t address, uint8_t *buffer256);
  uint32_t getSize(); // get size in bytes
private:
  void enable();
  void disable();
  uint8_t transfer(uint8_t what);
  uint32_t transfer24(uint32_t address);
  uint64_t transfer64(uint64_t address);

  int cspin;          // chip select pin
  uint64_t frequency; // SPI frequency
  uint64_t timeout;   // busywait timeout in milliseconds
};

#endif