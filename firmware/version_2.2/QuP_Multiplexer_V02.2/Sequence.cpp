
#include "Sequence.h"

#define MASK_01 0b0000000000000011

//Circuilar list to get access to the sequence 
CircularBuffer<void*> SeqList(NOSEQUENCES);

//Default construntor 
//Initialize the buffer in RAM to store the sequence
Sequence::Sequence(uint8_t SeqLength)
{
  SeqBuffer = new struct Seq[SeqLength];
  SeqSize = SeqLength;
  slavecnt = 0;

  //Init all sequence mem buffers in ZERO
  for(int k=0; k < SeqSize; k++){
     this->SeqBuffer[k].byteOne = 0b0;
     this->SeqBuffer[k].byteTwo = 0b0;
     this->SeqBuffer[k].byteThree = 0b1;

     //Add a node to the sequence circular list
     SeqList.Add(&SeqBuffer[k]);
  }

  //Then, there is not sequence in mem, so I set the SeqSize to Zero to avoid errors
  SeqSize = 0; //To Zero
    
  eeAddress = CureeAddress= 0xA; //Init the EEPROM Address to read/write
  return;
}

//Function to run a Sequence
//SeqNo: the sequence numbre to run
//TRGwait: the address( reference) to a variable where the waiting time (number of trigger pulses) is store
bool Sequence::setSequenceTask(uint8_t SeqNo, volatile byte& TRGwait)
{

  
  Slave *pSlave=NULL;
  uint16_t Activate=0x0000;
  
  byte Act=0x00;
  
  //Set Activate variable for all slaves
  Activate = SeqBuffer[SeqNo].byteTwo;
  Activate <<= 8;
  Activate |= SeqBuffer[SeqNo].byteOne;

  

  //Here I have to parse and config the sequence to be run by the controller

  for(int k=0; k < slavecnt; k++){
    pSlave = (*pslavesList)[k];
    
    switch(pSlave->ID){
      case 01:
        Act = Activate & MASK_01;
        break;
      case 02:
        Act = (Activate>>2) & MASK_01;
        break;
      case 03:
        Act = (Activate>>4) & MASK_01;
        break;
      case 04:
        Act = (Activate>>6) & MASK_01;
        break;
      case 05:
        Act = (Activate>>8) & MASK_01;
        break;
      case 06:
        Act = (Activate>>10) & MASK_01;
        break;
      default:
        Serial.println("SEQ ERROR");
        return false;
    }
  
    
    switch (Act){
      case 0b01:
        //CH01
        pSlave->CH_CloseTask(1,true);
        break;
      case 0b10: 
        //CH02
        pSlave->CH_CloseTask(2, true);
        break;
      case 0b11:
        //BOTH
        pSlave->CH_CloseTask(1, true);
        pSlave->CH_CloseTask(2, true);
        break;
      default:
        break; //NOT Valid
    }
  }
  TRGwait = SeqBuffer[SeqNo].byteThree;
    
  return true;
}


//SEQUENCE running functions
//First OPEN ALL GND of the sequence
bool Sequence::openGND(uint8_t SeqNo)
{
  
  Slave *pSlave=NULL;
  uint16_t Activate=0x0000;
  
  byte Act=0x00;
  
  //Set Activate variable for all slaves
  Activate = SeqBuffer[SeqNo].byteTwo;
  Activate <<= 8;
  Activate |= SeqBuffer[SeqNo].byteOne;

  
  //Here I have to parse and config the sequence to be run by the controller

  for(int k=0; k < slavecnt; k++){
    pSlave = (*pslavesList)[k];
    
    switch(pSlave->ID){
      case 01:
        Act = Activate & MASK_01;
        break;
      case 02:
        Act = (Activate>>2) & MASK_01;
        break;
      case 03:
        Act = (Activate>>4) & MASK_01;
        break;
      case 04:
        Act = (Activate>>6) & MASK_01;
        break;
      case 05:
        Act = (Activate>>8) & MASK_01;
        break;
      case 06:
        Act = (Activate>>10) & MASK_01;
        break;
      default:
        Serial.println("SEQ ERROR");
        return false;
    }
  
    
    switch (Act){
      case 0b01:
        //CH01
        pSlave->CH_GND(1,OFF);
        break;
      case 0b10: 
        //CH02
        pSlave->CH_GND(2, OFF);
        break;
      case 0b11:
        //BOTH
        pSlave->CH_GND(1, OFF);
        pSlave->CH_GND(2, OFF);
        break;
      default:
        break; //NOT Valid
    }
  }
  
  return true;
}

