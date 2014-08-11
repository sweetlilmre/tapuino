#ifndef _COMMS_H
#define _COMMS_H


#define COMMAND_IDLE        0
#define COMMAND_SELECT      1
#define COMMAND_SELECT_LONG 2
#define COMMAND_ABORT       3
#define COMMAND_ABORT_LONG  4
#define COMMAND_NEXT        5
#define COMMAND_PREVIOUS    6

void input_callback();
extern volatile uint8_t g_cur_command;

#endif
