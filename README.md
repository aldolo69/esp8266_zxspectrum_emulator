# esp8266_zxspectrum_emulator
ZX Spectrum emulator with esp8266 and ili9341 display.

Compiled using https://github.com/uli/Arduino_nowifi

Please check https://hackaday.io/project/169711-zx-spectrum-emulator-with-esp8266 for the libs i've used

* audio
* joystick
* load .z80 from SD
* load .z80 from EEPROM
* save .z80 to SD

![Screenshot](zxpicture.jpg)

 
[![Alt text](https://img.youtube.com/vi/D_M-rMHq4L0/0.jpg)](https://www.youtube.com/watch?v=D_M-rMHq4L0)

Parts list:

1 es8266 nodemcu

1 ili9341 display with sd card adapter

2 4081 and gate

2 4017 decade counter

1 4503 3state buffer

1 npn transistor

1 40ohm speaker

1 2.2kohm resistor

5 100nf decoupling capacitors

6 10kohm resistors

15 1n4148 diodes

47 buttons

#update 2021-08
After an update of the regular 8266 core the nowifi core compile failed. Had to change 2 things:

- to compile use extesa compiler 2.5. look in arduino/packages/esp8266/tools/xtesa-lx106-elf-gcc.
if there is a versione > 2.5.0 then rename the newer folders (in example 3.0.0xxx to 0.3.0.0xxxx)

- in case of missing .ld file change the platform.txt file to add the absolute path to the linker script
compiler.c.elf.flags=-g {compiler.warning_flags} -O2 -nostdlib -Wl,--no-check-sections -u call_user_start -u _printf_float -u _scanf_float -Wl,-static "-L{compiler.libc.path}/lib" "-TC:/Users/it03281/Documents/Arduino/hardware/esp8266com/Arduino_nowifi-sdknowifi/tools/sdk/ld/{build.flash_ld}" -Wl,--gc-sections -Wl,-wrap,system_restart_local -Wl,-wrap,spi_flash_read



![Screenshot](zx_esp8266.JPG)
![Screenshot](zx_keyboard_joystick.JPG)
![Screenshot](zx_display_sdcard.JPG)
![Screenshot](zxkeyboard.png)


