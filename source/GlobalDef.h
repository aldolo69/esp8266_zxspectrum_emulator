/*
  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  #########################################################################
*/

//look for these lines:
//#define ILI9341_DRIVER
//#define TFT_DC 4
//#define TFT_CS 8
//#define TFT_RST 3




#define SD3        10   //nodemcu available input

#define TFT_CS     D1    
//#define TFT_CS     D8    
#define SPEAKER_CS D0   //directly connected to speaker
#define SD_CS      D2
#define KEYB_CS    D3


//defined here only to remember them. they are defined in tft lib
#define TFT_DC     D4
#define TFT_RST    SD3
#define SPI_CLOCK  D5
#define SPI_MOSI   D7
#define SPI_MISO   D6



#define SPI_SPEED_SD    1000000UL
#define SPI_SPEED_TFT   27000000UL 
#define SPI_SPEED_KEYB  200000UL

/*
  d5 CLK SPI 
  d6 MISO SPI (input) 
  d7 MOSI SPI (output) 

  DISPLAY connections
  d4 Data/Command  (output) 
  d5 clk 
  d6 MISO (input) 
  d7 MOSI (output)  
  d1 cs 
  d3 reset

for compatibility with the sd card lib the d8 pin is not used. 
it is usually used by display and sdcard lib-->incompatibility.
better to drive the right cs pins before using sd or tft lib
  
*/

#define ZXSCREENSIZE (32*192+32*24)

#define ZXDISPLAYUSEINTERRUPT  // undefine to disable interrupt routine. debug purpose. usually defined
#define ZXKEYBOARDFILLBUFFER  // undefine to keep clear the emulator keyb buffer. debug purpose. usually defined
//#define BORDERCOLORCHANGE1HZ  //define to cycle border color. debug purpose. usually undefined
#define ZXKEYBOARDENABLED //undefine to disable spi for the keyboard. usually defined
#define NOGAMESROM //undefine to add sample games


//task performed in loop()
#define EMUTASK_EMULATOR 0
#define EMUTASK_MENU 1
#define EMUTASK_SD 2
#define EMUTASK_EPROM 3
#define EMUTASK_KEYB 4
#define EMUTASK_JOY 5
 


#define DEBUG_PRINT //undefine to remove debug code

#ifdef DEBUG_PRINT
 #define DEBUG_PRINTLN(x)  Serial.println(x)
 #define DEBUG_PRINT(x)  Serial.print(x)
#else
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINT(x)
#endif




//bit used by keyboard key in zx spectrum
#define BUTTON_6 16
#define BUTTON_7 8
#define BUTTON_8 4
#define BUTTON_9 2
#define BUTTON_0 1
#define BUTTON_Y 16
#define BUTTON_U 8
#define BUTTON_I 4
#define BUTTON_O 2
#define BUTTON_P 1
#define BUTTON_H 16
#define BUTTON_J 8
#define BUTTON_K 4
#define BUTTON_L 2
#define BUTTON_EN 1
#define BUTTON_B 16
#define BUTTON_N 8
#define BUTTON_M 4
#define BUTTON_SS 2
#define BUTTON_SP 1



#define BUTTON_1 1
#define BUTTON_2 2
#define BUTTON_3 4
#define BUTTON_4 8
#define BUTTON_5 16
#define BUTTON_Q 1
#define BUTTON_W 2
#define BUTTON_E 4
#define BUTTON_R 8
#define BUTTON_T 16
#define BUTTON_A 1
#define BUTTON_S 2
#define BUTTON_D 4
#define BUTTON_F 8
#define BUTTON_G 16
#define BUTTON_CS 1
#define BUTTON_Z 2
#define BUTTON_X 4
#define BUTTON_C 8
#define BUTTON_V 16



//kemston joystick
#define BUTTON_LEFT 2
#define BUTTON_RIGHT 1
#define BUTTON_UP 8
#define BUTTON_DOWN 4
#define BUTTON_FIRE2 32
#define BUTTON_FIRE 16
#define BUTTON_ESC 64


#define CPUDELAYSLOWEST 500000
#define CPUDELAYSLOWER 100000
#define CPUDELAYNORMAL 50000
#define CPUDELAYFASTER 10000
#define CPUDELAYFASTEST 1
