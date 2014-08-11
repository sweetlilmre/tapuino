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

void handle_play_mode(FILINFO* pfile_info) {
  lcd_title_P(S_SELECT_FILE);
  if (!get_file_at_index(pfile_info, g_cur_file_index)) {
    // shouldn't happen...
    lcd_title_P(S_NO_FILES_FOUND);
    return;
  }

  display_filename(pfile_info);
  
  while (1) {
    switch(get_cur_command())
    {
      case COMMAND_SELECT:
      {
        if (pfile_info->fattrib & AM_DIR) {
          if (change_dir(pfile_info->fname) == FR_OK) {
            g_num_files = get_num_files(pfile_info);
            g_cur_file_index = 0;
            get_file_at_index(pfile_info, g_cur_file_index);
            display_filename(pfile_info);
          } else {
            lcd_status_P(S_DIRECTORY_ERROR);
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
      }
      case COMMAND_ABORT:
      {
        if (g_fs.cdir != 0) {
          if (change_dir("..") == FR_OK) {
            g_num_files = get_num_files(pfile_info);
            g_cur_file_index = 0;
            get_file_at_index(pfile_info, g_cur_file_index);
            display_filename(pfile_info);
          } else {
            lcd_status_P(S_DIRECTORY_ERROR);
          }        
        } else {
          // back to main menu
          return;
        }
        break;
      }
      case COMMAND_NEXT:
      {
        if (++g_cur_file_index >= g_num_files) {
          g_cur_file_index = 0;
        }
        get_file_at_index(pfile_info, g_cur_file_index);
        display_filename(pfile_info);
        break;
      }
      case COMMAND_PREVIOUS:
      {
        if (--g_cur_file_index < 0) {
          g_cur_file_index = g_num_files - 1;
        }
        get_file_at_index(pfile_info, g_cur_file_index);
        display_filename(pfile_info);
        break;
      }
    }
    filename_ticker(pfile_info, get_timer_tick());
  }
}

void handle_record_mode_ready(char* pfile_name) {
  lcd_title_P(S_READY_RECORD);
  lcd_status_P(S_PRESS_START);
  
  while (1) {
    switch(get_cur_command())
    {
      case COMMAND_SELECT:
      {
        record_file(pfile_name);
        lcd_title_P(S_READY_RECORD);
        lcd_status_P(S_PRESS_START);
        break;
      }
      case COMMAND_ABORT:
      {
        // back to main menu
        return;
      }
    }
  }
}

uint8_t handle_manual_filename(FILINFO* pfile_info) {
  uint8_t cur_char_pos = 0;
  uint8_t cursor_pos = 0;
  uint8_t max_chars = strlen_P(S_FILENAME_CHARS);
  uint8_t cur_char = 0;
  lcd_title_P(S_ENTER_FILENAME);
  lcd_status(S_MAX_BLANK_LINE);
  lcd_cursor();
  lcd_setCursor(0, 1);
  
  // start with a nicely terminated string!
  memset(pfile_info->lfname, 0, pfile_info->lfsize);
  
  while (1) {
    switch(get_cur_command())
    {
      case COMMAND_SELECT:
      {
        if (cursor_pos < (MAX_LCD_LINE_LEN - 1)) {
          cur_char = pgm_read_byte(S_FILENAME_CHARS + cur_char_pos);
          pfile_info->lfname[cursor_pos] = cur_char;        
          cursor_pos++;
          lcd_setCursor(cursor_pos, 1);
          cur_char_pos = 0;
        }
        break;
      }
      case COMMAND_SELECT_LONG:
      {
        strcat(pfile_info->lfname, ".tap");
        // exit to previous menu, with accept
        return 1;
      }
      case COMMAND_ABORT:
      {
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
      }
      case COMMAND_ABORT_LONG:
      {
        lcd_title_P(S_OPERATION_ABORTED);
        lcd_busy_spinner();
        // exit to previous menu, with cancel
        return 0;
      }
      case COMMAND_NEXT:
      {
        cur_char_pos = (cur_char_pos + 1) % max_chars;
        cur_char = pgm_read_byte(S_FILENAME_CHARS + cur_char_pos);
        lcd_write(cur_char);
        lcd_setCursor(cursor_pos, 1);
        pfile_info->lfname[cursor_pos] = cur_char;
        break;
      }
      case COMMAND_PREVIOUS:
      {
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
  
}

void handle_record_mode_select(FILINFO* pfile_info) {
  FRESULT res;
  uint8_t prev_mode = REC_MODE_LAST;
  uint8_t cur_mode = REC_MODE_FIRST;

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
    if (prev_mode != cur_mode) {
      switch (cur_mode)
      {
        case REC_MODE_MANUAL:
          lcd_status_P(S_REC_MODE_MANUAL);
        break;
        case REC_MODE_AUTO:
          lcd_status_P(S_REC_MODE_AUTO);
        break;
      }
      prev_mode = cur_mode;
    }
    
    switch(get_cur_command())
    {
      case COMMAND_SELECT:
      {
        switch(cur_mode)
        {
          case REC_MODE_AUTO:
            handle_record_mode_ready(NULL);
          break;
          case REC_MODE_MANUAL:
            if (handle_manual_filename(pfile_info)) {
              handle_record_mode_ready(pfile_info->lfname);
            }
          break;
        }
        lcd_title_P(S_SELECT_RECORD_MODE);
      }
      case COMMAND_ABORT:
      {
        // reset to the root after a record operation
        f_chdir("/");
        // refresh the file list to avoid blank entries bug
        if ((g_num_files = get_num_files(pfile_info)) == 0) {
          lcd_title_P(S_NO_FILES_FOUND);
          return;
        }
        return;
      }
      case COMMAND_NEXT:
      {
        if (cur_mode == REC_MODE_LAST) {
          cur_mode = REC_MODE_FIRST;
        } else {
          cur_mode++;
        }
        break;
      }
      case COMMAND_PREVIOUS:
      {
        if (cur_mode == REC_MODE_FIRST) {
          cur_mode = REC_MODE_LAST;
        } else {
          cur_mode--;
        }
        break;
      }
    }
  }
}

void handle_mode_options() {
}


uint8_t handle_select_mode() {
  uint8_t prev_mode = MODE_LAST;
  uint8_t cur_mode = MODE_PLAY;

  lcd_title_P(S_SELECT_MODE);
  
  while (1) {
    if (prev_mode != cur_mode) {
      switch (cur_mode)
      {
        case MODE_PLAY:
          lcd_status_P(S_MODE_PLAY);
        break;
        case MODE_RECORD:
          lcd_status_P(S_MODE_RECORD);
        break;
        case MODE_OPTIONS:
          lcd_status_P(S_MODE_OPTIONS);
        break;
      }
      prev_mode = cur_mode;
    }
    
    switch(get_cur_command())
    {
      case COMMAND_SELECT:
      {
        return cur_mode;
      }
      case COMMAND_ABORT:
      {
        break;
      }
      case COMMAND_NEXT:
      {
        if (cur_mode == MODE_LAST) {
          cur_mode = MODE_FIRST;
        } else {
          cur_mode++;
        }
        break;
      }
      case COMMAND_PREVIOUS:
      {
        if (cur_mode == MODE_FIRST) {
          cur_mode = MODE_LAST;
        } else {
          cur_mode--;
        }
        break;
      }
    }
  }
}

void main_menu(FILINFO* pfile_info) {
  if ((g_num_files = get_num_files(pfile_info)) == 0) {
    lcd_title_P(S_NO_FILES_FOUND);
    return;
  }

  while (1) {
    switch (handle_select_mode()) {
      case MODE_PLAY:
        handle_play_mode(pfile_info);
      break;
      case MODE_RECORD:
        handle_record_mode_select(pfile_info);
      break;
      case MODE_OPTIONS:
        handle_mode_options();
      break;
    }
  }
}