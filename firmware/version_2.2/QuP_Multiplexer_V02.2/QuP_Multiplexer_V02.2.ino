/***************************************************
Multiplexer Arduino firmware

Author: RJI
Date: May-2021
Date: Sept-2022
Date: May-2023

This is the multiplexer firmware based on Arduino UNO
for the QuP project

Project: QuP

Version: 2.2

Revision: - Fix some bugs and errors
          ' *IDN? returns compile date/time 
          - CMD OK response to a commands Set operation
          - NSLAVE? command added
          - WSLAVES? command added
          - ERROR Codes
          - ERROR MSGs
          - MSG text are store in program mem
          - TRIGGER POL polarity (SEPT-2022)
          - Sequence late start. The control variables are not init at every start/stop
 ****************************************************/
//Libraries includes
#include <SerialCommands.h>
#include "CircularBufferLib.h"
#include <DebounceEvent.h>
#include <avr/pgmspace.h>

//My Includes
#include "Mux.h"
#include "Slave.h"
#include "Sequence.h"


//#define _DEBUG_ // Uncomment to have a defualt sequence at start-up 

MUX MUX(1);
Slave slave[MAXBOARDS], *pSlave=NULL;
Sequence sequence(NOSEQUENCES), *pSequence=NULL;
CircularBuffer<Slave*> slavesList(MAXBOARDS);

char msg[40];

byte buttons[] = {TRIGGER_PIN,
                  BUTTON_PIN,
                  RENLOCAL_PIN,
                  RESET_PIN};//2 EXT TRIGGER
                          //4 Change Channel
                          //8 LOC / REM not yet implemented
                          //9 INT / EXT not yet implemented
byte nrButtons = 4; // change to 4 if additional button added
bool bPressed[] = {false, false,false, false};


// Set the LCD address to 0x27 for a 20 chars and 4 line display
LiquidCrystal_I2C lcd(LCDADD, DSPCOLS, DSPROWS);

//#define NUMBEROFCHANNELS 4

volatile byte iChManual = 0;
volatile byte iSeqManual = 0;
volatile uint8_t islaveSeq=0; //Slave sequence counter

volatile unsigned int timer_T=1000; // 1 sec period timer_T
double _desired_T=1; // 1 sec initial TIMER control variable by serial commands

// TRG and Sequence control variables
volatile byte TRGCounter=0;
volatile byte trgWait=0;
volatile bool extTRG=false;


byte SLPins[MAXBOARDS][6] ={
{CH1_ENA, CH2_ENA, CH1_GND, CH2_GND, CH1_GRD, CH2_GRD},
{CH3_ENA, CH4_ENA, CH3_GND, CH4_GND, CH3_GRD, CH4_GRD},
{CH5_ENA, CH6_ENA, CH5_GND, CH6_GND, CH5_GRD, CH6_GRD},
{CH7_ENA, CH8_ENA, CH7_GND, CH8_GND, CH7_GRD, CH8_GRD},
{CH9_ENA, CH10_ENA, CH9_GND, CH10_GND, CH9_GRD, CH10_GRD},
{CH11_ENA, CH12_ENA, CH11_GND, CH12_GND, CH11_GRD, CH12_GRD}};

int  bPins [MAXBOARDS] = {BRD_1, BRD_2, BRD_3, BRD_4, BRD_5, BRD_6}; //Boards detection PIN assigment


//Text Messages
const PROGMEM char IdName[] = "QuP Multiplexer V2.2 ";
const PROGMEM char compile_date[] = "2023-04-14";
const PROGMEM char msg_bad_slave[] = "ERROR_BAD_SLAVE";
const PROGMEM char msg_bad_channel[] = "ERROR_BAD_CHANNEL";
const PROGMEM char msg_bad_state[] = "BAD_STATE";
const PROGMEM char msg_slv_notpresent[] = "SLAVE_NOT_PRESENT";

// Then set up a table to refer to your strings.
const char* const msg_table[] PROGMEM = {IdName, compile_date, msg_bad_slave, msg_bad_channel, msg_bad_state, msg_slv_notpresent};

//ERROR Message index
unsigned int idErrMSG=0;

// Initial configuration
// LOCAL and TRIGGER INT
volatile bool _LOCAL = true;
typedef enum modes {INT, EXT} modes_t;

bool _Start = false;
typedef enum trg_polarity {POS, NEG} trg_pol_t;
typedef enum mx_enabled {ENABLED, DISABLED} mx_ena_t;
typedef enum {DRY=1, NDRY=0} mx_rdy_t;



//Field bits STATUS bytes
struct _status_byte{
  unsigned int REM : 1; //REM=0 / LOC=1
  modes_t TRG : 1; //TRG INT=0 / EXT=1
  trg_pol_t TRG_POL : 1;//TRG POLARITY POS=0 / NEG=1
  mx_ena_t MX_ENA : 1; //multiplexer enable=0 true / 1 false 
  mx_rdy_t RDY : 1; //Multiplexer Armed, ready for trigger

  unsigned int LASTERR : 3; //Last error code
  
} mx_stb; //Status Byte

struct _functional_byte{
  unsigned int SL_BOARDS : 3; //Number of slave boards detected
  unsigned int CH_ENA : 4;  //Number of enabled channels
  unsigned int N_SLAVE: 8; //bit field the bit number indicates the present slave
} mx_fnb; //Functional Byte


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serial Commands Defines and functions
//This should be in another file ??
char serial_command_buffer_[64];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");


//This is the default handler, and gets called when no other command matches. 
void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
  sender->GetSerial()->print("Unrecognized command [");
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");

  mx_stb.LASTERR = CMD_ERR; //Serial Command Error
}


