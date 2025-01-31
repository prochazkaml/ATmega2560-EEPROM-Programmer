# Parallel EEPROM Programmer based on ATmega2560
Parallel EEPROM Programmer for SST29EE020 (and possibly others which use 128 byte pages, the protocol for programming EEPROMs is standard) featuring:
- GUI-based front-end written in Python
- Ability to program memories up to 1 MB in size with the current pin configuration, theoretically up to 4 GB possible (although good luck with the 500 Kbps transfer rate)
- Possibility to access the programmer via a serial monitor
- Hardware UART with 500 Kbps for data transfer to/from PC via USB
- Utilizing the fast page write mode of the EEPROM
- Binary data transmission

![EEPROM_hw1.jpeg](https://raw.githubusercontent.com/prochazkaml/ATmega2560-EEPROM-Programmer/master/documentation/EEPROM_hw1.jpeg)
![EEPROM_hw2.jpeg](https://raw.githubusercontent.com/prochazkaml/ATmega2560-EEPROM-Programmer/master/documentation/EEPROM_hw2.jpeg)

# Hardware
The heart of the EEPROM programmer is an ATmega2560 microcontroller. The address bus of the EEPROM (up to 20 bit in the current configuration, with modifications 32 bits are possible) as well as the data bus is controlled directly via the pins of the ATmega. The data connection to the PC runs via the hardware UART interface of the ATmega transfering the data in binary format with up to 500 Kbps.

# What are EEPROMs?
The EEPROM is accessed like a Static RAM for the read or write cycle without the need for external components. Many EEPROMs contain a 128-byte page register to allow writing of up to 128 bytes simultaneously. The end of a write cycle can be detected by data polling. Once the end of a write cycle has been detected a new access for a read or write can begin.
An optional software data protection mechanism is available to guard against inadvertent writes.

## Read Access
The EEPROM is accessed like a Static RAM. When !CE and !OE are low and !WE is high, the data stored at the memory location determined by the address pins is asserted on the outputs. The outputs are put in the high impedance state when either !CE or !OE is high.

## Write Access
A low pulse on the !WE or !CE input with !CE or !WE low (respectively) and !OE high initiates a write cycle. The address is latched on the falling edge of !CE or !WE, whichever occurs last. The data is latched by the first rising edge of !CE or !WE. Once a byte write has been started it will automatically time itself to completion.
The page write operation allows 1 to 64 bytes of data to be written into the device during a single internal programming period. A page write operation is initiated in the same manner as a byte write; the first byte written can then be followed by 1 to 127 additional bytes.
Each successive byte must be written within 150 μs of the previous byte. All bytes during a page write operation must reside on the same page as defined by the state of the A7 - A(max) inputs. For each WE high to low transition during the page write operation, A7 - A14 must be the same.
The A0 to A6 inputs are used to specify which bytes within the page are to be written. The bytes may be loaded in any order and may be altered within the same load period. Only bytes which are specified for writing will be written; unnecessary cycling of other bytes within the page does not occur.

## Data Polling
The EEPROM features DATA Polling to indicate the end of a write cycle. During a byte or page write cycle an attempted read of the last byte written will result in the complement of the written data to be presented on I/O7. Once the write cycle has been completed, true data is valid on all outputs, and the next write cycle may begin. DATA Polling may begin at anytime during the write cycle.
In addition to DATA Polling the EEPROM provides another method for determining the end of a write cycle. During the write operation, successive attempts to read data from the device will result in I/O6 toggling between one and zero. Once the write has completed, I/O6 will stop toggling and valid data will be read. Reading the toggle bit may begin at any time during the write cycle.

# Software
## Implementation
On the microcontroller side, data is received via UART and written to the EEPROM according to the data sheet or vice versa. The programmer is controlled with simple commands, which are also sent via the serial interface:

|Command|Function|
|:-|:-|
|i                |Print "EEPROM Programmer" (for identification)|
|v                |Print firmware version|
|a 00000100       |Set address bus to 00000100 (hex) (for test purposes)|
|d 00000000 0003ffff      |Print hex dump of memory addresses 0000000-0003ffff (hex) = 256 KiB|
|f 00001000 00001fff ff   |Fill memory (00001000-00001fff) with value ff (hex)|
|r 00000000 0003ffff      |Read memory addresses 0000000-0003ffff (hex) and send as binary data|
|p 00000100 0000017f      |Page write binary data to memory page 00000100-0000017f (bytes must follow)|
|l                |Lock EEPROM (enable write protection)|
|u                |Unlock EEPROM (disable write protection)|
|e                |Perform chip erase|

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
