#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "integer.h"
#include "config.h"

#include "spi.h"
#include "ff.h"
#include "diskio.h"
#include "serial.h"
#include "comms.h"
#include "lcd.h"
#include "lcdutils.h"
#include "memstrings.h"
#include "fileutils.h"


#ifdef USE_NTSC_TIMING
  #define CYCLE_MULT_RAW    0.978 // (1000000 / 1022730 NTSC  cycles)
  #define CYCLE_MULT_8      7.82  // (CYCLE_MULT_RAW * 8)
#else
  #define CYCLE_MULT_RAW    1.015 // (1000000 / 985248 PAL cycles)
  #define CYCLE_MULT_8      8.12  // (CYCLE_MULT_RAW * 8)
#endif


// the maximum TAP delay is a 24-bit value i.e. 0xFFFFFF cycles
// we need this constant to determine if the loader has switched off the motor before the tap has completed
// which would cause the code to enter an endless loop (when the motor is off the buffers do not progress)
// so we check during the buffer wait loop to see if we have exceeded the maximum delay time and exit if so.
#define MAX_SIGNAL_CYCLES     (CYCLE_MULT_RAW * 0xFFFFFF * 2)

// helper variables for the ISR and loader code

static volatile uint8_t g_read_index;           // read index in the 256 byte buffer
static volatile uint8_t g_write_index;          // write index in the 256 byte buffer
static volatile uint8_t g_signal_2nd_half;      // flag to indicate that the 2nd half of the signal should be output
static volatile uint32_t g_total_timer_count;   // number of (AVR) cycles that the timer has been running for
static volatile uint8_t g_tap_file_complete;    // flag to indicate that all bytes have been read from the TAP
static volatile uint32_t g_tap_file_pos;        // current read position in the TAP (bytes)
static volatile uint32_t g_tap_file_len;        // total length of the TAP  (bytes)
static uint32_t g_pulse_length = 0;             // length of pulse in uS
static uint32_t g_pulse_length_save;            
static uint8_t g_recording = 0;                 // recording flag for the ISR


int g_num_files = 0;
int g_cur_file_index = 0;

static inline void tap_write_routine() {
  static uint8_t last_signal = 0;
  uint8_t signal = TAPE_WRITE_PINS & _BV(TAPE_WRITE_PIN);
  uint32_t tap_data;

  if (signal != last_signal) {
    last_signal = signal;
    if (signal != 0) {
      if(g_signal_2nd_half) {
        tap_data = g_total_timer_count / CYCLE_MULT_8;
        
        if (tap_data == 0) {
          tap_data++;
        }
        if (tap_data < 256) {
          g_fat_buffer[g_read_index++] = (uint8_t) tap_data;
        } else {
          g_fat_buffer[g_read_index++] = 0;
          g_fat_buffer[g_read_index++] = (uint8_t) (g_total_timer_count & 0xff);
          g_fat_buffer[g_read_index++] = (uint8_t) ((g_total_timer_count & 0xff00) >> 8);
          g_fat_buffer[g_read_index++] = (uint8_t) ((g_total_timer_count & 0xff0000) >> 16);
        }
      } else {
        g_signal_2nd_half = 0;
        g_total_timer_count = 0;
        OCR1A = 0x8;
      }
    }
  }
}

