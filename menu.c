#include <inttypes.h>

#include "ff.h"
#include "config.h"
#include "comms.h"
#include "memstrings.h"
#include "fileutils.h"
#include "tapuino.h"
#include "lcdutils.h"

#define MODE_FIRST    0
#define MODE_PLAY     0
#define MODE_RECORD   1
#define MODE_OPTIONS  2
#define MODE_LAST     2

int g_num_files = 0;
int g_cur_file_index = 0;

void handle_play_mode(FILINFO* pfile_info) {
  lcd_title_P(S_SELECT_FILE);
  if (!get_file_at_index(pfile_info, g_cur_file_index)) {
    // shouldn't happen...
    lcd_title_P(S_NO_FILES_FOUND);
    return;
  }

  display_filename(pfile_info);
  
  while(1)
  {
    switch(g_cur_command)
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
        g_cur_command = COMMAND_IDLE;
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
        g_cur_command = COMMAND_IDLE;
        break;
      }
      case COMMAND_NEXT:
      {
        if (++g_cur_file_index >= g_num_files) {
          g_cur_file_index = 0;
        }
        get_file_at_index(pfile_info, g_cur_file_index);
        display_filename(pfile_info);
        g_cur_command = COMMAND_IDLE;
        break;
      }
      case COMMAND_PREVIOUS:
      {
        if (--g_cur_file_index < 0) {
          g_cur_file_index = g_num_files - 1;
        }
        get_file_at_index(pfile_info, g_cur_file_index);
        display_filename(pfile_info);
        g_cur_command = COMMAND_IDLE;
        break;
      }
    }
    filename_ticker(get_timer_tick());
  }
}

void handle_mode_record(FILINFO* pfile_info) {
  lcd_title_P(S_READY_RECORD);
  lcd_status_P(S_PRESS_START);
  
  while(1)
  {
    switch(g_cur_command)
    {
      case COMMAND_SELECT:
      {
        record_file();
        lcd_title_P(S_READY_RECORD);
        lcd_status_P(S_PRESS_START);
        g_cur_command = COMMAND_IDLE;
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

void handle_mode_options() {
}


uint8_t handle_select_mode() {
  uint8_t prev_mode = MODE_LAST;
  uint8_t cur_mode = MODE_PLAY;

  lcd_title_P(S_SELECT_MODE);
  
  while(1)
  {
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
    
    switch(g_cur_command)
    {
      case COMMAND_SELECT:
      {
        g_cur_command = COMMAND_IDLE;
        return cur_mode;
      }
      case COMMAND_ABORT:
      {
        g_cur_command = COMMAND_IDLE;
        break;
      }
      case COMMAND_NEXT:
      {
        if (cur_mode == MODE_LAST) {
          cur_mode = MODE_FIRST;
        } else {
          cur_mode++;
        }
        g_cur_command = COMMAND_IDLE;
        break;
      }
      case COMMAND_PREVIOUS:
      {
        if (cur_mode == MODE_FIRST) {
          cur_mode = MODE_LAST;
        } else {
          cur_mode--;
        }
        g_cur_command = COMMAND_IDLE;
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
        handle_mode_record(pfile_info);
      break;
      case MODE_OPTIONS:
        handle_mode_options();
      break;
    }
  }
}