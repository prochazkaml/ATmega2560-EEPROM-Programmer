// Parallel EEPROM Programmer for SST29EE020 and compatible devices - for ATmega2560
//
// This version of the software implements:
// - Direct address bus access
// - Hardware UART with 1 Mbps for data transfer to/from PC via USB 2.0
// - Fast page write mode
// - Access via serial monitor
// - Binary data transmission
//
// Usage examples via serial monitor (set baud rate to 1000000):
// - i                  - print "EEPROM Programmer" (for identification)
// - v                  - print firmware version
// - a 0100             - set address bus to 0100 (hex) (for test purposes)
// - d 0000 7fff        - print hex dump of memory addresses 0000-7fff (hex)
// - e                  - perform chip erase
//
// Usage examples for binary data transmissions:
// (do not use these commands via serial monitor !!!)
// - r 0000 3fff        - read memory addresses 0000-3fff (hex) and send as binary data
// - p 0100 013f        - page write binary data to memory page 0100-013f (hex)
// - l                  - lock EEPROM (enable write protection)
// - u                  - unlock EEPROM (disable write protection)
// 
// See README.md for more info.
//
// 2019 by Stefan Wagner, 2022 Michal Procházka
// Project Files (EasyEDA): https://easyeda.com/wagiminator
// Project Files (Github):  https://github.com/wagiminator
//                          https://github.com/prochazkaml
// License: http://creativecommons.org/licenses/by-sa/3.0/

#ifndef __AVR_ATmega2560__
#define __AVR_ATmega2560__
#endif

// Libraries
#include <avr/io.h>
#include <util/delay.h>

// Identifiers
#define VERSION         "1.0"
#define IDENT           "EEPROM Programmer"

// Pin and port definitions
#define CONTROL_REG     DDRG
#define CONTROL_PORT    PORTG
#define DATA_REG        DDRL
#define DATA_PORT       PORTL
#define DATA_INPUT      PINL
#define LED_REG         DDRB
#define LED_PORT        PORTB
#define ADDL_REG        DDRA
#define ADDL_PORT       PORTA
#define ADDH_REG        DDRC
#define ADDH_PORT       PORTC
#define ADDE_REG        DDRB
#define ADDE_PORT       PORTB

#define CHIP_ENABLE     (1<<PG2)              // EEPROM !CE
#define WRITE_ENABLE    (1<<PG0)              // EEPROM !WE
#define OUTPUT_ENABLE   (1<<PG1)              // EEPROM !OE

#define LED_PIN         (1<<PB7)              // read indicator LED  

// Macros
#define LEDon           LED_PORT |=  LED_PIN
#define LEDoff          LED_PORT &= ~LED_PIN

#define enableChip      CONTROL_PORT &= ~CHIP_ENABLE
#define disableChip     CONTROL_PORT |=  CHIP_ENABLE
#define enableOutput    CONTROL_PORT &= ~OUTPUT_ENABLE
#define disableOutput   CONTROL_PORT |= OUTPUT_ENABLE
#define enableWrite     CONTROL_PORT &= ~WRITE_ENABLE
#define disableWrite    CONTROL_PORT |=  WRITE_ENABLE

#define readDataBus     DATA_INPUT
#define setDataBusRead  { DATA_REG = 0x00; }
#define setDataBusWrite { DATA_REG = 0xFF; }

#define delay125ns      {asm volatile("nop"); asm volatile("nop");}

// Buffers
uint8_t pageBuffer[4096];                     // page buffer
char    cmdBuffer[32];                        // command buffer

// -----------------------------------------------------------------------------
// UART Implementation (1M BAUD)
// -----------------------------------------------------------------------------

#define UART_available()  (UCSR0A & (1<<RXC0))  // check if byte was received
#define UART_read()       UDR0                  // read received byte

// UART init
void UART_init(void) {
  DDRE |= (1 << PE1);
  UCSR0B = (1<<RXEN0)  | (1<<TXEN0);          // enable RX and TX
  UCSR0C = (3<<UCSZ00);                       // 8 data bits, no parity, 1 stop bit
  UBRR0 = 1;
}

// UART send data byte
void UART_write(uint8_t data) {
  while(!(UCSR0A & (1<<UDRE0)));              // wait until previous byte is completed
  UDR0 = data;                                // send data byte
}

// UART send string
void UART_print(const char *str) {
  while (*str) UART_write(*str++);            // write characters of string
}

// UART send string with new line
void UART_println(const char *str) {
  UART_print(str);                            // print string
  UART_write('\n');                           // send new line command
}

// Reads command string via UART
void readCommand() {
  for(uint8_t i=0; i< 32; i++) cmdBuffer[i] = 0;  // clear command buffer
  char c; uint8_t idx = 0;                        // initialize variables
  
  // Read serial data until linebreak or buffer is full
  do {
    if(UART_available()) {
      c = UART_read();
      cmdBuffer[idx++] = c;
    }
  } while (c != '\n' && idx < 32);
  cmdBuffer[idx - 1] = 0;                     // change last newline to '\0' termination
}

