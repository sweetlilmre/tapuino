#include <inttypes.h>
#include <string.h>
#include "config.h"
#include "lcd.h"
#include "lcdutils.h"
#include "memstrings.h"
#include "serial.h"

char g_char_buffer[MAX_LCD_LINE_LEN + 1] = {0};

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

void lcd_spinner(int32_t wait) {
  static uint8_t indicators[] = {'|', '/', '-', 0};
  static uint8_t pos = 0;
  static int32_t wait_for = 0;
  if (wait_for++ < wait) {
    return;
  }
  wait_for = 0;
  lcd_setCursor(MAX_LCD_LINE_LEN - 1, 0);
  lcd_write(indicators[pos++]);
  if (pos > 3) {
    pos = 0;
  }
}

void lcd_show_dir() {
  lcd_setCursor(MAX_LCD_LINE_LEN - 1, 1);
  lcd_write(0b01111110);
}

void lcd_line(char* msg, int line, uint8_t usepgm) {
  int len;
  strncpy_P(g_char_buffer, S_MAX_BLANK_LINE, MAX_LCD_LINE_LEN);
  
  lcd_setCursor(0, line);
  if (usepgm) {
    len = strlen_P(msg);
    memcpy_P(g_char_buffer, msg, len > MAX_LCD_LINE_LEN ? MAX_LCD_LINE_LEN : len);
  } else {
    len = strlen(msg);
    memcpy(g_char_buffer, msg, len > MAX_LCD_LINE_LEN ? MAX_LCD_LINE_LEN : len);
  }
  lcd_print(g_char_buffer);
  serial_println(g_char_buffer);
}

void lcd_title(char* msg) {
  lcd_line(msg, 0, 0);
}

void lcd_title_P(const char* msg) {
  lcd_line((char*)msg, 0, 1);
}

void lcd_status(char* msg) {
  lcd_line(msg, 1, 0);
}

void lcd_status_P(const char* msg) {
  lcd_line((char*)msg, 1, 1);
}

void lcd_setup() {
  lcd_begin(0x27, 20, 4, LCD_5x8DOTS);
  lcd_backlight();
  lcd_createChar(0, backslashChar);
}
