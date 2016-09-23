#include <avr/pgmspace.h>
#include "config.h"
#include "memstrings.h"

#ifdef TAPUINO_LANGUAGE_DE

const char S_DEFAULT_RECORD_DIR[] PROGMEM =  "/aufnahmen";
const char S_RECORDING[] PROGMEM = "Aufnahme";
const char S_SELECT_RECORD_MODE[] PROGMEM = "Aufnahmemodus:";
const char S_REC_MODE_MANUAL[] PROGMEM = "Manuell";
const char S_REC_MODE_AUTO[] PROGMEM = "Automatisch";
const char S_ENTER_FILENAME[] PROGMEM = "Dateiname:";

const char S_INIT[] PROGMEM = "Starten...";
const char S_INIT_FAILED[] PROGMEM = "Fehler";
const char S_INIT_OK[] PROGMEM = "Start OK";

const char S_NO_FILES_FOUND[] PROGMEM = "Keine Dateien";
const char S_SELECT_FILE[] PROGMEM = "Datei waehlen:";
const char S_SELECT_MODE[] PROGMEM = "Modus waehlen:";
const char S_MODE_PLAY[] PROGMEM = "Laden";
const char S_MODE_RECORD[] PROGMEM = "Aufnehmen";
const char S_READY_RECORD[] PROGMEM = "Bereit...";
const char S_PRESS_START[] PROGMEM = "Druecke START";

const char S_MODE_OPTIONS[] PROGMEM = "Optionen";
const char S_OPTION_SIGNAL[] PROGMEM = "Invertieren";
const char S_OPTION_KEY_REPEAT[] PROGMEM = "Tastenwiederhlng";
const char S_OPTION_TICKER_SPEED[] PROGMEM = "Ticker-Geschw.";
const char S_OPTION_TICKER_HOLD[] PROGMEM = "Ticker-Pause";
const char S_OPTION_REC_FINALIZE[] PROGMEM = "Finalisieren";
const char S_OPTION_REC_AUTO_FINALIZE[] PROGMEM = "Automatisch fin.";

const char S_MKDIR_FAILED[] PROGMEM = "MKDIR Fehler";
const char S_CHDIR_FAILED[] PROGMEM = "CHDIR Fehler";
const char S_READ_FAILED[] PROGMEM = "READ Fehler";
const char S_OPEN_FAILED[] PROGMEM = "OPEN Fehler";
const char S_INVALID_TAP[] PROGMEM = "Ungueltiges TAP";
const char S_INVALID_SIZE[] PROGMEM = "Falsche Groesse";

const char S_LOADING[] PROGMEM = "Laden...";
const char S_OPERATION_COMPLETE[] PROGMEM = "Vollstaendig";
const char S_OPERATION_ABORTED[] PROGMEM = "Abbruch...";

const char S_OPTION_MACHINE_TYPE[] PROGMEM = "Computer-Typ";
const char S_OPTION_VIDEO_MODE[] PROGMEM = "Video-Standard";

#endif
