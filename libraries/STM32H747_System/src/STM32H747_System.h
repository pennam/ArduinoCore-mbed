#ifndef H747_System_h_
#define H747_System_h_

#include "Arduino.h"

enum STM32H747_RDP
  {
    LEVEL_0 = 0xAA00U,
    LEVEL_1 = 0x5500U,
    LEVEL_2 = 0xCC00U
  };

class STM32H747 {

public:
  virtual bool begin() = 0;
  virtual bool enterLowPower() = 0;
  virtual bool setRDPLevel(STM32H747_RDP level) = 0;



protected:
  uint8_t readReg(uint8_t subAddress);
  void setRegister(uint8_t reg, uint8_t val);
  bool useInternalOscillator(bool lowspeed = false);
};

#endif