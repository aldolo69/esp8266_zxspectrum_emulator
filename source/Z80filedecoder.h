#ifndef Z80FILEDECODER_H
#define Z80FILEDECODER_H
#include<Arduino.h>

/*
        0       1       A register
        1       1       F register
        2       2       BC register pair (LSB, i.e. C, first)
        4       2       HL register pair
        6       2       Program counter
        8       2       Stack pointer
        10      1       Interrupt register
        11      1       Refresh register (Bit 7 is not significant!)
        12      1       Bit 0  : Bit 7 of the R-register
                        Bit 1-3: Border colour
                        Bit 4  : 1=Basic SamRom switched in
                        Bit 5  : 1=Block of data is compressed
                        Bit 6-7: No meaning
        13      2       DE register pair
        15      2       BC' register pair
        17      2       DE' register pair
        19      2       HL' register pair
        21      1       A' register
        22      1       F' register
        23      2       IY register (Again LSB first)
        25      2       IX register
        27      1       Interrupt flipflop, 0=DI, otherwise EI
        28      1       IFF2 (not particularly important...)
        29      1       Bit 0-1: Interrupt mode (0, 1 or 2)
                        Bit 2  : 1=Issue 2 emulation
                        Bit 3  : 1=Double interrupt frequency
                        Bit 4-5: 1=High video synchronisation
                                 3=Low video synchronisation
                                 0,2=Normal
                        Bit 6-7: 0=Cursor/Protek/AGF joystick
                                 1=Kempston joystick
                                 2=Sinclair 2 Left joystick (or user
                                   defined, for version 3 .z80 files)
                                 3=Sinclair 2 Right joystick
*/

typedef struct z80fileheader { 
    uint8_t _A;
    uint8_t _F;
    uint16_t _BC;//(LSB, i.e. C, first)
    uint16_t _HL;
    uint16_t _PC;
    uint16_t _SP;
    uint8_t _InterruptRegister;
    uint8_t _RefreshRegister;
    uint8_t _Flags1;
    uint16_t _DE;
    uint16_t _BC_Dash;
    uint16_t _DE_Dash;
    uint16_t _HL_Dash;
    uint8_t _A_Dash;
    uint8_t _F_Dash;
    uint16_t _IY;
    uint16_t _IX;
    uint8_t _InterruptFlipFlop;
    uint8_t _IFF2;
    uint8_t _Flags2;
};

typedef struct z80fileheader2 {
      uint16_t _header2len; //        Length of additional header block (see below)
      uint16_t _PC;//  Program counter
//The value of the word at position 30 is 23 for version 2 files, and 54 or 55 for version 3; the fields marked '*' are the ones that are present in the version 2 header. The final byte (marked '**') is present only if the word at position 30 is 55.
};




#ifdef __cplusplus
#include <SPI.h>
#include "SdFat.h"
#include "FreeStack.h"
int  z80FileLoad(SdFile *file);
void z80FileDecompress(SdFile *file,boolean m_isCompressed);
void z80FileLoadFromEeprom(const unsigned char *MEM,int iMEMsize);
void z80FileDecompressFromEeprom(const unsigned char *MEM,int iMEMsize,int offset,boolean m_isCompressed);

void z80FileDecompressV2(int iDataSize, int iDataOffset, void(*callbackFillCache)(void));




#endif

#endif
