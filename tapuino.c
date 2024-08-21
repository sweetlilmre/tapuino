#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "version.h"
#include "integer.h"
#include "config.h"

#include "spi.h"
#include "ff.h"
#include "mmc.h"
#include "diskio.h"
#include "serial.h"
#include "comms.h"
#include "lcd_interface.h"
#include "lcdutils.h"
#include "memstrings.h"
#include "fileutils.h"
#include "menu.h"


typedef enum
{
  C64,
  VIC,
  C16,
} MACHINE_TYPE;

typedef enum
{
  PAL,
  NSTC,
} VIDEO_MODE;


// magic strings found in the TAP header, represented as uint32_t values in little endian format (reversed string)
#define TAP_MAGIC_C64      0x2D343643   // "C64-"  as "-46C"
#define TAP_MAGIC_C16      0x2D363143   // "C16-"  as "-61C"
#define TAP_MAGIC_POSTFIX1 0x45504154   // "TAPE"  as "EPAT"
#define TAP_MAGIC_POSTFIX2 0x5741522D   // "-RAW"  as "WAR-"

struct TAP_INFO {
  uint8_t version;              // TAP file format version:
                                // Version  0: 8-bit data, 0x00 indicates overflow
                                //          1: 8-bit, 0x00 indicates 24-bit overflow to follow
                                //          2: same as 1 but with 2 half-wave values
  uint8_t platform;             // Platform 0: C64
                                //          1: VIC
                                //          2: C16
  uint8_t video;                // Video    0: PAL 
                                //          1: NTSC
  uint8_t reserved;
  volatile uint32_t length;     // total length of the TAP data excluding header in bytes
  volatile uint32_t cycles;     // 
};

static struct TAP_INFO g_tap_info;

// the maximum TAP delay is a 24-bit value i.e. 0xFFFFFF cycles
// we need this constant to determine if the loader has switched off the motor before the tap has completed
// which would cause the code to enter an endless loop (when the motor is off the buffers do not progress)
// so we check during the buffer wait loop to see if we have exceeded the maximum delay time and exit if so.
#define MAX_SIGNAL_CYCLES     (g_cycle_mult_raw * 0xFFFFFF * 2)

// helper variables for the ISR and loader code

static volatile uint8_t g_read_index;           // read index in the 256 byte buffer
static volatile uint8_t g_write_index;          // write index in the 256 byte buffer
static volatile uint8_t g_signal_2nd_half;      // dual use flag: flag to indicate that the 2nd half of the signal should be output, or that recording has started
static volatile uint32_t g_total_timer_count;   // number of (AVR) cycles that the timer has been running for
static volatile uint8_t g_tap_file_complete;    // flag to indicate that all bytes have been read from the TAP
static volatile uint32_t g_tap_file_pos;        // current read position in the TAP (bytes)
static uint32_t g_pulse_length = 0;             // length of pulse in uS
static uint32_t g_pulse_length_save;            // saved length for 2nd half of the signal
static volatile uint32_t g_overflow;            // write signal overflow timer detection
static volatile uint32_t g_timer_tick = 0;      // timer tick at 100Hz (10 ms interval)

uint8_t g_machine_type = C64;
uint8_t g_video_mode = PAL;
static double g_cycle_mult_raw = 0;             // multiplier to convert a 24 bit TAP value into uS
static double g_cycle_mult_8 = 0;               // multiplier to convert an 8 bit TAP value into uS

volatile uint8_t g_invert_signal = 0;           // invert the signal for transmission/reception to/from a real Datasette

// all rate values are stored as milliseconds / 10.
// the global ticker runs at 100Hz which then maps 1-1 with these values
// So e.g. the key read function in comms.c: input_callback() is called by the global 100Hz timer, in this function g_key_repeat_next is used to determine the time between key repeats.
// each time this value is decremented 10 ms have passed, so a value of 100 is 100*10 = 1000 ms or 1 second repeat.

