#include <avr/pgmspace.h>
#include "config.h"
#include "memstrings.h"

#ifdef TAPUINO_LANGUAGE_TR

const char S_DEFAULT_RECORD_DIR[] PROGMEM =  "/kaydedilen";
const char S_RECORDING[] PROGMEM = "Kaydediyor";
const char S_SELECT_RECORD_MODE[] PROGMEM = "Isim modu:";
const char S_REC_MODE_MANUAL[] PROGMEM = "Manuel";
const char S_REC_MODE_AUTO[] PROGMEM = "Otomatik";
const char S_ENTER_FILENAME[] PROGMEM = "Isim girin";

const char S_INIT[] PROGMEM = "Basliyor...";
const char S_INIT_FAILED[] PROGMEM = "Baslama hatasi!";
const char S_INIT_OK[] PROGMEM = "Baslama TAMAM.";

const char S_NO_FILES_FOUND[] PROGMEM = "Hic dosya yok!";
const char S_SELECT_FILE[] PROGMEM = "Dosya sec:";
const char S_SELECT_MODE[] PROGMEM = "Mod sec:";
const char S_MODE_PLAY[] PROGMEM = "Oynat";
const char S_MODE_RECORD[] PROGMEM = "Kaydet";
const char S_READY_RECORD[] PROGMEM = "Hazir...";
const char S_PRESS_START[] PROGMEM = "BASLA tusuna bas";

const char S_MODE_OPTIONS[] PROGMEM = "Secenekler";
const char S_OPTION_SIGNAL[] PROGMEM = "Ters cevir";
const char S_OPTION_KEY_REPEAT[] PROGMEM = "Tus hizi";
const char S_OPTION_TICKER_SPEED[] PROGMEM = "Kayma hizi";
const char S_OPTION_TICKER_HOLD[] PROGMEM = "Kayma bekleme";
const char S_OPTION_REC_FINALIZE[] PROGMEM = "Kaydi sonlandir";

const char S_MKDIR_FAILED[] PROGMEM = "MKDIR hatasi!";
const char S_CHDIR_FAILED[] PROGMEM = "CHDIR hatasi!";
const char S_READ_FAILED[] PROGMEM = "OKUMA hatasi!";
const char S_OPEN_FAILED[] PROGMEM = "ACMA hatasi!";
const char S_INVALID_TAP[] PROGMEM = "Gecersiz TAP!";
const char S_INVALID_SIZE[] PROGMEM = "Gecersiz boyut!";

const char S_LOADING[] PROGMEM = "Yukleniyor:";
const char S_OPERATION_COMPLETE[] PROGMEM = "Tamamlandi!";
const char S_OPERATION_ABORTED[] PROGMEM = "Vazgecildi!";

const char S_OPTION_MACHINE_TYPE[] PROGMEM = "Makine";
const char S_OPTION_VIDEO_MODE[] PROGMEM = "Video";

#endif
