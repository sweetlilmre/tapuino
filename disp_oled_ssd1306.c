#include "config.h"
#ifdef LCD_USE_SSD1306_OLED_MODULE

#include <string.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "i2c_master.h"
#include "font8x8.h"

#define NUM_COLS  16
#define NUM_ROWS  2
#define NUM_CHARS NUM_COLS * NUM_ROWS

static uint8_t _addr; // I2C address
static uint8_t _row, _col;
static uint8_t _buffer[NUM_CHARS];
static uint8_t _displayCursor;

void write_raw(uint8_t value, uint8_t cursor);
void ssd1306_fillscreen(uint8_t fill);
void ssd1306_send_command_start(void);
void ssd1306_send_command(uint8_t command);
void ssd1306_send_data_start(void);

#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETSTARTLINE        0x40
#define SSD1306_CHARGEPUMP          0x8D
#define SSD1306_MEMORYMODE          0x20
#define SSD1306_SEGREMAP            0xA0
#define SSD1306_COMSCANDEC          0xC8
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETCONTRAST         0x81
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETVCOMDETECT       0xDB
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_DISPLAYON           0xAF
#define SSD1306_SETSTARTPAGE        0xB0
#define SSD1306_LOWCOLUMNADDR       0x00
#define SSD1306_HIGHCOLUMNADDR      0x10

// Init Sequence
const uint8_t ssd1306_init_sequence [] PROGMEM = {
  SSD1306_DISPLAYOFF,                   // 0xAE Display OFF (sleep mode)
  SSD1306_SETDISPLAYCLOCKDIV,   0x80,   // 0xD5 Set display clock divide ratio/oscillator frequency
#if defined LCD_SSD1306_128x64  
  SSD1306_SETMULTIPLEX,         0x3F,   // 0xA8 Set multiplex ratio (1 to 64)
#elif defined LCD_SSD1306_128x32
  SSD1306_SETMULTIPLEX,         0x1F,   // 0xA8 Set multiplex ratio (1 to 64)
#endif
  SSD1306_SETDISPLAYOFFSET,     0x00,   // 0xD3 Set display offset. 00 = no offset
  SSD1306_SETSTARTLINE | 0x00,          // 0x40 Set start line address
  SSD1306_CHARGEPUMP,           0x14,   // 0x8D Set DC-DC enable: internal VCC
  SSD1306_MEMORYMODE,           0x10,   // 0x20 Set Memory Addressing Mode
                                        //      00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
                                        //      10=Page Addressing Mode (RESET); 11=Invalid
  SSD1306_SEGREMAP | 0x01,              // 0xA0 Set Segment Re-map. A0=address mapped; A1=address 127 mapped. 
  SSD1306_COMSCANDEC,                   // 0xC8 Set COM Output Scan Direction - descending
#if defined LCD_SSD1306_128x64  
#ifdef LCD_SSD1306_BIG_FONTS
  SSD1306_SETCOMPINS,           0x02,   // 0xDA Set com pins hardware configuration
#else
  SSD1306_SETCOMPINS,           0x12,   // 0xDA Set com pins hardware configuration
#endif
#elif defined LCD_SSD1306_128x32
  SSD1306_SETCOMPINS,           0x02,   // 0xDA Set com pins hardware configuration
#endif
  
  SSD1306_SETCONTRAST,          0x3F,   // 0x81 Set contrast control register
  SSD1306_SETPRECHARGE,         0x22,   // 0xD9 Set pre-charge period
  SSD1306_SETVCOMDETECT,        0x20,   // 0xDB Set vcomh 0x20,0.77xVcc
  SSD1306_DISPLAYALLON_RESUME,          // 0xA4 Output RAM to Display 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
  SSD1306_NORMALDISPLAY,                // 0xA6 Set display mode. A6=Normal; A7=Inverse
  SSD1306_SETSTARTPAGE | 0x00,          // Set Page Start Address for Page Addressing Mode, 0-7
  SSD1306_LOWCOLUMNADDR,                // ---set low column address
  SSD1306_HIGHCOLUMNADDR,               // ---set high column address
  SSD1306_DISPLAYON                     // 0xAF Display ON in normal mode
};


