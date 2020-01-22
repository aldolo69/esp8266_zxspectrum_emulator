#include "zxsound.h"
 
extern unsigned char soundenabled;
void  zxSoundSetup()
{
  pinMode(SPEAKER_CS, OUTPUT);
}

void  zxSoundSet(unsigned char c)
{
  digitalWrite(SPEAKER_CS, c&soundenabled);
//DEBUG_PRINT(c);
}
