// Multiplexer class definition

#ifndef _MULTIPLEXER_H
#define _MULTIPLEXER_H


#include "Slave.h"
#include "MuxDefs.h"
#include "CircularBufferLib.h"

#include <LiquidCrystal_I2C.h>


class MUX : public Slave
{
public:
  MUX(const uint8_t board_id);
  
  bool IsEnabled(){return bEnabled;};
  bool IsUsed(){return bInUse;};

  uint8_t idBoard;
  uint8_t Scan(void); //Scan for the slaves. Return the number of slaves presents
  unsigned long setdly(unsigned long dly){_ena_delay = dly; return dly;} /*dly in ms*/
  unsigned long getdly(void){return _ena_delay;};
  
private:
  bool bEnabled=false;
  bool bInUse=false;
  byte nrButtons = NROBUTTONS; // change to 4 if additional button added

  
};

#endif
