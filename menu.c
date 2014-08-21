#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"
#include "config.h"
#include "comms.h"
#include "memstrings.h"
#include "fileutils.h"
#include "tapuino.h"
#include "lcd.h"
#include "lcdutils.h"

#define MODE_FIRST    0
#define MODE_PLAY     0
#define MODE_RECORD   1
#define MODE_OPTIONS  2
#define MODE_LAST     2

#define REC_MODE_FIRST   0
#define REC_MODE_MANUAL  0
#define REC_MODE_AUTO    1
#define REC_MODE_LAST    1

#define SELECT_MODE_EXIT 0xFF

int g_num_files = 0;
int g_cur_file_index = 0;

uint8_t get_cur_command() {
// this order of operations is very important
// first get an 'atomic' read of g_cur_command into a local
  uint8_t cur_command = g_cur_command;
// then compare the _local_ against a non-IDLE command
  if (cur_command != COMMAND_IDLE) {
    // and clear the global i.e. only if the global was non-idle at the time of read
    // this prevents clearing the global as a key is pressed and missing it
    g_cur_command = COMMAND_IDLE;
  }
  return cur_command;
}

uint8_t handle_select_mode(const char* ptitle, const char** ppitems, uint8_t max) {
  uint8_t prev_mode = max - 1;
  uint8_t cur_mode = 0;

  lcd_title_P(ptitle);
  
  while (1) {
    if (prev_mode != cur_mode) {
      lcd_status_P(ppitems[cur_mode]);
      prev_mode = cur_mode;
    }
    
    switch(get_cur_command()) {
      case COMMAND_SELECT:
        return cur_mode;
      break;
      case COMMAND_ABORT:
        return SELECT_MODE_EXIT;
      break;
      case COMMAND_NEXT:
        if (cur_mode == (max - 1)) {
          cur_mode = 0;
        } else {
          cur_mode++;
        }
      break;
      case COMMAND_PREVIOUS:
        if (cur_mode == 0) {
          cur_mode = max -1;
        } else {
          cur_mode--;
        }
      break;
    }
  }
}

uint8_t handle_option_mode(const char* ptitle, const char* poption, uint16_t* pcur_value, uint16_t min_value, uint16_t max_value, uint16_t step_value) {
  int32_t cur_value = *pcur_value;
  lcd_title_P(ptitle);
  
  while (1) {
    switch(get_cur_command()) {
      case COMMAND_SELECT:
        *pcur_value = (uint16_t) cur_value;
        return 1;
      break;
      case COMMAND_ABORT:
        return 0;
      break;
      case COMMAND_NEXT:
        cur_value += step_value;
        if (cur_value > max_value) {
          cur_value = max_value;
        }
      break;
      case COMMAND_PREVIOUS:
        cur_value -= step_value;
        if (cur_value < min_value) {
          cur_value = min_value;
        }
      break;
    }
  }
}

void handle_play_mode(FILINFO* pfile_info) {
  // reset to the root after a possible record operation
  change_dir("/");
  // refresh the file list to avoid blank entries bug
  if ((g_num_files = get_num_files(pfile_info)) == 0) {
    lcd_title_P(S_NO_FILES_FOUND);
    lcd_busy_spinner();
    return;
  }

  g_cur_file_index = 0;
  if (!get_file_at_index(pfile_info, g_cur_file_index)) {
    // shouldn't happen...
    lcd_title_P(S_NO_FILES_FOUND);
    lcd_busy_spinner();
    return;
  }

  lcd_title_P(S_SELECT_FILE);
  display_filename(pfile_info);
  
  while (1) {
    switch(get_cur_command()) {
      case COMMAND_SELECT:
        if (pfile_info->fattrib & AM_DIR) {
          if (change_dir(pfile_info->fname) == FR_OK) {
            g_num_files = get_num_files(pfile_info);
            g_cur_file_index = 0;
            get_file_at_index(pfile_info, g_cur_file_index);
            display_filename(pfile_info);
          } else {
            lcd_status_P(S_CHDIR_FAILED);
          }
        } else {
          display_filename(pfile_info);
          play_file(pfile_info);
          lcd_title_P(S_SELECT_FILE);
          // buffer is used so get the file again
          get_file_at_index(pfile_info, g_cur_file_index);
          display_filename(pfile_info);
        }
      break;
      case COMMAND_ABORT:
        if (g_fs.cdir != 0) {
          if (change_dir("..") == FR_OK) {
            g_num_files = get_num_files(pfile_info);
            g_cur_file_index = 0;
            get_file_at_index(pfile_info, g_cur_file_index);
            display_filename(pfile_info);
          } else {
            lcd_status_P(S_CHDIR_FAILED);
          }        
        } else {
          // back to main menu
          return;
        }
      break;
      case COMMAND_NEXT:
        if (++g_cur_file_index >= g_num_files) {
          g_cur_file_index = 0;
        }
        get_file_at_index(pfile_info, g_cur_file_index);
        display_filename(pfile_info);
      break;
      case COMMAND_PREVIOUS:
        if (--g_cur_file_index < 0) {
          g_cur_file_index = g_num_files - 1;
        }
        get_file_at_index(pfile_info, g_cur_file_index);
        display_filename(pfile_info);
      break;
    }
    filename_ticker(pfile_info, get_timer_tick());
  }
}

