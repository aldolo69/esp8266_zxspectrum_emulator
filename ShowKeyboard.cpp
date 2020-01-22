#include <Arduino.h>**

#include "ShowKeyboard.h"
#include "GlobalDef.h"
#include "Zxkeyboard.h"

extern unsigned char RAM[];
extern const unsigned char ROM[];
extern unsigned char KEY[];
extern unsigned char KEMPSTONJOYSTICK;

extern void waitforclearkeyb(void);

int showKeyboard()
{
  static boolean bdisplayed = false;

  if (!bdisplayed)
  {
    bdisplayed = true;

    memcpy_P(RAM, keyboard_scr, ZXSCREENSIZE);
    waitforclearkeyb();
    return 0;
  }

  if (checkKeybBreak() || !(KEY[4] & 1) ||  !(KEY[6] & 1) || KEMPSTONJOYSTICK  ) //'0' key or 'enter' key  or joy
  {
    delay(100);
    bdisplayed = false;
    return -1;//return to old program

  }


  return 0;
}
