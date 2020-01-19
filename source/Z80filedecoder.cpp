#include <Arduino.h>
#include"z80.h"
 
#include "Z80filedecoder.h"

extern Z80 state;
extern unsigned char RAM[];
extern unsigned char CACHE[];



//decode v1 .z80 file
void ICACHE_FLASH_ATTR z80FileDecompress(SdFile *file, boolean m_isCompressed) {
  uint32_t iMemSize = file->fileSize();
  uint32_t offset = 0; // Current offset into memory
  int iCacheOffset = 0;
  int nextyeld = 1000;

  DEBUG_PRINTLN(iMemSize);


  // TODO: It's 30 just now, but if this is v.2/3 of the file then it's not.. See docs.
  for (int i = 30; i < iMemSize ; i++)
  {

    if (i >= (iCacheOffset + 512))
    {
      //   DEBUG_PRINTLN(F("cacheoffset"));
      //   DEBUG_PRINTLN(iCacheOffset);
      memcpy(CACHE, CACHE + 512, 512); //shift down
      iCacheOffset += 512;
      file->read(CACHE + 512, 512); //read next block
      //     DEBUG_PRINTLN(F("read512"));
    }



    if (CACHE[(i) - iCacheOffset] == 0x00 && CACHE[(i + 1) - iCacheOffset] == 0xED && CACHE[(i + 2) - iCacheOffset] == 0xED && CACHE[(i + 3) - iCacheOffset] == 0x00)
    {
      break;
    }

    if (i < iMemSize  - 4)
    {
      if (CACHE[(i) - iCacheOffset] == 0xED && CACHE[(i + 1) - iCacheOffset] == 0xED && m_isCompressed)
      {
        i += 2;
        unsigned int repeat = (unsigned char) CACHE[(i) - iCacheOffset];
        i++;
        unsigned char value = (unsigned char) CACHE[(i) - iCacheOffset];

        //       DEBUG_PRINTLN(repeat);
        for (int j = 0; j < repeat; j++)
        {
          if (offset < RAMSIZE)   RAM[offset] = value;
          offset++;
        }
      }
      else
      {
        if (offset < RAMSIZE) RAM[offset] = CACHE[(i) - iCacheOffset];
        offset++;
      }
    }
    else
    {
      if (offset < RAMSIZE) RAM[offset] = CACHE[(i) - iCacheOffset];
      offset++;

    }
  }


}







//decode v1 .z80 eeprom
void ICACHE_FLASH_ATTR z80FileDecompressFromEeprom(const unsigned char *MEM, int iMEMsize, int startoffset, boolean m_isCompressed)
{
  uint32_t offset = 0; // Current offset into memory
  int nextyeld = 1000;
  // TODO: It's 30 just now, but if this is v.2/3 of the file then it's not.. See docs.

  if (m_isCompressed) DEBUG_PRINTLN("compressed");

  for (int i = startoffset; i < iMEMsize; i++)
  {

    if (pgm_read_byte(MEM + i) == 0x00 && pgm_read_byte(MEM + i + 1) == 0xED && pgm_read_byte(MEM + i + 2) == 0xED && pgm_read_byte(MEM + i + 3) == 0x00)
    {
      break;
    }

    if (i < iMEMsize - 4)
    {
      if (pgm_read_byte(MEM + i) == 0xED && pgm_read_byte(MEM + i + 1) == 0xED && m_isCompressed)
      {
        i += 2;
        unsigned int repeat = (unsigned char) pgm_read_byte(MEM + i);
        i++;
        unsigned char value = (unsigned char) pgm_read_byte(MEM + i);


        for (int j = 0; j < repeat; j++)
        {
          if (offset < RAMSIZE)   RAM[offset] = value;
          offset++;
        }
      }
      else
      {
        if (offset < RAMSIZE) RAM[offset] = pgm_read_byte(MEM + i);
        offset++;
      }
    }
    else
    {
      if (offset < RAMSIZE) RAM[offset] = pgm_read_byte(MEM + i);
      offset++;

    }
  }


}






//variable for buffered file or eeprom decoding
const unsigned char *z80FileFillCacheAdd;
SdFile *z80FileFillCachefile;
int z80FileLen = 0;
int z80FileOff = 0;

//start decoding from eeprom
void z80FileFillCacheStartFromEeprom(const unsigned char *p, int len)
{
  z80FileFillCacheAdd = p;
  z80FileLen = len;
  memcpy_P(CACHE, p, 1024);
  z80FileOff = 1024;
}

void z80FileFillCacheContinueFromEeprom()
{
  DEBUG_PRINTLN("readcache");
  DEBUG_PRINTLN(z80FileOff);

  memcpy_P(CACHE + 512, z80FileFillCacheAdd + z80FileOff, (z80FileLen - z80FileOff) >= 512 ? 512 : (z80FileLen - z80FileOff));
  z80FileOff += (z80FileLen - z80FileOff) >= 512 ? 512 : (z80FileLen - z80FileOff);
}

//start decoding from file
void z80FileFillCacheStart(SdFile *p, int len)
{
  z80FileFillCachefile = p;
  z80FileLen = len;
  z80FileFillCachefile->read(CACHE , 1024);
  z80FileOff = 1024;
}

void z80FileFillCacheContinue()
{
  DEBUG_PRINTLN("readcache");
  DEBUG_PRINTLN(z80FileOff);
  z80FileFillCachefile->read(CACHE + 512 , (z80FileLen - z80FileOff) >= 512 ? 512 : (z80FileLen - z80FileOff));
  z80FileOff += (z80FileLen - z80FileOff) >= 512 ? 512 : (z80FileLen - z80FileOff);
}






