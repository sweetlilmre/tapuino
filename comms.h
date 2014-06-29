#ifndef _COMMS_H
#define _COMMS_H


#define COMMAND_IDLE        0
#define COMMAND_SELECT      1
#define COMMAND_NEXT        2
#define COMMAND_PREVIOUS    3
#define COMMAND_ABORT       4

unsigned char input_callback();

extern BYTE g_curCommand;
extern BYTE g_exitFlag;

#endif