static inline void tap_read_routine() {
  uint32_t tap_data;
  
  if (g_signal_2nd_half) {                // 2nd half of the signal
    if (g_pulse_length > 0xFFFF) {        // check to see if its bigger than 16 bits
      g_pulse_length -= 0xFFFF;
      OCR1A = 0xFFFF;
    } else {
      OCR1A = (unsigned short) g_pulse_length;
      g_pulse_length = 0;                 // clear this, for 1st half check so that the next data is loaded
      g_signal_2nd_half = 0;              // next time round switch to 1st half
    }
    TAPE_READ_HIGH();                     // set the signal high
  } else {                                // 1st half of the signal
    if (g_pulse_length) {                 // do we have any pulse left?
      if (g_pulse_length > 0xFFFF) {      // check to see if its bigger than 16 bits
        g_pulse_length -= 0xFFFF;
        OCR1A = 0xFFFF;
      } else {
        OCR1A = (unsigned short) g_pulse_length;
        g_pulse_length = g_pulse_length_save; // restore pulse length for the 2nd half of the signal
        g_signal_2nd_half = 1;            // next time round switch to 2nd half
      }
    } else {
      g_total_timer_count = 0;
      if (g_tap_file_pos >= g_tap_file_len) {
        g_tap_file_complete = 1;
        return;                           // reached the end of the TAP file so don't process any more!
      }
      tap_data = (unsigned long) g_fat_buffer[g_read_index++];
      g_tap_file_pos++;
      if (tap_data == 0) {
        g_pulse_length =  (unsigned long) g_fat_buffer[g_read_index++];
        g_pulse_length |= ((unsigned long) g_fat_buffer[g_read_index++]) << 8;
        g_pulse_length |= ((unsigned long) g_fat_buffer[g_read_index++]) << 16;
        g_pulse_length *= CYCLE_MULT_RAW;
        g_tap_file_pos += 3;
      } else {
        g_pulse_length = tap_data * CYCLE_MULT_8;
      }
      g_pulse_length_save = g_pulse_length;   // save this for the 2nd half of the wave
      if (g_pulse_length > 0xFFFF) {        // check to see if its bigger than 16 bits
        g_pulse_length -= 0xFFFF;
        OCR1A = 0xFFFF;
      } else {
        OCR1A = (unsigned short) g_pulse_length;
        g_pulse_length = g_pulse_length_save; // restore pulse length for the 2nd half of the signal
        g_signal_2nd_half = 1;            // next time round switch to 2nd half
      }
      TAPE_READ_LOW();
    }
  }
}

// timer1 is running at 2MHz or 0.5 uS per tick.
// signal values are measured in uS, so OCR1A is set to the value from the TAP file (converted into uS) for each signal half
// i.e. TAP value converted to uS * 2 == full signal length
ISR(TIMER1_COMPA_vect) {
  // keep track of the number of cycles in case we get to a MOTOR stop situation before the TAP has completed
  g_total_timer_count += OCR1A;
  
  // don't process if the MOTOR is off!
  if (MOTOR_IS_OFF()) {
    return;
  }
  
  if (g_recording) {
    tap_write_routine();
  } else {
    tap_read_routine();
  }
}

void signal_timer_setup() {
  TCCR1A = 0x00;   // clear timer registers
  TCCR1B = 0x00;
  TIMSK1 = 0x00;
 
  TCCR1B |=  _BV(CS11) | _BV(WGM12);  // pre-scaler 8 = 2 MHZ
}

void signal_timer_start() {
  OCR1A = 0xFFFF;
  TCNT1 = 0;
  g_total_timer_count = 0;
  TIMSK1 |=  _BV(OCIE1A);
}

void signal_timer_stop() {
  TIMSK1 &= ~_BV(OCIE1A);
}

void print_pos() {
  serial_print("g_tap_file_pos: ");
  ultoa(g_tap_file_pos, g_char_buffer, 10);
  serial_println(g_char_buffer);

  serial_print("g_tap_file_len: ");
  ultoa(g_tap_file_len, g_char_buffer, 10);
  serial_println(g_char_buffer);
}


void busy_spinner() {
  int i;
  for (i = 0; i < 100; i++) {
    lcd_spinner(0, 100);
    _delay_ms(20);
  }
}

