#include <avr/pgmspace.h>
#include "config.h"
#include "memstrings.h"

#ifdef TAPUINO_LANGUAGE_IT

const char S_DEFAULT_RECORD_DIR[] PROGMEM =  "/salvataggi";
const char S_RECORDING[] PROGMEM = "Recording";
const char S_SELECT_RECORD_MODE[] PROGMEM = "Nome file:";
const char S_REC_MODE_MANUAL[] PROGMEM = "Manuale";
const char S_REC_MODE_AUTO[] PROGMEM = "Automatico";
const char S_ENTER_FILENAME[] PROGMEM = "Inserisci nome";

const char S_INIT[] PROGMEM = "Avvio in corso..";
const char S_INIT_FAILED[] PROGMEM = "Avvio fallito!";
const char S_INIT_OK[] PROGMEM = "Avvio OK!";

const char S_NO_FILES_FOUND[] PROGMEM = "Nessun file!";
const char S_SELECT_FILE[] PROGMEM = "Scegli file:";
const char S_SELECT_MODE[] PROGMEM = "Scegli modo:";
const char S_MODE_PLAY[] PROGMEM = "Play";
const char S_MODE_RECORD[] PROGMEM = "Record";
const char S_READY_RECORD[] PROGMEM = "Pronto...";
const char S_PRESS_START[] PROGMEM = "Premi START";

const char S_MODE_OPTIONS[] PROGMEM = "Opzioni";
const char S_OPTION_SIGNAL[] PROGMEM = "Inverti";
const char S_OPTION_KEY_REPEAT[] PROGMEM = "Velocita' scorr.";
const char S_OPTION_TICKER_SPEED[] PROGMEM = "Velocita' scroll";
const char S_OPTION_TICKER_HOLD[] PROGMEM = "Pausa scroll";
const char S_OPTION_REC_FINALIZE[] PROGMEM = "Pausa a fine REC";
const char S_OPTION_REC_AUTO_FINALIZE[] PROGMEM = "Auto finalizza";

const char S_MKDIR_FAILED[] PROGMEM = "Errore MKDIR!";
const char S_CHDIR_FAILED[] PROGMEM = "Errore CHDIR!";
const char S_READ_FAILED[] PROGMEM = "READ fallito!";
const char S_OPEN_FAILED[] PROGMEM = "OPEN fallito!";
const char S_INVALID_TAP[] PROGMEM = "TAP Invalido!";
const char S_INVALID_SIZE[] PROGMEM = "Dimens. invalida";

const char S_LOADING[] PROGMEM = "Loading:";
const char S_OPERATION_COMPLETE[] PROGMEM = "Completato!";
const char S_OPERATION_ABORTED[] PROGMEM = "Annullato!";

const char S_OPTION_MACHINE_TYPE[] PROGMEM = "Macchina";
const char S_OPTION_VIDEO_MODE[] PROGMEM = "Video";

#endif