void lcd_init(uint8_t lcd_addr)
{
  uint8_t i;
  _addr = lcd_addr;
  _row = 0;
  _col = 0;
  _displayCursor = 0;
  
  i2c_init();
  _delay_ms(100);

  for (i = 0; i < sizeof (ssd1306_init_sequence); i++) {
    ssd1306_send_command(pgm_read_byte(&ssd1306_init_sequence[i]));
  }
  ssd1306_fillscreen(0);
  memset(_buffer, 32, NUM_CHARS);
}

void lcd_cursor() {
  uint8_t curValue = _buffer[_row * NUM_COLS + _col];
  _displayCursor = 1;
  write_raw(curValue, 1);
}

void lcd_noCursor(){
  uint8_t curValue = _buffer[_row * NUM_COLS + _col];
  _displayCursor = 0;
  write_raw(curValue, 0);
}

void lcd_backlight() {
  
}

void lcd_noBacklight() {
  
}


void write_raw(uint8_t value, uint8_t cursor) {
  uint8_t i, v, col;
  uint8_t c = value - 32;
 
  col = _col << 3; // convert to pixel: character column * 8
  ssd1306_send_command_start();
  i2c_write(SSD1306_SETSTARTPAGE + _row);
  i2c_write((col & 0x0f) | SSD1306_LOWCOLUMNADDR);
  i2c_write(((col & 0xf0) >> 4) | SSD1306_HIGHCOLUMNADDR);
    
  ssd1306_send_data_start();
  // write 1 column of the character per iteration
  for (i = 0; i < 8; i++)
  {
    v = pgm_read_byte(&font8x8[c * 8 + i]);
    // add underline for cursor: last pixel in the column
    if (cursor) v |= 0x80;
    i2c_write(v);
  }
  i2c_stop();
}

void lcd_setCursor(uint8_t col, uint8_t row)
{
  if ((row < NUM_ROWS) && (col < NUM_COLS)) {
    if (_displayCursor) {
      uint8_t curValue = _buffer[_row * NUM_COLS + _col];
      write_raw(curValue, 0);
    }
    
    _col = col;
    _row = row;
    
    if (_displayCursor) {
      uint8_t curValue = _buffer[_row * NUM_COLS + _col];
      write_raw(curValue, 1);
    }
  } 
}

// ----------------------------------------------------------------------------

void lcd_write(uint8_t value) {
  write_raw(value, 0);
  
  _buffer[_row * NUM_COLS + _col] = value;
  _col++;
  if (_col >= NUM_COLS) {
    _col = 0;
    _row = (_row + 1) % NUM_ROWS;
  }
  if (_displayCursor) {
    uint8_t curValue = _buffer[_row * NUM_COLS + _col];
    write_raw(curValue, 1);
  }
}

void lcd_print(char *s) {
  while (*s) {
    lcd_write(*s++);
  }
}

// ----------------------------------------------------------------------------
void ssd1306_fillscreen(uint8_t fill) {
  uint8_t m, n;
  for (m = 0; m < 8; m++)
  {
    ssd1306_send_command_start();
    i2c_write(SSD1306_SETSTARTPAGE + m);  // page0 - page7
    i2c_write(SSD1306_LOWCOLUMNADDR);      // low column start address    
    i2c_write(SSD1306_HIGHCOLUMNADDR);      // high column start address
    ssd1306_send_data_start();
    for (n = 0; n < 128; n++)
    {
      i2c_write(fill);
    }
    i2c_stop();
  }
}

void ssd1306_send_command_start(void) {
  i2c_start(_addr << 1);
  i2c_write(0x00); // command
}

void ssd1306_send_command(uint8_t command) {
  ssd1306_send_command_start();
  i2c_write(command);
  i2c_stop();
}

void ssd1306_send_data_start(void) {
  i2c_start(_addr << 1);
  i2c_write(0x40); // data
}

#endif