volatile uint16_t g_ticker_rate = TICKER_RATE / 10;
volatile uint16_t g_ticker_hold_rate = TICKER_HOLD / 10;
// volatile uint16_t g_key_repeat_start = KEY_REPEAT_START / 10;
volatile uint16_t g_key_repeat_next = KEY_REPEAT_NEXT / 10;
volatile uint16_t g_rec_finalize_time = REC_FINALIZE_TIME / 10;

volatile uint8_t g_rec_auto_finalize = 1;

volatile uint8_t g_is_paused = 0;

void setup_cycle_timing() {
  double ntsc_cycles_per_second;
  double pal_cycles_per_second;

  switch (g_machine_type)
  {
    case C64:
      ntsc_cycles_per_second = 1022272;
      pal_cycles_per_second  = 985248;
    break;
    case VIC:
      ntsc_cycles_per_second = 1022727;
      pal_cycles_per_second  = 1108404;
    break;
    case C16:
      ntsc_cycles_per_second = 894886;
      pal_cycles_per_second  = 886724;
    break;
  }
  
  switch(g_video_mode)
  {
    case PAL:
      g_cycle_mult_raw = (1000000.0 / pal_cycles_per_second);
    break;
    case NSTC:
      g_cycle_mult_raw = (1000000.0 / ntsc_cycles_per_second);
    break;
  }
  g_cycle_mult_8   = (g_cycle_mult_raw * 8.0);
}

uint32_t get_timer_tick() {
  return g_timer_tick;
}

// on rising edge of signal from C64
ISR(TIMER1_CAPT_vect) {
  uint32_t tap_data;

  if(g_signal_2nd_half) {                             // finished measuring 
    g_pulse_length = ICR1;                            // pulse length = ICR1 i.e. timer count
    g_pulse_length += (g_overflow << 16);             // add overflows
    g_pulse_length >>= 4;                             // turn raw tick values at 1/16th us into 1us i.e. divide by 16
    // start counting here
    g_overflow = 0;
    TCNT1 = 0;
    tap_data = g_pulse_length / g_cycle_mult_8;         // get the standard divided value
    
    if (tap_data == 0) {                              // safe guard
      tap_data++;
    }
    if (tap_data < 256) {                             // short signal
      g_fat_buffer[g_read_index++] = (uint8_t) tap_data;
      g_tap_file_pos++;
    } else {                                          // long signal
      tap_data = g_pulse_length / g_cycle_mult_raw;     // get the raw divided value (cycles)
      g_fat_buffer[g_read_index++] = 0;
      g_fat_buffer[g_read_index++] = (uint8_t) (tap_data & 0xff);
      g_fat_buffer[g_read_index++] = (uint8_t) ((tap_data & 0xff00) >> 8);
      g_fat_buffer[g_read_index++] = (uint8_t) ((tap_data & 0xff0000) >> 16);
      g_tap_file_pos += 4;
    }
  } else {
    // if this is the first time in, zero the count, otherwise the count starts at the high edge of the signal in the code above
    g_signal_2nd_half = 1;
    g_overflow = 0;
    TCNT1 = 0;
  }
  
}

ISR(TIMER1_OVF_vect){
  if (!MOTOR_IS_OFF()) {
    g_overflow++;
  }
}

