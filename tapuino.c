#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "integer.h"
#include "config.h"

#include "spi.h"
#include "pff.h"
#include "diskio.h"
#include "serial.h"
#include "comms.h"
#include "lcd.h"
#include "lcdutils.h"
#include "memstrings.h"



#define DEFAULT_DIR "/"
#define INVALID_FILE_ATTR   (AM_LFN | AM_VOL)

#define SENSE_PORT          PORTD
#define SENSE_DDR           DDRD
#define SENSE_PIN           5
#define SENSE_ON()          SENSE_PORT &= ~_BV(SENSE_PIN)
#define SENSE_OFF()         SENSE_PORT |=  _BV(SENSE_PIN)

#define TAPE_READ_PORT      PORTD
#define TAPE_READ_DDR       DDRD
#define TAPE_READ_PIN       3
#define TAPE_READ_PINS      PIND
#define TAPE_READ_LOW()     TAPE_READ_PORT &= ~_BV(TAPE_READ_PIN)
#define TAPE_READ_HIGH()    TAPE_READ_PORT |=  _BV(TAPE_READ_PIN)
#define TAPE_READ_TOGGLE()  TAPE_READ_PINS |=  _BV(TAPE_READ_PIN)

#define MOTOR_PORT          PORTD
#define MOTOR_DDR           DDRD
#define MOTOR_PIN           4
#define MOTOR_PINS          PIND
#define MOTOR_IS_ON()       (MOTOR_PINS & _BV(MOTOR_PIN))

#define C64_CYCLES_PAL      985248
#define C64_CYCLES_NTSC     1022730

#ifndef USE_NTSC_TIMING
  #define CYCLE_MULT_RAW    (1000000 / C64_CYCLES_PAL)
#else
  #define CYCLE_MULT_RAW    (1000000 / C64_CYCLES_NTSC)
#endif

#define CYCLE_MULT_8        (CYCLE_MULT_RAW * 8)

// max TAP delay is a 24-bit value i.e. 0xFFFFFF cycles
// we need this constant to determine if the loader has switched off the motor before the tap has completed
// which would cause the code to enter an endless loop (when the motor is off the buffers do not progress)
// so we check during the buffer wait loop to see if we have exceeded the maximum delay time and exit if so.
#define MAX_SIGNAL_CYCLES     (CYCLE_MULT_RAW * 0xFFFFFF * 2)

static FATFS g_fs;

static DIR g_dir;

uint8_t g_fat_buffer[FAT_BUF_SIZE];

static volatile uint8_t g_read_index;
static volatile uint8_t g_signal_2nd_half;
static volatile uint8_t g_write_index;
static volatile uint32_t g_total_timer_count;


int get_num_files(FILINFO* pfile_info)
{
  int num_files = 0;
  if (!pfile_info) {
    return 0;
  }
  // rewind directory to first file
  if (pf_readdir(&g_dir, 0) != FR_OK) {
    return 0;
  }
  
  while(1) {
    pfile_info->fname[0] = 0;
    if ((pf_readdir(&g_dir, pfile_info) != FR_OK) || !pfile_info->fname[0]) {
      break;
    }

    if (!(pfile_info->fattrib & INVALID_FILE_ATTR)) {
      num_files++;
    }
  }
  // add faked '..' entry for traversal
  if (g_fs.cdir != 0) {
    num_files ++;
  }
  return num_files;
}

int get_file_at_index(FILINFO* pfile_info, int index) {
  int cur_file_index = 0;

  if (!pfile_info) {
    return 0;
  }

  // rewind directory to first file
  if (pf_readdir(&g_dir, 0) != FR_OK) {
    return 0;
  }

  // are we in the root dir?
  if (g_fs.cdir != 0) {
    // and looking for the first indes?
    if (index == 0) {
      // then add the fake '..' entry and return
      memset(pfile_info, 0, sizeof(FILINFO));
      pfile_info->fattrib = AM_DIR;
      strcpy_P(pfile_info->fname, S_UP_A_DIR);
      return 1;
    } else {
      // otherwise decrement the index to point to the actual file we want
      index--;
    }
  }
  
  while(1) {
    pfile_info->fname[0] = 0;
    if ((pf_readdir(&g_dir, pfile_info) != FR_OK) || !pfile_info->fname[0]) {
      break;
    }

    if (!(pfile_info->fattrib & INVALID_FILE_ATTR)) {
      if (cur_file_index == index) {
        return 1;
      }
    }
    cur_file_index++;
  }
  return 0;  
}