//helper function to channel enable
bool ena_ch(int slv, int channel, char *stat)
{
  int pin = -1;
  bool bEnable = false;
  bool bSuccess = false;

  
  if(!slave[slv].bPresent)
    bSuccess = false;
  else if (channel > CHA_PER_SLAVE || channel <= 0)
    bSuccess = false;
  else if (strcmp(stat, "off") == 0 || strcmp(stat, "OFF") == 0){
    bEnable = false;
    bSuccess = true;
  }
  else if (strcmp(stat, "on") == 0 || strcmp(stat, "ON") == 0){
    bEnable = true;
    bSuccess = true;
  }


  
  if (bSuccess) {   
    if (bEnable){
      slave[slv].CH_CloseTask(channel, true);
      slave[slv].iState[channel] = ON;
    }
    else{
      slave[slv].CH_OpenTask(channel, true);
      slave[slv].iState[channel] = OFF;
    }
    return true;
  }

  
  return false;

  
}

//Channel Enable command function
void cmd_ena_ch(SerialCommands* sender)
{
   // unsigned long timeBegin = micros();
  //

  //Note: Every call to Next moves the pointer to next parameter
  char* slv_str = sender->Next();
  if (slv_str == NULL || strlen(slv_str) != 3)
  {
    strcpy_P(msg, (char *)pgm_read_word(&msg_table[2]));
    sender->GetSerial()->println(msg);
    mx_stb.LASTERR = ENA_ERR;
    idErrMSG=2;
    return;
  }
  int slv = atoi(slv_str+2)-1;
  if(!slave[slv].bPresent)
  {
    strcpy_P(msg, (char *)pgm_read_word(&msg_table[5]));
    sender->GetSerial()->println(msg);
    mx_stb.LASTERR = SLV_ERR;
    idErrMSG=2;
    return;
  }

  
  char* ch_str = sender->Next();
  if (ch_str == NULL || strlen(ch_str) != 3)
  {
    strcpy_P(msg, (char *)pgm_read_word(&msg_table[3]));
    sender->GetSerial()->println(msg);
    mx_stb.LASTERR = ENA_ERR;
    idErrMSG=2;
    return;
  }
  int chId = atoi(ch_str+2);
   if (chId > CHA_PER_SLAVE || chId <= 0)
  {
    strcpy_P(msg, (char *)pgm_read_word(&msg_table[3]));
    sender->GetSerial()->println(msg);
    mx_stb.LASTERR = CH_ERR;
    idErrMSG=1;
    return;
  }
    
  char* stat_str=sender->Next();
  
  if ((stat_str) != NULL)
    if (ena_ch(slv, chId, stat_str))
    {

      //sprintf(msg,"SLV %s %s %s",slv_str, ch_str, stat_str);
      sprintf(msg,"ENA OK");
      sender->GetSerial()->println(msg);
      /*
      sender->GetSerial()->print("SLV ");
      sender->GetSerial()->print(slv_str);
      sender->GetSerial()->print(" ");
      sender->GetSerial()->print(ch_str);
      sender->GetSerial()->print(" ");
      sender->GetSerial()->println(stat_str);
      */

      //output to LCD
      lcd.setCursor(0,3);
      lcd.print(ch_str);
      lcd.setCursor(3,3);
      lcd.print(" ");
      lcd.print(stat_str);
      lcd.print(" ");
    }
    else
    {
      mx_stb.LASTERR = SLV_ERR;
      sender->GetSerial()->print("ERROR ");
      strcpy_P(msg, (char *)pgm_read_word(&msg_table[4]));
      sender->GetSerial()->println(msg);
    }
  else{
      mx_stb.LASTERR = SLV_ERR;
      sender->GetSerial()->print("ERROR ");
      strcpy_P(msg, (char *)pgm_read_word(&msg_table[4]));
      sender->GetSerial()->println(msg);
    
  }


    /*
  unsigned long timeEnd = micros();
  unsigned long totalDuration = timeEnd - timeBegin;
  double duration = (double)totalDuration / 1000.0;

  Serial.print("Total duration: ");
  Serial.print(duration);
  Serial.println(" ms");
  */

}


//helper function to channel GRD enable
bool ena_ch_grd(int slv, int channel, char *stat)
{
  int pin = -1;
  bool bEnable = false;
  bool bSuccess = false;
  state_t state;


  if(!slave[slv].bPresent)
    bSuccess = false;

  else if (strcmp(stat, "off") == 0 || strcmp(stat, "OFF") == 0){
    bEnable = false;
    bSuccess = true;
  }
  else if (strcmp(stat, "on") == 0 || strcmp(stat, "ON") == 0){
    bEnable = true;
    bSuccess = true;
  }


  if (bSuccess) { 
    state = bEnable? ON : OFF;   
    slave[slv].CH_ENAGRD(channel, state);
    
    return true;
  }

  
  return false;

  
}


//Channel GRD Enable command function
void cmd_grd_ena(SerialCommands* sender)
{
  //Note: Every call to Next moves the pointer to next parameter
  char* slv_str = sender->Next();
  if (slv_str == NULL || strlen(slv_str) != 3)
  {
    strcpy_P(msg, (char *)pgm_read_word(&msg_table[2]));
    sender->GetSerial()->println(msg);
    mx_stb.LASTERR = GRD_ERR;
    return;
  }
  int slv = atoi(slv_str+2)-1;
  
  char* ch_str = sender->Next();
  if (ch_str == NULL || strlen(ch_str) != 3)
  {
    strcpy_P(msg, (char *)pgm_read_word(&msg_table[3]));
    sender->GetSerial()->println(msg);
    mx_stb.LASTERR = GRD_ERR;
    return;
  }
  int chId = atoi(ch_str+2);

  char* stat_str = sender->Next();
  if ((stat_str) != NULL)
  {
    if (ena_ch_grd(slv, chId, stat_str))
    {
      //sprintf(msg,"SLV %s CH_GRD %s %s",slv_str, ch_str, stat_str);
      sprintf(msg,"GRD OK");
      sender->GetSerial()->println(msg);
    }
    else
    {
      mx_stb.LASTERR = GRD_ERR;
      sender->GetSerial()->print("ERROR ");
      strcpy_P(msg, (char *)pgm_read_word(&msg_table[4]));
      sender->GetSerial()->println(msg);
    }
  }
  else{
      mx_stb.LASTERR = GRD_ERR;
      sender->GetSerial()->print("ERROR ");
      strcpy_P(msg, (char *)pgm_read_word(&msg_table[4]));
      sender->GetSerial()->println(msg);
    
  }
}



