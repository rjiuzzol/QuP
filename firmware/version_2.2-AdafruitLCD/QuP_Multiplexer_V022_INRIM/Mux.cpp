//Multiplexer class implementation

#include "Mux.h"


MUX::MUX(const uint8_t board_id=0) : Slave(2)
{
  idBoard = board_id;
  
  CircularBuffer<void*> tasksBuffer(MAX_NO_SEQUENCES);

}
  

uint8_t MUX::Scan(void)
{
  uint8_t iPresents=0;
  /* TO BE IMPLEMENTED*/
  return iPresents;
}
