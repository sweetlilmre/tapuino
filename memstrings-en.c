#include <avr/pgmspace.h>
#include "config.h"
#include "memstrings.h"

#ifdef TAPUINO_LANGUAGE_EN

const char S_DEFAULT_RECORD_DIR[] PROGMEM =  "/recorded";
const char S_RECORDING[] PROGMEM = "Recording";
const char S_SELECT_RECORD_MODE[] PROGMEM = "Name mode:";
const char S_REC_MODE_MANUAL[] PROGMEM = "Manual";
const char S_REC_MODE_AUTO[] PROGMEM = "Auto";
const char S_ENTER_FILENAME[] PROGMEM = "Enter name";

const char S_INIT[] PROGMEM = "Init...";
const char S_INIT_FAILED[] PROGMEM = "Init failed!";
const char S_INIT_OK[] PROGMEM = "Init OK.";

const char S_NO_FILES_FOUND[] PROGMEM = "No files found!";
const char S_SELECT_FILE[] PROGMEM = "Select file:";
const char S_SELECT_MODE[] PROGMEM = "Select mode:";
const char S_MODE_PLAY[] PROGMEM = "Play";
const char S_MODE_RECORD[] PROGMEM = "Record";
const char S_READY_RECORD[] PROGMEM = "Ready...";
const char S_PRESS_START[] PROGMEM = "Press START";

const char S_MODE_OPTIONS[] PROGMEM = "Options";
const char S_OPTION_SIGNAL[] PROGMEM = "Invert";
const char S_OPTION_KEY_REPEAT[] PROGMEM = "Key speed";
const char S_OPTION_TICKER_SPEED[] PROGMEM = "Ticker speed";
const char S_OPTION_TICKER_HOLD[] PROGMEM = "Ticker hold";
const char S_OPTION_REC_FINALIZE[] PROGMEM = "Record finalize";
const char S_OPTION_REC_AUTO_FINALIZE[] PROGMEM = "Auto finalize";

const char S_MKDIR_FAILED[] PROGMEM = "MKDIR fail!";
const char S_CHDIR_FAILED[] PROGMEM = "CHDIR fail!";
const char S_READ_FAILED[] PROGMEM = "READ fail!";
const char S_OPEN_FAILED[] PROGMEM = "OPEN fail!";
const char S_INVALID_TAP[] PROGMEM = "Invalid TAP!";
const char S_INVALID_SIZE[] PROGMEM = "Invalid size!";

const char S_LOADING[] PROGMEM = "Loading:";
const char S_OPERATION_COMPLETE[] PROGMEM = "Complete!";
const char S_OPERATION_ABORTED[] PROGMEM = "Aborted!";

const char S_OPTION_MACHINE_TYPE[] PROGMEM = "Machine";
const char S_OPTION_VIDEO_MODE[] PROGMEM = "Video";

#endif
