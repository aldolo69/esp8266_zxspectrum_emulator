#include <Arduino.h>
#include <TFT_eSPI.h>
// Hardware-specific library. lib usually uses D8 in for cs. incompatible with sd card
//also uses reset on D3

#include <SPI.h>

#include "Zxdisplay.h"
#include "Zxkeyboard.h"
#include "GlobalDef.h"
#include "SpiSwitch.h"


TFT_eSPI zxDisplayTft;// = TFT_eSPI();       // Invoke custom library

void  zxDisplayStartWrite(void);
void  zxDisplayContinueWrite(char*buffer, int counter);
void  zxDisplayStopWrite(void);


#define TIMER_DIVISOR 208//24000 interrupts per second
int TIMER_50HZ = 480; //480=50hz
//every 480 interrupt, timer keyboard is read instead of writing display
extern char zxInterruptPending;//set to 1 to start interrupt



volatile int zxDisplay_timer_counter_50hz = 0;


#define HW_FOREGROUND_RED_HI  0x07E0
#define HW_FOREGROUND_GREEN_HI 0x001F
#define HW_FOREGROUND_BLUE_HI 0xF800
#define HW_FOREGROUND_RED  0x03E0
#define HW_FOREGROUND_GREEN 0x000F
#define HW_FOREGROUND_BLUE 0x7800

#define HW_BLACK   (0)
#define HW_RED     ( HW_FOREGROUND_RED )
#define HW_GREEN   ( HW_FOREGROUND_GREEN )
#define HW_YELLOW  ( HW_FOREGROUND_RED | HW_FOREGROUND_GREEN )
#define HW_BLUE    ( HW_FOREGROUND_BLUE )
#define HW_MAGENTA ( HW_FOREGROUND_RED | HW_FOREGROUND_BLUE )
#define HW_CYAN    ( HW_FOREGROUND_BLUE | HW_FOREGROUND_GREEN )
#define HW_WHITE   ( HW_FOREGROUND_RED | HW_FOREGROUND_BLUE | HW_FOREGROUND_GREEN )
#define HW_BLACK_HI   0
#define HW_RED_HI     ( HW_FOREGROUND_RED_HI )
#define HW_GREEN_HI   ( HW_FOREGROUND_GREEN_HI )
#define HW_YELLOW_HI  ( HW_FOREGROUND_RED_HI | HW_FOREGROUND_GREEN_HI )
#define HW_BLUE_HI    ( HW_FOREGROUND_BLUE_HI )
#define HW_MAGENTA_HI ( HW_FOREGROUND_RED_HI | HW_FOREGROUND_BLUE_HI )
#define HW_CYAN_HI    ( HW_FOREGROUND_BLUE_HI | HW_FOREGROUND_GREEN_HI )
#define HW_WHITE_HI   ( HW_FOREGROUND_RED_HI | HW_FOREGROUND_BLUE_HI | HW_FOREGROUND_GREEN_HI )

static const uint32_t hw_32bits_colors[] =
{
  HW_BLACK,
  HW_BLUE,
  HW_RED,
  HW_MAGENTA,
  HW_GREEN,
  HW_CYAN,
  HW_YELLOW,
  HW_WHITE,
  HW_BLACK_HI,
  HW_BLUE_HI,
  HW_RED_HI,
  HW_MAGENTA_HI,
  HW_GREEN_HI,
  HW_CYAN_HI,
  HW_YELLOW_HI,
  HW_WHITE_HI
};


uint32_t zxDisplayX = 0;
uint32_t zxDisplayY = 0;
uint32_t zxDisplayBorder = HW_BLACK;
uint8_t *zxDisplayScanPixel;
uint8_t *zxDisplayScanColor;
uint8_t zxDisplayScanToggle = 0; //0..1..0..1 at a 50hz frequency
uint8_t *zxDisplayMem;//[32 * 192 + 32 * 24]; //pixel+color
uint32_t zxDisplayBuffer[16];
extern "C" {
  uint32_t zxDisplayColor(int i) {
    return hw_32bits_colors[i];
  }
}