//Multiplexer ID command function
//Return ID Message
void cmd_id(SerialCommands* sender)
{
  strcpy_P(msg, (char *)pgm_read_word(&msg_table[0]));
  strcat_P(msg, (char *)pgm_read_word(&msg_table[1]));
  
  //sender->GetSerial()->println(IdName);
  sender->GetSerial()->println(msg);
}

//
// Open ALL CHANNELS


//OPen all channel Slave version
void open_all_slaves(byte nSlaves)
{
  //OPen all channels signal path and close the ground relay
  int j=0;
  
  //First OPEN all Signal Relays of all present slaves
  for(j=0; j < nSlaves; j++){
    pSlave =  slavesList[j];
    pSlave->CH_ENA(1, OFF);
    pSlave->CH_ENA(2, OFF);
  }
  
  delay(MUX.getdly());

  //The after the settle waiting time CLOSE ALL GND relays
  for(j=0; j < nSlaves; j++){
    pSlave =  slavesList[j];
    pSlave->CH_GND(1, ON);
    pSlave->CH_GND(2, ON);
  }

  delay(MUX.getdly());
  
  return;

}


//Helper funtion to reset the MUX
void rst_helper(void)
{
  uint8_t nSlaves = slavesList.Count();  
  
  open_all_slaves(nSlaves);

  //OPen all GRD
  for(int j=0; j < nSlaves; j++){
    pSlave =  slavesList[j];
    pSlave->CH_ENAGRD(1, OFF);
    pSlave->CH_ENAGRD(2, OFF);
  }

  
  _LOCAL = true;
  setTrigMode(INT);

  InitLCD();

  setDefaultSTB();

  //2023 sequence control
  TRGCounter=0;
  trgWait=0;
  //
  
  idErrMSG=0; //NO ERROR_MSG

}

/*
 * Respond to *RST 
 */
void cmd_rst(SerialCommands* sender)
{
  rst_helper();
  sender->GetSerial()->println("RST DONE");

}

/*
 * Respond to *CLS
 */
void cmd_cls(SerialCommands* sender)
{

  uint8_t nSlaves = slavesList.Count();  
  open_all_slaves(nSlaves);

  //2023 sequence control
  TRGCounter=0;
  trgWait=0;
  //
  
  mx_stb.LASTERR = NO_ERR; //Last error code
  idErrMSG=0; //NO ERROR_MSG
  
  lcd.setCursor(0,3);
  lcd.print(("CLEAR    "));
  sender->GetSerial()->println("CLS DONE");

}

//Helper function for channel STAT command
//Return the channel X status
bool ch_stat(int slv, int channel)
{
  if (slave[slv].iState[channel] == ON)
    return true;
  else
    return false;
}

//Channel STATUS command function
void cmd_ch_stat(SerialCommands* sender)
{

  
  //Note: Every call to Next moves the pointer to next parameter
  char* slv_str = sender->Next();
  if (slv_str == NULL)
  {
    strcpy_P(msg, (char *)pgm_read_word(&msg_table[2]));
    sender->GetSerial()->println(msg);
    return;
  }
  int slv = atoi(slv_str+2)-1;
  

  char* ch_str = sender->Next();
  if (ch_str == NULL)
  {
    strcpy_P(msg, (char *)pgm_read_word(&msg_table[3]));
    sender->GetSerial()->println(msg);
    return;
  }
  int chId = atoi(ch_str+2);

  char* stat_str;
  bool state = ch_stat(slv, chId);

  sender->GetSerial()->print("SLV ");
  sender->GetSerial()->print(slv_str);
  sender->GetSerial()->print(" ");

  //sender->GetSerial()->print("CH ");
  sender->GetSerial()->print(ch_str);
  sender->GetSerial()->print(" ");

  if(state)
    sender->GetSerial()->println("ON");
  else
    sender->GetSerial()->println("OFF");

  

}

//Go To Local command function
//Set the multiplexer to LOCAL control, e.g. the push button are enabled
void cmd_gtl(SerialCommands* sender)
{
  _LOCAL = true;
  mx_stb.REM = _LOCAL;
  
  lcd.setCursor(17,3);
  lcd.print("LOC");

  sender->GetSerial()->println("GTL OK");
  
}

//REM enable command function
//Set the multiplexer to remote control
void cmd_rem(SerialCommands* sender)
{
  _LOCAL = false;
  mx_stb.REM = _LOCAL;

  lcd.setCursor(17,3);
  lcd.print("REM");

  sender->GetSerial()->println("REM OK");
}



