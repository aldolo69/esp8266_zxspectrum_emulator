//browse sd card or demo app storedin eeprom
#include <SPI.h>
#include "SdFat.h"
#include "FreeStack.h"
#include "Zxdisplay.h"
#include "Zxsound.h"
#include "Z80filedecoder.h"
#include "Zxkeyboard.h"
#include "GlobalDef.h"
#include "SpiSwitch.h"
#include "SdNavigation.h"


//to add/change demos in rom:
//-change z80file_demo.h content
//-change iromfilesize
//-change promfilename. leave the last 0 there
#include "Z80file_demo.h"
const unsigned char * pRomFile[] = {};//z80file_bubblefrenzy, z80file_fistrofighter, z80file_invasivespecies};
int iRomFileSize[] = {};//sizeof(z80file_bubblefrenzy), sizeof(z80file_fistrofighter), sizeof(z80file_invasivespecies)};
char * pRomFileName[] = {0};//F("Bubble frenzy"), F("Fist RO Fighter"), F("Invasive species"), 0};



extern void waitforclearkeyb(void);

extern unsigned char RAM[];
extern const unsigned char ROM[];
extern unsigned char KEY[];
extern unsigned char KEMPSTONJOYSTICK;


#define LISTLEN 11

int sdNavigationReadFiles = -1; //-1 if nothing available
int sdNavigationIndex = 0;
int sdNavigationCursor = 0;






void ICACHE_FLASH_ATTR sdNavigationCls(int col)
{
  memset(RAM, 0, 32 * 192);
  memset(RAM + 32 * 192, col, 32 * 24);
}


//x,y in char coordinates. 0..31 0..23
//return address of bitmap and color
void ICACHE_FLASH_ATTR sdNavigationPixMem(int x, int y, int *bmp, int *col)
{
  y *= 8;
  *bmp = x + (  ((y & (8 + 16 + 32)) << 2) + ((y & 7) << 8) + ((y & (64 + 128)) << 5));
  *col = x + ((192 * 32) + ((y & (255 - 7)) << 2) );
}


//clear a line
void ICACHE_FLASH_ATTR sdNavigationClearLine(int line, int color)
{
  int bmpoffset;
  int coloffset;
  sdNavigationPixMem(0, line, &bmpoffset, &coloffset);

  for (int i = 0; i < 8; i++) {
    memset(RAM + bmpoffset, 0, 32 );
    bmpoffset += 256;
  }
  memset(RAM + coloffset, color, 32);

}

//print a character
void ICACHE_FLASH_ATTR sdNavigationPrintCh(int x, int y, unsigned char c, int col)
{
  const unsigned char *p = ROM + (c * 8 + 15360);//zx spectrum char set
  int bmpoffset;
  int coloffset;

  unsigned char row;

  sdNavigationPixMem(x, y, &bmpoffset, &coloffset);

  RAM[coloffset] = col;

  for (int i = 0; i < 8; i++)
  {
    row = pgm_read_byte(p);
    p++;
    RAM[bmpoffset] = row;
    bmpoffset += 256;
  }
}


//print a 2x2 character doubling scanlines and horizontal bits
void ICACHE_FLASH_ATTR sdNavigationPrintChBig(int x, int y, unsigned char c, int col)
{
  const unsigned char *p = ROM + (c * 8 + 15360);
  int bmpoffset;
  int coloffset;

  unsigned char row;
  unsigned char row1;
  unsigned char row2;

  sdNavigationPixMem(x, y, &bmpoffset, &coloffset);

  RAM[coloffset] = col;
  RAM[coloffset + 1] = col;
  RAM[coloffset + 32] = col;
  RAM[coloffset + 33] = col;

  for (int i = 0; i < 8; i++)
  {

    if (i == 4)
    {
      sdNavigationPixMem(x, y + 1, &bmpoffset, &coloffset);
    }

//doubles horizontal pixels
    row = pgm_read_byte(p);
    row1 = 0;
    row2 = 0;
    row1 |= row & 128 ? (128 + 64) : 0;
    row1 |= row & 64 ? (32 + 16) : 0;
    row1 |= row & 32 ? (8 + 4) : 0;
    row1 |= row & 16 ? (2 + 1) : 0;
    row2 |= row & 8 ? (128 + 64) : 0;
    row2 |= row & 4 ? (32 + 16) : 0;
    row2 |= row & 2 ? (8 + 4) : 0;
    row2 |= row & 1 ? (2 + 1) : 0;



    p++;
    RAM[bmpoffset] = row1;
    RAM[bmpoffset + 1] = row2;
    RAM[bmpoffset + 256] = row1;
    RAM[bmpoffset + 1 + 256] = row2;
    bmpoffset += 512;
  }
}


