#include "config.h"
#ifdef LCD_USE_SSD1306_OLED_MODULE

#include <string.h>
#include <avr/pgmspace.h>
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

// Init Sequence
const uint8_t ssd1306_init_sequence [] PROGMEM = {
  0xAE,         // Display OFF (sleep mode)
  0x20, 0b00,   // Set Memory Addressing Mode
                // 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
                // 10=Page Addressing Mode (RESET); 11=Invalid
  0xB0,         // Set Page Start Address for Page Addressing Mode, 0-7
  0xC8,         // Set COM Output Scan Direction
  0x00,         // ---set low column address
  0x10,         // ---set high column address
  0x40,         // --set start line address
  0x81, 0x3F,   // Set contrast control register
  0xA1,         // Set Segment Re-map. A0=address mapped; A1=address 127 mapped. 
  0xA6,         // Set display mode. A6=Normal; A7=Inverse
  0xA8, 0x3F,   // Set multiplex ratio (1 to 64)
  0xA4,         // Output RAM to Display
                // 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
  0xD3, 0x00,   // Set display offset. 00 = no offset
  0xD5,         // --set display clock divide ratio/oscillator frequency
  0xF0,         // --set divide ratio
  0xD9, 0x22,   // Set pre-charge period
  0xDA, 0x12,   // Set com pins hardware configuration    
  0xDB,         // --set vcomh
  0x20,         // 0x20,0.77xVcc
  0x8D, 0x14,   // Set DC-DC enable
  0xAF          // Display ON in normal mode
};


void lcd_init(uint8_t lcd_addr)
{
  uint8_t i;
  _addr = lcd_addr;
  _row = 0;
  _col = 0;
  _displayCursor = 0;
  
  i2c_init();

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
 
  col = _col << 3; // convert to pixel
  ssd1306_send_command_start();
  i2c_write(0xb0 + _row);
  i2c_write((col & 0x0f) | 0x00);
  i2c_write(((col & 0xf0) >> 4) | 0x10);
    
  ssd1306_send_data_start();
  for (i = 0; i < 8; i++)
  {
    v = pgm_read_byte(&font8x8[c * 8 + i]);
    // add underline for cursor
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
    i2c_write(0xb0 + m);  // page0 - page7
    i2c_write(0x00);      // low column start address    
    i2c_write(0x10);      // high column start address
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