// Channel switching sequence commands
// Command ADDSEQ SLx CHx SLy CHy SLz CHz 
unsigned char _nSeq=0; // number of Sequence in memory to run
void cmd_addSequence(SerialCommands* sender)
{

  char* nSeq_str=NULL;

  _nSeq = sequence.getSeqSize();

  nSeq_str = sender->Next();
  
  while( nSeq_str != NULL){
    if(_nSeq == MAX_NO_SEQUENCES){
        mx_stb.LASTERR = SEQ_ERR_MAX;
        sender->GetSerial()->println("MAX SEQ REACHED");
        return;
    }
  
    
    //First Param SLAVE NO
    if(strcmp(nSeq_str,"SL1") == 0){
    
        //SLAVE 1
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[_nSeq].byteOne |= 0b00000001;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[_nSeq].byteOne |= 0b00000010;
        
    }
    else if(strcmp(nSeq_str,"SL2") == 0){
        //SLAVE 2
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[_nSeq].byteOne |= 0b00000100;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[_nSeq].byteOne |= 0b00001000;
    }
    else if(strcmp(nSeq_str,"SL3") == 0){
        //SLAVE 3
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[_nSeq].byteOne |= 0b00010000;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[_nSeq].byteOne |= 0b00100000;
    }
      
    else if(strcmp(nSeq_str,"SL4") == 0){
        //SLAVE 4
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[_nSeq].byteOne |= 0b01000000;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[_nSeq].byteOne |= 0b10000000;
    }
      
    else if(strcmp(nSeq_str,"SL5") == 0){
        //SLAVE 5  
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[_nSeq].byteTwo |= 0b00000001;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[_nSeq].byteTwo |= 0b00000010;
    }
      
    else if(strcmp(nSeq_str,"SL6") == 0){
        //SLAVE 6  
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[_nSeq].byteTwo |= 0b00000100;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[_nSeq].byteTwo |= 0b00001000;
    }

    //Trigger pulses to wait
    else if(strcmp(nSeq_str,"W") == 0){
      nSeq_str = sender->Next();
      
      sequence.SeqBuffer[_nSeq].byteThree = atoi(nSeq_str);
      
    }
    
  nSeq_str = sender->Next();
  
  }

  _nSeq++;
  
  //sprintf(msg, "ADDSEQ %d OK", _nSeq);
  sprintf(msg, "ADDSEQ OK");
  sender->GetSerial()->println(msg);

  sequence.setSeqSize(_nSeq);
  return;
}


bool cmd_edtSequence(SerialCommands* sender)
{
  
  char* nSeq_str=NULL;

  uint8_t iSeq = atoi(sender->Next())-1;

  nSeq_str = sender->Next();
  
  while( nSeq_str != NULL){
    
    //First Param SLAVE NO
    if(strcmp(nSeq_str,"SL1") == 0){
    
        //SLAVE 1
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[iSeq].byteOne |= 0b00000001;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[iSeq].byteOne |= 0b00000010;
        
    }
    else if(strcmp(nSeq_str,"SL2") == 0){
        //SLAVE 2
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[iSeq].byteOne |= 0b00000100;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[iSeq].byteOne |= 0b00001000;
    }
    else if(strcmp(nSeq_str,"SL3") == 0){
        //SLAVE 3
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[iSeq].byteOne |= 0b00010000;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[iSeq].byteOne |= 0b00100000;
    }
      
    else if(strcmp(nSeq_str,"SL4") == 0){
        //SLAVE 4
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[iSeq].byteOne |= 0b01000000;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[iSeq].byteOne |= 0b10000000;
    }
      
    else if(strcmp(nSeq_str,"SL5") == 0){
        //SLAVE 5  
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[iSeq].byteTwo |= 0b00000001;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[iSeq].byteTwo |= 0b00000010;
    }
      
    else if(strcmp(nSeq_str,"SL6") == 0){
        //SLAVE 6  
        //Now Channel
        nSeq_str = sender->Next();
        if(strcmp(nSeq_str,"CH1") == 0)
          sequence.SeqBuffer[iSeq].byteTwo |= 0b00000100;
        else if(strcmp(nSeq_str,"CH2") == 0)
          sequence.SeqBuffer[iSeq].byteTwo |= 0b00001000;
    }

    //Trigger pulses to wait
    else if(strcmp(nSeq_str,"W") == 0){
      nSeq_str = sender->Next();
      
      sequence.SeqBuffer[iSeq].byteThree = atoi(nSeq_str);
      
    }
    
  nSeq_str = sender->Next();
  
  }

  
  //sprintf(msg, "EDTSEQ %d OK", iSeq+1);
  sprintf(msg, "EDTSEQ OK");
  sender->GetSerial()->println(msg);

  
  return true;
}

//Command Get Sequence callback function
//Return the sequence <n> stored in the multiplexer memory
void cmd_getSequence(SerialCommands* sender)
{
  
  char *sSeqToGet = sender->Next();
  unsigned int SeqToGet = (atoi(sSeqToGet)-1);
  String sSeq="";
  char buffer[8]="";

  if(SeqToGet > sequence.getSeqSize()){
    mx_stb.LASTERR = SEQ_ERR_MAX;
    sprintf(msg, "SEQ NO ERR");
    
  }
  else
    sprintf(msg, "SEQ <%d>: b1:%X b2:%X b3:%X",SeqToGet+1,sequence.SeqBuffer[SeqToGet].byteOne,
                             sequence.SeqBuffer[SeqToGet].byteTwo,
                             sequence.SeqBuffer[SeqToGet].byteThree);
                           
    
  sender->GetSerial()->println(msg);
  
  return;
  
}

//Return the stored total number of sequences in the multiplexer
void cmd_getNSeq(SerialCommands* sender)
{
  sender->GetSerial()->println(sequence.getSeqSize());
  return;
  
}


//Delete the last Sequence
//If this function is call recursevelly can be used to delete all sequence
void cmd_delSequence(SerialCommands* sender)
{
   if(_nSeq == 0){
     sender->GetSerial()->println("DELSEQ ERROR");
     mx_stb.LASTERR= SEQ_ERR; //Cero seq reached
   }
   else{
     _nSeq--;
     sequence.setSeqSize(_nSeq);

    sender->GetSerial()->println("LAST SEQ REMOVED");
   }
   return;
  
}

/////////////////////////////////////////////////////////////////
// Helper function to TRG commmand
//Set the Trigger mode configuration
void setTrigMode(modes_t mode)
{
  mx_stb.TRG = mode;
}

