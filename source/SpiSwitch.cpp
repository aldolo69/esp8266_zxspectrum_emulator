/*in case of new spi devices add here the set/reset code for the new CS pin
 */


#include "GlobalDef.h"
#include "SpiSwitch.h"
#include <Arduino.h>



//define cs pins as output and reset them all
void spiSwitchSetup()
{
  spiSwitchReset();
  pinMode(SD_CS, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(KEYB_CS, OUTPUT);
  
  }


//select one device only
void spiSwitchSet(int cs)
{
  spiSwitchReset();
  switch (cs)
  {
    case SD_CS:
      digitalWrite(SD_CS, LOW);
      break;
    case TFT_CS:
      digitalWrite(TFT_CS, LOW);
      break;
    case KEYB_CS:
      digitalWrite(KEYB_CS, LOW);
      break;
  }


}

//turn off all spi devices
void spiSwitchReset()
{
  digitalWrite(SD_CS, HIGH);         
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(KEYB_CS, HIGH);
}


