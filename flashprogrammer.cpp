#include "flashprogrammer.h"
#include "spiflash.h"

void FlashProgrammer::begin( uint32_t offset) {
  _address = offset;

  _flash.begin();
  _flash.reset();
}
void FlashProgrammer::write(uint8_t b) {
  _buffer[_address & 0xFF] = b;
  _bufcontent++;
  _performWrite(); //erases page on early page boundary, writes page on late page boundary
  _address++;
}
void FlashProgrammer::end() {
  //Write the last bits from buffer
  if(_bufcontent > 0) {
    Serial.println("Bufcontent was");
    Serial.print(_bufcontent);
    for(int i = _bufcontent; i < 256; i++) {
      _buffer[i] = 0xFF; //clear invalid data
    }
    _address |= 0xFF; //trigger page write
    _performWrite();  //^ mildly ugly >:/
  }
  _flash.end();
}
void FlashProgrammer::_performWrite() {
  if((_address & 0xFFF) == 0x000) {  //early sector boundary
    _flash.busyWait();
    _flash.writeEnable();
    _flash.busyWait();
    Serial.print("Erasing sector ");
    Serial.println(_address, HEX);
    _flash.sectorErase(_address);
    _flash.busyWait();
    _flash.fastReadBegin(_address & 0xFFF000);
    for(int i = 0; i < 4096; i++) {
      if(_flash.fastReadByte() != 0xFF) {
        Serial.print("!");
      }else{
        Serial.print(".");
      }
      if((i & 0x7F) == 0x7F)
        Serial.println();
    }
    _flash.fastReadEnd();
  }
  
  if((_address & 0xFF) == 0xFF) {  //late page boundary
    _bufcontent = 0;
    _flash.busyWait();
    Serial.print("Writing page ");
    Serial.println(_address, HEX);
    _flash.writeEnable();
    _flash.busyWait();
    _flash.pageProgram(_address & 0xFFFF00, _buffer);
    _flash.busyWait();
    _flash.fastReadBegin(_address & 0xFFFF00);
    for(int i = 0; i < 256; i++) {
      if(_flash.fastReadByte() != _buffer[i]) {
        Serial.print("!");
      }else{
        Serial.print(".");
      }
      if((i & 0x7F) == 0x7F)
        Serial.println();
    }
    _flash.fastReadEnd();
  }
}