//decode v2 .z80 file
void ICACHE_FLASH_ATTR z80FileDecompressV2(int iDataSize, int iDataOffset, void(*callbackFillCache)(void)) {

  uint32_t offset = 0; // Current offset into memory
  int iCacheOffset = 0;
  int nextyeld = 1000;

  int nextBlock = 0;
  int nextBlockSize = 0;

  DEBUG_PRINTLN("startdecode");
  DEBUG_PRINTLN(iDataSize);



  for (int i = iDataOffset; i < iDataSize ; i++)
  {


    if (nextBlockSize == 0)
    { //look for another block becouse this one is over
      nextBlock = 0;
      DEBUG_PRINTLN("actualoffset");
      DEBUG_PRINTLN(offset);

    }



    if (nextBlock == 0)
    { //get block size and address

      DEBUG_PRINTLN("NextBlock=");
      DEBUG_PRINTLN(CACHE[(i + 2) - iCacheOffset]);

      nextBlockSize = CACHE[(i) - iCacheOffset] + (CACHE[(i) - iCacheOffset + 1] << 8);


      switch (CACHE[(i + 2) - iCacheOffset])
      {
        case 4:
          DEBUG_PRINTLN("0x8000");
          offset = 0x8000 - ROMSIZE;
          nextBlock = 4;
          break;
        case 5:
          DEBUG_PRINTLN("0xc000");
          offset = 0xc000 - ROMSIZE;
          nextBlock = 5;
          break;
        case 8:
          DEBUG_PRINTLN("0x4000");

          offset = 0x4000 - ROMSIZE;
          nextBlock = 8;
          break;
      }

      DEBUG_PRINTLN(offset);
      DEBUG_PRINTLN(nextBlock);
      DEBUG_PRINTLN(nextBlockSize);
      i += 3;

    }




    if (i >= (iCacheOffset + 512))
    { //cache empty
      DEBUG_PRINTLN(F("cacheoffset"));
      DEBUG_PRINTLN(iCacheOffset);
      memcpy(CACHE, CACHE + 512, 512); //shift down
      iCacheOffset += 512;
      (*callbackFillCache)(); //read next block
    }



    if (  nextBlockSize  >= 4)
    {
      if (CACHE[(i) - iCacheOffset] == 0xED && CACHE[(i + 1) - iCacheOffset] == 0xED )
      {
        i += 2;
        nextBlockSize -= 2;
        unsigned int repeat = (unsigned char) CACHE[(i) - iCacheOffset];
        i++;
        nextBlockSize -= 1;
        unsigned char value = (unsigned char) CACHE[(i) - iCacheOffset];

        //       DEBUG_PRINTLN(repeat);
        for (int j = 0; j < repeat; j++)
        {
          if (offset < RAMSIZE)   RAM[offset] = value;
          offset++;
        }
      }
      else
      {
        if (offset < RAMSIZE) RAM[offset] = CACHE[(i) - iCacheOffset];
        offset++;
      }
    }
    else
    {
      if (offset < RAMSIZE) RAM[offset] = CACHE[(i) - iCacheOffset];
      offset++;

    }

    nextBlockSize -= 1;


  }






}










void ICACHE_FLASH_ATTR z80FileLoadFromEeprom(const unsigned char *MEM, int MemSize)
{
  struct z80fileheader header;
  struct z80fileheader2 header2;
  int offset = 30;

  z80FileFillCacheStartFromEeprom(MEM, MemSize);


  ResetZ80(&state);

  memcpy(&header, CACHE, 30);
  memcpy(&header2, CACHE + 30, 4);

  if (header._PC == 0)
  {
    DEBUG_PRINTLN("ver2/3");
    DEBUG_PRINTLN(header2._header2len);
    offset += header2._header2len + 2;
    header._PC = header2._PC;
    DEBUG_PRINTLN("PC");
    DEBUG_PRINTLN(header._PC);

    SetZ80(&state, &header);
    z80FileDecompressV2(MemSize, offset, &z80FileFillCacheContinueFromEeprom);

  }
  else
  {

    SetZ80(&state, &header);

    //DEBUG_PRINTLN("start");
    //DEBUG_PRINTLN(sizeof(z80file_decathlon));
    z80FileDecompressFromEeprom(MEM, MemSize, offset, (header._Flags1 & 0x20) ? true : false);
  }

  DEBUG_PRINTLN("end");

}








int  ICACHE_FLASH_ATTR z80FileLoad(SdFile *file)
{
  struct z80fileheader header;
  struct z80fileheader2 header2;
  int offset = 30;
int filesize=file->fileSize();

  z80FileFillCacheStart(file, filesize);


  ResetZ80(&state );

  memcpy(&header, CACHE, 30);
  memcpy(&header2, CACHE + 30, 4);

  if (header._PC == 0)
  {
    DEBUG_PRINTLN("ver2/3");
    DEBUG_PRINTLN(header2._header2len);
    offset += header2._header2len + 2;
    header._PC = header2._PC;
    DEBUG_PRINTLN("PC");
    DEBUG_PRINTLN(header._PC);

    SetZ80(&state, &header);
    z80FileDecompressV2(filesize, offset, &z80FileFillCacheContinue);

  }
  else
  {

    SetZ80(&state, &header);

    //DEBUG_PRINTLN("start");
    //DEBUG_PRINTLN(sizeof(z80file_decathlon));
    z80FileDecompress( file, (header._Flags1 & 0x20) ? true : false);
  }


  return 0;
}