int play_file(FILINFO* pfile_info)
{
  FRESULT res;
  UINT br;
  int perc = 0;
  g_tap_file_complete = 0;
  g_tap_file_pos = 0;
  g_tap_file_len = pfile_info->fsize;

  res = f_open(&g_fil, pfile_info->fname, FA_READ);
  if (res != FR_OK) {
    lcd_title_P(S_OPEN_FAILED);
    return 0;
  }

  res = f_read(&g_fil, (void*) g_fat_buffer, FAT_BUF_SIZE, &br);
  if (res != FR_OK) {
    lcd_title_P(S_READ_FAILED);
    return 0;
  }

  if (strncmp_P((const char*) g_fat_buffer, S_TAP_MAGIC_C64, 12) != 0) {
    lcd_title_P(S_INVALID_TAP);
    return 0;
  }
  
  g_write_index = 0;
  g_read_index = 20; // Skip header
  g_tap_file_pos = 20;
  g_signal_2nd_half = 0;

  lcd_title_P(S_LOADING);

  TAPE_READ_LOW();
  SENSE_ON();
  // Start send-ISR
  signal_timer_start();

  while (br > 0) {
    // Wait until ISR is in the new half of the buffer
    while ((g_read_index & 0x80) == (g_write_index & 0x80)) {
      // process input for abort
      input_callback();
      // feedback to the user
      lcd_spinner(SPINNER_RATE, perc);
      
      // if the C64 stopped the motor for longer than the longest possible signal time
      // then we need to get out of here. This happens in Rambo First Blood Part II.
      // The loader seems to stop the tape before the tap file is complete.
      if ((g_total_timer_count > MAX_SIGNAL_CYCLES) || (g_cur_command == COMMAND_ABORT)) {
        g_tap_file_complete = 1;
        break;
      }
    }

    // exit outer while
    if (g_tap_file_complete) {
      break;
    }
    
    f_read(&g_fil, (void*) g_fat_buffer + g_write_index, 128, &br);
    g_write_index += 128;
    perc = (g_tap_file_pos * 100) / g_tap_file_len;
    print_pos();
    input_callback();
  }

  // wait for the remaining buffer to be read.
  while (!g_tap_file_complete) {
    // process input for abort
    input_callback();
    // feedback to the user
    lcd_spinner(SPINNER_RATE, perc);
    // we need to do the same trick as above, BC's Quest for Tires stops the motor right near the
    // end of the tape, then restarts for the last bit of data, so we can't rely on the motor signal
    // a better approach might be to see if we have read all the data and then break. //
    if ((g_cur_command == COMMAND_ABORT) || (g_total_timer_count > MAX_SIGNAL_CYCLES)) {
      break;
    }
  }

  signal_timer_stop();
  f_close(&g_fil);
  
  print_pos();

  TAPE_READ_LOW();
  SENSE_OFF();

  if (g_cur_command == COMMAND_ABORT) {
    lcd_title_P(S_LOADING_ABORTED);
  } else {
    lcd_title_P(S_LOADING_COMPLETE);
  }

  // end of load UI indicator
  busy_spinner();

  return 1;
}


void record_file() {
  FRESULT res;
  UINT br;
  int tmp = 0;
  g_tap_file_complete = 0;
  g_tap_file_pos = 0;
  g_tap_file_len = 0;

  strcpy_P(g_char_buffer, S_DEFAULT_RECORD_DIR);
  res = f_opendir(&g_dir, g_char_buffer);
  if (res != FR_OK) {
    res = f_mkdir(g_char_buffer);
    if (res != FR_OK || f_opendir(&g_dir, g_char_buffer) != FR_OK) {
      lcd_status_P(S_MKDIR_FAILED);
      busy_spinner();
      return;
    }
  }
  
  br = 0;
  while (1) {
    sprintf_P(g_char_buffer, S_NAME_PATTERN, br);
    res = f_open(&g_fil, g_char_buffer, FA_READ);
    if (res != FR_OK) {
      break;
    }
    f_close(&g_fil);
    br++;
  }
  res = f_open(&g_fil, g_char_buffer, FA_CREATE_NEW);
  if (res != FR_OK) {
    lcd_status_P(S_OPEN_FAILED);
    busy_spinner();
    return;
  }
  
  // write header
  memset (g_fat_buffer, 0, sizeof(g_fat_buffer));
  strcpy_P((char*)g_fat_buffer, S_TAP_MAGIC_C64);
  tmp = strlen((char*)g_fat_buffer);
  g_fat_buffer[tmp] = 0x01;

  g_write_index = 0;
  g_read_index = 20; // Skip header
  g_tap_file_pos = 20;
  g_signal_2nd_half = 0;
  
  lcd_title_P(S_LOADING);
  SENSE_ON();
  // Start send-ISR
  signal_timer_start();

  while (!MOTOR_IS_OFF()) {
    while ((g_read_index & 0x80) == (g_write_index & 0x80)) {
      // process input for abort
      input_callback();
      // feedback to the user
      lcd_spinner(SPINNER_RATE, 0);
      
      if (MOTOR_IS_OFF() || (g_cur_command == COMMAND_ABORT)) {
        g_tap_file_complete = 1;
        break;
      }
    }

    // exit outer while
    if (g_tap_file_complete) {
      break;
    }
    
    f_write(&g_fil, (void*) g_fat_buffer + g_write_index, 128, &br);
    g_write_index += 128;
    input_callback();
  }
  
  if (g_write_index != g_read_index) {
    f_write(&g_fil, (void*) g_fat_buffer + g_write_index, g_read_index & 0x80, &br);
  }

  signal_timer_stop();
  f_close(&g_fil);
  
  if (g_cur_command == COMMAND_ABORT) {
    lcd_title_P(S_LOADING_ABORTED);
  } else {
    lcd_title_P(S_LOADING_COMPLETE);
  }

  // end of load UI indicator
  busy_spinner();
}

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
      default:
        input_callback();
      break;
    }
    filename_ticker();
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
      default:
        input_callback();
      break;
    }
  }
}