//changing the timer you can slowdown a game
void zxDisplaySetIntFrequency(int freqHz)
{
  TIMER_50HZ = 480 * 50 / freqHz;
}






//start SPI transfer. wait for other transfer pending
void zxDisplayStartWrite()
{
  while (SPI1CMD & SPIBUSY) {};
  spiSwitchSet(TFT_CS);
  int bitcount = 64 * 8 - 1;//always 32 pixels=64 bytes at a time
  uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));

  SPI1U1 = (SPI1U1 & mask) | (bitcount << SPILMOSI) | (bitcount << SPILMISO);

  SPI1CMD |= SPIBUSY;

}


//before disabling the CS pin spi transfer must be finished
void zxDisplayStopWrite()
{
  while (SPI1CMD & SPIBUSY) {};

}

//set border colod
void zxDisplayBorderSet(int i)
{
  zxDisplayBorder = hw_32bits_colors[i & 7];
}

int zxDisplayBorderGet()
{
  return zxDisplayBorder ;
}


//help function to use serial from z80 core
void zxDisplayWriteSerial(int i)
{
  DEBUG_PRINTLN(i);
}





/*
  to translate y to memory location some bit fiddling is required
  xxyyyzzz = y coordinate
  xx000000->xx00000000000
  00000zzz->00zzz00000000
  00yyy000->00000yyy00000

  xxyyyzzz->xxyyy000->xxyyy00000

*/
void zxDisplayScanPixMem(uint32_t y)
{
  zxDisplayScanPixel = zxDisplayMem + (  ((y & (8 + 16 + 32)) << 2) + ((y & 7) << 8) + ((y & (64 + 128)) << 5));
  zxDisplayScanColor = zxDisplayMem + ((192 * 32) + ((y & (255 - 7)) << 2) );
}


//convert zx color to tft color with attribure found at *zxDisplayScanColor
void zxDisplayScanCol(uint32_t *forecol, uint32_t *backcol)
{

  uint8_t col = *zxDisplayScanColor;
  uint32_t f;
  uint32_t b;
  uint32_t swapcol;

  if (col & 64) //bright
  {
    f = hw_32bits_colors[8 + (col & 7)];
    b = hw_32bits_colors[8 + ((col & 56) >> 3)];
  }
  else
  {
    f = hw_32bits_colors[(col & 7)];
    b = hw_32bits_colors[(col & 56) >> 3];
  }

  if ((col & 128) && (zxDisplayScanToggle & 16))
  { //swap for blink function
    swapcol = f;
    f = b;
    b = swapcol;
  }

  *forecol = f;
  *backcol = b;
}