// timer1 is running at 2MHz or 0.5 uS per tick.
// signal values are measured in uS, so OCR1A is set to the value from the TAP file (converted into uS) for each signal half
// for v0 and v1 TAP files that means that we will use the same signal value twice (1 for each half of the wave)
// i.e. TAP value converted to uS * 2 == full signal length
// for v2 TAP files we will read the 2nd half of the signal from the TAP file for the 2nd half of the wave
//
// Thanks to @matsstaff for his contributions on refactoring this code
ISR(TIMER1_COMPA_vect) {
  uint32_t tap_data;

  // keep track of the number of cycles in case we get to a MOTOR stop situation before the TAP has completed
  g_total_timer_count += OCR1A;
  
  // don't process if the MOTOR is off!
  if (MOTOR_IS_OFF() || g_is_paused) {
    return;
  }

  if (g_pulse_length == 0) {                // If this is the first time in, or the current pulse time has elapsed, then
    if (g_pulse_length_save != 0) {         // if g_pulse_length_save > 0 then we have a non v2 format, so restore the pulse value and count down again for the 2nd half
      g_pulse_length = g_pulse_length_save;
      g_pulse_length_save = 0;              // making sure to clear g_pulse_length_save!
    } else {                                // otherwise load a new pulse value from the TAP file
      g_total_timer_count = 0;

      if (g_tap_file_pos >= g_tap_info.length) {  // basic sanity check to see if we have any TAP data left!
        g_tap_file_complete = 1;
        return;                             // reached the end of the TAP file so don't process any more!
      }

      tap_data = (uint32_t) g_fat_buffer[g_read_index++];

                                            // code for v0 TAP handling, a zero in a v0 file indicates an overflow, we replace by 256
      if (g_tap_info.version == 0 && tap_data == 0) {
        tap_data = 256;
      }        
                                            // if tap_data != 0 then this is a simple 8 bit pulse
      if (tap_data != 0) {
        g_pulse_length = tap_data * g_cycle_mult_8;
        g_tap_file_pos++;
      } else {                              // otherwise we have a 24 bit pulse value, read it from the buffer
        g_pulse_length =  (uint32_t) g_fat_buffer[g_read_index++];
        g_pulse_length |= ((uint32_t) g_fat_buffer[g_read_index++]) << 8;
        g_pulse_length |= ((uint32_t) g_fat_buffer[g_read_index++]) << 16;
        g_pulse_length *= g_cycle_mult_raw;
        g_tap_file_pos += 3;
      }

      if (g_tap_info.version == 2) {
        g_pulse_length <<= 1;               // TAP v2 format is half-wave and the timer is running at 2Mhz so double the value
      } else {
        g_pulse_length_save = g_pulse_length; // otherwise save the pulse length for the 2nd half of a v0 or v1 wave
      }
    }
  }

                                            // Set output signal. The wave transition is HIGH->LOW for the 1st half of the signal, LOW->HIGH for the 2nd half of the signal
                                            // if the signal is inverted we have the opposite condition
                                            // 
  if (g_invert_signal ^ g_signal_2nd_half) {
    TAPE_READ_HIGH();                       // set the signal high
  } else {
    TAPE_READ_LOW();                        // set the signal low
  }

                                            // Set timer
  if (g_pulse_length > 0xFFFF) {            // check to see if its bigger than 16 bits
    g_pulse_length -= 0xFFFF;
    OCR1A = 0xFFFF;
  } else {
    OCR1A = (unsigned short) g_pulse_length;
    g_pulse_length = 0;
    g_signal_2nd_half = !g_signal_2nd_half;  // Flag that we will be processing the 2nd half of the signal next time the interrupt occurs
  }
}


ISR(TIMER2_COMPA_vect) {
  disk_timerproc(); // Drive timer procedure for FatFs low level disk I/O module
  input_callback();
  g_timer_tick++;   // system ticker for timing
}

void disk_timer_setup() {
  TCCR2A = 0;
  TCCR2B = 0;
  
  OCR2A = F_CPU / 1024 / 100 - 1; // 100Hz timer
  TCCR2A = _BV(WGM21);            // CTC Mode
  TCCR2B |=  (1 << CS22) | (1 << CS21) | (1 << CS20);  //pre-scaler 1024 
  TIMSK2 |= _BV(OCIE2A);
}

void signal_timer_start(uint8_t recording) {
  TCCR1A = 0x00;   // clear timer registers
  TCCR1B = 0x00;
  TIMSK1 = 0x00;
  TCNT1  = 0x00;

  if (recording) {
    g_overflow = 0;
    TCCR1B = _BV(CS10) | _BV(ICNC1);    // input capture, FALLING edge, pre-scaler 1 = 16 MHZ, Input Capture Noise Canceller
    if (!g_invert_signal) {
      TCCR1B |= _BV(ICES1);             // switch to RISING edge
    }
    TIMSK1 = _BV(ICIE1) | _BV(TOIE1);   // input capture interrupt enable, overflow interrupt enable
  } else {
    g_total_timer_count = 0;
    TCCR1B |=  _BV(CS11) | _BV(WGM12);  // pre-scaler 8 = 2 MHZ, CTC Mode
    OCR1A = 0xFFFF;
    TIMSK1 |=  _BV(OCIE1A);             // output compare interrupt
  }
}