// timer1 is running at 2MHz or 0.5 uS per tick.
// signal values are measured in uS, so OCR1A is set to the value from the TAP file (converted into uS) for each signal half
// i.e. TAP value converted to uS * 2 == full signal length
ISR(TIMER1_COMPA_vect) {
  static unsigned long pulse_length = 0;
  static unsigned long pulse_length_save;
  unsigned long tap_data;
  
  g_total_timer_count += OCR1A;
  
  if (!MOTOR_IS_ON()) {
    return;
  }
  
  if (g_signal_2nd_half) {                // 2nd half of the signal
    if (pulse_length > 0xFFFF) {          // check to see if its bigger than 16 bits
      pulse_length -= 0xFFFF;
      OCR1A = 0xFFFF;
    } else {
      OCR1A = (unsigned short) pulse_length;
      pulse_length = 0;                   // clear this, for 1st half check so that the next data is loaded
      g_signal_2nd_half = 0;              // next time round switch to 1st half
    }
    TAPE_READ_HIGH();                     // set the signal high
  } else {                                // 1st half of the signal
    if (pulse_length) {                   // do we have any pulse left?
      if (pulse_length > 0xFFFF) {        // check to see if its bigger than 16 bits
        pulse_length -= 0xFFFF;
        OCR1A = 0xFFFF;
      } else {
        OCR1A = (unsigned short) pulse_length;
        pulse_length = pulse_length_save; // restore pulse length for the 2nd half of the signal
        g_signal_2nd_half = 1;            // next time round switch to 2nd half
      }
    } else {
      g_total_timer_count = 0;
      tap_data = (unsigned long) g_fat_buffer[g_read_index++];
      if (tap_data == 0) {
        pulse_length =  (unsigned long) g_fat_buffer[g_read_index++];
        pulse_length |= ((unsigned long) g_fat_buffer[g_read_index++]) << 8;
        pulse_length |= ((unsigned long) g_fat_buffer[g_read_index++]) << 16;
        pulse_length *= 1.015;
      } else {
        pulse_length = tap_data * 8.12;
      }
      pulse_length_save = pulse_length;   // save this for the 2nd half of the wave
      if (pulse_length > 0xFFFF) {        // check to see if its bigger than 16 bits
        pulse_length -= 0xFFFF;
        OCR1A = 0xFFFF;
      } else {
        OCR1A = (unsigned short) pulse_length;
        pulse_length = pulse_length_save; // restore pulse length for the 2nd half of the signal
        g_signal_2nd_half = 1;            // next time round switch to 2nd half
      }
      TAPE_READ_LOW();
    }
  }
}

