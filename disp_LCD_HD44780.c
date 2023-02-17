//www.DFRobot.com
//last updated on 21/12/2011
//Tim Starling Fix the reset bug (Thanks Tim)
//wiki doc http://www.dfrobot.com/wiki/index.php?title=I2C/TWI_LCD1602_Module_(SKU:_DFR0063)
//Support Forum: http://www.dfrobot.com/forum/
//Compatible with the Arduino IDE 1.0
//Library version:1.1


#include "config.h"
#ifdef LCD_USE_1602_LCD_MODULE

#include "lcd_interface.h"

#include <util/delay.h>
#include <avr/pgmspace.h>

#include "i2c_master.h"

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for backlight control
#define LCD_BACKLIGHT     _BV(LCD_BIT_BACKLIGHT)
#define LCD_NOBACKLIGHT   0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

uint8_t backslashChar[8] = {
    0b00000,
    0b10000,
    0b01000,
    0b00100,
    0b00010,
    0b00001,
    0b00000,
    0b00000
};


// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

uint8_t _addr;
uint8_t _displayfunction;
uint8_t _displaycontrol;
uint8_t _displaymode;
uint8_t _numlines;
uint8_t _cols;
uint8_t _rows;
uint8_t _backlightval;

void send(uint8_t value, uint8_t mode);
void write4bits(uint8_t value, uint8_t mode);
void expanderWrite(uint8_t _data);
void pulseEnable(uint8_t _data);
void lcd_begin(uint8_t lcd_addr, uint8_t cols, uint8_t lines, uint8_t dotsize);
void lcd_display();
void lcd_clear();
void lcd_home();
void lcd_createChar(uint8_t location, uint8_t charmap[]);

void lcd_write(uint8_t value) {
  // replace backslash with redefined char
  if (value == '\\') value = 0x01;
	send(value, _BV(LCD_BIT_RS));
}

// mid level commands, for sending data/cmds 

void command(uint8_t value) {
	send(value, 0);
}

void lcd_init(uint8_t lcd_addr) {
  lcd_begin(lcd_addr, MAX_LCD_LINE_LEN, LCD_NUM_LINES, LCD_5x8DOTS);
  // can't define this as the zeroth character as zero is null in strings :)! :)
  lcd_createChar(1, backslashChar);
}

void lcd_begin(uint8_t lcd_addr, uint8_t cols, uint8_t lines, uint8_t dotsize) {
  _addr = lcd_addr;
  _cols = cols;
  _rows = lines;
  _backlightval = LCD_NOBACKLIGHT;
  i2c_init();
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  

	if (lines > 1) {
		_displayfunction |= LCD_2LINE;
	}
	_numlines = lines;

	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != 0) && (lines == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	_delay_ms(50); 
  
	// Now we pull both RS and R/W low to begin commands
	lcd_noBacklight();	// reset expander and turn backlight off (Bit 8 =1)
	_delay_ms(1000);

  	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46
	
	  // we start in 8bit mode, try to set 4 bit mode
   write4bits(0x03, 0);
   _delay_us(4500); // wait min 4.1ms
   
   // second try
   write4bits(0x03, 0);
   _delay_us(4500); // wait min 4.1ms
   
   // third go!
   write4bits(0x03, 0); 
   _delay_us(150);
   
   // finally, set to 4-bit interface
   write4bits(0x02, 0); 

	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);  
	
	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	lcd_display();

	// clear it off
	lcd_clear();

	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);

	lcd_home();
}

// high level commands, for the user!
void lcd_clear() {
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	_delay_us(2000);  // this command takes a long time!
}

void lcd_home() {
	command(LCD_RETURNHOME);  // set cursor position to zero
	_delay_us(2000);  // this command takes a long time!
}

void lcd_setCursor(uint8_t col, uint8_t row) {
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row > _numlines ) {
		row = _numlines - 1;    // we count rows starting w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void lcd_noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void lcd_noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void lcd_cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void lcd_createChar(uint8_t location, uint8_t charmap[]) {
  int i;
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (i = 0; i < 8; i++) {
		lcd_write(charmap[i]);
	}
}

// Turn the (optional) backlight off/on
void lcd_noBacklight(void) {
	_backlightval = LCD_NOBACKLIGHT;
	expanderWrite(_backlightval);
}

void lcd_backlight(void) {
	_backlightval = LCD_BACKLIGHT;
	expanderWrite(_backlightval);
}

void lcd_print(char* msg) {
  uint8_t v;
  while((v = *msg++)) {
    lcd_write(v);
  }
}

// low level data pushing commands

// write either command or data
void send(uint8_t value, uint8_t mode) {
	uint8_t highnib = value >> 4;
	uint8_t lownib = value & 0x0f;
  write4bits(highnib, mode);
	write4bits(lownib, mode); 
}

void write4bits(uint8_t value, uint8_t mode) {
  uint8_t res = 0;
  if (value & 0x1) res |= _BV(LCD_BIT_DATA0);
  if (value & 0x2) res |= _BV(LCD_BIT_DATA1);
  if (value & 0x4) res |= _BV(LCD_BIT_DATA2);
  if (value & 0x8) res |= _BV(LCD_BIT_DATA3);
  res |= mode | _backlightval;

	expanderWrite(res);
	pulseEnable(res);
}

void expanderWrite(uint8_t _data){                                        
	i2c_start((_addr << 1) | I2C_WRITE);
	i2c_write(_data);
	i2c_stop();   
}

void pulseEnable(uint8_t _data){
	expanderWrite(_data | _BV(LCD_BIT_EN));	// En high
	_delay_us(1);		// enable pulse must be >450ns
	
	expanderWrite(_data & ~_BV(LCD_BIT_EN));	// En low
	_delay_us(50);		// commands need > 37us to settle
} 

#endif