void signal_timer_stop() {
  // stop all timer1 interrupts
  TIMSK1 = 0;
}

int verify_tap(FILINFO* pfile_info) {
  FRESULT res;
  UINT br;

  memset(&g_tap_info, 0, sizeof(g_tap_info));

  res = f_open(&g_fil, pfile_info->fname, FA_READ);
  if (res != FR_OK) {
    lcd_title_P(S_OPEN_FAILED);
    return 0;
  }

  res = f_read(&g_fil, (void*) g_fat_buffer, 12, &br);
  if (res != FR_OK) {
    lcd_title_P(S_READ_FAILED);
    return 0;
  }

  res = f_read(&g_fil, (void*) &g_tap_info, 8, &br);
  if (res != FR_OK) {
    lcd_title_P(S_READ_FAILED);
    return 0;
  }

  // check size first
  if (g_tap_info.length != (pfile_info->fsize - 20)) {
    lcd_title_P(S_INVALID_SIZE);
    g_tap_info.length = pfile_info->fsize - 20;
    lcd_busy_spinner();
  }
  
  uint32_t* tap_magic = (uint32_t*) g_fat_buffer;

  // check the post fix for "TAPE-RAW", use a 4-byte magic trick
  if (tap_magic[1] != TAP_MAGIC_POSTFIX1 || tap_magic[2] != TAP_MAGIC_POSTFIX2) {
    lcd_title_P(S_INVALID_TAP);
    return 0;
  }
  
  // now check type: C16 or C64, use a 4-byte magic trick
  if (tap_magic[0] != TAP_MAGIC_C64 && tap_magic[0] != TAP_MAGIC_C16 ) {
    lcd_title_P(S_INVALID_TAP);
    return 0;
  }

  // get the first buffer ready
  res = f_read(&g_fil, (void*) g_fat_buffer, FAT_BUF_SIZE, &br);
  if ((res != FR_OK) || (br < FAT_BUF_SIZE)) {
    lcd_title_P(S_READ_FAILED);
    return 0;
  }

  return 1;
}