void signal_timer_setup() {
  TCCR1A = 0x00;   // clear timer registers
  TCCR1B = 0x00;
  TIMSK1 = 0x00;
 
  TCCR1B |=  _BV(CS11) | _BV(WGM12);  //prescaller 8 = 2 MHZ
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

int play_file(FILINFO* pfile_info)
{
  FRESULT res;
  WORD br;

  res = pf_open(pfile_info->fname);
  if (res != FR_OK)
  {
    lcd_title_P(S_OPEN_FAILED);
    return 0;
  }

  // Initialize with very short pulses in case the TAP file
  // is smaller than the buffer
  memset(g_fat_buffer,1,256);

  res = pf_read((void*) g_fat_buffer, FAT_BUF_SIZE, &br);
  if (res != FR_OK)
  {
    lcd_title_P(S_READ_FAILED);
    return 0;
  }
  
  if (strncmp_P((const char*) g_fat_buffer, S_TAP_MAGIC_C64, 12) != 0)
  {
    lcd_title_P(S_INVALID_TAP);
    return 0;
  }
  
  // Start send-ISR
  TAPE_READ_LOW();
  SENSE_ON();
  
  g_write_index = 0;
  g_read_index = 20; // Skip header
  g_signal_2nd_half = 0;

  lcd_title_P(S_LOADING);
  signal_timer_start();

  while (br > 0) {
    // Wait until ISR is in the new half of the buffer
    while ((g_read_index & 0x80) == (g_write_index & 0x80)) {
      input_callback();
      lcd_spinner(50000);
      // if the load was aborted or the C64 stopped the motor for longer than the longest possible signal time
      // then we need to get out of here
      if ((g_total_timer_count > MAX_SIGNAL_CYCLES) || (g_curCommand == COMMAND_ABORT)) {
        break;
      }
    }
    pf_read((void*) g_fat_buffer + g_write_index, 128, &br);
    input_callback();
    if (g_curCommand == COMMAND_ABORT) {
      break;
    }
    g_write_index += 128;
  }

  // Wait once more (last read tried to read new half of buffer,
  // but failed -> need to wait until ISR is in new half, but
  // g_write_index was incremented unconditionally -> wait until
  // ISR has left g_write_index-half of buffer)
  while (((g_read_index & 0x80) == (g_write_index & 0x80)) && MOTOR_IS_ON() && (g_curCommand != COMMAND_ABORT)) ;

  signal_timer_stop();

  SENSE_OFF();
  TAPE_READ_LOW();
  if (g_curCommand == COMMAND_ABORT) {
    lcd_title_P(S_LOADING_ABORTED);
  } else {
    lcd_title_P(S_LOADING_COMPLETE);
  }

  for (br = 0; br < 100; br++) {
    lcd_spinner(0);
    _delay_ms(20);
  }

  g_curCommand = COMMAND_IDLE;
  return 1;
}

int player_hardwareSetup(void)
{
  FRESULT res;
  SENSE_DDR |= _BV(SENSE_PIN);
  SENSE_OFF();
  TAPE_READ_DDR |= _BV(TAPE_READ_PIN);
  TAPE_READ_HIGH();
  MOTOR_DDR &= ~_BV(MOTOR_PIN);
  MOTOR_PORT |= _BV(MOTOR_PIN);
  
  signal_timer_setup();
  
  SPI_Init();
  SPI_Speed_Slow();
  SPI_Send (0xFF);
  
  _delay_ms(200);
  SPI_Speed_Fast();
  serial_init();
  lcd_setup();
  lcd_title_P(S_INIT);
    
  res = pf_mount(&g_fs);
  if (res == FR_OK)
  {
    res = pf_opendir(&g_dir, DEFAULT_DIR);
  }
  else
  {
    lcd_title_P(S_INIT_FAILED);
  }

  return(res == FR_OK);
}

FRESULT change_dir(char* dir) {
  FRESULT fr = FR_OK;
  if ((fr = pf_chdir(dir)) == FR_OK) {
    return pf_opendir( &g_dir, "." );
  }
  return fr;
}

int num_files = 0;
int cur_file_index = 0;

void player_run()
{
  FILINFO file_info;

  if (!player_hardwareSetup())
  {
    lcd_title_P(S_INIT_FAILED);
    return;
  }
  
  //if (!find_first_file(&file_info))
  if ((num_files = get_num_files(&file_info)) == 0)
  {
    lcd_title_P(S_NO_FILES_FOUND);
    return;
  }
  lcd_title_P(S_SELECT_FILE);
  if (!get_file_at_index(&file_info, cur_file_index)) {
    // shouldn't happen...
    lcd_title_P(S_NO_FILES_FOUND);
    return;
  }
  lcd_status(file_info.fname);
  if (file_info.fattrib & AM_DIR) {
    lcd_show_dir();
  }
        
  while(1)
  {
    switch(g_curCommand)
    {
      case COMMAND_SELECT:
      {
        if (file_info.fattrib & AM_DIR) {
          if (change_dir(file_info.fname) == FR_OK) {
            num_files = get_num_files(&file_info);
            cur_file_index = 0;
            get_file_at_index(&file_info, cur_file_index);
            lcd_status(file_info.fname);
            lcd_show_dir();
          } else {
            lcd_status_P(S_DIRECTORY_ERROR);
          }
        } else {
          play_file(&file_info);
          lcd_title_P(S_SELECT_FILE);
          lcd_status(file_info.fname);
        }
        g_curCommand = COMMAND_IDLE;
        break;
      }
      case COMMAND_NEXT:
      {
        if (++cur_file_index >= num_files) {
          cur_file_index = 0;
        }
        get_file_at_index(&file_info, cur_file_index);
        lcd_status(file_info.fname);
        if (file_info.fattrib & AM_DIR) {
          lcd_show_dir();
        }
        g_curCommand = COMMAND_IDLE;
        break;
      }
      case COMMAND_PREVIOUS:
      {
        if (--cur_file_index < 0) {
          cur_file_index = num_files - 1;
        }
        get_file_at_index(&file_info, cur_file_index);
        lcd_status(file_info.fname);
        if (file_info.fattrib & AM_DIR) {
          lcd_show_dir();
        }
        g_curCommand = COMMAND_IDLE;
        break;
      }
      default:
        input_callback();
      break;
    }
  }
}
