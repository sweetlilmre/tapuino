#ifndef LCD_INTERFACE_H
#define LCD_INTERFACE_H

#include <inttypes.h>

void lcd_init(uint8_t lcd_addr);
void lcd_cursor();
void lcd_noCursor();
void lcd_backlight();
void lcd_noBacklight();
void lcd_setCursor(uint8_t col, uint8_t row); 
void lcd_print(char* msg);
void lcd_write(uint8_t value);

#endif
