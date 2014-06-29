#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "integer.h"
#include "spi.h"
#include "pff.h"
#include "diskio.h"
#include "serial.h"
#include "comms.h"
#include "lcd.h"
#include "memstrings.h"
#include "tapuino.h"


#define FAT_BUF_SIZE 256
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

#define MAX_LCD_LINE_LEN    20

static FATFS g_fs;

static DIR g_dir;

uint8_t g_fatBuffer[256];

static volatile uint8_t g_readIdx;
static volatile uint8_t g_halfWave;
static volatile uint8_t g_writeIdx;

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

void lcd_spinner() {
  static uint8_t indicators[] = {'|', '/', '-', 0};
  static uint8_t pos = 0;
  lcd_setCursor(MAX_LCD_LINE_LEN - 1, 0);
  lcd_write(indicators[pos++]);
  if (pos > 3) {
    pos = 0;
  }
}


void lcd_line(char* msg, int line, uint8_t usepgm)
{
  char buffer[MAX_LCD_LINE_LEN + 1] = {0};
  int len;
  strncpy_P(buffer, S_MAXBLANKLINE, MAX_LCD_LINE_LEN);
  
  lcd_setCursor(0, line);
  if (usepgm) {
    len = strlen_P(msg);
    memcpy_P(buffer, msg, len > MAX_LCD_LINE_LEN ? MAX_LCD_LINE_LEN : len);
  } else {
    len = strlen(msg);
    memcpy(buffer, msg, len > MAX_LCD_LINE_LEN ? MAX_LCD_LINE_LEN : len);
  }
  lcd_print(buffer);
  serial_println(buffer);
}

void lcd_title(char* msg)
{
  lcd_line(msg, 0, 0);
}

void lcd_titleP(const char* msg)
{
  lcd_line(msg, 0, 1);
}

void lcd_status(char* msg)
{
  lcd_line(msg, 1, 0);
}

void lcd_statusP(const char* msg)
{
  lcd_line(msg, 1, 1);
}

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

ISR(TIMER1_COMPA_vect) {
  static unsigned long pulse_length = 0;
  static unsigned long pulse_length_save;
  unsigned long tap_data;
  
  if (!MOTOR_IS_ON()) {
    return;
  }
  
  if (g_halfWave) {               // 2nd half of the signal
    if (pulse_length > 0xFFFF) {     // check to see if its bigger than 16 bits
      pulse_length -= 0xFFFF;
      OCR1A = 0xFFFF;
    } else {
      OCR1A = (unsigned short) pulse_length;
      pulse_length = 0;         // clear this!!!! for 1st half check so that the next data is loaded!
      g_halfWave = 0;           // next time round switch to 1st half
    }
    TAPE_READ_HIGH();           // set the signal high
  } else {                        // 1st half of the signal
    if (pulse_length) {                 // do we have any pulse left?
      if (pulse_length > 0xFFFF) {      // check to see if its bigger than 16 bits
        pulse_length -= 0xFFFF;
        OCR1A = 0xFFFF;
      } else {
        OCR1A = (unsigned short) pulse_length;
        pulse_length = pulse_length_save;   // restore pulse length for the 2nd half of the signal
        g_halfWave = 1;                     // next time round switch to 2nd half
      }
    } else {
      tap_data = (unsigned long) g_fatBuffer[g_readIdx++];
      if (tap_data == 0) {
        pulse_length =  (unsigned long) g_fatBuffer[g_readIdx++];
        pulse_length |= ((unsigned long) g_fatBuffer[g_readIdx++]) << 8;
        pulse_length |= ((unsigned long) g_fatBuffer[g_readIdx++]) << 16;
        pulse_length *= 1.015;
      } else {
        pulse_length = tap_data * 8.12;
      }
      pulse_length_save = pulse_length; // save this for the 2nd half of the wave
      if (pulse_length > 0xFFFF) {     // check to see if its bigger than 16 bits
        pulse_length -= 0xFFFF;
        OCR1A = 0xFFFF;
      } else {
        OCR1A = (unsigned short) pulse_length;
        pulse_length = pulse_length_save; // restore pulse length for the 2nd half of the signal
        g_halfWave = 1;                   // next time round switch to 2nd half
      }
      TAPE_READ_LOW();
    }
  }
}

