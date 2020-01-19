#ifndef SCREEN_H
#define SCREEN_H

void zxDisplaySetup(unsigned char *ram);
#ifdef __cplusplus
extern "C" {
#endif
void  zxDisplayBorderSet(int i);
void  zxDisplayWriteSerial(int i);

#ifdef __cplusplus
}
#endif

void zxDisplayScan(void);


void zxDisplayReset(void);
void zxDisplayDisableInterrupt(void);
void zxDisplayEnableInterrupt(void);
void zxDisplaySetIntFrequency(int);


#endif



