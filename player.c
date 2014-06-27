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
#include "player.h"
#include "serial.h"
#include "delayMicroseconds.h"

#define RECV_WAIT_START     1
#define RECV_WAIT_CMD       2
#define RECV_WAIT_DATA      3

#define COMMAND_IDLE        0
#define COMMAND_PLAY        1
#define COMMAND_NEXT        2

#define CMD_ARG_MAX 12
#define FAT_BUF_SIZE 256
#define DEFAULT_DIR "/"
#define INVALID_FILE_ATTR   (AM_DIR | AM_LFN | AM_VOL)

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

static FATFS g_fs;

static DIR g_dir;

static BYTE g_recvState = RECV_WAIT_START;
static BYTE g_pendingCommand = COMMAND_IDLE;
static BYTE g_curCommand = COMMAND_IDLE;

static BYTE g_cmdArgPos = 0;
char g_cmdArg[ CMD_ARG_MAX + 1 ];

static BYTE g_exitFlag = 0;

uint8_t g_fatBuffer[256];
static volatile uint8_t g_readIdx;

static volatile uint8_t g_halfWave;
static volatile uint8_t g_writeIdx;

BYTE player_handleInput(char ch);

void responseCallback(char* msg)
{
  serial_println(msg);
}

unsigned char inputCallback()
{
  if (serial_available() > 0)
  {
    char ch = serial_read();
    return(player_handleInput(ch));
  }
  return(0);
}

