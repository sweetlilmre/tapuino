#ifndef _LCDUTILS_H
#define _LCDUTILS_H

void lcd_setup();

void filename_ticker(FILINFO* pfile_info, uint32_t cur_tick);
void display_filename(FILINFO* pfile_info);
void lcd_busy_spinner();
void lcd_spinner(uint32_t wait, int8_t perc);
void lcd_show_dir();
void lcd_title(char* msg);
void lcd_title_P(const char* msg);
void lcd_status(char* msg);
void lcd_status_P(const char* msg);

extern char g_char_buffer[];

#endif