void timer_setup() {
  TCCR1A = 0x00;   // clear timer registers
  TCCR1B = 0x00;
  TIMSK1 = 0x00;
 
  TCCR1B |=  _BV(CS11) | _BV(WGM12);  //prescaller 8 = 2 MHZ
}

void timer_start() {
  OCR1A = 0xFFFF;
  TCNT1 = 0;
  TIMSK1 |=  _BV(OCIE1A);
}

void timer_stop() {
  TIMSK1 &= ~_BV(OCIE1A);
}

BYTE play_file(char* pFile)
{
  FRESULT res;
  WORD br;
  g_exitFlag = 0;

  res = pf_open(pFile);
  if (res != FR_OK)
  {
    lcd_titleP(S_OPENFAILED);
    return(1);
  }

  // Initialize with very short pulses in case the TAP file
  // is smaller than the buffer
  memset(g_fatBuffer,1,256);

  res = pf_read((void*) g_fatBuffer, FAT_BUF_SIZE, &br);
  if (res != FR_OK)
  {
    lcd_titleP(S_READFAILED);
    return(1);
  }  
  
  if (strncmp((const char*) g_fatBuffer, "C64-TAPE-RAW", 12) != 0)
  {
    lcd_titleP(S_INVALIDTAP);
    return 1;
  }
  
  // Start send-ISR
  TAPE_READ_LOW();
  SENSE_ON();
  
  g_writeIdx = 0;
  g_readIdx = 20; // Skip header
  g_halfWave = 0;

  lcd_titleP(S_LOADING);
  timer_start();

  while (br > 0) {
    // Wait until ISR is in the new half of the buffer
    while ((g_readIdx & 0x80) == (g_writeIdx & 0x80)) ;
    pf_read((void*) g_fatBuffer + g_writeIdx, 128, &br);
    lcd_spinner();
    g_writeIdx += 128;
  }

  // Wait once more (last read tried to read new half of buffer,
  // but failed -> need to wait until ISR is in new half, but
  // g_writeIdx was incremented unconditionally -> wait until
  // ISR has left g_writeIdx-half of buffer)
  while (((g_readIdx & 0x80) == (g_writeIdx & 0x80)) && MOTOR_IS_ON()) ;

  timer_stop();

  SENSE_OFF();
  TAPE_READ_LOW();
  lcd_titleP(S_LOADCOMPLETE);

  g_curCommand = COMMAND_IDLE;
  return(0);
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
  
  timer_setup();
  
  SPI_Init();
  SPI_Speed_Slow();
  SPI_Send (0xFF);
  
  _delay_ms(200);
  SPI_Speed_Fast();
  serial_init();
  lcd_begin(0x27, 20, 4, LCD_5x8DOTS);
  lcd_backlight();
  lcd_createChar(0, backslashChar);
  lcd_titleP(S_INIT);
    
  res = pf_mount(&g_fs);
  if (res == FR_OK)
  {
    res = pf_opendir(&g_dir, DEFAULT_DIR);
  }
  else
  {
    lcd_titleP(S_INITFAILED);
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
    lcd_titleP(S_INITFAILED);
    return;
  }
  
  //if (!find_first_file(&file_info))
  if ((num_files = get_num_files(&file_info)) == 0)
  {
    lcd_titleP(S_NOFILES);
    return;
  }
  lcd_titleP(S_SELECTFILE);
  if (!get_file_at_index(&file_info, cur_file_index)) {
    lcd_titleP(S_NOFILES);
  }
  lcd_status(file_info.fname);

  while(1)
  {
    switch(g_curCommand)
    {
      case COMMAND_SELECT:
      {
        if (file_info.fattrib & AM_DIR) {
          if (change_dir(file_info.fname) == FR_OK) {
            num_files = get_num_files(&file_info);
            get_file_at_index(&file_info, cur_file_index);
            lcd_status(file_info.fname);
          } else {
            lcd_statusP(S_CHANGEDIRERROR);
          }
        } else {
          play_file(file_info.fname);
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
        g_curCommand = COMMAND_IDLE;
        break;
      }
      default:
        inputCallback();
      break;
    }
  }
}
