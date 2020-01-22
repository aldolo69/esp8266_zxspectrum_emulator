

#ifndef KEYBOARD_H
#define KEYBOARD_H

void zxKeyboardStopRead(void);
void zxKeyboardStartRead(void);
void zxKeyboardSetup(void);
int  zxKeyboardGetSpecialKey(void);

boolean ICACHE_FLASH_ATTR checkKeyBit( unsigned char *KEY, unsigned char button);
void ICACHE_FLASH_ATTR waitforclearkeyb();
void ICACHE_FLASH_ATTR getKeyb(unsigned char **p, unsigned char *v);

char ICACHE_FLASH_ATTR getPressedCharacter(void);
boolean ICACHE_FLASH_ATTR checkKeybBreak(void);

#endif