int play_file(FILINFO* pfile_info)
{
  int perc = 0;
  g_tap_file_complete = 0;
  
  setup_cycle_timing();
  
  if (!verify_tap(pfile_info)) {
    lcd_busy_spinner();
    return 0;
  }

  // verify_tap() has read the full buffer (FAT_BUF_SIZE) into g_fat_buffer (or failed).
  UINT br = FAT_BUF_SIZE;

  // setup all start conditions
  g_write_index = 0;
  g_read_index = 0;
  g_tap_file_pos = 0;
  g_signal_2nd_half = 0;
  g_is_paused = 0;
  g_pulse_length = 0;
  g_pulse_length_save = 0;

  lcd_title_P(S_LOADING);

  if (g_invert_signal) {
    TAPE_READ_LOW();                     // set the signal low
  } else {
    TAPE_READ_HIGH();                   // set the signal high
  }
  SENSE_ON();
  // Start send-ISR
  signal_timer_start(0);

  while (br > 0) {
    // Wait until ISR is in the new half of the buffer
    while ((g_read_index & 0x80) == (g_write_index & 0x80)) {
      // feedback to the user
      lcd_spinner(g_timer_tick, perc);

      // for multiload games we need to remove the previous timeout fix
      // (checking against g_total_timer_count > MAX_SIGNAL_CYCLES) and look for 
      // g_tap_file_complete instead.
      if (g_tap_file_complete || (g_cur_command == COMMAND_ABORT)) {
        g_tap_file_complete = 1;
        break;
      }
      if (g_cur_command == COMMAND_SELECT) {
        g_cur_command = COMMAND_IDLE;
        g_is_paused = !g_is_paused;
      }
    }

    // exit outer while
    if (g_tap_file_complete) {
      break;
    }
    
    f_read(&g_fil, (void*) g_fat_buffer + g_write_index, 128, &br);
    g_write_index += 128;
    perc = (g_tap_file_pos * 100) / g_tap_info.length;
  }

  // wait for the remaining buffer to be read.
  while (!g_tap_file_complete) {
    // feedback to the user
    lcd_spinner(g_timer_tick, perc);
    // we need to do the same trick as above, BC's Quest for Tires stops the motor right near the
    // end of the tape, then restarts for the last bit of data, so we can't rely on the motor signal
    // a better approach might be to see if we have read all the data and then break. //
    if ((g_cur_command == COMMAND_ABORT) || (g_total_timer_count > MAX_SIGNAL_CYCLES)) {
      break;
    }
  }

  signal_timer_stop();
  f_close(&g_fil);
  
  if (g_invert_signal) {
    TAPE_READ_LOW();                     // set the signal high
  } else {
    TAPE_READ_HIGH();                   // set the signal high
  }
  SENSE_OFF();

  if (g_cur_command == COMMAND_ABORT) {
    lcd_title_P(S_OPERATION_ABORTED);
  } else {
    lcd_title_P(S_OPERATION_COMPLETE);
  }

  // end of load UI indicator
  lcd_busy_spinner();
  
  // switch off read LED
  TAPE_READ_LOW();
  
  // prevent leakage of g_cur_command, the standard mechanism is to use get_cur_command() which would clear the global
  g_cur_command = COMMAND_IDLE;
  
  return 1;
}


