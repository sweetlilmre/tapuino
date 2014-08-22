#include <avr/pgmspace.h>
#include "memstrings.h"

prog_char S_DEFAULT_RECORD_DIR[] PROGMEM =  "/recorded";
prog_char S_NAME_PATTERN[] PROGMEM = "rec-%.4d.tap";
prog_char S_RECORDING[] PROGMEM = "Recording";
prog_char S_SELECT_RECORD_MODE[] PROGMEM = "Name mode:";
prog_char S_REC_MODE_MANUAL[] PROGMEM = "Manual";
prog_char S_REC_MODE_AUTO[] PROGMEM = "Auto";
prog_char S_ENTER_FILENAME[] PROGMEM = "Enter name";
prog_char S_FILENAME_CHARS[] PROGMEM = " abcdefghijklmnopqrstuvwxyz0123456789_-";

prog_char S_INIT[] PROGMEM = "Init...";
prog_char S_INIT_FAILED[] PROGMEM = "Init failed!";
prog_char S_INIT_OK[] PROGMEM = "Init OK.";

prog_char S_NO_FILES_FOUND[] PROGMEM = "No files found!";
prog_char S_SELECT_FILE[] PROGMEM = "Select file:";
prog_char S_SELECT_MODE[] PROGMEM = "Select mode:";
prog_char S_MODE_PLAY[] PROGMEM = "Play";
prog_char S_MODE_RECORD[] PROGMEM = "Record";
prog_char S_READY_RECORD[] PROGMEM = "Ready...";
prog_char S_PRESS_START[] PROGMEM = "Press START";

prog_char S_MODE_OPTIONS[] PROGMEM = "Options";
prog_char S_OPTION_SIGNAL[] PROGMEM = "Invert";
prog_char S_OPTION_KEY_REPEAT[] PROGMEM = "Key speed";
prog_char S_OPTION_TICKER_SPEED[] PROGMEM = "Ticker speed";
prog_char S_OPTION_TICKER_HOLD[] PROGMEM = "Ticker hold";
prog_char S_OPTION_REC_FINALIZE[] PROGMEM = "Record finalize";

prog_char S_MKDIR_FAILED[] PROGMEM = "MKDIR fail!";
prog_char S_CHDIR_FAILED[] PROGMEM = "CHDIR fail!";
prog_char S_READ_FAILED[] PROGMEM = "READ fail!";
prog_char S_OPEN_FAILED[] PROGMEM = "OPEN fail!";
prog_char S_INVALID_TAP[] PROGMEM = "Invalid TAP!";

prog_char S_LOADING[] PROGMEM = "Loading:";
prog_char S_OPERATION_COMPLETE[] PROGMEM = "Complete!";
prog_char S_OPERATION_ABORTED[] PROGMEM = "Aborted!";

prog_char S_MAX_BLANK_LINE[] PROGMEM = "                    ";
prog_char S_TAP_MAGIC_C64[] PROGMEM = "C64-TAPE-RAW";
