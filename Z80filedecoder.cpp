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
File   *z80FileFillCachefile2;
int z80FileLen = 0;
int z80FileOff = 0;

//start decoding from eeprom
void ICACHE_FLASH_ATTR z80FileFillCacheStartFromEeprom(const unsigned char *p, int len)
{
  z80FileFillCacheAdd = p;
  z80FileLen = len;
  memcpy_P(CACHE, p, 1024);
  z80FileOff = 1024;
}

void ICACHE_FLASH_ATTR z80FileFillCacheContinueFromEeprom()
{
  DEBUG_PRINTLN("readcache");
  DEBUG_PRINTLN(z80FileOff);

  memcpy_P(CACHE + 512, z80FileFillCacheAdd + z80FileOff, (z80FileLen - z80FileOff) >= 512 ? 512 : (z80FileLen - z80FileOff));
  z80FileOff += (z80FileLen - z80FileOff) >= 512 ? 512 : (z80FileLen - z80FileOff);
}

//start decoding from file
void ICACHE_FLASH_ATTR z80FileFillCacheStart(SdFile *p, int len)
{
  z80FileFillCachefile = p;
  z80FileLen = len;
  z80FileFillCachefile->read(CACHE , 1024);
  z80FileOff = 1024;
}

void ICACHE_FLASH_ATTR z80FileFillCacheContinue()
{
  DEBUG_PRINTLN("readcache");
  DEBUG_PRINTLN(z80FileOff);
  z80FileFillCachefile->read(CACHE + 512 , (z80FileLen - z80FileOff) >= 512 ? 512 : (z80FileLen - z80FileOff));
  z80FileOff += (z80FileLen - z80FileOff) >= 512 ? 512 : (z80FileLen - z80FileOff);
}


void ICACHE_FLASH_ATTR z80FileSaveFillCacheStart(File *p, int offset)
{
  z80FileFillCachefile2 = p;
  z80FileOff = offset;
}

int ICACHE_FLASH_ATTR z80FileSaveFillCacheContinue(unsigned char c )
{
  CACHE[z80FileOff] = c;
  z80FileOff++;
  if (z80FileOff == 512)
  {
    if (z80FileOff != z80FileFillCachefile2->write(CACHE , z80FileOff))
    {
      DEBUG_PRINTLN(F("WRITE KO"));
      return -1;
    }

     
    DEBUG_PRINTLN(F("WRITE:"));
    DEBUG_PRINTLN(z80FileOff);
   z80FileOff = 0;
  }
  return 0;
}

int ICACHE_FLASH_ATTR z80FileSaveFillCacheEnd( )
{
  int i;
  if (z80FileOff != 0)
  {
//    i=;
//    DEBUG_PRINTLN(F("WRITE return:"));
//    DEBUG_PRINTLN(i);

    if (z80FileOff != z80FileFillCachefile2->write(CACHE , z80FileOff))
    {
      DEBUG_PRINTLN(F("WRITE KO END"));
      return -1;
    }
    DEBUG_PRINTLN(F("WRITE:"));
    DEBUG_PRINTLN(z80FileOff);

  }

  z80FileFillCachefile2->flush();
  z80FileFillCachefile2->close();

  DEBUG_PRINTLN(F("WRITE END END"));
  return 0;
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
  int filesize = file->fileSize();

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





//save z80 file. file format 1 with 30 bytes header
int  ICACHE_FLASH_ATTR z80FileSave(File *file)
{

  int offset;
  int counter;

  GetZ80(&state, (struct z80fileheader *)(CACHE));
  z80FileSaveFillCacheStart(file, 30);


  DEBUG_PRINTLN("SAVE start");
  //DEBUG_PRINTLN(sizeof(z80file_decathlon));

  for (offset = 0; offset < RAMSIZE - 4;)
  {
    //The compression method is very simple: it replaces repetitions of at least five equal bytes by a four-byte code ED ED xx yy,
    //which stands for "byte yy repeated xx times". Only sequences of length at least 5 are coded.
    //The exception is sequences consisting of ED's; if they are encountered, even two ED's are encoded into ED ED 02 ED.

    //Finally, every byte directly following a single ED is not taken into a block,
    //for example ED 6*00 is not encoded into ED ED ED 06 00 but into ED 00 ED ED 05 00. The block is terminated by an end marker, 00 ED ED 00.


    if (offset > 1 && RAM[offset - 2] != 0xed && RAM[offset - 1] == 0xed && RAM[offset] != 0xed)
    {
      if(z80FileSaveFillCacheContinue(RAM[offset] )) return -1;
      offset++;
      continue;
    }

    if (RAM[offset] == 0xed &&
        RAM[offset] == RAM[offset + 1]
       )
    {
      //ED compression

      //up to ff bytes...
      for (counter = 0; counter < 254; counter++)
      {
        if ( RAM[offset + counter] != RAM[offset + counter + 1]) break;
        if (offset + counter + 1 == RAMSIZE) break;
      }

      counter++;
      if(z80FileSaveFillCacheContinue(0xed )) return -1;;
      if(z80FileSaveFillCacheContinue(0xed )) return -1;;
      if(z80FileSaveFillCacheContinue(counter )) return -1;;
      if(z80FileSaveFillCacheContinue(RAM[offset] )) return -1;;
      offset += counter;

      continue;
    }

    if (RAM[offset] == RAM[offset + 1] &&
        RAM[offset] == RAM[offset + 2] &&
        RAM[offset] == RAM[offset + 3] &&
        RAM[offset] == RAM[offset + 4]
       )
    {
      //non ED compression

      //up to ff bytes...
      for (counter = 0; counter < 254; counter++)
      {
        if ( RAM[offset + counter] != RAM[offset + counter + 1]) break;
        if (offset + counter + 1 == RAMSIZE) break;
      }

      counter++;
      if(z80FileSaveFillCacheContinue(0xed )) return -1;;
      if(z80FileSaveFillCacheContinue(0xed )) return -1;;
      if(z80FileSaveFillCacheContinue(counter )) return -1;;
      if(z80FileSaveFillCacheContinue(RAM[offset] )) return -1;;
      offset += counter;
      continue;
    }

    if(z80FileSaveFillCacheContinue(RAM[offset] )) return -1;
    offset++;
  }

  for (; offset < RAMSIZE;)
  {
    if(z80FileSaveFillCacheContinue(RAM[offset++] )) return -1;;
  }

  if(z80FileSaveFillCacheContinue(0 )) return -1;;
  if(z80FileSaveFillCacheContinue(0xed )) return -1;;
  if(z80FileSaveFillCacheContinue(0xed )) return -1;;
  if(z80FileSaveFillCacheContinue(0 )) return -1;;

  return z80FileSaveFillCacheEnd( );
   
}
