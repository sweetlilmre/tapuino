#include <avr/pgmspace.h>
#include "config.h"
#include "memstrings.h"

#ifdef TAPUINO_LANGUAGE_ES

const char S_DEFAULT_RECORD_DIR[] PROGMEM =  "/grabado";
const char S_RECORDING[] PROGMEM = "Grabando";
const char S_SELECT_RECORD_MODE[] PROGMEM = "Modo:";
const char S_REC_MODE_MANUAL[] PROGMEM = "Manual";
const char S_REC_MODE_AUTO[] PROGMEM = "Auto";
const char S_ENTER_FILENAME[] PROGMEM = "Teclear nombre";

const char S_INIT[] PROGMEM = "Inicializando...";
const char S_INIT_FAILED[] PROGMEM = "No Inicializado!";
const char S_INIT_OK[] PROGMEM = "Inicializado!";

const char S_NO_FILES_FOUND[] PROGMEM = "No hay ficheros!";
const char S_SELECT_FILE[] PROGMEM = "Escoger fichero:";
const char S_SELECT_MODE[] PROGMEM = "Modo:";
const char S_MODE_PLAY[] PROGMEM = "Reproducir";
const char S_MODE_RECORD[] PROGMEM = "Grabar";
const char S_READY_RECORD[] PROGMEM = "Preparado!...";
const char S_PRESS_START[] PROGMEM = "Pulsar START";

const char S_MODE_OPTIONS[] PROGMEM = "Opciones";
const char S_OPTION_SIGNAL[] PROGMEM = "Invertir";
const char S_OPTION_KEY_REPEAT[] PROGMEM = "Vel. Tecla";
const char S_OPTION_TICKER_SPEED[] PROGMEM = "Vel. Ticker";
const char S_OPTION_TICKER_HOLD[] PROGMEM = "Sos. Ticker";
const char S_OPTION_REC_FINALIZE[] PROGMEM = "Fin Grabacion";
const char S_OPTION_REC_AUTO_FINALIZE[] PROGMEM = "Auto-Finalizar";

const char S_MKDIR_FAILED[] PROGMEM = "Fallo en MKDIR!";
const char S_CHDIR_FAILED[] PROGMEM = "Fallo en CHDIR!";
const char S_READ_FAILED[] PROGMEM = "Fallo en READ!";
const char S_OPEN_FAILED[] PROGMEM = "Fallo en OPEN!";
const char S_INVALID_TAP[] PROGMEM = "TAP invalido!";
const char S_INVALID_SIZE[] PROGMEM = "Tamaño Invalido!";

const char S_LOADING[] PROGMEM = "Cargando:";
const char S_OPERATION_COMPLETE[] PROGMEM = "Completado!";
const char S_OPERATION_ABORTED[] PROGMEM = "Cancelado!";

const char S_OPTION_MACHINE_TYPE[] PROGMEM = "Tipo de maquina";
const char S_OPTION_VIDEO_MODE[] PROGMEM = "Video";

#endif
