#include <avr/pgmspace.h>
#include "memstrings.h"

prog_char S_INIT[] PROGMEM = "Initialising";
prog_char S_INIT_FAILED[] PROGMEM = "Init failed!";
prog_char S_INIT_OK[] PROGMEM = "Init OK.";
prog_char S_NO_FILES_FOUND[] PROGMEM = "No files found!";
prog_char S_SELECT_FILE[] PROGMEM = "Select file:";
prog_char S_READ_FAILED[] PROGMEM = "Read failed";
prog_char S_OPEN_FAILED[] PROGMEM = "Open failed";
prog_char S_INVALID_TAP[] PROGMEM = "Invalid TAP file!";
prog_char S_LOADING[] PROGMEM = "Loading...";
prog_char S_LOADING_COMPLETE[] PROGMEM = "Loading complete!";
prog_char S_LOADING_ABORTED[] PROGMEM = "Loading aborted!";
prog_char S_DIRECTORY_ERROR[] PROGMEM = "CHDIR ERROR!";
prog_char S_MAX_BLANK_LINE[] PROGMEM = "                    ";
prog_char S_TAP_MAGIC_C64[] PROGMEM = "C64-TAPE-RAW";
prog_char S_UP_A_DIR[] PROGMEM = "..";
