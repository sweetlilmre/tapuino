#include <avr/pgmspace.h>
#include "memstrings.h"

const char S_NAME_PATTERN[] PROGMEM = "rec-%.4d.tap";
const char S_VERSION_PATTERN[] PROGMEM = "V: %d.%d.%d";
const char S_FILENAME_CHARS[] PROGMEM = " abcdefghijklmnopqrstuvwxyz0123456789_-";

const char S_C64[] PROGMEM = "C64";
const char S_C16[] PROGMEM = "C16";
const char S_VIC[] PROGMEM = "VIC";
const char S_PAL[] PROGMEM = "PAL";
const char S_NTSC[] PROGMEM = "NTSC";