//print a string
void ICACHE_FLASH_ATTR sdNavigationPrintStr(int x, int y, char *str, int col)
{
  for (int i = 0; str[i] && i < 32; i++)
  {
    sdNavigationPrintCh(x++, y, str[i], col);
  }
}

void ICACHE_FLASH_ATTR sdNavigationPrintStrBig(int x, int y, char *str, int col)
{
  for (int i = 0; str[i] && i < 32; i++)
  {
    sdNavigationPrintChBig(x, y, str[i], col);
    x += 2;
  }
}

//print a F string
void ICACHE_FLASH_ATTR sdNavigationPrintFStr(int x, int y, char *str, int col)
{
  char c;
  for (int i = 0; i < 32; i++)
  {
    c = pgm_read_byte(((PGM_P)str ) + i);
    if (c == 0) break;
    sdNavigationPrintCh(x++, y, c, col);
  }
}

void ICACHE_FLASH_ATTR sdNavigationPrintFStrBig(int x, int y, char *str, int col)
{
  char c;
  for (int i = 0; i < 32; i++)
  {
    c = pgm_read_byte(((PGM_P)str ) + i);
    if (c == 0) break;
    sdNavigationPrintChBig(x, y, c, col);
    x += 2;
  }
}


//stop display scan
void ICACHE_FLASH_ATTR sdNavigationLockSPI()
{
  zxDisplayDisableInterrupt();
  delay(100);//enough to consume an interrupt
  spiSwitchSet(SD_CS);
}


//after sd card usage restore spi for display
void ICACHE_FLASH_ATTR sdNavigationUnlockSPI()
{
  spiSwitchSet(TFT_CS);
  SPI.setFrequency(SPI_SPEED_TFT);//restore display spi speed
  zxDisplayEnableInterrupt();
}












/*perform a command callbackprocess on the files of the directrory
   from fromindex file. up to maxfiles processed. callbackfilter
   remove unwanted files form list.
   example: print on screen the found files ending with .z80
   callbackprocess print the file according to its index
   callbackfilter check the filename for .z80 extension

   if the command fails return -1. for example in case of removed sd
   or not formatted sd

   the command return the number of found files. 0 at the end

   no sd initialization is required before
   remember to unlock spi before return
*/
int ICACHE_FLASH_ATTR sdNavigationFileProcess(int fromindex, int maxfiles,
    boolean(*callbackFilter)(SdFile *file, int indx),
    void(*callbackProcess)(SdFile *file, int indx)) {


  DEBUG_PRINTLN("listfiles");
  DEBUG_PRINTLN(fromindex);
  DEBUG_PRINTLN(maxfiles);

  sdNavigationLockSPI();

  SdFat sd;
  SdFile file;
  SdFile dirFile;
  int foundfiles = 0;
  int index = 0;



  if (!sd.begin(SD_CS, SPISettings(
                  SPI_SPEED_SD,
                  MSBFIRST,
                  SPI_MODE0))) {
    sdNavigationUnlockSPI();
    return -1;
  }

  // List files in root directory. always start from the first
  if (!dirFile.open("/", O_RDONLY)) {
    sdNavigationUnlockSPI();
    return -1;
  }

  while (file.openNext(&dirFile, O_RDONLY)) {

    if (foundfiles == maxfiles)
    {
      sdNavigationUnlockSPI();
      return foundfiles;
    }

    if (callbackFilter != NULL)
    {
      if (!(*callbackFilter)(&file, index))
      {
        file.close();
        continue;
      }
    }


    // Skip directories and hidden files.
    if (file.isSubDir() || file.isHidden()) {
      //do nothing
      file.close();
      continue;
    }

    //found a file
    if (index >= fromindex)
    {
      //DEBUG_PRINT(index);
      //DEBUG_PRINT(' ' );
      //file.printName(&Serial);
      //DEBUG_PRINTLN  ();
      if (callbackProcess != NULL)  (*callbackProcess)(&file, index);
      foundfiles++;
    }
    index++;

    file.close();
  }

  sdNavigationUnlockSPI();
  return foundfiles;//no sd error
}





int ICACHE_FLASH_ATTR sdNavigationProcessFromEeprom(int fromindex, int maxfiles,
    void(*callbackProcess)( int indx)) {

  DEBUG_PRINTLN("eepromfiles");
  DEBUG_PRINTLN(fromindex);
  DEBUG_PRINTLN(maxfiles);

  int foundfiles = 0;
  int index = 0;




  for (; pRomFileName[index];) {

    if (foundfiles == maxfiles)
    {
      return foundfiles;
    }


    //found a file
    if (index >= fromindex)
    {
      //DEBUG_PRINT(index);
      //DEBUG_PRINT(' ' );
      //file.printName(&Serial);
      //DEBUG_PRINTLN  ();
      if (callbackProcess != NULL)  (*callbackProcess)(index);
      foundfiles++;
    }
    index++;

  }

  return foundfiles;//no sd error
}