//every 32 bit register hold 2 pixel ,16 bits each
//there are 4 banks of 4 regs
void zxDisplayOutput(uint8_t pixels, uint32_t forecol, uint32_t backcol, uint8_t regbank)
{
  uint32_t forecolup = forecol << 16; //shift color in 2 upper bytes
  uint32_t backcolup = backcol << 16;


  switch (regbank)
  {
    case 0:
      zxDisplayBuffer[0] = (pixels & 128 ? forecol : backcol) | (pixels & 64 ? forecolup : backcolup);
      zxDisplayBuffer[1] = (pixels & 32 ? forecol : backcol) | (pixels & 16 ? forecolup : backcolup);
      zxDisplayBuffer[2] = (pixels & 8 ? forecol : backcol) | (pixels & 4 ? forecolup : backcolup);
      zxDisplayBuffer[3] = (pixels & 2 ? forecol : backcol) | (pixels & 1 ? forecolup : backcolup);
      break;
    case 1:
      zxDisplayBuffer[4] = (pixels & 128 ? forecol : backcol) | (pixels & 64 ? forecolup : backcolup);
      zxDisplayBuffer[5] = (pixels & 32 ? forecol : backcol) | (pixels & 16 ? forecolup : backcolup);
      zxDisplayBuffer[6] = (pixels & 8 ? forecol : backcol) | (pixels & 4 ? forecolup : backcolup);
      zxDisplayBuffer[7] = (pixels & 2 ? forecol : backcol) | (pixels & 1 ? forecolup : backcolup);
      break;
    case 2:
      zxDisplayBuffer[8] = (pixels & 128 ? forecol : backcol) | (pixels & 64 ? forecolup : backcolup);
      zxDisplayBuffer[9] = (pixels & 32 ? forecol : backcol) | (pixels & 16 ? forecolup : backcolup);
      zxDisplayBuffer[10] = (pixels & 8 ? forecol : backcol) | (pixels & 4 ? forecolup : backcolup);
      zxDisplayBuffer[11] = (pixels & 2 ? forecol : backcol) | (pixels & 1 ? forecolup : backcolup);
      break;
    case 3:
      zxDisplayBuffer[12] = (pixels & 128 ? forecol : backcol) | (pixels & 64 ? forecolup : backcolup);
      zxDisplayBuffer[13] = (pixels & 32 ? forecol : backcol) | (pixels & 16 ? forecolup : backcolup);
      zxDisplayBuffer[14] = (pixels & 8 ? forecol : backcol) | (pixels & 4 ? forecolup : backcolup);
      zxDisplayBuffer[15] = (pixels & 2 ? forecol : backcol) | (pixels & 1 ? forecolup : backcolup);
      break;
  }
}

//copy buffer to spi reg
void zxDisplayOutputReg()
{

  SPI1W0 = zxDisplayBuffer[0];
  SPI1W1 = zxDisplayBuffer[1];
  SPI1W2 = zxDisplayBuffer[2];
  SPI1W3 = zxDisplayBuffer[3];
  SPI1W4 = zxDisplayBuffer[4];
  SPI1W5 = zxDisplayBuffer[5];
  SPI1W6 = zxDisplayBuffer[6];
  SPI1W7 = zxDisplayBuffer[7];
  SPI1W8 = zxDisplayBuffer[8];
  SPI1W9 = zxDisplayBuffer[9];
  SPI1W10 = zxDisplayBuffer[10];
  SPI1W11 = zxDisplayBuffer[11];
  SPI1W12 = zxDisplayBuffer[12];
  SPI1W13 = zxDisplayBuffer[13];
  SPI1W14 = zxDisplayBuffer[14];
  SPI1W15 = zxDisplayBuffer[15];


}



