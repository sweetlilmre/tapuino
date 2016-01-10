#include <inttypes.h>
#include <avr/io.h>
#include "config.h"
#include "rotaryencoder.h"

uint8_t state;

#define R_START     0x0
#define R_CW_BEGIN  0x1
#define R_CW_NEXT   0x2
#define R_CW_FINAL  0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_NEXT  0x5
#define R_CCW_FINAL 0x6

uint8_t ttable[7][4] = {
  {R_START, R_CCW_BEGIN, R_CW_BEGIN,  R_START},
  {R_START, R_START,     R_CW_BEGIN,  R_CW_NEXT},
  {R_START, R_CW_FINAL,  R_START,     R_CW_NEXT},
  {R_START, R_CW_FINAL,  R_START,     R_START},
  {R_START, R_CCW_BEGIN, R_START,     R_CCW_NEXT},
  {R_START, R_START,     R_CCW_FINAL, R_CCW_NEXT},
  {R_START, R_START,     R_CCW_FINAL, R_START},
};

void encoder_setup() {
  ttable[R_CW_FINAL][R_START] |= DIR_CW;
  ttable[R_CCW_FINAL][R_START] |= DIR_CCW;
  // Initialise state.
  state = R_START;
}

uint8_t encoder_process() {
  // Grab state of input pins.
  uint8_t tmp = KEYS_READ_PINS;
  uint8_t pinstate = 0;
  // pins are pull-up so look for pull down
  if (!(tmp & _BV(KEY_PREV_PIN))) pinstate |= 2;
  if (!(tmp & _BV(KEY_NEXT_PIN))) pinstate |= 1;
  
  // Determine new state from the pins and state table.
  state = ttable[state & 0xf][pinstate];
  // Return emit bits, ie the generated event.
  return state & 0x30;
}