void record_file(char* pfile_name) {
  FRESULT res;
  UINT br;
  uint32_t tmp = 0;
  g_tap_file_complete = 0;
  g_tap_file_pos = 0;

  setup_cycle_timing();
  
  if (pfile_name == NULL) {
    // generate a filename
    br = 0;
    while (1) {
      sprintf_P((char*)g_fat_buffer, S_NAME_PATTERN, br);
      res = f_open(&g_fil, (char*)g_fat_buffer, FA_READ);
      if (res != FR_OK) {
        break;
      }
      f_close(&g_fil);
      br++;
    }
    pfile_name = (char*)g_fat_buffer;
  }
  
  res = f_open(&g_fil, pfile_name, FA_CREATE_NEW | FA_WRITE);

  if (res != FR_OK) {
    lcd_status_P(S_OPEN_FAILED);
    lcd_busy_spinner();
    return;
  }
  
  REC_LED_ON();
  
  lcd_title_P(S_RECORDING);
  lcd_status(pfile_name);
  
  // write TAP header
  memset (g_fat_buffer, 0, sizeof(g_fat_buffer));   // clear it all out
  uint32_t* buffer_magic = (uint32_t*) g_fat_buffer;
  //set appropriate header informations for C64 or C16  
  if (g_machine_type == C16) {
    buffer_magic[0] = TAP_MAGIC_C16;
    g_fat_buffer[13] = C16;
  } else {
    buffer_magic[0] = TAP_MAGIC_C64;
    g_fat_buffer[13] = C64;
  }
  buffer_magic[1] = TAP_MAGIC_POSTFIX1;             // copy the magic to the buffer: "TAPE"
  buffer_magic[2] = TAP_MAGIC_POSTFIX2;             // copy the magic to the buffer: "-RAW"
  g_fat_buffer[12] = 0x01;                          // get to the end of the magic and set TAP format to 1
  f_write(&g_fil, (void*)g_fat_buffer, 20, &br);    // write out the header with zero length field
  memset (g_fat_buffer, 0, sizeof(g_fat_buffer));   // clear it all out again for the read / write operation

  g_write_index = 0;
  g_read_index = 0;
  g_tap_file_pos = 0; // header is already written so don't add it on here
  // set flag to indicate the start of recording
  g_signal_2nd_half = 0;
  
  SENSE_ON();
  while (MOTOR_IS_OFF()) {
    if (g_cur_command == COMMAND_ABORT) {
      break;
    }
    // feedback to the user
    lcd_spinner(g_timer_tick, -1);
  }

  // activate pull-up
  TAPE_WRITE_PORT |= _BV(TAPE_WRITE_PIN);
  // Start recv-ISR
  signal_timer_start(1);

  while (!g_tap_file_complete) {
    while ((g_read_index & 0x80) == (g_write_index & 0x80)) {
      // nasty bit of code to wait after the motor is shut off to finalise the TAP
      if (MOTOR_IS_OFF() && g_rec_auto_finalize) {
        // use the 100Hz timer to wait some time after the motor has shut off
        if ((g_timer_tick - tmp) > (uint32_t) g_rec_finalize_time) {
          g_tap_file_complete = 1;
          break;
        }
      } else { // reset here while the motor is on so that we have the most current count
        tmp = g_timer_tick;
      }
      // feedback to the user
      lcd_spinner(g_timer_tick, -1);
      if ((g_cur_command == COMMAND_ABORT)) {
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
    
  }
  
  // finish any remaining writes
  if (g_write_index != g_read_index) {
    res = f_write(&g_fil, (void*) g_fat_buffer + g_write_index, g_read_index & 0x7F, &br);
  }

  signal_timer_stop();
  SENSE_OFF();
 
  // write in the length of the TAP file
  f_lseek(&g_fil, 0x0010);
  f_write(&g_fil, (void*) &g_tap_file_pos, 4, &br);
  f_close(&g_fil);
  
  if ((g_cur_command == COMMAND_ABORT) && !g_rec_auto_finalize) {
    lcd_title_P(S_OPERATION_ABORTED);
  } else {
    lcd_title_P(S_OPERATION_COMPLETE);
  }

  // prevent leakage of g_cur_command, the standard mechanism is to use get_cur_command() which would clear the global
  g_cur_command = COMMAND_IDLE;

  // end of load UI indicator
  lcd_busy_spinner();
  // deactivate pull-up
  TAPE_WRITE_PORT &= ~_BV(TAPE_WRITE_PIN);

  REC_LED_OFF();
}

/*
int free_ram() {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
*/

#ifndef eeprom_update_byte
#define eeprom_update_byte(loc, val) \
do \
{ \
if((uint8_t)(val) != eeprom_read_byte((loc))) \
{ eeprom_write_byte((loc), (val)); } \
} while(0)
#endif 

void load_eeprom_data() {
  if (eeprom_read_byte((uint8_t *) 0) == 0xB6) {
    g_machine_type = eeprom_read_byte((uint8_t *) 1);
    g_video_mode = eeprom_read_byte((uint8_t *) 2);
    
    g_ticker_rate = eeprom_read_byte((uint8_t *) 3);
    g_ticker_hold_rate = eeprom_read_byte((uint8_t *) 4);
    // g_key_repeat_start = eeprom_read_byte((uint8_t *) 5);
    g_key_repeat_next = eeprom_read_byte((uint8_t *) 6);
    g_rec_finalize_time = eeprom_read_byte((uint8_t *) 7);
    g_rec_auto_finalize = eeprom_read_byte((uint8_t *) 8);
  }
}

void save_eeprom_data() {
  eeprom_update_byte((uint8_t *) 0, 0xB6);

  eeprom_update_byte((uint8_t *) 1, g_machine_type);
  eeprom_update_byte((uint8_t *) 2, g_video_mode);
  eeprom_update_byte((uint8_t *) 3, g_ticker_rate);
  eeprom_update_byte((uint8_t *) 4, g_ticker_hold_rate);
  // eeprom_update_byte((uint8_t *) 5, g_key_repeat_start);
  eeprom_update_byte((uint8_t *) 6, g_key_repeat_next);
  eeprom_update_byte((uint8_t *) 7, g_rec_finalize_time);
  eeprom_update_byte((uint8_t *) 8, g_rec_auto_finalize);
}

int tapuino_hardware_setup(void)
{
  FRESULT res;
  uint8_t tmp;
  
  load_eeprom_data();
  
  // enable TWI pullups
  TWI_PORT |= _BV(TWI_PIN_SDA);
  TWI_PORT |= _BV(TWI_PIN_SCL);
    
  // sense is output to C64 but it works in an open-collector fashion, so no need to set DDR, just make sure it is off
  SENSE_OFF();
  
  // read is output to C64
  TAPE_READ_DDR |= _BV(TAPE_READ_PIN);
  // start with tape read LED off
  TAPE_READ_LOW();
  
  // write is input from C64, activate pullups
  TAPE_WRITE_DDR &= ~_BV(TAPE_WRITE_PIN);
  // no pull-up for now, activate pull-up just before write section, write LED off
  TAPE_WRITE_PORT &= ~_BV(TAPE_WRITE_PIN);
  
  // motor is input from C64, activate pullups
  MOTOR_DDR &= ~_BV(MOTOR_PIN);
  MOTOR_PORT |= _BV(MOTOR_PIN);
  
  // Control pins are output
  CONTROL_DDR |= _BV(CONTROL_PIN0) | _BV(CONTROL_PIN1);
  // default both LOW so BUS 0
  CONTROL_SET_BUS0();
  
  // recording led (Arduino: D2, Atmel: PD2)
  REC_LED_DDR |= _BV(REC_LED_PIN);
  REC_LED_OFF();
  
  // keys are all inputs, activate pullups
  KEYS_READ_DDR &= ~_BV(KEY_SELECT_PIN);
  KEYS_READ_PORT |= _BV(KEY_SELECT_PIN);

  KEYS_READ_DDR &= ~_BV(KEY_ABORT_PIN);
  KEYS_READ_PORT |= _BV(KEY_ABORT_PIN);

  KEYS_READ_DDR &= ~_BV(KEY_PREV_PIN);
  KEYS_READ_PORT |= _BV(KEY_PREV_PIN);

  KEYS_READ_DDR &= ~_BV(KEY_NEXT_PIN);
  KEYS_READ_PORT |= _BV(KEY_NEXT_PIN);
  
  disk_timer_setup();
  
//  serial_init();
//  serial_println_P(S_INIT);
//  sprintf((char*)g_fat_buffer, "%d", free_ram());
//  serial_println((char*)g_fat_buffer);
  lcd_setup();
//  serial_println_P(S_INITI2COK);
  lcd_title_P(S_INIT);
  sprintf_P((char*) g_fat_buffer, S_VERSION_PATTERN, TAPUINO_MAJOR_VERSION, TAPUINO_MINOR_VERSION, TAPUINO_BUILD_VERSION);
  lcd_status((char*) g_fat_buffer);
  _delay_ms(2000);
  
  
  // something (possibly) dodgy in the bootloader causes a fail on cold boot.
  // retrying here seems to fix it (could just be the bootloader on my cheap Chinese clone?)
  for (tmp = 0; tmp < 10; tmp++) {
    res = f_mount(&g_fs, "", 1);
    _delay_ms(200);
    if (res == FR_OK) break;
  }
  
  if (res == FR_OK) {
    SPI_Speed_Fast();
    // attempt to open the recording dir
    strcpy_P((char*)g_fat_buffer, S_DEFAULT_RECORD_DIR);
    res = f_opendir(&g_dir, (char*)g_fat_buffer);
    if (res != FR_OK) { // try to make it if its not there
      res = f_mkdir((char*)g_fat_buffer);
      if (res != FR_OK || f_opendir(&g_dir, (char*)g_fat_buffer) != FR_OK) {
        lcd_title_P(S_INIT_FAILED);
        lcd_status_P(S_MKDIR_FAILED);
        lcd_busy_spinner();
        return 0;
      }
    }
    res = f_opendir(&g_dir, "/");
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
  
  main_menu(&file_info);
}
