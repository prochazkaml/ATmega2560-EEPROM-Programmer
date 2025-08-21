# Parallel EEPROM/Flash Programmer based on ATmega2560 (Arduino Mega)
Parallel EEPROM/Flash Programmer for many different devices, featuring:
- GUI-based front-end written in Python
- Ability to program memories up to 1 MB in size with the current pin configuration, theoretically up to 4 GB possible (although good luck with the 500 Kbps transfer rate)
- Support for programing devices with 1-byte pages all the way up to devices with 4096-byte pages
- Possibility to access the programmer via a serial monitor
- Hardware UART with 500 Kbps for data transfer to/from PC via USB
- Utilizing the fast page write mode of the EEPROM
- Binary data transmission

# Hardware
The heart of the EEPROM programmer is an ATmega2560 microcontroller. The address bus of the EEPROM (up to 20 bit in the current configuration, with modifications 32 bits are possible) as well as the data bus is controlled directly via the pins of the ATmega. The data connection to the PC runs via the hardware UART interface of the ATmega transfering the data in binary format with up to 500 Kbps.

![EEPROM_hw1.jpeg](https://raw.githubusercontent.com/prochazkaml/ATmega2560-EEPROM-Programmer/master/documentation/EEPROM_hw1.jpeg)
![EEPROM_hw2.jpeg](https://raw.githubusercontent.com/prochazkaml/ATmega2560-EEPROM-Programmer/master/documentation/EEPROM_hw2.jpeg)

The connections from the MCU to the target device is as follows:

|ATmega2560|Arduino Mega equiv. pins|Target pins|
|:-|:-|:-|
|PA0..7|22..29|Address lines A0..7|
|PC0..7|37..30|Address lines A8..15|
|PB0..3|53..50|Address lines A16..19|
|PL0..7|49..42|Data lines Q0..7|
|PG0|41|Write enable (#WE)|
|PG1|40|Output enable (#OE)|
|PG2|39|Chip enable (#CE)|

All of the required pins are located on the largest header of the Mega. If your target device has less address lines, just leave them disconnected on the Mega.

# Tested target devices
Below is a list of all target devices that have been tested with this programmer, along with the recommended settings. You are welcome to expand this list in case you find an unmentioned device that works with this programmer.

|Device name|Manufacturer|Voltage|Size|Type|Page size|
|:-|:-|:-|:-|:-|:-|
|SST29EE020|SST/Microchip|5 V|256k|EEPROM|128 bytes|
|SST39LF040|SST/Microchip|**3.3 V**|512k|Flash|does not matter, 4096 bytes is the fastest|

**NOTE: MAKE SURE TO USE PROPER 3.3 V LEVEL SHIFTERS WHEN TARGETING 3.3 V DEVICES, OTHERWISE YOU'LL FRY THEM!**

# Software
## Implementation
On the microcontroller side, data is received via UART and written to the EEPROM according to the data sheet or vice versa. The programmer is controlled with simple commands, which are also sent via the serial interface:

|Command|Function|
|:-|:-|
|i                |Print "EEPROM Programmer" (for identification)|
|v                |Print firmware version|
|a 00000100       |Set address bus to 00000100 (hex) (for test purposes)|
|d 00000000 0003ffff      |Print hex dump of memory addresses 0000000-0003ffff (hex) = 256 KiB|
|r 00000000 0003ffff      |Read memory addresses 0000000-0003ffff (hex) and send as binary data|
|p 00000100 0000017f      |Write binary data to memory page 00000100-0000017f (bytes must follow)|
|l                |Lock EEPROM (enable write protection)|
|u                |Unlock EEPROM (disable write protection)|
|e                |Perform chip erase (necessary for Flash)|
|E                |Switch to EEPROM-compatible write mode (page-based)|
|F                |Switch to Flash-compatible write mode (byte-based)|

Any serial monitor (set BAUD rate to 500000) can be used for control from the PC. However, in order to use the full capabilities, it is recommended to use the attached Python scripts. The script "eepromgui.py" offers a simple graphical user interface and functions for reading and writing binary files as well as for displaying the EEPROM content. The scripts have only been tested on Linux, but should work on all operating systems. A [driver for the CH330N/CH340N](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all#drivers-if-you-need-them) may be required for Windows if you are using cheap Arduino Mega clones.

![EEPROM_sw1.png](https://raw.githubusercontent.com/prochazkaml/ATmega2560-EEPROM-Programmer/master/documentation/EEPROM_sw1.png)
![EEPROM_sw2.png](https://raw.githubusercontent.com/prochazkaml/ATmega2560-EEPROM-Programmer/master/documentation/EEPROM_sw2.png)

## Compiling and Uploading
### If using the Arduino IDE
- Go to **Tools -> Board -> Arduino AVR Boards** and select **Arduino Mega or Mega 2560**.
- Go to **Tools -> Processor** and select **ATmega2560 (Mega 2560)**.
- Connect your Arduino Mega to your PC.
- Open EEPROM_Programmer sketch and click **Upload**.

### If using the precompiled hex-file
- Make sure you have installed [avrdude](https://learn.adafruit.com/usbtinyisp/avrdude).
- Connect your Arduino Mega to your PC.
- Open a terminal.
- Navigate to the folder with the hex-file.
- Execute the following command (if necessary replace "/dev/ttyUSB0" with your port, on Windows, it would be eg. COM3):
  ```
  avrdude -c wiring -p m2560 -P /dev/ttyUSB0 -b 115200 -U flash:w:eeprom_programmer_m2560.hex -D
  ```

### If using the makefile (Linux/Mac)
- Make sure you have installed [avr-gcc toolchain and avrdude](http://maxembedded.com/2015/06/setting-up-avr-gcc-toolchain-on-linux-and-mac-os-x/).
- Connect your Arduino Mega to your PC.
- Open a terminal.
- Navigate to the folder with the makefile and the Arduino sketch.
- Run "make upload" to compile and upload the firmware.

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