//TRG callback function
void cmd_trigger(SerialCommands* sender)
{
  char* tgr_str = sender->Next();

  if (strcmp(tgr_str, "INT") == 0)
    setTrigMode(INT);
  else if (strcmp(tgr_str, "EXT") == 0)
    setTrigMode(EXT);
  else
    sender->GetSerial()->println("ERROR TRIGGER");

  lcd.setCursor(13,3);
  lcd.print(tgr_str);lcd.print(" ");

  sender->GetSerial()->println("TRG OK");
  //sender->GetSerial()->println(tgr_str);
  
}

//Timer functions
//Set the multiplexer internal TIMER

void cmd_timer(SerialCommands* sender)
{
  
  char* time_str = sender->Next();
  _desired_T = atoi(time_str)/1000.0;

  double desired_T = (65536 - (62500*_desired_T/2));

  timer_T =(unsigned int) desired_T;
  
  sender->GetSerial()->println("TIMER OK");

}


//Return the current Trigger setting
void cmd_get_timer(SerialCommands* sender)
{
  sender->GetSerial()->print("TIMER ");
  sender->GetSerial()->print(_desired_T*1000);
  sender->GetSerial()->println(" ms");
}

//Start to RUN the multiplexer Sequence
void cmd_Start(SerialCommands* sender)
{

  _Start = true;

  sender->GetSerial()->println("STARTED");
  lcd.setCursor(0,3);
  lcd.print("STARTED      ");


  return;
  
}

//Stop the multiplexer 
void cmd_Stop(SerialCommands* sender)
{
  _Start = false;
  sender->GetSerial()->println("STOPPED");
  lcd.setCursor(0,3);
  lcd.print("STOPPED      ");

  islaveSeq = 0; //Return to sequence number cero
  //2023 sequence control
  TRGCounter=0;
  trgWait=0;
  //

  return;
  
}

//Pause de multiplexer
void cmd_Pause(SerialCommands* sender)
{
  _Start = false;
  sender->GetSerial()->println("PAUSED");
  lcd.setCursor(0,3);
  lcd.print("PAUSED       ");

  return;
  
}

//Resume the multiplexer after a pause
void cmd_Resume(SerialCommands* sender)
{
  cmd_Start(sender);
  return;
}




//Read the status bytes
void cmd_rd_stb(SerialCommands* sender)
{
  
  sender->GetSerial()->print("STB: [ ");
  sender->GetSerial()->write((byte*)&mx_stb, (int)sizeof(struct _status_byte));
  sender->GetSerial()->println(" ]");

  return;
}

//Set TRG polarity POS or NEG
// SEPT-2022 RJI
//NOTE: The trigger input is fed by an optocupler inverting its polarity
// Thus, POS has to be FALLING 
//        NEG has to be set to RISING
void cmd_trg_pol(SerialCommands* sender)
{
  char* trg_pol = sender->Next();
  
  noInterrupts();
  
  if (strcmp(trg_pol, "POS") == 0){
    detachInterrupt(digitalPinToInterrupt(TRIGGER_PIN));
    mx_stb.TRG_POL = POS;
    attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), triggerChINT, FALLING);  
  }else if(strcmp(trg_pol, "NEG") == 0){
    detachInterrupt(digitalPinToInterrupt(TRIGGER_PIN));
    mx_stb.TRG_POL = NEG;
    attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), triggerChINT, RISING);  
  }else
    sender->GetSerial()->println("TRGPOL ERROR");
  interrupts();
  
  sender->GetSerial()->print("TRGPOL ");
  //sender->GetSerial()->print(trg_pol);
  sender->GetSerial()->println("OK");
  
}

//Load a sequence (binary format)
void cmd_ldseq(SerialCommands* sender)
{

  int nSeq=0; 
  char* chr_seq = sender->Next();

  nSeq=sequence.ReadSequence(atoi(chr_seq));

  sequence.setSeqSize(nSeq);
  _nSeq = nSeq;
  
  sequence.pslavesList=&slavesList;
  
  sender->GetSerial()->println("LDSEQ OK");

  chr_seq = sender->Next();  

  return; 
}

 //Send to Serial port a sequence (binary format)
void cmd_gtseq(SerialCommands* sender)
{
  sender->GetSerial()->println("GTSEQ OK");
  return;
}

 //Store the current sequence into EEPROM
void cmd_stseq(SerialCommands* sender)
{
  //sequence.Store();
  sender->GetSerial()->println("STSEQ OK");
  return;
}

//Recall the last stored sequence into EEPROM to internal memory
void cmd_rlseq(SerialCommands* sender) 
{
  //sequence.Recall();
  //_nSeq = sequence.getSeqSize();
  
  sender->GetSerial()->println("RLSEQ OK");
  return;
}


//Set the delay to wait until open/close a channel
//this configure the ENA_DELAY
//delay in ms
void cmd_setdly(SerialCommands* sender)
{

  unsigned long _dly = atoi(sender->Next());

  MUX.setdly(_dly);
  //sprintf(msg, "DELAY %d ms OK", _dly);
  sender->GetSerial()->println("DELAY OK");
  return;
}

//This function send the current delay
void cmd_getdly(SerialCommands* sender)
{
  unsigned long udly= MUX.getdly();

  sprintf(msg, "DLY %d ms", udly);
  sender->GetSerial()->println(msg);
  return;
}

void cmd_getnSlaves(SerialCommands* sender)
{

  sprintf(msg, "TOTAL SLAVES: %d", mx_fnb.SL_BOARDS);
  sender->GetSerial()->println(msg);
  
  return;
}