//Second Close ALL channels 
bool Sequence::closeCH(uint8_t SeqNo)
{
  
  Slave *pSlave=NULL;
  uint16_t Activate=0x0000;
  
  byte Act=0x00;
  
  //Set Activate variable for all slaves
  Activate = SeqBuffer[SeqNo].byteTwo;
  Activate <<= 8;
  Activate |= SeqBuffer[SeqNo].byteOne;

  


  //Here I have to parse and config the sequence to be run by the controller

  for(int k=0; k < slavecnt; k++){
    pSlave = (*pslavesList)[k];
    
    switch(pSlave->ID){
      case 01:
        Act = Activate & MASK_01;
        break;
      case 02:
        Act = (Activate>>2) & MASK_01;
        break;
      case 03:
        Act = (Activate>>4) & MASK_01;
        break;
      case 04:
        Act = (Activate>>6) & MASK_01;
        break;
      case 05:
        Act = (Activate>>8) & MASK_01;
        break;
      case 06:
        Act = (Activate>>10) & MASK_01;
        break;
      default:
        Serial.println("SEQ ERROR");
        return false;
    }
  
    
    switch (Act){
      case 0b01:
        //CH01
        pSlave->CH_ENA(1, ON);
        break;
      case 0b10: 
        //CH02
        pSlave->CH_ENA(2, ON);
        break;
      case 0b11:
        //BOTH
        pSlave->CH_ENA(1, ON);
        pSlave->CH_ENA(2, ON);
        break;
      default:
        break; //NOT Valid
    }
  }

  return true;
}


//Store.
//Function to store the current sequence to the internal EEPROM
bool Sequence::Store()
{
  EEPROM.put(eeAddress,SeqSize);
  CureeAddress = eeAddress + sizeof(byte);
  
  for(unsigned int k=0; k < SeqSize; k++){
    EEPROM.put(CureeAddress, SeqBuffer+k);
    CureeAddress += sizeof(Seq);
  }
    

  return true;
}
  
//Function Recall
//Recall the stored sequence from the EEPROM
bool Sequence::Recall()
{
  
  uint8_t Cnt = EEPROM.read(eeAddress);
  uint8_t auxAdd = eeAddress + sizeof(byte);
  Serial.println(Cnt);
  
  for(uint8_t k=0; k< (Cnt*3);k++){
    EEPROM.get(auxAdd, SeqBuffer[k]);
    auxAdd += sizeof(Seq);
  }

  setSeqSize(Cnt);
  return true;
}


//Send the current sequence to a Serial port
void Sequence::SendSequence(byte *structurePointer, int structureLength)
{
  /*TO BE IMPLEMENTED*/

}


//Read the sequence from the serial Port 
//Read the sequence from the serial Port 
int Sequence::ReadSequence(Stream* serial, uint8_t Cnt)
{
    
  for(uint8_t k=0; k<Cnt;k++){
    serial->readBytes((byte*)(SeqBuffer+k), sizeof(Seq));
  }

  
  return Cnt;
}

//Read the sequence from the serial Port 
int Sequence::ReadSequence(uint8_t Cnt)
{

    
  for(uint8_t k=0; k<Cnt;k++){
    Serial.readBytes((byte*)(SeqBuffer+k), sizeof(Seq));
  }

  
  return Cnt;
}
