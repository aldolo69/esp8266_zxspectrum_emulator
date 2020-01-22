#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>
#include "GlobalDef.h"



void zxSoundSetup(void);

#ifdef __cplusplus
extern "C" {
#endif

void  zxSoundSet(unsigned char c);

#ifdef __cplusplus
}
#endif


#endif