void cmd_getwSlaves(SerialCommands* sender)
{
  byte nSlaves = mx_fnb.N_SLAVE;
  sender->GetSerial()->print("SLAVES : ");
  sender->GetSerial()->write((byte*)&nSlaves, (int)sizeof(byte));
  sender->GetSerial()->println("");
  return;
  
}


//DO NOT USE
void cmd_softrst(SerialCommands* sender)
{
  sender->GetSerial()->println("reseting...");

  delay(3000); // A 3 second delay
  
  software_Reset();
}
void software_Reset() // Restarts program from begining but does not reset the peripherals and registers
{

  asm volatile ("  jmp 0");  
}
/**********************************************************************/


//List of commands
//
//Note: Commands are case sensitive
SerialCommand cmd_ena_channel_("ENA", cmd_ena_ch);
SerialCommand cmd_channel_stat_("STAT", cmd_ch_stat);
SerialCommand cmd_id_("*IDN?", cmd_id);
SerialCommand cmd_reset_("*RST",cmd_rst);
SerialCommand cmd_clear_("*CLS", cmd_cls);
SerialCommand cmd_gtl_("GTL",cmd_gtl);
SerialCommand cmd_rem_("REM",cmd_rem);
SerialCommand cmd_trigger_("TRG", cmd_trigger);
SerialCommand cmd_timer_("TIMER", cmd_timer);
SerialCommand cmd_get_timer_("TIMER?", cmd_get_timer);
SerialCommand cmd_addSequence_("ADDSEQ", cmd_addSequence);
SerialCommand cmd_edtSequence_("EDTSEQ", cmd_edtSequence);
SerialCommand cmd_getSequence_("SEQ?", cmd_getSequence);
SerialCommand cmd_getNSeq_("NSEQ?", cmd_getNSeq);
SerialCommand cmd_delSequence_("DELSEQ", cmd_delSequence);
SerialCommand cmd_Start_("START", cmd_Start);
SerialCommand cmd_Stop_("STOP", cmd_Stop);
SerialCommand cmd_Resume_("RESUME", cmd_Resume);
SerialCommand cmd_Pause_("PAUSE", cmd_Pause);
SerialCommand cmd_STB_("*STB?", cmd_rd_stb);
SerialCommand cmd_trg_pol_("TRGPOL", cmd_trg_pol); //Set trigger polarity
SerialCommand cmd_grd_ena_("GRD", cmd_grd_ena);//ENABLE/DISABLE GRD Channel connection
SerialCommand cmd_ldseq_("LDSEQ", cmd_ldseq); //Load a sequence (binary format)
SerialCommand cmd_gtseq_("GTSEQ", cmd_gtseq); //Send to Serial port a sequence (binary format)
SerialCommand cmd_stseq_("STSEQ", cmd_stseq); //Store the current sequence into EEPROM
SerialCommand cmd_rlseq_("RLSEQ", cmd_rlseq); //Recall the last stored sequence into EEPROM to internal memory
SerialCommand cmd_setdly_("DELAY", cmd_setdly); //Configure the delay to wait until open/close a channel ->ENA_DELAY
SerialCommand cmd_getdly_("DELAY?", cmd_getdly); //Get the current delay

SerialCommand cmd_nslaves_("NSLAVES?", cmd_getnSlaves); //Returns the number of slaves present
SerialCommand cmd_wslaves_("WSLAVES?", cmd_getwSlaves); //Returns which slaves number are present


SerialCommand cmd_softRST_("reset", cmd_softrst);


/*******************************************************/




void LCDClear()
{
  lcd.clear();
  lcd.setCursor (0,0); //
  lcd.print(IdName);

}

/////////////////////////////////////////////////////
// Tasks to ENABLE Multiplexer channels

////////////////////////////////////////////////////////////////////////
/*
 * Default SEQUENCE to be Used with the button switch in manual mode
 * */
void setTaskList() 
{
 
  for(int j=0; j < MAXBOARDS; j++){
    if(slave[j].bPresent){
       slavesList.Add(&slave[j]);
    }
  }
 
}

/******* */


void  setDefaultSTB()
{
  mx_stb.REM = _LOCAL;
  mx_stb.TRG = INT;
  mx_stb.TRG_POL = POS;
  mx_stb.MX_ENA = ENABLED;
  mx_stb.RDY = DRY;
  mx_stb.LASTERR = NO_ERR; //Last error code
  
}

///////////////////////////////////////////////////////////////////////
// LCD Init
void InitLCD(void)
{
  // initialize the LCD, 
  lcd.init();
  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.clear();
  lcd.setCursor (0,0); //

  strcpy_P(msg, (char *)pgm_read_word(&msg_table[0]));
  lcd.print(msg);
  
  lcd.setCursor(0,1);
  lcd.print("Ready");

  lcd.setCursor(7,1);
  lcd.print("Slaves:");
  lcd.setCursor(14,1);
  
  
  for(int j=0; j < MAXBOARDS; j++){
    if(slave[j].bPresent)
      lcd.print(j+1);
      
    else
      lcd.print(" ");
  }
  

  lcd.setCursor(17,3);
  lcd.print("LOC");

  lcd.setCursor(13,3);
  lcd.print("INT");

  


}


//////////////////////////////////////////////////////////////////
// Serial commands 

