
/*
   acknowledge of others valuable works:

   Marat Fayzullin for z80 cpu emulation

   https://mikrocontroller.bplaced.net/wordpress/?page_id=756

   https://github.com/uli/Arduino_nowifi

   https://github.com/greiman/SdFat

*/




#include <Arduino.h>
#include "Zxdisplay.h"
#include "Zxsound.h"
#include "Zxkeyboard.h"
#include "GlobalDef.h"
#include "ShowKeyboard.h"
#include "z80.h"
#include "SpiSwitch.h"
#include "SdNavigation.h"




unsigned char KEY[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
unsigned char KEMPSTONJOYSTICK = 0x00;
//0..7 used for zx spectrum keyboard emulation
//[8] used for kempston joystik and special keys
//0=right
//1=left
//2=down
//3=up
//4=button2
//5=button1
//6=ESC key
//7=

//to simulate keyboard with 6 joy keys
unsigned char *pJoyKeyAdd[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
unsigned char  pJoyKeyVal[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
//up down left right f1 f2


unsigned char RAM[RAMSIZE];//48k
unsigned char CACHE[ZXSCREENSIZE]; //used for video backup and file decoding
const unsigned char ROM[ROMSIZE] PROGMEM = {
#include "zxrom.h" //original rom
};

Z80 state;//emulator status
int z80DelayCycle = 1;

char zxInterruptPending = 0;//enabled by interrupt routine
int timerfreq = 50; //default
unsigned char soundenabled = 1; //default sound on


void ICACHE_FLASH_ATTR stampabinario(unsigned char c);


void setup() {
  //SET 160MHZ
  REG_SET_BIT(0x3ff00014, BIT(0));
  ets_update_cpu_frequency(160);

  spiSwitchSetup();

  sdNavigationSetup();
  zxDisplaySetup(RAM);
  zxKeyboardSetup();
  zxSoundSetup();

#ifdef DEBUG_PRINT
  Serial.begin(115200);
#endif.

  ResetZ80(&state);


}
void waitforclearkeyb()
{
  for (;;)
  {
    yield();
    if (
      KEY[0] == 0xff &&
      KEY[1] == 0xff &&
      KEY[2] == 0xff &&
      KEY[3] == 0xff &&
      KEY[4] == 0xff &&
      KEY[5] == 0xff &&
      KEY[6] == 0xff &&
      KEY[7] == 0xff &&
      KEMPSTONJOYSTICK == 0) return;
  }
}

void getKeyb(unsigned char **p, unsigned char *v)
{
  for (;;)
  {
    yield();
    for (int i = 0; i < 8; i++)
    {
      if (KEY[i] != 0xff)
      {
        *p = &KEY[i];
        *v = KEY[i];
        return;
      }
    }
  }
}


boolean checkKeyBit( unsigned char *KEY, unsigned char button)
{
  if (!((*KEY) & button) ) // key pressed
  {
    *KEY =  0xff;
    delay(100);
    return true;//back to old program
  }
  return false;
}




int showJoystick()
{
  static char cStep = 0;

  switch (cStep)
  { case 0:
      pJoyKeyVal[0] = pJoyKeyVal[1] = pJoyKeyVal[2] = pJoyKeyVal[3] = pJoyKeyVal[4] = pJoyKeyVal[5] = 0xff;
      cStep++;
      sdNavigationCls(COLOR_TEXT);
      waitforclearkeyb();
      return 0;
    case 1:
      cStep++;
      sdNavigationPrintFStrBig(0, 0, F("Press UP   "), COLOR_TEXT);
      getKeyb(&pJoyKeyAdd[0], &pJoyKeyVal[0]);
      waitforclearkeyb();
      return 0;
    case 2:
      cStep++;
      sdNavigationPrintFStrBig(0, 0, F("Press DOWN "), COLOR_TEXT);
      getKeyb(&pJoyKeyAdd[1], &pJoyKeyVal[1]);
      waitforclearkeyb();
      return 0;
    case 3:
      cStep++;
      sdNavigationPrintFStrBig(0, 0, F("Press LEFT "), COLOR_TEXT);
      getKeyb(&pJoyKeyAdd[2], &pJoyKeyVal[2]);
      waitforclearkeyb();
      return 0;
    case 4:
      cStep++;
      sdNavigationPrintFStrBig(0, 0, F("Press RIGHT"), COLOR_TEXT);
      getKeyb(&pJoyKeyAdd[3], &pJoyKeyVal[3]);
      waitforclearkeyb();
      return 0;
    case 5:
      cStep++;
      sdNavigationPrintFStrBig(0, 0, F("Press FIRE1"), COLOR_TEXT);
      getKeyb(&pJoyKeyAdd[4], &pJoyKeyVal[4]);
      waitforclearkeyb();
      return 0;
    case 6:
      cStep = 0;
      sdNavigationPrintFStrBig(0, 0, F("Press FIRE2"), COLOR_TEXT);
      getKeyb(&pJoyKeyAdd[5], &pJoyKeyVal[5]);
      waitforclearkeyb();
      return -1;
  }


}








//return 0 to do nothing
//1..n to select an option
//-1 to return to program
int showMenu()
{
  static boolean bdisplayedmenu = false;

  if (!bdisplayedmenu)
  {
    bdisplayedmenu = true;
    sdNavigationCls(COLOR_TEXT);
    sdNavigationPrintFStrBig(0, 0, F("0 back"), COLOR_TEXT);
    sdNavigationPrintFStrBig(0, 2, F("1 SD list"), COLOR_TEXT);
    sdNavigationPrintFStrBig(0, 4, F("2 EEPROM list"), COLOR_TEXT);
    sdNavigationPrintFStrBig(0, 6, F("3 ZX keyboard"), COLOR_TEXT);
    sdNavigationPrintFStrBig(0, 8, F("4 CPU"), COLOR_TEXT);
    switch (z80DelayCycle)
    { case CPUDELAYSLOWEST:// 500
        sdNavigationPrintFStrBig(12, 8, F("SLOWEST"), COLOR_TEXT);
        break;
      case CPUDELAYSLOWER:// 1000
        sdNavigationPrintFStrBig(12, 8, F("SLOWER "), COLOR_TEXT);
        break;
      case CPUDELAYNORMAL:// 5000
        sdNavigationPrintFStrBig(12, 8, F("NORMAL "), COLOR_TEXT);
        break;
      case CPUDELAYFASTER:// 5000
        sdNavigationPrintFStrBig(12, 8, F("FASTER "), COLOR_TEXT);
        break;
      case CPUDELAYFASTEST:// 50000
        sdNavigationPrintFStrBig(12, 8, F("FASTEST"), COLOR_TEXT);
        break;
    }



    sdNavigationPrintFStrBig(0, 10, F("5 Timer ..Hz"), COLOR_TEXT);
    sdNavigationPrintChBig(16, 10, '0' + (timerfreq / 10), COLOR_TEXT);
    sdNavigationPrintChBig(18, 10, '0' + (timerfreq % 10), COLOR_TEXT);

    sdNavigationPrintFStrBig(0, 12, F("6 Sound"), COLOR_TEXT);
    if (soundenabled)
    {
      sdNavigationPrintFStrBig(16, 12, F("ON"), COLOR_TEXT);
    }
    else
    {
      sdNavigationPrintFStrBig(16, 12, F("OFF"), COLOR_TEXT);
    }
    sdNavigationPrintFStrBig(0, 14, F("7 Joystik setup"), COLOR_TEXT);


    //before returning wait for an empity kayb buffer
    waitforclearkeyb();
    return 0;
  }

  if (   checkKeyBit(&(KEY[4]),  BUTTON_0)
         || (KEMPSTONJOYSTICK && BUTTON_LEFT) ) //'0' key or left
  {
    bdisplayedmenu = false;
    return -1;//back to old program
  }

  if (   checkKeyBit(&(KEY[4]),  BUTTON_6) ) //'6'
  {
    soundenabled ^= 1;
    bdisplayedmenu = false;
    return 0;
  }




  if (checkKeyBit(&(KEY[3]),  BUTTON_1) ) //'1'
  {
    bdisplayedmenu = false;
    return EMUTASK_SD;//sd upload
  }
  if (checkKeyBit(&(KEY[3]),  BUTTON_2) ) //'2'
  {
    bdisplayedmenu = false;
    return EMUTASK_EPROM;//eprom upload
  }
  if (checkKeyBit(&(KEY[3]),  BUTTON_3) ) //'3'
  {
    bdisplayedmenu = false;
    return EMUTASK_KEYB;//keyboard
  }

  if (checkKeyBit(&(KEY[4]),  BUTTON_7) ) //'3'
  {
    bdisplayedmenu = false;
    return EMUTASK_JOY;//setup joystick
  }




  if (checkKeyBit(&(KEY[3]),  BUTTON_4) ) //'4'
  {
    bdisplayedmenu = false;

    switch (z80DelayCycle) {
      case CPUDELAYSLOWEST:
        z80DelayCycle  = CPUDELAYSLOWER;
        break;
      case CPUDELAYSLOWER:
        z80DelayCycle  = CPUDELAYNORMAL;
        break;
      case CPUDELAYNORMAL:
        z80DelayCycle  = CPUDELAYFASTER;
        break;
      case CPUDELAYFASTER:
        z80DelayCycle  = CPUDELAYFASTEST;
        break;
      case CPUDELAYFASTEST:
        z80DelayCycle  = CPUDELAYSLOWEST;
        break;
    }


    return 0;
  }


  if (checkKeyBit(&(KEY[3]),  BUTTON_5) ) //'4'
  {
    switch (timerfreq)
    {
      case 25:
        timerfreq = 38;
        break;
      case 38:
        timerfreq = 50;
        break;
      case 50:
        timerfreq = 62;
        break;
      case 62:
        timerfreq = 75;
        break;
      case 75:
        timerfreq = 25;
        break;
    }
    bdisplayedmenu = false;
    return 0;
  }





  return 0;
}




/*
  int showFreqMenu()
  {
  static boolean bdisplayedmenu = false;
  if (!bdisplayedmenu)
  {
    bdisplayedmenu = true;
    sdNavigationCls(COLOR_TEXT);
    sdNavigationPrintFStr(5, 2,  F("0 back to emulator"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 3,  F("1 Fastest----"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 4,  F("2 Faster----"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 5,  F("3 Equal----"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 6, F("4 Lower---"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 7, F("5 Lowest-"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 8, F("Q 75Hz interrupt"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 9, F("W 62,5Hz interrupt"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 10, F("E 50Hz interrupt"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 9, F("R 38,5Hz interrupt"), COLOR_TEXT);
    sdNavigationPrintFStr(5, 10, F("T 25Hz interrupt"), COLOR_TEXT);
    return 0;
  }


  if (   checkKeyBit(&(KEY[4]),  BUTTON_0)
         || checkKeyBit(&KEMPSTONJOYSTICK,  BUTTON_LEFT) ) //'0' key or left
  {
    bdisplayedmenu = false;
    return -1;//back to old program
  }



  if (checkKeyBit(&(KEY[2]),  BUTTON_Q) ) //'1'
  {
    zxDisplaySetIntFrequency(75);
    return 0;
  }
  if (checkKeyBit(&(KEY[2]),  BUTTON_W) ) //'2'
  {
    zxDisplaySetIntFrequency(62);
    return 0;
  }
  if (checkKeyBit(&(KEY[2]),  BUTTON_E) ) //'3'
  {
    zxDisplaySetIntFrequency(50);
    return 0;
  }
  if (checkKeyBit(&(KEY[2]),  BUTTON_R) ) //'3'
  {
    zxDisplaySetIntFrequency(38);
    return 0;
  }
  if (checkKeyBit(&(KEY[2]),  BUTTON_T) ) //'3'
  {
    zxDisplaySetIntFrequency(25);
    return 0;
  }



  if (checkKeyBit(&(KEY[3]),  BUTTON_1) ) //'1'
  {
    cpuDELAY = 50000;
    return 0;
  }
  if (checkKeyBit(&(KEY[3]),  BUTTON_2) ) //'2'
  {
    cpuDELAY = 30000;
    return 0;
  }
  if (checkKeyBit(&(KEY[3]),  BUTTON_3) ) //'3'
  {
    cpuDELAY = 5000;
    return 0;
  }
  if (checkKeyBit(&(KEY[3]),  BUTTON_4) ) //'4'
  {
    cpuDELAY = 2000;
    return 0;
  }
  if (checkKeyBit(&(KEY[3]),  BUTTON_5) ) //'5'
  {
    cpuDELAY = 500;
    return 0;
  }

  return 0;
  }
*/

#ifdef DEBUG_PRINT

void simulate_key(char cRead, char cCheck, unsigned char cBit, unsigned char *KEY, unsigned long *ulResetSerialKeyboard)
{
  if (cRead == cCheck)
  {
    *KEY = 0xff - cBit;
    *ulResetSerialKeyboard = millis() + 50;
  }
}
#endif

void loop() {

#ifdef DEBUG_PRINT


  // if (KEMPSTONJOYSTICK != 0)
  // {
  //   DEBUG_PRINTLN(KEMPSTONJOYSTICK);
  // }


  //stampabinario(KEY[0]);DEBUG_PRINT("-");
  //stampabinario(KEY[1]);DEBUG_PRINTLN("");
  // stampabinario(KEY[2]);DEBUG_PRINT("-");
  //stampabinario(KEY[3]);DEBUG_PRINTLN("");
  //stampabinario(KEY[4]);DEBUG_PRINT("-");
  //stampabinario(KEY[5]);DEBUG_PRINTLN("");
  //stampabinario(KEY[6]);DEBUG_PRINT("-");
  // stampabinario(KEY[7]);DEBUG_PRINTLN("");
  if (KEMPSTONJOYSTICK != 0)
    stampabinario(KEMPSTONJOYSTICK); DEBUG_PRINTLN("");




  //simulation of zx keyboard from serial data
  static unsigned long ulResetSerialKeyboard = 0;
  unsigned long ulNow = millis();
  if (ulNow > ulResetSerialKeyboard && ulResetSerialKeyboard)
  {
    ulResetSerialKeyboard = 0;
    KEMPSTONJOYSTICK = 0;
    KEY[0] = KEY[1] = KEY[2] = KEY[3] = KEY[4] = KEY[5] = KEY[6] = KEY[7] = 0xff;
  }
  if (Serial.available())
  {
    char cRead = Serial.read();

    simulate_key(cRead, 'y', BUTTON_Y, &KEY[5], &ulResetSerialKeyboard);
    simulate_key(cRead, 'u', BUTTON_U, &KEY[5], &ulResetSerialKeyboard);
    simulate_key(cRead, 'i', BUTTON_I, &KEY[5], &ulResetSerialKeyboard);
    simulate_key(cRead, 'o', BUTTON_O, &KEY[5], &ulResetSerialKeyboard);
    simulate_key(cRead, 'p', BUTTON_P, &KEY[5], &ulResetSerialKeyboard);

    simulate_key(cRead, 'h', BUTTON_Y, &KEY[6], &ulResetSerialKeyboard);
    simulate_key(cRead, 'j', BUTTON_U, &KEY[6], &ulResetSerialKeyboard);
    simulate_key(cRead, 'k', BUTTON_I, &KEY[6], &ulResetSerialKeyboard);
    simulate_key(cRead, 'l', BUTTON_O, &KEY[6], &ulResetSerialKeyboard);
    simulate_key(cRead, '\\', BUTTON_P, &KEY[6], &ulResetSerialKeyboard);

    simulate_key(cRead, 'b', BUTTON_Y, &KEY[7], &ulResetSerialKeyboard);
    simulate_key(cRead, 'n', BUTTON_U, &KEY[7], &ulResetSerialKeyboard);
    simulate_key(cRead, 'm', BUTTON_I, &KEY[7], &ulResetSerialKeyboard);
    simulate_key(cRead, '<', BUTTON_O, &KEY[7], &ulResetSerialKeyboard);
    simulate_key(cRead, ' ', BUTTON_P, &KEY[7], &ulResetSerialKeyboard);


    simulate_key(cRead, '0', BUTTON_0, &KEY[4], &ulResetSerialKeyboard);
    simulate_key(cRead, '9', BUTTON_9, &KEY[4], &ulResetSerialKeyboard);
    simulate_key(cRead, '8', BUTTON_8, &KEY[4], &ulResetSerialKeyboard);
    simulate_key(cRead, '7', BUTTON_7, &KEY[4], &ulResetSerialKeyboard);
    simulate_key(cRead, '6', BUTTON_6, &KEY[4], &ulResetSerialKeyboard);

    simulate_key(cRead, '1', BUTTON_1, &KEY[3], &ulResetSerialKeyboard);
    simulate_key(cRead, '2', BUTTON_2, &KEY[3], &ulResetSerialKeyboard);
    simulate_key(cRead, '3', BUTTON_3, &KEY[3], &ulResetSerialKeyboard);
    simulate_key(cRead, '4', BUTTON_4, &KEY[3], &ulResetSerialKeyboard);
    simulate_key(cRead, '5', BUTTON_5, &KEY[3], &ulResetSerialKeyboard);

    simulate_key(cRead, 'q', BUTTON_1, &KEY[2], &ulResetSerialKeyboard);
    simulate_key(cRead, 'w', BUTTON_2, &KEY[2], &ulResetSerialKeyboard);
    simulate_key(cRead, 'e', BUTTON_3, &KEY[2], &ulResetSerialKeyboard);
    simulate_key(cRead, 'r', BUTTON_4, &KEY[2], &ulResetSerialKeyboard);
    simulate_key(cRead, 't', BUTTON_5, &KEY[2], &ulResetSerialKeyboard);


    simulate_key(cRead, 'a', BUTTON_1, &KEY[1], &ulResetSerialKeyboard);
    simulate_key(cRead, 's', BUTTON_2, &KEY[1], &ulResetSerialKeyboard);
    simulate_key(cRead, 'd', BUTTON_3, &KEY[1], &ulResetSerialKeyboard);
    simulate_key(cRead, 'f', BUTTON_4, &KEY[1], &ulResetSerialKeyboard);
    simulate_key(cRead, 'g', BUTTON_5, &KEY[1], &ulResetSerialKeyboard);


    simulate_key(cRead, '>', BUTTON_1, &KEY[0], &ulResetSerialKeyboard);
    simulate_key(cRead, 'z', BUTTON_2, &KEY[0], &ulResetSerialKeyboard);
    simulate_key(cRead, 'x', BUTTON_3, &KEY[0], &ulResetSerialKeyboard);
    simulate_key(cRead, 'c', BUTTON_4, &KEY[0], &ulResetSerialKeyboard);
    simulate_key(cRead, 'v', BUTTON_5, &KEY[0], &ulResetSerialKeyboard);




    if (cRead == '|') //esc
    {
      KEMPSTONJOYSTICK = BUTTON_ESC; //esc special key
      ulResetSerialKeyboard = ulNow + 50;
    }



  }
#endif

  static char ongoingtask = EMUTASK_EMULATOR; //0=emulator,1=file browser,2=display keyboard,3=load demo from rom



  switch (ongoingtask)
  {
    case EMUTASK_EMULATOR: //emulator


      if (KEMPSTONJOYSTICK & BUTTON_ESC ) //'ESC special key
      {
        // DEBUG_PRINTLN("ESC");
        ongoingtask = EMUTASK_MENU;//keyb display
        memcpy(CACHE, RAM, ZXSCREENSIZE);
        break;
      }


      ExecZ80(&state, 50000);
      if (zxInterruptPending) //set=1 by screen routine at the end of draw
      {
        zxInterruptPending = 0;
        IntZ80(&state, INT_IRQ);
#ifdef BORDERCOLORCHANGE1HZ
        static int border = 0;
        static int cnt = 0;
        cnt++;
        if (cnt == 50)
        {
          cnt = 0;
          zxDisplayBorderSet(border);
          border++;
          if (border == 8)border = 0;
        }
#endif
      }
      break;
    case EMUTASK_MENU:
      {
        int iReturn = showMenu();
        if (iReturn < 0)
        { //back to old program
          ongoingtask = EMUTASK_EMULATOR;
          memcpy(RAM, CACHE, ZXSCREENSIZE);//restore screen
          waitforclearkeyb();
        }
        if (iReturn > 0)
        {
          ongoingtask = iReturn;
        }
      }
      break;

    case EMUTASK_JOY:
      if (showJoystick() < 0)
      { //back to old program
        ongoingtask = EMUTASK_EMULATOR;
        memcpy(RAM, CACHE, ZXSCREENSIZE);//restore screen
        waitforclearkeyb();
      }
      break;

    case EMUTASK_KEYB:
      if (showKeyboard() < 0)
      { //back to old program
        ongoingtask = EMUTASK_EMULATOR;
        memcpy(RAM, CACHE, ZXSCREENSIZE);//restore screen
        waitforclearkeyb();
      }
      break;

    case EMUTASK_SD://file browser
      switch (sdNavigation(false))
      {
        case -1://back to old program
          ongoingtask = EMUTASK_EMULATOR;
          memcpy(RAM, CACHE, ZXSCREENSIZE);
          waitforclearkeyb();
          break;
        case 2://jump to new proGRAM
          ongoingtask = EMUTASK_EMULATOR;
          break;
      }
      break;
    case EMUTASK_EPROM://eprom browser
      switch (sdNavigation(true))
      {
        case -1://back to old program
          ongoingtask = EMUTASK_EMULATOR;
          memcpy(RAM, CACHE, ZXSCREENSIZE);
          waitforclearkeyb();
          break;
        case 2://jump to new proGRAM
          ongoingtask = EMUTASK_EMULATOR;
          break;
      }
      break;

  }
}


/*zxKeyboardStartRead();
  zxKeyboardStopRead();
  volatile uint8_t *p=(volatile uint8_t *)(0x60000000+0x140);
     stampabinario(*p);p++;DEBUG_PRINT("-");
     stampabinario(*p);p++;DEBUG_PRINT("-");
     stampabinario(*p);p++;DEBUG_PRINT("-");
     stampabinario(*p);p++;DEBUG_PRINT("-");
     stampabinario(*p);p++;DEBUG_PRINT("-");
     stampabinario(*p);p++;DEBUG_PRINT("-");
     stampabinario(*p);p++;DEBUG_PRINTLN("");

  return;
*/


//static   uint32_t lastInterrupt = 0;
//uint32_t now;
//static int col = 0;


//
//
//if (zxKeyboardGetSpecialKey())
//{
// DEBUG_PRINTLN(state.PC.W);
//     stampabinario(KEY[0]);DEBUG_PRINT("-");
//     stampabinario(KEY[1]);DEBUG_PRINTLN("");
//     stampabinario(KEY[2]);DEBUG_PRINT("-");
//     stampabinario(KEY[3]);DEBUG_PRINTLN("");
//     stampabinario(KEY[4]);DEBUG_PRINT("-");
//     stampabinario(KEY[5]);DEBUG_PRINTLN("");
//     stampabinario(KEY[6]);DEBUG_PRINT("-");
//        stampabinario(KEY[7]);DEBUG_PRINTLN("");
//
//}


//ExecZ80(&state, 10000);




//now = millis();
//if ((now - lastInterrupt) >= 20)
//{
//   DEBUG_PRINTLN(state.pc);
// lastInterrupt =  now  ;
//  IntZ80(&state, INT_IRQ);
//    zxDisplayBorderSet(col);
//    col++;
//   col = col & 7;
//IntZ80(&state, 0xffff);
//    if ( state.status & Z80_STATUS_FLAG_HALT) state.pc++;

//  static char c=0;
//zxSoundSet(c);
//c^=1;

//}










void ICACHE_FLASH_ATTR stampabinario(unsigned char c)
{
  DEBUG_PRINT(c & 128 ? "1" : "0");
  DEBUG_PRINT(c & 64 ? "1" : "0");
  DEBUG_PRINT(c & 32 ? "1" : "0");
  DEBUG_PRINT(c & 16 ? "1" : "0");
  DEBUG_PRINT(c & 8 ? "1" : "0");
  DEBUG_PRINT(c & 4 ? "1" : "0");
  DEBUG_PRINT(c & 2 ? "1" : "0");
  DEBUG_PRINT(c & 1 ? "1" : "0");
}
//
//
//
//void  stampachar(unsigned char c) {
//  DEBUG_PRINT(c);
//  DEBUG_PRINT(",");
//}