void handle_record_mode_ready(char* pfile_name) {
  lcd_title_P(S_READY_RECORD);
  lcd_status_P(S_PRESS_START);
  
  while (1) {
    switch(get_cur_command()) {
      case COMMAND_SELECT:
        record_file(pfile_name);
        return;
      break;
      case COMMAND_ABORT:
        // back to main menu
        return;
      break;
    }
  }
}

uint8_t handle_manual_filename(FILINFO* pfile_info) {
  uint8_t cur_char_pos = 0;
  uint8_t cursor_pos = 0;
  uint8_t max_chars = strlen_P(S_FILENAME_CHARS);
  uint8_t cur_char = 0;
  lcd_title_P(S_ENTER_FILENAME);
  lcd_status("");
  lcd_cursor();
  lcd_setCursor(0, 1);
  
  // start with a nicely terminated string!
  memset(pfile_info->lfname, 0, pfile_info->lfsize);
  
  while (1) {
    switch(get_cur_command()) {
      case COMMAND_SELECT:
        if (cursor_pos < (MAX_LCD_LINE_LEN - 1)) {
          cur_char = pgm_read_byte(S_FILENAME_CHARS + cur_char_pos);
          pfile_info->lfname[cursor_pos] = cur_char;        
          cursor_pos++;
          lcd_setCursor(cursor_pos, 1);
          cur_char_pos = 0;
        }
      break;
      case COMMAND_SELECT_LONG:
        strcat(pfile_info->lfname, ".tap");
        lcd_noCursor();
        // exit to previous menu, with accept
        return 1;
      break;
      case COMMAND_ABORT:
        if (cursor_pos != 0) {
          pfile_info->lfname[cursor_pos] = 0;
          lcd_setCursor(cursor_pos, 1);
          lcd_write(' ');
          cursor_pos--;
          lcd_setCursor(cursor_pos, 1);
          cur_char_pos = 0;
          cur_char = pfile_info->lfname[cursor_pos];
          while (pgm_read_byte(S_FILENAME_CHARS + cur_char_pos) != cur_char) {
            cur_char_pos++;
          }
        }
      break;
      case COMMAND_ABORT_LONG:
        lcd_title_P(S_OPERATION_ABORTED);
        lcd_noCursor();
        lcd_busy_spinner();
        // exit to previous menu, with cancel
        return 0;
      break;
      case COMMAND_NEXT:
        cur_char_pos = (cur_char_pos + 1) % max_chars;
        cur_char = pgm_read_byte(S_FILENAME_CHARS + cur_char_pos);
        lcd_write(cur_char);
        lcd_setCursor(cursor_pos, 1);
        pfile_info->lfname[cursor_pos] = cur_char;
      break;
      case COMMAND_PREVIOUS: 
        if (cur_char_pos == 0) {
          cur_char_pos = max_chars;
        }
        cur_char_pos--;
        cur_char = pgm_read_byte(S_FILENAME_CHARS + cur_char_pos);
        lcd_write(cur_char);
        lcd_setCursor(cursor_pos, 1);
        pfile_info->lfname[cursor_pos] = cur_char;
      break;
    }    
  }
}

void handle_record_mode(FILINFO* pfile_info) {
  const char* ppitems[] = {S_REC_MODE_MANUAL, S_REC_MODE_AUTO};
  FRESULT res;

  lcd_title_P(S_SELECT_RECORD_MODE);

  // attempt to open the recording dir
  strcpy_P((char*)g_fat_buffer, S_DEFAULT_RECORD_DIR);
  res = f_opendir(&g_dir, (char*)g_fat_buffer);
  if (res != FR_OK) { // try to make it if its not there
    res = f_mkdir((char*)g_fat_buffer);
    if (res != FR_OK || f_opendir(&g_dir, (char*)g_fat_buffer) != FR_OK) {
      lcd_status_P(S_MKDIR_FAILED);
      lcd_busy_spinner();
      return;
    }
  }
  // change to the recording dir
  if (f_chdir((char*)g_fat_buffer) != FR_OK) {
    lcd_status_P(S_CHDIR_FAILED);
    lcd_busy_spinner();
    return;
  }
  
  while (1) {
    switch (handle_select_mode(S_SELECT_RECORD_MODE, ppitems, 2)) {
      case REC_MODE_AUTO:
        handle_record_mode_ready(NULL);
        return;
      break;
      case REC_MODE_MANUAL:
        if (handle_manual_filename(pfile_info)) {
          handle_record_mode_ready(pfile_info->lfname);
          return;
        }
      break;
      case SELECT_MODE_EXIT:
        return;
      break;
    }
  }
}

void handle_mode_options() {
  const char* ppitems[] = {S_OPTION_SIGNAL, S_OPTION_KEYS, S_OPTION_DISPLAY};

  while (1) {
    switch (handle_select_mode(S_SELECT_MODE, ppitems, 3)) {
      case SELECT_MODE_EXIT:
        return;
      break;
    }
  }
}



void main_menu(FILINFO* pfile_info) {
  const char* ppitems[] = {S_MODE_PLAY, S_MODE_RECORD, S_MODE_OPTIONS};
  if ((g_num_files = get_num_files(pfile_info)) == 0) {
    lcd_title_P(S_NO_FILES_FOUND);
    return;
  }

  while (1) {
    switch (handle_select_mode(S_SELECT_MODE, ppitems, 3)) {
      case MODE_PLAY:
        handle_play_mode(pfile_info);
      break;
      case MODE_RECORD:
        handle_record_mode(pfile_info);
      break;
      case MODE_OPTIONS:
        handle_mode_options();
      break;
    }
  }
}