void addSerialCommands (void)
{
    //Serial commando to start the shell  
  serial_commands_.SetDefaultHandler(cmd_unrecognized);
  serial_commands_.AddCommand(&cmd_ena_channel_);
  serial_commands_.AddCommand(&cmd_channel_stat_);
  serial_commands_.AddCommand(&cmd_id_);
  serial_commands_.AddCommand(&cmd_reset_);
  serial_commands_.AddCommand(&cmd_clear_);
  serial_commands_.AddCommand(&cmd_gtl_);
  serial_commands_.AddCommand(&cmd_rem_);
  serial_commands_.AddCommand(&cmd_trigger_);
  serial_commands_.AddCommand(&cmd_timer_);
  serial_commands_.AddCommand(&cmd_get_timer_);
  
  serial_commands_.AddCommand(&cmd_addSequence_);
  serial_commands_.AddCommand(&cmd_edtSequence_);
  serial_commands_.AddCommand(&cmd_getSequence_);
  serial_commands_.AddCommand(&cmd_getNSeq_);
  serial_commands_.AddCommand(&cmd_delSequence_);
  
  serial_commands_.AddCommand(&cmd_Start_);
  serial_commands_.AddCommand(&cmd_Stop_);
  serial_commands_.AddCommand(&cmd_STB_);
  serial_commands_.AddCommand(&cmd_trg_pol_);
  serial_commands_.AddCommand(&cmd_grd_ena_);

  serial_commands_.AddCommand(&cmd_Resume_);
  serial_commands_.AddCommand(&cmd_Pause_);


  serial_commands_.AddCommand(&cmd_ldseq_); //Load a sequence (binary format)
  serial_commands_.AddCommand(&cmd_gtseq_); //Send to Serial port a sequence (binary format)
  serial_commands_.AddCommand(&cmd_stseq_); //Store the current sequence into EEPROM
  serial_commands_.AddCommand(&cmd_rlseq_); //Recall the last stored sequence into EEPROM to internal memory

  serial_commands_.AddCommand(&cmd_setdly_); //Configure the delay to wait until open/close a channel ->ENA_DELAY
  serial_commands_.AddCommand(&cmd_getdly_); //Query for the current delay

  serial_commands_.AddCommand(&cmd_nslaves_); //Returns the number of slaves present
  serial_commands_.AddCommand(&cmd_wslaves_); //Returns the slave number present


  serial_commands_.AddCommand(&cmd_softRST_); //uncomment function to reset the microcontroller
  
}


/*RUN SEQUENCE when an external Trigger*/

//TRG INT Service request function

void triggerChINT()
{
  if(_Start && mx_stb.TRG == EXT){
      extTRG=true;
      if(trgWait !=0 )  TRGCounter++;
  }
}


//RUN Sequence Slave version

