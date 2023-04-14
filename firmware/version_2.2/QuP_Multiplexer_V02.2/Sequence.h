//Control files for a sequence handler
//binary format



#ifndef _SEQUENCE_H
#define _SEQUENCE_H

#include <EEPROM.h>

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "MuxDefs.h"
#include "Slave.h"
#include "CircularBufferLib.h"
#include <SerialCommands.h>

#define NOSEQUENCES  32

#define MAX_NO_TASKS MAXBOARDS*NOSEQUENCES

struct Seq
{
   byte byteOne; //SL4:7:6 SL3:5:4 SL2:3:2 SL1:1:0
   byte byteTwo; //RSV:7:4 SL6:3:2 SL5:1:0
   byte byteThree;//TRG DELAY
};


class Sequence
{
public:
  Sequence(uint8_t SeqLength);
  
  struct Seq  *SeqBuffer;

  void SendSequence(byte *structurePointer, int structureLength);

  int ReadSequence(Stream*,uint8_t);
  int ReadSequence(uint8_t);

  bool Store();//bool Store(struct Seq* );
  bool Recall();//bool Recall(struct Seq* );
  uint8_t getSeqSize(void) { return SeqSize;};
  void setSeqSize(uint8_t seqSize){ SeqSize = seqSize;};

  bool setSequenceTask(uint8_t SeqNo, volatile byte&);
  void setSlaves(Slave *pslaves) {pSlaves = pslaves;};
  void setSlavesCnt(uint8_t cnt) {slavecnt = cnt;};
  uint8_t getSlaveCnt(void){return slavecnt;};

  CircularBuffer<Slave*> *pslavesList=NULL;


  bool openGND(uint8_t SeqNo);
  bool closeCH(uint8_t SeqNo);
  
private:

  int8_t eeAddress, CureeAddress;
  uint8_t SeqSize;
  Slave *pSlaves;
  uint8_t slavecnt;
  
  
};

#endif
