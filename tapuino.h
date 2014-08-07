#ifndef TAPUINO_H
#define TAPUINO_H

#include "ff.h"

void tapuino_run();
void record_file(char* pfile_name);
int play_file(FILINFO* pfile_info);
uint32_t get_timer_tick();

#endif
