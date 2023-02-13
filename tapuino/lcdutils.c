#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include "tapuino.h"
#include "ff.h"
#include "config.h"
#include "lcd_interface.h"
#include "lcdutils.h"
#include "memstrings.h"

// right arrow character 
#define DIRECTORY_INDICATOR 0b01111110

char g_char_buffer[MAX_LCD_LINE_LEN + 1] = {0};
static uint8_t g_ticker_enabled  = 0;
static uint8_t g_ticker_index = 0;
static uint32_t g_last_tick = 0;
static uint32_t g_last_hold = 0;
static char* g_ticker_string = NULL;

void filename_ticker(FILINFO* pfile_info, uint32_t cur_tick) {
  if (g_ticker_enabled) {
    if (!g_last_tick) g_last_tick = cur_tick;
    
    // how often do we tick?
    if (cur_tick - g_last_tick < (uint32_t) g_ticker_rate) {
      return;
    }
    g_last_tick = cur_tick;

    if (!g_last_hold) g_last_hold = cur_tick;
    // how long do we hold?
    if (cur_tick - g_last_hold < (uint32_t) g_ticker_hold_rate) {
      return;
    }

    // is the filename within screen bounds?
    if ((strlen(g_ticker_string) - g_ticker_index) < MAX_LCD_LINE_LEN) {
      // how long do we hold at the end?
      if (cur_tick - g_last_hold < (uint32_t) (g_ticker_hold_rate << 1)) {
        return;
      }
      g_ticker_index = 0;
      g_last_tick = g_last_hold = 0;
    } else {
      //reset to avoid overflow
      g_last_hold = cur_tick - g_ticker_hold_rate - 1;
      g_ticker_index++;
    }
    
    lcd_status(&g_ticker_string[g_ticker_index]);
    if (pfile_info->fattrib & AM_DIR) {
      lcd_show_dir();
    }  
  }
}

void display_filename(FILINFO* pfile_info) {
  g_ticker_string = pfile_info->lfname[0] ? pfile_info->lfname : pfile_info->fname;
  lcd_status(g_ticker_string);
  if (pfile_info->fattrib & AM_DIR) {
    lcd_show_dir();
  }  
  g_ticker_enabled = strlen(g_ticker_string) > (MAX_LCD_LINE_LEN - 1);
  g_ticker_index = 0;
  g_last_tick = 0;
  g_last_hold = 0;
}

void lcd_spinner_internal(uint32_t cur_tick, uint8_t perc, uint16_t rate) {
  static uint8_t indicators[] = {'|', '/', '-', '\\'};
  static uint8_t pos = 0;
  if (cur_tick - g_last_tick < rate) {
    return;
  }
  
  g_last_tick = cur_tick;
  lcd_setCursor(MAX_LCD_LINE_LEN - 7, 0);
  memset(g_char_buffer, 32, 7);
  if (perc < 101) {
    uint8_t off = perc < 10 ? 2 : perc < 100 ? 1 : 0;
    itoa(perc, g_char_buffer + off, 10);
    g_char_buffer[3] = '%';
  }
  g_char_buffer[5] = g_is_paused ? 'P' : MOTOR_IS_OFF() ? 'm' : 'M';
  g_char_buffer[6] = indicators[pos++];
  g_char_buffer[7] = 0;
  lcd_print(g_char_buffer);

  if (pos > 3) {
    pos = 0;
  }
}

void lcd_spinner(uint32_t cur_tick, int8_t perc) {
  lcd_spinner_internal(cur_tick, perc, g_ticker_rate);
}

void lcd_busy_spinner() {
  uint8_t i;
  for (i = 0; i < 100; i++) {
    lcd_spinner_internal(i, 101, 0);
    _delay_ms(20);
  }
}

void lcd_show_dir() {
  lcd_setCursor(MAX_LCD_LINE_LEN - 1, 1);
  lcd_write(DIRECTORY_INDICATOR);
}

void lcd_line(char* msg, int line, uint8_t usepgm) {
  int len;
  memset(g_char_buffer, 32, MAX_LCD_LINE_LEN);
  g_char_buffer[MAX_LCD_LINE_LEN] = 0;
  
  lcd_setCursor(0, line);
  if (usepgm) {
    len = strlen_P(msg);
    memcpy_P(g_char_buffer, msg, len > MAX_LCD_LINE_LEN ? MAX_LCD_LINE_LEN : len);
  } else {
    len = strlen(msg);
    memcpy(g_char_buffer, msg, len > MAX_LCD_LINE_LEN ? MAX_LCD_LINE_LEN : len);
  }
  lcd_print(g_char_buffer);
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
  lcd_init(LCD_I2C_ADDR);
  lcd_backlight();
}
