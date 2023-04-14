//Multiplexer Common defines
//Pins

#ifndef _MUXDEFS_H
#define _MUXDEFS_H

#include <avr/pgmspace.h>

//Display LCD I2C
//LiquidCrystal_I2C lcd(0x27, 20, 4);
#define LCDADD  0x27
#define DSPCOLS   20
#define DSPROWS    4

#define NROBUTTONS 4

#define MAXBOARDS 6
#define CHA_PER_SLAVE 2

// Sequences
#define MAX_NO_SEQUENCES 32

#define OUTPUTTRG_PIN 10 //25//CHANGE To PIN 10 BEFORE DELIVER --> WHEN READY // PIN 25 IS FOR DEVELOP  at HOME


//Conditions
#define FALSE 0
#define TRUE  1


//ERROR CODES
#define NO_ERR      0b000
#define CMD_ERR     0b001
#define SEQ_ERR     0b010
#define SEQ_ERR_MAX 0b011
#define ENA_ERR     0b100
#define SLV_ERR     0b101
#define GRD_ERR     0b110
#define CH_ERR      0b111



//#define MASTER_V1   //Uncomment to compile with the first version of the MASTER PCB - NOT DELIVERED
#define MASTER_V2  //Uncomment to compile with the right PIN ASSIGMENT DEFINITION


#ifdef MASTER_V1

  //PINS DEFINES for MASTER_V2_BOARD
  #define TRIGGER_PIN   3
  #define BUTTON_PIN    30
  #define RENLOCAL_PIN  29
  #define RESET_PIN     2
  
  
  //SLAVES PINS
  //Channels PIN defines
  //SLAVE 01//DONE
  #define CH1_ENA 6
  #define CH2_ENA 5
  #define CH1_GND 8
  #define CH2_GND 4
  #define CH1_GRD 7
  #define CH2_GRD 9

  //SLAVE 02 -- TO BE CHANGED PIn 13 IS USED INTERNALLY
  #define CH3_ENA 14
  #define CH4_ENA 13
  #define CH3_GND 16
  #define CH4_GND 12
  #define CH3_GRD 15
  #define CH4_GRD 17
  
  
  
  //SLAVE 03 //DONE
  #define CH5_ENA 24
  #define CH6_ENA 23
  #define CH5_GND 26 
  #define CH6_GND 22
  #define CH5_GRD 25
  #define CH6_GRD 27
  
  
  //SLAVE 04 //DONE
  #define CH7_ENA 34
  #define CH8_ENA 33
  #define CH7_GND 36 
  #define CH8_GND 32
  #define CH7_GRD 35
  #define CH8_GRD 37
  
  
  //SLAVE 05 //DONE
  #define CH9_ENA   42
  #define CH10_ENA  39
  #define CH9_GND   44
  #define CH10_GND  40
  #define CH9_GRD   41
  #define CH10_GRD  43
  
  
  //SLAVE 06 //DONE
  #define CH11_ENA 50
  #define CH12_ENA 45
  #define CH11_GND 52
  #define CH12_GND 47
  #define CH11_GRD 48
  #define CH12_GRD 49
  
  
  //SCAN PIN to detect for board present
  #define BRD_1 10
  #define BRD_2 18
  #define BRD_3 28
  #define BRD_4 38
  #define BRD_5 46
  #define BRD_6 51
  

#else //MASTER_V2
  ////////////////////////////////////////////////////////////////////////////////////////

  //PINS DEFINES for MASTER_V2_BOARD
  #define TRIGGER_PIN   3
  #define BUTTON_PIN    30
  #define RENLOCAL_PIN  29
  #define RESET_PIN     2
  
  
  //SLAVES PINS
  //Channels PIN defines
  //SLAVE 01//DONE
  #define CH1_ENA 6
  #define CH2_ENA 5
  #define CH1_GND 8
  #define CH2_GND 4
  #define CH1_GRD 7
  #define CH2_GRD 9
  
  
  //SLAVE 02 //DONE
  #define CH3_ENA 17
  #define CH4_ENA 12
  #define CH3_GND 19
  #define CH4_GND 15
  #define CH3_GRD 14
  #define CH4_GRD 16
  
  
  
  //SLAVE 03 //DONE
  #define CH5_ENA 24
  #define CH6_ENA 23
  #define CH5_GND 26 
  #define CH6_GND 22
  #define CH5_GRD 25
  #define CH6_GRD 27
  
  
  //SLAVE 04 //DONE
  #define CH7_ENA 34
  #define CH8_ENA 33
  #define CH7_GND 36 
  #define CH8_GND 32
  #define CH7_GRD 35
  #define CH8_GRD 37
  
  
  //SLAVE 05 //DONE
  #define CH9_ENA   42
  #define CH10_ENA  39
  #define CH9_GND   44
  #define CH10_GND  40
  #define CH9_GRD   41
  #define CH10_GRD  43
  
  
  //SLAVE 06 //DONE
  #define CH11_ENA 50
  #define CH12_ENA 45
  #define CH11_GND 52
  #define CH12_GND 47
  #define CH11_GRD 48
  #define CH12_GRD 49
  
  
  //SCAN PIN to detect for board present
  #define BRD_1 10
  #define BRD_2 18
  #define BRD_3 28
  #define BRD_4 38
  #define BRD_5 46
  #define BRD_6 51
  
//#else

//  #error "NO MASTER PCB DEFINED!!"

#endif

#define ENA_DELAY 2

#endif