//write index SPACE and file name.
//color according to current cursor position
void ICACHE_FLASH_ATTR sdNavigationCallbackPrint(SdFile *file, int indx)
{
  char str[33];
  int y = 2 * (indx % LISTLEN);

  sdNavigationClearLine(y, COLOR_FILE);
  sdNavigationClearLine(y + 1, COLOR_FILE);

  sdNavigationPrintNumberBig(indx, COLOR_FILE);

  file->getName(str, 14);
  sdNavigationPrintStrBig(6, y, str, COLOR_FILE);

  //  int filesize = file->fileSize();
  //  sprintf(str, "%6d", filesize);
  //  sdNavigationPrintStrBig(26, y, str, COLOR_FILE);

  DEBUG_PRINTLN(str);

}

void ICACHE_FLASH_ATTR sdNavigationCallbackPrintFromEeprom(int indx)
{
  char str[33];
  int y = 2 * (indx % LISTLEN);

  sdNavigationClearLine(y, COLOR_FILE);
  sdNavigationClearLine(y + 1, COLOR_FILE);

  sdNavigationPrintNumberBig(indx, COLOR_FILE);
  memcpy_P(str, pRomFileName[indx], 13);
  str[13] = 0;
  sdNavigationPrintStrBig(6, y, str, COLOR_FILE);


}





boolean ICACHE_FLASH_ATTR sdNavigationCallbackFilter(SdFile *file, int indx)
{
  char nome[128];
  int len;

  file->getName(nome, 128);
  len = strlen(nome);
  if (len < 4) return false;
  //name.z80 or nome.Z80
  //
  len -= 4;
  if (strcmp(nome + len, ".z80") == 0) return true;
  if (strcmp(nome + len, ".Z80") == 0) return true;
  return false;
}




void ICACHE_FLASH_ATTR sdNavigationCallbackLoad(SdFile *file, int indx)
{
  z80FileLoad(file);
}

void ICACHE_FLASH_ATTR sdNavigationCallbackLoadFromEeprom(int indx)
{

  z80FileLoadFromEeprom(pRomFile[indx], iRomFileSize[indx]);

}









void ICACHE_FLASH_ATTR sdNavigationPrintNumber(int indx, int color)
{
  char str[4];
  sprintf(str, "%3d", indx + 1);
  sdNavigationPrintStr(0, indx % LISTLEN, str, color);
}

void ICACHE_FLASH_ATTR sdNavigationPrintNumberBig(int indx, int color)
{
  char str[3];
  sprintf(str, "%2d", indx + 1);
  sdNavigationPrintStrBig(0, 2 * (indx % LISTLEN), str, color);
}

void ICACHE_FLASH_ATTR sdNavigationReset()
{
  sdNavigationCursor = 0;
  sdNavigationIndex = 0;
  sdNavigationReadFiles = -1;

}



int ICACHE_FLASH_ATTR sdNavigationList(int fromindex, int len, boolean fromEeprom)
{
  static unsigned long ulNextCheck = 0;
  unsigned long ulNow = millis();
  int foundfiles ;

  if (ulNextCheck && ulNow < ulNextCheck) return -1; //timeout before next check
  if (fromEeprom)
  {
    foundfiles = sdNavigationProcessFromEeprom(fromindex, len , &sdNavigationCallbackPrintFromEeprom);
  }
  else {
    foundfiles = sdNavigationFileProcess(fromindex, len , &sdNavigationCallbackFilter, &sdNavigationCallbackPrint);
  }
  if ( foundfiles   < 0)
  {
    ulNextCheck = ulNow + 1000;

    sdNavigationReset();
    sdNavigationCls(INK_BLACK);
    sdNavigationPrintFStr(0, LISTLEN, F("SD not available"), COLOR_ERROR);

    return (-1);
  }

  ulNextCheck = 0; //no timeout

  //update video
  //
  //clear remaining lines
  //sdNavigationReadFiles = foundfiles;
  if (foundfiles)
  {
    for (int i = foundfiles; i < LISTLEN; i++)
    {
      sdNavigationClearLine(i * 2, COLOR_FILE);
      sdNavigationClearLine(i * 2 + 1, COLOR_FILE);
    }
  }

  sdNavigationPrintFStrBig(0, 22, F("Q/A/0/ENT or joy"), COLOR_TEXT);


  return foundfiles;
}