void handle_mode_options() {
}


#define MODE_FIRST    0
#define MODE_PLAY     0
#define MODE_RECORD   1
#define MODE_OPTIONS  2
#define MODE_LAST     2

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
      default:
        input_callback();
      break;
    }
  }
}


int tapuino_hardware_setup(void)
{
  FRESULT res;
  uint8_t tmp;
  
  // enable TWI pullups
  TWI_PORT |= _BV(TWI_PIN_SDA);
  TWI_PORT |= _BV(TWI_PIN_SCL);
    
  // sense is output to C64
  SENSE_DDR |= _BV(SENSE_PIN);
  SENSE_OFF();
  
  // read is output to C64
  TAPE_READ_DDR |= _BV(TAPE_READ_PIN);
  TAPE_READ_HIGH();
  
  // write is input from C64, activate pullups
  TAPE_WRITE_DDR &= ~_BV(TAPE_WRITE_PIN);
  TAPE_WRITE_PORT |= _BV(TAPE_WRITE_PIN);
  
  // motor is input from C64, activate pullups
  MOTOR_DDR &= ~_BV(MOTOR_PIN);
  MOTOR_PORT |= _BV(MOTOR_PIN);
  
  // keys are all inputs, activate pullups
  KEYS_READ_DDR &= ~_BV(KEY_SELECT_PIN);
  KEYS_READ_PORT |= _BV(KEY_SELECT_PIN);

  KEYS_READ_DDR &= ~_BV(KEY_ABORT_PIN);
  KEYS_READ_PORT |= _BV(KEY_ABORT_PIN);

  KEYS_READ_DDR &= ~_BV(KEY_PREV_PIN);
  KEYS_READ_PORT |= _BV(KEY_PREV_PIN);

  KEYS_READ_DDR &= ~_BV(KEY_NEXT_PIN);
  KEYS_READ_PORT |= _BV(KEY_NEXT_PIN);
  
  signal_timer_setup();
  
  serial_init();
  serial_println_P(S_STARTINGINIT);
  lcd_setup();
  serial_println_P(S_INITI2COK);
  lcd_title_P(S_INIT);
  
  // something (possibly) dodgy in the bootloader causes a fail on cold boot.
  // retrying here seems to fix it (could just be the bootloader on my cheap Chinese clone?)
  for (tmp = 0; tmp < 10; tmp++) {
    res = f_mount(&g_fs, "", 1);
    _delay_ms(200);
    if (res == FR_OK) break;
  }
  
  if (res == FR_OK) {
    SPI_Speed_Fast();
    strcpy_P(g_char_buffer, S_DEFAULT_DIR);
    res = f_opendir(&g_dir, g_char_buffer);
  } else {
    lcd_title_P(S_INIT_FAILED);
  }

  return(res == FR_OK);
}

void tapuino_run()
{
  FILINFO file_info;
  file_info.lfname = (TCHAR*)g_fat_buffer;
  file_info.lfsize = sizeof(g_fat_buffer);

  if (!tapuino_hardware_setup()) {
    lcd_title_P(S_INIT_FAILED);
    return;
  }
  
  if ((g_num_files = get_num_files(&file_info)) == 0) {
    lcd_title_P(S_NO_FILES_FOUND);
    return;
  }
  while (1) {
    switch (handle_select_mode()) {
      case MODE_PLAY:
        handle_play_mode(&file_info);
      break;
      case MODE_RECORD:
        handle_mode_record(&file_info);
      break;
      case MODE_OPTIONS:
        handle_mode_options();
      break;
    }
  }
}
