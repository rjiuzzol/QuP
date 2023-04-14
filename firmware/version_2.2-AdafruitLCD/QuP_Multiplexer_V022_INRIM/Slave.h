//Slave class definition

#ifndef _SLAVE_H
#define _SLAVE_H

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include <EEPROM.h>

#define NUMBER_OF_CHANNELS 2

typedef enum {ON=HIGH, OFF=LOW} state_t;

class Slave
{
public:
  Slave(const uint8_t nChannels=2);
  uint8_t GetNChannels(void){return NChannels;};
  bool bPresent=false;

  state_t iState[NUMBER_OF_CHANNELS];
  
  void setCH1gPin(uint8_t Pin){CH1gPin = Pin;}; //Ground PIN
  void setCH1iPin(uint8_t Pin){CH1iPin = Pin;}; //Signal PIN
  void setCH1grdPin(uint8_t Pin){CH1grdPin = Pin;}; //Guard PIN
  
  void setCH2gPin(uint8_t Pin){CH2gPin = Pin;}; //Ground PIN
  void setCH2iPin(uint8_t Pin){CH2iPin = Pin;}; //Signal PIN
  void setCH2grdPin(uint8_t Pin){CH2grdPin = Pin;}; //Gruard PIN


  void CH_CloseTask(uint8_t iCH, bool);
  void CH_OpenTask(uint8_t iCH, bool);
  
  void CH_ENAGRD(uint8_t iCH, state_t state);  
  
  void CH_Task();
  void CH_ENA(uint8_t iCH, state_t state);
  void CH_GND(uint8_t iCH, state_t state);
  
  void (*pointerT)(int, int);

  uint8_t setEnaDelay(uint8_t dly){Enadly=dly;};
  uint8_t getEnaDelay(){return Enadly;};
   
  byte ID; //SLAVE ID number 

  byte getEEAddr(){return EEAddr;};
  bool setEEAddr(byte Addr){EEAddr=Addr; return true;};


  unsigned long _ena_delay;
  
private:

  uint8_t NChannels;
  uint8_t CH1gPin, CH1iPin, CH1grdPin;
  uint8_t CH2gPin, CH2iPin, CH2grdPin;
  uint8_t Enadly; //waiting delay

  byte EEAddr;
  
  

  
   
};

#endif 
