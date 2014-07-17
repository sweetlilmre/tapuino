#include <avr/io.h>
#include <inttypes.h>
#include "config.h"
#include "comms.h"
#include "serial.h"

#define RECV_WAIT_CMD       0
#define RECV_WAIT_DATA      1
#define MAX_CHECKS 20

uint8_t g_debounced_state;
uint8_t g_key_state[MAX_CHECKS] = {0};
uint8_t g_debounce_index = 0;
uint8_t g_recvState = RECV_WAIT_CMD;
uint8_t g_pending_command = COMMAND_IDLE;
uint8_t g_cur_command = COMMAND_IDLE;
uint8_t g_last_command = COMMAND_IDLE;

void debounce_switches() {
  uint8_t i,j;
  g_key_state[g_debounce_index] = KEYS_READ_PINS;
  ++g_debounce_index;
  j = 0xFF;
  for (i = 0; i < MAX_CHECKS; i++) {
    j = j & g_key_state[i];
  }
  g_debounced_state = j;
  if (g_debounce_index >= MAX_CHECKS) {
    g_debounce_index = 0;
  }
}
void player_handleInputKeys() {
  uint8_t tmp_command = COMMAND_IDLE;
  debounce_switches();
  
  if (g_debounced_state & _BV(KEY_SELECT_PIN)) {
    tmp_command = COMMAND_SELECT;
  }
  if (g_debounced_state & _BV(KEY_ABORT_PIN)) {
    tmp_command = COMMAND_ABORT;
  }
  if (g_debounced_state & _BV(KEY_PREV_PIN)) {
    tmp_command = COMMAND_PREVIOUS;
  }
  if (g_debounced_state & _BV(KEY_NEXT_PIN)) {
    tmp_command = COMMAND_NEXT;
  }
  if (tmp_command != g_last_command) {
    g_cur_command = tmp_command;
    g_last_command = tmp_command;
  }
}

void player_handleInput(char ch) {
  if (ch >= 'a' && ch <= 'z') {
    ch = (ch - 'a') + 'A';
  }
  
  switch(g_recvState) {
    case RECV_WAIT_CMD:
    {
      switch(ch) 
      {
        case 'S':
        {
          g_pending_command = COMMAND_SELECT;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'N':
        {
          g_pending_command = COMMAND_NEXT;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'P':
        {
          g_pending_command = COMMAND_PREVIOUS;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'A':
        {
          g_pending_command = COMMAND_ABORT;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
      }
      break;
    }
    
    case RECV_WAIT_DATA:
    {
      if (ch != '\n') {
        if (ch == '\r') {
          // ignore carriage return
          return;
        } else {
          // invalid command
          g_recvState = RECV_WAIT_CMD;
        }
      } else {
        g_cur_command = g_pending_command;
        g_recvState = RECV_WAIT_CMD;
      }
      break;
    }
  }
}

void input_callback()
{
  if (serial_available() > 0) {
    char ch = serial_read();
    player_handleInput(ch);
  }
  player_handleInputKeys();
}