//copy 32 pixels to video
void zxDisplayScan()
{
  uint8_t pixels = 0;
  uint32_t forecol = 0;
  uint32_t backcol = 0;

  if (SPI1CMD & SPIBUSY) return; //dma in use now...



  if (zxDisplayY < 24 || zxDisplayY >= (24 + 192))
  {
    //border only
    zxDisplayOutput(0, zxDisplayBorder, zxDisplayBorder, 0);
    zxDisplayOutput(0, zxDisplayBorder, zxDisplayBorder, 1);
    zxDisplayOutput(0, zxDisplayBorder, zxDisplayBorder, 2);
    zxDisplayOutput(0, zxDisplayBorder, zxDisplayBorder, 3);
    zxDisplayStopWrite();
    zxDisplayOutputReg();
    zxDisplayStartWrite();
    zxDisplayX += 32;
    if (zxDisplayX == 320)
    {
      zxDisplayX = 0;
      zxDisplayY++;
      if (zxDisplayY == 240)
      {
        zxDisplayScanToggle++;//blink!!!
        zxDisplayY = 0;
      }
    }
  }
  else
  {
    if (zxDisplayX == 0 || zxDisplayX == (320 - 32)) //
    { //4 byte from border color and 1 from display color
      zxDisplayOutput(0, zxDisplayBorder, zxDisplayBorder, 0);
      zxDisplayOutput(0, zxDisplayBorder, zxDisplayBorder, 1);
      zxDisplayOutput(0, zxDisplayBorder, zxDisplayBorder, 2);
      zxDisplayOutput(0, zxDisplayBorder, zxDisplayBorder, 3);

      zxDisplayStopWrite();
      zxDisplayOutputReg();
      zxDisplayStartWrite();

      zxDisplayX += 32;
      if (zxDisplayX == 320)
      {
        zxDisplayX = 0;
        zxDisplayY++;
      }
    }

    else
    { //4 byte from screen

      if (zxDisplayX == 32)
      { //begin of line
        zxDisplayScanPixMem(zxDisplayY - 24);
      }

      zxDisplayScanCol(&forecol, &backcol);
      zxDisplayOutput(*zxDisplayScanPixel, forecol, backcol, 0);
      zxDisplayScanPixel++;
      zxDisplayScanColor++;
      zxDisplayScanCol(&forecol, &backcol);
      zxDisplayOutput(*zxDisplayScanPixel, forecol, backcol, 1);
      zxDisplayScanPixel++;
      zxDisplayScanColor++;
      zxDisplayScanCol(&forecol, &backcol);
      zxDisplayOutput(*zxDisplayScanPixel, forecol, backcol, 2);
      zxDisplayScanPixel++;
      zxDisplayScanColor++;
      zxDisplayScanCol(&forecol, &backcol);
      zxDisplayOutput(*zxDisplayScanPixel, forecol, backcol, 3);
      zxDisplayScanPixel++;
      zxDisplayScanColor++;
      zxDisplayX += 32;
      zxDisplayStopWrite();
      zxDisplayOutputReg();
      zxDisplayStartWrite();
    }
  }
}






//called 24000 times per second
//every time 32 pixels are copied to tft
void zxDisplay_timer1_ISR (void) {


  static boolean zxKeyboardReading = false;//true when returning from a keyboard spi cicle
  if (zxKeyboardReading == true )
  { //keyboard reading ongoing?
    zxKeyboardReading = false;
    #ifdef ZXKEYBOARDENABLED
    zxKeyboardStopRead();
    #endif
    zxInterruptPending = 1; //start interrupt 50 times per second AFTER keyboard read
  }


  zxDisplay_timer_counter_50hz++;
  if (zxDisplay_timer_counter_50hz >= TIMER_50HZ)
  {
    zxDisplayStopWrite();
    #ifdef ZXKEYBOARDENABLED
    zxKeyboardStartRead();
    #endif
    zxKeyboardReading = true;
    zxDisplay_timer_counter_50hz = 0;
    return;
  }

  zxDisplayScan();//show 32 pixel at a time

}


void zxDisplayReset()
{
  spiSwitchSet(TFT_CS);

  zxDisplayTft.init();
  zxDisplayTft.setRotation(3);
  zxDisplayTft.setWindow(0, 0, 319, 239);

  zxDisplayX = 0;
  zxDisplayY = 0;
  zxDisplayBorder = HW_BLACK;
  zxDisplayScanToggle = 0;
}





void zxDisplayDisableInterrupt()
{
#ifdef ZXDISPLAYUSEINTERRUPT
  timer1_disable();
#endif
}
void zxDisplayEnableInterrupt()
{

#ifdef ZXDISPLAYUSEINTERRUPT
  timer1_disable();
  timer1_attachInterrupt(zxDisplay_timer1_ISR);
  timer1_isr_init();
  //30ms full display . 32 pixel every tick-->2400 tick for a full frame-->12,5micro seconds to send 32 pixel
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);//80mhz/16=5mhz
  // 2400 blocks of 32 pixel to redraw the screen
  //to get 10hz-->24000 interrupt-->divisor 208
  timer1_write(TIMER_DIVISOR);
#endif
}


void zxDisplaySetup(unsigned char *ram)
{
  spiSwitchSet(TFT_CS);
  zxDisplayTft = TFT_eSPI();
  spiSwitchSet(-1);

  zxDisplayMem = ram;

  zxDisplayReset();
  zxDisplayEnableInterrupt();
}