BYTE player_handleInput(char ch)
{
  if (ch >= 'a' && ch <= 'z')
  {
    ch = (ch - 'a') + 'A';
  }
  
  switch(g_recvState)
  {
    case RECV_WAIT_START:
    {
      if (ch == ':')
      {
        g_recvState = RECV_WAIT_CMD;
      }
      break;
    }
    
    case RECV_WAIT_CMD:
    {
      switch(ch)
      {
        case 'P':
        {
          g_pendingCommand = COMMAND_PLAY;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'N':
        {
          g_pendingCommand = COMMAND_NEXT;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        default:
        {
          g_recvState = RECV_WAIT_START;
          break;
        }
      }
      break;
    }
    
    case RECV_WAIT_DATA:
    {
      if (ch != '\n')
      {
        if (ch == '\r')
        {
          // ignore carrige return
          return(0);
        }
        if (g_cmdArgPos < CMD_ARG_MAX)
        {
          g_cmdArg[ g_cmdArgPos++ ] = ch;
        }
        else
        {
          // command is too long, ignore and reset for next command
          g_recvState = RECV_WAIT_START;
          g_cmdArgPos = 0;
          g_cmdArg[ 0 ] = 0;
        }
      }
      else
      {
        // null terminate and reset state
        g_cmdArg[ g_cmdArgPos ] = 0;
        g_cmdArgPos = 0;
        g_recvState = RECV_WAIT_START;
        
        switch(g_pendingCommand)
        {
          default:
          {
            g_curCommand = g_pendingCommand;
            g_exitFlag = 1;
            break;
          }
        }
      }
      break;
    }
  }
  return(0);
}


int find_first_file(char* pFile, FILINFO* pfile_info)
{
  FRESULT res;

  if (!pfile_info)
  {
    return(0);
  }
  // rewind directory to first file
  pf_readdir(&g_dir, 0);
  
  while(1)
  {
    pfile_info->fname[0] = 0;
    res = pf_readdir(&g_dir, pfile_info);
    if (res != FR_OK || !pfile_info->fname[0])
    {
      break;
    }

    if (!(pfile_info->fattrib & INVALID_FILE_ATTR))
    {
      responseCallback(pfile_info->fname);
      if (strstr(pfile_info->fname, ".TAP") != 0)
      {
        if (!pFile || *pFile == 0 || !strncmp(pfile_info->fname, pFile, 12))
        {
          // found the file we were looking for or a playable file
          return(1);
        }
      }
    }
  }
  return(0);
}

int find_next_file(FILINFO* pfile_info)
{
  FRESULT res;
  BYTE count = 0;

  if (!pfile_info)
  {
    return(0);
  }
  
  while(1)
  {
    pfile_info->fname[0] = 0;
    res = pf_readdir(&g_dir, pfile_info);
    if (res != FR_OK)
    {
      break;
    }
    
    if (!pfile_info->fname[0])
    {
      if (count++ < 1)
      {
        // rewind directory to first file and start from the top
        pf_readdir(&g_dir, 0);
        continue;
      }
      return(0);
    }

    if (!(pfile_info->fattrib & INVALID_FILE_ATTR))
    {
      responseCallback(pfile_info->fname);
      if (strstr(pfile_info->fname, ".TAP") != 0)
      {
        return(1);
      }
    }
  }
  return(0);
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

//purple green gray brown cyan green
BYTE play_file(char* pFile)
{
  FRESULT res;
  WORD br;
  g_exitFlag = 0;

  res = pf_open(pFile);
  if (res != FR_OK)
  {
    responseCallback("pf_open failed");
    return(1);
  }

  // Initialize with very short pulses in case the TAP file
  // is smaller than the buffer
  memset(g_fatBuffer,1,256);

  res = pf_read((void*) g_fatBuffer, FAT_BUF_SIZE, &br);
  if (res != FR_OK)
  {
    responseCallback("pf_read failed");
    return(1);
  }  
  
  if (strncmp((const char*) g_fatBuffer, "C64-TAPE-RAW", 12) != 0)
  {
    responseCallback("Invalid TAP file!");
    return 1;
  }
  
  cli();
  
  TCCR1A = 0x00;   // clear timer registers
  TCCR1B = 0x00;
  TIMSK1 = 0x00;
 
  //TCCR1A |=  _BV(WGM12);
  TCCR1B |=  _BV(CS11) | _BV(WGM12);  //prescaller 8 = 2 MHZ
  
  sei();
  serial_println("Loading...");
  if (MOTOR_IS_ON()) {
    serial_println("Motor on");
  }
  
  // Start send-ISR
  TAPE_READ_LOW();
  SENSE_ON();

  g_writeIdx = 0;
  g_readIdx = 20; // Skip header
  g_halfWave = 0;
  _delay_ms(100);

  OCR1A = 0xFFFF;
  TCNT1 = 0;
  TIMSK1 |=  _BV(OCIE1A);

  while (br > 0) {
    // Wait until ISR is in the new half of the buffer
    while ((g_readIdx & 0x80) == (g_writeIdx & 0x80)) ;
    pf_read((void*) g_fatBuffer + g_writeIdx, 128, &br);
    g_writeIdx += 128;
  }

  // Wait once more (last read tried to read new half of buffer,
  // but failed -> need to wait until ISR is in new half, but
  // g_writeIdx was incremented unconditionally -> wait until
  // ISR has left g_writeIdx-half of buffer)
  while ((g_readIdx & 0x80) == (g_writeIdx & 0x80)) ;

//  pf_close();

  TIMSK1 &= ~_BV(OCIE1A);

  SENSE_OFF();
  TAPE_READ_LOW();
  serial_println("Loading complete!");

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
  
  SPI_Init();
  SPI_Speed_Slow();
  SPI_Send (0xFF);
  
  _delay_ms(200);
  SPI_Speed_Fast();
  serial_init();
      
  res = pf_mount(&g_fs);
  if (res == FR_OK)
  {
    res = pf_opendir(&g_dir, DEFAULT_DIR);
  }
  else
  {
    responseCallback(":risd mount failed");
  }

  return(res == FR_OK);
}



void player_run()
{
  FILINFO file_info;

  if (!player_hardwareSetup())
  {
    responseCallback("fail");
    return;
  }
  if (!find_first_file(NULL, &file_info))
  {
    responseCallback(":rpno file");
    return;
  }
  responseCallback("run");

  while(1)
  {
    switch(g_curCommand)
    {
      case COMMAND_PLAY:
      {
        responseCallback(file_info.fname);
        play_file(file_info.fname);
        //send_file();
        break;
      }
      case COMMAND_NEXT:
      {
        find_next_file(&file_info);
        g_curCommand = COMMAND_IDLE;
        break;
      }
      default:
        inputCallback();
      break;
    }
  }
}
