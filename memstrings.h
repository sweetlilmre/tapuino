#ifndef _MEMSTRINGS_H
#define _MEMSTRINGS_H

#include <avr/pgmspace.h>

prog_char S_INIT[] PROGMEM = "Initialising";
prog_char S_INITFAILED[] PROGMEM = "Init failed!";
prog_char S_INITOK[] PROGMEM = "Init OK.";
prog_char S_NOFILES[] PROGMEM = "No files found!";
prog_char S_SELECTFILE[] PROGMEM = "Select file:";
prog_char S_READFAILED[] PROGMEM = "Read failed";
prog_char S_OPENFAILED[] PROGMEM = "Open failed";
prog_char S_INVALIDTAP[] PROGMEM = "Invalid TAP file!";
prog_char S_LOADING[] PROGMEM = "Loading...";
prog_char S_LOADCOMPLETE[] PROGMEM = "Loading complete!";
prog_char S_CHANGEDIRERROR[] PROGMEM = "CHDIR ERROR!";

#endif
