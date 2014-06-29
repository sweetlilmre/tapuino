#ifndef _COMMS_H
#define _COMMS_H


#define COMMAND_IDLE        0
#define COMMAND_SELECT      1
#define COMMAND_NEXT        2
#define COMMAND_PREVIOUS    3


#define CMD_ARG_MAX 12

BYTE player_handleInput(char ch);
unsigned char inputCallback();
void responseCallback(char* msg);

extern BYTE g_curCommand;
extern BYTE g_exitFlag;

#endif
