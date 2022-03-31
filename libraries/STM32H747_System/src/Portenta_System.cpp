#ifdef ARDUINO_PORTENTA_H7_M7

#include "Portenta_System.h"
#include "Wire.h"
#include "mbed.h"

#define PMIC_ADDRESS 0x08

bool arduino::Portenta_System::begin()
{
  Wire1.begin();
}

bool arduino::Portenta_System::enterLowPower() {
  /* TO DO */
}

bool arduino::Portenta_System::setRDPLevel(STM32H747_RDP level) {
  FLASH_OBProgramInitTypeDef OBInit;
  bool result = true;

  HAL_FLASHEx_OBGetConfig(&OBInit);

  if(OBInit.RDPLevel != level) {
    if(HAL_FLASH_OB_Unlock() != HAL_OK) {
      return false;
    }
    OBInit.OptionType = OPTIONBYTE_RDP;
    OBInit.RDPLevel = level;
    if(HAL_FLASHEx_OBProgram(&OBInit) == HAL_OK) {
      result = HAL_FLASH_OB_Launch();
    }
    HAL_FLASH_OB_Lock();
  }

  return result;
}

arduino::Portenta_System Portenta;

#endif