// -----------------------------------------------------------------------------
// String Converting
// -----------------------------------------------------------------------------

// Convert byte nibble into hex character and print it via UART
void printNibble(uint8_t nibble) {
  uint8_t c;
  if (nibble <= 9)  c = '0' + nibble;
  else              c = 'a' + nibble - 10;
  UART_write(c);
}

// Convert byte into hex characters and print it via UART
void printByte(uint8_t value) {
  printNibble (value >> 4);
  printNibble (value & 0x0f);
}

// Convert long into hex characters and print it via UART
void printLong(uint32_t value) {
  printByte(value >> 24);
  printByte(value >> 16);
  printByte(value >> 8);
  printByte(value);
}

// Convert character representing a hex nibble into 4-bit value
uint8_t hexDigit(char c) {
  if      (c >= '0' && c <= '9') return c - '0';
  else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F') return c - 'A' + 10; 
  else return 0;
}

// Convert string containing a hex byte into 8-bit value
uint8_t hexByte(char* a) {
  return ((hexDigit(a[0]) << 4) + hexDigit(a[1]));
}

// Convert string containing a hex long into 32-bit value
uint32_t hexLong(char* data) {
  uint32_t val = 0;

  uint8_t *valptr = (uint8_t*)(&val);

  valptr[0] = hexByte(data + 6);
  valptr[1] = hexByte(data + 4);
  valptr[2] = hexByte(data + 2);
  valptr[3] = hexByte(data + 0);

  return val;
}

// -----------------------------------------------------------------------------
// Low Level Communication with EEPROM
// -----------------------------------------------------------------------------

// Set the address
void setAddress (uint32_t addr) { 
  ADDL_PORT = addr;
  ADDH_PORT = addr >> 8;
  ADDE_PORT = (addr >> 16) & 0xF;
}

// Write a single byte to the EEPROM at the given address
void setByte (uint32_t addr, uint8_t value) {
  setAddress(addr);

  DATA_PORT = value;

  // write data byte to EEPROM
  enableWrite;                                // set low for write enable
  delay125ns;                                 // wait >100ns
  disableWrite;                               // set high to initiate write cycle
}

// Wait for write cycle to finish using data polling
void waitWriteCycle (uint8_t writtenByte) {
  enableOutput;                               // EEPROM output enable
  delay125ns;                                 // wait for output valid
  while (writtenByte != (readDataBus));       // wait until valid reading
  disableOutput;                              // EEPROM output disable
}

// Read a byte from the EEPROM at the given address
uint8_t readDataByte(uint32_t addr) { 
  enableOutput;                               // EEPROM output enable
  setAddress (addr);                          // set address bus
  delay125ns;                                 // wait for output valid
  uint8_t value = readDataBus;                // read data byte from data bus
  disableOutput;                              // EEPROM output disable 
  return value;                               // return byte
}

// Write a byte to the EEPROM at the given address
void writeDataByte (uint32_t addr, uint8_t value) {
  setDataBusWrite;                            // set data bus pins as output  
  setByte (addr, value);                      // write byte to EEPROM
  setDataBusRead;                             // release data bus (set as input)
  waitWriteCycle (value);                     // wait for write cycle to finish
}

// Write up to 256 bytes to EEPROM; bytes have to be page aligned
// Compatible with SST29*E0*0 devices
void writePageEEPROM (uint32_t addr, uint8_t count) {
  if (!count) return;                         // return if no bytes to write
  LEDon;                                      // turn on write LED
  setDataBusWrite;                            // set data bus pins as output
  
  for (uint8_t i=0; i<count; i++) {           // write <count> numbers of bytes 
    setByte (addr++, pageBuffer[i]);          // to EEPROM
  }

  setDataBusRead;                             // release data bus (set as input)
  waitWriteCycle (pageBuffer[count-1]);       // wait for write cycle to finish
  LEDoff;                                     // turn off write LED
}

// Write up to 256 bytes to flash, pages don't matter
// Compatible with SST39*F0*0 devices
void writePageFlash (uint32_t addr, uint8_t count) {
  if (!count) return;                         // return if no bytes to write
  LEDon;                                      // turn on write LED
  setDataBusWrite;                            // set data bus pins as output
  
  for (uint8_t i=0; i<count; i++) {           // write <count> numbers of bytes to flash
    setByte (0x5555, 0xaa);                   // write code sequence
    setByte (0x2aaa, 0x55);
    setByte (0x5555, 0xa0);
    setByte (addr++, pageBuffer[i]);
    _delay_us(20);
  }

  setDataBusRead;                             // release data bus (set as input)
  LEDoff;                                     // turn off write LED
}

