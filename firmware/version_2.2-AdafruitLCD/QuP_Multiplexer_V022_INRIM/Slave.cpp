//Slave class implementation file

#include "MuxDefs.h"
#include "Slave.h"

Slave::Slave(const uint8_t nChannels=2)
{
  NChannels = nChannels;
  CH1gPin=0; CH1iPin=0;
  CH2gPin=0; CH2iPin=0;
  EEAddr=0x00;

  for(int k=0; k<NUMBER_OF_CHANNELS; k++)
    iState[k]=OFF;

  _ena_delay = ENA_DELAY;
}

/*
 * CloseTask() close the channel. 
 * iCH: Channel no. to be close. Close the signal relays and open the GND relay
 * The last delay is optinal: if bWait=TRUE wait
 */
void Slave::CH_CloseTask(uint8_t iCH, bool bWait)
{
  uint8_t iPins=0, gPins=0;
  
  
  switch (iCH){
    case 1:
      gPins = CH1gPin;
      iPins = CH1iPin;
      break;
    case 2:
      gPins = CH2gPin;
      iPins = CH2iPin;
      break;
    default:
      gPins = iPins = 0;
      return;
      
  }
  
  //CLOSE
  digitalWrite(gPins, LOW);
  delay(_ena_delay);
  digitalWrite(iPins, HIGH);
  if(bWait) delay(_ena_delay);

  return;
}

/*
 * CH_OpenTask() open the channel. 
 * iCH: Channel no. to be open. Open the signal relays and close the GND relay
 * The last delay is optinal: if bWait=TRUE wait
 */

void Slave::CH_OpenTask(uint8_t iCH, bool bWait)
{
  uint8_t iPins, gPins;
  
  switch (iCH){
    case 1:
      gPins = CH1gPin;
      iPins = CH1iPin;
      break;
    case 2:
      gPins = CH2gPin;
      iPins = CH2iPin;
      break;
      
  }
  //OPEN
  digitalWrite(iPins, LOW);
  delay(_ena_delay);
  digitalWrite(gPins, HIGH);
  if(bWait) delay(_ena_delay);
  
}

/*
 * CH_ENAGRD: Open/close the GRD relay
 * iCH: the channel no. to enable de GRD
 * state: ON/OFF
*/
void Slave::CH_ENAGRD(uint8_t iCH, state_t state)
{
  uint8_t  Pin=0;
  
  
  switch (iCH){
    case 1:
      Pin = CH1grdPin;
      break;
    case 2:
      Pin = CH2grdPin;
      break;
    default:
      Pin = 0;
      return;
      
  }

  digitalWrite(Pin, state);
  delay(_ena_delay);

  return;
}


//OPEN/CLOSE the corresponding signal relays for the given Channel
void Slave::CH_ENA(uint8_t iCH, state_t state)
{
  uint8_t iPins;
  
  switch (iCH){
    case 1:
      iPins = CH1iPin;
      iState[1] = state;
      break;
    case 2:
      iPins = CH2iPin;
      iState[2] = state;
      break;
      
  }
  //ENA
  digitalWrite(iPins, state);

  return;  
}


//OPEN/CLOSE the corresponding GND relay for the given Channel
void Slave::CH_GND(uint8_t iCH, state_t state)
{
  uint8_t gPins;
  
  switch (iCH){
    case 1:
      gPins = CH1gPin;

      break;
    case 2:
      gPins = CH2gPin;
      
      break;
      
  }
  //ENA
  digitalWrite(gPins, state);

  return;  
}
