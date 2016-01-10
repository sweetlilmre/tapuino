#ifndef ROTARYENCODER_H
#define ROTARYENCODER_H

#define DIR_NONE 0x0
#define DIR_CW 0x10
#define DIR_CCW 0x20

void encoder_setup();
uint8_t encoder_process();

#endif