void (*writePage)(uint32_t, uint8_t) = &writePageEEPROM;

// -----------------------------------------------------------------------------
// High Level EEPROM Functions
// -----------------------------------------------------------------------------

// Write the special six-byte code to perform a chip erase
void chipErase() {
  setDataBusWrite;                            // set data bus pins as output 
  setByte (0x5555, 0xaa);                     // write code sequence
  setByte (0x2aaa, 0x55);
  setByte (0x5555, 0x80);
  setByte (0x5555, 0xaa);
  setByte (0x2aaa, 0x55);
  setByte (0x5555, 0x10);
  setDataBusRead;                             // release data bus (set as input)
  _delay_ms(100);                             // wait enough time
}

// Write the special six-byte code to turn off software data protection
void disableWriteProtection() {
  setDataBusWrite;                            // set data bus pins as output 
  setByte (0x5555, 0xaa);                     // write code sequence
  setByte (0x2aaa, 0x55);
  setByte (0x5555, 0x80);
  setByte (0x5555, 0xaa);
  setByte (0x2aaa, 0x55);
  setByte (0x5555, 0x20);
  setDataBusRead;                             // release data bus (set as input)
  _delay_ms(10);                              // wait write cycle time
}

// Write the special three-byte code to turn on software data protection
void enableWriteProtection() {
  setDataBusWrite;                            // set data bus pins as output
  setByte (0x5555, 0xaa);                     // write code sequence
  setByte (0x2aaa, 0x55);
  setByte (0x5555, 0xa0);
  setDataBusRead;                             // release data bus (set as input)
  _delay_ms(10);                              // wait write cycle time
}

// Read content of EEPROM and print hex dump via UART
void printContents(uint32_t addr, uint32_t count) {
  static char ascii[17];                      // buffer string
  ascii[16] = 0;                              // string terminator
  LEDon;                                      // turn on read LED
  for (uint32_t base = 0; base < count; base += 16) {
    printLong(base); UART_print(":  ");
    for (uint8_t offset = 0; offset <= 15; offset += 1) {
      uint8_t databyte = readDataByte(addr + base + offset);
      if (databyte > 31 && databyte < 127) ascii[offset] = databyte;
      else ascii[offset] = '.';
      printByte(databyte);
      UART_print(" ");
    }
    UART_print(" "); UART_println(ascii);
  }
  LEDoff;                                     // turn off read LED
}

// Read content of EEPROM and send it as binary data via UART
void readBinary(uint32_t addr, uint32_t count) {
  LEDon;
  while (count) {
    UART_write(readDataByte(addr++));
    count--;
  }
  LEDoff;
}

// Write binary data from UART to a memory page
void writePageBinary(uint32_t startAddr, uint8_t count) {
  for (uint8_t i=0; i<count; i++) {
    while (!UART_available());
    pageBuffer[i] = UART_read();
  }
  (*writePage)(startAddr, count);
}

// -----------------------------------------------------------------------------
// Main Function
// -----------------------------------------------------------------------------

int main(void) {
  // Setup EEPROM control pins
  CONTROL_PORT |= (WRITE_ENABLE | OUTPUT_ENABLE);   // set high to disable
  CONTROL_REG  |= (WRITE_ENABLE | OUTPUT_ENABLE | CHIP_ENABLE);   // set pins as output
  CONTROL_PORT &= ~CHIP_ENABLE;                     // set low to enable
  
  // Setup LED pins
  LED_REG   |=  (LED_PIN);                          // set LED pin as output
  LED_PORT  &= ~(LED_PIN);                          // turn off LED
  
  // Setup data bus
  DATA_REG = 0x00;

  // Setup address bus
  ADDL_REG = 0xFF;
  ADDH_REG = 0xFF;
  ADDE_REG = 0x0F;

  // Setup UART for 500k BAUD
  UART_init();

  // Loop
  while(1) {
    UART_println("Ready");
    readCommand();
    char cmd = cmdBuffer[0];
    uint32_t startAddr = hexLong(cmdBuffer+2);
    uint32_t endAddr   = hexLong(cmdBuffer+11);
    if (endAddr < startAddr) endAddr = startAddr;
    uint32_t dataLength = endAddr - startAddr + 1;

    switch(cmd) {
      case 'i':   UART_println(IDENT); break;
      case 'v':   UART_println(VERSION); break;
      case 'a':   setAddress(startAddr); break;
      case 'd':   printContents(startAddr, dataLength); break;
      case 'r':   readBinary(startAddr, dataLength); break;
      case 'p':   writePageBinary(startAddr, dataLength); break;
      case 'l':   enableWriteProtection(); break;
      case 'u':   disableWriteProtection(); break;
      case 'e':   chipErase(); break;
      case 'E':   writePage = &writePageEEPROM; break;
      case 'F':   writePage = &writePageFlash; break;
      default:    break;    
    }
  }
}
