#ifndef TAPUINO_H
#define TAPUINO_H

#include "ff.h"

void tapuino_run();
void record_file(char* pfile_name);
int play_file(FILINFO* pfile_info);
uint32_t get_timer_tick();
void save_eeprom_data();

extern volatile uint8_t g_invert_signal;
extern volatile uint16_t g_ticker_rate;
extern volatile uint16_t g_ticker_hold_rate;
extern volatile uint16_t g_key_repeat_next;
extern volatile uint16_t g_rec_finalize_time;
extern volatile uint8_t g_rec_auto_finalize;
extern uint8_t g_machine_type;
extern uint8_t g_video_mode;
extern volatile uint8_t g_is_paused;

#endif
