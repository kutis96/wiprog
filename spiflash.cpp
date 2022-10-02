#include <stdint.h>
#include <list>
#include "spiflash.h"
#include <SPI.h>

const uint8_t SPIF_ENABLE_RESET = 0x66;
const uint8_t SPIF_RESET = 0x99;
const uint8_t SPIF_READ_JEDEC_ID = 0x9F;
const uint8_t SPIF_READ_UNIQUE_ID = 0x4B;
const uint8_t SPIF_FAST_READ = 0x0B;
const uint8_t SPIF_READ_STATUS_REG_1 = 0x05;
const uint8_t SPIF_READ_STATUS_REG_2 = 0x35;
const uint8_t SPIF_READ_STATUS_REG_3 = 0x15;
const uint8_t SPIF_WRITE_ENABLE = 0x06;
const uint8_t SPIF_WRITE_DISABLE = 0x04;
const uint8_t SPIF_SECTOR_ERASE = 0x20;
const uint8_t SPIF_CHIP_ERASE = 0x60;
const uint8_t SPIF_PAGE_PROGRAM = 0x02;


void SPIFlash::reset() {
  enable();
  transfer(SPIF_ENABLE_RESET);
  disable();
  enable();
  transfer(SPIF_RESET);
  disable();
}

void SPIFlash::writeEnable() {
  enable();
  transfer(SPIF_WRITE_ENABLE);
  disable();
}

void SPIFlash::writeDisable() {
  enable();
  transfer(SPIF_WRITE_DISABLE);
  disable();  
}

void SPIFlash::sectorErase(uint32_t address) {  
  enable();
  transfer(SPIF_SECTOR_ERASE);
  transfer24(address);
  disable();
}

void SPIFlash::chipErase() {  
  enable();
  transfer(SPIF_CHIP_ERASE);
  disable();
}

void SPIFlash::pageProgram(uint32_t address, uint8_t buffer256[]) {
  Serial.print("Programming from address ");
  Serial.println(address, HEX);
  enable();
  transfer(SPIF_PAGE_PROGRAM);
  transfer24(address);
  for(int i = 0; i < 256; i++) {
    transfer(buffer256[i]);
  }
  disable();
}

void SPIFlash::busyWait() {
  enable();
  transfer(SPIF_READ_STATUS_REG_1);
  long startTime = millis();
  while(transfer(0) & 1) {
    //busywait
    delay(1);
    if((millis() - startTime) > timeout) {
      disable();
      throw TIMEOUT_EXPIRED;
    }
  };
  disable();
}

uint32_t SPIFlash::transfer24(uint32_t address) {
  uint32_t buf = transfer((address >> 16) & 0xFF);
  buf = (buf << 8) | transfer((address >> 8) & 0xFF);
  buf = (buf << 8) | transfer((address) & 0xFF);
  return buf;
}

uint64_t SPIFlash::transfer64(uint64_t data) {
  uint64_t result = 0;
  for(uint8_t i = 0; i < 64/8; i++) {
    result = (result << 8) | transfer(data & (0xFFLL << (64-8)));
    data = data << 8;
  }
  return result;
}

uint32_t SPIFlash::getJedecId() {
  enable();
  transfer(SPIF_READ_JEDEC_ID);
  uint32_t result = 0;
  result |= transfer(0x00) << 16;
  result |= transfer(0x00) << 8;
  result |= transfer(0x00);
  disable();
  return result;
}

uint64_t SPIFlash::getUniqueId() {
  enable();
  transfer(SPIF_READ_UNIQUE_ID);
  transfer(0); //dummy
  transfer(0); //dummy
  transfer(0); //dummy
  transfer(0); //dummy
  uint64_t uid = transfer64(0);
  disable();
  return uid;
}

uint32_t SPIFlash::getSize() {
  return 1 << 24; //TODO: Implement
}

void SPIFlash::fastReadBegin(uint32_t address) {
  enable();
  transfer(SPIF_FAST_READ);
  transfer24(address);
  transfer(0); //dummy
}

void SPIFlash::fastReadEnd() {
  disable();
}

uint8_t SPIFlash::fastReadByte() {
  return transfer(0);
}

void SPIFlash::fastRead(uint32_t address, const uint32_t howMany, uint8_t* buffer) {
  fastReadBegin(address);
  for(uint32_t i = 0; i < howMany; i++)
    buffer[i] = fastReadByte();
  fastReadEnd();
}

void SPIFlash::enable() {
  digitalWrite(cspin, LOW);
  SPI.beginTransaction(
    SPISettings(frequency, MSBFIRST, SPI_MODE0)
  );
}
void SPIFlash::disable() {
  SPI.endTransaction();
  digitalWrite(cspin, HIGH);
}
uint8_t SPIFlash::transfer(uint8_t what) {
  return (SPI.transfer(what) & 0xFF);
}