void runSlaveSequence(void)
{

  
  uint8_t nSlv=slavesList.Count();
  bool bWait=true;
  mx_stb.RDY = NDRY;
  
  if(TRGCounter == trgWait){
    open_all_slaves(nSlv);
  

    sequence.openGND(islaveSeq);
    delay(MUX.getdly());
    
    sequence.closeCH(islaveSeq);  
    

    //Get the waiting time
    trgWait = sequence.SeqBuffer[islaveSeq].byteThree;
    islaveSeq++;
    if (islaveSeq == sequence.getSeqSize())
        islaveSeq = 0;


    TRGCounter = 0;
  }
  
  mx_stb.RDY = DRY;
  return;
  
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Buttons rutines

#define CUSTOM_DEBOUNCE_DELAY   50

// Time the library waits for a second (or more) clicks
// Set to 0 to disable double clicks but get a faster response
#define CUSTOM_REPEAT_DELAY     500

int nSlave=0;
void changeChannel(void)
{
  uint8_t k=slavesList.Count();
    
  if(_nSeq != 0){
    open_all_slaves(k);
        
    pSlave =  slavesList[nSlave];
    
    sequence.setSequenceTask(iSeqManual++, trgWait);  
      
          
    iSeqManual %= sequence.getSeqSize();
  }
    
}

void enaCH(uint8_t pin, uint8_t event, uint8_t count, uint16_t length) 
{
    
  if (_LOCAL)
    if(event == EVENT_RELEASED){ 
      
      lcd.setCursor(0,3);
      
      sprintf(msg,"SEQ: %2d     ",iSeqManual+1);
      lcd.print(msg);
      changeChannel();
  }

}

void toggleLOCAL(uint8_t pin, uint8_t event, uint8_t count, uint16_t length)
{
  static uint8_t SeqToGet=0;

  if(event == EVENT_RELEASED && length > 3000){
      _LOCAL = !_LOCAL;
      mx_stb.REM = _LOCAL;

      lcd.setCursor(17,3);
      if(_LOCAL)
        lcd.print("LOC");
      else
        lcd.print("REM");
        
  }
  else if(event == EVENT_RELEASED && length < 3000){
      sprintf(msg, "%2d: %2X %2X %2X",SeqToGet+1,sequence.SeqBuffer[SeqToGet].byteOne,
                           sequence.SeqBuffer[SeqToGet].byteTwo,
                           sequence.SeqBuffer[SeqToGet].byteThree);
                           
    
      lcd.setCursor(0,3);
      lcd.print(msg);
      SeqToGet++;
      SeqToGet %= sequence.getSeqSize();
  }
}

void ResetCH(uint8_t pin, uint8_t event, uint8_t count, uint16_t length)
{
  
    if(event == EVENT_RELEASED){
        
        //open_all_slaves(nSlaves);
        rst_helper();
        iSeqManual=0;
        
        lcd.setCursor(0,3);
        lcd.print("RESET     ");

    }
}

DebounceEvent buttonCH = DebounceEvent(BUTTON_PIN, enaCH, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP,CUSTOM_DEBOUNCE_DELAY, CUSTOM_REPEAT_DELAY);
DebounceEvent buttonLOCAL = DebounceEvent(RENLOCAL_PIN, toggleLOCAL, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP,CUSTOM_DEBOUNCE_DELAY, CUSTOM_REPEAT_DELAY);
DebounceEvent buttonRST = DebounceEvent(RESET_PIN, ResetCH, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP,CUSTOM_DEBOUNCE_DELAY, CUSTOM_REPEAT_DELAY);



/////////////////////////////
// Mux Init function
//Search for the slaves board connected
//Initialize variables and power-on conditions


void MuxInitialize(void)
{
  unsigned j=0, nslaves=0;
  //Board detection pins configuration as input
  for( j=0; j < MAXBOARDS; j++){
    
    slave[j].setEEAddr((byte)j);

    slave[j].setEnaDelay(ENA_DELAY);
         
    if(digitalRead(bPins[j]) == LOW){
      slave[j].bPresent = true;
      
      switch(j){
        case 0:
          mx_fnb.N_SLAVE |= 0b00000001;
          break;
        case 1: 
          mx_fnb.N_SLAVE |= 0b00000010;
          break;
        case 2:
          mx_fnb.N_SLAVE |= 0b00000100;
          break;
        case 3:
          mx_fnb.N_SLAVE |= 0b00001000;
          break;

        case 4:
          mx_fnb.N_SLAVE |= 0b00010000;
          break;

        case 5:
          mx_fnb.N_SLAVE |= 0b00100000;
          break;

        default:
          mx_fnb.N_SLAVE |= 0b00000000;
          break;
          
      }

      nslaves++;
      
      slave[j].setCH1iPin(SLPins[j][0]);
      slave[j].setCH1gPin(SLPins[j][2]);
      slave[j].setCH1grdPin(SLPins[j][4]);
      
      slave[j].setCH2iPin(SLPins[j][1]);
      slave[j].setCH2gPin(SLPins[j][3]);
      slave[j].setCH2grdPin(SLPins[j][5]);
    }
  }

  mx_fnb.SL_BOARDS = nslaves; //Number of slave boards detected
  mx_fnb.CH_ENA = nslaves*2;  //Number of enabled channels

  return;
}

/////////////////////////
//This is a default sequence just to test

void defSequenceInit(void)
{
  
  sequence.SeqBuffer[0].byteOne = 0b00000001;
  sequence.SeqBuffer[0].byteTwo = 0b00000000;
  sequence.SeqBuffer[0].byteThree = 0b00000001;

  sequence.SeqBuffer[1].byteOne = 0b00000010;
  sequence.SeqBuffer[1].byteTwo = 0b00000000;
  sequence.SeqBuffer[1].byteThree = 0b00000001;

  sequence.SeqBuffer[2].byteOne = 0b00000100;
  sequence.SeqBuffer[2].byteTwo = 0b00000000;
  sequence.SeqBuffer[2].byteThree = 0b00000001;

  sequence.SeqBuffer[3].byteOne = 0b00001000;
  sequence.SeqBuffer[3].byteTwo = 0b00000000;
  sequence.SeqBuffer[3].byteThree = 0b00000001;

  sequence.setSeqSize(4);
  _nSeq = 4;

  
  sequence.pslavesList=&slavesList;
  sequence.setSlavesCnt(slavesList.Count());
  
  return;
}

void  SequenceInit()
{

  sequence.setSeqSize(_nSeq);
    
  sequence.pslavesList=&slavesList;
  sequence.setSlavesCnt(slavesList.Count());
  
  return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino Inital SETUP after Power UP or Reset
void setup()
{

   
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //INIT trigger output pin
  pinMode(OUTPUTTRG_PIN, OUTPUT);
  digitalWrite(OUTPUTTRG_PIN, LOW);
  
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = 65536-62500*(timer_T);  // preload timer 65536-16MHz/256/1Hz
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
  /*********************************************************/

  int i = 0;

  for (i = 0; i < MAXBOARDS; i++) {
    for(int j=0; j < CHA_PER_SLAVE*3; j++){
      pinMode(SLPins[i][j], OUTPUT);
      digitalWrite(SLPins[i][j], LOW);
    }
  }

  //Board detection pins configuration as input
  for(int j=0; j < MAXBOARDS; j++){
    slave[j].ID = j+1;
    pinMode(bPins[j], INPUT_PULLUP);
    slave[j].bPresent = false;
    
  }
  
  // Pin to buttons init
  for ( i=0; i<nrButtons; i++)
    pinMode(buttons[i], INPUT_PULLUP);

    
  //Serial commands configuration
  addSerialCommands();


  //Serial.println("Ready!");

  _LOCAL = true;
  setTrigMode(INT);

  setDefaultSTB();
  mx_fnb.N_SLAVE = 0b00000000;
  
  // TRG IN INTERRUPT 
  attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), triggerChINT, RISING);

  
  mx_fnb.SL_BOARDS = 2; //Number of slave boards detected
  mx_fnb.CH_ENA = 3;  //Number of enabled channels

  MuxInitialize();


  setTaskList(); //Default Sequence for manual operation
  InitLCD();

#ifdef _DEBUG_
  defSequenceInit(); //Defaul Sequence initialization for DEBUG popurse
#else
  SequenceInit();
#endif

}
//Helper function to run the sequences using the internal TIMER ONLY for INTI or debugging proporses
volatile bool intTRG=false;
void runIntTrg(void)
{
    if(_Start && mx_stb.TRG == INT){
      intTRG=true;
      if(trgWait !=0 )  TRGCounter++;
  }

}

//Timer Interrupt service rutine
ISR(TIMER1_OVF_vect)        // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  TCNT1 = (timer_T);            // preload timer
  digitalWrite(OUTPUTTRG_PIN, digitalRead(OUTPUTTRG_PIN) ^ 1);
  runIntTrg();
}


//Main LOOP
void loop()
{

  
  if (_LOCAL){
    buttonCH.loop();
    buttonRST.loop();
  }   
  
  

  buttonLOCAL.loop();
  

    
  serial_commands_.ReadSerial();

  if(mx_stb.RDY == DRY){
    if(mx_stb.TRG == EXT){
      if(extTRG){
        runSlaveSequence();
        extTRG=false;
      }
    }
    else{
      if(intTRG){
      runSlaveSequence();
      intTRG=false;
      }
    }
  }

    
 
}