int ICACHE_FLASH_ATTR sdNavigation(boolean fromEeprom)
{
  static boolean bdisplayed = false;
  
  if (sdNavigationReadFiles == -1) //initialize list
  {
    unsigned long int lastcheck = 0;
    unsigned long now = millis();
    if (now - lastcheck < 1000) return -1;
    lastcheck = now;

    sdNavigationReset();
    int foundfiles = sdNavigationList(sdNavigationIndex, LISTLEN, fromEeprom);

    sdNavigationReadFiles = foundfiles;
    if (foundfiles > 0) sdNavigationPrintNumberBig(0, COLOR_FILE_SELECTED);

  if (!bdisplayed)
  {
    bdisplayed = true;
    waitforclearkeyb();
  }
  }

  //check up/down/enter

  //
  //  sdNavigationCursor = 0;
  //  sdNavigationIndex = 0;
  //  sdNavigationReadFiles = -1;

  if (sdNavigationReadFiles > 0)
  {
    if (!(KEY[1] & BUTTON_A) ||  (KEMPSTONJOYSTICK & BUTTON_DOWN)) //'a' key=DOWN
    {
    waitforclearkeyb();
    
      if ( (sdNavigationCursor + 1) == (sdNavigationIndex + sdNavigationReadFiles)) //goto next page
      {
        // DEBUG_PRINTLN("DOWNPAGE");
        int foundfiles = sdNavigationList(sdNavigationCursor + 1 , LISTLEN, fromEeprom);
         DEBUG_PRINTLN("down");
        DEBUG_PRINTLN(foundfiles);
        if (foundfiles <= 0)//page empty!!! go back...
        {
          delay(100);
          return (0); //no sd or no more files
        }
        //goto next page with cursor on the first line
        sdNavigationCursor++;
        sdNavigationIndex = sdNavigationCursor ;
        sdNavigationReadFiles = foundfiles;
        sdNavigationPrintNumberBig(sdNavigationCursor, COLOR_FILE_SELECTED);
        delay(100);
        return (0);
      }
      //go down a row
      //DEBUG_PRINTLN("DOWNROW");
      sdNavigationPrintNumberBig(sdNavigationCursor, COLOR_FILE);
      sdNavigationCursor++;
      sdNavigationPrintNumberBig(sdNavigationCursor, COLOR_FILE_SELECTED);
      delay(100);
      return (0);

    }
    if (!(KEY[2] & BUTTON_Q) ||  (KEMPSTONJOYSTICK & BUTTON_UP)) //'q' key=UP
    {
     waitforclearkeyb();
  
      if (sdNavigationCursor == 0) return (0);

      if ((sdNavigationCursor % LISTLEN) == 0) //return to previous page
      {
        int foundfiles = sdNavigationList(sdNavigationIndex - LISTLEN , LISTLEN, fromEeprom);
        if (foundfiles <= 0)//page empty!!! go back...
        {
          delay(100);
          return (0); //no sd or no more files
        }
        //goto prev page with cursor on the first line
        sdNavigationIndex -= LISTLEN;
        sdNavigationCursor--;
        sdNavigationReadFiles = foundfiles;
        sdNavigationPrintNumberBig(sdNavigationCursor, COLOR_FILE_SELECTED);
        delay(100);
        return (0);
      }

      sdNavigationPrintNumberBig(sdNavigationCursor, COLOR_FILE);
      sdNavigationCursor--;
      sdNavigationPrintNumberBig(sdNavigationCursor, COLOR_FILE_SELECTED);
      delay(100);
      return (0);


    }
    if (!(KEY[6] & 1) ||  (KEMPSTONJOYSTICK & BUTTON_FIRE)) //'enter' key  or button
    {
    waitforclearkeyb();
       //scan the root for the selected file and load it
      if (fromEeprom == false)
      {
        sdNavigationFileProcess(sdNavigationCursor, 1 , &sdNavigationCallbackFilter, &sdNavigationCallbackLoad);
      }
      else
      {
        sdNavigationProcessFromEeprom(sdNavigationCursor, 1 , &sdNavigationCallbackLoadFromEeprom);
      }
      sdNavigationReset();
      delay(100);
       bdisplayed = false;
      return 2;//continue to new program

    }
  }//at least 1 file found

  if (!(KEY[4] & 1) ||  (KEMPSTONJOYSTICK & BUTTON_LEFT)) //'0' key or left
  {
    waitforclearkeyb();
     sdNavigationReset();
    delay(100);
     bdisplayed = false;
    return -1;//back to old program
  }


  return 0;
}


void ICACHE_FLASH_ATTR sdNavigationSetup()
{
  //spi pin enabled in spiswitch
}
